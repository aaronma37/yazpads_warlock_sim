#pragma once
#include <vector>
#include <string>
#include <functional>
#include "warlock_sim.hpp"
#include "parallel_runner.hpp"

namespace warlock {

struct CandidateResult {
    int rank = 0;
    std::string name;
    std::string category; // "Talents", "Gear", "Policy", "Snapshotting"
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
};

class Optimizer {
public:
    // Compares standard talent specs
    static std::vector<CandidateResult> optimize_talents(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 2000,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr
    );

    // Dynamic combinatorial brute-force exploration across talent configurations
    static std::vector<CandidateResult> explore_combinatorial_talents(
        const WarlockSimulator& base_sim,
        int iterations_per_candidate = 1500,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr
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
