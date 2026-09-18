#!/usr/bin/env python3
"""
Wowhead Consumable & Crafted Items Scraper
=========================================
Gathers item metadata, required levels, tooltip descriptions, subclasses,
reagents, and icons for Alchemy (or other professions) from Wowhead Forever.

Usage:
    python3 scripts/gather_wowhead_consumables.py
    python3 scripts/gather_wowhead_consumables.py --skill-id 171 --format all
    python3 scripts/gather_wowhead_consumables.py --output alchemy_consumables.json
"""

import argparse
import concurrent.futures
import csv
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
import xml.etree.ElementTree as ET
from typing import Any, Dict, List, Optional

DEFAULT_HEADERS = {
    "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.5",
    "Sec-Fetch-Dest": "document",
    "Sec-Fetch-Mode": "navigate",
    "Sec-Fetch-Site": "none",
    "Sec-Fetch-User": "?1",
    "Upgrade-Insecure-Requests": "1",
}


def clean_html_tooltip(html_str: str) -> List[str]:
    """Convert raw Wowhead HTML tooltip into clean list of text lines."""
    if not html_str:
        return []

    # Strip HTML comments
    text = re.sub(r"<!--.*?-->", "", html_str)

    # Format table/cell breaks and newline tags
    text = re.sub(r"<table[^>]*>", "\n", text)
    text = re.sub(r"</table>", "\n", text)
    text = re.sub(r"<tr[^>]*>", "\n", text)
    text = re.sub(r"</tr>", "", text)
    text = re.sub(r"<td[^>]*>", " ", text)
    text = re.sub(r"</td>", " ", text)
    text = re.sub(r"<br\s*/?>", "\n", text)
    text = re.sub(r"<div[^>]*>", "\n", text)
    text = re.sub(r"</div>", "", text)

    # Strip remaining HTML tags
    text = re.sub(r"<[^>]+>", "", text)

    # Clean unescaped entities and extra whitespaces
    text = text.replace("&nbsp;", " ").replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">")

    lines = [line.strip() for line in text.splitlines() if line.strip()]
    return lines


def extract_use_description(tooltip_lines: List[str]) -> str:
    """Extract the 'Use: ...' effect description from tooltip lines."""
    use_parts = []
    for line in tooltip_lines:
        if line.startswith("Use:"):
            use_parts.append(line)
        elif "Increases" in line or "Restores" in line or "While applied" in line:
            if not use_parts:
                use_parts.append(line)

    return " ".join(use_parts) if use_parts else ""


def extract_req_level_from_lines(tooltip_lines: List[str]) -> Optional[int]:
    """Extract required level if stated in tooltip lines."""
    for line in tooltip_lines:
        m = re.search(r"Requires Level\s+(\d+)", line, re.IGNORECASE)
        if m:
            return int(m.group(1))
    return None


def fetch_url(url: str, retries: int = 3, timeout: int = 15) -> str:
    """Fetch text content from URL with exponential backoff retries."""
    req = urllib.request.Request(url, headers=DEFAULT_HEADERS)
    for attempt in range(retries):
        try:
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                return resp.read().decode("utf-8", errors="ignore")
        except urllib.error.HTTPError as e:
            if e.code == 429:
                time.sleep(2.0 * (attempt + 1))
            elif attempt == retries - 1:
                raise
        except Exception:
            if attempt == retries - 1:
                raise
            time.sleep(1.0 * (attempt + 1))
    raise RuntimeError(f"Failed to fetch {url} after {retries} retries")


def fetch_crafted_item_list(skill_id: int = 171, env: str = "forever") -> List[Dict[str, Any]]:
    """
    Fetch the list of crafted items for a profession skill ID from Wowhead.
    Default skill 171 = Alchemy.
    """
    url = f"https://www.wowhead.com/{env}/skill={skill_id}"
    print(f"[*] Fetching skill index from: {url}")
    html = fetch_url(url)

    # Locate the 'crafted-items' listview
    idx = html.find("id: 'crafted-items'")
    if idx == -1:
        # Fallback search for any crafted items listview
        idx = html.find("'crafted-items'")
    if idx == -1:
        raise ValueError(f"Could not find 'crafted-items' section on {url}")

    d_idx = html.find("data: [", idx)
    if d_idx == -1:
        raise ValueError(f"Could not find 'data: [' for crafted items on {url}")

    sub = html[d_idx + 6 :]
    cnt = 0
    end_bracket = 0
    for i, ch in enumerate(sub):
        if ch == "[":
            cnt += 1
        elif ch == "]":
            cnt -= 1
            if cnt == 0:
                end_bracket = i + 1
                break

    if end_bracket == 0:
        raise ValueError("Could not parse JSON array bracket bounds for crafted items")

    json_str = sub[:end_bracket]
    items = json.loads(json_str)
    print(f"[+] Found {len(items)} crafted items in listview.")
    return items


