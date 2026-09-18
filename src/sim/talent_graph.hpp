#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include "talents.hpp"
#include "mechanics.hpp"
#include "des_engine.hpp"

namespace warlock {

constexpr size_t AFFLICTION_NODE_COUNT = 17;
constexpr size_t DEMONOLOGY_NODE_COUNT = 19;
constexpr size_t DESTRUCTION_NODE_COUNT = 16;
constexpr size_t TOTAL_TALENT_NODES = AFFLICTION_NODE_COUNT + DEMONOLOGY_NODE_COUNT + DESTRUCTION_NODE_COUNT; // 52

struct TalentGraphNode {
    size_t global_idx;
    int tree_idx; // 0 = Affliction, 1 = Demonology, 2 = Destruction
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

    // Convert Talents struct to flat array of 52 ranks
    std::array<int, TOTAL_TALENT_NODES> to_vector(const Talents& t) const {
        std::array<int, TOTAL_TALENT_NODES> v{};
        for (size_t i = 0; i < AFFLICTION_NODE_COUNT; ++i) {
            v[i] = t.aff.get_points_by_index(i);
        }
        for (size_t i = 0; i < DEMONOLOGY_NODE_COUNT; ++i) {
            v[AFFLICTION_NODE_COUNT + i] = t.demo.get_points_by_index(i);
        }
        for (size_t i = 0; i < DESTRUCTION_NODE_COUNT; ++i) {
            v[AFFLICTION_NODE_COUNT + DEMONOLOGY_NODE_COUNT + i] = t.destro.get_points_by_index(i);
        }
        return v;
    }

