#pragma once
#include "policy.hpp"
#include <array>
#include <functional>
#include <algorithm>
#include <numeric>

namespace warlock {
// A continuous action is a vector of spell preferences. The simulator executes
// the highest-ranked admissible rule. PPO learns the density of the complete
// sampled vector, not a fabricated categorical probability of the chosen spell.
inline constexpr std::array neural_actions = {
    PriorityAction::LIFE_TAP, PriorityAction::CURSE_OF_AGONY,
    PriorityAction::CURSE_OF_DOOM, PriorityAction::NIGHTFALL_SHADOW_BOLT,
    PriorityAction::DECIMATION_SEARING_PAIN, PriorityAction::DECIMATION_SOUL_FIRE,
    PriorityAction::DEMONIC_BRAND_SEARING_PAIN, PriorityAction::CORRUPTION,
    PriorityAction::SIPHON_LIFE, PriorityAction::DRAIN_HOPE,
    PriorityAction::IMMOLATE, PriorityAction::CONFLAGRATE,
    PriorityAction::SHADOWBURN, PriorityAction::SHADOWBURN_ISB,
    PriorityAction::INCINERATE_FILLER, PriorityAction::SEARING_PAIN_FILLER,
    PriorityAction::DRAIN_LIFE_FILLER, PriorityAction::DRAIN_SOUL_FILLER,
    PriorityAction::SHADOW_BOLT_FILLER};
using NeuralObservation = std::array<float, 24>;
using NeuralAction = std::array<float, neural_actions.size()>;
using NeuralDecision = std::function<NeuralAction(const NeuralObservation&, double, double)>;

inline NeuralObservation normalize_neural_observation(NeuralObservation obs) {
    // Fixed scaling is identical during collection, PPO updates and inference.
    obs[3] /= 300.0f;
    obs[5] /= 10.0f;
    for (size_t i = 8; i < obs.size(); ++i) obs[i] /= (i == 12 || i == 19) ? 5.0f : 60.0f;
    for (auto& value : obs) value = std::clamp(value, 0.0f, 10.0f);
    return obs;
}

inline std::vector<PriorityRule> neural_priority_rules(const NeuralAction& scores) {
    std::array<size_t, neural_actions.size()> order;
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return scores[a] > scores[b]; });
    std::vector<PriorityRule> rules;
    for (auto i : order) {
        PriorityRule rule;
        rule.action = neural_actions[i];
        rules.push_back(rule);
    }
    return rules;
}
}
