#pragma once
#include "sim_state_vector.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace sim {

// A single split condition node in the decision tree
struct DecisionTreeNode {
    bool is_leaf = false;
    size_t node_id = 0;
    size_t depth = 0;

    // Split properties (if not leaf)
    size_t feature_index = 0;
    std::string feature_name;
    float threshold = 0.0f;
    std::unique_ptr<DecisionTreeNode> left_child;
    std::unique_ptr<DecisionTreeNode> right_child;

    // Leaf / Node properties
    uint8_t predicted_class = 0;
    std::string class_name;
    double sample_count = 0.0;
    double weighted_sample_count = 0.0;
    double impurity = 0.0;
    std::vector<double> class_probabilities; // Normalized class distribution
};

// Represents an extracted path rule from root to leaf
struct ExtractedDecisionRule {
    uint8_t action = 0;
    std::string action_name;
    double confidence = 0.0;
    double sample_weight = 0.0;
    size_t sample_count = 0;
    std::vector<std::string> conditions; // Human-readable conditions e.g. "player_mana_pct <= 0.250"

    // Continuous & Structured Predicate Bounds
    bool has_custom_bounds = false;
    float max_mana_pct = 1.0f;
    float min_mana_pct = 0.0f;
    float max_target_hp_pct = 1.0f;
    float min_target_hp_pct = 0.0f;
    float min_time_remaining = 0.0f;
    float max_time_remaining = 9999.0f;
    float max_dot_rem_sec = 0.0f;
    float min_hp_pct = 0.15f;

    std::string to_string() const {
        std::ostringstream ss;
        ss << "IF ";
        if (conditions.empty()) {
            ss << "TRUE";
        } else {
            for (size_t i = 0; i < conditions.size(); ++i) {
                if (i > 0) ss << " AND ";
                ss << conditions[i];
            }
        }
        ss << " THEN Action = " << action_name << " (conf: " 
           << std::fixed << std::setprecision(1) << (confidence * 100.0) << "%, n=" << sample_count << ")";
        return ss.str();
    }
};

// Hyperparameters for Decision Tree Training
struct DecisionTreeConfig {
    size_t max_depth = 5;
    size_t min_samples_leaf = 10;
    size_t min_samples_split = 20;
    double min_impurity_decrease = 1e-4;
    size_t max_features = 0; // 0 = all features
};

// Pure C++ CART (Classification and Regression Tree) classifier with Q-weighting
class DecisionTreeClassifier {
public:
    DecisionTreeClassifier(const DecisionTreeConfig& config = DecisionTreeConfig())
        : config_(config) {}
    DecisionTreeClassifier(DecisionTreeClassifier&&) noexcept = default;
    DecisionTreeClassifier& operator=(DecisionTreeClassifier&&) noexcept = default;

    // Trains the decision tree on a VIPERDataset
    void fit(const VIPERDataset& dataset, size_t num_classes = 32) {
        if (dataset.samples.empty()) {
            root_ = nullptr;
            return;
        }

        num_classes_ = num_classes;
        feature_names_ = SimObservation::feature_names();

        // Build training matrix
        size_t n = dataset.samples.size();
        size_t d = SimObservation::FEATURE_COUNT;

        std::vector<std::array<float, SimObservation::FEATURE_COUNT>> X(n);
        std::vector<uint8_t> y(n);
        std::vector<double> weights(n);

        double total_weight = 0.0;
        for (size_t i = 0; i < n; ++i) {
            X[i] = dataset.samples[i].state.to_array();
            y[i] = dataset.samples[i].oracle_action;
            weights[i] = std::max(1e-4, static_cast<double>(dataset.samples[i].sample_weight));
            total_weight += weights[i];
            if (!dataset.samples[i].action_name.empty()) {
                class_names_[y[i]] = dataset.samples[i].action_name;
            }
        }

        // Normalize weights so average weight is 1.0
        double norm_scale = static_cast<double>(n) / std::max(1e-6, total_weight);
        for (size_t i = 0; i < n; ++i) {
            weights[i] *= norm_scale;
        }

        std::vector<size_t> sample_indices(n);
        std::iota(sample_indices.begin(), sample_indices.end(), 0);

        next_node_id_ = 0;
        root_ = build_tree_recursive(X, y, weights, sample_indices, 0);
    }

