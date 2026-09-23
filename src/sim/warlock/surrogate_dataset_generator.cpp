#include "surrogate_dataset_generator.hpp"
#include "spec_presets.hpp"
#include "talent_graph.hpp"
#include "policy.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cmath>
#include <filesystem>
#include <algorithm>

namespace warlock {

enum class DemonSetup : uint8_t {
    NO_PET_NO_SAC = 0,             // No active pet, no sacrifice
    ACTIVE_IMP = 1,                // Active Imp, no sacrifice
    ACTIVE_SUCCUBUS = 2,           // Active Succubus, no sacrifice
    SAC_IMP = 3,                   // No active pet, Imp sacrificed (+15% Shadow in Forever)
    SAC_SUCCUBUS = 4,              // No active pet, Succubus sacrificed (+15% Fire in Forever)
    DEMONIC_PACT_SUCC_SAC_IMP = 5, // Demonic Pact: Succubus active + Imp sacrificed
    DEMONIC_PACT_IMP_SAC_SUCC = 6  // Demonic Pact: Imp active + Succubus sacrificed
};

inline void apply_demon_setup_to_sim(DemonSetup ds, WarlockSimulator& sim) {
    sim.policy.pet = PetChoice::NONE;
    sim.buffs.sacrifice_imp = false;
    sim.buffs.sacrifice_succubus = false;

    switch (ds) {
        case DemonSetup::ACTIVE_IMP:
            sim.policy.pet = PetChoice::IMP;
            break;
        case DemonSetup::ACTIVE_SUCCUBUS:
            sim.policy.pet = PetChoice::SUCCUBUS;
            break;
        case DemonSetup::SAC_IMP:
            sim.buffs.sacrifice_imp = true;
            break;
        case DemonSetup::SAC_SUCCUBUS:
            sim.buffs.sacrifice_succubus = true;
            break;
        case DemonSetup::DEMONIC_PACT_SUCC_SAC_IMP:
            sim.policy.pet = PetChoice::SUCCUBUS;
            sim.buffs.sacrifice_imp = true;
            break;
        case DemonSetup::DEMONIC_PACT_IMP_SAC_SUCC:
            sim.policy.pet = PetChoice::IMP;
            sim.buffs.sacrifice_succubus = true;
            break;
        case DemonSetup::NO_PET_NO_SAC:
        default:
            break;
    }
}

struct SampleRow {
    std::string build_desc;
    int aff_points = 0;
    int demo_points = 0;
    int destro_points = 0;
    std::array<int, TOTAL_TALENT_NODES> talents{};

    int race_id = 0;
    double fight_duration = 0.0;
    double spell_power = 0.0;
    double shadow_power = 0.0;
    double fire_power = 0.0;
    double spell_hit_percent = 0.0;
    double spell_crit_percent = 0.0;
    double spell_haste_percent = 0.0;
    double intellect = 0.0;
    double spirit = 0.0;
    double stamina = 0.0;
    double max_mana = 0.0;
    int target_level = 63;
    double target_shadow_res = 0.0;
    double target_fire_res = 0.0;
    int curse_of_shadows = 0;
    int curse_of_elements = 0;
    int demon_setup = 0;
    int rotation_id = 0;
    int maintain_immolate = 0;

