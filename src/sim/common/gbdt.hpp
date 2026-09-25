#pragma once
#include "sim_state_vector.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <functional>
#include <iomanip>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace sim {

// Hyperparameters for Gradient Boosted Decision Tree Regressor
struct GBDTConfig {
    size_t num_trees = 40;
    size_t max_depth = 4;
    float learning_rate = 0.1f;
    float l2_reg = 1.0f;             // L2 regularization (lambda)
    float min_child_weight = 5.0f;   // Minimum sum of hessian (samples/weight) in a leaf
    float min_split_gain = 1e-4f;    // Minimum gain required to make a further partition
    float subsample = 1.0f;          // Subsample ratio of the training instances
    float colsample_bytree = 1.0f;   // Subsample ratio of columns when constructing each tree
};

// Flat, cache-friendly node representation for single tree inference
struct GBDTNode {
    uint16_t feature_index = 0;
    bool is_leaf = false;
    float threshold = 0.0f;
    float leaf_value = 0.0f;
    int32_t left_child = -1;
    int32_t right_child = -1;
};

// Single Regression Tree in the GBDT Ensemble
class GBDTTree {
public:
    std::vector<GBDTNode> nodes;

    // Fast linear-array prediction
    inline float predict(const float* x) const {
        if (nodes.empty()) return 0.0f;
        int32_t idx = 0;
        while (idx >= 0 && idx < static_cast<int32_t>(nodes.size())) {
            const auto& node = nodes[idx];
            if (node.is_leaf) {
                return node.leaf_value;
            }
            if (x[node.feature_index] <= node.threshold) {
                idx = node.left_child;
            } else {
                idx = node.right_child;
            }
        }
        return 0.0f;
    }
};

// Pure C++ Gradient Boosted Decision Tree Regressor (LightGBM/XGBoost style 2nd-order boosting)
class GBDTRegressor {
public:
    GBDTRegressor(const GBDTConfig& config = GBDTConfig())
        : config_(config), base_score_(0.0f) {}

    // Trains the GBDT regressor on tabular feature matrix X and target y
    void fit(const std::vector<std::vector<float>>& X,
             const std::vector<float>& y,
             const std::vector<float>& weights = {})
    {
        if (X.empty() || y.empty() || X.size() != y.size()) {
            trees_.clear();
            base_score_ = 0.0f;
            return;
        }

        size_t n = X.size();
        size_t d = X[0].size();
        num_features_ = d;
        trees_.clear();
        trees_.reserve(config_.num_trees);
        feature_importances_.assign(d, 0.0);

        std::vector<float> w(n, 1.0f);
        if (!weights.empty() && weights.size() == n) {
            w = weights;
        }

        // Initialize base prediction (weighted mean)
        double sum_yw = 0.0;
        double sum_w = 0.0;
        for (size_t i = 0; i < n; ++i) {
            sum_yw += static_cast<double>(y[i]) * static_cast<double>(w[i]);
            sum_w += static_cast<double>(w[i]);
        }
        base_score_ = (sum_w > 1e-6) ? static_cast<float>(sum_yw / sum_w) : 0.0f;

        // Current ensemble predictions F(x)
        std::vector<float> preds(n, base_score_);

        // Gradients and Hessians for squared loss: L(y, F) = 0.5 * w * (y - F)^2
        // g = -dL/dF = w * (y - F) (negative gradient / residual)
        // h = d2L/dF2 = w
        std::vector<float> g(n);
        std::vector<float> h(n);

        std::mt19937 rng(1337);

        for (size_t iter = 0; iter < config_.num_trees; ++iter) {
            for (size_t i = 0; i < n; ++i) {
                g[i] = w[i] * (y[i] - preds[i]);
                h[i] = w[i];
            }

            // Subsample rows if configured
            std::vector<size_t> sample_indices;
            sample_indices.reserve(n);
            if (config_.subsample < 0.999f) {
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                for (size_t i = 0; i < n; ++i) {
                    if (dist(rng) <= config_.subsample) {
                        sample_indices.push_back(i);
                    }
                }
                if (sample_indices.empty()) {
                    sample_indices.resize(n);
                    std::iota(sample_indices.begin(), sample_indices.end(), 0);
                }
            } else {
                sample_indices.resize(n);
                std::iota(sample_indices.begin(), sample_indices.end(), 0);
            }

            // Build single tree
            GBDTTree tree;
            build_tree(tree, X, g, h, sample_indices, 0);

            if (tree.nodes.empty()) break;

            // Update predictions: F(x) += learning_rate * tree(x)
            for (size_t i = 0; i < n; ++i) {
                preds[i] += config_.learning_rate * tree.predict(X[i].data());
            }

            trees_.push_back(std::move(tree));
        }
    }