    // Predict best action for a state observation
    uint8_t predict(const SimObservation& obs) const {
        if (!root_) return 0;
        auto arr = obs.to_array();
        const DecisionTreeNode* curr = root_.get();
        while (curr && !curr->is_leaf) {
            if (arr[curr->feature_index] <= curr->threshold) {
                curr = curr->left_child.get();
            } else {
                curr = curr->right_child.get();
            }
        }
        return curr ? curr->predicted_class : 0;
    }

    // Predict class probability distribution
    std::vector<double> predict_proba(const SimObservation& obs) const {
        if (!root_) return std::vector<double>(num_classes_, 0.0);
        auto arr = obs.to_array();
        const DecisionTreeNode* curr = root_.get();
        while (curr && !curr->is_leaf) {
            if (arr[curr->feature_index] <= curr->threshold) {
                curr = curr->left_child.get();
            } else {
                curr = curr->right_child.get();
            }
        }
        if (curr && !curr->class_probabilities.empty()) {
            return curr->class_probabilities;
        }
        std::vector<double> def(num_classes_, 0.0);
        if (curr) def[curr->predicted_class] = 1.0;
        return def;
    }

    // Unweighted accuracy on a dataset
    double score_unweighted(const VIPERDataset& dataset) const {
        if (dataset.samples.empty() || !root_) return 0.0;
        size_t correct = 0;
        for (const auto& sample : dataset.samples) {
            if (predict(sample.state) == sample.oracle_action) {
                correct++;
            }
        }
        return static_cast<double>(correct) / static_cast<double>(dataset.samples.size());
    }

    // Q-weighted fidelity on a dataset
    double score_weighted(const VIPERDataset& dataset) const {
        if (dataset.samples.empty() || !root_) return 0.0;
        double weighted_correct = 0.0;
        double total_weight = 0.0;
        for (const auto& sample : dataset.samples) {
            double w = std::max(1e-4, static_cast<double>(sample.sample_weight));
            total_weight += w;
            if (predict(sample.state) == sample.oracle_action) {
                weighted_correct += w;
            }
        }
        return (total_weight > 0.0) ? (weighted_correct / total_weight) : 0.0;
    }

    // Tree properties
    size_t get_depth() const {
        return compute_depth(root_.get());
    }

    size_t get_leaf_count() const {
        return compute_leaf_count(root_.get());
    }

    size_t get_node_count() const {
        return compute_node_count(root_.get());
    }

    const DecisionTreeNode* get_root() const {
        return root_.get();
    }

    // Transpiles decision tree into standalone C++ function
    std::string to_cpp(const std::string& function_name = "evaluate_viper_tree_policy") const {
        if (!root_) {
            return "// Empty decision tree\n";
        }

        std::ostringstream ss;
        ss << "// Auto-generated VIPER CART Decision Tree Policy\n";
        ss << "inline PriorityAction " << function_name << "(const sim::SimObservation& obs) {\n";

        std::function<void(const DecisionTreeNode*, size_t)> recurse = [&](const DecisionTreeNode* node, size_t depth) {
            std::string indent(depth * 4, ' ');
            if (node->is_leaf) {
                std::string action_str = get_action_enum_string(node->predicted_class);
                ss << indent << "return PriorityAction::" << action_str << "; // samples: " 
                   << static_cast<size_t>(node->sample_count) << "\n";
            } else {
                ss << indent << "if (obs." << node->feature_name << " <= " 
                   << std::fixed << std::setprecision(4) << node->threshold << "f) {\n";
                recurse(node->left_child.get(), depth + 1);
                ss << indent << "} else {\n";
                recurse(node->right_child.get(), depth + 1);
                ss << indent << "}\n";
            }
        };

        recurse(root_.get(), 1);
        ss << "}\n";
        return ss.str();
    }

