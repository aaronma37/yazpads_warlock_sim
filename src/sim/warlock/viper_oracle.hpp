#pragma once
#include "src/sim/common/sim_state_vector.hpp"
#include "src/sim/common/decision_tree.hpp"
#include "src/sim/common/gbdt.hpp"
#include "src/sim/warlock/warlock_sim.hpp"
#include "src/sim/warlock/policy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <mutex>
#include <unordered_map>

namespace warlock {

struct ActionQValue {
    PriorityAction action = PriorityAction::SHADOW_BOLT_FILLER;
    SpellID spell_id = SpellID::SHADOW_BOLT;
    std::string name;
    double q_value = 0.0; // Expected total damage or DPS from this state
    bool is_legal = true;
};

struct StateQEvaluation {
    sim::SimObservation observation;
    std::vector<ActionQValue> action_q_values;
    PriorityAction best_action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string best_action_name;
    double max_q = 0.0;
    double min_q = 0.0;
    double second_best_q = 0.0;
    double sample_weight = 0.0; // max_q - min_q (or max_q - second_best_q)
};

struct ActionStat {
    PriorityAction action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string name;
    size_t selection_count = 0;
    float selection_pct = 0.0f;
    double avg_q_value = 0.0;
    double avg_regret = 0.0;
};

struct RuleShift {
    PriorityAction action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string name;
    int baseline_rank = -1; // 1-indexed, -1 if not in baseline
    int viper_rank = -1;    // 1-indexed, -1 if not in viper
    std::string change_type; // "PROMOTED", "DEMOTED", "NEW", "UNCHANGED", "DISABLED"
};

class VIPEROracle {
public:
    // List of candidate rotational actions to evaluate for Warlock
    static std::vector<std::pair<PriorityAction, SpellID>> get_candidate_actions(bool allow_rank2 = false) {
        std::vector<std::pair<PriorityAction, SpellID>> actions = {
            {PriorityAction::LIFE_TAP, SpellID::LIFE_TAP},
            {PriorityAction::RACIAL_EUREKA, SpellID::SHADOW_BOLT},
            {PriorityAction::RACIAL_BLOOD_FURY, SpellID::SHADOW_BOLT},
            {PriorityAction::RACIAL_BERSERKING, SpellID::SHADOW_BOLT},
            {PriorityAction::AMPLIFY_CURSE, SpellID::AMPLIFY_CURSE},
            {PriorityAction::NIGHTFALL_SHADOW_BOLT, SpellID::SHADOW_BOLT},
            {PriorityAction::DECIMATION_SOUL_FIRE, SpellID::SOUL_FIRE},
            {PriorityAction::DECIMATION_SEARING_PAIN, SpellID::SEARING_PAIN},
            {PriorityAction::IMMOLATE, SpellID::IMMOLATE},
            {PriorityAction::CONFLAGRATE, SpellID::CONFLAGRATE},
            {PriorityAction::CORRUPTION, SpellID::CORRUPTION},
            {PriorityAction::CURSE_OF_DOOM, SpellID::CURSE_OF_DOOM},
            {PriorityAction::CURSE_OF_AGONY, SpellID::CURSE_OF_AGONY},
            {PriorityAction::SIPHON_LIFE, SpellID::SIPHON_LIFE},
            {PriorityAction::DRAIN_HOPE, SpellID::DRAIN_HOPE},
            {PriorityAction::SHADOWBURN, SpellID::SHADOWBURN},
            {PriorityAction::SHADOWBURN_ISB, SpellID::SHADOWBURN},
            {PriorityAction::DEMONIC_BRAND_SEARING_PAIN, SpellID::DEMONIC_BRAND},
            {PriorityAction::INCINERATE_FILLER, SpellID::INCINERATE},
            {PriorityAction::SEARING_PAIN_FILLER, SpellID::SEARING_PAIN},
            {PriorityAction::DRAIN_SOUL_FILLER, SpellID::DRAIN_SOUL},
            {PriorityAction::DRAIN_LIFE_FILLER, SpellID::DRAIN_LIFE},
            {PriorityAction::SHADOW_BOLT_FILLER, SpellID::SHADOW_BOLT}
        };
        if (allow_rank2) actions.emplace_back(PriorityAction::SHADOW_BOLT_RANK2, SpellID::SHADOW_BOLT);
        return actions;
    }

    static const char* get_action_name(PriorityAction action) {
        switch (action) {
            case PriorityAction::LIFE_TAP: return "Life Tap";
            case PriorityAction::RACIAL_EUREKA: return "Eureka";
            case PriorityAction::RACIAL_BLOOD_FURY: return "Blood Fury";
            case PriorityAction::RACIAL_BERSERKING: return "Berserking";
            case PriorityAction::AMPLIFY_CURSE: return "Amplify Curse";
            case PriorityAction::BANE_OF_HAVOC: return "Bane of Havoc";
            case PriorityAction::NIGHTFALL_SHADOW_BOLT: return "Shadow Bolt";
            case PriorityAction::DECIMATION_SOUL_FIRE: return "Soul Fire";
            case PriorityAction::DECIMATION_SEARING_PAIN: return "Searing Pain";
            case PriorityAction::DEMONIC_BRAND_SEARING_PAIN: return "Demonic Brand";
            case PriorityAction::CORRUPTION: return "Corruption";
            case PriorityAction::SIPHON_LIFE: return "Siphon Life";
            case PriorityAction::CURSE_OF_AGONY: return "Bane of Agony";
            case PriorityAction::CURSE_OF_DOOM: return "Bane of Doom";
            case PriorityAction::IMMOLATE: return "Immolate";
            case PriorityAction::CONFLAGRATE: return "Conflagrate";
            case PriorityAction::SHADOWBURN: return "Shadowburn";
            case PriorityAction::SHADOWBURN_ISB: return "Shadowburn";
            case PriorityAction::DRAIN_HOPE: return "Wrack";
            case PriorityAction::INCINERATE_FILLER: return "Incinerate";
            case PriorityAction::SEARING_PAIN_FILLER: return "Searing Pain";
            case PriorityAction::DRAIN_LIFE_FILLER: return "Drain Life";
            case PriorityAction::DRAIN_SOUL_FILLER: return "Drain Soul";
            case PriorityAction::SHADOW_BOLT_RANK2: return "Shadow Bolt Rank 2";
            case PriorityAction::SHADOW_BOLT_FILLER: return "Shadow Bolt";
            default: return "Unknown Action";
        }
    }

