#pragma once
#include "src/sim/warlock/warlock_sim.hpp"
#include "src/sim/warlock/policy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <mutex>
#include <cmath>

namespace warlock {

struct APLRuleShift {
    PriorityAction action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string name;
    int baseline_rank = -1; // 1-indexed, -1 if not in baseline
    int optimized_rank = -1; // 1-indexed, -1 if not in optimized
    std::string change_type; // "PROMOTED", "DEMOTED", "NEW", "UNCHANGED", "PRUNED"
};

struct APLOptimizerConfig {
    size_t population_size = 32;
    size_t generations = 25;
    size_t eval_simulations = 80;
    size_t benchmark_simulations = 500;
    float mutation_rate = 0.40f;
    float crossover_rate = 0.70f;
    bool allow_dual_life_tap = true;
    bool seed_from_current_apl = true;
    bool use_simulated_annealing = false;
    float sa_initial_temp = 40.0f;
    float sa_min_temp = 0.2f;
    std::vector<PriorityRule> custom_seed_rules;
    uint64_t seed = 42;
};

struct APLOptimizationResult {
    double baseline_dps = 0.0;
    double baseline_stddev = 0.0;
    double baseline_min_dps = 0.0;
    double baseline_max_dps = 0.0;

    double optimized_dps = 0.0;
    double optimized_stddev = 0.0;
    double optimized_min_dps = 0.0;
    double optimized_max_dps = 0.0;

    double dps_gain = 0.0;
    double dps_gain_pct = 0.0;

    std::vector<PriorityRule> baseline_rules;
    std::vector<PriorityRule> optimized_rules;
    std::vector<APLRuleShift> rule_shifts;

    struct CandidatePolicy {
        int rank = 1;
        std::string name;
        double dps = 0.0;
        double stddev = 0.0;
        double gain_pct = 0.0;
        std::vector<PriorityRule> rules;
        float emergency_tap_mana = 0.15f;
        float maint_tap_mana = 0.35f;
        float curse_of_doom_cutoff = 60.0f;
        float dot_pandemic_window = 1.0f;
        float execute_hp_threshold = 0.35f;
    };
    std::vector<CandidatePolicy> top_candidates;

    // Extracted continuous parameter levers
    float emergency_tap_mana = 0.15f;
    float maint_tap_mana = 0.35f;
    float curse_of_doom_cutoff = 60.0f;
    float dot_pandemic_window = 1.0f;
    float execute_hp_threshold = 0.35f;