def fetch_item_details(item_id: int, base_info: Optional[Dict[str, Any]] = None, env: str = "forever") -> Dict[str, Any]:
    """
    Fetch detailed XML information for a single item ID and parse attributes.
    """
    url = f"https://www.wowhead.com/{env}/item={item_id}&xml"
    raw_xml = fetch_url(url)
    root = ET.fromstring(raw_xml)
    item_elem = root.find("item")
    if item_elem is None:
        raise ValueError(f"XML root does not contain <item> for item {item_id}")

    name = item_elem.findtext("name", default="").strip()
    level_text = item_elem.findtext("level", default="0")
    item_level = int(level_text) if level_text.isdigit() else 0

    quality_elem = item_elem.find("quality")
    quality = quality_elem.text.strip() if quality_elem is not None and quality_elem.text else "Common"
    quality_id = int(quality_elem.attrib.get("id", "1")) if quality_elem is not None else 1

    class_elem = item_elem.find("class")
    item_class = class_elem.text.strip() if class_elem is not None and class_elem.text else "Consumables"

    subclass_elem = item_elem.find("subclass")
    subclass = subclass_elem.text.strip() if subclass_elem is not None and subclass_elem.text else "Other"

    icon = item_elem.findtext("icon", default="").strip()
    html_tooltip = item_elem.findtext("htmlTooltip", default="")
    link = item_elem.findtext("link", default=f"https://www.wowhead.com/{env}/item={item_id}")

    tooltip_lines = clean_html_tooltip(html_tooltip)
    use_description = extract_use_description(tooltip_lines)

    # Required level: check listview base_info, XML json tags, or tooltip text
    req_level = 1
    if base_info and "reqlevel" in base_info:
        req_level = int(base_info["reqlevel"])
    else:
        parsed_req = extract_req_level_from_lines(tooltip_lines)
        if parsed_req is not None:
            req_level = parsed_req

    # Parse recipe reagents if present
    reagents = []
    created_by = item_elem.find("createdBy")
    spell_id = None
    spell_name = ""
    if created_by is not None:
        spell_elem = created_by.find("spell")
        if spell_elem is not None:
            spell_id = int(spell_elem.attrib.get("id", "0"))
            spell_name = spell_elem.attrib.get("name", "")
            for r in spell_elem.findall("reagent"):
                reagents.append({
                    "id": int(r.attrib.get("id", "0")),
                    "name": r.attrib.get("name", ""),
                    "count": int(r.attrib.get("count", "1")),
                    "icon": r.attrib.get("icon", ""),
                    "quality": int(r.attrib.get("quality", "1")),
                })

    return {
        "id": item_id,
        "name": name,
        "required_level": req_level,
        "item_level": item_level,
        "quality": quality,
        "quality_id": quality_id,
        "class": item_class,
        "subclass": subclass,
        "icon": icon,
        "use_description": use_description,
        "tooltip_lines": tooltip_lines,
        "recipe": {
            "spell_id": spell_id,
            "spell_name": spell_name,
            "reagents": reagents,
        } if spell_id else None,
        "link": link,
    }


def gather_all_crafted_items(skill_id: int = 171, env: str = "forever", max_threads: int = 10) -> List[Dict[str, Any]]:
    """Gather full details for all crafted items in parallel."""
    raw_list = fetch_crafted_item_list(skill_id=skill_id, env=env)
    total = len(raw_list)
    results = []

    print(f"[*] Fetching XML tooltips & details for {total} items using {max_threads} worker threads...")

    with concurrent.futures.ThreadPoolExecutor(max_workers=max_threads) as executor:
        future_to_item = {
            executor.submit(fetch_item_details, item["id"], item, env): item for item in raw_list
        }

        completed = 0
        for future in concurrent.futures.as_completed(future_to_item):
            item_stub = future_to_item[future]
            try:
                data = future.result()
                results.append(data)
            except Exception as e:
                print(f"[!] Error fetching item {item_stub.get('id')} ({item_stub.get('name')}): {e}", file=sys.stderr)
            completed += 1
            if completed % 25 == 0 or completed == total:
                print(f"    -> Progress: {completed}/{total} items ({completed * 100 // total}%)")

    # Sort results deterministically by subclass, then required level, then name
    results.sort(key=lambda x: (x.get("subclass", ""), x.get("required_level", 0), x.get("name", "")))
    return results


