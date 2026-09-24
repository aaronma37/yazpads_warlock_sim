#pragma once
#include "src/sim/common/sim_state_vector.hpp"
#include "src/sim/warlock/warlock_sim.hpp"
#include "src/sim/warlock/policy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <optional>
#include <unordered_map>

namespace warlock {

enum class AnalysisMode {
    BOTH = 0,
    BLUNDERS_ONLY = 1,
    FULL_MCTS_ONLY = 2
};

// Detailed statistical evaluation for a single candidate action branch in MCTS
struct ActionMCTSEval {
    PriorityAction action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string name;
    double mean_dps = 0.0;
    double stddev_dps = 0.0;
    double std_error = 0.0;
    double ci_lower_95 = 0.0;
    double ci_upper_95 = 0.0;
    size_t rollout_count = 0;
    bool is_skipped = false;
    std::string skip_reason;
};

// A single decision-point discrepancy where APL differed from the optimal MCTS choice
struct APLDivergenceEvent {
    size_t blunder_rank = 1;            // 1 = Worst move in the run, 2 = 2nd worst, etc.
    double timestamp = 0.0;
    size_t decision_step = 0;
    sim::SimObservation state;
    
    PriorityAction apl_action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string apl_action_name;
    double apl_expected_dps = 0.0;
    double apl_std_error = 0.0;

    PriorityAction mcts_action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string mcts_action_name;
    double mcts_expected_dps = 0.0;
    double mcts_std_error = 0.0;

    double delta_dps = 0.0;             // mcts_expected_dps - apl_expected_dps (DPS loss)
    double delta_dps_ci_lower = 0.0;    // 95% Confidence interval lower bound
    double delta_dps_ci_upper = 0.0;    // 95% Confidence interval upper bound
    double confidence_pct = 0.0;        // Statistical confidence that MCTS action is strictly superior
    double z_score = 0.0;

    std::vector<ActionMCTSEval> candidate_evals; // Full distribution of all evaluated branches (sorted best to worst)
    std::string top_alternatives_summary;        // Quick text summary of the highest value alternative actions
    std::string rationale;
};

// Spell Cast event for timeline Gantt/lane rendering
struct TimelineSpellBlock {
    double start_time = 0.0;
    double end_time = 0.0;
    double duration = 0.0;
    SpellID spell_id = SpellID::NONE;
    std::string spell_name;
    double damage = 0.0;
    bool is_crit = false;
    bool is_miss = false;
    std::string tag;
};

// Summary & trace log for a single simulation run
struct APLAnalysisRun {
    size_t run_index = 0;
    uint64_t seed = 0;
    double fight_duration = 180.0;
    double apl_dps = 0.0;
    double mcts_dps = 0.0;
    double dps_difference = 0.0; // mcts_dps - apl_dps

    size_t total_decisions = 0;
    size_t divergence_count = 0;
    double agreement_rate_pct = 100.0;

    std::vector<APLDivergenceEvent> events;

    // Timeline Spell Blocks for Lane Rendering
    std::vector<TimelineSpellBlock> apl_spells;
    std::vector<TimelineSpellBlock> mcts_spells;

    // Aligned time series curves for continuous plotting (0..fight_duration)
    std::vector<double> ts_time;
    std::vector<double> ts_apl_dps;
    std::vector<double> ts_mcts_dps;
    std::vector<double> ts_apl_mana;
    std::vector<double> ts_mcts_mana;

    std::vector<SpellCastLog> apl_cast_sequence;
    std::vector<SpellCastLog> mcts_cast_sequence;
};

// Top mismatched rule diagnostic
struct APLRuleMismatchStat {
    PriorityAction apl_action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string apl_action_name;
    PriorityAction preferred_mcts_action = PriorityAction::SHADOW_BOLT_FILLER;
    std::string preferred_mcts_action_name;
    size_t occurrences = 0;
    double total_dps_loss = 0.0;
    double avg_dps_loss = 0.0;
    double avg_confidence_pct = 0.0;
    std::string primary_cause;
};

// Aggregate report over N runs
struct APLAnalysisReport {
    std::vector<APLAnalysisRun> runs;
    double avg_apl_dps = 0.0;
    double avg_mcts_dps = 0.0;
    double avg_dps_loss = 0.0;
    double avg_dps_loss_pct = 0.0;

    size_t total_runs = 0;
    size_t total_decisions_evaluated = 0;
    size_t total_divergences = 0;
    double overall_agreement_pct = 100.0;