    static SpellID get_spell_id(PriorityAction action) {
        switch (action) {
            case PriorityAction::LIFE_TAP: return SpellID::LIFE_TAP;
            case PriorityAction::RACIAL_EUREKA: return SpellID::RACIAL_EUREKA;
            case PriorityAction::RACIAL_BLOOD_FURY: return SpellID::RACIAL_BLOOD_FURY;
            case PriorityAction::RACIAL_BERSERKING: return SpellID::RACIAL_BERSERKING;
            case PriorityAction::AMPLIFY_CURSE: return SpellID::AMPLIFY_CURSE;
            case PriorityAction::BANE_OF_HAVOC: return SpellID::BANE_OF_HAVOC;
            case PriorityAction::NIGHTFALL_SHADOW_BOLT: return SpellID::SHADOW_BOLT;
            case PriorityAction::DECIMATION_SOUL_FIRE: return SpellID::SOUL_FIRE;
            case PriorityAction::DECIMATION_SEARING_PAIN: return SpellID::SEARING_PAIN;
            case PriorityAction::DEMONIC_BRAND_SEARING_PAIN: return SpellID::DEMONIC_BRAND;
            case PriorityAction::CORRUPTION: return SpellID::CORRUPTION;
            case PriorityAction::SIPHON_LIFE: return SpellID::SIPHON_LIFE;
            case PriorityAction::CURSE_OF_AGONY: return SpellID::CURSE_OF_AGONY;
            case PriorityAction::CURSE_OF_DOOM: return SpellID::CURSE_OF_DOOM;
            case PriorityAction::IMMOLATE: return SpellID::IMMOLATE;
            case PriorityAction::CONFLAGRATE: return SpellID::CONFLAGRATE;
            case PriorityAction::SHADOWBURN: return SpellID::SHADOWBURN;
            case PriorityAction::SHADOWBURN_ISB: return SpellID::SHADOWBURN;
            case PriorityAction::DRAIN_HOPE: return SpellID::DRAIN_HOPE;
            case PriorityAction::INCINERATE_FILLER: return SpellID::INCINERATE;
            case PriorityAction::SEARING_PAIN_FILLER: return SpellID::SEARING_PAIN;
            case PriorityAction::DRAIN_LIFE_FILLER: return SpellID::DRAIN_LIFE;
            case PriorityAction::DRAIN_SOUL_FILLER: return SpellID::DRAIN_SOUL;
            case PriorityAction::SHADOW_BOLT_RANK2:
            case PriorityAction::SHADOW_BOLT_FILLER: return SpellID::SHADOW_BOLT;
            default: return SpellID::SHADOW_BOLT;
        }
    }

    // Evaluates legality of an action given the current observation state and talents
    static bool is_action_legal(PriorityAction action, const sim::SimObservation& obs, const Talents& talents) {
        switch (action) {
            case PriorityAction::LIFE_TAP:
                return true;
            case PriorityAction::RACIAL_EUREKA:
                return obs.eureka_charges > 0.0f;
            case PriorityAction::RACIAL_BLOOD_FURY:
            case PriorityAction::RACIAL_BERSERKING:
                return obs.cd_racial_sec <= 0.0f;
            case PriorityAction::AMPLIFY_CURSE:
                return talents.aff.amplify_curse > 0 && obs.cd_amplify_curse_sec <= 0.0f;
            case PriorityAction::CORRUPTION:
                return obs.time_remaining_sec >= 4.0f && obs.dot_corruption_rem_sec <= 0.5f;
            case PriorityAction::CURSE_OF_AGONY:
                return obs.time_remaining_sec >= 6.0f && obs.dot_agony_rem_sec <= 0.5f && obs.dot_doom_rem_sec <= 0.0f;
            case PriorityAction::CURSE_OF_DOOM:
                return obs.time_remaining_sec >= 65.0f && obs.cd_curse_of_doom_sec <= 0.0f && obs.dot_agony_rem_sec <= 0.0f;
            case PriorityAction::IMMOLATE:
                return obs.time_remaining_sec >= 3.0f && obs.dot_immolate_rem_sec <= 0.5f;
            case PriorityAction::CONFLAGRATE:
                return talents.destro.conflagrate > 0 && obs.dot_immolate_rem_sec > 0.0f && obs.cd_conflagrate_sec <= 0.0f;
            case PriorityAction::SHADOWBURN:
                return talents.destro.shadowburn > 0 && obs.cd_shadowburn_sec <= 0.0f;
            case PriorityAction::SHADOWBURN_ISB:
                return talents.destro.shadowburn > 0 && obs.cd_shadowburn_sec <= 0.0f && obs.isb_charges_rem > 0.0f;
            case PriorityAction::NIGHTFALL_SHADOW_BOLT:
                return obs.nightfall_proc_active > 0.5f;
            case PriorityAction::DECIMATION_SOUL_FIRE:
                return (talents.demo.decimation > 0 || obs.decimation_rem_sec > 0.0f) && obs.target_hp_pct <= 0.35f;
            case PriorityAction::DECIMATION_SEARING_PAIN:
                return talents.demo.decimation > 0 && obs.target_hp_pct <= 0.35f && obs.decimation_rem_sec <= 0.0f;
            case PriorityAction::SIPHON_LIFE:
                return talents.aff.siphon_life > 0 && obs.time_remaining_sec >= 6.0f && obs.dot_siphon_life_rem_sec <= 0.5f;
            case PriorityAction::DRAIN_HOPE:
                return talents.aff.drain_hope > 0;
            case PriorityAction::DEMONIC_BRAND_SEARING_PAIN:
                return talents.demo.demonic_brand > 0;
            case PriorityAction::INCINERATE_FILLER:
                return talents.destro.incinerate > 0;
            case PriorityAction::SEARING_PAIN_FILLER:
                return talents.demo.demonic_brand > 0;
            case PriorityAction::DRAIN_SOUL_FILLER:
                return talents.aff.drain_hope > 0;
            case PriorityAction::DRAIN_LIFE_FILLER:
                return talents.aff.improved_drains > 0;
            case PriorityAction::SHADOW_BOLT_FILLER:
                return true;
            default:
                return true;
        }
    }

    // Checks whether an action can ever be cast given a player's talent spec
    static bool is_action_available_for_talents(PriorityAction action, const Talents& talents) {
        switch (action) {
            case PriorityAction::AMPLIFY_CURSE:
                return talents.aff.amplify_curse > 0;
            case PriorityAction::SIPHON_LIFE:
                return talents.aff.siphon_life > 0;
            case PriorityAction::DRAIN_HOPE:
                return talents.aff.drain_hope > 0;
            case PriorityAction::DECIMATION_SOUL_FIRE:
            case PriorityAction::DECIMATION_SEARING_PAIN:
                return talents.demo.decimation > 0;
            case PriorityAction::DEMONIC_BRAND_SEARING_PAIN:
                return talents.demo.demonic_brand > 0;
            case PriorityAction::CONFLAGRATE:
                return talents.destro.conflagrate > 0;
            case PriorityAction::SHADOWBURN:
            case PriorityAction::SHADOWBURN_ISB:
                return talents.destro.shadowburn > 0;
            case PriorityAction::INCINERATE_FILLER:
                return talents.destro.incinerate > 0;
            case PriorityAction::SEARING_PAIN_FILLER:
                return talents.demo.demonic_brand > 0;
            case PriorityAction::DRAIN_SOUL_FILLER:
                return talents.aff.drain_hope > 0;
            case PriorityAction::DRAIN_LIFE_FILLER:
                return talents.aff.improved_drains > 0;
            case PriorityAction::RACIAL_EUREKA:
            case PriorityAction::RACIAL_BLOOD_FURY:
            case PriorityAction::RACIAL_BERSERKING:
            case PriorityAction::BANE_OF_HAVOC:
                return false; // Off-GCD cooldowns or multi-target handled separately
            default:
                return true;
        }
    }