    // Predict continuous value for a sample
    inline float predict(const float* x) const {
        float val = base_score_;
        for (const auto& tree : trees_) {
            val += config_.learning_rate * tree.predict(x);
        }
        return val;
    }

    // Predict continuous value from SimObservation
    inline float predict(const SimObservation& obs) const {
        auto arr = obs.to_array();
        return predict(arr.data());
    }

    // Feature importance (total split gain accumulated per feature)
    const std::vector<double>& get_feature_importances() const {
        return feature_importances_;
    }

    size_t num_trees() const { return trees_.size(); }
    float base_score() const { return base_score_; }
    const GBDTConfig& config() const { return config_; }

    // Serialization to compact JSON/text representation
    std::string to_json() const {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        ss << "{\n";
        ss << "  \"base_score\": " << base_score_ << ",\n";
        ss << "  \"learning_rate\": " << config_.learning_rate << ",\n";
        ss << "  \"num_features\": " << num_features_ << ",\n";
        ss << "  \"trees\": [\n";
        for (size_t t = 0; t < trees_.size(); ++t) {
            ss << "    {\n      \"nodes\": [\n";
            for (size_t i = 0; i < trees_[t].nodes.size(); ++i) {
                const auto& n = trees_[t].nodes[i];
                ss << "        {\"is_leaf\": " << (n.is_leaf ? "true" : "false")
                   << ", \"feat\": " << n.feature_index
                   << ", \"thresh\": " << n.threshold
                   << ", \"val\": " << n.leaf_value
                   << ", \"left\": " << n.left_child
                   << ", \"right\": " << n.right_child << "}"
                   << (i + 1 < trees_[t].nodes.size() ? ",\n" : "\n");
            }
            ss << "      ]\n    }" << (t + 1 < trees_.size() ? ",\n" : "\n");
        }
        ss << "  ]\n}";
        return ss.str();
    }

private:
    struct SplitInfo {
        size_t feature_idx = 0;
        float threshold = 0.0f;
        double gain = -1.0;
        std::vector<size_t> left_indices;
        std::vector<size_t> right_indices;
    };

    void build_tree(GBDTTree& tree,
                    const std::vector<std::vector<float>>& X,
                    const std::vector<float>& g,
                    const std::vector<float>& h,
                    const std::vector<size_t>& sample_indices,
                    size_t depth)
    {
        int32_t node_idx = static_cast<int32_t>(tree.nodes.size());
        tree.nodes.push_back(GBDTNode{});

        double sum_g = 0.0;
        double sum_h = 0.0;
        for (size_t idx : sample_indices) {
            sum_g += g[idx];
            sum_h += h[idx];
        }

        // Optimal leaf output calculation: sum(g) / (sum(h) + lambda)
        float leaf_val = static_cast<float>(sum_g / (sum_h + config_.l2_reg));

        if (depth >= config_.max_depth || sum_h < config_.min_child_weight || sample_indices.size() <= 2) {
            tree.nodes[node_idx].is_leaf = true;
            tree.nodes[node_idx].leaf_value = leaf_val;
            return;
        }

        SplitInfo best_split = find_best_split(X, g, h, sample_indices, sum_g, sum_h);

        if (best_split.gain <= config_.min_split_gain ||
            best_split.left_indices.empty() ||
            best_split.right_indices.empty())
        {
            tree.nodes[node_idx].is_leaf = true;
            tree.nodes[node_idx].leaf_value = leaf_val;
            return;
        }

        // Record feature importance gain
        if (best_split.feature_idx < feature_importances_.size()) {
            feature_importances_[best_split.feature_idx] += best_split.gain;
        }

        tree.nodes[node_idx].is_leaf = false;
        tree.nodes[node_idx].feature_index = static_cast<uint16_t>(best_split.feature_idx);
        tree.nodes[node_idx].threshold = best_split.threshold;

        // Build Left Child
        tree.nodes[node_idx].left_child = static_cast<int32_t>(tree.nodes.size());
        build_tree(tree, X, g, h, best_split.left_indices, depth + 1);

        // Build Right Child
        tree.nodes[node_idx].right_child = static_cast<int32_t>(tree.nodes.size());
        build_tree(tree, X, g, h, best_split.right_indices, depth + 1);
    }