    std::vector<APLRuleMismatchStat> top_mismatches;
    bool is_valid = false;
};

class APLAnalyzer {
public:
    static std::vector<std::pair<PriorityAction, SpellID>> get_candidate_actions() {
        return {
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
    }

    static const char* get_action_clean_name(PriorityAction action) {
        switch (action) {
            case PriorityAction::LIFE_TAP: return "Life Tap";
            case PriorityAction::RACIAL_EUREKA: return "Eureka";
            case PriorityAction::RACIAL_BLOOD_FURY: return "Blood Fury";
            case PriorityAction::RACIAL_BERSERKING: return "Berserking";
            case PriorityAction::AMPLIFY_CURSE: return "Amplify Curse";
            case PriorityAction::BANE_OF_HAVOC: return "Bane of Havoc";
            case PriorityAction::NIGHTFALL_SHADOW_BOLT: return "Shadow Bolt (Trance)";
            case PriorityAction::DECIMATION_SOUL_FIRE: return "Soul Fire (Decimate)";
            case PriorityAction::DECIMATION_SEARING_PAIN: return "Searing Pain (Decimate)";
            case PriorityAction::DEMONIC_BRAND_SEARING_PAIN: return "Demonic Brand";
            case PriorityAction::CORRUPTION: return "Corruption";
            case PriorityAction::SIPHON_LIFE: return "Siphon Life";
            case PriorityAction::CURSE_OF_AGONY: return "Bane of Agony";
            case PriorityAction::CURSE_OF_DOOM: return "Bane of Doom";
            case PriorityAction::IMMOLATE: return "Immolate";
            case PriorityAction::CONFLAGRATE: return "Conflagrate";
            case PriorityAction::SHADOWBURN: return "Shadowburn";
            case PriorityAction::SHADOWBURN_ISB: return "Shadowburn (ISB)";
            case PriorityAction::DRAIN_HOPE: return "Wrack";
            case PriorityAction::INCINERATE_FILLER: return "Incinerate";
            case PriorityAction::SEARING_PAIN_FILLER: return "Searing Pain";
            case PriorityAction::DRAIN_LIFE_FILLER: return "Drain Life";
            case PriorityAction::DRAIN_SOUL_FILLER: return "Drain Soul";
            case PriorityAction::SHADOW_BOLT_FILLER: return "Shadow Bolt";
            default: return "Unknown";
        }
    }

    static const char* get_action_name(PriorityAction action) {
        return get_action_clean_name(action);
    }

    static SpellID get_action_spell_id(PriorityAction action) {
        for (const auto& [act, sp] : get_candidate_actions()) {
            if (act == action) return sp;
        }
        return SpellID::SHADOW_BOLT;
    }

    static bool is_action_legal(PriorityAction action, const sim::SimObservation& obs, const Talents& talents) {
        std::string reason;
        return check_action_legality(action, obs, talents, reason);
    }

    static bool check_action_legality(PriorityAction action, const sim::SimObservation& obs, const Talents& talents, std::string& out_reason) {
        switch (action) {
            case PriorityAction::LIFE_TAP:
                return true; // No HP constraints; healers cover Life Tap costs in raid simulation
            case PriorityAction::RACIAL_EUREKA:
                if (obs.eureka_charges <= 0.0f) { out_reason = "No Eureka Charges"; return false; }
                return true;
            case PriorityAction::RACIAL_BLOOD_FURY:
            case PriorityAction::RACIAL_BERSERKING:
                if (obs.cd_racial_sec > 0.0f) { out_reason = "Racial On Cooldown"; return false; }
                return true;
            case PriorityAction::AMPLIFY_CURSE:
                if (talents.aff.amplify_curse <= 0) { out_reason = "Not Talented"; return false; }
                if (obs.cd_amplify_curse_sec > 0.0f) { out_reason = "On Cooldown"; return false; }
                return true;
            case PriorityAction::CORRUPTION:
                if (obs.time_remaining_sec < 4.0f) { out_reason = "Fight Ending (<4s)"; return false; }
                if (obs.dot_corruption_rem_sec > 0.5f) { out_reason = "DoT Active (" + std::to_string(static_cast<int>(obs.dot_corruption_rem_sec)) + "s left)"; return false; }
                return true;
            case PriorityAction::CURSE_OF_AGONY:
                if (obs.time_remaining_sec < 6.0f) { out_reason = "Fight Ending (<6s)"; return false; }
                if (obs.dot_doom_rem_sec > 0.0f) { out_reason = "Bane of Doom Active"; return false; }
                if (obs.dot_agony_rem_sec > 0.5f) { out_reason = "DoT Active (" + std::to_string(static_cast<int>(obs.dot_agony_rem_sec)) + "s left)"; return false; }
                return true;
            case PriorityAction::CURSE_OF_DOOM:
                if (obs.time_remaining_sec < 60.0f) { out_reason = "Fight Ending (<60s)"; return false; }
                if (obs.cd_curse_of_doom_sec > 0.0f) { out_reason = "On Cooldown"; return false; }
                if (obs.dot_agony_rem_sec > 0.0f) { out_reason = "Bane of Agony Active"; return false; }
                return true;
            case PriorityAction::IMMOLATE:
                if (obs.time_remaining_sec < 3.0f) { out_reason = "Fight Ending (<3s)"; return false; }
                if (obs.dot_immolate_rem_sec > 0.5f) { out_reason = "DoT Active (" + std::to_string(static_cast<int>(obs.dot_immolate_rem_sec)) + "s left)"; return false; }
                return true;
            case PriorityAction::CONFLAGRATE:
                if (talents.destro.conflagrate <= 0) { out_reason = "Not Talented"; return false; }
                if (obs.dot_immolate_rem_sec <= 0.0f) { out_reason = "No Immolate on Target"; return false; }
                if (obs.cd_conflagrate_sec > 0.0f) { out_reason = "On Cooldown"; return false; }
                return true;
            case PriorityAction::SHADOWBURN:
                if (talents.destro.shadowburn <= 0) { out_reason = "Not Talented"; return false; }
                if (obs.cd_shadowburn_sec > 0.0f) { out_reason = "On Cooldown"; return false; }
                return true;
            case PriorityAction::SHADOWBURN_ISB:
                if (talents.destro.shadowburn <= 0) { out_reason = "Not Talented"; return false; }
                if (obs.cd_shadowburn_sec > 0.0f) { out_reason = "On Cooldown"; return false; }
                if (obs.isb_charges_rem <= 0.0f) { out_reason = "No ISB Charges"; return false; }
                return true;
            case PriorityAction::NIGHTFALL_SHADOW_BOLT:
                if (obs.nightfall_proc_active <= 0.5f) { out_reason = "No Shadow Trance Proc"; return false; }
                return true;
            case PriorityAction::DECIMATION_SOUL_FIRE:
                if (talents.demo.decimation <= 0 && obs.decimation_rem_sec <= 0.0f) { out_reason = "Not Talented"; return false; }
                if (obs.target_hp_pct > 0.35f) { out_reason = "Target HP > 35%"; return false; }
                return true;
            case PriorityAction::DECIMATION_SEARING_PAIN:
                if (talents.demo.decimation <= 0) { out_reason = "Not Talented"; return false; }
                if (obs.target_hp_pct > 0.35f) { out_reason = "Target HP > 35%"; return false; }
                if (obs.decimation_rem_sec > 0.0f) { out_reason = "Decimation Buff Active"; return false; }
                return true;
            case PriorityAction::SIPHON_LIFE:
                if (talents.aff.siphon_life <= 0) { out_reason = "Not Talented"; return false; }
                if (obs.time_remaining_sec < 6.0f) { out_reason = "Fight Ending (<6s)"; return false; }
                if (obs.dot_siphon_life_rem_sec > 0.5f) { out_reason = "DoT Active (" + std::to_string(static_cast<int>(obs.dot_siphon_life_rem_sec)) + "s left)"; return false; }
                return true;
            case PriorityAction::DRAIN_HOPE:
                if (talents.aff.drain_hope <= 0) { out_reason = "Not Talented"; return false; }
                return true;
            case PriorityAction::DEMONIC_BRAND_SEARING_PAIN:
                if (talents.demo.demonic_brand <= 0) { out_reason = "Not Talented"; return false; }
                return true;
            case PriorityAction::INCINERATE_FILLER:
                if (talents.destro.incinerate <= 0) { out_reason = "Not Talented"; return false; }
                return true;
            case PriorityAction::SEARING_PAIN_FILLER:
                return true;
            case PriorityAction::DRAIN_SOUL_FILLER:
                return true;
            case PriorityAction::DRAIN_LIFE_FILLER:
                return true;
            case PriorityAction::SHADOW_BOLT_FILLER:
                return true;
            default:
                return true;
        }
    }
public:
    // Computes statistical confidence (P(MCTS > APL)) using normal CDF erf
    static double compute_statistical_confidence(double delta_dps, double se_delta) {
        if (se_delta <= 1e-6) {
            return (delta_dps > 0.0) ? 99.9 : 50.0;
        }
        double z = delta_dps / se_delta;
        // Standard normal CDF: Phi(z) = 0.5 * (1 + erf(z / sqrt(2)))
        double p = 0.5 * (1.0 + std::erf(z / std::sqrt(2.0)));
        return std::clamp(p * 100.0, 50.0, 99.9);
    }

    // Helper to generate human-readable strategy rationale explaining why MCTS preferred its action
    static std::string generate_rationale(
        PriorityAction apl_act,
        PriorityAction mcts_act,
        const sim::SimObservation& obs,
        const Talents& talents,
        double delta_dps,
        double confidence_pct)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);

        if (apl_act == PriorityAction::LIFE_TAP) {
            ss << "APL tapped prematurely (Player Mana: " << (obs.player_mana_pct * 100.0f) << "%). MCTS verified with "
               << confidence_pct << "% confidence (+ " << delta_dps << " DPS) that mana was sufficient to continue casting DPS spells without stalling.";
            return ss.str();
        }

        if (mcts_act == PriorityAction::CORRUPTION) {
            if (obs.dot_corruption_rem_sec <= 0.5f) {
                ss << "Corruption expiring (" << obs.dot_corruption_rem_sec << "s left). MCTS refreshes to maintain tick uptime ("
                   << confidence_pct << "% confidence, +" << delta_dps << " DPS).";
            } else {
                ss << "MCTS prioritizes Corruption application for steady shadow DoT throughput (+" << delta_dps << " DPS).";
            }
            return ss.str();
        }