    // Creates a valid default PriorityRule for an action
    static PriorityRule create_default_rule_for_action(PriorityAction action, const Talents& talents) {
        PriorityRule r;
        r.action = action;
        r.spell_id = get_spell_id(action);
        r.name = get_action_name(action);
        r.enabled = true;
        r.use_custom_thresholds = true;

        if (action == PriorityAction::LIFE_TAP) {
            r.check_mana = true;
            r.max_mana_pct = 0.35f;
            r.min_hp_pct = 0.15f;
            r.condition_summary = r.format_condition_summary();
        } else if (action == PriorityAction::CURSE_OF_DOOM) {
            r.check_fight_time = true;
            r.min_time_remaining = 65.0f;
            r.check_doom_debuff = true;
            r.require_doom_missing = true;
            r.condition_summary = r.format_condition_summary();
        } else if (action == PriorityAction::DECIMATION_SOUL_FIRE) {
            r.check_target_hp = true;
            r.max_target_hp_pct = 0.35f;
            r.check_decimation = true;
            r.require_decimation_active = true;
            r.name = "Decimation Soul Fire";
            r.condition_summary = r.format_condition_summary();
        } else if (action == PriorityAction::DECIMATION_SEARING_PAIN) {
            r.check_target_hp = true;
            r.max_target_hp_pct = 0.35f;
            r.check_decimation = true;
            r.require_decimation_active = false;
            r.name = "Decimation Trigger (Searing Pain)";
            r.condition_summary = r.format_condition_summary();
        } else if (action == PriorityAction::NIGHTFALL_SHADOW_BOLT) {
            r.check_shadow_trance = true;
            r.condition_summary = r.format_condition_summary();
        } else if (action == PriorityAction::SHADOWBURN_ISB) {
            r.check_isb_debuff = true;
            r.require_isb_active = true;
            r.condition_summary = r.format_condition_summary();
        } else if (action == PriorityAction::CORRUPTION || action == PriorityAction::IMMOLATE || action == PriorityAction::SIPHON_LIFE || action == PriorityAction::CURSE_OF_AGONY) {
            r.check_dot_refresh = true;
            r.max_dot_rem_sec = 0.0f;
            r.condition_summary = r.format_condition_summary();
        } else {
            r.use_custom_thresholds = false;
            r.trigger_condition = "(Always)";
            r.condition_summary = "Always / Filler";
        }
        r.rule_explanation = "Candidate Action Injection";
        return r;
    }

    // Three slots per available action, including the fixed final filler slot.
    // Start unused slots disabled (Never), without losing their mutable predicates.
    static std::vector<PriorityRule> make_search_slots(const std::vector<PriorityRule>& seed_rules,
                                                     const WarlockSimulator& sim) {
        const auto candidates = get_candidate_actions(sim.mechanics.allow_rank2_shadow_bolt);
        auto available = [&](PriorityAction action) {
            return is_action_available_for_talents(action, sim.talents) &&
                std::any_of(candidates.begin(), candidates.end(), [&](const auto& c) { return c.first == action; });
        };
        const auto filler = sim.talents.destro.incinerate > 0
            ? PriorityAction::INCINERATE_FILLER : PriorityAction::SHADOW_BOLT_FILLER;
        std::vector<PriorityRule> rules;
        std::unordered_map<int, size_t> counts;
        // Reserve one filler slot for an unconditional final fallback.
        for (const auto& r : seed_rules) {
            const size_t limit = r.action == filler ? 2 : 3;
            if (r.action == filler && !r.use_custom_thresholds) continue;
            if (available(r.action) && counts[int(r.action)] < limit) {
                rules.push_back(r);
                ++counts[int(r.action)];
            }
        }
        for (const auto& [action, spell] : candidates) {
            if (!available(action)) continue;
            const size_t limit = action == filler ? 2 : 3;
            while (counts[int(action)] < limit) {
                auto r = create_default_rule_for_action(action, sim.talents);
                r.enabled = false;
                r.condition_summary = "Never (disabled)";
                rules.push_back(r);
                ++counts[int(action)];
            }
        }
        rules.push_back(create_default_rule_for_action(filler, sim.talents));
        return rules;
    }

    static void mutate_search_rule(PriorityRule& r, FastRNG& rng) {
        auto unit = [&]() { return static_cast<float>(rng.next_double()); };
        auto toggle = [&]() { return rng.next_double() < 0.5; };
        r.use_custom_thresholds = true;
        switch (rng.next_u64() % 11) {
            case 0: r.enabled = !r.enabled; break;
            case 1:
                r.check_mana = toggle();
                r.min_mana_pct = r.check_mana ? unit() : 0.0f;
                r.max_mana_pct = r.check_mana ? unit() : 1.0f;
                if (r.min_mana_pct > r.max_mana_pct) std::swap(r.min_mana_pct, r.max_mana_pct);
                break;
            case 2:
                r.check_target_hp = toggle();
                r.min_target_hp_pct = r.check_target_hp ? unit() : 0.0f;
                r.max_target_hp_pct = r.check_target_hp ? unit() : 1.0f;
                if (r.min_target_hp_pct > r.max_target_hp_pct) std::swap(r.min_target_hp_pct, r.max_target_hp_pct);
                break;
            case 3:
                r.check_fight_time = toggle();
                r.min_time_remaining = r.check_fight_time && toggle() ? unit() * 90.0f : 0.0f;
                r.max_time_remaining = r.check_fight_time && toggle() ? r.min_time_remaining + unit() * 90.0f : 9999.0f;
                break;
            case 4:
                r.check_dot_refresh = true;
                r.max_dot_rem_sec = unit() * 5.0f;
                break;
            case 5:
                r.check_isb_debuff = toggle();
                r.require_isb_active = r.check_isb_debuff;
                r.min_isb_rem_sec = 0.0f;
                break;
            case 6: r.check_shadow_trance = toggle(); break;
            case 7:
                r.check_decimation = toggle();
                r.require_decimation_active = toggle();
                break;
            case 8:
                r.check_demonic_brand = toggle();
                r.require_demonic_brand_missing = toggle();
                break;
            case 9:
                r.check_doom_debuff = toggle();
                r.require_doom_missing = toggle();
                break;
            case 10:
                r.max_mana_pct = std::clamp(r.max_mana_pct + (unit() - 0.5f) * 0.2f, r.min_mana_pct, 1.0f);
                r.max_target_hp_pct = std::clamp(r.max_target_hp_pct + (unit() - 0.5f) * 0.1f, r.min_target_hp_pct, 1.0f);
                if (r.check_fight_time) r.min_time_remaining = std::clamp(r.min_time_remaining + (unit() - 0.5f) * 10.0f, 0.0f, r.max_time_remaining);
                if (r.check_dot_refresh) r.max_dot_rem_sec = std::clamp(r.max_dot_rem_sec + unit() - 0.5f, 0.0f, 5.0f);
                break;
        }
        r.condition_summary.clear();
        r.condition_summary = r.enabled ? r.format_condition_summary() : "Never (disabled)";
        r.trigger_condition = r.condition_summary;
        r.rule_explanation = "Independently optimized action slot";
    }

