import json
import re

def to_snake(name):
    return re.sub(r'[^a-zA-Z0-9]+', '_', name).strip('_').lower()

def clean_tooltip(desc):
    clean = desc.replace("<br />", "\\n").replace("<!--bc-->", "").replace("<!--sp1225132:0-->", "").replace("<!--sp1225132-->", "").replace("<!--sp14898:0-->", "").replace("<!--sp14898-->", "").replace("<!--sp1225139:0-->", "").replace("<!--sp1225139-->", "")
    clean = clean.replace('\\', '\\\\').replace('"', '\\"')
    return clean

with open("data/forever/priest_talents.json") as f:
    data = json.load(f)

# ID lookup
id_to_talent = {}
for tree_name, talents in data["trees"].items():
    for t in talents:
        id_to_talent[t["id"]] = t

trees = [
    ("Discipline", "disc", data["trees"]["Discipline"]),
    ("Holy", "holy", data["trees"]["Holy"]),
    ("Shadow", "shadow", data["trees"]["Shadow"])
]

# 1. Generate talents.hpp
out = """#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

namespace priest {

struct TalentNodeDef {
    const char* id;
    const char* name;
    int row;
    int col;
    int max_points;
    const char* icon;
    const char* req;
    const char* desc[5];
};

"""

for tname, short_name, talents in trees:
    struct_name = f"{tname}Talents"
    out += f"struct {struct_name} {{\n"
    for t in talents:
        var = to_snake(t["name"])
        max_p = len(t["descriptions"])
        row = t["row"] + 1
        col = t["col"] + 1
        out += f"    int {var} = 0; // max: {max_p}, R{row}C{col}\n"
    
    out += "\n    int total_points() const {\n        return "
    out += " + \n               ".join([to_snake(t["name"]) for t in talents])
    out += ";\n    }\n"
    
    out += "\n    int& get_points_by_index(size_t idx) {\n        switch (idx) {\n"
    for i, t in enumerate(talents):
        var = to_snake(t["name"])
        out += f"            case {i}: return {var};\n"
    out += f"            default: return {to_snake(talents[0]['name'])};\n"
    out += "        }\n    }\n"
    
    out += f"\n    int get_points_by_index(size_t idx) const {{\n        switch (idx) {{\n"
    for i, t in enumerate(talents):
        var = to_snake(t["name"])
        out += f"            case {i}: return {var};\n"
    out += f"            default: return 0;\n"
    out += "        }\n    }\n"
    out += "};\n\n"

# Talents struct with presets
out += """struct Talents {
    DisciplineTalents disc;
    HolyTalents holy;
    ShadowTalents shadow;

    int total_points() const {
        return disc.total_points() + holy.total_points() + shadow.total_points();
    }

    // Canonical builds
    static Talents create_forever_shadow() {
        Talents t;
        // Standard Shadow (14/0/37) - 51 points
        t.disc.twin_disciplines = 5;
        t.disc.silent_resolve = 2;
        t.disc.improved_power_word_shield = 3;
        t.disc.inner_focus = 1;
        t.disc.meditation = 3;

        t.shadow.shadow_focus = 5;
        t.shadow.spirit_tap = 5;
        t.shadow.improved_shadow_word_pain = 2;
        t.shadow.shadow_reach = 2;
        t.shadow.improved_mind_blast = 5;
        t.shadow.mind_flay = 1;
        t.shadow.improved_mind_flay = 2;
        t.shadow.vampiric_embrace = 1;
        t.shadow.shadow_weaving = 3;
        t.shadow.silence = 1;
        t.shadow.devouring_contagion = 2;
        t.shadow.early_demise = 2;
        t.shadow.darkness = 5;
        t.shadow.shadowform = 1;
        return t;
    }

    static Talents create_forever_smite() {
        Talents t;
        // Smite / Holy DPS (14/37/0) - 51 points
        t.disc.power_in_light = 5;
        t.disc.twin_disciplines = 5;
        t.disc.inner_focus = 1;
        t.disc.meditation = 3;

        t.holy.twilight_focus = 3;
        t.holy.holy_specialization = 5;
        t.holy.divine_fury = 5;
        t.holy.holy_reach = 2;
        t.holy.searing_light = 2;
        t.holy.spiritual_guidance = 5;
        t.holy.holy_nova = 1;
        t.holy.improved_renew = 3;
        t.holy.improved_healing = 3;
        t.holy.spiritual_healing = 3;
        t.holy.prayer_of_mending = 1;
        t.holy.spirit_of_redemption = 1;
        t.holy.litany_of_light = 2;
        t.holy.blessed_recovery = 1;
        return t;
    }
};

"""