def export_json(data: List[Dict[str, Any]], filepath: str):
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"[+] Saved JSON to: {filepath}")


def export_csv(data: List[Dict[str, Any]], filepath: str):
    fieldnames = [
        "id",
        "name",
        "required_level",
        "item_level",
        "quality",
        "subclass",
        "icon",
        "use_description",
        "reagents_summary",
        "link",
    ]
    with open(filepath, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for item in data:
            reagents_str = ""
            if item.get("recipe") and item["recipe"].get("reagents"):
                reagents_str = ", ".join(f"{r['count']}x {r['name']}" for r in item["recipe"]["reagents"])

            writer.writerow({
                "id": item["id"],
                "name": item["name"],
                "required_level": item["required_level"],
                "item_level": item["item_level"],
                "quality": item["quality"],
                "subclass": item["subclass"],
                "icon": item["icon"],
                "use_description": item["use_description"],
                "reagents_summary": reagents_str,
                "link": item["link"],
            })
    print(f"[+] Saved CSV to: {filepath}")


def export_markdown(data: List[Dict[str, Any]], filepath: str):
    with open(filepath, "w", encoding="utf-8") as f:
        f.write("# Alchemy Consumables Database (WoW Forever)\n\n")
        f.write(f"Total Items: **{len(data)}**\n\n")

        # Group by subclass
        subclasses = {}
        for item in data:
            sc = item.get("subclass", "Other")
            subclasses.setdefault(sc, []).append(item)

        for sc_name, items in subclasses.items():
            f.write(f"## {sc_name} ({len(items)})\n\n")
            f.write("| Req Lvl | Item Name | iLvl | Quality | Tooltip / Use Effect | Reagents |\n")
            f.write("| :---: | :--- | :---: | :--- | :--- | :--- |\n")
            for item in items:
                reagents_str = ""
                if item.get("recipe") and item["recipe"].get("reagents"):
                    reagents_str = "<br>".join(f"{r['count']}x {r['name']}" for r in item["recipe"]["reagents"])
                link = item["link"]
                name_md = f"[{item['name']}]({link})"
                use_desc = item["use_description"].replace("|", "\\|")
                f.write(f"| {item['required_level']} | {name_md} | {item['item_level']} | {item['quality']} | {use_desc} | {reagents_str} |\n")
            f.write("\n")
    print(f"[+] Saved Markdown to: {filepath}")


def main():
    parser = argparse.ArgumentParser(description="Gather WoW Forever alchemy consumables and crafted items from Wowhead.")
    parser.add_argument("--skill-id", type=int, default=171, help="Profession skill ID (default: 171 for Alchemy)")
    parser.add_argument("--env", type=str, default="forever", help="Wowhead data environment (default: forever)")
    parser.add_argument("-o", "--output", type=str, default="alchemy_consumables.json", help="Output JSON filepath")
    parser.add_argument("--format", choices=["json", "csv", "md", "all"], default="json", help="Export format (default: json)")
    parser.add_argument("--threads", type=int, default=10, help="Concurrent request threads (default: 10)")

    args = parser.parse_args()

    start_time = time.time()
    items = gather_all_crafted_items(skill_id=args.skill_id, env=args.env, max_threads=args.threads)
    elapsed = time.time() - start_time
    print(f"[+] Successfully gathered {len(items)} items in {elapsed:.2f}s")

    base_name = os.path.splitext(args.output)[0]

    if args.format in ("json", "all"):
        export_json(items, f"{base_name}.json")
    if args.format in ("csv", "all"):
        export_csv(items, f"{base_name}.csv")
    if args.format in ("md", "all"):
        export_markdown(items, f"{base_name}.md")


if __name__ == "__main__":
    main()