    // Evaluates state-dependent local Q-value of taking action 'act' in observation state 'obs'
    static double estimate_local_q_value(const sim::SimObservation& obs, PriorityAction action, const Talents& talents) {
        if (!is_action_legal(action, obs, talents)) {
            return -1000.0;
        }

        switch (action) {
            case PriorityAction::LIFE_TAP: {
                if (obs.player_mana_pct <= 0.15f) return 3200.0; // Critical tap to avoid OOM
                if (obs.player_mana_pct <= 0.35f && obs.time_remaining_sec >= 15.0f) return 1800.0;
                if (obs.player_mana_pct <= 0.50f && obs.time_remaining_sec >= 30.0f) return 1400.0;
                return -500.0; // Unnecessary tap
            }
            case PriorityAction::NIGHTFALL_SHADOW_BOLT: {
                if (obs.nightfall_proc_active > 0.5f) return 2700.0;
                return -1000.0;
            }
            case PriorityAction::DECIMATION_SOUL_FIRE: {
                if (obs.target_hp_pct <= 0.35f && (talents.demo.decimation > 0 || obs.decimation_rem_sec > 0.0f)) return 3000.0;
                return -1000.0;
            }
            case PriorityAction::DECIMATION_SEARING_PAIN: {
                if (obs.target_hp_pct <= 0.35f && talents.demo.decimation > 0 && obs.decimation_rem_sec <= 0.0f) return 2400.0;
                return -1000.0;
            }
            case PriorityAction::CONFLAGRATE: {
                if (talents.destro.conflagrate > 0 && obs.dot_immolate_rem_sec > 0.0f && obs.cd_conflagrate_sec <= 0.0f) return 2300.0;
                return -1000.0;
            }
            case PriorityAction::SHADOWBURN: {
                if (talents.destro.shadowburn > 0 && obs.cd_shadowburn_sec <= 0.0f) {
                    if (obs.target_hp_pct <= 0.20f || obs.time_remaining_sec <= 8.0f) return 2200.0;
                    return 1700.0;
                }
                return -1000.0;
            }
            case PriorityAction::SHADOWBURN_ISB: {
                if (talents.destro.shadowburn > 0 && obs.cd_shadowburn_sec <= 0.0f && obs.isb_charges_rem > 0.0f) {
                    if (obs.target_hp_pct <= 0.20f || obs.time_remaining_sec <= 8.0f) return 2300.0;
                    return 1900.0;
                }
                return -1000.0;
            }
            case PriorityAction::CURSE_OF_DOOM: {
                if (obs.time_remaining_sec >= 65.0f && obs.cd_curse_of_doom_sec <= 0.0f && obs.dot_agony_rem_sec <= 0.0f) return 2400.0;
                return -1000.0;
            }
            case PriorityAction::CORRUPTION: {
                if (obs.dot_corruption_rem_sec <= 0.5f && obs.time_remaining_sec >= 6.0f) return 2100.0;
                return -1000.0;
            }
            case PriorityAction::IMMOLATE: {
                if (obs.dot_immolate_rem_sec <= 0.5f && obs.time_remaining_sec >= 4.0f) return 2050.0;
                return -1000.0;
            }
            case PriorityAction::CURSE_OF_AGONY: {
                if (obs.dot_agony_rem_sec <= 0.5f && obs.time_remaining_sec >= 8.0f && obs.dot_doom_rem_sec <= 0.0f) return 1800.0;
                return -1000.0;
            }
            case PriorityAction::SIPHON_LIFE: {
                if (talents.aff.siphon_life > 0 && obs.dot_siphon_life_rem_sec <= 0.5f && obs.time_remaining_sec >= 12.0f) return 1650.0;
                return -1000.0;
            }
            case PriorityAction::INCINERATE_FILLER: {
                if (talents.destro.incinerate > 0) return 1300.0;
                return 900.0;
            }
            case PriorityAction::SEARING_PAIN_FILLER: {
                return 1100.0;
            }
            case PriorityAction::DRAIN_SOUL_FILLER: {
                return 1000.0;
            }
            case PriorityAction::SHADOW_BOLT_FILLER:
            default: {
                return 1250.0;
            }
        }
    }

    struct DAggerIterationLog {
        size_t iteration = 0;
        size_t samples_added = 0;
        size_t total_samples = 0;
        double candidate_dps = 0.0;
        double tree_weighted_fidelity_pct = 0.0;
        double tree_unweighted_accuracy_pct = 0.0;
        size_t tree_depth = 0;
        size_t tree_leaf_count = 0;
    };

    struct VIPERExtractionResult {
        // 1. Expected Value Metrics
        double baseline_expected_dps = 0.0;
        double baseline_dps_stddev = 0.0;
        double baseline_min_dps = 0.0;
        double baseline_max_dps = 0.0;

        double oracle_expected_dps = 0.0;
        double oracle_dps_stddev = 0.0;
        double oracle_min_dps = 0.0;
        double oracle_max_dps = 0.0;
        double oracle_expected_gain = 0.0;
        double oracle_gain_pct = 0.0;

        double viper_expected_dps = 0.0;
        double viper_dps_stddev = 0.0;
        double viper_min_dps = 0.0;
        double viper_max_dps = 0.0;
        double viper_gain_over_baseline = 0.0;
        double viper_gain_pct = 0.0;

        double oracle_potential_captured_pct = 0.0;
        double oracle_agreement_fidelity_pct = 0.0;

        // 2. Rollout & Search Details
        size_t total_samples_collected = 0;
        size_t episodes_run = 0;
        size_t benchmark_iterations = 0;
        size_t dagger_iterations_run = 0;
        double elapsed_time_ms = 0.0;
        std::vector<DAggerIterationLog> dagger_history;

        // 3. Actions & Rules
        std::vector<PriorityRule> extracted_rules;
        std::vector<PriorityRule> baseline_rules;
        std::vector<ActionStat> action_stats;
        std::vector<RuleShift> rule_shifts;
        std::string summary_report;

        // 4. Embedded CART Decision Tree & Extracted Continuous Conditions
        sim::DecisionTreeConfig tree_config;
        size_t tree_depth = 0;
        size_t tree_leaf_count = 0;
        double tree_weighted_fidelity_pct = 0.0;
        double tree_unweighted_accuracy_pct = 0.0;
        std::string generated_cpp_code;
        std::string tree_ascii_visualization;
        std::vector<sim::ExtractedDecisionRule> decision_rules;

        // 5. GBDT Q-Policy (LightGBM/Tree Ensemble Policy)
        sim::GBDTMultiActionQPolicy gbdt_q_policy;
        double gbdt_policy_expected_dps = 0.0;
        double gbdt_policy_dps_stddev = 0.0;
        double gbdt_policy_gain_pct = 0.0;
        double gbdt_potential_captured_pct = 0.0;
        std::vector<std::pair<std::string, double>> gbdt_feature_importances;
    };

    // Checks if two PriorityRules have identical action and identical triggering conditions
    static bool are_rules_identical_condition(const PriorityRule& a, const PriorityRule& b) {
        if (a.action != b.action) return false;
        if (a.use_custom_thresholds != b.use_custom_thresholds) return false;
        if (!a.use_custom_thresholds) return true; // Both are default/unconditional for this action
        
        return (std::abs(a.max_mana_pct - b.max_mana_pct) < 1e-4f &&
                std::abs(a.min_mana_pct - b.min_mana_pct) < 1e-4f &&
                std::abs(a.max_target_hp_pct - b.max_target_hp_pct) < 1e-4f &&
                std::abs(a.min_target_hp_pct - b.min_target_hp_pct) < 1e-4f &&
                std::abs(a.min_time_remaining - b.min_time_remaining) < 1e-4f &&
                std::abs(a.max_time_remaining - b.max_time_remaining) < 1e-4f &&
                std::abs(a.max_dot_rem_sec - b.max_dot_rem_sec) < 1e-4f &&
                std::abs(a.min_hp_pct - b.min_hp_pct) < 1e-4f &&
                a.min_isb_rem_sec == b.min_isb_rem_sec &&
                a.require_isb_active == b.require_isb_active &&
                a.check_mana == b.check_mana && a.check_target_hp == b.check_target_hp &&
                a.check_fight_time == b.check_fight_time && a.check_dot_refresh == b.check_dot_refresh &&
                a.check_isb_debuff == b.check_isb_debuff && a.check_shadow_trance == b.check_shadow_trance &&
                a.check_decimation == b.check_decimation && a.require_decimation_active == b.require_decimation_active &&
                a.check_demonic_brand == b.check_demonic_brand && a.require_demonic_brand_missing == b.require_demonic_brand_missing &&
                a.check_doom_debuff == b.check_doom_debuff && a.require_doom_missing == b.require_doom_missing);
    }

