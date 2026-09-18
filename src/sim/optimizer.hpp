#pragma once
#include <vector>
#include <string>
#include <functional>
#include "warlock_sim.hpp"
#include "parallel_runner.hpp"

namespace warlock {

struct StatWeights {
    double dps_per_sp = 0.0;    // DPS gained per 1 Spell Power
    double dps_per_hit = 0.0;   // DPS gained per 1% Spell Hit
    double dps_per_crit = 0.0;  // DPS gained per 1% Spell Crit
    double dps_per_haste = 0.0; // DPS gained per 1% Spell Haste
    double dps_per_int = 0.0;   // DPS gained per 1 Intellect
    bool valid = false;
};

struct CandidateResult {
    int rank = 0;
    std::string name;
    std::string category; // "Talents", "Gear", "Policy", "Snapshotting"
    Race race = Race::HUMAN;
    double mean_dps = 0.0;
    double std_dev_dps = 0.0;
    double min_dps = 0.0;
    double max_dps = 0.0;
    double isb_uptime = 0.0;

    // Specific settings tested
    Talents talents;
    GearLoadout gear;
    BuffConfig buffs;
    PolicyConfig policy;
    MechanicsConfig mechanics;
    bool use_raw_stats = false;
    Stats raw_stats;

    // Full detailed batch simulation results (damage breakdown, crits, percentiles)
    BatchSimResult batch;

    // Local Stat Sensitivity / Stat Weights (DPS per stat change)
    StatWeights stat_weights;
};

class Optimizer {
public:
    // Helper to calculate local stat sensitivities for a candidate simulator configuration
    static StatWeights calculate_candidate_stat_weights(
        const WarlockSimulator& candidate_sim,
        int iterations_per_sample = 1500
    );

    // Compares standard talent specs
    static std::vector<CandidateResult> optimize_talents(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 2000,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr,
        bool compare_all_races = false,
        bool calculate_stat_weights = false
    );

    // Dynamic combinatorial brute-force exploration across talent configurations
    static std::vector<CandidateResult> explore_combinatorial_talents(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 1500,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr,
        bool compare_all_races = false,
        bool calculate_stat_weights = false
    );

    // Genetic Algorithm with Learned Linear/Ridge Regression Surrogate Guidance
    static std::vector<CandidateResult> optimize_genetic_ai(
        const WarlockSimulator& base_sim,
        int population_size = 50,
        int generations = 20,
        int screening_sims = 400,
        int final_sims = 2500,
        bool seed_with_presets = true,
        bool optimize_race = true,
        double mutation_rate = 0.45,
        double initial_exploration = 0.50,
        double min_exploration = 0.15,
        const std::vector<int>& required_talents = {},
        int forced_race = -1,
        int forced_rotation = -1,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr,
        std::function<void(const std::vector<CandidateResult>& current_elites)> generation_callback = nullptr,
        const std::atomic<bool>* should_stop = nullptr
    );

    // Compares gear loadouts & trinkets
    static std::vector<CandidateResult> optimize_gear(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 2000,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr
    );

    // Compares consumable tiers (Naked vs Basic vs Full Raid vs World Buffs)
    static std::vector<CandidateResult> compare_consumable_tiers(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 2000,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr
    );

    // Compares value-based stats (Stat Weight Sensitivity / Equivalence Points)
    static std::vector<CandidateResult> compare_stat_values(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 2500,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr
    );

    // Compares policy / APL configurations (Life tap thresholds, Curses, DoTs)
    static std::vector<CandidateResult> optimize_policy(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 2000,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr
    );

    // Evaluates the DPS delta of Snapshotting ON vs OFF across various specs
    static std::vector<CandidateResult> evaluate_snapshotting_impact(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 2500,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr
    );

    // Perturbations engine: explores local talent point redistributions and rotational APL mutations
    static std::vector<CandidateResult> perturb_preset(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 2000,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr
    );
};

} // namespace warlock