# Node definition arrays
for tname, short_name, talents in trees:
    func_name = f"get_{short_name}_nodes"
    count = len(talents)
    out += f"inline const std::array<TalentNodeDef, {count}>& {func_name}() {{\n"
    out += f"    static const std::array<TalentNodeDef, {count}> nodes = {{\n"
    for t in talents:
        var = to_snake(t["name"])
        name = t["name"]
        row = t["row"] + 1
        col = t["col"] + 1
        max_p = len(t["descriptions"])
        icon = t["icon"]
        req = t.get("requires", [])
        req_str = f'"{id_to_talent[req[0]["id"]]["name"]}"' if req else "nullptr"
        
        desc_list = []
        for r_num in range(1, 6):
            if str(r_num) in t["descriptions"]:
                cleaned = clean_tooltip(t["descriptions"][str(r_num)])
                desc_list.append(f'"{cleaned}"')
            else:
                desc_list.append("nullptr")
        desc_str = ", ".join(desc_list)
        
        out += f'        TalentNodeDef{{"{var}", "{name}", {row}, {col}, {max_p}, "{icon}", {req_str}, {{{desc_str}}}}},\n'
    out += "    };\n    return nodes;\n}\n\n"

out += "} // namespace priest\n"

with open("src/sim/priest/talents.hpp", "w") as f:
    f.write(out)
print("Updated src/sim/priest/talents.hpp successfully!")

# 2. Update talent_graph.hpp
graph_code = """#pragma once
#include <vector>
#include <string>
#include <array>
#include <algorithm>
#include <numeric>
#include "talents.hpp"

namespace priest {

struct TalentGraphNode {
    int id = 0; // 0..52
    std::string name;
    int tree = 0; // 0 = Disc, 1 = Holy, 2 = Shadow
    int row = 0;  // 1..7
    int col = 0;  // 1..4
    int max_rank = 1;
    int req_points_in_tree = 0;
    int prereq_id = -1;
};

class TalentGraph {
public:
    static const TalentGraph& get() {
        static TalentGraph instance;
        return instance;
    }

    const std::vector<TalentGraphNode>& nodes() const { return nodes_; }

    std::vector<int> to_vector(const Talents& t) const {
        std::vector<int> v(nodes_.size(), 0);
        // Disc 0..17
        for (size_t i = 0; i < 18; ++i) v[i] = t.disc.get_points_by_index(i);
        // Holy 18..34
        for (size_t i = 0; i < 17; ++i) v[18 + i] = t.holy.get_points_by_index(i);
        // Shadow 35..52
        for (size_t i = 0; i < 18; ++i) v[35 + i] = t.shadow.get_points_by_index(i);
        return v;
    }

    Talents to_talents(const std::vector<int>& v) const {
        Talents t;
        for (size_t i = 0; i < 18 && i < v.size(); ++i) t.disc.get_points_by_index(i) = v[i];
        for (size_t i = 0; i < 17 && (18 + i) < v.size(); ++i) t.holy.get_points_by_index(i) = v[18 + i];
        for (size_t i = 0; i < 18 && (35 + i) < v.size(); ++i) t.shadow.get_points_by_index(i) = v[35 + i];
        return t;
    }

    bool is_valid(const std::vector<int>& v, int max_pts = 51) const {
        int total = 0;
        int tree_pts[3] = {0, 0, 0};

        for (size_t i = 0; i < nodes_.size() && i < v.size(); ++i) {
            int pts = v[i];
            if (pts < 0 || pts > nodes_[i].max_rank) return false;
            tree_pts[nodes_[i].tree] += pts;
            total += pts;
        }
        if (total > max_pts) return false;

        // Verify tier requirements and prerequisites
        for (size_t i = 0; i < nodes_.size() && i < v.size(); ++i) {
            if (v[i] > 0) {
                const auto& node = nodes_[i];
                int pts_in_lower_tiers = 0;
                for (size_t j = 0; j < nodes_.size(); ++j) {
                    if (nodes_[j].tree == node.tree && nodes_[j].row < node.row) {
                        pts_in_lower_tiers += v[j];
                    }
                }
                if (pts_in_lower_tiers < (node.row - 1) * 5) return false;

                if (node.prereq_id >= 0) {
                    if (v[node.prereq_id] < nodes_[node.prereq_id].max_rank) return false;
                }
            }
        }
        return true;
    }

    int count_total_points(const std::vector<int>& v) const {
        return std::accumulate(v.begin(), v.end(), 0);
    }

private:
    std::vector<TalentGraphNode> nodes_;

    TalentGraph() {
        nodes_.reserve(53);
        int current_id = 0;

        auto add_tree = [&](int tree_idx, const auto& defs) {
            int start_idx = current_id;
            for (const auto& d : defs) {
                TalentGraphNode n;
                n.id = current_id++;
                n.name = d.name;
                n.tree = tree_idx;
                n.row = d.row;
                n.col = d.col;
                n.max_rank = d.max_points;
                n.req_points_in_tree = (d.row - 1) * 5;
                n.prereq_id = -1;
                nodes_.push_back(n);
            }
            // Link prereqs
            for (size_t i = 0; i < defs.size(); ++i) {
                if (defs[i].req) {
                    for (size_t j = 0; j < defs.size(); ++j) {
                        if (std::string(defs[j].name) == defs[i].req) {
                            nodes_[start_idx + i].prereq_id = start_idx + static_cast<int>(j);
                            break;
                        }
                    }
                }
            }
        };

        add_tree(0, get_disc_nodes());
        add_tree(1, get_holy_nodes());
        add_tree(2, get_shadow_nodes());
    }
};

} // namespace priest
"""

with open("src/sim/priest/talent_graph.hpp", "w") as f:
    f.write(graph_code)
print("Updated src/sim/priest/talent_graph.hpp successfully!")
