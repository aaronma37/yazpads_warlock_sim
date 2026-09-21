#pragma once
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

    int count_tree_points(const std::vector<int>& v, int tree_idx) const {
        int sum = 0;
        for (size_t i = 0; i < nodes_.size() && i < v.size(); ++i) {
            if (nodes_[i].tree == tree_idx) {
                sum += v[i];
            }
        }
        return sum;
    }

    int count_total_points(const std::vector<int>& v) const {
        int sum = 0;
        for (size_t i = 0; i < nodes_.size() && i < v.size(); ++i) {
            sum += v[i];
        }
        return sum;
    }

    bool is_valid(const std::vector<int>& v, int target_total_points = 51) const {
        int total = 0;
        for (size_t i = 0; i < nodes_.size() && i < v.size(); ++i) {
            int pts = v[i];
            if (pts < 0 || pts > nodes_[i].max_rank) return false;
            total += pts;
        }
        if (target_total_points > 0 && total != target_total_points) return false;

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

    std::vector<size_t> get_valid_donors(const std::vector<int>& v, const std::vector<int>& req_talents = {}) const {
        std::vector<size_t> donors;
        for (size_t i = 0; i < nodes_.size() && i < v.size(); ++i) {
            if (v[i] <= 0) continue;

            // Cannot donate required talents
            if (std::find(req_talents.begin(), req_talents.end(), static_cast<int>(i)) != req_talents.end()) {
                continue;
            }

            // Check if any active talent requires node i as a prerequisite
            bool has_dependent = false;
            for (size_t j = 0; j < nodes_.size() && j < v.size(); ++j) {
                if (v[j] > 0 && nodes_[j].prereq_id == static_cast<int>(i)) {
                    has_dependent = true;
                    break;
                }
            }
            if (has_dependent) continue;

            // Check if removing a point violates row requirements for any higher-row talents in the same tree
            int tree = nodes_[i].tree;
            std::array<int, 8> points_below_row{};
            int running = 0;
            for (int r = 1; r <= 7; ++r) {
                points_below_row[r] = running;
                for (size_t k = 0; k < nodes_.size() && k < v.size(); ++k) {
                    if (nodes_[k].tree == tree && nodes_[k].row == r) {
                        int count = v[k] - (k == i ? 1 : 0);
                        running += count;
                    }
                }
            }

            bool row_valid = true;
            for (size_t k = 0; k < nodes_.size() && k < v.size(); ++k) {
                if (nodes_[k].tree == tree) {
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

    std::vector<size_t> get_valid_receivers(const std::vector<int>& v) const {
        std::vector<size_t> receivers;
        for (size_t i = 0; i < nodes_.size() && i < v.size(); ++i) {
            if (v[i] >= nodes_[i].max_rank) continue;

            // Check prerequisite
            if (nodes_[i].prereq_id >= 0) {
                size_t req_idx = static_cast<size_t>(nodes_[i].prereq_id);
                if (v[req_idx] < nodes_[req_idx].max_rank) continue;
            }

            // Check row requirement
            int tree = nodes_[i].tree;
            int required_points = 5 * (nodes_[i].row - 1);
            int pts_below = 0;
            for (size_t k = 0; k < nodes_.size() && k < v.size(); ++k) {
                if (nodes_[k].tree == tree && nodes_[k].row < nodes_[i].row) {
                    pts_below += v[k];
                }
            }
            if (pts_below < required_points) continue;

            receivers.push_back(i);
        }
        return receivers;
    }

    template <typename RNG>
    void repair(std::vector<int>& v, RNG& rng, const std::vector<int>& req_talents = {}, int target_total_points = 51) const {
        if (v.size() != nodes_.size()) v.resize(nodes_.size(), 0);

        // 1. Clamp ranks
        for (size_t i = 0; i < nodes_.size(); ++i) {
            v[i] = std::clamp(v[i], 0, nodes_[i].max_rank);
        }

        // 2. Top-down validation: clear illegal points whose prerequisites or row limits are broken
        for (int tree = 0; tree < 3; ++tree) {
            int running = 0;
            for (int r = 1; r <= 7; ++r) {
                int required = 5 * (r - 1);
                for (size_t i = 0; i < nodes_.size(); ++i) {
                    if (nodes_[i].tree == tree && nodes_[i].row == r) {
                        bool prereq_ok = (nodes_[i].prereq_id < 0 || v[nodes_[i].prereq_id] == nodes_[nodes_[i].prereq_id].max_rank);
                        if (running < required || !prereq_ok) {
                            v[i] = 0; // Clear invalid points!
                        }
                    }
                }
                for (size_t i = 0; i < nodes_.size(); ++i) {
                    if (nodes_[i].tree == tree && nodes_[i].row == r) {
                        running += v[i];
                    }
                }
            }
        }

        // 3. Enforce required talents
        for (int req_id : req_talents) {
            if (req_id < 0 || req_id >= static_cast<int>(nodes_.size())) continue;
            const auto& target_node = nodes_[req_id];

            // If prerequisite exists, max it
            if (target_node.prereq_id >= 0) {
                int p_idx = target_node.prereq_id;
                v[p_idx] = nodes_[p_idx].max_rank;
            }

            // Ensure sufficient points in preceding rows of the tree
            int tree = target_node.tree;
            int required_below = 5 * (target_node.row - 1);

            int cur_below = 0;
            for (size_t k = 0; k < nodes_.size(); ++k) {
                if (nodes_[k].tree == tree && nodes_[k].row < target_node.row) {
                    cur_below += v[k];
                }
            }

            int attempts = 0;
            while (cur_below < required_below && attempts++ < 200) {
                std::vector<size_t> row_cands;
                for (size_t k = 0; k < nodes_.size(); ++k) {
                    if (nodes_[k].tree == tree && nodes_[k].row < target_node.row) {
                        if (v[k] < nodes_[k].max_rank) {
                            // Also check prerequisite of candidate
                            if (nodes_[k].prereq_id < 0 || v[nodes_[k].prereq_id] == nodes_[nodes_[k].prereq_id].max_rank) {
                                int cand_req = 5 * (nodes_[k].row - 1);
                                int cand_below = 0;
                                for (size_t m = 0; m < nodes_.size(); ++m) {
                                    if (nodes_[m].tree == tree && nodes_[m].row < nodes_[k].row) {
                                        cand_below += v[m];
                                    }
                                }
                                if (cand_below >= cand_req) {
                                    row_cands.push_back(k);
                                }
                            }
                        }
                    }
                }
                if (row_cands.empty()) break;
                size_t pick = row_cands[rng() % row_cands.size()];
                v[pick]++;
                cur_below++;
            }

            v[req_id] = target_node.max_rank;
        }

        // 4. Adjust total points to target_total_points
        int current = count_total_points(v);
        int guard = 0;
        while (current > target_total_points && guard++ < 500) {
            auto donors = get_valid_donors(v, req_talents);
            if (donors.empty()) {
                v = generate_random_valid(rng, target_total_points, req_talents);
                return;
            }
            size_t pick = donors[rng() % donors.size()];
            v[pick]--;
            current--;
        }

        guard = 0;
        while (current < target_total_points && guard++ < 500) {
            auto receivers = get_valid_receivers(v);
            if (receivers.empty()) break;
            size_t pick = receivers[rng() % receivers.size()];
            v[pick]++;
            current++;
        }

        // 5. Final fallback guarantee
        if (!is_valid(v, target_total_points)) {
            v = generate_random_valid(rng, target_total_points, req_talents);
        }
    }

    template <typename RNG>
    std::vector<int> generate_random_valid(RNG& rng, int target_total_points = 51, const std::vector<int>& req_talents = {}) const {
        std::vector<int> v(nodes_.size(), 0);

        // Enforce required talents first
        for (int req_id : req_talents) {
            if (req_id < 0 || req_id >= static_cast<int>(nodes_.size())) continue;
            const auto& target_node = nodes_[req_id];

            if (target_node.prereq_id >= 0) {
                int p_idx = target_node.prereq_id;
                v[p_idx] = nodes_[p_idx].max_rank;
            }

            int tree = target_node.tree;
            int required_below = 5 * (target_node.row - 1);
            int cur_below = 0;
            for (size_t k = 0; k < nodes_.size(); ++k) {
                if (nodes_[k].tree == tree && nodes_[k].row < target_node.row) {
                    cur_below += v[k];
                }
            }

            int attempts = 0;
            while (cur_below < required_below && attempts++ < 200) {
                std::vector<size_t> row_cands;
                for (size_t k = 0; k < nodes_.size(); ++k) {
                    if (nodes_[k].tree == tree && nodes_[k].row < target_node.row) {
                        if (v[k] < nodes_[k].max_rank) {
                            if (nodes_[k].prereq_id < 0 || v[nodes_[k].prereq_id] == nodes_[nodes_[k].prereq_id].max_rank) {
                                int cand_req = 5 * (nodes_[k].row - 1);
                                int cand_below = 0;
                                for (size_t m = 0; m < nodes_.size(); ++m) {
                                    if (nodes_[m].tree == tree && nodes_[m].row < nodes_[k].row) {
                                        cand_below += v[m];
                                    }
                                }
                                if (cand_below >= cand_req) {
                                    row_cands.push_back(k);
                                }
                            }
                        }
                    }
                }
                if (row_cands.empty()) break;
                size_t pick = row_cands[rng() % row_cands.size()];
                v[pick]++;
                cur_below++;
            }

            v[req_id] = target_node.max_rank;
        }

        int current = count_total_points(v);
        int guard = 0;
        while (current < target_total_points && guard++ < 500) {
            auto receivers = get_valid_receivers(v);
            if (receivers.empty()) break;
            size_t pick = receivers[rng() % receivers.size()];
            v[pick]++;
            current++;
        }

        return v;
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