    // Convert flat array of 52 ranks back to Talents struct
    Talents to_talents(const std::array<int, TOTAL_TALENT_NODES>& v) const {
        Talents t;
        for (size_t i = 0; i < AFFLICTION_NODE_COUNT; ++i) {
            t.aff.get_points_by_index(i) = v[i];
        }
        for (size_t i = 0; i < DEMONOLOGY_NODE_COUNT; ++i) {
            t.demo.get_points_by_index(i) = v[AFFLICTION_NODE_COUNT + i];
        }
        for (size_t i = 0; i < DESTRUCTION_NODE_COUNT; ++i) {
            t.destro.get_points_by_index(i) = v[AFFLICTION_NODE_COUNT + DEMONOLOGY_NODE_COUNT + i];
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

    // Check if the point allocation is 100% valid under WoW talent rules
    bool is_valid(const std::array<int, TOTAL_TALENT_NODES>& v, int target_total_points = 51) const {
        if (count_total_points(v) != target_total_points) return false;

        // Check node bounds and prerequisites
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            if (v[i] < 0 || v[i] > nodes_[i].max_points) return false;
            if (v[i] > 0 && nodes_[i].req_global_idx >= 0) {
                size_t req_idx = static_cast<size_t>(nodes_[i].req_global_idx);
                if (v[req_idx] < nodes_[req_idx].max_points) return false;
            }
        }

        // Check row requirements per tree
        for (int tree = 0; tree < 3; ++tree) {
            std::array<int, 8> points_below_row{}; // points_below_row[r] is sum of points in rows 1..(r-1)
            int running = 0;
            for (int r = 1; r <= 7; ++r) {
                points_below_row[r] = running;
                for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
                    if (nodes_[i].tree_idx == tree && nodes_[i].row == r) {
                        running += v[i];
                    }
                }
            }

            for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
                if (nodes_[i].tree_idx == tree && v[i] > 0) {
                    int required = 5 * (nodes_[i].row - 1);
                    if (points_below_row[nodes_[i].row] < required) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    // Returns a list of indices where 1 point can be safely removed
    std::vector<size_t> get_valid_donors(const std::array<int, TOTAL_TALENT_NODES>& v) const {
        std::vector<size_t> donors;
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            if (v[i] <= 0) continue;

            // Check if any active talent requires node i as a prerequisite
            bool has_dependent = false;
            for (size_t j = 0; j < TOTAL_TALENT_NODES; ++j) {
                if (v[j] > 0 && nodes_[j].req_global_idx == static_cast<int>(i)) {
                    has_dependent = true;
                    break;
                }
            }
            if (has_dependent) continue;

            // Check if removing a point violates row requirements for any higher-row talents in the same tree
            int tree = nodes_[i].tree_idx;
            int donor_row = nodes_[i].row;

            // Test if test_v remains valid for all nodes in this tree
            std::array<int, 8> points_below_row{};
            int running = 0;
            for (int r = 1; r <= 7; ++r) {
                points_below_row[r] = running;
                for (size_t k = 0; k < TOTAL_TALENT_NODES; ++k) {
                    if (nodes_[k].tree_idx == tree && nodes_[k].row == r) {
                        int count = v[k] - (k == i ? 1 : 0);
                        running += count;
                    }
                }
            }

            bool row_valid = true;
            for (size_t k = 0; k < TOTAL_TALENT_NODES; ++k) {
                if (nodes_[k].tree_idx == tree) {
                    int points_in_k = v[k] - (k == i ? 1 : 0);
                    if (points_in_k > 0) {
                        int required = 5 * (nodes_[k].row - 1);
                        if (points_below_row[nodes_[k].row] < required) {
                            row_valid = false;
                            break;
                        }
                    }
                }
            }

            if (row_valid) {
                donors.push_back(i);
            }
        }
        return donors;
    }

    // Returns a list of indices where 1 point can be safely added
    std::vector<size_t> get_valid_receivers(const std::array<int, TOTAL_TALENT_NODES>& v) const {
        std::vector<size_t> receivers;
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            if (v[i] >= nodes_[i].max_points) continue;

            // Check prerequisite
            if (nodes_[i].req_global_idx >= 0) {
                size_t req_idx = static_cast<size_t>(nodes_[i].req_global_idx);
                if (v[req_idx] < nodes_[req_idx].max_points) continue;
            }

            // Check row requirement
            int tree = nodes_[i].tree_idx;
            int required_points = 5 * (nodes_[i].row - 1);
            int current_tree_points = count_tree_points(v, tree);
            if (current_tree_points < required_points) continue;

            receivers.push_back(i);
        }
        return receivers;
    }

    // Deterministically repair an arbitrary vector into a strictly valid 51-point vector
    void repair(std::array<int, TOTAL_TALENT_NODES>& v, FastRNG& rng, int target_total_points = 51) const {
        // Clamp ranks
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            v[i] = std::clamp(v[i], 0, nodes_[i].max_points);
        }

        // Top-down validation: clear illegal points whose prerequisites or row limits are broken
        for (int tree = 0; tree < 3; ++tree) {
            int running = 0;
            for (int r = 1; r <= 7; ++r) {
                int required = 5 * (r - 1);
                for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
                    if (nodes_[i].tree_idx == tree && nodes_[i].row == r) {
                        if (running < required || (nodes_[i].req_global_idx >= 0 && v[nodes_[i].req_global_idx] < nodes_[nodes_[i].req_global_idx].max_points)) {
                            v[i] = 0; // Invalid point
                        }
                    }
                }
                for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
                    if (nodes_[i].tree_idx == tree && nodes_[i].row == r) {
                        running += v[i];
                    }
                }
            }
        }

        // Adjust total points to target_total_points
        int current = count_total_points(v);
        while (current > target_total_points) {
            auto donors = get_valid_donors(v);
            if (donors.empty()) {
                // Reset to clean 0 and rebuild if locked
                v.fill(0);
                current = 0;
                break;
            }
            size_t pick = donors[rng.next_u64() % donors.size()];
            v[pick]--;
            current--;
        }