    // Generates ASCII / hierarchical text tree visualization
    std::string to_text_tree(size_t max_print_depth = 6) const {
        if (!root_) return "(Empty Decision Tree)";

        std::ostringstream ss;
        std::function<void(const DecisionTreeNode*, size_t, const std::string&)> recurse = 
            [&](const DecisionTreeNode* node, size_t depth, const std::string& prefix) {
            if (!node || depth > max_print_depth) return;

            if (node->is_leaf) {
                double conf = 0.0;
                if (!node->class_probabilities.empty() && node->predicted_class < node->class_probabilities.size()) {
                    conf = node->class_probabilities[node->predicted_class];
                }
                std::string name = get_action_display_name(node->predicted_class);
                ss << prefix << "[Leaf] Action: " << name 
                   << " | Purity: " << std::fixed << std::setprecision(1) << (conf * 100.0) << "%"
                   << " | Samples: " << static_cast<size_t>(node->sample_count)
                   << " (W=" << std::setprecision(1) << node->weighted_sample_count << ")\n";
            } else {
                ss << prefix << "[Split] " << node->feature_name << " <= " 
                   << std::fixed << std::setprecision(4) << node->threshold
                   << " (Gini=" << std::setprecision(3) << node->impurity << ")\n";
                
                std::string left_prefix = prefix + "  |--[<=] ";
                std::string right_prefix = prefix + "  +--[ >] ";
                recurse(node->left_child.get(), depth + 1, left_prefix);
                recurse(node->right_child.get(), depth + 1, right_prefix);
            }
        };

        recurse(root_.get(), 0, "");
        return ss.str();
    }

    // Extracts flat decision rules from root to each leaf with continuous bounds
    std::vector<ExtractedDecisionRule> extract_rules() const {
        std::vector<ExtractedDecisionRule> rules;
        if (!root_) return rules;

        std::vector<std::string> current_conditions;

        struct ActiveBounds {
            float max_mana_pct = 1.0f;
            float min_mana_pct = 0.0f;
            float max_target_hp_pct = 1.0f;
            float min_target_hp_pct = 0.0f;
            float min_time_remaining = 0.0f;
            float max_time_remaining = 9999.0f;
            float max_dot_rem_sec = 0.0f;
            float min_hp_pct = 0.15f;
            bool has_bounds = false;
        };

        std::function<void(const DecisionTreeNode*, ActiveBounds)> traverse = 
            [&](const DecisionTreeNode* node, ActiveBounds bounds) {
            if (!node) return;
            if (node->is_leaf) {
                ExtractedDecisionRule r;
                r.action = node->predicted_class;
                r.action_name = get_action_display_name(node->predicted_class);
                r.sample_count = static_cast<size_t>(node->sample_count);
                r.sample_weight = node->weighted_sample_count;
                if (!node->class_probabilities.empty() && node->predicted_class < node->class_probabilities.size()) {
                    r.confidence = node->class_probabilities[node->predicted_class];
                }
                r.conditions = current_conditions;
                r.has_custom_bounds = bounds.has_bounds;
                r.max_mana_pct = bounds.max_mana_pct;
                r.min_mana_pct = bounds.min_mana_pct;
                r.max_target_hp_pct = bounds.max_target_hp_pct;
                r.min_target_hp_pct = bounds.min_target_hp_pct;
                r.min_time_remaining = bounds.min_time_remaining;
                r.max_time_remaining = bounds.max_time_remaining;
                r.max_dot_rem_sec = bounds.max_dot_rem_sec;
                r.min_hp_pct = bounds.min_hp_pct;
                rules.push_back(r);
            } else {
                std::ostringstream left_cond, right_cond;
                left_cond << node->feature_name << " <= " << std::fixed << std::setprecision(3) << node->threshold;
                right_cond << node->feature_name << " > " << std::fixed << std::setprecision(3) << node->threshold;

                ActiveBounds left_b = bounds;
                ActiveBounds right_b = bounds;
                left_b.has_bounds = true;
                right_b.has_bounds = true;

                if (node->feature_name == "player_mana_pct") {
                    left_b.max_mana_pct = std::min(left_b.max_mana_pct, node->threshold);
                    right_b.min_mana_pct = std::max(right_b.min_mana_pct, node->threshold);
                } else if (node->feature_name == "target_hp_pct") {
                    left_b.max_target_hp_pct = std::min(left_b.max_target_hp_pct, node->threshold);
                    right_b.min_target_hp_pct = std::max(right_b.min_target_hp_pct, node->threshold);
                } else if (node->feature_name == "time_remaining_sec") {
                    left_b.max_time_remaining = std::min(left_b.max_time_remaining, node->threshold);
                    right_b.min_time_remaining = std::max(right_b.min_time_remaining, node->threshold);
                } else if (node->feature_name.find("dot_") != std::string::npos) {
                    left_b.max_dot_rem_sec = std::max(left_b.max_dot_rem_sec, node->threshold);
                } else if (node->feature_name == "player_hp_pct") {
                    left_b.min_hp_pct = std::max(left_b.min_hp_pct, node->threshold);
                }

                current_conditions.push_back(left_cond.str());
                traverse(node->left_child.get(), left_b);
                current_conditions.pop_back();

                current_conditions.push_back(right_cond.str());
                traverse(node->right_child.get(), right_b);
                current_conditions.pop_back();
            }
        };

        traverse(root_.get(), ActiveBounds{});
        return rules;
    }