    struct GenerationStat {
        size_t generation = 0;
        double best_dps = 0.0;
        double avg_dps = 0.0;
    };
    std::vector<GenerationStat> evolution_history;
};

class APLOptimizer {
public:
    // Returns spell ID associated with an action
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
            case PriorityAction::DEMONIC_BRAND_SEARING_PAIN: return SpellID::SEARING_PAIN;
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
            case PriorityAction::SHADOW_BOLT_FILLER: return SpellID::SHADOW_BOLT;
            default: return SpellID::SHADOW_BOLT;
        }
    }

    static const char* get_action_name(PriorityAction action) {
        switch (action) {
            case PriorityAction::LIFE_TAP: return "Life Tap (Mana Management)";
            case PriorityAction::RACIAL_EUREKA: return "Racial: Eureka (Gnome)";
            case PriorityAction::RACIAL_BLOOD_FURY: return "Racial: Blood Fury (Orc)";
            case PriorityAction::RACIAL_BERSERKING: return "Racial: Berserking (Troll)";
            case PriorityAction::AMPLIFY_CURSE: return "Amplify Curse";
            case PriorityAction::BANE_OF_HAVOC: return "Bane of Havoc (Secondary)";
            case PriorityAction::NIGHTFALL_SHADOW_BOLT: return "Shadow Trance / Nightfall Instant SB";
            case PriorityAction::DECIMATION_SOUL_FIRE: return "Decimation Soul Fire (Execute)";
            case PriorityAction::DECIMATION_SEARING_PAIN: return "Decimation Searing Pain (Proc Trigger)";
            case PriorityAction::DEMONIC_BRAND_SEARING_PAIN: return "Demonic Brand Searing Pain";
            case PriorityAction::CORRUPTION: return "Corruption DoT Upkeep";
            case PriorityAction::SIPHON_LIFE: return "Siphon Life DoT Upkeep";
            case PriorityAction::CURSE_OF_AGONY: return "Bane of Agony Upkeep";
            case PriorityAction::CURSE_OF_DOOM: return "Bane of Doom (>=60s remaining)";
            case PriorityAction::IMMOLATE: return "Immolate DoT Upkeep";
            case PriorityAction::CONFLAGRATE: return "Conflagrate (Consume Immolate)";
            case PriorityAction::SHADOWBURN: return "Shadowburn (On Cooldown)";
            case PriorityAction::SHADOWBURN_ISB: return "Shadowburn (ISB Active)";
            case PriorityAction::DRAIN_HOPE: return "Drain Hope Channel";
            case PriorityAction::INCINERATE_FILLER: return "Incinerate Filler";
            case PriorityAction::SEARING_PAIN_FILLER: return "Searing Pain Filler";
            case PriorityAction::DRAIN_LIFE_FILLER: return "Drain Life Filler";
            case PriorityAction::DRAIN_SOUL_FILLER: return "Drain Soul Filler";
            case PriorityAction::SHADOW_BOLT_FILLER: return "Shadow Bolt Filler";
            default: return "Unknown Action";
        }
    }

    // Checks whether an action can ever be cast given a player's talent spec, race, and policy
    static bool is_action_available_for_talents(PriorityAction action, const Talents& talents, Race race = Race::UNDEAD, bool maintain_immolate = true) {
        switch (action) {
            case PriorityAction::NIGHTFALL_SHADOW_BOLT:
                return talents.aff.nightfall > 0;
            case PriorityAction::IMMOLATE:
                return maintain_immolate && (talents.destro.conflagrate > 0 || talents.destro.incinerate > 0 ||
                                            talents.destro.fire_and_brimstone > 0 || talents.destro.aftermath > 0);
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
            case PriorityAction::DRAIN_LIFE_FILLER:
            case PriorityAction::DRAIN_SOUL_FILLER:
            case PriorityAction::SHADOW_BOLT_FILLER:
                return true;
            case PriorityAction::RACIAL_EUREKA:
                return race == Race::GNOME;
            case PriorityAction::RACIAL_BLOOD_FURY:
                return race == Race::ORC;
            case PriorityAction::RACIAL_BERSERKING:
                return race == Race::TROLL;
            case PriorityAction::BANE_OF_HAVOC:
                return false; // Multi-target handled separately
            default:
                return true;
        }
    }

    // Returns all legal fallback filler actions for a given talent spec
    static std::vector<PriorityAction> get_all_legal_fillers(const Talents& talents) {
        std::vector<PriorityAction> fillers = {
            PriorityAction::SHADOW_BOLT_FILLER,
            PriorityAction::SEARING_PAIN_FILLER,
            PriorityAction::DRAIN_LIFE_FILLER,
            PriorityAction::DRAIN_SOUL_FILLER
        };
        if (talents.destro.incinerate > 0) {
            fillers.push_back(PriorityAction::INCINERATE_FILLER);
        }
        return fillers;
    }

    // Returns all legal offensive actions for a given talent spec and policy
    static std::vector<PriorityAction> get_all_legal_offensive_actions(const Talents& talents, Race race = Race::UNDEAD, bool maintain_immolate = true) {
        std::vector<PriorityAction> all_actions = {
            PriorityAction::NIGHTFALL_SHADOW_BOLT,
            PriorityAction::DECIMATION_SEARING_PAIN,
            PriorityAction::DECIMATION_SOUL_FIRE,
            PriorityAction::IMMOLATE,
            PriorityAction::CONFLAGRATE,
            PriorityAction::CURSE_OF_DOOM,
            PriorityAction::CURSE_OF_AGONY,
            PriorityAction::AMPLIFY_CURSE,
            PriorityAction::CORRUPTION,
            PriorityAction::SIPHON_LIFE,
            PriorityAction::DRAIN_HOPE,
            PriorityAction::SHADOWBURN,
            PriorityAction::SHADOWBURN_ISB,
            PriorityAction::DEMONIC_BRAND_SEARING_PAIN
        };
        std::vector<PriorityAction> legal;
        for (auto act : all_actions) {
            if (is_action_available_for_talents(act, talents, race, maintain_immolate)) {
                legal.push_back(act);
            }
        }
        return legal;
    }

    // Creates a valid default PriorityRule for an action
    static PriorityRule create_default_rule(PriorityAction action, const Talents& talents, Race race = Race::UNDEAD) {
        PriorityRule r;
        r.action = action;
        r.spell_id = get_spell_id(action);
        r.name = get_action_name(action);
        r.enabled = true;
        r.use_custom_thresholds = false;
        r.trigger_condition = "(Always)";
        r.condition_summary = "Default";
        r.rule_explanation = "Genetic APL Rule";

        if (action == PriorityAction::LIFE_TAP) {
            r.use_custom_thresholds = true;
            r.max_mana_pct = 0.35f;
            r.min_hp_pct = 0.15f;
            r.condition_summary = "Player Mana <= 35%";
        } else if (action == PriorityAction::CURSE_OF_DOOM) {
            r.use_custom_thresholds = true;
            r.min_time_remaining = 60.0f;
            r.condition_summary = "Time Remaining >= 60s";
        } else if (action == PriorityAction::SHADOWBURN) {
            r.condition_summary = "CD Ready (8s)";
            r.trigger_condition = "Trigger when: Shadowburn cooldown is ready (8s).";
            r.rule_explanation = "Instant Shadow burst damage on 8s cooldown.";
        } else if (action == PriorityAction::SHADOWBURN_ISB) {
            r.use_custom_thresholds = true;
            r.require_isb_active = true;
            r.condition_summary = "CD Ready & ISB Active (+20% Dmg)";
            r.trigger_condition = "Trigger when: Shadowburn cooldown is ready (8s) AND target has active Improved Shadow Bolt.";
            r.rule_explanation = "Instant Shadow burst damage only when empowered by Improved Shadow Bolt (+20% Shadow).";
        } else if (action == PriorityAction::RACIAL_EUREKA) {
            r.name = "Eureka! (Gnome)";
            r.condition_summary = "Align 0-6s Before Doom Tick / Execute";
            r.trigger_condition = "Trigger when: 0-6s before Bane of Doom damage tick, or during Execute (<35% HP), or on cooldown.";
            r.rule_explanation = "Instant off-GCD racial. Aligns with Bane of Doom detonation ticks (+10% damage, -50% mana) and execute phase.";
        } else if (action == PriorityAction::RACIAL_BLOOD_FURY) {
            r.name = "Blood Fury (Orc)";
            r.condition_summary = "Align 0-14s Before Doom Tick / Execute";
            r.trigger_condition = "Trigger when: 0-14s before Bane of Doom damage tick, or during Execute (<35% HP), or on cooldown.";
            r.rule_explanation = "Instant off-GCD racial (+10% SP for 15s). Aligns with Bane of Doom detonation ticks and execute phase.";
        } else if (action == PriorityAction::RACIAL_BERSERKING) {
            r.name = "Berserking (Troll)";
            r.condition_summary = "Smart Execute & CD Ready (180s)";
            r.trigger_condition = "Trigger when: Target <35% HP (Execute) or on cooldown.";
            r.rule_explanation = "Instant off-GCD racial (+10-30% haste for 10s) popped during execute burst.";
        }
        return r;
    }

    enum class TapStrategy : uint8_t {
        SINGLE_TOP = 0,
        DUAL_TAP = 1,
        SINGLE_BOTTOM = 2
    };

    // Chromosome representation for Genetic Search
    struct Individual {
        std::vector<PriorityRule> rules;
        TapStrategy tap_strategy = TapStrategy::SINGLE_TOP;
        PriorityAction fallback_filler_action = PriorityAction::SHADOW_BOLT_FILLER;
        double fitness_dps = 0.0;

        // Continuous Gene Parameters (strictly domain bounded)
        float emergency_tap_mana = 0.15f; // [0.10 .. 0.25]
        float maint_tap_mana = 0.35f;     // [0.25 .. 0.50]
        float cod_time_cutoff = 60.0f;    // [60.0 .. 75.0] (strictly >= 60s for full damage tick)
        float dot_refresh_window = 0.0f;  // [0.0 .. 2.5] (0.0 = only refresh on expiration, no clipping)
        float exec_hp_threshold = 0.35f;  // [0.35 .. 0.35] (Decimation active at <= 35% HP)
    };

    // Enforces structural constraints: Action Uniqueness, Talent Legality, Tap Strategy, Sane Parameter Clamps, and Final Filler
    static void enforce_constraints(Individual& ind, const Talents& talents, Race race, bool allow_dual_tap = true, bool maintain_immolate = true) {
        ind.emergency_tap_mana = std::clamp(ind.emergency_tap_mana, 0.10f, 0.25f);
        ind.maint_tap_mana = std::clamp(ind.maint_tap_mana, 0.25f, 0.50f);
        ind.cod_time_cutoff = std::clamp(ind.cod_time_cutoff, 60.0f, 75.0f);
        ind.dot_refresh_window = std::clamp(ind.dot_refresh_window, 0.0f, 2.5f);
        ind.exec_hp_threshold = 0.35f;

        if (!allow_dual_tap && ind.tap_strategy == TapStrategy::DUAL_TAP) {
            ind.tap_strategy = TapStrategy::SINGLE_TOP;
        }

        // Validate fallback filler legality
        if (ind.fallback_filler_action == PriorityAction::INCINERATE_FILLER && talents.destro.incinerate == 0) {
            ind.fallback_filler_action = PriorityAction::SHADOW_BOLT_FILLER;
        }

        std::vector<PriorityRule> offensive_spells;
        std::vector<PriorityAction> seen_actions;

        for (auto& r : ind.rules) {
            if (!r.enabled) continue;
            if (!is_action_available_for_talents(r.action, talents, race, maintain_immolate)) continue;
            if (r.action == PriorityAction::LIFE_TAP) continue;
            if (r.action == PriorityAction::RACIAL_EUREKA ||
                r.action == PriorityAction::RACIAL_BLOOD_FURY ||
                r.action == PriorityAction::RACIAL_BERSERKING) {
                continue; // Racials handled separately at top
            }
            if (r.action == PriorityAction::SHADOW_BOLT_FILLER ||
                r.action == PriorityAction::INCINERATE_FILLER ||
                r.action == PriorityAction::SEARING_PAIN_FILLER ||
                r.action == PriorityAction::DRAIN_LIFE_FILLER ||
                r.action == PriorityAction::DRAIN_SOUL_FILLER) {
                ind.fallback_filler_action = r.action;
                continue;
            }

            // Enforce at most 1 instance of any unique spell
            if (std::find(seen_actions.begin(), seen_actions.end(), r.action) != seen_actions.end()) {
                continue; // Skip duplicate action
            }
            seen_actions.push_back(r.action);

            // Apply continuous parameters
            if (r.action == PriorityAction::CURSE_OF_DOOM) {
                r.use_custom_thresholds = true;
                r.min_time_remaining = ind.cod_time_cutoff;
                r.condition_summary = "Time Remaining >= " + std::to_string(static_cast<int>(ind.cod_time_cutoff)) + "s";
            } else if (r.action == PriorityAction::CORRUPTION || r.action == PriorityAction::IMMOLATE || r.action == PriorityAction::SIPHON_LIFE) {
                if (ind.dot_refresh_window > 0.0f) {
                    r.use_custom_thresholds = true;
                    r.max_dot_rem_sec = ind.dot_refresh_window;
                    r.condition_summary = "Duration Left <= " + std::to_string(ind.dot_refresh_window).substr(0, 3) + "s";
                } else {
                    r.use_custom_thresholds = false;
                    r.max_dot_rem_sec = 0.0f;
                    r.condition_summary = "DoT Expired / Missing";
                }
            } else if (r.action == PriorityAction::DECIMATION_SOUL_FIRE || r.action == PriorityAction::DECIMATION_SEARING_PAIN) {
                r.use_custom_thresholds = true;
                r.max_target_hp_pct = ind.exec_hp_threshold;
                r.condition_summary = "Target HP <= " + std::to_string(static_cast<int>(ind.exec_hp_threshold * 100.0f)) + "%";
            } else if (r.action == PriorityAction::SHADOWBURN) {
                if (r.use_custom_thresholds && r.max_target_hp_pct < 1.0f) {
                    r.max_target_hp_pct = ind.exec_hp_threshold;
                    r.condition_summary = "Target HP <= " + std::to_string(static_cast<int>(ind.exec_hp_threshold * 100.0f)) + "%";
                }
            } else if (r.action == PriorityAction::SHADOWBURN_ISB) {
                r.use_custom_thresholds = true;
                r.require_isb_active = true;
                if (r.max_target_hp_pct < 1.0f) {
                    r.max_target_hp_pct = ind.exec_hp_threshold;
                    r.condition_summary = "ISB Active & Target HP <= " + std::to_string(static_cast<int>(ind.exec_hp_threshold * 100.0f)) + "%";
                } else {
                    r.condition_summary = "CD Ready & ISB Active (+20% Dmg)";
                }
            }

            offensive_spells.push_back(r);
        }

        // Domain Invariant 1: DECIMATION_SEARING_PAIN must precede DECIMATION_SOUL_FIRE
        auto sp_it = std::find_if(offensive_spells.begin(), offensive_spells.end(), [](const PriorityRule& r) {
            return r.action == PriorityAction::DECIMATION_SEARING_PAIN;
        });
        auto sf_it = std::find_if(offensive_spells.begin(), offensive_spells.end(), [](const PriorityRule& r) {
            return r.action == PriorityAction::DECIMATION_SOUL_FIRE;
        });
        if (sp_it != offensive_spells.end() && sf_it != offensive_spells.end()) {
            if (std::distance(offensive_spells.begin(), sp_it) > std::distance(offensive_spells.begin(), sf_it)) {
                std::iter_swap(sp_it, sf_it);
            }
        }

        // Domain Invariant 2: CURSE_OF_DOOM must precede CURSE_OF_AGONY
        auto cod_it = std::find_if(offensive_spells.begin(), offensive_spells.end(), [](const PriorityRule& r) {
            return r.action == PriorityAction::CURSE_OF_DOOM;
        });
        auto coa_it = std::find_if(offensive_spells.begin(), offensive_spells.end(), [](const PriorityRule& r) {
            return r.action == PriorityAction::CURSE_OF_AGONY;
        });
        if (cod_it != offensive_spells.end() && coa_it != offensive_spells.end()) {
            if (std::distance(offensive_spells.begin(), cod_it) > std::distance(offensive_spells.begin(), coa_it)) {
                std::iter_swap(cod_it, coa_it);
            }
        }

        // Domain Invariant 3: AMPLIFY_CURSE must precede CURSE_OF_AGONY
        auto amp_it = std::find_if(offensive_spells.begin(), offensive_spells.end(), [](const PriorityRule& r) {
            return r.action == PriorityAction::AMPLIFY_CURSE;
        });
        if (amp_it != offensive_spells.end() && coa_it != offensive_spells.end()) {
            if (std::distance(offensive_spells.begin(), amp_it) > std::distance(offensive_spells.begin(), coa_it)) {
                std::iter_swap(amp_it, coa_it);
            }
        }

        // Domain Invariant 4: IMMOLATE must precede CONFLAGRATE
        auto immo_it = std::find_if(offensive_spells.begin(), offensive_spells.end(), [](const PriorityRule& r) {
            return r.action == PriorityAction::IMMOLATE;
        });
        auto confl_it = std::find_if(offensive_spells.begin(), offensive_spells.end(), [](const PriorityRule& r) {
            return r.action == PriorityAction::CONFLAGRATE;
        });
        if (immo_it != offensive_spells.end() && confl_it != offensive_spells.end()) {
            if (std::distance(offensive_spells.begin(), immo_it) > std::distance(offensive_spells.begin(), confl_it)) {
                std::iter_swap(immo_it, confl_it);
            }
        }

        std::vector<PriorityRule> constrained;

        // Life Tap rule definitions
        PriorityRule top_tap;
        top_tap.action = PriorityAction::LIFE_TAP;
        top_tap.spell_id = SpellID::LIFE_TAP;
        top_tap.name = (ind.tap_strategy == TapStrategy::DUAL_TAP) ? "Emergency Life Tap" : "Life Tap";
        top_tap.enabled = true;
        top_tap.use_custom_thresholds = true;
        top_tap.max_mana_pct = (ind.tap_strategy == TapStrategy::DUAL_TAP) ? ind.emergency_tap_mana : ind.maint_tap_mana;
        top_tap.min_hp_pct = 0.15f;
        top_tap.condition_summary = "Player Mana <= " + std::to_string(static_cast<int>(top_tap.max_mana_pct * 100.0f)) + "%";
        top_tap.rule_explanation = (ind.tap_strategy == TapStrategy::DUAL_TAP)
            ? "High-priority emergency tap to prevent OOM stalling"
            : "High-priority life tap to maintain mana pool";

        PriorityRule bottom_tap;
        bottom_tap.action = PriorityAction::LIFE_TAP;
        bottom_tap.spell_id = SpellID::LIFE_TAP;
        bottom_tap.name = (ind.tap_strategy == TapStrategy::DUAL_TAP) ? "Maintenance Life Tap" : "Life Tap";
        bottom_tap.enabled = true;
        bottom_tap.use_custom_thresholds = true;
        bottom_tap.max_mana_pct = ind.maint_tap_mana;
        bottom_tap.min_hp_pct = 0.15f;
        bottom_tap.condition_summary = "Player Mana <= " + std::to_string(static_cast<int>(ind.maint_tap_mana * 100.0f)) + "%";
        bottom_tap.rule_explanation = "Maintenance tap when resources are below threshold";

        // 1. Top Life Tap (if Single Top or Dual Tap)
        if (ind.tap_strategy == TapStrategy::SINGLE_TOP || ind.tap_strategy == TapStrategy::DUAL_TAP) {
            constrained.push_back(top_tap);
        }

        // 2. Off-GCD Racial Ability (Orc / Troll / Gnome)
        if (race == Race::ORC) {
            constrained.push_back(create_default_rule(PriorityAction::RACIAL_BLOOD_FURY, talents, race));
        } else if (race == Race::TROLL) {
            constrained.push_back(create_default_rule(PriorityAction::RACIAL_BERSERKING, talents, race));
        } else if (race == Race::GNOME) {
            constrained.push_back(create_default_rule(PriorityAction::RACIAL_EUREKA, talents, race));
        }

        // 3. Rotational Offensive Spells
        for (const auto& r : offensive_spells) {
            constrained.push_back(r);
        }

        // 4. Bottom Life Tap (if Dual Tap or Single Bottom)
        if (ind.tap_strategy == TapStrategy::DUAL_TAP || ind.tap_strategy == TapStrategy::SINGLE_BOTTOM) {
            constrained.push_back(bottom_tap);
        }

        // 5. Fallback Filler (Bottom)
        PriorityRule fallback_filler;
        fallback_filler.action = ind.fallback_filler_action;
        fallback_filler.spell_id = get_spell_id(fallback_filler.action);
        fallback_filler.name = get_action_name(fallback_filler.action);
        fallback_filler.enabled = true;
        fallback_filler.use_custom_thresholds = false;
        fallback_filler.condition_summary = "Fallback Filler";
        fallback_filler.trigger_condition = "Always";
        fallback_filler.rule_explanation = "Mandatory filler fallback";
        constrained.push_back(fallback_filler);

        ind.rules = constrained;
    }

    // Statistical expected DPS calculation across multi-threaded simulator instances
    struct StatSummary {
        double mean_dps = 0.0;
        double stddev_dps = 0.0;
        double min_dps = 0.0;
        double max_dps = 0.0;
    };

    static StatSummary compute_expected_dps(WarlockSimulator sim_copy, size_t iterations = 300, uint64_t seed = 42) {
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
                sim::FastRNG rng(seed + t * 65537);
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

    // Main Genetic APL Policy Optimization Pipeline
    static APLOptimizationResult optimize_apl(
        WarlockSimulator& sim,
        const APLOptimizerConfig& config,
        std::function<void(float progress, const std::string& status)> progress_cb = nullptr)
    {
        APLOptimizationResult res;
        auto report_progress = [&](float p, const std::string& msg) {
            if (progress_cb) progress_cb(p, msg);
        };

        // Phase 1: Measure Baseline Expected Value
        report_progress(0.05f, "Phase 1/3: Measuring Baseline APL Expected Value...");
        if (!config.custom_seed_rules.empty()) {
            res.baseline_rules = config.custom_seed_rules;
        } else if (config.seed_from_current_apl) {
            res.baseline_rules = sim.policy.get_priority_rules(sim.talents, sim.race);
        } else {
            res.baseline_rules = sim.policy.build_preset_rules(sim.talents, sim.race);
        }

        // Measure actual baseline DPS for the seed policy
        WarlockSimulator base_sim = sim;
        base_sim.policy.custom_rules = res.baseline_rules;
        base_sim.policy.use_custom_apl = true;
        StatSummary base_stats = compute_expected_dps(base_sim, config.benchmark_simulations, config.seed);
        res.baseline_dps = base_stats.mean_dps;
        res.baseline_stddev = base_stats.stddev_dps;
        res.baseline_min_dps = base_stats.min_dps;
        res.baseline_max_dps = base_stats.max_dps;

        // Phase 2: Evolutionary Population Search
        auto quick_eval = [&](const std::vector<PriorityRule>& candidate_rules, size_t eval_iters) -> double {
            WarlockSimulator test_sim = sim;
            test_sim.policy.custom_rules = candidate_rules;
            test_sim.policy.use_custom_apl = true;
            test_sim.record_viper_samples = false;
            StatSummary s = compute_expected_dps(test_sim, eval_iters, config.seed + 101);
            return s.mean_dps;
        };

        sim::FastRNG rng(config.seed + 1337);
        std::vector<Individual> population;

        // Seed 1: Active Baseline APL (exact replication)
        Individual ind_base;
        ind_base.rules = res.baseline_rules;
        size_t tap_count = 0;
        size_t first_tap_idx = 0;
        float base_tap = static_cast<float>(sim.policy.life_tap_threshold_pct) / 100.0f;
        ind_base.maint_tap_mana = std::clamp(base_tap, 0.25f, 0.50f);
        ind_base.emergency_tap_mana = std::clamp(ind_base.maint_tap_mana * 0.5f, 0.10f, 0.20f);

        for (size_t i = 0; i < res.baseline_rules.size(); ++i) {
            const auto& r = res.baseline_rules[i];
            if (r.action == PriorityAction::LIFE_TAP) {
                if (tap_count == 0) first_tap_idx = i;
                tap_count++;
                if (r.use_custom_thresholds) {
                    if (r.max_mana_pct <= 0.25f && tap_count > 1) {
                        ind_base.emergency_tap_mana = r.max_mana_pct;
                    } else {
                        ind_base.maint_tap_mana = r.max_mana_pct;
                    }
                }
            } else if (r.action == PriorityAction::CURSE_OF_DOOM && r.min_time_remaining > 0.0f) {
                ind_base.cod_time_cutoff = r.min_time_remaining;
            } else if ((r.action == PriorityAction::CORRUPTION || r.action == PriorityAction::IMMOLATE || r.action == PriorityAction::CURSE_OF_AGONY) && r.max_dot_rem_sec > 0.0f) {
                ind_base.dot_refresh_window = r.max_dot_rem_sec;
            } else if ((r.action == PriorityAction::DECIMATION_SOUL_FIRE || r.action == PriorityAction::DECIMATION_SEARING_PAIN) && r.max_target_hp_pct < 1.0f) {
                ind_base.exec_hp_threshold = r.max_target_hp_pct;
            }
        }

        if (tap_count >= 2) {
            ind_base.tap_strategy = TapStrategy::DUAL_TAP;
        } else if (tap_count == 1 && first_tap_idx > res.baseline_rules.size() / 2) {
            ind_base.tap_strategy = TapStrategy::SINGLE_BOTTOM;
        } else {
            ind_base.tap_strategy = TapStrategy::SINGLE_TOP;
        }

        enforce_constraints(ind_base, sim.talents, sim.race, config.allow_dual_life_tap);
        population.push_back(ind_base);

        // Seed 2: Dual-Tap Variant of Baseline (if dual tap enabled)
        if (config.allow_dual_life_tap) {
            Individual ind_dual = ind_base;
            ind_dual.tap_strategy = TapStrategy::DUAL_TAP;
            ind_dual.emergency_tap_mana = std::clamp(ind_base.maint_tap_mana * 0.45f, 0.12f, 0.20f);
            enforce_constraints(ind_dual, sim.talents, sim.race, true);
            population.push_back(ind_dual);
        }

        // Seed 3: Natural Priority Tier Order for Warlock
        Individual ind_natural;
        ind_natural.tap_strategy = ind_base.tap_strategy;
        ind_natural.fallback_filler_action = ind_base.fallback_filler_action;
        std::vector<PriorityAction> natural_actions = {
            PriorityAction::NIGHTFALL_SHADOW_BOLT,
            PriorityAction::DECIMATION_SEARING_PAIN,
            PriorityAction::DECIMATION_SOUL_FIRE,
            PriorityAction::IMMOLATE,
            PriorityAction::CONFLAGRATE,
            PriorityAction::CURSE_OF_DOOM,
            PriorityAction::CURSE_OF_AGONY,
            PriorityAction::AMPLIFY_CURSE,
            PriorityAction::CORRUPTION,
            PriorityAction::SIPHON_LIFE,
            PriorityAction::SHADOWBURN,
            ind_base.fallback_filler_action
        };
        if (config.seed_from_current_apl) {
            // Re-order active baseline rules according to natural hierarchy without adding foreign spells
            for (auto act : natural_actions) {
                for (const auto& r : res.baseline_rules) {
                    if (r.action == act && r.action != PriorityAction::LIFE_TAP) {
                        ind_natural.rules.push_back(r);
                        break;
                    }
                }
            }
        } else {
            for (auto act : natural_actions) {
                if (is_action_available_for_talents(act, sim.talents, sim.race, sim.policy.maintain_immolate)) {
                    ind_natural.rules.push_back(create_default_rule(act, sim.talents, sim.race));
                }
            }
        }
        enforce_constraints(ind_natural, sim.talents, sim.race, config.allow_dual_life_tap, sim.policy.maintain_immolate);
        population.push_back(ind_natural);

        // Seed 4: Aggressive Execute / Burst Variant
        Individual ind_aggro = ind_base;
        ind_aggro.emergency_tap_mana = 0.12f;
        ind_aggro.maint_tap_mana = 0.28f;
        ind_aggro.exec_hp_threshold = 0.35f;
        ind_aggro.dot_refresh_window = 0.0f;
        enforce_constraints(ind_aggro, sim.talents, sim.race, config.allow_dual_life_tap, sim.policy.maintain_immolate);
        population.push_back(ind_aggro);

        // Seed 5: Conservative Mana Variant
        Individual ind_cons = ind_base;
        ind_cons.emergency_tap_mana = 0.20f;
        ind_cons.maint_tap_mana = 0.45f;
        ind_cons.exec_hp_threshold = 0.35f;
        ind_cons.dot_refresh_window = 0.0f;
        enforce_constraints(ind_cons, sim.talents, sim.race, config.allow_dual_life_tap, sim.policy.maintain_immolate);
        population.push_back(ind_cons);

        // Fill remaining population with mutated variants exploring outwards from active seed
        while (population.size() < config.population_size) {
            Individual ind = (rng.next_double() < 0.70) ? ind_base : ind_natural;
            ind.emergency_tap_mana = std::clamp(ind.emergency_tap_mana + static_cast<float>(rng.next_double() - 0.5) * 0.08f, 0.10f, 0.25f);
            ind.maint_tap_mana = std::clamp(ind.maint_tap_mana + static_cast<float>(rng.next_double() - 0.5) * 0.10f, 0.25f, 0.50f);
            ind.cod_time_cutoff = std::clamp(ind.cod_time_cutoff + static_cast<float>(rng.next_double() - 0.5) * 10.0f, 60.0f, 75.0f);
            ind.dot_refresh_window = std::clamp(ind.dot_refresh_window + static_cast<float>(rng.next_double() - 0.5) * 0.8f, 0.0f, 2.5f);
            ind.exec_hp_threshold = 0.35f;

            if (config.allow_dual_life_tap && rng.next_double() < 0.35) {
                ind.tap_strategy = (rng.next_double() < 0.5) ? TapStrategy::DUAL_TAP : TapStrategy::SINGLE_TOP;
            }

            // Mutation 1a: Swap offensive spells safely without touching Life Tap or Filler
            std::vector<size_t> off_idx;
            for (size_t k = 0; k < ind.rules.size(); ++k) {
                auto act = ind.rules[k].action;
                if (act != PriorityAction::LIFE_TAP && act != PriorityAction::SHADOW_BOLT_FILLER &&
                    act != PriorityAction::INCINERATE_FILLER && act != PriorityAction::SEARING_PAIN_FILLER &&
                    act != PriorityAction::DRAIN_LIFE_FILLER && act != PriorityAction::DRAIN_SOUL_FILLER) {
                    off_idx.push_back(k);
                }
            }
            if (off_idx.size() >= 2 && rng.next_double() < 0.70) {
                size_t i = off_idx[rng.next_u64() % off_idx.size()];
                size_t j = off_idx[rng.next_u64() % off_idx.size()];
                std::swap(ind.rules[i], ind.rules[j]);
            }

            // Mutation 1b: Rule Insertion (Variable Length Growth: test adding an unused legal spell)
            if (rng.next_double() < 0.35) {
                auto legal_actions = get_all_legal_offensive_actions(sim.talents, sim.race, sim.policy.maintain_immolate);
                std::vector<PriorityAction> missing_actions;
                for (auto act : legal_actions) {
                    bool present = false;
                    for (const auto& r : ind.rules) {
                        if (r.action == act) { present = true; break; }
                    }
                    if (!present) missing_actions.push_back(act);
                }
                if (!missing_actions.empty()) {
                    PriorityAction to_add = missing_actions[rng.next_u64() % missing_actions.size()];
                    PriorityRule new_rule = create_default_rule(to_add, sim.talents, sim.race);
                    if (ind.rules.size() > 1) {
                        size_t insert_pos = 1 + (rng.next_u64() % (ind.rules.size() - 1));
                        ind.rules.insert(ind.rules.begin() + insert_pos, new_rule);
                    } else {
                        ind.rules.push_back(new_rule);
                    }
                }
            }

            // Mutation 1c: Rule Pruning (Variable Length Shrink: test dropping an optional spell)
            if (rng.next_double() < 0.35) {
                std::vector<size_t> prunable_idx;
                for (size_t k = 0; k < ind.rules.size(); ++k) {
                    auto act = ind.rules[k].action;
                    if (act != PriorityAction::LIFE_TAP && act != PriorityAction::SHADOW_BOLT_FILLER &&
                        act != PriorityAction::INCINERATE_FILLER && act != PriorityAction::SEARING_PAIN_FILLER &&
                        act != PriorityAction::DRAIN_LIFE_FILLER && act != PriorityAction::DRAIN_SOUL_FILLER) {
                        prunable_idx.push_back(k);
                    }
                }
                if (prunable_idx.size() > 1) {
                    size_t to_remove = prunable_idx[rng.next_u64() % prunable_idx.size()];
                    ind.rules.erase(ind.rules.begin() + to_remove);
                }
            }

            // Mutation 1d: Fallback Filler Mutation (Explore alternative legal fillers: Shadow Bolt, Incinerate, Searing Pain, Drain Life)
            if (rng.next_double() < 0.25) {
                auto legal_fillers = get_all_legal_fillers(sim.talents);
                if (!legal_fillers.empty()) {
                    ind.fallback_filler_action = legal_fillers[rng.next_u64() % legal_fillers.size()];
                }
            }

            enforce_constraints(ind, sim.talents, sim.race, config.allow_dual_life_tap, sim.policy.maintain_immolate);
            population.push_back(ind);
        }

        // Evaluate initial generation
        for (auto& ind : population) {
            ind.fitness_dps = quick_eval(ind.rules, config.eval_simulations);
        }

        Individual best_individual = population[0];
        for (const auto& ind : population) {
            if (ind.fitness_dps > best_individual.fitness_dps) {
                best_individual = ind;
            }
        }

        if (config.use_simulated_annealing) {
            // Simulated Annealing (SA) Search
            Individual current = best_individual;
            size_t total_steps = config.generations * config.population_size;
            float t_start = config.sa_initial_temp;
            float t_min = config.sa_min_temp;
            double log_cooling = std::log(std::max(1e-4f, t_min) / std::max(1e-3f, t_start));

            size_t step_count = 0;
            for (size_t epoch = 0; epoch < config.generations; ++epoch) {
                float epoch_prog = 0.15f + 0.70f * (static_cast<float>(epoch) / static_cast<float>(config.generations));
                std::string status_msg = "Phase 2/3: Simulated Annealing APL (Epoch " + std::to_string(epoch + 1) + "/" +
                                         std::to_string(config.generations) + " | Best: " +
                                         std::to_string(static_cast<int>(best_individual.fitness_dps)) + " DPS)...";
                report_progress(epoch_prog, status_msg);

                double epoch_sum_dps = 0.0;

                for (size_t iter = 0; iter < config.population_size; ++iter, ++step_count) {
                    float frac = static_cast<float>(step_count) / static_cast<float>(std::max<size_t>(1, total_steps));
                    float current_temp = t_start * std::exp(static_cast<float>(log_cooling) * frac);

                    // Neighbor perturbation
                    Individual neighbor = current;

                    // Choose perturbation type
                    double r = rng.next_double();
                    if (r < 0.30) {
                        // Perturbation A: Swap offensive rules
                        std::vector<size_t> off_idx;
                        for (size_t k = 0; k < neighbor.rules.size(); ++k) {
                            auto act = neighbor.rules[k].action;
                            if (act != PriorityAction::LIFE_TAP && act != PriorityAction::SHADOW_BOLT_FILLER &&
                                act != PriorityAction::INCINERATE_FILLER && act != PriorityAction::SEARING_PAIN_FILLER &&
                                act != PriorityAction::DRAIN_LIFE_FILLER && act != PriorityAction::DRAIN_SOUL_FILLER) {
                                off_idx.push_back(k);
                            }
                        }
                        if (off_idx.size() >= 2) {
                            size_t i = off_idx[rng.next_u64() % off_idx.size()];
                            size_t j = off_idx[rng.next_u64() % off_idx.size()];
                            std::swap(neighbor.rules[i], neighbor.rules[j]);
                        }
                    } else if (r < 0.48) {
                        // Perturbation B: Rule Insertion
                        auto legal_actions = get_all_legal_offensive_actions(sim.talents, sim.race, sim.policy.maintain_immolate);
                        std::vector<PriorityAction> missing_actions;
                        for (auto act : legal_actions) {
                            bool present = false;
                            for (const auto& rul : neighbor.rules) {
                                if (rul.action == act) { present = true; break; }
                            }
                            if (!present) missing_actions.push_back(act);
                        }
                        if (!missing_actions.empty()) {
                            PriorityAction to_add = missing_actions[rng.next_u64() % missing_actions.size()];
                            PriorityRule new_rule = create_default_rule(to_add, sim.talents, sim.race);
                            if (neighbor.rules.size() > 1) {
                                size_t insert_pos = 1 + (rng.next_u64() % (neighbor.rules.size() - 1));
                                neighbor.rules.insert(neighbor.rules.begin() + insert_pos, new_rule);
                            } else {
                                neighbor.rules.push_back(new_rule);
                            }
                        }
                    } else if (r < 0.62) {
                        // Perturbation C: Rule Pruning
                        std::vector<size_t> prunable_idx;
                        for (size_t k = 0; k < neighbor.rules.size(); ++k) {
                            auto act = neighbor.rules[k].action;
                            if (act != PriorityAction::LIFE_TAP && act != PriorityAction::SHADOW_BOLT_FILLER &&
                                act != PriorityAction::INCINERATE_FILLER && act != PriorityAction::SEARING_PAIN_FILLER &&
                                act != PriorityAction::DRAIN_LIFE_FILLER && act != PriorityAction::DRAIN_SOUL_FILLER) {
                                prunable_idx.push_back(k);
                            }
                        }
                        if (prunable_idx.size() > 1) {
                            size_t to_remove = prunable_idx[rng.next_u64() % prunable_idx.size()];
                            neighbor.rules.erase(neighbor.rules.begin() + to_remove);
                        }
                    } else if (r < 0.72) {
                        // Perturbation D: Fallback Filler Mutation
                        auto legal_fillers = get_all_legal_fillers(sim.talents);
                        if (!legal_fillers.empty()) {
                            neighbor.fallback_filler_action = legal_fillers[rng.next_u64() % legal_fillers.size()];
                        }
                    } else {
                        // Perturbation E: Continuous parameter jitter
                        neighbor.emergency_tap_mana += static_cast<float>((rng.next_double() - 0.5) * 0.05);
                        neighbor.maint_tap_mana += static_cast<float>((rng.next_double() - 0.5) * 0.08);
                        neighbor.cod_time_cutoff += static_cast<float>((rng.next_double() - 0.5) * 6.0);
                        neighbor.dot_refresh_window += static_cast<float>((rng.next_double() - 0.5) * 0.5);
                        if (config.allow_dual_life_tap && rng.next_double() < 0.25) {
                            neighbor.tap_strategy = (neighbor.tap_strategy == TapStrategy::SINGLE_TOP) ? TapStrategy::DUAL_TAP : TapStrategy::SINGLE_TOP;
                        }
                    }

                    enforce_constraints(neighbor, sim.talents, sim.race, config.allow_dual_life_tap, sim.policy.maintain_immolate);
                    neighbor.fitness_dps = quick_eval(neighbor.rules, config.eval_simulations);

                    epoch_sum_dps += neighbor.fitness_dps;
                    population.push_back(neighbor);

                    // Metropolis Acceptance Criterion
                    double delta = neighbor.fitness_dps - current.fitness_dps;
                    if (delta > 0.0 || (current_temp > 0.001f && rng.next_double() < std::exp(delta / current_temp))) {
                        current = neighbor;
                    }

                    if (current.fitness_dps > best_individual.fitness_dps) {
                        best_individual = current;
                    }
                }

                double avg_dps = epoch_sum_dps / static_cast<double>(config.population_size);
                res.evolution_history.push_back({epoch + 1, best_individual.fitness_dps, avg_dps});
            }
        } else {
            // Genetic Algorithm (GA) Evolution loop
            for (size_t gen = 0; gen < config.generations; ++gen) {
                float gen_prog = 0.15f + 0.70f * (static_cast<float>(gen) / static_cast<float>(config.generations));
                std::string status_msg = "Phase 2/3: Evolving APL Policies (Gen " + std::to_string(gen + 1) + "/" +
                                         std::to_string(config.generations) + " | Best: " +
                                         std::to_string(static_cast<int>(best_individual.fitness_dps)) + " DPS)...";
                report_progress(gen_prog, status_msg);

                std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
                    return a.fitness_dps > b.fitness_dps;
                });

                if (population[0].fitness_dps > best_individual.fitness_dps) {
                    best_individual = population[0];
                }

                // Telemetry
                double avg_dps = 0.0;
                for (const auto& ind : population) avg_dps += ind.fitness_dps;
                avg_dps /= static_cast<double>(population.size());
                res.evolution_history.push_back({gen + 1, best_individual.fitness_dps, avg_dps});

                // Tournament selection (k=3)
                auto tournament_select = [&]() -> const Individual& {
                    size_t best_idx = rng.next_u64() % population.size();
                    for (int k = 0; k < 2; ++k) {
                        size_t idx = rng.next_u64() % population.size();
                        if (population[idx].fitness_dps > population[best_idx].fitness_dps) {
                            best_idx = idx;
                        }
                    }
                    return population[best_idx];
                };

                std::vector<Individual> next_pop;
                // Elitism: Top 4 survive unchanged
                for (size_t e = 0; e < 4 && e < population.size(); ++e) {
                    next_pop.push_back(population[e]);
                }

                // Breed next generation
                while (next_pop.size() < config.population_size) {
                    const auto& parent1 = tournament_select();
                    const auto& parent2 = tournament_select();

                    Individual child;
                    // BLX-alpha parameter blending
                    float alpha = static_cast<float>(rng.next_double());
                    child.emergency_tap_mana = alpha * parent1.emergency_tap_mana + (1.0f - alpha) * parent2.emergency_tap_mana;
                    child.maint_tap_mana = alpha * parent1.maint_tap_mana + (1.0f - alpha) * parent2.maint_tap_mana;
                    child.cod_time_cutoff = alpha * parent1.cod_time_cutoff + (1.0f - alpha) * parent2.cod_time_cutoff;
                    child.dot_refresh_window = alpha * parent1.dot_refresh_window + (1.0f - alpha) * parent2.dot_refresh_window;
                    child.exec_hp_threshold = 0.35f;

                    // Rule sequence and TapStrategy crossover
                    child.rules = (rng.next_double() < config.crossover_rate) ? parent1.rules : parent2.rules;
                    child.tap_strategy = (rng.next_double() < config.crossover_rate) ? parent1.tap_strategy : parent2.tap_strategy;
                    child.fallback_filler_action = (rng.next_double() < config.crossover_rate) ? parent1.fallback_filler_action : parent2.fallback_filler_action;

                    // Mutation 1a: Offensive Rule Swap (re-order)
                    if (rng.next_double() < config.mutation_rate) {
                        std::vector<size_t> off_idx;
                        for (size_t k = 0; k < child.rules.size(); ++k) {
                            auto act = child.rules[k].action;
                            if (act != PriorityAction::LIFE_TAP && act != PriorityAction::SHADOW_BOLT_FILLER &&
                                act != PriorityAction::INCINERATE_FILLER && act != PriorityAction::SEARING_PAIN_FILLER &&
                                act != PriorityAction::DRAIN_LIFE_FILLER && act != PriorityAction::DRAIN_SOUL_FILLER) {
                                off_idx.push_back(k);
                            }
                        }
                        if (off_idx.size() >= 2) {
                            size_t i = off_idx[rng.next_u64() % off_idx.size()];
                            size_t j = off_idx[rng.next_u64() % off_idx.size()];
                            std::swap(child.rules[i], child.rules[j]);
                        }
                    }

                    // Mutation 1b: Rule Insertion (Variable Length Growth: test adding an unused legal spell)
                    if (rng.next_double() < config.mutation_rate * 0.40f) {
                        auto legal_actions = get_all_legal_offensive_actions(sim.talents, sim.race, sim.policy.maintain_immolate);
                        std::vector<PriorityAction> missing_actions;
                        for (auto act : legal_actions) {
                            bool present = false;
                            for (const auto& r : child.rules) {
                                if (r.action == act) { present = true; break; }
                            }
                            if (!present) missing_actions.push_back(act);
                        }
                        if (!missing_actions.empty()) {
                            PriorityAction to_add = missing_actions[rng.next_u64() % missing_actions.size()];
                            PriorityRule new_rule = create_default_rule(to_add, sim.talents, sim.race);
                            if (child.rules.size() > 1) {
                                size_t insert_pos = 1 + (rng.next_u64() % (child.rules.size() - 1));
                                child.rules.insert(child.rules.begin() + insert_pos, new_rule);
                            } else {
                                child.rules.push_back(new_rule);
                            }
                        }
                    }

                    // Mutation 1c: Rule Pruning (Variable Length Shrink: test dropping an optional spell)
                    if (rng.next_double() < config.mutation_rate * 0.40f) {
                        std::vector<size_t> prunable_idx;
                        for (size_t k = 0; k < child.rules.size(); ++k) {
                            auto act = child.rules[k].action;
                            if (act != PriorityAction::LIFE_TAP && act != PriorityAction::SHADOW_BOLT_FILLER &&
                                act != PriorityAction::INCINERATE_FILLER && act != PriorityAction::SEARING_PAIN_FILLER &&
                                act != PriorityAction::DRAIN_LIFE_FILLER && act != PriorityAction::DRAIN_SOUL_FILLER) {
                                prunable_idx.push_back(k);
                            }
                        }
                        if (prunable_idx.size() > 1) {
                            size_t to_remove = prunable_idx[rng.next_u64() % prunable_idx.size()];
                            child.rules.erase(child.rules.begin() + to_remove);
                        }
                    }

                    // Mutation 1d: Fallback Filler Mutation
                    if (rng.next_double() < config.mutation_rate * 0.25f) {
                        auto legal_fillers = get_all_legal_fillers(sim.talents);
                        if (!legal_fillers.empty()) {
                            child.fallback_filler_action = legal_fillers[rng.next_u64() % legal_fillers.size()];
                        }
                    }

                    // Mutation 2: Tap Strategy Switch
                    if (config.allow_dual_life_tap && rng.next_double() < config.mutation_rate) {
                        child.tap_strategy = (child.tap_strategy == TapStrategy::SINGLE_TOP) ? TapStrategy::DUAL_TAP : TapStrategy::SINGLE_TOP;
                    }

                    // Mutation 3: Continuous Gene Jitter
                    if (rng.next_double() < config.mutation_rate) {
                        child.emergency_tap_mana += static_cast<float>((rng.next_double() - 0.5) * 0.05);
                    }
                    if (rng.next_double() < config.mutation_rate) {
                        child.maint_tap_mana += static_cast<float>((rng.next_double() - 0.5) * 0.08);
                    }
                    if (rng.next_double() < config.mutation_rate) {
                        child.cod_time_cutoff += static_cast<float>((rng.next_double() - 0.5) * 6.0);
                    }
                    if (rng.next_double() < config.mutation_rate) {
                        child.dot_refresh_window += static_cast<float>((rng.next_double() - 0.5) * 0.5);
                    }

                    enforce_constraints(child, sim.talents, sim.race, config.allow_dual_life_tap, sim.policy.maintain_immolate);
                    child.fitness_dps = quick_eval(child.rules, config.eval_simulations);
                    next_pop.push_back(child);
                }

                population = std::move(next_pop);
            }
        }

        // Phase 3: Final High-Precision Benchmark of Top Candidate Policies
        report_progress(0.90f, "Phase 3/3: Benchmarking Top Candidate Policies with High Precision...");

        std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
            return a.fitness_dps > b.fitness_dps;
        });

        // Collect unique candidate policy structures
        std::vector<Individual> unique_candidates;
        for (const auto& ind : population) {
            bool duplicate = false;
            for (const auto& existing : unique_candidates) {
                if (existing.rules.size() == ind.rules.size()) {
                    bool match = true;
                    for (size_t k = 0; k < ind.rules.size(); ++k) {
                        if (existing.rules[k].action != ind.rules[k].action ||
                            existing.rules[k].max_mana_pct != ind.rules[k].max_mana_pct) {
                            match = false;
                            break;
                        }
                    }
                    if (match) { duplicate = true; break; }
                }
            }
            if (!duplicate) {
                unique_candidates.push_back(ind);
                if (unique_candidates.size() >= 8) break;
            }
        }

        // Benchmark unique candidates
        std::vector<APLOptimizationResult::CandidatePolicy> evaluated_candidates;
        for (size_t c = 0; c < unique_candidates.size(); ++c) {
            const auto& cand = unique_candidates[c];
            WarlockSimulator eval_cand_sim = sim;
            eval_cand_sim.policy.custom_rules = cand.rules;
            eval_cand_sim.policy.use_custom_apl = true;
            eval_cand_sim.record_viper_samples = false;

            StatSummary s = compute_expected_dps(eval_cand_sim, config.benchmark_simulations, config.seed + c * 31);
            APLOptimizationResult::CandidatePolicy cp;
            cp.dps = s.mean_dps;
            cp.stddev = s.stddev_dps;
            cp.gain_pct = ((cp.dps - res.baseline_dps) / std::max(1.0, res.baseline_dps)) * 100.0;
            cp.rules = cand.rules;
            cp.emergency_tap_mana = cand.emergency_tap_mana;
            cp.maint_tap_mana = cand.maint_tap_mana;
            cp.curse_of_doom_cutoff = cand.cod_time_cutoff;
            cp.dot_pandemic_window = cand.dot_refresh_window;
            cp.execute_hp_threshold = cand.exec_hp_threshold;

            if (cand.tap_strategy == TapStrategy::DUAL_TAP) {
                cp.name = "Dual-Tap Weave Variant";
            } else if (c == 0) {
                cp.name = "Optimal Evolved Policy";
            } else if (cand.dot_refresh_window > 1.2f) {
                cp.name = "Aggressive Pandemic DoT Upkeep";
            } else if (cand.exec_hp_threshold > 0.30f) {
                cp.name = "Deep Execute Priority Variant";
            } else {
                cp.name = "Evolved Priority Variant #" + std::to_string(c + 1);
            }
            evaluated_candidates.push_back(cp);
        }

        // Add Baseline as candidate
        APLOptimizationResult::CandidatePolicy base_cp;
        base_cp.name = "Baseline Active APL";
        base_cp.dps = res.baseline_dps;
        base_cp.stddev = res.baseline_stddev;
        base_cp.gain_pct = 0.0;
        base_cp.rules = res.baseline_rules;
        base_cp.emergency_tap_mana = 0.15f;
        base_cp.maint_tap_mana = static_cast<float>(sim.policy.life_tap_threshold_pct) / 100.0f;
        base_cp.curse_of_doom_cutoff = 60.0f;
        base_cp.dot_pandemic_window = 1.0f;
        base_cp.execute_hp_threshold = 0.35f;
        evaluated_candidates.push_back(base_cp);

        // Sort all evaluated candidates strictly by DPS
        std::sort(evaluated_candidates.begin(), evaluated_candidates.end(), [](const auto& a, const auto& b) {
            return a.dps > b.dps;
        });

        for (size_t r = 0; r < evaluated_candidates.size(); ++r) {
            evaluated_candidates[r].rank = static_cast<int>(r + 1);
        }
        res.top_candidates = evaluated_candidates;

        // Champion is guaranteed rank 1 (strictly best among all evolved and baseline)
        const auto& champ = evaluated_candidates[0];
        res.optimized_rules = champ.rules;
        res.optimized_dps = champ.dps;
        res.optimized_stddev = champ.stddev;
        res.emergency_tap_mana = champ.emergency_tap_mana;
        res.maint_tap_mana = champ.maint_tap_mana;
        res.curse_of_doom_cutoff = champ.curse_of_doom_cutoff;
        res.dot_pandemic_window = champ.dot_pandemic_window;
        res.execute_hp_threshold = champ.execute_hp_threshold;

        res.dps_gain = res.optimized_dps - res.baseline_dps;
        res.dps_gain_pct = (res.dps_gain / std::max(1.0, res.baseline_dps)) * 100.0;

        // Compute Rule Shifts (Baseline vs Optimized)
        std::vector<APLRuleShift> shifts;
        for (size_t i = 0; i < res.optimized_rules.size(); ++i) {
            const auto& opt_r = res.optimized_rules[i];
            APLRuleShift s;
            s.action = opt_r.action;
            s.name = opt_r.name;
            s.optimized_rank = static_cast<int>(i + 1);

            for (size_t j = 0; j < res.baseline_rules.size(); ++j) {
                if (res.baseline_rules[j].action == opt_r.action) {
                    s.baseline_rank = static_cast<int>(j + 1);
                    break;
                }
            }

            if (s.baseline_rank == -1) {
                s.change_type = "NEW";
            } else if (s.optimized_rank < s.baseline_rank) {
                s.change_type = "PROMOTED";
            } else if (s.optimized_rank > s.baseline_rank) {
                s.change_type = "DEMOTED";
            } else {
                s.change_type = "UNCHANGED";
            }
            shifts.push_back(s);
        }

        // Check for pruned baseline rules
        for (size_t j = 0; j < res.baseline_rules.size(); ++j) {
            const auto& base_r = res.baseline_rules[j];
            bool in_opt = false;
            for (const auto& opt_r : res.optimized_rules) {
                if (opt_r.action == base_r.action) { in_opt = true; break; }
            }
            if (!in_opt) {
                APLRuleShift s;
                s.action = base_r.action;
                s.name = base_r.name;
                s.baseline_rank = static_cast<int>(j + 1);
                s.optimized_rank = -1;
                s.change_type = "PRUNED";
                shifts.push_back(s);
            }
        }

        res.rule_shifts = shifts;
        return res;
    }
};

} // namespace warlock
