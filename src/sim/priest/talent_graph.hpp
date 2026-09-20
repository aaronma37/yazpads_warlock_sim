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