        while (current < target_total_points) {
            auto receivers = get_valid_receivers(v);
            if (receivers.empty()) break;
            size_t pick = receivers[rng.next_u64() % receivers.size()];
            v[pick]++;
            current++;
        }
    }

    // Generates a fully randomized valid 51-point build
    std::array<int, TOTAL_TALENT_NODES> generate_random_valid(FastRNG& rng, int target_total_points = 51) const {
        std::array<int, TOTAL_TALENT_NODES> v{};
        for (int p = 0; p < target_total_points; ++p) {
            auto receivers = get_valid_receivers(v);
            if (receivers.empty()) break;
            size_t pick = receivers[rng.next_u64() % receivers.size()];
            v[pick]++;
        }
        return v;
    }

private:
    TalentGraph() {
        // Affliction nodes (0..16)
        for (size_t i = 0; i < AFFLICTION_NODE_COUNT; ++i) {
            const auto& def = FOREVER_AFFLICTION_NODES[i];
            TalentGraphNode n;
            n.global_idx = i;
            n.tree_idx = 0;
            n.tree_node_idx = static_cast<int>(i);
            n.id = def.id;
            n.name = def.name;
            n.row = def.row;
            n.col = def.col;
            n.max_points = def.max_points;
            n.req_global_idx = -1;
            nodes_[i] = n;
        }

        // Demonology nodes (17..35)
        for (size_t i = 0; i < DEMONOLOGY_NODE_COUNT; ++i) {
            const auto& def = FOREVER_DEMONOLOGY_NODES[i];
            size_t g_idx = AFFLICTION_NODE_COUNT + i;
            TalentGraphNode n;
            n.global_idx = g_idx;
            n.tree_idx = 1;
            n.tree_node_idx = static_cast<int>(i);
            n.id = def.id;
            n.name = def.name;
            n.row = def.row;
            n.col = def.col;
            n.max_points = def.max_points;
            n.req_global_idx = -1;
            nodes_[g_idx] = n;
        }

        // Destruction nodes (36..51)
        for (size_t i = 0; i < DESTRUCTION_NODE_COUNT; ++i) {
            const auto& def = FOREVER_DESTRUCTION_NODES[i];
            size_t g_idx = AFFLICTION_NODE_COUNT + DEMONOLOGY_NODE_COUNT + i;
            TalentGraphNode n;
            n.global_idx = g_idx;
            n.tree_idx = 2;
            n.tree_node_idx = static_cast<int>(i);
            n.id = def.id;
            n.name = def.name;
            n.row = def.row;
            n.col = def.col;
            n.max_points = def.max_points;
            n.req_global_idx = -1;
            nodes_[g_idx] = n;
        }

        // Link prerequisites by name
        auto find_global_by_name = [&](const char* name) -> int {
            if (!name) return -1;
            for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
                if (std::string(nodes_[i].name) == name) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        };

        for (size_t i = 0; i < AFFLICTION_NODE_COUNT; ++i) {
            if (FOREVER_AFFLICTION_NODES[i].req) {
                nodes_[i].req_global_idx = find_global_by_name(FOREVER_AFFLICTION_NODES[i].req);
            }
        }
        for (size_t i = 0; i < DEMONOLOGY_NODE_COUNT; ++i) {
            if (FOREVER_DEMONOLOGY_NODES[i].req) {
                nodes_[AFFLICTION_NODE_COUNT + i].req_global_idx = find_global_by_name(FOREVER_DEMONOLOGY_NODES[i].req);
            }
        }
        for (size_t i = 0; i < DESTRUCTION_NODE_COUNT; ++i) {
            if (FOREVER_DESTRUCTION_NODES[i].req) {
                nodes_[AFFLICTION_NODE_COUNT + DEMONOLOGY_NODE_COUNT + i].req_global_idx = find_global_by_name(FOREVER_DESTRUCTION_NODES[i].req);
            }
        }
    }

    std::array<TalentGraphNode, TOTAL_TALENT_NODES> nodes_;
};

} // namespace warlock
