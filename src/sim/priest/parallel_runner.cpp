#include "parallel_runner.hpp"
#include <numeric>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace priest {

BatchSimResult ParallelSimRunner::run_batch(
    const PriestSimulator& base_sim,
    int iterations,
    int num_threads,
    std::function<void(float progress)> progress_callback,
    uint64_t base_seed
) {
    if (iterations <= 0) iterations = 1;
    if (num_threads <= 0) {
        num_threads = static_cast<int>(std::thread::hardware_concurrency());
        if (num_threads <= 0) num_threads = 4;
    }
    if (num_threads > iterations) num_threads = iterations;

    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t initial_seed = (base_seed != 0) ? base_seed : (0x1337BEEFULL + static_cast<uint64_t>(start_time.time_since_epoch().count()));

    struct ThreadOutput {
        std::vector<double> dps_list;
        double sum_dps = 0.0;
        double sum_dps_sq = 0.0;
        double sum_mana_spent = 0.0;
        double sum_mana_gained = 0.0;
        int sum_sw_procs = 0;

        double sum_dmg_swp = 0.0;
        double sum_dmg_mf = 0.0;
        double sum_dmg_mb = 0.0;
        double sum_dmg_swd = 0.0;
        double sum_dmg_dp = 0.0;
        double sum_dmg_smite = 0.0;
        double sum_dmg_hf = 0.0;
        double sum_dmg_total = 0.0;

        std::array<sim::SpellCombatStats, static_cast<size_t>(SpellID::COUNT)> sum_spell{};
    };

    std::vector<ThreadOutput> thread_outputs(num_threads);
    std::atomic<int> completed_iterations(0);
    std::vector<std::thread> workers;
    workers.reserve(num_threads);

    int iters_per_thread = iterations / num_threads;
    int remainder = iterations % num_threads;

    for (int t = 0; t < num_threads; ++t) {
        int count = iters_per_thread + (t < remainder ? 1 : 0);
        uint64_t seed = initial_seed + t * 99991ULL;

        workers.emplace_back([&, t, count, seed]() {
            sim::FastRNG rng(seed);
            PriestSimulator local_sim = base_sim;
            local_sim.record_timeline = false;
            auto& out = thread_outputs[t];
            out.dps_list.reserve(count);

            for (int i = 0; i < count; ++i) {
                SimResult res = local_sim.run_single_simulation(rng);
                out.dps_list.push_back(res.dps);
                out.sum_dps += res.dps;
                out.sum_dps_sq += res.dps * res.dps;
                out.sum_mana_spent += res.mana_spent;
                out.sum_mana_gained += res.mana_gained;
                out.sum_sw_procs += res.shadow_weaving_procs;

                out.sum_dmg_swp += res.dmg_sw_pain;
                out.sum_dmg_mf += res.dmg_mind_flay;
                out.sum_dmg_mb += res.dmg_mind_blast;
                out.sum_dmg_swd += res.dmg_sw_death;
                out.sum_dmg_dp += res.dmg_devouring_plague;
                out.sum_dmg_smite += res.dmg_smite;
                out.sum_dmg_hf += res.dmg_holy_fire;
                out.sum_dmg_total += res.total_damage;

                for (size_t s = 0; s < static_cast<size_t>(SpellID::COUNT); ++s) {
                    out.sum_spell[s].casts += res.spell_stats[s].casts;
                    out.sum_spell[s].hits += res.spell_stats[s].hits;
                    out.sum_spell[s].crits += res.spell_stats[s].crits;
                    out.sum_spell[s].misses += res.spell_stats[s].misses;
                    out.sum_spell[s].damage += res.spell_stats[s].damage;
                }

                int prev = completed_iterations.fetch_add(1, std::memory_order_relaxed);
                if (progress_callback && (prev % 500 == 0 || prev == iterations - 1)) {
                    progress_callback(static_cast<float>(prev + 1) / static_cast<float>(iterations));
                }
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    BatchSimResult batch;
    batch.total_iterations = iterations;
    batch.total_sim_time_seconds = elapsed.count();
    batch.iterations_per_second = (elapsed.count() > 0.0) ? (iterations / elapsed.count()) : 0.0;

    std::vector<double> all_dps;
    all_dps.reserve(iterations);
    double grand_sum_dps = 0.0;
    double grand_sum_dps_sq = 0.0;
    double grand_dmg_total = 0.0;
    double grand_dmg_swp = 0.0;
    double grand_dmg_mf = 0.0;
    double grand_dmg_mb = 0.0;
    double grand_dmg_swd = 0.0;
    double grand_dmg_dp = 0.0;
    double grand_dmg_smite = 0.0;
    double grand_dmg_hf = 0.0;

    for (const auto& out : thread_outputs) {
        all_dps.insert(all_dps.end(), out.dps_list.begin(), out.dps_list.end());
        grand_sum_dps += out.sum_dps;
        grand_sum_dps_sq += out.sum_dps_sq;
        grand_dmg_total += out.sum_dmg_total;
        grand_dmg_swp += out.sum_dmg_swp;
        grand_dmg_mf += out.sum_dmg_mf;
        grand_dmg_mb += out.sum_dmg_mb;
        grand_dmg_swd += out.sum_dmg_swd;
        grand_dmg_dp += out.sum_dmg_dp;
        grand_dmg_smite += out.sum_dmg_smite;
        grand_dmg_hf += out.sum_dmg_hf;
        batch.mean_mana_spent += out.sum_mana_spent;
        batch.mean_mana_gained += out.sum_mana_gained;
        batch.mean_sw_weaving_procs += out.sum_sw_procs;

        for (size_t s = 0; s < static_cast<size_t>(SpellID::COUNT); ++s) {
            batch.spell_stats[s].mean_casts += out.sum_spell[s].casts;
            batch.spell_stats[s].mean_hits += out.sum_spell[s].hits;
            batch.spell_stats[s].mean_crits += out.sum_spell[s].crits;
            batch.spell_stats[s].mean_misses += out.sum_spell[s].misses;
            batch.spell_stats[s].mean_damage += out.sum_spell[s].damage;
        }
    }

    batch.mean_dps = grand_sum_dps / iterations;
    double variance = (grand_sum_dps_sq / iterations) - (batch.mean_dps * batch.mean_dps);
    batch.std_dev_dps = (variance > 0.0) ? std::sqrt(variance) : 0.0;
    batch.mean_mana_spent /= iterations;
    batch.mean_mana_gained /= iterations;
    batch.mean_sw_weaving_procs /= iterations;

    for (size_t s = 0; s < static_cast<size_t>(SpellID::COUNT); ++s) {
        batch.spell_stats[s].mean_casts /= iterations;
        batch.spell_stats[s].mean_hits /= iterations;
        batch.spell_stats[s].mean_crits /= iterations;
        batch.spell_stats[s].mean_misses /= iterations;
        batch.spell_stats[s].mean_damage /= iterations;
    }

    if (grand_dmg_total > 0.0) {
        batch.pct_sw_pain = (grand_dmg_swp / grand_dmg_total) * 100.0;
        batch.pct_mind_flay = (grand_dmg_mf / grand_dmg_total) * 100.0;
        batch.pct_mind_blast = (grand_dmg_mb / grand_dmg_total) * 100.0;
        batch.pct_sw_death = (grand_dmg_swd / grand_dmg_total) * 100.0;
        batch.pct_devouring_plague = (grand_dmg_dp / grand_dmg_total) * 100.0;
        batch.pct_smite = (grand_dmg_smite / grand_dmg_total) * 100.0;
        batch.pct_holy_fire = (grand_dmg_hf / grand_dmg_total) * 100.0;
    }

    std::sort(all_dps.begin(), all_dps.end());
    batch.min_dps = all_dps.front();
    batch.max_dps = all_dps.back();
    batch.p50_dps = all_dps[static_cast<size_t>(iterations * 0.50)];
    batch.p5_dps = all_dps[static_cast<size_t>(iterations * 0.05)];
    batch.p95_dps = all_dps[static_cast<size_t>(iterations * 0.95)];

    // 40-bin Histogram
    int num_bins = 40;
    double bin_width = (batch.max_dps - batch.min_dps) / num_bins;
    if (bin_width > 0.0) {
        batch.histogram.resize(num_bins);
        for (int i = 0; i < num_bins; ++i) {
            batch.histogram[i].min_dps = batch.min_dps + i * bin_width;
            batch.histogram[i].max_dps = batch.min_dps + (i + 1) * bin_width;
            batch.histogram[i].count = 0;
        }
        for (double d : all_dps) {
            int bin = std::min(num_bins - 1, static_cast<int>((d - batch.min_dps) / bin_width));
            if (bin >= 0 && bin < num_bins) batch.histogram[bin].count++;
        }
    }

    // Run single detailed sample timeline
    PriestSimulator sample_sim = base_sim;
    sample_sim.record_timeline = true;
    sim::FastRNG sample_rng(initial_seed + 7777);
    batch.sample_timeline = sample_sim.run_single_simulation(sample_rng);

    return batch;
}

} // namespace priest
