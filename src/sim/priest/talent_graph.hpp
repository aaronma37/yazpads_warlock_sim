#pragma once
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
