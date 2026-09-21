#pragma once
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include "priest_sim.hpp"
#include "parallel_runner.hpp"

namespace priest {

struct CandidateResult {
    int rank = 0;
    std::string name;
    std::string category = "Talents";
    sim::Race race = sim::Race::HUMAN;
    double mean_dps = 0.0;
    double std_dev_dps = 0.0;
    double min_dps = 0.0;
    double max_dps = 0.0;
    double sw_procs = 0.0;
    double mean_mana_spent = 0.0;
    double mean_mana_gained = 0.0;

    Talents talents;
    sim::GearLoadout gear;
    sim::BuffConfig buffs;
    PolicyConfig policy;
    MechanicsConfig mechanics;
    bool use_raw_stats = true;
    sim::Stats raw_stats;

    BatchSimResult batch;
};

class Optimizer {
public:
    // Compares standard Priest talent specs (and optionally across all playable Priest races)
    static std::vector<CandidateResult> optimize_talents(
        const PriestSimulator& base_sim,
        int iterations_per_candidate = 2000,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr,
        bool compare_all_races = false
    );

    // Genetic AI search across the 53-node Priest talent graph
    static std::vector<CandidateResult> optimize_genetic_ai(
        const PriestSimulator& base_sim,
        int population_size = 50,
        int generations = 400,
        int screening_sims = 400,
        int final_sims = 2500,
        bool seed_with_presets = false,
        bool optimize_race = true,
        double mutation_rate = 0.45,
        double initial_explore = 0.50,
        double min_explore = 0.15,
        const std::vector<int>& req_talents = {},
        int forced_race = -1,
        int forced_rotation = -1,
        int thread_count = 0,
        std::function<void(float progress, const std::string& current_name)> callback = nullptr,
        std::function<void(const std::vector<CandidateResult>& current_elites)> generation_callback = nullptr,
        const std::atomic<bool>* should_stop = nullptr
    );
};

} // namespace priest
