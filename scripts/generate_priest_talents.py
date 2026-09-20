import json
import re

def to_snake(name):
    return re.sub(r'[^a-zA-Z0-9]+', '_', name).strip('_').lower()

def escape_str(s):
    if not s:
        return ""
    return s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')

with open('scratch_hyjal_talents.json') as f:
    raw = json.load(f)

trees = [
    ("Discipline", "disc", raw[208:226]),
    ("Holy", "holy", raw[226:243]),
    ("Shadow", "shadow", raw[243:261])
]

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
        max_p = t["max"]
        row = t["row"]
        col = t["col"]
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

# Talents struct
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
        // Standard Shadow (13/0/38)
        t.disc.twin_disciplines = 5;
        t.disc.inner_focus = 1;
        t.disc.meditation = 3;
        t.disc.mental_agility = 3;
        t.disc.improved_power_word_shield = 1;

        t.shadow.shadow_magic = 5;
        t.shadow.spirit_tap = 3;
        t.shadow.improved_shadow_word_pain = 2;
        t.shadow.shadow_reach = 2;
        t.shadow.improved_mind_blast = 5;
        t.shadow.mind_flay = 1;
        t.shadow.improved_mind_flay = 2;
        t.shadow.vampiric_embrace = 1;
        t.shadow.shadow_weaving = 3;
        t.shadow.devouring_contagion = 2;
        t.shadow.early_demise = 2;
        t.shadow.darkness = 5;
        t.shadow.shadowform = 1;
        return t;
    }

    static Talents create_forever_smite() {
        Talents t;
        // Smite / Holy DPS (14/37/0)
        t.disc.discipline = 5;
        t.disc.twin_disciplines = 5;
        t.disc.inner_focus = 1;
        t.disc.meditation = 3;

        t.holy.holy = 3;
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

# Node definitions
for tname, short_name, talents in trees:
    out += f"inline const std::array<TalentNodeDef, {len(talents)}>& get_{short_name}_nodes() {{\n"
    out += f"    static const std::array<TalentNodeDef, {len(talents)}> nodes = {{\n"
    for t in talents:
        var = to_snake(t["name"])
        name = escape_str(t["name"])
        icon = escape_str(t.get("icon", ""))
        req_val = t.get("req")
        req = f'"{escape_str(req_val)}"' if req_val else "nullptr"
        max_p = t["max"]
        row = t["row"]
        col = t["col"]
        descs = []
        for r in range(1, 6):
            if r <= max_p and str(r) in t.get("desc", {}):
                dtext = escape_str(t["desc"][str(r)])
                descs.append(f'"{dtext}"')
            else:
                descs.append("nullptr")
        desc_str = ", ".join(descs)
        out += f'        TalentNodeDef{{"{var}", "{name}", {row}, {col}, {max_p}, "{icon}", {req}, {{{desc_str}}}}},\n'
    out += "    };\n    return nodes;\n}\n\n"

out += "} // namespace priest\n"

with open("src/sim/priest/talents.hpp", "w") as f:
    f.write(out)
print("Generated src/sim/priest/talents.hpp successfully.")

# Also generate talent_graph.hpp
graph_out = """#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include "talents.hpp"

namespace priest {

constexpr size_t DISCIPLINE_NODE_COUNT = 18;
constexpr size_t HOLY_NODE_COUNT = 17;
constexpr size_t SHADOW_NODE_COUNT = 18;
constexpr size_t TOTAL_TALENT_NODES = DISCIPLINE_NODE_COUNT + HOLY_NODE_COUNT + SHADOW_NODE_COUNT; // 53

struct TalentGraphNode {
    size_t global_idx;
    int tree_idx; // 0 = Discipline, 1 = Holy, 2 = Shadow
    int tree_node_idx;
    const char* id;
    const char* name;
    int row; // 1 to 7
    int col; // 1 to 4
    int max_points;
    int req_global_idx; // -1 if no prerequisite
};

class TalentGraph {
public:
    static const TalentGraph& get() {
        static TalentGraph instance;
        return instance;
    }

    const std::array<TalentGraphNode, TOTAL_TALENT_NODES>& nodes() const {
        return nodes_;
    }

    const TalentGraphNode& node(size_t idx) const {
        return nodes_[idx];
    }

    std::array<int, TOTAL_TALENT_NODES> to_vector(const Talents& t) const {
        std::array<int, TOTAL_TALENT_NODES> v{};
        for (size_t i = 0; i < DISCIPLINE_NODE_COUNT; ++i) {
            v[i] = t.disc.get_points_by_index(i);
        }
        for (size_t i = 0; i < HOLY_NODE_COUNT; ++i) {
            v[DISCIPLINE_NODE_COUNT + i] = t.holy.get_points_by_index(i);
        }
        for (size_t i = 0; i < SHADOW_NODE_COUNT; ++i) {
            v[DISCIPLINE_NODE_COUNT + HOLY_NODE_COUNT + i] = t.shadow.get_points_by_index(i);
        }
        return v;
    }

    Talents to_talents(const std::array<int, TOTAL_TALENT_NODES>& v) const {
        Talents t;
        for (size_t i = 0; i < DISCIPLINE_NODE_COUNT; ++i) {
            t.disc.get_points_by_index(i) = v[i];
        }
        for (size_t i = 0; i < HOLY_NODE_COUNT; ++i) {
            t.holy.get_points_by_index(i) = v[DISCIPLINE_NODE_COUNT + i];
        }
        for (size_t i = 0; i < SHADOW_NODE_COUNT; ++i) {
            t.shadow.get_points_by_index(i) = v[DISCIPLINE_NODE_COUNT + HOLY_NODE_COUNT + i];
        }
        return t;
    }

    int count_tree_points(const std::array<int, TOTAL_TALENT_NODES>& v, int tree_idx) const {
        int sum = 0;
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            if (nodes_[i].tree_idx == tree_idx) {
                sum += v[i];
            }
        }
        return sum;
    }

    int count_total_points(const std::array<int, TOTAL_TALENT_NODES>& v) const {
        int sum = 0;
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            sum += v[i];
        }
        return sum;
    }

    bool is_valid(const std::array<int, TOTAL_TALENT_NODES>& v, int target_total_points = 51) const {
        if (count_total_points(v) != target_total_points) return false;

        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            if (v[i] < 0 || v[i] > nodes_[i].max_points) return false;
            if (v[i] > 0 && nodes_[i].req_global_idx >= 0) {
                int req_idx = nodes_[i].req_global_idx;
                if (v[req_idx] < nodes_[req_idx].max_points) return false;
            }
        }

        for (int tree = 0; tree < 3; ++tree) {
            for (int r = 1; r <= 7; ++r) {
                int pts_in_or_above_r = 0;
                int pts_below_r = 0;
                for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
                    if (nodes_[i].tree_idx == tree) {
                        if (nodes_[i].row >= r && v[i] > 0) pts_in_or_above_r += v[i];
                        if (nodes_[i].row < r) pts_below_r += v[i];
                    }
                }
                if (pts_in_or_above_r > 0) {
                    int req_pts = (r - 1) * 5;
                    if (pts_below_r < req_pts) return false;
                }
            }
        }
        return true;
    }

private:
    std::array<TalentGraphNode, TOTAL_TALENT_NODES> nodes_;

    TalentGraph() {
        size_t g_idx = 0;
        const auto& d_nodes = get_disc_nodes();
        for (size_t i = 0; i < DISCIPLINE_NODE_COUNT; ++i) {
            nodes_[g_idx] = {g_idx, 0, static_cast<int>(i), d_nodes[i].id, d_nodes[i].name, d_nodes[i].row, d_nodes[i].col, d_nodes[i].max_points, -1};
            g_idx++;
        }
        const auto& h_nodes = get_holy_nodes();
        for (size_t i = 0; i < HOLY_NODE_COUNT; ++i) {
            nodes_[g_idx] = {g_idx, 1, static_cast<int>(i), h_nodes[i].id, h_nodes[i].name, h_nodes[i].row, h_nodes[i].col, h_nodes[i].max_points, -1};
            g_idx++;
        }
        const auto& s_nodes = get_shadow_nodes();
        for (size_t i = 0; i < SHADOW_NODE_COUNT; ++i) {
            nodes_[g_idx] = {g_idx, 2, static_cast<int>(i), s_nodes[i].id, s_nodes[i].name, s_nodes[i].row, s_nodes[i].col, s_nodes[i].max_points, -1};
            g_idx++;
        }

        // Connect prerequisites
        auto find_global_by_name = [&](const char* name) -> int {
            for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
                if (std::string(nodes_[i].name) == name) return static_cast<int>(i);
            }
            return -1;
        };

        for (size_t i = 0; i < DISCIPLINE_NODE_COUNT; ++i) {
            if (d_nodes[i].req) nodes_[i].req_global_idx = find_global_by_name(d_nodes[i].req);
        }
        for (size_t i = 0; i < HOLY_NODE_COUNT; ++i) {
            if (h_nodes[i].req) nodes_[DISCIPLINE_NODE_COUNT + i].req_global_idx = find_global_by_name(h_nodes[i].req);
        }
        for (size_t i = 0; i < SHADOW_NODE_COUNT; ++i) {
            if (s_nodes[i].req) nodes_[DISCIPLINE_NODE_COUNT + HOLY_NODE_COUNT + i].req_global_idx = find_global_by_name(s_nodes[i].req);
        }
    }
};

} // namespace priest
"""

with open("src/sim/priest/talent_graph.hpp", "w") as f:
    f.write(graph_out)
print("Generated src/sim/priest/talent_graph.hpp successfully.")