    void set_class_name(uint8_t class_id, const std::string& name) {
        class_names_[class_id] = name;
    }

private:
    DecisionTreeConfig config_;
    std::unique_ptr<DecisionTreeNode> root_;
    size_t num_classes_ = 32;
    std::vector<std::string> feature_names_;
    std::unordered_map<uint8_t, std::string> class_names_;
    size_t next_node_id_ = 0;

    // Calculates Gini impurity for a weighted set of labels
    double compute_gini(
        const std::vector<uint8_t>& y,
        const std::vector<double>& weights,
        const std::vector<size_t>& indices,
        std::vector<double>& out_class_weights,
        double& out_total_weight) const
    {
        out_class_weights.assign(num_classes_, 0.0);
        out_total_weight = 0.0;

        for (size_t idx : indices) {
            uint8_t c = y[idx];
            double w = weights[idx];
            if (c < num_classes_) {
                out_class_weights[c] += w;
            }
            out_total_weight += w;
        }

        if (out_total_weight <= 0.0) return 0.0;

        double sum_sq = 0.0;
        for (double cw : out_class_weights) {
            double p = cw / out_total_weight;
            sum_sq += p * p;
        }
        return 1.0 - sum_sq;
    }

    struct SplitCandidate {
        bool found = false;
        size_t feature_idx = 0;
        float threshold = 0.0f;
        double impurity_decrease = 0.0;
        std::vector<size_t> left_indices;
        std::vector<size_t> right_indices;
    };

