#include "parallel_runner.hpp"
#include <numeric>
#include <cmath>
#include <algorithm>

namespace warlock {

BatchSimResult ParallelSimRunner::run_batch(
    const WarlockSimulator& base_sim,
    int iterations,
    int num_threads,
    std::function<void(float progress)> progress_callback
) {
    if (iterations <= 0) iterations = 1;
    if (num_threads <= 0) {
        num_threads = static_cast<int>(std::thread::hardware_concurrency());
        if (num_threads <= 0) num_threads = 4;
    }
    if (num_threads > iterations) {
        num_threads = iterations;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    struct ThreadOutput {
        std::vector<double> dps_list;
        double sum_dps = 0.0;
        double sum_dps_sq = 0.0;
        double sum_isb_uptime = 0.0;
        int sum_shadow_bolts = 0;
        int sum_crits = 0;
        int sum_direct_casts = 0;
        int sum_direct_crits = 0;
        int sum_damage_events = 0;
        int sum_damage_crits = 0;
        int sum_misses = 0;
        int sum_casts = 0;
        int sum_life_taps = 0;
        double sum_mana_spent = 0.0;

        double sum_dmg_sb = 0.0;
        double sum_dmg_corr = 0.0;
        double sum_dmg_curse = 0.0;
        double sum_dmg_agony = 0.0;
        double sum_dmg_doom = 0.0;
        double sum_dmg_siphon_life = 0.0;
        double sum_dmg_imm = 0.0;
        double sum_dmg_sb_urn = 0.0;
        double sum_dmg_conflag = 0.0;
        double sum_dmg_incin = 0.0;
        double sum_dmg_sp = 0.0;
        double sum_dmg_sf = 0.0;
        double sum_dmg_dh = 0.0;
        double sum_dmg_dl = 0.0;
        double sum_dmg_ds = 0.0;
        double sum_dmg_pet = 0.0;
        double sum_dmg_pet_imp = 0.0;
        double sum_dmg_pet_succubus = 0.0;
        double sum_dmg_total = 0.0;
    };

    std::vector<ThreadOutput> thread_outputs(num_threads);
    std::atomic<int> completed_iterations(0);
    std::vector<std::thread> workers;
    workers.reserve(num_threads);

    int iters_per_thread = iterations / num_threads;
    int remainder = iterations % num_threads;

    for (int t = 0; t < num_threads; ++t) {
        int count = iters_per_thread + (t < remainder ? 1 : 0);
        uint64_t seed = 0x85467291ULL + static_cast<uint64_t>(t * 192837465ULL) + static_cast<uint64_t>(start_time.time_since_epoch().count());

        workers.emplace_back([&base_sim, &thread_outputs, &completed_iterations, t, count, seed, iterations, progress_callback]() {
            FastRNG rng(seed);
            WarlockSimulator sim = base_sim;
            sim.record_timeline = false; // Never record timeline in worker threads for max performance

            auto& out = thread_outputs[t];
            out.dps_list.reserve(count);

            for (int i = 0; i < count; ++i) {
                SimResult res = sim.run_single_simulation(rng);

                out.dps_list.push_back(res.dps);
                out.sum_dps += res.dps;
                out.sum_dps_sq += res.dps * res.dps;
                out.sum_isb_uptime += res.isb_uptime_percent;
                out.sum_shadow_bolts += res.shadow_bolt_casts;
                out.sum_crits += res.shadow_bolt_crits;
                out.sum_direct_casts += res.direct_spell_casts;
                out.sum_direct_crits += res.direct_spell_crits;
                out.sum_damage_events += res.total_damage_events;
                out.sum_damage_crits += res.total_damage_crits;
                out.sum_misses += res.misses;
                out.sum_casts += res.total_casts;
                out.sum_life_taps += res.life_taps;
                out.sum_mana_spent += res.mana_spent;

                out.sum_dmg_sb += res.dmg_shadow_bolt;
                out.sum_dmg_corr += res.dmg_corruption;
                out.sum_dmg_curse += (res.dmg_curse + res.dmg_siphon_life);
                out.sum_dmg_agony += res.dmg_agony;
                out.sum_dmg_doom += res.dmg_doom;
                out.sum_dmg_siphon_life += res.dmg_siphon_life;
                out.sum_dmg_imm += res.dmg_immolate;
                out.sum_dmg_sb_urn += res.dmg_shadowburn;
                out.sum_dmg_conflag += res.dmg_conflagrate;
                out.sum_dmg_incin += res.dmg_incinerate;
                out.sum_dmg_sp += res.dmg_searing_pain;
                out.sum_dmg_sf += res.dmg_soul_fire;
                out.sum_dmg_dh += res.dmg_drain_hope;
                out.sum_dmg_dl += res.dmg_drain_life;
                out.sum_dmg_ds += res.dmg_drain_soul;
                out.sum_dmg_pet += res.dmg_pet;
                out.sum_dmg_pet_imp += res.dmg_pet_imp;
                out.sum_dmg_pet_succubus += res.dmg_pet_succubus;
                out.sum_dmg_total += res.total_damage;

                int done = ++completed_iterations;
                if (progress_callback && (done % 500 == 0 || done == iterations)) {
                    progress_callback(static_cast<float>(done) / static_cast<float>(iterations));
                }
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration<double>(end_time - start_time).count();

    // Aggregate results
    BatchSimResult batch;
    batch.total_iterations = iterations;
    batch.total_sim_time_seconds = duration_sec;
    batch.iterations_per_second = (duration_sec > 0.0) ? (iterations / duration_sec) : 0.0;

    std::vector<double> all_dps;
    all_dps.reserve(iterations);

    double total_dps = 0.0;
    double total_dps_sq = 0.0;
    double total_isb_uptime = 0.0;
    int total_sb = 0;
    int total_crits = 0;
    int total_direct_casts = 0;
    int total_direct_crits = 0;
    int total_damage_events = 0;
    int total_damage_crits = 0;
    int total_misses = 0;
    int total_casts = 0;
    int total_life_taps = 0;
    double total_mana_spent = 0.0;

    double total_dmg_sb = 0.0;
    double total_dmg_corr = 0.0;
    double total_dmg_curse = 0.0;
    double total_dmg_agony = 0.0;
    double total_dmg_doom = 0.0;
    double total_dmg_siphon_life = 0.0;
    double total_dmg_imm = 0.0;
    double total_dmg_sb_urn = 0.0;
    double total_dmg_conflag = 0.0;
    double total_dmg_incin = 0.0;
    double total_dmg_sp = 0.0;
    double total_dmg_sf = 0.0;
    double total_dmg_dh = 0.0;
    double total_dmg_dl = 0.0;
    double total_dmg_ds = 0.0;
    double total_dmg_pet = 0.0;
    double total_dmg_pet_imp = 0.0;
    double total_dmg_pet_succubus = 0.0;
    double total_dmg_all = 0.0;

    for (const auto& out : thread_outputs) {
        all_dps.insert(all_dps.end(), out.dps_list.begin(), out.dps_list.end());
        total_dps += out.sum_dps;
        total_dps_sq += out.sum_dps_sq;
        total_isb_uptime += out.sum_isb_uptime;
        total_sb += out.sum_shadow_bolts;
        total_crits += out.sum_crits;
        total_direct_casts += out.sum_direct_casts;
        total_direct_crits += out.sum_direct_crits;
        total_damage_events += out.sum_damage_events;
        total_damage_crits += out.sum_damage_crits;
        total_misses += out.sum_misses;
        total_casts += out.sum_casts;
        total_life_taps += out.sum_life_taps;
        total_mana_spent += out.sum_mana_spent;

        total_dmg_sb += out.sum_dmg_sb;
        total_dmg_corr += out.sum_dmg_corr;
        total_dmg_curse += out.sum_dmg_curse;
        total_dmg_agony += out.sum_dmg_agony;
        total_dmg_doom += out.sum_dmg_doom;
        total_dmg_siphon_life += out.sum_dmg_siphon_life;
        total_dmg_imm += out.sum_dmg_imm;
        total_dmg_sb_urn += out.sum_dmg_sb_urn;
        total_dmg_conflag += out.sum_dmg_conflag;
        total_dmg_incin += out.sum_dmg_incin;
        total_dmg_sp += out.sum_dmg_sp;
        total_dmg_sf += out.sum_dmg_sf;
        total_dmg_dh += out.sum_dmg_dh;
        total_dmg_dl += out.sum_dmg_dl;
        total_dmg_ds += out.sum_dmg_ds;
        total_dmg_pet += out.sum_dmg_pet;
        total_dmg_pet_imp += out.sum_dmg_pet_imp;
        total_dmg_pet_succubus += out.sum_dmg_pet_succubus;
        total_dmg_all += out.sum_dmg_total;
    }

    std::sort(all_dps.begin(), all_dps.end());

    batch.mean_dps = total_dps / iterations;
    batch.min_dps = *std::min_element(all_dps.begin(), all_dps.end());
    batch.max_dps = *std::max_element(all_dps.begin(), all_dps.end());

    double variance = (total_dps_sq / iterations) - (batch.mean_dps * batch.mean_dps);
    batch.std_dev_dps = (variance > 0.0) ? std::sqrt(variance) : 0.0;

    // Percentiles
    auto get_pct = [&](double pct) {
        size_t idx = static_cast<size_t>(pct * (all_dps.size() - 1));
        return all_dps[idx];
    };
    batch.p1_dps = get_pct(0.01);
    batch.p5_dps = get_pct(0.05);
    batch.p25_dps = get_pct(0.25);
    batch.p50_dps = get_pct(0.50);
    batch.p75_dps = get_pct(0.75);
    batch.p95_dps = get_pct(0.95);
    batch.p99_dps = get_pct(0.99);

    batch.mean_isb_uptime = total_isb_uptime / iterations;
    batch.mean_shadow_bolts = static_cast<double>(total_sb) / iterations;
    batch.mean_crits = static_cast<double>(total_damage_crits) / iterations;
    batch.crit_percent = (total_damage_events > 0) ? (static_cast<double>(total_damage_crits) / total_damage_events) * 100.0 : 0.0;
    batch.miss_percent = (total_casts > 0) ? (static_cast<double>(total_misses) / total_casts) * 100.0 : 0.0;
    batch.mean_life_taps = static_cast<double>(total_life_taps) / iterations;
    batch.mean_mana_spent = total_mana_spent / iterations;
    batch.mean_pet_dps = (total_dmg_pet / iterations) / base_sim.fight_duration;

    if (total_dmg_all > 0.0) {
        batch.pct_shadow_bolt = (total_dmg_sb / total_dmg_all) * 100.0;
        batch.pct_corruption = (total_dmg_corr / total_dmg_all) * 100.0;
        batch.pct_curse = (total_dmg_curse / total_dmg_all) * 100.0;
        batch.pct_agony = (total_dmg_agony / total_dmg_all) * 100.0;
        batch.pct_doom = (total_dmg_doom / total_dmg_all) * 100.0;
        batch.pct_siphon_life = (total_dmg_siphon_life / total_dmg_all) * 100.0;
        batch.pct_immolate = (total_dmg_imm / total_dmg_all) * 100.0;
        batch.pct_shadowburn = (total_dmg_sb_urn / total_dmg_all) * 100.0;
        batch.pct_conflagrate = (total_dmg_conflag / total_dmg_all) * 100.0;
        batch.pct_incinerate = (total_dmg_incin / total_dmg_all) * 100.0;
        batch.pct_searing_pain = (total_dmg_sp / total_dmg_all) * 100.0;
        batch.pct_soul_fire = (total_dmg_sf / total_dmg_all) * 100.0;
        batch.pct_drain_hope = (total_dmg_dh / total_dmg_all) * 100.0;
        batch.pct_drain_life = (total_dmg_dl / total_dmg_all) * 100.0;
        batch.pct_drain_soul = (total_dmg_ds / total_dmg_all) * 100.0;
        batch.pct_pet = (total_dmg_pet / total_dmg_all) * 100.0;
        batch.pct_pet_imp = (total_dmg_pet_imp / total_dmg_all) * 100.0;
        batch.pct_pet_succubus = (total_dmg_pet_succubus / total_dmg_all) * 100.0;
    }

    // Build 40-bin Histogram
    const int num_bins = 40;
    double span = batch.max_dps - batch.min_dps;
    if (span <= 0.001) span = 1.0;
    double bin_width = span / num_bins;

    batch.histogram.resize(num_bins);
    for (int b = 0; b < num_bins; ++b) {
        batch.histogram[b].min_dps = batch.min_dps + b * bin_width;
        batch.histogram[b].max_dps = batch.histogram[b].min_dps + bin_width;
        batch.histogram[b].count = 0;
    }

    for (double val : all_dps) {
        int b = static_cast<int>((val - batch.min_dps) / bin_width);
        if (b < 0) b = 0;
        if (b >= num_bins) b = num_bins - 1;
        batch.histogram[b].count++;
    }

    // Run 1 sample timeline simulation
    WarlockSimulator timeline_sim = base_sim;
    timeline_sim.record_timeline = true;
    FastRNG sample_rng(0x13374242ULL);
    batch.sample_timeline = timeline_sim.run_single_simulation(sample_rng);

    return batch;
}

} // namespace warlock