    // Checks if a PriorityRule is unconstrained / unconditional (Always triggers)
    static bool is_rule_unconditional(const PriorityRule& r) {
        if (!r.use_custom_thresholds) return true;
        if (r.trigger_condition == "(Always)" || r.trigger_condition == "Always" || r.trigger_condition.empty()) {
            if (r.max_mana_pct >= 1.0f && r.min_mana_pct <= 0.0f &&
                r.max_target_hp_pct >= 1.0f && r.min_target_hp_pct <= 0.0f &&
                r.min_time_remaining <= 0.0f && r.max_time_remaining >= 999.0f &&
                r.max_dot_rem_sec <= 0.0f && r.min_hp_pct <= 0.0f) {
                return true;
            }
        }
        return false;
    }

    // Condenses and deduplicates sequential / redundant rules in a PriorityRule chain
    static std::vector<PriorityRule> condense_and_deduplicate_rules(const std::vector<PriorityRule>& rules) {
        std::vector<PriorityRule> result;
        for (const auto& r : rules) {
            if (!r.enabled) continue;

            // Check if identical to the previous rule in sequence (consecutive duplicate)
            if (!result.empty() && are_rules_identical_condition(result.back(), r)) {
                continue;
            }

            // Remove only exact duplicates: default action behavior can itself be conditional.
            bool redundant = false;
            for (const auto& earlier : result) {
                if (earlier.action == r.action) {
                    if (are_rules_identical_condition(earlier, r)) {
                        redundant = true;
                        break;
                    }
                }
            }

            if (!redundant) {
                result.push_back(r);
            }
        }
        return result;
    }

    // Compiles Decision Tree Leaf Paths directly into a Multi-Instance Parameterized APL
    static std::vector<PriorityRule> compile_tree_to_apl(
        const sim::DecisionTreeClassifier& tree,
        const Talents& talents,
        Race race)
    {
        std::vector<PriorityRule> synthesized_rules;
        auto decision_rules = tree.extract_rules();

        // Sort extracted rules by confidence and sample count
        std::sort(decision_rules.begin(), decision_rules.end(), [](const sim::ExtractedDecisionRule& a, const sim::ExtractedDecisionRule& b) {
            if (a.confidence != b.confidence) return a.confidence > b.confidence;
            return a.sample_count > b.sample_count;
        });

        // Convert each valid leaf into a parameterized PriorityRule
        for (const auto& drule : decision_rules) {
            PriorityAction act = static_cast<PriorityAction>(drule.action);
            if (drule.sample_count < 2) continue; // Filter noisy 1-sample leaves

            PriorityRule r;
            r.action = act;
            r.spell_id = get_spell_id(act);
            r.name = get_action_name(act);
            r.enabled = true;
            r.use_custom_thresholds = drule.has_custom_bounds;
            r.max_mana_pct = drule.max_mana_pct;
            r.min_mana_pct = drule.min_mana_pct;
            r.max_target_hp_pct = drule.max_target_hp_pct;
            r.min_target_hp_pct = drule.min_target_hp_pct;
            r.min_time_remaining = drule.min_time_remaining;
            r.max_time_remaining = drule.max_time_remaining;
            r.max_dot_rem_sec = drule.max_dot_rem_sec;
            r.min_hp_pct = drule.min_hp_pct;

            std::string conds;
            for (size_t i = 0; i < drule.conditions.size(); ++i) {
                if (i > 0) conds += " & ";
                conds += drule.conditions[i];
            }
            r.trigger_condition = conds.empty() ? "(Always)" : conds;
            r.condition_summary = r.trigger_condition;
            r.rule_explanation = "Decision Tree Leaf (" + std::to_string(drule.sample_count) + " samples, " + 
                                 std::to_string(static_cast<int>(drule.confidence * 100.0)) + "% purity)";

            synthesized_rules.push_back(r);
        }

        // Condense duplicate leaves and remove shadowed redundancies
        synthesized_rules = condense_and_deduplicate_rules(synthesized_rules);

        // Ensure at least one valid fallback filler rule exists at the end
        bool has_filler = false;
        for (const auto& r : synthesized_rules) {
            if (r.action == PriorityAction::SHADOW_BOLT_FILLER ||
                r.action == PriorityAction::INCINERATE_FILLER ||
                r.action == PriorityAction::SEARING_PAIN_FILLER) {
                has_filler = true;
                break;
            }
        }

        if (!has_filler) {
            PriorityRule fallback_filler;
            fallback_filler.action = (talents.destro.incinerate > 0) ? PriorityAction::INCINERATE_FILLER : PriorityAction::SHADOW_BOLT_FILLER;
            fallback_filler.spell_id = get_spell_id(fallback_filler.action);
            fallback_filler.name = get_action_name(fallback_filler.action);
            fallback_filler.enabled = true;
            fallback_filler.use_custom_thresholds = false;
            fallback_filler.condition_summary = "Fallback Filler";
            fallback_filler.trigger_condition = "Always";
            fallback_filler.rule_explanation = "Default filler fallback";
            synthesized_rules.push_back(fallback_filler);
        }

        return synthesized_rules;
    }

    // Collect visited states and label them using heuristic local action scores.
    static sim::VIPERDataset collect_viper_dataset(
        WarlockSimulator& sim,
        size_t num_episodes,
        double& out_oracle_dps,
        uint64_t seed = 42)
    {
        sim::VIPERDataset dataset;
        if (num_episodes == 0) {
            out_oracle_dps = 0.0;
            return dataset;
        }

        unsigned int hw_threads = std::max(1u, std::thread::hardware_concurrency());
        size_t num_threads = std::min(static_cast<size_t>(hw_threads), num_episodes);

        auto candidate_actions = get_candidate_actions(sim.mechanics.allow_rank2_shadow_bolt);

        struct ThreadOutput {
            sim::VIPERDataset local_dataset;
            double oracle_dps_sum = 0.0;
            size_t episodes_done = 0;
        };

        std::vector<ThreadOutput> outputs(num_threads);
        std::vector<std::thread> workers;

        size_t eps_per_thread = num_episodes / num_threads;
        size_t rem_eps = num_episodes % num_threads;

        for (size_t t = 0; t < num_threads; ++t) {
            size_t start_ep = t * eps_per_thread + std::min(t, rem_eps);
            size_t count = eps_per_thread + (t < rem_eps ? 1 : 0);

            workers.emplace_back([&, t, start_ep, count]() {
                FastRNG rng(seed + t * 7919 + start_ep * 31);
                for (size_t ep_idx = 0; ep_idx < count; ++ep_idx) {
                    WarlockSimulator ep_sim = sim;
                    ep_sim.record_timeline = false;
                    ep_sim.record_viper_samples = true;
                    ep_sim.viper_dataset.clear();

                    SimResult ep_res = ep_sim.run_single_simulation(rng);
                    outputs[t].oracle_dps_sum += ep_res.dps;
                    outputs[t].episodes_done++;

                    for (auto& sample : ep_sim.viper_dataset.samples) {
                        PriorityAction best_action = static_cast<PriorityAction>(sample.oracle_action);
                        double max_q = -9999.0;
                        double second_max_q = -9999.0;

                        for (const auto& [act, _] : candidate_actions) {
                            if (is_action_legal(act, sample.state, ep_sim.talents)) {
                                double q_val = estimate_local_q_value(sample.state, act, ep_sim.talents);
                                if (q_val > max_q) {
                                    second_max_q = max_q;
                                    max_q = q_val;
                                    best_action = act;
                                } else if (q_val > second_max_q) {
                                    second_max_q = q_val;
                                }
                            }
                        }

                        sample.oracle_action = static_cast<uint8_t>(best_action);
                        sample.action_name = get_action_name(best_action);
                        sample.sample_weight = static_cast<float>(std::max(0.1, (max_q - second_max_q) / 100.0));

                        outputs[t].local_dataset.add_sample(sample);
                    }
                }
            });
        }

        for (auto& w : workers) {
            if (w.joinable()) w.join();
        }

        double total_oracle_dps = 0.0;
        for (auto& out : outputs) {
            total_oracle_dps += out.oracle_dps_sum;
            for (const auto& s : out.local_dataset.samples) {
                dataset.add_sample(s);
            }
        }

        out_oracle_dps = total_oracle_dps / static_cast<double>(std::max(size_t(1), num_episodes));
        return dataset;
    }