        if (mcts_act == PriorityAction::IMMOLATE) {
            ss << "Immolate DoT expired/expiring (" << obs.dot_immolate_rem_sec << "s left). MCTS reapplies for high DPE & Conflagrate readiness (+"
               << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::CURSE_OF_DOOM) {
            ss << "Fight duration remaining (" << obs.time_remaining_sec << "s) >= 65s. Bane of Doom provides massive DPE and saves GCD casting budget over Bane of Agony (+"
               << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::CURSE_OF_AGONY) {
            ss << "Bane of Agony expiring (" << obs.dot_agony_rem_sec << "s left). MCTS maintains curse uptime (+" << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::SIPHON_LIFE) {
            ss << "Siphon Life expiring (" << obs.dot_siphon_life_rem_sec << "s left). MCTS maintains DoT (+" << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::DECIMATION_SOUL_FIRE) {
            ss << "Target in execute range (" << (obs.target_hp_pct * 100.0f) << "% <= 35%). Decimation Soul Fire deals extreme execute DPS; APL cast slower/lower-yield filler (+"
               << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::DECIMATION_SEARING_PAIN) {
            ss << "Target <= 35% HP without Decimation buff. Fast Searing Pain cast triggers Decimation proc (+" << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::NIGHTFALL_SHADOW_BOLT) {
            ss << "Shadow Trance (Nightfall) proc active! MCTS fires instant Shadow Bolt before expiration (+" << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::CONFLAGRATE) {
            ss << "Immolate active and Conflagrate off-cooldown. MCTS consumes Immolate for instant burst before reapplication (+" << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::SHADOWBURN || mcts_act == PriorityAction::SHADOWBURN_ISB) {
            if (obs.isb_charges_rem > 0.0f) {
                ss << "Improved Shadow Bolt (ISB) active with " << static_cast<int>(obs.isb_charges_rem) << " charges (+20% Shadow). MCTS capitalizes with instant Shadowburn (+" << delta_dps << " DPS).";
            } else if (obs.time_remaining_sec <= 8.0f || obs.target_hp_pct <= 0.20f) {
                ss << "Encounter ending (" << obs.time_remaining_sec << "s left). Instant Shadowburn completes burst (+" << delta_dps << " DPS).";
            } else {
                ss << "Shadowburn off-cooldown; MCTS weaves instant burst (+" << delta_dps << " DPS).";
            }
            return ss.str();
        }

        if (mcts_act == PriorityAction::LIFE_TAP) {
            if (obs.player_mana_pct <= 0.15f) {
                ss << "Mana critically low (" << (obs.player_mana_pct * 100.0f) << "%). MCTS taps now to prevent hard OOM stalling on upcoming casts (+" << delta_dps << " DPS).";
            } else {
                ss << "MCTS weaves Life Tap during safe window to maintain mana reserves (+" << delta_dps << " DPS).";
            }
            return ss.str();
        }

        if (mcts_act == PriorityAction::INCINERATE_FILLER) {
            ss << "Fire Destro synergy: Incinerate filler scales higher than alternative spells (+" << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::SHADOW_BOLT_FILLER) {
            ss << "MCTS forward rollouts confirmed Shadow Bolt filler yields higher expected throughput than alternative actions (+" << delta_dps << " DPS, " << confidence_pct << "% confidence).";
            return ss.str();
        }

        ss << "MCTS forward rollouts evaluated higher expected yield: +" << delta_dps << " DPS (" << confidence_pct << "% confidence).";
        return ss.str();
    }

    // Evaluates all candidate action branches simultaneously using Common Random Numbers (CRN)
    // Supports Hard-Capped Adaptive Rollouts with statistical confidence early-stopping and branch pruning
    static std::vector<ActionMCTSEval> evaluate_candidate_branches_crn(
        const WarlockSimulator& base_sim,
        const std::vector<PriorityAction>& prefix,
        const std::vector<PriorityAction>& candidate_actions,
        size_t rollouts_count = 256,
        uint64_t base_seed = 42,
        bool adaptive_rollouts = true,
        size_t min_rollouts = 16)
    {
        size_t K = candidate_actions.size();
        if (K == 0) return {};

        std::vector<ActionMCTSEval> results(K);
        for (size_t k = 0; k < K; ++k) {
            results[k].action = candidate_actions[k];
            results[k].name = get_action_name(candidate_actions[k]);
            results[k].rollout_count = 0;
        }

        size_t max_rollouts = std::max(size_t(2), rollouts_count);
        size_t pilot = std::min(max_rollouts, std::max(size_t(4), min_rollouts));
        size_t step_batch = 16;

        std::vector<double> sum_dps(K, 0.0);
        std::vector<double> sum_sq_dps(K, 0.0);
        std::vector<size_t> counts(K, 0);
        std::vector<bool> is_active(K, true);

        size_t current_rollout = 0;

        while (current_rollout < max_rollouts) {
            size_t batch_target = (current_rollout == 0)
                ? pilot
                : std::min(max_rollouts, current_rollout + step_batch);

            for (size_t r = current_rollout; r < batch_target; ++r) {
                uint64_t rollout_seed = base_seed + r * 7919 + 17;

                for (size_t k = 0; k < K; ++k) {
                    if (!is_active[k]) continue;

                    FastRNG rollout_rng(rollout_seed); // CRN: exact same seed for all candidate branches in trial r!
                    WarlockSimulator rollout_sim = base_sim;
                    rollout_sim.randomize_duration = false;
                    rollout_sim.record_timeline = false;
                    rollout_sim.record_viper_samples = false;
                    rollout_sim.use_oracle_execution_policy = false;
                    
                    std::vector<PriorityAction> branch_prefix = prefix;
                    branch_prefix.push_back(candidate_actions[k]);
                    rollout_sim.forced_action_prefix = std::move(branch_prefix);

                    SimResult res = rollout_sim.run_single_simulation(rollout_rng);
                    sum_dps[k] += res.dps;
                    sum_sq_dps[k] += res.dps * res.dps;
                    counts[k]++;
                }
            }

            current_rollout = batch_target;

            // If adaptive early stopping is enabled and we have evaluated at least pilot rollouts
            if (adaptive_rollouts && current_rollout >= pilot && current_rollout < max_rollouts) {
                struct CandStat {
                    size_t idx;
                    double mean;
                    double se;
                };
                std::vector<CandStat> active_stats;
                for (size_t k = 0; k < K; ++k) {
                    if (counts[k] > 0 && is_active[k]) {
                        double n = static_cast<double>(counts[k]);
                        double mean = sum_dps[k] / n;
                        double var = (n > 1.0) ? std::max(0.0, (sum_sq_dps[k] - (sum_dps[k] * sum_dps[k] / n)) / (n - 1.0)) : 0.0;
                        double se = std::sqrt(var) / std::sqrt(n);
                        active_stats.push_back({k, mean, se});
                    }
                }

                if (active_stats.size() >= 2) {
                    std::sort(active_stats.begin(), active_stats.end(), [](const CandStat& a, const CandStat& b) {
                        return a.mean > b.mean;
                    });

                    double delta = active_stats[0].mean - active_stats[1].mean;
                    double se_diff = std::sqrt(active_stats[0].se * active_stats[0].se + active_stats[1].se * active_stats[1].se);

                    // 1. Decisive winner: Z-score >= 2.576 (99% confidence separation)
                    if (se_diff > 1e-5 && (delta / se_diff) >= 2.576) {
                        break;
                    }

                    // 2. Statistical tie: negligible difference (<0.05 DPS) with tight error bound (<0.05 DPS)
                    if (delta < 0.05 && se_diff < 0.05) {
                        break;
                    }

                    // 3. Prune clearly inferior candidate branches from subsequent rollout batches
                    double best_ci_lower = active_stats[0].mean - 2.576 * active_stats[0].se;
                    for (size_t i = 1; i < active_stats.size(); ++i) {
                        double cand_ci_upper = active_stats[i].mean + 2.576 * active_stats[i].se;
                        if (cand_ci_upper < best_ci_lower) {
                            is_active[active_stats[i].idx] = false;
                        }
                    }
                } else if (active_stats.size() <= 1) {
                    break;
                }
            }
        }

        // Calculate final sample statistics for each branch
        for (size_t k = 0; k < K; ++k) {
            double n = static_cast<double>(std::max(size_t(1), counts[k]));
            results[k].rollout_count = counts[k];
            results[k].mean_dps = sum_dps[k] / n;
            double var = (n > 1.0) ? std::max(0.0, (sum_sq_dps[k] - (sum_dps[k] * sum_dps[k] / n)) / (n - 1.0)) : 0.0;
            results[k].stddev_dps = std::sqrt(var);
            results[k].std_error = results[k].stddev_dps / std::sqrt(n);
            results[k].ci_lower_95 = results[k].mean_dps - 1.96 * results[k].std_error;
            results[k].ci_upper_95 = results[k].mean_dps + 1.96 * results[k].std_error;
        }

        // Sort candidate branches from highest expected DPS to lowest
        std::sort(results.begin(), results.end(), [](const ActionMCTSEval& a, const ActionMCTSEval& b) {
            return a.mean_dps > b.mean_dps;
        });

        return results;
    }

