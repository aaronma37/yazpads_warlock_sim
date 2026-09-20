#pragma once
#include <vector>
#include <array>
#include <thread>
#include <atomic>
#include <functional>
#include "priest_sim.hpp"
#include "src/sim/common/spell_types.hpp"

namespace priest {

struct HistogramBin {
    double min_dps = 0.0;
    double max_dps = 0.0;
    int count = 0;
};

struct BatchSimResult {
    int total_iterations = 0;
    double total_sim_time_seconds = 0.0;
    double iterations_per_second = 0.0;

    double mean_dps = 0.0;
    double min_dps = 0.0;
    double max_dps = 0.0;
    double std_dev_dps = 0.0;
    double p50_dps = 0.0;
    double p5_dps = 0.0;
    double p95_dps = 0.0;

    double mean_sw_weaving_procs = 0.0;
    double mean_mana_spent = 0.0;
    double mean_mana_gained = 0.0;

    // Damage Breakdown
    double pct_sw_pain = 0.0;
    double pct_mind_flay = 0.0;
    double pct_mind_blast = 0.0;
    double pct_sw_death = 0.0;
    double pct_devouring_plague = 0.0;
    double pct_smite = 0.0;
    double pct_holy_fire = 0.0;

    std::array<sim::BatchSpellStats, static_cast<size_t>(SpellID::COUNT)> spell_stats{};
    std::vector<HistogramBin> histogram;
    SimResult sample_timeline;
};

class ParallelSimRunner {
public:
    static BatchSimResult run_batch(
        const PriestSimulator& base_sim,
        int iterations = 10000,
        int num_threads = 0,
        std::function<void(float progress)> progress_callback = nullptr,
        uint64_t base_seed = 0
    );
};

} // namespace priest