    // Target Outputs
    double dps = 0.0;
    double dps_std = 0.0;
    double isb_uptime_percent = 0.0;
    double avg_life_taps = 0.0;
    double avg_mana_spent = 0.0;
    double avg_mana_gained = 0.0;
};

bool SurrogateDatasetGenerator::generate_dataset(
    const SurrogateConfig& config,
    std::function<void(int current, int total)> progress_cb
) {
    int num_threads = config.num_threads;
    if (num_threads <= 0) {
        num_threads = static_cast<int>(std::thread::hardware_concurrency());
        if (num_threads <= 0) num_threads = 4;
    }

    const auto& presets = standard_spec_presets();
    const auto& graph = TalentGraph::get();

    std::vector<SampleRow> dataset(config.num_samples);
    std::atomic<size_t> next_sample_idx(0);
    std::atomic<int> completed_samples(0);

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            while (true) {
                size_t idx = next_sample_idx.fetch_add(1, std::memory_order_relaxed);
                if (idx >= static_cast<size_t>(config.num_samples)) break;

                // Deterministic PRNG per sample index
                uint64_t sample_seed = config.base_seed + (idx * 0x9e3779b97f4a7c15ULL);
                FastRNG sample_rng(sample_seed);

                // 1. Sample Strictly Valid 51-Point Talent Configuration
                std::array<int, TOTAL_TALENT_NODES> talent_vec{};
                std::string build_desc;

                if (sample_rng.chance(0.50)) {
                    // Method A: Pick standard preset and apply 0 to 4 valid point mutations
                    int p_idx = static_cast<int>(sample_rng.next_u64() % presets.size());
                    const SpecPreset& preset = presets[p_idx];
                    talent_vec = graph.to_vector(preset.make_talents());
                    build_desc = preset.short_label;

                    int num_mutations = static_cast<int>(sample_rng.next_u64() % 5);
                    for (int m = 0; m < num_mutations; ++m) {
                        auto donors = graph.get_valid_donors(talent_vec);
                        if (donors.empty()) break;
                        size_t d = donors[sample_rng.next_u64() % donors.size()];
                        talent_vec[d]--;

                        auto receivers = graph.get_valid_receivers(talent_vec);
                        if (!receivers.empty()) {
                            size_t r = receivers[sample_rng.next_u64() % receivers.size()];
                            talent_vec[r]++;
                        } else {
                            talent_vec[d]++; // Rollback if no valid receiver
                        }
                    }
                } else {
                    // Method B: Randomly grow a strictly valid 51-point build from scratch
                    talent_vec.fill(0);
                    for (int pt = 0; pt < 51; ++pt) {
                        auto receivers = graph.get_valid_receivers(talent_vec);
                        if (receivers.empty()) break;
                        size_t r = receivers[sample_rng.next_u64() % receivers.size()];
                        talent_vec[r]++;
                    }
                    if (!graph.is_valid(talent_vec, 51)) {
                        graph.repair(talent_vec, sample_rng, 51);
                    }
                    build_desc = "random_valid_51";
                }

                int aff_pts = graph.count_tree_points(talent_vec, 0);
                int demo_pts = graph.count_tree_points(talent_vec, 1);
                int destro_pts = graph.count_tree_points(talent_vec, 2);

                // Capstone / Key talent flags
                bool has_ds = (talent_vec[17 + 9] > 0);        // Demonic Sacrifice
                bool has_dp = (talent_vec[17 + 18] > 0);       // Demonic Pact
                bool has_ruin = (talent_vec[36 + 6] > 0);      // Ruin
                bool has_incin = (talent_vec[36 + 15] > 0);    // Incinerate
                bool has_conflag = (talent_vec[36 + 10] > 0);  // Conflagrate
                bool has_sm = (talent_vec[15] > 0);            // Shadow Mastery
                bool has_wrack = (talent_vec[16] > 0);         // Wrack

                // 2. Select Aligned Demon Setup (8 valid mutually exclusive states)
                DemonSetup demon_setup = DemonSetup::NO_PET_NO_SAC;
                if (has_dp) {
                    demon_setup = sample_rng.chance(0.8) ? DemonSetup::DEMONIC_PACT_SUCC_SAC_IMP : DemonSetup::DEMONIC_PACT_IMP_SAC_SUCC;
                } else if (has_ds) {
                    if (destro_pts >= 30 && has_incin && sample_rng.chance(0.6)) {
                        demon_setup = DemonSetup::SAC_SUCCUBUS; // +15% Fire for Fire Destro
                    } else if (sample_rng.chance(0.75)) {
                        demon_setup = DemonSetup::SAC_IMP;      // +15% Shadow for Shadow Destro
                    } else {
                        demon_setup = DemonSetup::ACTIVE_SUCCUBUS;
                    }
                } else {
                    if (demo_pts >= 15) {
                        demon_setup = sample_rng.chance(0.5) ? DemonSetup::ACTIVE_SUCCUBUS : DemonSetup::ACTIVE_IMP;
                    } else if (sample_rng.chance(0.7)) {
                        demon_setup = DemonSetup::ACTIVE_IMP;
                    } else {
                        demon_setup = DemonSetup::ACTIVE_SUCCUBUS;
                    }
                }

                // 3. Select Aligned Combat Policy / Rotation
                RotationChoice rotation = RotationChoice::SHADOW_DESTRO;
                bool maintain_immolate = false;

                if (has_incin || (destro_pts >= 30 && demon_setup == DemonSetup::SAC_SUCCUBUS)) {
                    maintain_immolate = true;
                    rotation = (has_conflag && sample_rng.chance(0.4)) 
                               ? RotationChoice::SHADOW_AND_FLAME_FIRE_2 
                               : RotationChoice::FIRE_DESTRO;
                } else if (has_wrack || aff_pts >= 35) {
                    maintain_immolate = false;
                    rotation = RotationChoice::DEEP_AFFLICTION_SB;
                } else if (has_dp || (demo_pts >= 25 && !has_ds)) {
                    maintain_immolate = false;
                    rotation = RotationChoice::DP_AF_SHADOW;
                } else if (has_sm && has_ruin) {
                    maintain_immolate = false;
                    rotation = RotationChoice::SM_RUIN;
                } else {
                    maintain_immolate = has_conflag;
                    rotation = RotationChoice::SHADOW_DESTRO;
                }

                // 4. Sample Final Character Stats (Continuous, post-buff state space)
                int race_id = static_cast<int>(sample_rng.next_u64() % 5);
                Race race = static_cast<Race>(race_id);

                double fight_duration = sample_rng.range(30.0, 300.0);
                double sp = sample_rng.range(100.0, 1400.0);       // Final effective SP
                double shadow_p = sample_rng.range(0.0, 200.0);    // Final bonus shadow SP
                double fire_p = sample_rng.range(0.0, 200.0);      // Final bonus fire SP
                double hit = sample_rng.range(0.0, 18.0);
                double crit = sample_rng.range(5.0, 55.0);         // Final effective spell crit %
                double haste = sample_rng.range(0.0, 35.0);
                double intel = sample_rng.range(100.0, 500.0);
                double spirit = sample_rng.range(50.0, 350.0);
                double stam = sample_rng.range(100.0, 600.0);

                int target_lvl = 60 + static_cast<int>(sample_rng.next_u64() % 4); // 60..63
                double shadow_res = sample_rng.range(0.0, 120.0);
                double fire_res = sample_rng.range(0.0, 120.0);

                bool cos = sample_rng.chance(0.5);
                bool coe = sample_rng.chance(0.5);

                // Build simulator instance
                WarlockSimulator sim;
                sim.race = race;
                sim.base_attrs = get_base_attributes_for_race(race);
                sim.talents = graph.to_talents(talent_vec);
                sim.policy.rotation = rotation;
                sim.policy.maintain_immolate = maintain_immolate;

                apply_demon_setup_to_sim(demon_setup, sim);

                sim.fight_duration = fight_duration;
                sim.randomize_duration = false;
                sim.record_timeline = false;

                // Final effective stats passed directly to raw_stats
                sim.use_raw_stats = true;
                sim.raw_stats.spell_power = sp;
                sim.raw_stats.shadow_power = shadow_p;
                sim.raw_stats.fire_power = fire_p;
                sim.raw_stats.spell_hit_percent = hit;
                sim.raw_stats.spell_crit_percent = crit;
                sim.raw_stats.spell_haste_percent = haste;
                sim.raw_stats.intellect = intel;
                sim.raw_stats.spirit = spirit;
                sim.raw_stats.stamina = stam;
                sim.raw_stats.max_mana = sim.base_attrs.base_mana + (intel * 15.0);
                sim.raw_stats.max_health = sim.base_attrs.base_health + (stam * 10.0);

                // Target and encounter
                sim.target_config.level = target_lvl;
                sim.target_config.base_shadow_resistance = shadow_res;
                sim.target_config.base_fire_resistance = fire_res;
                sim.target_config.current_shadow_resistance = shadow_res;
                sim.target_config.current_fire_resistance = fire_res;

                sim.buffs.curse_of_shadows = cos;
                sim.buffs.curse_of_elements = coe;

                // Run Monte-Carlo iterations for this sample point
                FastRNG sim_rng(sample_seed + 0xDEADBEEF);
                double sum_dps = 0.0;
                double sum_dps_sq = 0.0;
                double sum_isb_uptime = 0.0;
                double sum_life_taps = 0.0;
                double sum_mana_spent = 0.0;
                double sum_mana_gained = 0.0;

                int iters = config.iters_per_sample;
                for (int i = 0; i < iters; ++i) {
                    SimResult res = sim.run_single_simulation(sim_rng);
                    sum_dps += res.dps;
                    sum_dps_sq += res.dps * res.dps;
                    sum_isb_uptime += res.isb_uptime_percent;
                    sum_life_taps += res.life_taps;
                    sum_mana_spent += res.mana_spent;
                    sum_mana_gained += res.mana_gained;
                }

                double mean_dps = sum_dps / iters;
                double variance = (sum_dps_sq / iters) - (mean_dps * mean_dps);
                double std_dps = std::sqrt(std::max(0.0, variance));

                // Populate row
                SampleRow& row = dataset[idx];
                row.build_desc = build_desc;
                row.aff_points = aff_pts;
                row.demo_points = demo_pts;
                row.destro_points = destro_pts;
                row.talents = talent_vec;

                row.race_id = race_id;
                row.fight_duration = fight_duration;
                row.spell_power = sp;
                row.shadow_power = shadow_p;
                row.fire_power = fire_p;
                row.spell_hit_percent = hit;
                row.spell_crit_percent = crit;
                row.spell_haste_percent = haste;
                row.intellect = intel;
                row.spirit = spirit;
                row.stamina = stam;
                row.max_mana = sim.raw_stats.max_mana;
                row.target_level = target_lvl;
                row.target_shadow_res = shadow_res;
                row.target_fire_res = fire_res;
                row.curse_of_shadows = cos ? 1 : 0;
                row.curse_of_elements = coe ? 1 : 0;
                row.demon_setup = static_cast<int>(demon_setup);
                row.rotation_id = static_cast<int>(rotation);
                row.maintain_immolate = maintain_immolate ? 1 : 0;

                row.dps = mean_dps;
                row.dps_std = std_dps;
                row.isb_uptime_percent = sum_isb_uptime / iters;
                row.avg_life_taps = sum_life_taps / iters;
                row.avg_mana_spent = sum_mana_spent / iters;
                row.avg_mana_gained = sum_mana_gained / iters;

                int done = completed_samples.fetch_add(1, std::memory_order_relaxed) + 1;
                if (progress_cb && (done % 100 == 0 || done == config.num_samples)) {
                    progress_cb(done, config.num_samples);
                }
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double total_sec = std::chrono::duration<double>(end_time - start_time).count();
    std::cout << "\nDataset generation completed in " << std::fixed << std::setprecision(2)
              << total_sec << "s (" << (config.num_samples / std::max(0.01, total_sec))
              << " samples/sec)\n";

    // Write to CSV
    std::filesystem::path p(config.output_path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    std::ofstream out(config.output_path);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open output CSV file " << config.output_path << "\n";
        return false;
    }

    // CSV Header: build metadata, tree totals, 52 talent nodes, character stats, encounter, setup, dps
    out << "build_desc,aff_points,demo_points,destro_points";
    for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
        out << "," << graph.node(i).id;
    }
    out << ",race_id,fight_duration,spell_power,shadow_power,fire_power,"
        << "spell_hit_percent,spell_crit_percent,spell_haste_percent,intellect,spirit,"
        << "stamina,max_mana,target_level,target_shadow_res,target_fire_res,"
        << "curse_of_shadows,curse_of_elements,demon_setup,rotation_id,maintain_immolate,"
        << "dps,dps_std,isb_uptime_percent,avg_life_taps,avg_mana_spent,avg_mana_gained\n";

    for (const auto& r : dataset) {
        out << r.build_desc << ","
            << r.aff_points << "," << r.demo_points << "," << r.destro_points;
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            out << "," << r.talents[i];
        }
        out << "," << r.race_id << "," << r.fight_duration << ","
            << r.spell_power << "," << r.shadow_power << "," << r.fire_power << ","
            << r.spell_hit_percent << "," << r.spell_crit_percent << ","
            << r.spell_haste_percent << "," << r.intellect << "," << r.spirit << ","
            << r.stamina << "," << r.max_mana << "," << r.target_level << ","
            << r.target_shadow_res << "," << r.target_fire_res << ","
            << r.curse_of_shadows << "," << r.curse_of_elements << ","
            << r.demon_setup << "," << r.rotation_id << "," << r.maintain_immolate << ","
            << r.dps << "," << r.dps_std << "," << r.isb_uptime_percent << ","
            << r.avg_life_taps << "," << r.avg_mana_spent << "," << r.avg_mana_gained << "\n";
    }

    std::cout << "Successfully saved " << dataset.size() << " samples to "
              << config.output_path << " ("
              << (std::filesystem::file_size(config.output_path) / 1024.0 / 1024.0) << " MB)\n";

    return true;
}

} // namespace warlock