    // Helper to generate aligned timeline time series and spell Gantt blocks for APL vs MCTS
    static void build_run_timeline_data(
        APLAnalysisRun& run,
        const SimResult& apl_res,
        const SimResult& mcts_res,
        double fight_duration)
    {
        run.apl_cast_sequence = apl_res.cast_sequence;
        run.mcts_cast_sequence = mcts_res.cast_sequence;

        // 1. Build APL Spell Blocks
        run.apl_spells.clear();
        for (const auto& cast : apl_res.cast_sequence) {
            TimelineSpellBlock b;
            b.start_time = cast.time;
            b.duration = (cast.cast_time > 0.0) ? cast.cast_time : 1.5;
            b.end_time = b.start_time + b.duration;
            b.spell_id = cast.spell_id;
            b.spell_name = spell_id_to_name(cast.spell_id);
            b.damage = cast.damage;
            b.is_crit = cast.is_crit;
            b.is_miss = cast.is_miss;
            b.tag = cast.tag;
            run.apl_spells.push_back(b);
        }

        // 2. Build MCTS Spell Blocks
        run.mcts_spells.clear();
        for (const auto& cast : mcts_res.cast_sequence) {
            TimelineSpellBlock b;
            b.start_time = cast.time;
            b.duration = (cast.cast_time > 0.0) ? cast.cast_time : 1.5;
            b.end_time = b.start_time + b.duration;
            b.spell_id = cast.spell_id;
            b.spell_name = spell_id_to_name(cast.spell_id);
            b.damage = cast.damage;
            b.is_crit = cast.is_crit;
            b.is_miss = cast.is_miss;
            b.tag = cast.tag;
            run.mcts_spells.push_back(b);
        }

        // 3. Build Aligned Time Series for Continuous Plotting (0.5s intervals)
        run.ts_time.clear();
        run.ts_apl_dps.clear();
        run.ts_mcts_dps.clear();
        run.ts_apl_mana.clear();
        run.ts_mcts_mana.clear();

        double dt = 0.5;
        size_t num_samples = static_cast<size_t>(std::ceil(fight_duration / dt)) + 1;
        run.ts_time.reserve(num_samples);
        run.ts_apl_dps.reserve(num_samples);
        run.ts_mcts_dps.reserve(num_samples);
        run.ts_apl_mana.reserve(num_samples);
        run.ts_mcts_mana.reserve(num_samples);

        size_t apl_t_idx = 0;
        double apl_cum_dmg = 0.0;
        double apl_cur_mana = apl_res.timeline.empty() ? 3000.0 : apl_res.timeline.front().mana;
        double apl_first_dmg_time = -1.0;

        size_t mcts_t_idx = 0;
        double mcts_cum_dmg = 0.0;
        double mcts_cur_mana = mcts_res.timeline.empty() ? 3000.0 : mcts_res.timeline.front().mana;
        double mcts_first_dmg_time = -1.0;

        for (double t = 0.0; t <= fight_duration + 1e-4; t += dt) {
            // Advance APL timeline up to time t
            while (apl_t_idx < apl_res.timeline.size() && apl_res.timeline[apl_t_idx].time <= t) {
                if (apl_res.timeline[apl_t_idx].damage > 0.0 && apl_first_dmg_time < 0.0) {
                    apl_first_dmg_time = apl_res.timeline[apl_t_idx].time;
                }
                apl_cum_dmg += apl_res.timeline[apl_t_idx].damage;
                apl_cur_mana = apl_res.timeline[apl_t_idx].mana;
                apl_t_idx++;
            }

            // Advance MCTS timeline up to time t
            while (mcts_t_idx < mcts_res.timeline.size() && mcts_res.timeline[mcts_t_idx].time <= t) {
                if (mcts_res.timeline[mcts_t_idx].damage > 0.0 && mcts_first_dmg_time < 0.0) {
                    mcts_first_dmg_time = mcts_res.timeline[mcts_t_idx].time;
                }
                mcts_cum_dmg += mcts_res.timeline[mcts_t_idx].damage;
                mcts_cur_mana = mcts_res.timeline[mcts_t_idx].mana;
                mcts_t_idx++;
            }

            double apl_dps_at_t = (t >= 1.0) ? (apl_cum_dmg / t) : (apl_cum_dmg > 0 ? (apl_cum_dmg / std::max(0.5, t)) : 0.0);
            double mcts_dps_at_t = (t >= 1.0) ? (mcts_cum_dmg / t) : (mcts_cum_dmg > 0 ? (mcts_cum_dmg / std::max(0.5, t)) : 0.0);

            run.ts_time.push_back(t);
            run.ts_apl_dps.push_back(apl_dps_at_t);
            run.ts_mcts_dps.push_back(mcts_dps_at_t);
            run.ts_apl_mana.push_back(apl_cur_mana);
            run.ts_mcts_mana.push_back(mcts_cur_mana);
        }
    }

    struct AsyncBlunderAnalysisSession {
        WarlockSimulator base_sim;
        size_t rollouts_per_action = 512;
        bool adaptive_rollouts = true;
        uint64_t seed = 42;

        SimResult apl_res;
        WarlockSimulator apl_sim;
        size_t total_decision_steps = 0;

        std::atomic<size_t> next_d{0};
        std::atomic<size_t> completed_d{0};
        std::vector<std::optional<APLDivergenceEvent>> decision_events;

        bool initialized = false;
        bool completed = false;

        void init(const WarlockSimulator& sim, size_t rollouts = 512, bool adaptive = true, uint64_t base_seed = 42) {
            base_sim = sim;
            rollouts_per_action = rollouts;
            adaptive_rollouts = adaptive;
            seed = base_seed;

            FastRNG rng(seed);
            apl_sim = base_sim;
            apl_sim.randomize_duration = false;
            apl_sim.record_timeline = false;
            apl_sim.record_viper_samples = true;
            apl_sim.viper_dataset.clear();
            apl_sim.use_oracle_execution_policy = false;
            apl_sim.forced_action_prefix.clear();

            apl_res = apl_sim.run_single_simulation(rng);
            total_decision_steps = std::min(apl_res.action_history.size(), apl_sim.viper_dataset.samples.size());
            decision_events.clear();
            decision_events.resize(total_decision_steps);
            next_d.store(0);
            completed_d.store(0);
            initialized = true;
            completed = false;
        }