    static sim::VIPERDataset collect_viper_dataset(
        WarlockSimulator& sim,
        size_t num_episodes = 20,
        uint64_t seed = 42)
    {
        double dummy_dps = 0.0;
        return collect_viper_dataset(sim, num_episodes, dummy_dps, seed);
    }

    // Comprehensive statistical evaluation of a simulator configuration with multi-threading
    struct StatSummary {
        double mean_dps = 0.0;
        double stddev_dps = 0.0;
        double min_dps = 0.0;
        double max_dps = 0.0;
    };

    static StatSummary compute_expected_dps(WarlockSimulator sim_copy, size_t iterations = 500, uint64_t seed = 42) {
        StatSummary summary;
        if (iterations == 0) return summary;

        unsigned int hw_threads = std::max(1u, std::thread::hardware_concurrency());
        size_t num_threads = std::min(static_cast<size_t>(hw_threads), iterations);

        struct WorkerResult {
            double sum_dps = 0.0;
            double sum_sq_dps = 0.0;
            double min_dps = 1e9;
            double max_dps = 0.0;
            size_t count = 0;
        };

        std::vector<WorkerResult> results(num_threads);
        std::vector<std::thread> workers;

        size_t iters_per_thread = iterations / num_threads;
        size_t rem = iterations % num_threads;

        for (size_t t = 0; t < num_threads; ++t) {
            size_t count = iters_per_thread + (t < rem ? 1 : 0);
            workers.emplace_back([&, t, count]() {
                FastRNG rng(seed + t * 65537);
                WarlockSimulator local_sim = sim_copy;
                local_sim.record_timeline = false;
                local_sim.record_viper_samples = false;

                for (size_t i = 0; i < count; ++i) {
                    SimResult res = local_sim.run_single_simulation(rng);
                    results[t].sum_dps += res.dps;
                    results[t].sum_sq_dps += res.dps * res.dps;
                    if (res.dps < results[t].min_dps) results[t].min_dps = res.dps;
                    if (res.dps > results[t].max_dps) results[t].max_dps = res.dps;
                    results[t].count++;
                }
            });
        }

        for (auto& w : workers) {
            if (w.joinable()) w.join();
        }

        double total_sum = 0.0;
        double total_sq = 0.0;
        summary.min_dps = 1e9;
        summary.max_dps = 0.0;

        for (const auto& r : results) {
            total_sum += r.sum_dps;
            total_sq += r.sum_sq_dps;
            if (r.min_dps < summary.min_dps) summary.min_dps = r.min_dps;
            if (r.max_dps > summary.max_dps) summary.max_dps = r.max_dps;
        }

        summary.mean_dps = total_sum / static_cast<double>(iterations);
        double variance = std::max(0.0, (total_sq / static_cast<double>(iterations)) - (summary.mean_dps * summary.mean_dps));
        summary.stddev_dps = std::sqrt(variance);
        return summary;
    }

    // Legacy API name: benchmarks the greedy local-Q heuristic, not MCTS.
    static StatSummary benchmark_live_mcts_oracle(WarlockSimulator sim_copy, size_t iterations = 500, uint64_t seed = 42) {
        WarlockSimulator oracle_sim = sim_copy;
        oracle_sim.use_oracle_execution_policy = true;
        oracle_sim.record_timeline = false;
        oracle_sim.record_viper_samples = false;
        return compute_expected_dps(oracle_sim, iterations, seed);
    }

