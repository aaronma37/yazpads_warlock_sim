import json
import os

with open("data/forever/priest_talents.json") as f:
    data = json.load(f)

id_to_name = {}
for tree_name, talents in data["trees"].items():
    for t in talents:
        id_to_name[t["id"]] = t["name"]

lines = []
lines.append("# WoW Forever Priest Talents & Tooltips Reference\n")
lines.append("**Source**: [Wowhead Forever Priest Talent Calculator](https://www.wowhead.com/forever/talent-calc/priest)\n")
lines.append("**Version**: WoW Forever 1.60.1\n")
lines.append("This file serves as the permanent reference copy for all 53 Priest talents in WoW Forever, including rows, columns, max ranks, prerequisite dependencies, icons, and tooltip texts.\n")

for tree_name, talents in data["trees"].items():
    lines.append(f"## {tree_name} Tree ({len(talents)} Talents)\n")
    lines.append("| Pos | Talent | Max Ranks | Icon | Prerequisite |")
    lines.append("| :---: | :--- | :---: | :--- | :--- |")
    for t in talents:
        row = t["row"] + 1
        col = t["col"] + 1
        name = t["name"]
        max_r = len(t["descriptions"])
        icon_str = "`" + t["icon"] + "`"
        prereqs = [f"{id_to_name.get(req['id'], str(req['id']))} ({req.get('qty', 1)} pts)" for req in t.get("requires", [])]
        prereq_str = ", ".join(prereqs) if prereqs else "None"
        lines.append(f"| R{row}C{col} | **{name}** | {max_r} | {icon_str} | {prereq_str} |")
    lines.append("\n### Detailed Tooltips\n")
    for t in talents:
        row = t["row"] + 1
        col = t["col"] + 1
        name = t["name"]
        max_r = len(t["descriptions"])
        icon = t["icon"]
        lines.append(f"#### R{row}C{col}: {name} (Max: {max_r})")
        lines.append(f"- **Icon**: `{icon}`")
        if t.get("requires"):
            prereqs = [f"{id_to_name.get(req['id'], str(req['id']))} ({req.get('qty', 1)} pts)" for req in t["requires"]]
            req_str = ", ".join(prereqs)
            lines.append(f"- **Requires**: {req_str}")
        lines.append("- **Ranks**:")
        for r_num, desc in sorted(t["descriptions"].items(), key=lambda x: int(x[0])):
            clean_desc = desc.replace("<br />", " ").replace("<!--bc-->", "").replace("<!--sp1225132:0-->", "").replace("<!--sp1225132-->", "").replace("<!--sp14898:0-->", "").replace("<!--sp14898-->", "").replace("<!--sp1225139:0-->", "").replace("<!--sp1225139-->", "")
            lines.append(f"  - **Rank {r_num}**: {clean_desc}")
        lines.append("")

with open("data/forever/priest_talents.md", "w") as f:
    f.write("\n".join(lines))

print("Saved data/forever/priest_talents.md successfully!")