    SplitInfo find_best_split(const std::vector<std::vector<float>>& X,
                              const std::vector<float>& g,
                              const std::vector<float>& h,
                              const std::vector<size_t>& sample_indices,
                              double sum_g,
                              double sum_h)
    {
        SplitInfo best;
        double parent_score = (sum_g * sum_g) / (sum_h + config_.l2_reg);

        struct SampleVal {
            float val;
            size_t idx;
        };
        std::vector<SampleVal> col(sample_indices.size());

        for (size_t f = 0; f < num_features_; ++f) {
            for (size_t i = 0; i < sample_indices.size(); ++i) {
                size_t s_idx = sample_indices[i];
                col[i] = { X[s_idx][f], s_idx };
            }

            std::sort(col.begin(), col.end(), [](const SampleVal& a, const SampleVal& b) {
                return a.val < b.val;
            });

            // If feature is constant across samples, skip
            if (col.front().val == col.back().val) continue;

            double g_left = 0.0;
            double h_left = 0.0;

            for (size_t i = 0; i < col.size() - 1; ++i) {
                size_t s_idx = col[i].idx;
                g_left += g[s_idx];
                h_left += h[s_idx];

                // Skip identical adjacent values
                if (col[i].val == col[i + 1].val) continue;

                double g_right = sum_g - g_left;
                double h_right = sum_h - h_left;

                if (h_left < config_.min_child_weight || h_right < config_.min_child_weight) {
                    continue;
                }

                double left_score = (g_left * g_left) / (h_left + config_.l2_reg);
                double right_score = (g_right * g_right) / (h_right + config_.l2_reg);
                double gain = 0.5 * (left_score + right_score - parent_score);

                if (gain > best.gain) {
                    best.gain = gain;
                    best.feature_idx = f;
                    best.threshold = 0.5f * (col[i].val + col[i + 1].val);
                }
            }
        }

        if (best.gain > 0.0) {
            best.left_indices.reserve(sample_indices.size() / 2);
            best.right_indices.reserve(sample_indices.size() / 2);
            for (size_t idx : sample_indices) {
                if (X[idx][best.feature_idx] <= best.threshold) {
                    best.left_indices.push_back(idx);
                } else {
                    best.right_indices.push_back(idx);
                }
            }
        }

        return best;
    }

    GBDTConfig config_;
    float base_score_;
    size_t num_features_ = 0;
    std::vector<GBDTTree> trees_;
    std::vector<double> feature_importances_;
};

// Multi-Action GBDT Q-Policy (predicts Q(s, a) for all candidate actions)
class GBDTMultiActionQPolicy {
public:
    GBDTMultiActionQPolicy(const GBDTConfig& config = GBDTConfig())
        : config_(config) {}

    // Trains a per-action GBDT regressor for each unique action present in samples
    // Samples: (observation, action_id, target_q_value, sample_weight)
    struct QSample {
        SimObservation state;
        uint8_t action = 0;
        float q_value = 0.0f;
        float weight = 1.0f;
    };

    void fit(const std::vector<QSample>& samples) {
        action_models_.clear();
        if (samples.empty()) return;

        // Group samples by action
        std::unordered_map<uint8_t, std::vector<std::vector<float>>> X_per_act;
        std::unordered_map<uint8_t, std::vector<float>> y_per_act;
        std::unordered_map<uint8_t, std::vector<float>> w_per_act;

        for (const auto& s : samples) {
            auto arr = s.state.to_array();
            std::vector<float> x_vec(arr.begin(), arr.end());
            X_per_act[s.action].push_back(std::move(x_vec));
            y_per_act[s.action].push_back(s.q_value);
            w_per_act[s.action].push_back(s.weight);
        }

        for (auto& [act, X] : X_per_act) {
            GBDTRegressor reg(config_);
            reg.fit(X, y_per_act[act], w_per_act[act]);
            action_models_[act] = std::move(reg);
        }
    }

    // Predict Q-value for specific action
    inline float predict_q(uint8_t action, const SimObservation& obs) const {
        auto it = action_models_.find(action);
        if (it == action_models_.end()) {
            return -10000.0f;
        }
        return it->second.predict(obs);
    }

    // Select optimal action among legal set
    uint8_t select_best_action(const SimObservation& obs, const std::vector<uint8_t>& legal_actions, uint8_t default_fallback = 0) const {
        if (legal_actions.empty()) return default_fallback;
        if (legal_actions.size() == 1) return legal_actions[0];

        uint8_t best_act = legal_actions[0];
        float best_q = -1e9f;

        for (uint8_t act : legal_actions) {
            float q = predict_q(act, obs);
            if (q > best_q) {
                best_q = q;
                best_act = act;
            }
        }
        return best_act;
    }

    bool has_model_for_action(uint8_t action) const {
        return action_models_.find(action) != action_models_.end();
    }

    const std::unordered_map<uint8_t, GBDTRegressor>& models() const {
        return action_models_;
    }

private:
    GBDTConfig config_;
    std::unordered_map<uint8_t, GBDTRegressor> action_models_;
};

} // namespace sim
