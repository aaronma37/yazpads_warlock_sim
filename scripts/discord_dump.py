#!/usr/bin/env python3
"""
Discord Channel, Forum & Thread Data Scraper
Extracts messages, pins, and forum posts/threads into a target directory as human-readable text and structured data.
"""

import argparse
import datetime
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Any, Dict, List, Optional, Set

API_BASE = "https://discord.com/api/v10"
USER_AGENT = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"

# Discord Channel Types
CHANNEL_TYPE_GUILD_TEXT = 0
CHANNEL_TYPE_ANNOUNCEMENT_THREAD = 10
CHANNEL_TYPE_PUBLIC_THREAD = 11
CHANNEL_TYPE_PRIVATE_THREAD = 12
CHANNEL_TYPE_GUILD_FORUM = 15
CHANNEL_TYPE_GUILD_MEDIA = 16


def sanitize_filename(name: str) -> str:
    """Sanitizes strings for safe filesystem directory/file names."""
    clean = re.sub(r'[\\/*?:"<>|]', "", name)
    clean = clean.strip().replace(" ", "_")
    return clean[:64] if clean else "unnamed"


class DiscordClient:
    def __init__(self, token: str, request_delay: float = 0.4):
        self.token = token.strip().strip("'\"")
        self.request_delay = request_delay

    def _request(self, endpoint: str, params: Optional[Dict[str, Any]] = None) -> Any:
        url = f"{API_BASE}{endpoint}"
        if params:
            url += f"?{urllib.parse.urlencode(params)}"

        headers = {
            "Authorization": self.token,
            "User-Agent": USER_AGENT,
            "Content-Type": "application/json",
        }

        req = urllib.request.Request(url, headers=headers)

        while True:
            try:
                time.sleep(self.request_delay)
                with urllib.request.urlopen(req) as response:
                    return json.loads(response.read().decode("utf-8"))
            except urllib.error.HTTPError as e:
                if e.code == 429:
                    try:
                        err_body = json.loads(e.read().decode("utf-8"))
                        retry_after = float(err_body.get("retry_after", 2.0))
                    except Exception:
                        retry_after = 2.0
                    print(f"[!] Rate limited (429). Backing off for {retry_after:.2f}s...")
                    time.sleep(retry_after + 0.5)
                    continue
                elif e.code == 401:
                    print("[ERROR] 401 Unauthorized: Invalid Discord Auth Token. Please check your DISCORD_AUTH variable.", file=sys.stderr)
                    sys.exit(1)
                elif e.code in (403, 404):
                    return None
                else:
                    print(f"[ERROR] HTTP Error {e.code} on {endpoint}: {e.reason}", file=sys.stderr)
                    raise
            except Exception as e:
                print(f"[ERROR] Connection failed: {e}", file=sys.stderr)
                raise

    def get_channel(self, channel_id: str) -> Optional[Dict[str, Any]]:
        return self._request(f"/channels/{channel_id}")

    def get_pins(self, channel_id: str) -> List[Dict[str, Any]]:
        pins = self._request(f"/channels/{channel_id}/pins")
        return pins if isinstance(pins, list) else []

    def get_forum_threads_via_search(self, channel_id: str) -> List[Dict[str, Any]]:
        """Uses forum threads search endpoint to discover all threads in a forum channel."""
        threads_by_id: Dict[str, Dict[str, Any]] = {}
        offset = 0

        while True:
            params = {
                "sort_by": "last_message_time",
                "sort_order": "desc",
                "limit": 25,
                "offset": offset,
            }
            res = self._request(f"/channels/{channel_id}/threads/search", params=params)
            if not res or "threads" not in res or not res["threads"]:
                break

            batch = res["threads"]
            for t in batch:
                threads_by_id[t["id"]] = t

            if not res.get("has_more", False) or len(batch) < 25:
                break
            offset += len(batch)

        return list(threads_by_id.values())

    def get_guild_active_threads(self, guild_id: str) -> List[Dict[str, Any]]:
        res = self._request(f"/guilds/{guild_id}/threads/active")
        if res and "threads" in res:
            return res["threads"]
        return []

    def get_archived_threads(self, channel_id: str, is_private: bool = False) -> List[Dict[str, Any]]:
        endpoint_type = "private" if is_private else "public"
        all_threads: List[Dict[str, Any]] = []
        before_timestamp = None

        while True:
            params: Dict[str, Any] = {"limit": 100}
            if before_timestamp:
                params["before"] = before_timestamp

            res = self._request(f"/channels/{channel_id}/threads/archived/{endpoint_type}", params=params)
            if not res or "threads" not in res or not res["threads"]:
                break

            batch = res["threads"]
            all_threads.extend(batch)

            if not res.get("has_more", False) or len(batch) < 100:
                break

            oldest_meta = batch[-1].get("thread_metadata", {})
            before_timestamp = oldest_meta.get("archive_timestamp")
            if not before_timestamp:
                break

        return all_threads

    def discover_all_threads(self, channel_info: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Finds all active and archived threads belonging to this channel/forum."""
        channel_id = channel_info["id"]
        guild_id = channel_info.get("guild_id")
        channel_type = channel_info.get("type", 0)
        threads_by_id: Dict[str, Dict[str, Any]] = {}

        # 1. If Forum channel, try threads/search first
        if channel_type in (CHANNEL_TYPE_GUILD_FORUM, CHANNEL_TYPE_GUILD_MEDIA):
            print("[*] Forum channel detected. Searching forum posts index...")
            forum_threads = self.get_forum_threads_via_search(channel_id)
            for t in forum_threads:
                threads_by_id[t["id"]] = t

        # 2. Guild-wide active threads filtered by parent_id
        if guild_id:
            active_guild_threads = self.get_guild_active_threads(guild_id)
            for t in active_guild_threads:
                if t.get("parent_id") == channel_id:
                    threads_by_id[t["id"]] = t

        # 3. Public archived threads
        archived_public = self.get_archived_threads(channel_id, is_private=False)
        for t in archived_public:
            threads_by_id[t["id"]] = t

        # 4. Private archived threads
        archived_private = self.get_archived_threads(channel_id, is_private=True)
        for t in archived_private:
            threads_by_id[t["id"]] = t

        return list(threads_by_id.values())

    def fetch_all_messages(
        self,
        channel_id: str,
        limit_total: Optional[int] = None,
        after_id: Optional[str] = None,
        before_id: Optional[str] = None,
    ) -> List[Dict[str, Any]]:
        """Paginates and fetches messages from a channel or thread."""
        messages: List[Dict[str, Any]] = []
        curr_before = before_id

        while True:
            batch_limit = 100
            if limit_total:
                remaining = limit_total - len(messages)
                if remaining <= 0:
                    break
                batch_limit = min(100, remaining)

            params: Dict[str, Any] = {"limit": batch_limit}
            if after_id:
                params["after"] = after_id
            elif curr_before:
                params["before"] = curr_before

            batch = self._request(f"/channels/{channel_id}/messages", params=params)
            if not batch or not isinstance(batch, list) or len(batch) == 0:
                break

            messages.extend(batch)
            print(f"    Fetched {len(messages)} messages...", end="\r", flush=True)

            if after_id:
                curr_after = batch[0]["id"]
                if len(batch) < batch_limit:
                    break
                after_id = curr_after
            else:
                curr_before = batch[-1]["id"]
                if len(batch) < batch_limit:
                    break

        return sorted(messages, key=lambda m: int(m["id"]))


def format_message_to_text(msg: Dict[str, Any]) -> str:
    """Formats a Discord message JSON dict into readable raw text format."""
    author = msg.get("author", {})
    author_name = author.get("global_name") or author.get("username") or "Unknown"
    author_tag = f"{author.get('username', 'unknown')}#{author.get('discriminator', '0')}"
    user_id = author.get("id", "0")
    timestamp = msg.get("timestamp", "")
    content = msg.get("content", "")
    is_pinned = msg.get("pinned", False)
    msg_id = msg.get("id", "")

    # Reactions
    reactions = msg.get("reactions", [])
    reaction_str = ""
    if reactions:
        rx_parts = []
        for rx in reactions:
            emoji = rx.get("emoji", {}).get("name", "?")
            count = rx.get("count", 1)
            rx_parts.append(f"{emoji}x{count}")
        reaction_str = f" [Reactions: {', '.join(rx_parts)}]"

    # Reply info
    reply_str = ""
    if "referenced_message" in msg and msg["referenced_message"]:
        ref = msg["referenced_message"]
        ref_author = ref.get("author", {}).get("username", "Unknown")
        ref_content = (ref.get("content", "")[:60] + "...") if len(ref.get("content", "")) > 60 else ref.get("content", "")
        reply_str = f"\n  ↳ Replying to @{ref_author}: \"{ref_content}\""

    # Attachments
    attachments = msg.get("attachments", [])
    att_str = ""
    if attachments:
        att_links = [att.get("url", "") for att in attachments if att.get("url")]
        if att_links:
            att_str = "\n  [Attachments]: " + ", ".join(att_links)

    # Embeds
    embeds = msg.get("embeds", [])
    embed_str = ""
    if embeds:
        embed_links = []
        for emb in embeds:
            url = emb.get("url")
            title = emb.get("title", "")
            if url:
                embed_links.append(f"{title} ({url})" if title else url)
            elif title:
                embed_links.append(f"Embed: {title}")
        if embed_links:
            embed_str = "\n  [Embeds]: " + " | ".join(embed_links)

    header = f"[{timestamp}] {author_name} (@{author_tag} / ID:{user_id}) [MsgID: {msg_id}]{' [PINNED]' if is_pinned else ''}{reaction_str}{reply_str}"
    divider = "-" * 80
    return f"{header}\n{content}{att_str}{embed_str}\n{divider}\n"


def parse_channel_id(input_str: str) -> str:
    """Extracts channel ID from full Discord URL or raw string."""
    input_str = input_str.strip()
    match = re.search(r"channels/\d+/(\d+)", input_str)
    if match:
        return match.group(1)
    if input_str.isdigit():
        return input_str
    return input_str


def main():
    parser = argparse.ArgumentParser(description="Scrape Discord channels, forums & threads to raw text files.")
    parser.add_argument(
        "--channel",
        "-c",
        default="1548410962464481280",
        help="Discord Channel/Forum ID or full URL (default: 1548410962464481280)",
    )
    parser.add_argument(
        "--auth",
        "-a",
        default=None,
        help="Discord user auth token (defaults to DISCORD_AUTH environment variable)",
    )
    parser.add_argument(
        "--out-dir",
        "-o",
        default="discord_data",
        help="Output directory (default: ./discord_data)",
    )
    parser.add_argument(
        "--limit",
        "-l",
        type=int,
        default=None,
        help="Limit number of messages to fetch per thread/channel (default: all)",
    )
    parser.add_argument(
        "--pins-only",
        action="store_true",
        help="Only fetch pinned messages",
    )
    parser.add_argument(
        "--no-threads",
        action="store_true",
        help="Do not fetch threads/forum posts",
    )
    parser.add_argument(
        "--save-json",
        action="store_true",
        help="Also save raw JSON files alongside text files",
    )
    args = parser.parse_args()

    token = args.auth or os.environ.get("DISCORD_AUTH")
    if not token:
        print("[ERROR] Discord auth token is required.", file=sys.stderr)
        print("Please pass --auth <TOKEN> or set DISCORD_AUTH in your environment.", file=sys.stderr)
        sys.exit(1)

    channel_id = parse_channel_id(args.channel)
    client = DiscordClient(token=token)

    # 1. Fetch channel metadata
    print(f"[*] Connecting to Discord API for channel {channel_id}...")
    channel_info = client.get_channel(channel_id)
    if not channel_info:
        print(f"[ERROR] Could not retrieve channel information for ID: {channel_id}. Verify permissions or ID.", file=sys.stderr)
        sys.exit(1)

    channel_name = sanitize_filename(channel_info.get("name", channel_id))
    channel_type = channel_info.get("type", 0)
    is_forum = channel_type in (CHANNEL_TYPE_GUILD_FORUM, CHANNEL_TYPE_GUILD_MEDIA)
    is_thread = channel_type in (CHANNEL_TYPE_PUBLIC_THREAD, CHANNEL_TYPE_PRIVATE_THREAD, CHANNEL_TYPE_ANNOUNCEMENT_THREAD)

    # Build tag mapping
    tag_map = {tag["id"]: tag["name"] for tag in channel_info.get("available_tags", [])}

    print(f"[+] Target Channel: #{channel_name} (Type: {'Forum' if is_forum else 'Thread' if is_thread else 'Text Channel'})")

    # Create root directory structure
    target_dir = os.path.join(args.out_dir, f"{channel_id}_{channel_name}")
    os.makedirs(target_dir, exist_ok=True)
    threads_dir = os.path.join(target_dir, "threads")
    os.makedirs(threads_dir, exist_ok=True)

    print(f"[+] Target output folder: {target_dir}")

    # 2. Fetch Pinned Messages (for regular channels)
    if not is_forum:
        print("[*] Checking pinned messages...")
        pins = client.get_pins(channel_id)
        if pins:
            pins_file = os.path.join(target_dir, "PINS.txt")
            with open(pins_file, "w", encoding="utf-8") as f:
                f.write(f"=== PINNED MESSAGES FOR #{channel_name} ({channel_id}) ===\n")
                f.write(f"Generated at: {datetime.datetime.utcnow().isoformat()}Z\n")
                f.write("=" * 80 + "\n\n")
                for pin in sorted(pins, key=lambda m: int(m["id"])):
                    f.write(format_message_to_text(pin) + "\n")
            print(f"[+] Saved {len(pins)} pinned messages to {pins_file}")
            if args.save_json:
                with open(os.path.join(target_dir, "pins.json"), "w", encoding="utf-8") as f:
                    json.dump(pins, f, indent=2)

    if args.pins_only:
        print("[*] Completed pins-only run.")
        return

    # 3. Fetch Main Channel Messages (if not a Forum channel)
    if not is_forum and not is_thread:
        print(f"[*] Fetching message history for #{channel_name}...")
        main_messages = client.fetch_all_messages(channel_id, limit_total=args.limit)
        if main_messages:
            main_file = os.path.join(target_dir, "messages.txt")
            with open(main_file, "w", encoding="utf-8") as f:
                f.write(f"=== MESSAGES FOR #{channel_name} ({channel_id}) ===\n")
                f.write(f"Generated at: {datetime.datetime.utcnow().isoformat()}Z\n")
                f.write(f"Total messages: {len(main_messages)}\n")
                f.write("=" * 80 + "\n\n")
                for msg in main_messages:
                    f.write(format_message_to_text(msg) + "\n")
            print(f"[+] Saved main channel history ({len(main_messages)} messages) to {main_file}")
            if args.save_json:
                with open(os.path.join(target_dir, "messages.json"), "w", encoding="utf-8") as f:
                    json.dump(main_messages, f, indent=2)

    # 4. Discover and fetch forum posts / threads
    if not args.no_threads and not is_thread:
        print("[*] Discovering all forum posts & threads...")
        threads = client.discover_all_threads(channel_info)

        if threads:
            print(f"[+] Discovered {len(threads)} total thread(s)/post(s).")
            # Create an index file of all threads
            index_file = os.path.join(target_dir, "THREADS_INDEX.md")
            with open(index_file, "w", encoding="utf-8") as idx:
                idx.write(f"# Forum Threads Index for #{channel_name}\n\n")
                idx.write(f"Total threads: {len(threads)}\n\n")
                idx.write("| # | Tags | Thread Title | Messages | Archived | File |\n")
                idx.write("| :--- | :--- | :--- | :--- | :--- | :--- |\n")
                for i, t in enumerate(threads, 1):
                    meta = t.get("thread_metadata", {})
                    is_archived = meta.get("archived", False)
                    msg_cnt = t.get("message_count", 0)
                    t_title = t.get("name", t["id"]).replace("|", "-")
                    applied_tags = [tag_map.get(tag_id, tag_id) for tag_id in t.get("applied_tags", [])]
                    tag_str = ", ".join(applied_tags) if applied_tags else "-"
                    filename = f"{t['id']}_{sanitize_filename(t.get('name', t['id']))}.txt"
                    idx.write(f"| {i} | {tag_str} | {t_title} | {msg_cnt} | {'Yes' if is_archived else 'No'} | [`{filename}`](threads/{filename}) |\n")

            # Fetch each thread's messages
            for idx_num, thread in enumerate(threads, 1):
                t_id = thread["id"]
                t_name = sanitize_filename(thread.get("name", t_id))
                applied_tags = [tag_map.get(tag_id, tag_id) for tag_id in thread.get("applied_tags", [])]
                tag_header = f" [Tags: {', '.join(applied_tags)}]" if applied_tags else ""

                print(f"[{idx_num}/{len(threads)}] Fetching: {thread.get('name')} ({t_id}){tag_header}...")
                t_msgs = client.fetch_all_messages(t_id, limit_total=args.limit)
                t_file = os.path.join(threads_dir, f"{t_id}_{t_name}.txt")
                with open(t_file, "w", encoding="utf-8") as f:
                    f.write(f"=== FORUM POST / THREAD: {thread.get('name')} ===\n")
                    f.write(f"Thread ID: {t_id}\n")
                    if applied_tags:
                        f.write(f"Tags: {', '.join(applied_tags)}\n")
                    f.write(f"Generated at: {datetime.datetime.utcnow().isoformat()}Z\n")
                    f.write(f"Total messages: {len(t_msgs)}\n")
                    f.write("=" * 80 + "\n\n")
                    for msg in t_msgs:
                        f.write(format_message_to_text(msg) + "\n")
                if args.save_json:
                    with open(os.path.join(threads_dir, f"{t_id}_{t_name}.json"), "w", encoding="utf-8") as f:
                        json.dump(t_msgs, f, indent=2)
                print(f"    Saved {len(t_msgs)} messages -> {t_file}")
        else:
            print("[-] No threads found.")

    print(f"\n[DONE] Finished dumping channel data to: {target_dir}")
    print(f"[+] Index generated at: {os.path.join(target_dir, 'THREADS_INDEX.md')}")


if __name__ == "__main__":
    main()