    // Recursively constructs decision tree nodes
    std::unique_ptr<DecisionTreeNode> build_tree_recursive(
        const std::vector<std::array<float, SimObservation::FEATURE_COUNT>>& X,
        const std::vector<uint8_t>& y,
        const std::vector<double>& weights,
        const std::vector<size_t>& indices,
        size_t depth)
    {
        auto node = std::make_unique<DecisionTreeNode>();
        node->node_id = next_node_id_++;
        node->depth = depth;
        node->sample_count = static_cast<double>(indices.size());

        std::vector<double> class_weights;
        double total_weight = 0.0;
        node->impurity = compute_gini(y, weights, indices, class_weights, total_weight);
        node->weighted_sample_count = total_weight;

        // Class probabilities and best class
        node->class_probabilities.resize(num_classes_, 0.0);
        uint8_t best_class = 0;
        double max_cw = -1.0;
        for (size_t c = 0; c < num_classes_; ++c) {
            if (total_weight > 0.0) {
                node->class_probabilities[c] = class_weights[c] / total_weight;
            }
            if (class_weights[c] > max_cw) {
                max_cw = class_weights[c];
                best_class = static_cast<uint8_t>(c);
            }
        }
        node->predicted_class = best_class;
        node->class_name = get_action_display_name(best_class);

        // Check stopping criteria
        if (depth >= config_.max_depth ||
            indices.size() < config_.min_samples_split ||
            indices.size() <= config_.min_samples_leaf ||
            node->impurity <= 1e-6)
        {
            node->is_leaf = true;
            return node;
        }

        // Find optimal split across all features
        SplitCandidate best_split = find_best_split(X, y, weights, indices, node->impurity, total_weight);

        if (!best_split.found || 
            best_split.impurity_decrease < config_.min_impurity_decrease ||
            best_split.left_indices.size() < config_.min_samples_leaf ||
            best_split.right_indices.size() < config_.min_samples_leaf)
        {
            node->is_leaf = true;
            return node;
        }

        node->is_leaf = false;
        node->feature_index = best_split.feature_idx;
        node->feature_name = (best_split.feature_idx < feature_names_.size()) 
            ? feature_names_[best_split.feature_idx] : ("f" + std::to_string(best_split.feature_idx));
        node->threshold = best_split.threshold;

        node->left_child = build_tree_recursive(X, y, weights, best_split.left_indices, depth + 1);
        node->right_child = build_tree_recursive(X, y, weights, best_split.right_indices, depth + 1);

        return node;
    }

    // Evaluates split points across features using fast sorted prefix scanning
    SplitCandidate find_best_split(
        const std::vector<std::array<float, SimObservation::FEATURE_COUNT>>& X,
        const std::vector<uint8_t>& y,
        const std::vector<double>& weights,
        const std::vector<size_t>& indices,
        double parent_impurity,
        double parent_weight)
    {
        SplitCandidate best;
        best.found = false;
        best.impurity_decrease = -1.0;

        size_t num_features = SimObservation::FEATURE_COUNT;

        // Total class weights in this node
        std::vector<double> total_class_weights(num_classes_, 0.0);
        for (size_t idx : indices) {
            uint8_t c = y[idx];
            if (c < num_classes_) {
                total_class_weights[c] += weights[idx];
            }
        }

        // Reusable sorted index vector
        std::vector<size_t> sorted_indices = indices;

        for (size_t f = 0; f < num_features; ++f) {
            // Sort sample indices by feature value
            std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t a, size_t b) {
                return X[a][f] < X[b][f];
            });

            // Check if feature has variation
            if (X[sorted_indices.front()][f] == X[sorted_indices.back()][f]) {
                continue;
            }

            std::vector<double> left_class_weights(num_classes_, 0.0);
            double left_weight = 0.0;