        void eval_decision(size_t d) {
            if (d >= total_decision_steps) return;
            const auto& apl_sample = apl_sim.viper_dataset.samples[d];
            const sim::SimObservation& apl_state = apl_sample.state;
            PriorityAction chosen_apl_act = apl_res.action_history[d];

            auto candidate_actions = APLAnalyzer::get_candidate_actions();
            std::vector<PriorityAction> legal_candidates;
            for (const auto& [act, _] : candidate_actions) {
                if (act == PriorityAction::RACIAL_EUREKA ||
                    act == PriorityAction::RACIAL_BLOOD_FURY ||
                    act == PriorityAction::RACIAL_BERSERKING ||
                    act == PriorityAction::AMPLIFY_CURSE ||
                    act == PriorityAction::BANE_OF_HAVOC) {
                    continue;
                }
                if (APLAnalyzer::is_action_legal(act, apl_state, base_sim.talents)) {
                    legal_candidates.push_back(act);
                }
            }
            if (std::find(legal_candidates.begin(), legal_candidates.end(), chosen_apl_act) == legal_candidates.end()) {
                legal_candidates.push_back(chosen_apl_act);
            }

            std::vector<PriorityAction> apl_prefix(apl_res.action_history.begin(), apl_res.action_history.begin() + d);

            auto evaluated_branches = APLAnalyzer::evaluate_candidate_branches_crn(
                base_sim,
                apl_prefix,
                legal_candidates,
                rollouts_per_action,
                seed + (d + 500) * 65537 + 19,
                adaptive_rollouts
            );

            ActionMCTSEval best_eval = evaluated_branches.empty() ? ActionMCTSEval{} : evaluated_branches.front();
            ActionMCTSEval apl_eval;
            apl_eval.mean_dps = -1e9;

            for (const auto& e : evaluated_branches) {
                if (e.action == chosen_apl_act) {
                    apl_eval = e;
                    break;
                }
            }
            if (apl_eval.mean_dps < -1e8) {
                apl_eval = best_eval;
            }

            double delta_dps = std::max(0.0, best_eval.mean_dps - apl_eval.mean_dps);
            double se_diff = std::sqrt(best_eval.std_error * best_eval.std_error + apl_eval.std_error * apl_eval.std_error);
            double z_score = (se_diff > 1e-5) ? (delta_dps / se_diff) : 0.0;
            double confidence = APLAnalyzer::compute_statistical_confidence(delta_dps, se_diff);

            if (best_eval.action != chosen_apl_act && delta_dps >= 1.0 && confidence >= 60.0) {
                APLDivergenceEvent ev;
                ev.timestamp = apl_state.fight_progress_pct * base_sim.fight_duration;
                ev.decision_step = d;
                ev.state = apl_state;

                ev.apl_action = chosen_apl_act;
                ev.apl_action_name = APLAnalyzer::get_action_name(chosen_apl_act);
                ev.apl_expected_dps = apl_eval.mean_dps;
                ev.apl_std_error = apl_eval.std_error;

                ev.mcts_action = best_eval.action;
                ev.mcts_action_name = APLAnalyzer::get_action_name(best_eval.action);
                ev.mcts_expected_dps = best_eval.mean_dps;
                ev.mcts_std_error = best_eval.std_error;

                ev.delta_dps = delta_dps;
                ev.delta_dps_ci_lower = std::max(0.0, delta_dps - 1.96 * se_diff);
                ev.delta_dps_ci_upper = delta_dps + 1.96 * se_diff;
                ev.confidence_pct = confidence;
                ev.z_score = z_score;

                ev.candidate_evals = evaluated_branches;
                for (const auto& [act, _] : candidate_actions) {
                    if (act == PriorityAction::RACIAL_EUREKA ||
                        act == PriorityAction::RACIAL_BLOOD_FURY ||
                        act == PriorityAction::RACIAL_BERSERKING ||
                        act == PriorityAction::AMPLIFY_CURSE ||
                        act == PriorityAction::BANE_OF_HAVOC) {
                        continue;
                    }
                    if (std::find(legal_candidates.begin(), legal_candidates.end(), act) == legal_candidates.end()) {
                        std::string reason;
                        APLAnalyzer::check_action_legality(act, apl_state, base_sim.talents, reason);
                        ActionMCTSEval skipped_eval;
                        skipped_eval.action = act;
                        skipped_eval.name = APLAnalyzer::get_action_name(act);
                        skipped_eval.is_skipped = true;
                        skipped_eval.skip_reason = reason.empty() ? "Conditions not met" : reason;
                        ev.candidate_evals.push_back(std::move(skipped_eval));
                    }
                }

                std::ostringstream alt_ss;
                alt_ss << std::fixed << std::setprecision(1);
                size_t alt_count = 0;
                for (const auto& cand : evaluated_branches) {
                    if (cand.action == chosen_apl_act) continue;
                    if (alt_count > 0) alt_ss << ", ";
                    double cand_gain = cand.mean_dps - apl_eval.mean_dps;
                    alt_ss << cand.name << " (" << (cand_gain >= 0 ? "+" : "") << cand_gain << " DPS)";
                    alt_count++;
                    if (alt_count >= 3) break;
                }
                ev.top_alternatives_summary = alt_ss.str();

                ev.rationale = APLAnalyzer::generate_rationale(chosen_apl_act, best_eval.action, apl_state, base_sim.talents, delta_dps, confidence);

                decision_events[d] = std::move(ev);
            }
        }

        APLAnalysisReport finalize() {
            APLAnalysisRun run;
            run.run_index = 1;
            run.seed = seed;
            run.fight_duration = apl_res.duration;
            run.apl_dps = apl_res.dps;
            run.total_decisions = total_decision_steps;

            for (size_t d = 0; d < total_decision_steps; ++d) {
                if (decision_events[d].has_value()) {
                    run.events.push_back(std::move(decision_events[d].value()));
                }
            }

            std::sort(run.events.begin(), run.events.end(), [](const APLDivergenceEvent& a, const APLDivergenceEvent& b) {
                if (std::abs(a.delta_dps - b.delta_dps) > 1e-4) {
                    return a.delta_dps > b.delta_dps;
                }
                return a.confidence_pct > b.confidence_pct;
            });

            for (size_t i = 0; i < run.events.size(); ++i) {
                run.events[i].blunder_rank = i + 1;
            }

            run.divergence_count = run.events.size();
            if (run.total_decisions > 0) {
                size_t matching = (run.total_decisions > run.divergence_count) ? (run.total_decisions - run.divergence_count) : 0;
                run.agreement_rate_pct = (static_cast<double>(matching) / static_cast<double>(run.total_decisions)) * 100.0;
            } else {
                run.agreement_rate_pct = 100.0;
            }

            APLAnalysisReport report;
            report.total_runs = 1;
            report.avg_apl_dps = run.apl_dps;
            report.avg_mcts_dps = run.apl_dps;
            report.total_decisions_evaluated = run.total_decisions;
            report.total_divergences = run.divergence_count;
            report.overall_agreement_pct = run.agreement_rate_pct;
            report.runs.push_back(std::move(run));
            report.is_valid = true;
            completed = true;
            return report;
        }
    };

