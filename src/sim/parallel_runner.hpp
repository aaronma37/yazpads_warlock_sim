#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include "warlock_sim.hpp"

namespace warlock {

struct HistogramBin {
    double min_dps = 0.0;
    double max_dps = 0.0;
    int count = 0;
};

struct BatchSimResult {
    int total_iterations = 0;
    double total_sim_time_seconds = 0.0;
    double iterations_per_second = 0.0;

    // DPS Summary
    double mean_dps = 0.0;
    double min_dps = 0.0;
    double max_dps = 0.0;
    double std_dev_dps = 0.0;

    double p1_dps = 0.0;
    double p5_dps = 0.0;
    double p25_dps = 0.0;
    double p50_dps = 0.0; // Median
    double p75_dps = 0.0;
    double p95_dps = 0.0;
    double p99_dps = 0.0;

    // Combat Stats
    double mean_isb_uptime = 0.0;
    double mean_shadow_bolts = 0.0;
    double mean_crits = 0.0;
    double crit_percent = 0.0;
    double miss_percent = 0.0;
    double mean_life_taps = 0.0;
    double mean_mana_spent = 0.0;

    // Damage Breakdown
    double pct_shadow_bolt = 0.0;
    double pct_corruption = 0.0;
    double pct_curse = 0.0; // Total Curse / Bane
    double pct_agony = 0.0;
    double pct_doom = 0.0;
    double pct_siphon_life = 0.0;
    double pct_immolate = 0.0;
    double pct_shadowburn = 0.0;
    double pct_conflagrate = 0.0;
    double pct_incinerate = 0.0;
    double pct_searing_pain = 0.0;
    double pct_soul_fire = 0.0;
    double pct_drain_hope = 0.0;
    double pct_drain_life = 0.0;
    double pct_drain_soul = 0.0;
    double pct_pet = 0.0; // Total Pet
    double pct_pet_imp = 0.0;
    double pct_pet_succubus = 0.0;
    double mean_pet_dps = 0.0;

    // Distribution Histogram (40 bins)
    std::vector<HistogramBin> histogram;

    // Detailed single timeline run for sample visualization
    SimResult sample_timeline;
};

class ParallelSimRunner {
public:
    static BatchSimResult run_batch(
        const WarlockSimulator& base_sim,
        int iterations = 10000,
        int num_threads = 0,
        std::function<void(float progress)> progress_callback = nullptr
    );
};

} // namespace warlock
