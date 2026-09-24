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
#include <unordered_map>

namespace warlock {

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
};

// A single decision-point discrepancy where APL differed from the optimal MCTS choice
struct APLDivergenceEvent {
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

    double delta_dps = 0.0;             // mcts_expected_dps - apl_expected_dps
    double confidence_pct = 0.0;        // Statistical confidence that MCTS action is strictly superior
    double z_score = 0.0;

    std::vector<ActionMCTSEval> candidate_evals; // Full distribution of all evaluated branches
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
            {PriorityAction::DEMONIC_BRAND_SEARING_PAIN, SpellID::SEARING_PAIN},
            {PriorityAction::INCINERATE_FILLER, SpellID::INCINERATE},
            {PriorityAction::SEARING_PAIN_FILLER, SpellID::SEARING_PAIN},
            {PriorityAction::DRAIN_SOUL_FILLER, SpellID::DRAIN_SOUL},
            {PriorityAction::DRAIN_LIFE_FILLER, SpellID::DRAIN_LIFE},
            {PriorityAction::SHADOW_BOLT_FILLER, SpellID::SHADOW_BOLT}
        };
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
            case PriorityAction::CURSE_OF_AGONY: return "Curse of Agony Upkeep";
            case PriorityAction::CURSE_OF_DOOM: return "Curse of Doom (>=60s remaining)";
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

    static bool is_action_legal(PriorityAction action, const sim::SimObservation& obs, const Talents& talents) {
        switch (action) {
            case PriorityAction::LIFE_TAP:
                return obs.player_hp_pct > 0.15f;
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
                return talents.aff.improved_drains > 0 || obs.player_hp_pct < 0.35f;
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
            ss << "Fight duration remaining (" << obs.time_remaining_sec << "s) >= 65s. Curse of Doom provides massive DPE and saves GCD casting budget over Curse of Agony (+"
               << delta_dps << " DPS).";
            return ss.str();
        }

        if (mcts_act == PriorityAction::CURSE_OF_AGONY) {
            ss << "Curse of Agony expiring (" << obs.dot_agony_rem_sec << "s left). MCTS maintains curse uptime (+" << delta_dps << " DPS).";
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
    static std::vector<ActionMCTSEval> evaluate_candidate_branches_crn(
        const WarlockSimulator& base_sim,
        const std::vector<PriorityAction>& prefix,
        const std::vector<PriorityAction>& candidate_actions,
        size_t rollouts_count = 32,
        uint64_t base_seed = 42)
    {
        size_t K = candidate_actions.size();
        std::vector<ActionMCTSEval> results(K);

        for (size_t k = 0; k < K; ++k) {
            results[k].action = candidate_actions[k];
            results[k].name = get_action_name(candidate_actions[k]);
            results[k].rollout_count = std::max(size_t(2), rollouts_count);
        }

        std::vector<std::vector<double>> sample_dps(K, std::vector<double>(rollouts_count, 0.0));

        // Common Random Number (CRN) loop: Every candidate action is evaluated on the IDENTICAL random stream
        for (size_t r = 0; r < rollouts_count; ++r) {
            uint64_t rollout_seed = base_seed + r * 7919 + 17;

            for (size_t k = 0; k < K; ++k) {
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
                sample_dps[k][r] = res.dps;
            }
        }

        // Calculate sample statistics for each branch
        double n = static_cast<double>(rollouts_count);
        for (size_t k = 0; k < K; ++k) {
            double sum = 0.0;
            double sum_sq = 0.0;
            for (size_t r = 0; r < rollouts_count; ++r) {
                sum += sample_dps[k][r];
                sum_sq += sample_dps[k][r] * sample_dps[k][r];
            }
            results[k].mean_dps = sum / n;
            double var = std::max(0.0, (sum_sq - (sum * sum / n)) / (n - 1.0));
            results[k].stddev_dps = std::sqrt(var);
            results[k].std_error = results[k].stddev_dps / std::sqrt(n);
            results[k].ci_lower_95 = results[k].mean_dps - 1.96 * results[k].std_error;
            results[k].ci_upper_95 = results[k].mean_dps + 1.96 * results[k].std_error;
        }

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

    // Evaluates a single trace run comparing APL decisions against true MCTS rollouts
    static APLAnalysisRun analyze_single_run(
        const WarlockSimulator& base_sim,
        size_t run_idx,
        uint64_t seed,
        size_t rollouts_per_action = 32)
    {
        APLAnalysisRun run;
        run.run_index = run_idx;
        run.seed = seed;
        run.fight_duration = base_sim.fight_duration;

        // 1. Run full APL baseline episode with timeline and observation logging
        FastRNG rng(seed);
        WarlockSimulator apl_sim = base_sim;
        apl_sim.randomize_duration = false;
        apl_sim.record_timeline = true;
        apl_sim.record_viper_samples = true;
        apl_sim.viper_dataset.clear();
        apl_sim.use_oracle_execution_policy = false;
        apl_sim.forced_action_prefix.clear();

        SimResult apl_res = apl_sim.run_single_simulation(rng);
        run.apl_dps = apl_res.dps;
        run.fight_duration = apl_res.duration;
        run.total_decisions = apl_sim.viper_dataset.samples.size();

        auto candidate_actions = get_candidate_actions();

        std::vector<PriorityAction> mcts_optimal_action_sequence;
        mcts_optimal_action_sequence.reserve(std::max(size_t(32), run.total_decisions));

        // 2. Closed-Loop Live State MCTS Step Search
        // At each step, simulate up to the current decision point using the cumulative MCTS prefix,
        // inspect the real live MCTS observation, evaluate legal candidate actions, and pick the best action.
        size_t decision_step = 0;
        const size_t max_allowed_decisions = 250; // Guard against infinite loop

        while (decision_step < max_allowed_decisions) {
            // Run a simulation on the base seed with the cumulative MCTS prefix to extract the live state at step `decision_step`
            FastRNG live_rng(seed);
            WarlockSimulator live_sim = base_sim;
            live_sim.randomize_duration = false;
            live_sim.record_timeline = false;
            live_sim.record_viper_samples = true;
            live_sim.viper_dataset.clear();
            live_sim.use_oracle_execution_policy = false;
            live_sim.forced_action_prefix = mcts_optimal_action_sequence;

            SimResult step_probe_res = live_sim.run_single_simulation(live_rng);

            // If the fight has finished (no more decision points generated past prefix), we are done!
            if (step_probe_res.action_history.size() <= mcts_optimal_action_sequence.size()) {
                break;
            }

            // Extract the TRUE LIVE observation and the default APL action at this exact decision point
            const auto& live_sample = live_sim.viper_dataset.samples[decision_step];
            const sim::SimObservation& live_obs = live_sample.state;
            PriorityAction default_apl_act = (decision_step < step_probe_res.action_history.size())
                                             ? step_probe_res.action_history[decision_step]
                                             : static_cast<PriorityAction>(live_sample.oracle_action);

            // Filter legal candidate actions for the REAL LIVE state observation & active talents
            std::vector<PriorityAction> legal_candidates;
            for (const auto& [act, _] : candidate_actions) {
                if (act == PriorityAction::RACIAL_EUREKA ||
                    act == PriorityAction::RACIAL_BLOOD_FURY ||
                    act == PriorityAction::RACIAL_BERSERKING ||
                    act == PriorityAction::AMPLIFY_CURSE ||
                    act == PriorityAction::BANE_OF_HAVOC) {
                    continue; // Off-GCD
                }
                if (is_action_legal(act, live_obs, base_sim.talents)) {
                    legal_candidates.push_back(act);
                }
            }

            // Always ensure the default APL action is evaluated in the candidate set
            if (std::find(legal_candidates.begin(), legal_candidates.end(), default_apl_act) == legal_candidates.end()) {
                legal_candidates.push_back(default_apl_act);
            }

            // Execute Common Random Numbers (CRN) Monte Carlo Forward Rollouts from current cumulative prefix
            std::vector<ActionMCTSEval> evals = evaluate_candidate_branches_crn(
                base_sim,
                mcts_optimal_action_sequence,
                legal_candidates,
                rollouts_per_action,
                seed + decision_step * 65537 + 13
            );

            ActionMCTSEval best_eval;
            best_eval.mean_dps = -1e9;
            ActionMCTSEval apl_eval;
            apl_eval.mean_dps = -1e9;

            for (const auto& e : evals) {
                if (e.mean_dps > best_eval.mean_dps) {
                    best_eval = e;
                }
                if (e.action == default_apl_act) {
                    apl_eval = e;
                }
            }

            if (apl_eval.mean_dps < -1e8) {
                apl_eval = best_eval; // Fallback safety
            }

            // Calculate delta DPS, Standard Error of difference, Z-score, and Confidence
            double delta_dps = std::max(0.0, best_eval.mean_dps - apl_eval.mean_dps);
            double se_diff = std::sqrt(best_eval.std_error * best_eval.std_error + apl_eval.std_error * apl_eval.std_error);
            double z_score = (se_diff > 1e-5) ? (delta_dps / se_diff) : 0.0;
            double confidence = compute_statistical_confidence(delta_dps, se_diff);

            PriorityAction chosen_mcts_action = default_apl_act;

            // If MCTS found a statistically superior action over APL with measurable confidence
            if (best_eval.action != default_apl_act && delta_dps >= 1.5 && confidence >= 70.0) {
                APLDivergenceEvent ev;
                ev.timestamp = live_obs.fight_progress_pct * base_sim.fight_duration;
                ev.decision_step = decision_step;
                ev.state = live_obs;
                ev.apl_action = default_apl_act;
                ev.apl_action_name = get_action_name(default_apl_act);
                ev.apl_expected_dps = apl_eval.mean_dps;
                ev.apl_std_error = apl_eval.std_error;

                ev.mcts_action = best_eval.action;
                ev.mcts_action_name = get_action_name(best_eval.action);
                ev.mcts_expected_dps = best_eval.mean_dps;
                ev.mcts_std_error = best_eval.std_error;

                ev.delta_dps = delta_dps;
                ev.confidence_pct = confidence;
                ev.z_score = z_score;
                ev.candidate_evals = std::move(evals);

                ev.rationale = generate_rationale(default_apl_act, best_eval.action, live_obs, base_sim.talents, delta_dps, confidence);
                run.events.push_back(std::move(ev));

                chosen_mcts_action = best_eval.action;
            }

            mcts_optimal_action_sequence.push_back(chosen_mcts_action);
            decision_step++;
        }

        // 3. Run full MCTS optimal policy trajectory on identical seed with full timeline recording
        FastRNG mcts_rng(seed);
        WarlockSimulator mcts_sim = base_sim;
        mcts_sim.randomize_duration = false;
        mcts_sim.record_timeline = true;
        mcts_sim.record_viper_samples = false;
        mcts_sim.use_oracle_execution_policy = false;
        mcts_sim.forced_action_prefix = mcts_optimal_action_sequence;

        SimResult mcts_res = mcts_sim.run_single_simulation(mcts_rng);
        run.mcts_dps = mcts_res.dps;
        run.dps_difference = run.mcts_dps - run.apl_dps;

        run.divergence_count = run.events.size();
        if (run.total_decisions > 0) {
            size_t matching = (run.total_decisions > run.divergence_count) ? (run.total_decisions - run.divergence_count) : 0;
            run.agreement_rate_pct = (static_cast<double>(matching) / static_cast<double>(run.total_decisions)) * 100.0;
        } else {
            run.agreement_rate_pct = 100.0;
        }

        // 4. Construct comprehensive timeline time series and spell blocks
        build_run_timeline_data(run, apl_res, mcts_res, run.fight_duration);

        return run;
    }

    // Runs a batch APL vs MCTS analysis across multiple runs with multi-threading
    static APLAnalysisReport run_analysis(
        const WarlockSimulator& sim,
        size_t num_runs = 5,
        size_t rollouts_per_action = 32,
        std::function<void(float progress, const std::string& status)> progress_cb = nullptr,
        uint64_t base_seed = 42)
    {
        APLAnalysisReport report;
        report.total_runs = std::max(size_t(1), num_runs);

        auto report_progress = [&](float p, const std::string& msg) {
            if (progress_cb) progress_cb(p, msg);
        };

        report_progress(0.05f, "Launching True MCTS Forward Rollout Workers with CRN & Confidence Bounds...");

        unsigned int hw_threads = std::max(1u, std::thread::hardware_concurrency());
        size_t num_threads = std::min(static_cast<size_t>(hw_threads), report.total_runs);

        std::vector<APLAnalysisRun> thread_runs(report.total_runs);
        std::vector<std::thread> workers;
        std::atomic<size_t> completed_runs{0};

        size_t runs_per_thread = report.total_runs / num_threads;
        size_t rem_runs = report.total_runs % num_threads;

        for (size_t t = 0; t < num_threads; ++t) {
            size_t start_idx = t * runs_per_thread + std::min(t, rem_runs);
            size_t count = runs_per_thread + (t < rem_runs ? 1 : 0);

            workers.emplace_back([&, t, start_idx, count]() {
                for (size_t i = 0; i < count; ++i) {
                    size_t run_idx = start_idx + i;
                    uint64_t run_seed = base_seed + run_idx * 1337 + 7;
                    thread_runs[run_idx] = analyze_single_run(sim, run_idx + 1, run_seed, rollouts_per_action);
                    
                    size_t done = completed_runs.fetch_add(1) + 1;
                    float prog = 0.05f + 0.90f * (static_cast<float>(done) / static_cast<float>(report.total_runs));
                    std::string status = "MCTS Rollout Trace #" + std::to_string(done) + " / " + std::to_string(report.total_runs) + "...";
                    report_progress(prog, status);
                }
            });
        }

        for (auto& w : workers) {
            if (w.joinable()) w.join();
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
        report_progress(1.0f, "True MCTS Policy Analysis Complete!");
        return report;
    }
};

} // namespace warlock