    // Evaluates a single trace run using two distinct MCTS processes:
    // 1. Autonomous MCTS Policy Trajectory (Ghost reference playthrough for continuous timeline & max yield benchmark)
    // 2. APL Decision Judge & Blunder Detector (evaluates APL's exact decision points, states, and blunders ranked worst-first)
    static APLAnalysisRun analyze_single_run(
        const WarlockSimulator& base_sim,
        size_t run_idx,
        uint64_t seed,
        size_t rollouts_per_action = 256,
        AnalysisMode mode = AnalysisMode::BOTH,
        bool adaptive_rollouts = true,
        std::function<void(float progress, const std::string& status)> progress_cb = nullptr,
        size_t max_threads = 0)
    {
        APLAnalysisRun run;
        run.run_index = run_idx;
        run.seed = seed;
        run.fight_duration = base_sim.fight_duration;

        auto candidate_actions = get_candidate_actions();

        // ---------------------------------------------------------------------
        // STEP 1: Run Full APL Baseline Episode
        // ---------------------------------------------------------------------
        FastRNG rng(seed);
        WarlockSimulator apl_sim = base_sim;
        apl_sim.randomize_duration = false;
        apl_sim.record_timeline = (mode != AnalysisMode::BLUNDERS_ONLY);
        apl_sim.record_viper_samples = true;
        apl_sim.viper_dataset.clear();
        apl_sim.use_oracle_execution_policy = false;
        apl_sim.forced_action_prefix.clear();

        SimResult apl_res = apl_sim.run_single_simulation(rng);
        run.apl_dps = apl_res.dps;
        run.fight_duration = apl_res.duration;
        run.total_decisions = apl_sim.viper_dataset.samples.size();

        SimResult mcts_res;

        // ---------------------------------------------------------------------
        // MCTS PROCESS 1: Autonomous MCTS Trajectory ("Ghost" Optimal Playthrough)
        // ---------------------------------------------------------------------
        if (mode == AnalysisMode::BOTH || mode == AnalysisMode::FULL_MCTS_ONLY) {
            std::vector<PriorityAction> mcts_optimal_action_sequence;
            mcts_optimal_action_sequence.reserve(std::max(size_t(32), run.total_decisions));

            size_t mcts_step = 0;
            const size_t max_allowed_decisions = 250;

            while (mcts_step < max_allowed_decisions) {
                FastRNG live_rng(seed);
                WarlockSimulator live_sim = base_sim;
                live_sim.randomize_duration = false;
                live_sim.record_timeline = false;
                live_sim.record_viper_samples = true;
                live_sim.viper_dataset.clear();
                live_sim.use_oracle_execution_policy = false;
                live_sim.forced_action_prefix = mcts_optimal_action_sequence;

                SimResult step_probe_res = live_sim.run_single_simulation(live_rng);

                if (step_probe_res.action_history.size() <= mcts_optimal_action_sequence.size()) {
                    break;
                }

                const auto& live_sample = live_sim.viper_dataset.samples[mcts_step];
                const sim::SimObservation& live_obs = live_sample.state;
                PriorityAction default_act = (mcts_step < step_probe_res.action_history.size())
                                                 ? step_probe_res.action_history[mcts_step]
                                                 : static_cast<PriorityAction>(live_sample.oracle_action);

                std::vector<PriorityAction> legal_candidates;
                for (const auto& [act, _] : candidate_actions) {
                    if (act == PriorityAction::RACIAL_EUREKA ||
                        act == PriorityAction::RACIAL_BLOOD_FURY ||
                        act == PriorityAction::RACIAL_BERSERKING ||
                        act == PriorityAction::AMPLIFY_CURSE ||
                        act == PriorityAction::BANE_OF_HAVOC) {
                        continue;
                    }
                    if (is_action_legal(act, live_obs, base_sim.talents)) {
                        legal_candidates.push_back(act);
                    }
                }
                if (std::find(legal_candidates.begin(), legal_candidates.end(), default_act) == legal_candidates.end()) {
                    legal_candidates.push_back(default_act);
                }

                auto evals = evaluate_candidate_branches_crn(
                    base_sim,
                    mcts_optimal_action_sequence,
                    legal_candidates,
                    rollouts_per_action,
                    seed + mcts_step * 65537 + 13,
                    adaptive_rollouts
                );

                PriorityAction chosen_action = default_act;
                if (!evals.empty()) {
                    chosen_action = evals[0].action; // evals are sorted descending by mean_dps
                }
                mcts_optimal_action_sequence.push_back(chosen_action);
                mcts_step++;
            }

            // Run full MCTS simulation to get timeline and spell sequence
            FastRNG mcts_rng(seed);
            WarlockSimulator mcts_sim = base_sim;
            mcts_sim.randomize_duration = false;
            mcts_sim.record_timeline = true;
            mcts_sim.record_viper_samples = false;
            mcts_sim.use_oracle_execution_policy = false;
            mcts_sim.forced_action_prefix = mcts_optimal_action_sequence;

            mcts_res = mcts_sim.run_single_simulation(mcts_rng);
            run.mcts_dps = mcts_res.dps;
            run.dps_difference = run.mcts_dps - run.apl_dps;
        }

        // ---------------------------------------------------------------------
        // MCTS PROCESS 2: APL Decision Judge & Blunder Detector
        // Evaluates each action the APL made during its ACTUAL simulation run
        // Massively multithreaded across all decision points
        // ---------------------------------------------------------------------
        if (mode == AnalysisMode::BOTH || mode == AnalysisMode::BLUNDERS_ONLY) {
            size_t total_decision_steps = std::min(apl_res.action_history.size(), apl_sim.viper_dataset.samples.size());
            std::vector<std::optional<APLDivergenceEvent>> decision_events(total_decision_steps);

            auto eval_single_decision = [&](size_t d) {
                const auto& apl_sample = apl_sim.viper_dataset.samples[d];
                const sim::SimObservation& apl_state = apl_sample.state;
                PriorityAction chosen_apl_act = apl_res.action_history[d];

                std::vector<PriorityAction> legal_candidates;
                for (const auto& [act, _] : candidate_actions) {
                    if (act == PriorityAction::RACIAL_EUREKA ||
                        act == PriorityAction::RACIAL_BLOOD_FURY ||
                        act == PriorityAction::RACIAL_BERSERKING ||
                        act == PriorityAction::AMPLIFY_CURSE ||
                        act == PriorityAction::BANE_OF_HAVOC) {
                        continue;
                    }
                    if (is_action_legal(act, apl_state, base_sim.talents)) {
                        legal_candidates.push_back(act);
                    }
                }
                if (std::find(legal_candidates.begin(), legal_candidates.end(), chosen_apl_act) == legal_candidates.end()) {
                    legal_candidates.push_back(chosen_apl_act);
                }

                std::vector<PriorityAction> apl_prefix(apl_res.action_history.begin(), apl_res.action_history.begin() + d);

                // Execute Common Random Numbers (CRN) Monte Carlo Forward Rollouts from the APL prefix
                auto evaluated_branches = evaluate_candidate_branches_crn(
                    base_sim,
                    apl_prefix,
                    legal_candidates,
                    rollouts_per_action,
                    seed + (d + 500) * 65537 + 19,
                    adaptive_rollouts
                );

                ActionMCTSEval best_eval = evaluated_branches.empty() ? ActionMCTSEval{} : evaluated_branches.front();
                ActionMCTSEval apl_eval;
                apl_eval.mean_dps = -1e9;

                for (const auto& e : evaluated_branches) {
                    if (e.action == chosen_apl_act) {
                        apl_eval = e;
                        break;
                    }
                }
                if (apl_eval.mean_dps < -1e8) {
                    apl_eval = best_eval;
                }

                double delta_dps = std::max(0.0, best_eval.mean_dps - apl_eval.mean_dps);
                double se_diff = std::sqrt(best_eval.std_error * best_eval.std_error + apl_eval.std_error * apl_eval.std_error);
                double z_score = (se_diff > 1e-5) ? (delta_dps / se_diff) : 0.0;
                double confidence = compute_statistical_confidence(delta_dps, se_diff);

                // If MCTS found a statistically superior action over what the APL did at this exact state
                if (best_eval.action != chosen_apl_act && delta_dps >= 1.0 && confidence >= 60.0) {
                    APLDivergenceEvent ev;
                    ev.timestamp = apl_state.fight_progress_pct * base_sim.fight_duration;
                    ev.decision_step = d;
                    ev.state = apl_state;

                    ev.apl_action = chosen_apl_act;
                    ev.apl_action_name = get_action_name(chosen_apl_act);
                    ev.apl_expected_dps = apl_eval.mean_dps;
                    ev.apl_std_error = apl_eval.std_error;

                    ev.mcts_action = best_eval.action;
                    ev.mcts_action_name = get_action_name(best_eval.action);
                    ev.mcts_expected_dps = best_eval.mean_dps;
                    ev.mcts_std_error = best_eval.std_error;

                    ev.delta_dps = delta_dps;
                    ev.delta_dps_ci_lower = std::max(0.0, delta_dps - 1.96 * se_diff);
                    ev.delta_dps_ci_upper = delta_dps + 1.96 * se_diff;
                    ev.confidence_pct = confidence;
                    ev.z_score = z_score;

                    // Build full candidate action list: Evaluated actions first (sorted by DPS), followed by Skipped actions
                    ev.candidate_evals = evaluated_branches;
                    for (const auto& [act, _] : candidate_actions) {
                        if (act == PriorityAction::RACIAL_EUREKA ||
                            act == PriorityAction::RACIAL_BLOOD_FURY ||
                            act == PriorityAction::RACIAL_BERSERKING ||
                            act == PriorityAction::AMPLIFY_CURSE ||
                            act == PriorityAction::BANE_OF_HAVOC) {
                            continue;
                        }
                        if (std::find(legal_candidates.begin(), legal_candidates.end(), act) == legal_candidates.end()) {
                            std::string reason;
                            check_action_legality(act, apl_state, base_sim.talents, reason);
                            ActionMCTSEval skipped_eval;
                            skipped_eval.action = act;
                            skipped_eval.name = get_action_name(act);
                            skipped_eval.is_skipped = true;
                            skipped_eval.skip_reason = reason.empty() ? "Conditions not met" : reason;
                            ev.candidate_evals.push_back(std::move(skipped_eval));
                        }
                    }

                    // Summarize top alternative actions evaluated at this state
                    std::ostringstream alt_ss;
                    alt_ss << std::fixed << std::setprecision(1);
                    size_t alt_count = 0;
                    for (const auto& cand : evaluated_branches) {
                        if (cand.action == chosen_apl_act) continue;
                        if (alt_count > 0) alt_ss << ", ";
                        double cand_gain = cand.mean_dps - apl_eval.mean_dps;
                        alt_ss << cand.name << " (" << (cand_gain >= 0 ? "+" : "") << cand_gain << " DPS)";
                        alt_count++;
                        if (alt_count >= 3) break;
                    }
                    ev.top_alternatives_summary = alt_ss.str();

                    ev.rationale = generate_rationale(chosen_apl_act, best_eval.action, apl_state, base_sim.talents, delta_dps, confidence);

                    decision_events[d] = std::move(ev);
                }
            };

            if (total_decision_steps > 0) {
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
                for (size_t d = 0; d < total_decision_steps; ++d) {
                    eval_single_decision(d);
                    if (progress_cb) {
                        float prog = 0.05f + 0.90f * (static_cast<float>(d + 1) / static_cast<float>(total_decision_steps));
                        progress_cb(prog, "Evaluating APL Decision #" + std::to_string(d + 1) + " / " + std::to_string(total_decision_steps) + "...");
                    }
                }
#else
                unsigned int hw_threads = std::max(1u, std::thread::hardware_concurrency());
#if defined(__EMSCRIPTEN__)
                // In Emscripten, reserve 1 pool worker for the parent async thread to prevent pool deadlock
                size_t avail_threads = (hw_threads > 1) ? (hw_threads - 1) : 1;
#else
                size_t avail_threads = hw_threads;
#endif
                size_t num_workers = (max_threads > 0) ? std::min(max_threads, avail_threads) : avail_threads;
                num_workers = std::min(num_workers, total_decision_steps);

                if (num_workers <= 1) {
                    for (size_t d = 0; d < total_decision_steps; ++d) {
                        eval_single_decision(d);
                        if (progress_cb) {
                            float prog = 0.05f + 0.90f * (static_cast<float>(d + 1) / static_cast<float>(total_decision_steps));
                            progress_cb(prog, "Evaluating APL Decision #" + std::to_string(d + 1) + " / " + std::to_string(total_decision_steps) + "...");
                        }
                    }
                } else {
                    std::atomic<size_t> next_d{0};
                    std::atomic<size_t> completed_d{0};
                    std::vector<std::thread> workers;
                    workers.reserve(num_workers);

                    for (size_t t = 0; t < num_workers; ++t) {
                        workers.emplace_back([&]() {
                            while (true) {
                                size_t d = next_d.fetch_add(1, std::memory_order_relaxed);
                                if (d >= total_decision_steps) break;

                                eval_single_decision(d);

                                size_t done = completed_d.fetch_add(1, std::memory_order_relaxed) + 1;
                                if (progress_cb) {
                                    float prog = 0.05f + 0.90f * (static_cast<float>(done) / static_cast<float>(total_decision_steps));
                                    progress_cb(prog, "Evaluating APL Decision #" + std::to_string(done) + " / " + std::to_string(total_decision_steps) + "...");
                                }
                            }
                        });
                    }

                    for (auto& w : workers) {
                        if (w.joinable()) w.join();
                    }
                }
#endif
            }

            // Collect detected divergence events
            for (size_t d = 0; d < total_decision_steps; ++d) {
                if (decision_events[d].has_value()) {
                    run.events.push_back(std::move(decision_events[d].value()));
                }
            }

            // Rank blunders from WORST move to least severe (highest delta_dps loss first)
            std::sort(run.events.begin(), run.events.end(), [](const APLDivergenceEvent& a, const APLDivergenceEvent& b) {
                if (std::abs(a.delta_dps - b.delta_dps) > 1e-4) {
                    return a.delta_dps > b.delta_dps;
                }
                return a.confidence_pct > b.confidence_pct;
            });

            for (size_t i = 0; i < run.events.size(); ++i) {
                run.events[i].blunder_rank = i + 1;
            }

            run.divergence_count = run.events.size();
            if (run.total_decisions > 0) {
                size_t matching = (run.total_decisions > run.divergence_count) ? (run.total_decisions - run.divergence_count) : 0;
                run.agreement_rate_pct = (static_cast<double>(matching) / static_cast<double>(run.total_decisions)) * 100.0;
            } else {
                run.agreement_rate_pct = 100.0;
            }
        }

        // ---------------------------------------------------------------------
        // STEP 4: Construct Aligned Timeline Time Series & Spell Gantt Blocks
        // ---------------------------------------------------------------------
        if (mode == AnalysisMode::BOTH || mode == AnalysisMode::FULL_MCTS_ONLY) {
            build_run_timeline_data(run, apl_res, mcts_res, run.fight_duration);
        }

        return run;
    }