            for (size_t i = 0; i < sorted_indices.size() - 1; ++i) {
                size_t idx = sorted_indices[i];
                uint8_t c = y[idx];
                double w = weights[idx];
                if (c < num_classes_) left_class_weights[c] += w;
                left_weight += w;

                size_t left_count = i + 1;
                size_t right_count = sorted_indices.size() - left_count;

                // Threshold condition check (adjacent values must differ)
                float current_val = X[idx][f];
                float next_val = X[sorted_indices[i + 1]][f];

                if (current_val == next_val || left_count < config_.min_samples_leaf || right_count < config_.min_samples_leaf) {
                    continue;
                }

                double right_weight = parent_weight - left_weight;
                if (left_weight <= 0.0 || right_weight <= 0.0) continue;

                // Fast Gini calculation for left and right partitions
                double left_sum_sq = 0.0;
                for (size_t k = 0; k < num_classes_; ++k) {
                    double p = left_class_weights[k] / left_weight;
                    left_sum_sq += p * p;
                }
                double left_gini = 1.0 - left_sum_sq;

                double right_sum_sq = 0.0;
                for (size_t k = 0; k < num_classes_; ++k) {
                    double right_cw = total_class_weights[k] - left_class_weights[k];
                    double p = right_cw / right_weight;
                    right_sum_sq += p * p;
                }
                double right_gini = 1.0 - right_sum_sq;

                double weighted_child_impurity = (left_weight / parent_weight) * left_gini + (right_weight / parent_weight) * right_gini;
                double impurity_decrease = parent_impurity - weighted_child_impurity;

                if (impurity_decrease > best.impurity_decrease) {
                    best.found = true;
                    best.feature_idx = f;
                    best.threshold = (current_val + next_val) * 0.5f;
                    best.impurity_decrease = impurity_decrease;
                }
            }
        }

        // If a split was chosen, partition the indices
        if (best.found) {
            best.left_indices.reserve(indices.size());
            best.right_indices.reserve(indices.size());
            for (size_t idx : indices) {
                if (X[idx][best.feature_idx] <= best.threshold) {
                    best.left_indices.push_back(idx);
                } else {
                    best.right_indices.push_back(idx);
                }
            }
        }

        return best;
    }

    size_t compute_depth(const DecisionTreeNode* node) const {
        if (!node || node->is_leaf) return 0;
        return 1 + std::max(compute_depth(node->left_child.get()), compute_depth(node->right_child.get()));
    }

    size_t compute_leaf_count(const DecisionTreeNode* node) const {
        if (!node) return 0;
        if (node->is_leaf) return 1;
        return compute_leaf_count(node->left_child.get()) + compute_leaf_count(node->right_child.get());
    }

    size_t compute_node_count(const DecisionTreeNode* node) const {
        if (!node) return 0;
        return 1 + compute_node_count(node->left_child.get()) + compute_node_count(node->right_child.get());
    }

    std::string get_action_display_name(uint8_t action) const {
        auto it = class_names_.find(action);
        if (it != class_names_.end()) return it->second;
        return "Action_" + std::to_string(static_cast<int>(action));
    }

    static std::string get_action_enum_string(uint8_t action) {
        switch (action) {
            case 0: return "LIFE_TAP";
            case 1: return "RACIAL_EUREKA";
            case 2: return "RACIAL_BLOOD_FURY";
            case 3: return "RACIAL_BERSERKING";
            case 4: return "AMPLIFY_CURSE";
            case 5: return "CURSE_OF_AGONY";
            case 6: return "CURSE_OF_DOOM";
            case 7: return "BANE_OF_HAVOC";
            case 8: return "NIGHTFALL_SHADOW_BOLT";
            case 9: return "DECIMATION_SEARING_PAIN";
            case 10: return "DECIMATION_SOUL_FIRE";
            case 11: return "DEMONIC_BRAND_SEARING_PAIN";
            case 12: return "CORRUPTION";
            case 13: return "SIPHON_LIFE";
            case 14: return "DRAIN_HOPE";
            case 15: return "IMMOLATE";
            case 16: return "CONFLAGRATE";
            case 17: return "SHADOWBURN";
            case 18: return "INCINERATE_FILLER";
            case 19: return "SEARING_PAIN_FILLER";
            case 20: return "DRAIN_LIFE_FILLER";
            case 21: return "DRAIN_SOUL_FILLER";
            case 22: return "SHADOW_BOLT_FILLER";
            default: return "SHADOW_BOLT_FILLER";
        }
    }
};

} // namespace sim