    // Direct APL search with a separate heuristic-labeled diagnostic tree.
    static VIPERExtractionResult extract_viper_apl(
        WarlockSimulator& input_sim,
        size_t num_episodes,
        size_t benchmark_iterations,
        size_t dagger_iterations,
        std::function<void(float progress, const std::string& status)> progress_cb = nullptr,
        uint64_t seed = 42)
    {
        WarlockSimulator sim = input_sim;
        sim.use_oracle_execution_policy = false;
        sim.policy.use_oracle_execution_policy = false;
        sim.use_gbdt_policy = false;
        sim.policy.use_gbdt_policy = false;
        sim.policy.use_imitation_policy = false;
        sim.neural_decision = {};
        sim.decision_controller = {};
        sim.forced_action_prefix.clear();
        VIPERExtractionResult res;
        res.episodes_run = num_episodes;
        res.benchmark_iterations = benchmark_iterations;
        res.dagger_iterations_run = std::max(size_t(1), dagger_iterations);

        auto report_progress = [&](float p, const std::string& msg) {
            if (progress_cb) progress_cb(p, msg);
        };

        // 1. Measure Baseline Expected Value
        report_progress(0.05f, "Phase 1/4: Measuring Baseline Policy Expected Value...");
        StatSummary base_stats = compute_expected_dps(sim, benchmark_iterations, seed);
        res.baseline_expected_dps = base_stats.mean_dps;
        res.baseline_dps_stddev = base_stats.stddev_dps;
        res.baseline_min_dps = base_stats.min_dps;
        res.baseline_max_dps = base_stats.max_dps;
        res.baseline_rules = sim.policy.use_custom_apl ? sim.policy.custom_rules : sim.policy.build_preset_rules(sim.talents, sim.race);

        // 2. Greedy heuristic reference (not an upper bound).
        report_progress(0.18f, "Phase 2/4: Greedy Heuristic Reference Benchmark...");
        StatSummary oracle_stats = benchmark_live_mcts_oracle(sim, benchmark_iterations, seed);
        res.oracle_expected_dps = oracle_stats.mean_dps;
        res.oracle_dps_stddev = oracle_stats.stddev_dps;
        res.oracle_min_dps = oracle_stats.min_dps;
        res.oracle_max_dps = oracle_stats.max_dps;
        res.oracle_expected_gain = res.oracle_expected_dps - res.baseline_expected_dps;
        res.oracle_gain_pct = (res.oracle_expected_gain / std::max(1.0, res.baseline_expected_dps)) * 100.0;

        // Direct APL search: each action owns at most three independently conditioned
        // slots. Disabled slots remain in the chromosome and can be reactivated.
        auto candidate_actions = get_candidate_actions(sim.mechanics.allow_rank2_shadow_bolt);
        struct APLIndividual {
            std::vector<PriorityRule> rules;
            double fitness_dps = 0.0;
        };
        const size_t POP_SIZE = 28;
        const size_t NUM_GENERATIONS = std::max(size_t(12), dagger_iterations * 6);
        const size_t EVAL_ITERS = std::max(size_t(50), benchmark_iterations / 4);
        FastRNG ga_rng(seed + 8888);
        auto quick_eval = [&](const std::vector<PriorityRule>& rules) {
            WarlockSimulator test_sim = sim;
            test_sim.policy.custom_rules = rules;
            test_sim.policy.use_custom_apl = true;
            test_sim.record_viper_samples = false;
            return compute_expected_dps(test_sim, EVAL_ITERS, seed + 101).mean_dps;
        };

        std::vector<APLIndividual> population;
        APLIndividual baseline;
        baseline.rules = make_search_slots(res.baseline_rules, sim);
        population.push_back(baseline);
        // Also seed the former search's broad action ordering: actions absent
        // from the user's baseline should not require several lucky mutations.
        std::vector<PriorityRule> natural_rules;
        for (auto action : {PriorityAction::LIFE_TAP, PriorityAction::NIGHTFALL_SHADOW_BOLT,
             PriorityAction::DECIMATION_SOUL_FIRE, PriorityAction::DECIMATION_SEARING_PAIN,
             PriorityAction::CONFLAGRATE, PriorityAction::SHADOWBURN_ISB,
             PriorityAction::CURSE_OF_DOOM, PriorityAction::CURSE_OF_AGONY,
             PriorityAction::CORRUPTION, PriorityAction::IMMOLATE, PriorityAction::SIPHON_LIFE,
             PriorityAction::DRAIN_HOPE, PriorityAction::SHADOWBURN, PriorityAction::LIFE_TAP}) {
            auto rule = create_default_rule_for_action(action, sim.talents);
            if (action == PriorityAction::LIFE_TAP)
                rule.max_mana_pct = natural_rules.empty() ? 0.15f : 0.38f;
            if (action == PriorityAction::SHADOWBURN) {
                rule.use_custom_thresholds = true;
                rule.check_target_hp = true;
                rule.max_target_hp_pct = 0.35f;
            }
            natural_rules.push_back(rule);
        }
        APLIndividual natural;
        natural.rules = make_search_slots(natural_rules, sim);
        population.push_back(natural);
        while (population.size() < POP_SIZE) {
            APLIndividual ind = population.size() % 2 ? baseline : natural;
            // Keep some local variants and some globally shuffled priorities.
            if (population.size() % 3 != 0) {
                for (size_t i = ind.rules.size() - 2; i > 0; --i)
                    std::swap(ind.rules[i], ind.rules[ga_rng.next_u64() % (i + 1)]);
            }
            for (size_t i = 0; i + 1 < ind.rules.size(); ++i) {
                if (ga_rng.next_double() < 0.5) mutate_search_rule(ind.rules[i], ga_rng);
            }
            population.push_back(std::move(ind));
        }
        for (auto& ind : population) ind.fitness_dps = quick_eval(ind.rules);
        auto fitter = [](const APLIndividual& a, const APLIndividual& b) {
            return a.fitness_dps > b.fitness_dps;
        };
        std::sort(population.begin(), population.end(), fitter);
        APLIndividual best_overall = population.front();
        size_t evaluations = POP_SIZE;
        size_t previous_evaluations = 0;
        for (size_t gen = 0; gen < NUM_GENERATIONS; ++gen) {
            report_progress(0.25f + 0.55f * float(gen) / float(NUM_GENERATIONS),
                "Phase 3/4: Independent-rule APL search (Gen " + std::to_string(gen + 1) +
                "/" + std::to_string(NUM_GENERATIONS) + " | Best: " +
                std::to_string(int(best_overall.fitness_dps)) + " DPS)...");
            auto select = [&]() -> const APLIndividual& {
                size_t best = ga_rng.next_u64() % population.size();
                for (int k = 0; k < 2; ++k) {
                    size_t other = ga_rng.next_u64() % population.size();
                    if (population[other].fitness_dps > population[best].fitness_dps) best = other;
                }
                return population[best];
            };
            std::vector<APLIndividual> next(population.begin(), population.begin() + 4);
            while (next.size() < POP_SIZE) {
                APLIndividual child = select();
                const auto& donor = select();
                const size_t n = child.rules.size() - 1;
                // Transfer a whole condition set for the same action, never shared genes.
                if (ga_rng.next_double() < 0.5) {
                    size_t i = ga_rng.next_u64() % n;
                    std::vector<size_t> matches;
                    for (size_t j = 0; j + 1 < donor.rules.size(); ++j)
                        if (donor.rules[j].action == child.rules[i].action) matches.push_back(j);
                    if (!matches.empty()) child.rules[i] = donor.rules[matches[ga_rng.next_u64() % matches.size()]];
                }
                if (ga_rng.next_double() < 0.7)
                    std::swap(child.rules[ga_rng.next_u64() % n], child.rules[ga_rng.next_u64() % n]);
                const size_t mutations = 1 + ga_rng.next_u64() % 3;
                for (size_t m = 0; m < mutations; ++m)
                    mutate_search_rule(child.rules[ga_rng.next_u64() % n], ga_rng);
                child.fitness_dps = quick_eval(child.rules);
                ++evaluations;
                next.push_back(std::move(child));
            }
            population = std::move(next);
            std::sort(population.begin(), population.end(), fitter);
            if (population.front().fitness_dps > best_overall.fitness_dps) best_overall = population.front();
            if ((gen + 1) % std::max(size_t(1), NUM_GENERATIONS / res.dagger_iterations_run) == 0 || gen + 1 == NUM_GENERATIONS) {
                DAggerIterationLog log;
                log.iteration = res.dagger_history.size() + 1;
                // These fields count fitness simulations, not oracle training samples.
                log.samples_added = (evaluations - previous_evaluations) * EVAL_ITERS;
                log.total_samples = evaluations * EVAL_ITERS;
                log.candidate_dps = best_overall.fitness_dps;
                log.tree_leaf_count = std::count_if(best_overall.rules.begin(), best_overall.rules.end(),
                    [](const PriorityRule& r) { return r.enabled; });
                res.dagger_history.push_back(log);
                previous_evaluations = evaluations;
            }
        }
        // Only clean up after search; disabled slots must survive mutation/crossover.
        res.extracted_rules = condense_and_deduplicate_rules(best_overall.rules);

        // Collect rollout samples from champion policy for Decision Tree & telemetry
        WarlockSimulator final_rollout_sim = sim;
        final_rollout_sim.policy.custom_rules = best_overall.rules;
        final_rollout_sim.policy.use_custom_apl = true;
        double dummy_d = 0.0;
        sim::VIPERDataset rollout_dataset = collect_viper_dataset(final_rollout_sim, std::max(size_t(8), num_episodes), dummy_d, seed + 999);
        res.total_samples_collected = rollout_dataset.size();

        // Fit in-memory CART Decision Tree for AST export, ASCII view, and transpiled C++
        sim::DecisionTreeConfig dt_cfg;
        dt_cfg.max_depth = 5;
        dt_cfg.min_samples_leaf = 6;
        dt_cfg.min_samples_split = 12;
        dt_cfg.min_impurity_decrease = 1e-4;
        res.tree_config = dt_cfg;

        sim::DecisionTreeClassifier final_tree(dt_cfg);
        for (const auto& [act, _] : candidate_actions) {
            final_tree.set_class_name(static_cast<uint8_t>(act), get_action_name(act));
        }
        final_tree.fit(rollout_dataset, static_cast<size_t>(PriorityAction::SHADOW_BOLT_RANK2) + 1);

        // Tree metrics from final fitted tree
        res.tree_depth = final_tree.get_depth();
        res.tree_leaf_count = final_tree.get_leaf_count();
        res.tree_unweighted_accuracy_pct = final_tree.score_unweighted(rollout_dataset) * 100.0;
        res.tree_weighted_fidelity_pct = final_tree.score_weighted(rollout_dataset) * 100.0;
        res.generated_cpp_code = final_tree.to_cpp("evaluate_viper_policy");
        res.tree_ascii_visualization = final_tree.to_text_tree(5);
        res.decision_rules = final_tree.extract_rules();

        // Compute heuristic label statistics from the collected rollout dataset.
        std::vector<ActionStat> action_stats_map;
        for (const auto& [act, _] : candidate_actions) {
            ActionStat st;
            st.action = act;
            st.name = get_action_name(act);
            action_stats_map.push_back(st);
        }

        for (const auto& sample : rollout_dataset.samples) {
            PriorityAction act = static_cast<PriorityAction>(sample.oracle_action);
            for (auto& st : action_stats_map) {
                if (st.action == act) {
                    st.selection_count++;
                    st.avg_regret += static_cast<double>(sample.sample_weight);
                    break;
                }
            }
        }

        size_t total_valid_samples = rollout_dataset.size();
        for (auto& st : action_stats_map) {
            if (total_valid_samples > 0) {
                st.selection_pct = (static_cast<float>(st.selection_count) / static_cast<float>(total_valid_samples)) * 100.0f;
            }
            if (st.selection_count > 0) {
                st.avg_regret /= static_cast<double>(st.selection_count);
            }
            st.avg_q_value = 0.0; // No measured return estimate is available.
        }

        std::sort(action_stats_map.begin(), action_stats_map.end(), [](const ActionStat& a, const ActionStat& b) {
            return a.selection_count > b.selection_count;
        });
        res.action_stats = action_stats_map;

        // 4. Final Benchmark Extracted APL Expected Value
        report_progress(0.88f, "Phase 4/4: Benchmarking Extracted VIPER APL with High Confidence...");
        WarlockSimulator eval_sim = sim;
        eval_sim.policy.custom_rules = res.extracted_rules;
        eval_sim.policy.use_custom_apl = true;
        eval_sim.record_viper_samples = false;

        StatSummary viper_stats = compute_expected_dps(eval_sim, benchmark_iterations, seed);
        res.viper_expected_dps = viper_stats.mean_dps;
        res.viper_dps_stddev = viper_stats.stddev_dps;
        res.viper_min_dps = viper_stats.min_dps;
        res.viper_max_dps = viper_stats.max_dps;

        res.viper_gain_over_baseline = res.viper_expected_dps - res.baseline_expected_dps;
        res.viper_gain_pct = (res.viper_gain_over_baseline / std::max(1.0, res.baseline_expected_dps)) * 100.0;

        if (res.oracle_expected_gain > 0.0) {
            res.oracle_potential_captured_pct = (res.viper_gain_over_baseline / res.oracle_expected_gain) * 100.0;
        } else {
            res.oracle_potential_captured_pct = 0.0;
        }

        // Oracle Fidelity / Action Agreement %
        res.oracle_agreement_fidelity_pct = 0.0; // Not measured for the selected APL.

        // 5. Fit & Benchmark High-Performance GBDT Q-Policy Ensemble
        report_progress(0.94f, "Phase 4/4: Fitting & Benchmarking GBDT Multi-Action Q-Policy Ensemble...");
        std::vector<sim::GBDTMultiActionQPolicy::QSample> q_samples;
        q_samples.reserve(rollout_dataset.samples.size() * 4);
        for (const auto& sample : rollout_dataset.samples) {
            for (const auto& [act, _] : candidate_actions) {
                if (is_action_legal(act, sample.state, sim.talents)) {
                    double local_q = estimate_local_q_value(sample.state, act, sim.talents);
                    if (local_q > -900.0) {
                        float target_q = static_cast<float>(local_q);
                        if (static_cast<uint8_t>(act) == sample.oracle_action) {
                            target_q += static_cast<float>(sample.sample_weight * 5.0f);
                        }
                        q_samples.push_back({sample.state, static_cast<uint8_t>(act), target_q, 1.0f});
                    }
                }
            }
        }
        sim::GBDTConfig gbdt_cfg;
        gbdt_cfg.num_trees = 35;
        gbdt_cfg.max_depth = 4;
        gbdt_cfg.learning_rate = 0.12f;
        res.gbdt_q_policy = sim::GBDTMultiActionQPolicy(gbdt_cfg);
        res.gbdt_q_policy.fit(q_samples);

        // Benchmark GBDT Q-Policy against baseline and oracle
        WarlockSimulator gbdt_sim = sim;
        gbdt_sim.use_gbdt_policy = true;
        gbdt_sim.gbdt_q_policy = std::make_shared<sim::GBDTMultiActionQPolicy>(res.gbdt_q_policy);
        gbdt_sim.record_viper_samples = false;
        StatSummary gbdt_stats = compute_expected_dps(gbdt_sim, benchmark_iterations, seed);
        res.gbdt_policy_expected_dps = gbdt_stats.mean_dps;
        res.gbdt_policy_dps_stddev = gbdt_stats.stddev_dps;
        res.gbdt_policy_gain_pct = ((res.gbdt_policy_expected_dps - res.baseline_expected_dps) / std::max(1.0, res.baseline_expected_dps)) * 100.0;
        if (res.oracle_expected_gain > 0.0) {
            res.gbdt_potential_captured_pct = ((res.gbdt_policy_expected_dps - res.baseline_expected_dps) / res.oracle_expected_gain) * 100.0;
        } else {
            res.gbdt_potential_captured_pct = 0.0;
        }

        // Feature Importance Aggregation across action models
        std::vector<double> agg_feat_importances(sim::SimObservation::FEATURE_COUNT, 0.0);
        for (const auto& [act_id, model] : res.gbdt_q_policy.models()) {
            const auto& fi = model.get_feature_importances();
            for (size_t f = 0; f < fi.size() && f < agg_feat_importances.size(); ++f) {
                agg_feat_importances[f] += fi[f];
            }
        }
        auto feat_names = sim::SimObservation::feature_names();
        res.gbdt_feature_importances.clear();
        for (size_t f = 0; f < agg_feat_importances.size() && f < feat_names.size(); ++f) {
            if (agg_feat_importances[f] > 0.0) {
                res.gbdt_feature_importances.push_back({feat_names[f], agg_feat_importances[f]});
            }
        }
        std::sort(res.gbdt_feature_importances.begin(), res.gbdt_feature_importances.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

        // Compute Rule Shifts (Diff between baseline and extracted rules)
        for (size_t i = 0; i < res.extracted_rules.size(); ++i) {
            const auto& vr = res.extracted_rules[i];
            RuleShift shift;
            shift.action = vr.action;
            shift.name = get_action_name(vr.action);
            shift.viper_rank = static_cast<int>(i + 1);

            int base_rank = -1;
            for (size_t j = 0; j < res.baseline_rules.size(); ++j) {
                if (res.baseline_rules[j].action == vr.action) {
                    base_rank = static_cast<int>(j + 1);
                    break;
                }
            }
            shift.baseline_rank = base_rank;

            if (base_rank == -1) {
                shift.change_type = "NEW";
            } else if (shift.viper_rank < base_rank) {
                shift.change_type = "PROMOTED";
            } else if (shift.viper_rank > base_rank) {
                shift.change_type = "DEMOTED";
            } else {
                shift.change_type = "UNCHANGED";
            }

            res.rule_shifts.push_back(shift);
        }

        report_progress(1.0f, "Independent-rule APL Search Complete!");
        return res;
    }

    static VIPERExtractionResult extract_viper_apl(
        WarlockSimulator& sim,
        size_t num_episodes = 50,
        size_t benchmark_iterations = 400,
        std::function<void(float progress, const std::string& status)> progress_cb = nullptr,
        uint64_t seed = 42)
    {
        return extract_viper_apl(sim, num_episodes, benchmark_iterations, 3, progress_cb, seed);
    }
};

} // namespace warlock
