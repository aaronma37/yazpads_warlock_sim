#!/usr/bin/env python3
"""
Scans the simulator C++ codebase and talent definitions to extract
only the exact icons and backgrounds used by the UI into assets_web/.
Reduces the asset footprint from 905 MB down to ~2-4 MB for WebAssembly.
"""

import os
import re
import json
import shutil

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(ROOT_DIR, "src")
ASSETS_DIR = os.path.join(ROOT_DIR, "assets")
OUT_DIR = os.path.join(ROOT_DIR, "assets_web")

def find_referenced_assets():
    referenced = set()

    # Pre-index available icon stems from assets/icons
    known_icon_stems = set()
    src_icons_dir = os.path.join(ASSETS_DIR, "icons")
    if os.path.exists(src_icons_dir):
        for f in os.listdir(src_icons_dir):
            stem = os.path.splitext(f)[0].lower()
            known_icon_stems.add(stem)

    # 1. Regex find all string literals containing .png or .jpg
    pattern_ext = re.compile(r'["\']([a-zA-Z0-9_\-]+\.(?:png|jpg))["\']', re.IGNORECASE)
    # 2. Regex find all string literals that match any known icon stem
    pattern_token = re.compile(r'["\']([a-zA-Z0-9_\-]+)["\']')

    for root, _, files in os.walk(SRC_DIR):
        for f in files:
            if f.endswith((".cpp", ".hpp", ".h")):
                with open(os.path.join(root, f), "r", encoding="utf-8", errors="ignore") as fp:
                    content = fp.read()
                    for match in pattern_ext.findall(content):
                        referenced.add(match.strip().lower())
                    for match in pattern_token.findall(content):
                        m_low = match.strip().lower()
                        if m_low in known_icon_stems:
                            referenced.add(m_low + ".png")

    # 3. Warlock talent JSON definitions
    json_path = os.path.join(ROOT_DIR, "warlock_forever_talents.json")
    if os.path.exists(json_path):
        with open(json_path, "r", encoding="utf-8") as fp:
            data = json.load(fp)
            for tree in data.get("trees", []):
                for t in tree.get("talents", []):
                    ic = t.get("icon", "")
                    if ic:
                        if not ic.endswith((".png", ".jpg")):
                            ic = ic + ".png"
                        referenced.add(ic.strip().lower())

    # 4. Priest spell JSON definitions
    priest_json_path = os.path.join(ROOT_DIR, "data", "forever", "priest_spells.json")
    if os.path.exists(priest_json_path):
        with open(priest_json_path, "r", encoding="utf-8") as fp:
            data = json.load(fp)
            for tab in data.get("tabs", []):
                tab_ic = tab.get("icon", "")
                if tab_ic:
                    if not tab_ic.endswith((".png", ".jpg")):
                        tab_ic = tab_ic + ".png"
                    referenced.add(tab_ic.strip().lower())
                for sp in tab.get("spells", []):
                    ic = sp.get("icon", "")
                    if ic:
                        if not ic.endswith((".png", ".jpg")):
                            ic = ic + ".png"
                        referenced.add(ic.strip().lower())

    # 5. Always include core backgrounds
    bgs = [
        "affliction_bg.png", "demonology_bg.png", "destruction_bg.png",
        "discipline_bg.png", "holy_bg.png", "shadow_bg.png",
        "affliction_bg.jpg", "demonology_bg.jpg", "destruction_bg.jpg"
    ]
    for bg in bgs:
        referenced.add(bg.lower())

    return referenced

def build_web_assets():
    referenced = find_referenced_assets()
    print(f"Discovered {len(referenced)} asset filenames referenced in code/talents.")

    # Target folders
    out_icons = os.path.join(OUT_DIR, "icons")
    out_bgs = os.path.join(OUT_DIR, "backgrounds")
    os.makedirs(out_icons, exist_ok=True)
    os.makedirs(out_bgs, exist_ok=True)

    copied = 0
    missing = []

    # Map available assets
    avail_icons = {}
    src_icons = os.path.join(ASSETS_DIR, "icons")
    if os.path.exists(src_icons):
        for f in os.listdir(src_icons):
            avail_icons[f.lower()] = os.path.join(src_icons, f)
            avail_icons[os.path.splitext(f)[0].lower()] = os.path.join(src_icons, f)

    src_bgs = os.path.join(ASSETS_DIR, "backgrounds")
    avail_bgs = {}
    if os.path.exists(src_bgs):
        for f in os.listdir(src_bgs):
            avail_bgs[f.lower()] = os.path.join(src_bgs, f)
            avail_bgs[os.path.splitext(f)[0].lower()] = os.path.join(src_bgs, f)

    # Copy files
    for ref in sorted(referenced):
        stem = os.path.splitext(ref)[0].lower()
        matched_src = None
        target_dest = None

        if "bg" in ref or "background" in ref:
            matched_src = avail_bgs.get(ref) or avail_bgs.get(stem)
            if matched_src:
                target_dest = os.path.join(out_bgs, os.path.basename(matched_src))

        if not matched_src:
            matched_src = avail_icons.get(ref) or avail_icons.get(stem)
            if matched_src:
                target_dest = os.path.join(out_icons, os.path.basename(matched_src))

        if matched_src and target_dest:
            shutil.copy2(matched_src, target_dest)
            copied += 1
        else:
            missing.append(ref)

    # Also copy all files from assets/backgrounds unconditionally
    if os.path.exists(src_bgs):
        for f in os.listdir(src_bgs):
            shutil.copy2(os.path.join(src_bgs, f), os.path.join(out_bgs, f))

    # Vendored Blizzard UI chrome (buttons, dialog frame, tooltip pieces).
    # The desktop build resolves these from assets/wow_classic via
    # AssetManager; the web build needs them under assets_web/wow_classic.
    src_chrome = os.path.join(ASSETS_DIR, "wow_classic")
    out_chrome = os.path.join(OUT_DIR, "wow_classic")
    if os.path.exists(src_chrome):
        shutil.copytree(src_chrome, out_chrome, dirs_exist_ok=True)
        copied += sum(len(files) for _, _, files in os.walk(out_chrome))

    # Calculate total size
    total_bytes = 0
    for root, _, files in os.walk(OUT_DIR):
        for f in files:
            total_bytes += os.path.getsize(os.path.join(root, f))

    print(f"Successfully copied {copied} assets to {OUT_DIR}")
    print(f"Total web asset bundle size: {total_bytes / (1024 * 1024):.2f} MB")
    if missing:
        print(f"Note: {len(missing)} references were not found in local assets/icons (will fallback to default placeholder texture if loaded):")
        for m in missing[:8]:
            print(f"  - {m}")

if __name__ == "__main__":
    build_web_assets()