    // Runs a batch APL vs MCTS analysis across multiple runs with multi-threading
    static APLAnalysisReport run_analysis(
        const WarlockSimulator& sim,
        size_t num_runs = 5,
        size_t rollouts_per_action = 256,
        std::function<void(float progress, const std::string& status)> progress_cb = nullptr,
        uint64_t base_seed = 42,
        AnalysisMode mode = AnalysisMode::BOTH,
        bool adaptive_rollouts = true)
    {
        APLAnalysisReport report;
        report.total_runs = std::max(size_t(1), num_runs);

        auto report_progress = [&](float p, const std::string& msg) {
            if (progress_cb) progress_cb(p, msg);
        };

        const char* start_msg = (mode == AnalysisMode::BLUNDERS_ONLY)
            ? "Launching APL Decision Blunder Rollout Workers..."
            : (mode == AnalysisMode::FULL_MCTS_ONLY)
                ? "Launching Autonomous MCTS Optimal Policy Rollout Workers..."
                : "Launching Parallel MCTS Trace Workers...";
        report_progress(0.05f, start_msg);

        std::vector<APLAnalysisRun> thread_runs(report.total_runs);

        if (report.total_runs == 1) {
            // For a single run (such as standard Blunder Analysis), let analyze_single_run use all available CPU cores for decision-level parallelization
            thread_runs[0] = analyze_single_run(sim, 1, base_seed + 7, rollouts_per_action, mode, adaptive_rollouts, report_progress, 0);
        } else {
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
            for (size_t i = 0; i < report.total_runs; ++i) {
                uint64_t run_seed = base_seed + i * 1337 + 7;
                thread_runs[i] = analyze_single_run(sim, i + 1, run_seed, rollouts_per_action, mode, adaptive_rollouts);
                float prog = 0.05f + 0.90f * (static_cast<float>(i + 1) / static_cast<float>(report.total_runs));
                std::string status = "Episode Trace #" + std::to_string(i + 1) + " / " + std::to_string(report.total_runs) + "...";
                report_progress(prog, status);
            }
#else
            unsigned int hw_threads = std::max(1u, std::thread::hardware_concurrency());
#if defined(__EMSCRIPTEN__)
            size_t avail_threads = (hw_threads > 1) ? (hw_threads - 1) : 1;
#else
            size_t avail_threads = hw_threads;
#endif
            size_t num_threads = std::min(avail_threads, report.total_runs);

            std::vector<std::thread> workers;
            std::atomic<size_t> completed_runs{0};

            size_t runs_per_thread = report.total_runs / num_threads;
            size_t rem_runs = report.total_runs % num_threads;

            for (size_t t = 0; t < num_threads; ++t) {
                size_t start_idx = t * runs_per_thread + std::min(t, rem_runs);
                size_t count = runs_per_thread + (t < rem_runs ? 1 : 0);

                workers.emplace_back([&, t, start_idx, count, mode, rollouts_per_action, adaptive_rollouts]() {
                    for (size_t i = 0; i < count; ++i) {
                        size_t run_idx = start_idx + i;
                        uint64_t run_seed = base_seed + run_idx * 1337 + 7;
                        thread_runs[run_idx] = analyze_single_run(sim, run_idx + 1, run_seed, rollouts_per_action, mode, adaptive_rollouts, nullptr, 1);
                        
                        size_t done = completed_runs.fetch_add(1) + 1;
                        float prog = 0.05f + 0.90f * (static_cast<float>(done) / static_cast<float>(report.total_runs));
                        std::string status = "Episode Trace #" + std::to_string(done) + " / " + std::to_string(report.total_runs) + "...";
                        report_progress(prog, status);
                    }
                });
            }

            for (auto& w : workers) {
                if (w.joinable()) w.join();
            }
#endif
        }

        report.runs = std::move(thread_runs);

        // Compute aggregate metrics
        double total_apl_dps = 0.0;
        double total_mcts_dps = 0.0;
        size_t total_decisions = 0;
        size_t total_divergences = 0;

        struct MismatchAgg {
            PriorityAction apl_act;
            std::string apl_name;
            PriorityAction mcts_act;
            std::string mcts_name;
            size_t count = 0;
            double sum_dps_loss = 0.0;
            double sum_confidence = 0.0;
            std::string sample_rationale;
        };
        std::unordered_map<std::string, MismatchAgg> mismatch_map;

        for (const auto& r : report.runs) {
            total_apl_dps += r.apl_dps;
            total_mcts_dps += r.mcts_dps;
            total_decisions += r.total_decisions;
            total_divergences += r.divergence_count;

            for (const auto& ev : r.events) {
                std::string key = ev.apl_action_name + " -> " + ev.mcts_action_name;
                auto& item = mismatch_map[key];
                item.apl_act = ev.apl_action;
                item.apl_name = ev.apl_action_name;
                item.mcts_act = ev.mcts_action;
                item.mcts_name = ev.mcts_action_name;
                item.count++;
                item.sum_dps_loss += ev.delta_dps;
                item.sum_confidence += ev.confidence_pct;
                if (item.sample_rationale.empty()) {
                    item.sample_rationale = ev.rationale;
                }
            }
        }

        report.avg_apl_dps = total_apl_dps / static_cast<double>(report.total_runs);
        report.avg_mcts_dps = total_mcts_dps / static_cast<double>(report.total_runs);
        report.avg_dps_loss = std::max(0.0, report.avg_mcts_dps - report.avg_apl_dps);
        report.avg_dps_loss_pct = (report.avg_dps_loss / std::max(1.0, report.avg_apl_dps)) * 100.0;

        report.total_decisions_evaluated = total_decisions;
        report.total_divergences = total_divergences;

        if (total_decisions > 0) {
            size_t matching = (total_decisions > total_divergences) ? (total_decisions - total_divergences) : 0;
            report.overall_agreement_pct = (static_cast<double>(matching) / static_cast<double>(total_decisions)) * 100.0;
        } else {
            report.overall_agreement_pct = 100.0;
        }

        // Build top mismatches list
        for (const auto& [_, agg] : mismatch_map) {
            APLRuleMismatchStat stat;
            stat.apl_action = agg.apl_act;
            stat.apl_action_name = agg.apl_name;
            stat.preferred_mcts_action = agg.mcts_act;
            stat.preferred_mcts_action_name = agg.mcts_name;
            stat.occurrences = agg.count;
            stat.total_dps_loss = agg.sum_dps_loss;
            stat.avg_dps_loss = agg.sum_dps_loss / static_cast<double>(std::max(size_t(1), agg.count));
            stat.avg_confidence_pct = agg.sum_confidence / static_cast<double>(std::max(size_t(1), agg.count));
            stat.primary_cause = agg.sample_rationale;
            report.top_mismatches.push_back(stat);
        }

        std::sort(report.top_mismatches.begin(), report.top_mismatches.end(), [](const auto& a, const auto& b) {
            if (a.occurrences != b.occurrences) return a.occurrences > b.occurrences;
            return a.total_dps_loss > b.total_dps_loss;
        });

        report.is_valid = true;
        report_progress(1.0f, "Analysis Complete!");
        return report;
    }

    static APLAnalysisReport run_blunder_analysis(
        const WarlockSimulator& sim,
        size_t num_runs = 1,
        size_t rollouts_per_action = 512,
        std::function<void(float progress, const std::string& status)> progress_cb = nullptr,
        uint64_t base_seed = 42,
        bool adaptive_rollouts = true)
    {
        return run_analysis(sim, num_runs, rollouts_per_action, progress_cb, base_seed, AnalysisMode::BLUNDERS_ONLY, adaptive_rollouts);
    }

    static APLAnalysisReport run_full_mcts_analysis(
        const WarlockSimulator& sim,
        size_t num_runs = 3,
        size_t rollouts_per_action = 256,
        std::function<void(float progress, const std::string& status)> progress_cb = nullptr,
        uint64_t base_seed = 42,
        bool adaptive_rollouts = true)
    {
        return run_analysis(sim, num_runs, rollouts_per_action, progress_cb, base_seed, AnalysisMode::FULL_MCTS_ONLY, adaptive_rollouts);
    }
};

} // namespace warlock
