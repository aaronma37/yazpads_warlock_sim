#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdint>
#include <vector>

#include "src/sim/warlock_sim.hpp"
#include "src/sim/parallel_runner.hpp"
#include "src/sim/optimizer.hpp"
#include "src/cli_config.hpp"
#include "src/sim/warlock/surrogate_dataset_generator.hpp"
#include "src/ui/ui_app.hpp"

using namespace warlock;

unsigned int GetWowTextureIdByName(const char* name) {
    if (!name) return 0;
    const auto& tex = warlock::AssetManager::get().get_texture(name);
    const auto& fb = warlock::AssetManager::get().get_fallback();
    if (tex.id > 0 && tex.id != fb.id) return tex.id;
    return 0;
}

void print_help() {
    std::cout << "WoW: Forever Warlock DES\n"
              << "Usage: ./warlock_sim [options]\n\n"
              << "Options:\n"
              << "  --headless                     Run in headless CLI mode (no window)\n"
              << "  --iterations <N>               Number of fight simulations (default: 10000)\n"
              << "  --duration <seconds>           Duration of each fight in seconds (default: 120)\n"
              << "  --randomize-duration <0|1>     Randomize fight duration (default: 0)\n"
              << "  --duration-variance <seconds>  Uniform +/- duration spread (default: 30)\n"
              << "  --threads <N>                  Worker threads (default: hardware concurrency)\n"
              << "  --seed <N>                     Base RNG seed for reproducible batches (default: 1337)\n"
              << "  --config <file>                Load JSON configuration before CLI overrides\n"
              << "  --spec <name>                  Talent spec: shadow_destro, fire_destro, demonic_pact, deep_affliction, sm_ruin, nf_af\n"
              << "  --gear <name>                  Gear preset: preraid, p3, p5, p6\n"
              << "  --race <name>                  Playable race: undead, orc, troll, human, gnome\n"
              << "  --raw-stats                    Direct stat mode (overrides gear items)\n"
              << "  --sp <value>                   Direct generic spell power value\n"
              << "  --snapshotting <0|1>           Toggle DoT snapshotting (1: Classic, 0: Forever default)\n"
              << "  --imp-firebolt <modern|classic> Toggle Imp Firebolt scaling (default: modern)\n"
              << "  --optimize-talents             Run brute-force talent optimizer\n"
              << "  --optimize-gear                Run brute-force gear optimizer\n"
              << "  --optimize-policy              Run brute-force rotation policy optimizer\n"
              << "  --snapshotting-study           Run comparative snapshotting impact study\n"
              << "  --json <file>                  Export batch summary to JSON\n"
              << "  --generate-surrogate-dataset <file> Generate Monte-Carlo ML training dataset\n"
              << "  --samples <N>                  Number of parameter samples for ML dataset (default: 2500)\n"
              << "  --sample-iters <M>             DES sims per sample point (default: 500)\n"
              << "  --help                         Show this help message\n";
}

int run_headless(int argc, char* argv[]) {
    WarlockSimulator sim;
    int iterations = 10000;
    int threads = static_cast<int>(std::thread::hardware_concurrency());
    bool opt_talents = false;
    bool opt_gear = false;
    bool opt_policy = false;
    bool opt_snapshot = false;
    std::string json_output = "";
    uint64_t base_seed = 1337;

    // Load the JSON first so explicit CLI flags later in argv override it.
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--config") {
            try {
                cli_config::load_file(argv[i + 1], sim, iterations, threads, base_seed);
            } catch (const std::exception& e) {
                std::cerr << "Configuration error: " << e.what() << "\n";
                return 2;
            }
            break;
        }
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            ++i;
        } else if (arg == "--iterations" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            sim.fight_duration = std::stod(argv[++i]);
        } else if (arg == "--randomize-duration" && i + 1 < argc) {
            sim.randomize_duration = (std::stoi(argv[++i]) != 0);
        } else if (arg == "--duration-variance" && i + 1 < argc) {
            sim.duration_variance = std::stod(argv[++i]);
            if (sim.duration_variance < 0.0) {
                std::cerr << "--duration-variance must be non-negative\n";
                return 2;
            }
        } else if (arg == "--seed" && i + 1 < argc) {
            base_seed = static_cast<uint64_t>(std::stoull(argv[++i]));
            if (base_seed == 0) {
                std::cerr << "--seed must be non-zero\n";
                return 2;
            }
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } else if (arg == "--spec" && i + 1 < argc) {
            std::string spec = argv[++i];
            if (spec == "sm_ruin" || spec == "sm-ruin" || spec == "sm") {
                sim.talents = Talents::create_forever_sm_ruin();
                sim.buffs.sacrifice_succubus = false;
                sim.buffs.sacrifice_imp = false;
                sim.policy.pet = PetChoice::SUCCUBUS;
            } else if (spec == "fire_destro" || spec == "fire-destro" || spec == "fire") {
                sim.talents = Talents::create_forever_fire_destro();
                sim.buffs.sacrifice_succubus = true;
                sim.buffs.sacrifice_imp = false;
                sim.policy.maintain_immolate = true;
                sim.policy.rotation = RotationChoice::FIRE_DESTRO;
                sim.policy.pet = PetChoice::NONE;
            } else if (spec == "demonic_pact" || spec == "demo_pact" || spec == "pact" || spec == "demo") {
                sim.talents = Talents::create_forever_demonic_pact();
                sim.buffs.sacrifice_succubus = false;
                sim.buffs.sacrifice_imp = true;
                sim.policy.pet = PetChoice::SUCCUBUS;
            } else if (spec == "deep_affliction" || spec == "affliction" || spec == "aff") {
                sim.talents = Talents::create_forever_deep_affliction();
                sim.buffs.sacrifice_succubus = false;
                sim.buffs.sacrifice_imp = true;
                sim.policy.rotation = RotationChoice::DEEP_AFFLICTION_SB;
                sim.policy.pet = PetChoice::NONE;
            } else if (spec == "md_ruin" || spec == "md-ruin" || spec == "md") {
                sim.talents = Talents::create_forever_md_ruin();
                sim.buffs.sacrifice_succubus = false;
                sim.buffs.sacrifice_imp = false;
                sim.policy.pet = PetChoice::SUCCUBUS;
            } else if (spec == "nf_af" || spec == "nf-af" || spec == "nf") {
                sim.talents = Talents::create_forever_nf_af();
                sim.buffs.sacrifice_succubus = false;
                sim.buffs.sacrifice_imp = false;
                sim.policy.pet = PetChoice::IMP;
                sim.policy.rotation = RotationChoice::SM_RUIN;
            } else {
                sim.talents = Talents::create_forever_shadow_destro();
                sim.buffs.sacrifice_succubus = false;
                sim.buffs.sacrifice_imp = true;
                sim.policy.pet = PetChoice::NONE;
            }
        } else if (arg == "--gear" && i + 1 < argc) {
            std::string g = argv[++i];
            if (g == "preraid") sim.gear = GearLoadout::create_preraid_bis();
            else if (g == "p5") sim.gear = GearLoadout::create_phase5_bis();
            else if (g == "p6") sim.gear = GearLoadout::create_phase6_bis();
            else sim.gear = GearLoadout::create_phase3_bis();
        } else if (arg == "--race" && i + 1 < argc) {
            std::string r = argv[++i];
            if (r == "orc" || r == "Orc") sim.race = Race::ORC;
            else if (r == "troll" || r == "Troll") sim.race = Race::TROLL;
            else if (r == "human" || r == "Human") sim.race = Race::HUMAN;
            else if (r == "gnome" || r == "Gnome") sim.race = Race::GNOME;
            else sim.race = Race::UNDEAD;
            sim.base_attrs = get_base_attributes_for_race(sim.race);
        } else if (arg == "--raw-stats" || arg == "--direct-stats") {
            sim.use_raw_stats = true;
            if (sim.raw_stats.spell_power == 0.0) sim.raw_stats = sim.gear.calculate_stats();
        } else if (arg == "--sp" && i + 1 < argc) {
            sim.use_raw_stats = true;
            sim.raw_stats.spell_power = std::stod(argv[++i]);
        } else if (arg == "--snapshotting" && i + 1 < argc) {
            sim.mechanics.snapshot_dots = (std::stoi(argv[++i]) != 0);
        } else if (arg == "--imp-firebolt" && i + 1 < argc) {
            std::string mode = argv[++i];
            sim.mechanics.imp_firebolt_modern_scaling = (mode != "classic" && mode != "0");
        } else if (arg == "--optimize-talents") {
            opt_talents = true;
        } else if (arg == "--optimize-gear") {
            opt_gear = true;
        } else if (arg == "--optimize-policy") {
            opt_policy = true;
        } else if (arg == "--snapshotting-study") {
            opt_snapshot = true;
        } else if (arg == "--optimize-consumables") {
            std::cout << ">>> Running Consumables Optimization..." << std::endl;
            auto results = Optimizer::compare_consumable_tiers(sim, iterations);
            std::cout << "\n=======================================================\n";
            std::cout << "             CONSUMABLES COMPARISON RESULTS            \n";
            std::cout << "=======================================================\n";
            for (const auto& r : results) {
                std::cout << "#" << r.rank << " " << r.name << "\n   Mean DPS: " << r.mean_dps << " +/- " << r.std_dev_dps << "\n";
            }
            return 0;
        } else if (arg == "--optimize-stats" || arg == "--stat-weights") {
            std::cout << ">>> Running Stat Weight & EP Analysis..." << std::endl;
            auto results = Optimizer::compare_stat_values(sim, iterations);
            std::cout << "\n=======================================================\n";
            std::cout << "             STAT WEIGHTS & EP RESULTS                 \n";
            std::cout << "=======================================================\n";
            for (const auto& r : results) {
                std::cout << "#" << r.rank << " " << r.name << "\n   Mean DPS: " << r.mean_dps << " +/- " << r.std_dev_dps << "\n";
            }
            return 0;
        } else if (arg == "--combinatorial-talents") {
            std::cout << ">>> Running Combinatorial Talent Point Exploration..." << std::endl;
            auto results = Optimizer::explore_combinatorial_talents(sim, iterations);
            std::cout << "\n=======================================================\n";
            std::cout << "         COMBINATORIAL TALENTS LEADERBOARD             \n";
            std::cout << "=======================================================\n";
            for (const auto& r : results) {
                std::cout << "#" << r.rank << " " << r.name << "\n   Mean DPS: " << r.mean_dps << " +/- " << r.std_dev_dps << "\n";
            }
            return 0;
        } else if (arg == "--json" && i + 1 < argc) {
            json_output = argv[++i];
        } else if (arg == "--generate-surrogate-dataset" || arg == "--generate-dataset") {
            SurrogateConfig cfg;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                cfg.output_path = argv[++i];
            }
            cfg.num_samples = 2500;
            cfg.iters_per_sample = 500;
            cfg.num_threads = threads;
            for (int j = 1; j < argc; ++j) {
                std::string sarg = argv[j];
                if (sarg == "--samples" && j + 1 < argc) cfg.num_samples = std::stoi(argv[++j]);
                if (sarg == "--sample-iters" && j + 1 < argc) cfg.iters_per_sample = std::stoi(argv[++j]);
            }
            std::cout << "========================================================================\n"
                      << "       WOW FOREVER WARLOCK SURROGATE DATASET GENERATOR                  \n"
                      << "========================================================================\n"
                      << "Output CSV:        " << cfg.output_path << "\n"
                      << "Parameter Samples: " << cfg.num_samples << "\n"
                      << "Sims Per Sample:   " << cfg.iters_per_sample << "\n"
                      << "Total Simulations: " << (cfg.num_samples * cfg.iters_per_sample) << "\n"
                      << "Worker Threads:    " << cfg.num_threads << "\n\n";
            SurrogateDatasetGenerator::generate_dataset(cfg, [](int cur, int tot) {
                std::cout << "  [" << (cur * 100 / tot) << "%] Generated " << cur << " / " << tot << " samples...\r" << std::flush;
            });
            return 0;
        }
    }

    std::cout << "========================================================================\n"
              << "     WOW FOREVER WARLOCK DES SIMULATOR (HEADLESS MULTI-THREADED)        \n"
              << "========================================================================\n"
              << "Settings: Duration=" << sim.fight_duration << "s | Workers=" << threads
              << " | Duration variance=" << (sim.randomize_duration ? "+/- " + std::to_string(sim.duration_variance) + "s" : "OFF")
              << " | Seed=" << base_seed
              << " | Snapshotting=" << (sim.mechanics.snapshot_dots ? "ON (Classic)" : "OFF (Forever)") << "\n\n";

    if (opt_talents) {
        std::cout << ">>> Running Brute-Force Talent Build Sweep (" << iterations << " iterations per spec)..." << std::endl;
        auto results = Optimizer::optimize_talents(sim, iterations, [](float p, const std::string& name) {
            std::cout << "  [" << static_cast<int>(p * 100) << "%] Testing: " << name << "\r" << std::flush;
        });
        std::cout << "\n\n--- TALENT SWEEP LEADERBOARD ---" << std::endl;
        for (const auto& r : results) {
            std::cout << "Rank #" << r.rank << " " << std::left << std::setw(44) << r.name
                      << " -> Mean DPS: " << std::fixed << std::setprecision(1) << r.mean_dps;
            if (r.inferred_dps > 0.0) {
                std::cout << " | Inferred: " << std::setprecision(1) << r.inferred_dps << " (" << std::showpos << std::setprecision(1) << (r.inferred_dps - r.mean_dps) << std::noshowpos << ")";
            }
            std::cout << " +/- " << r.std_dev_dps << " (ISB: " << r.isb_uptime << "%)\n";
        }
        return 0;
    }

    if (opt_gear) {
        std::cout << ">>> Running Gear Progression Tier Sweep..." << std::endl;
        auto results = Optimizer::optimize_gear(sim, iterations);
        std::cout << "\n--- GEAR PROGRESSION LEADERBOARD ---" << std::endl;
        for (const auto& r : results) {
            std::cout << "Rank #" << r.rank << " " << std::left << std::setw(40) << r.name
                      << " -> Mean DPS: " << std::fixed << std::setprecision(1) << r.mean_dps
                      << " +/- " << r.std_dev_dps << "\n";
        }
        return 0;
    }

    if (opt_policy) {
        std::cout << ">>> Running Rotational Policy Sweep..." << std::endl;
        auto results = Optimizer::optimize_policy(sim, iterations);
        std::cout << "\n--- ROTATION POLICY LEADERBOARD ---" << std::endl;
        for (const auto& r : results) {
            std::cout << "Rank #" << r.rank << " " << std::left << std::setw(44) << r.name
                      << " -> Mean DPS: " << std::fixed << std::setprecision(1) << r.mean_dps
                      << " +/- " << r.std_dev_dps << "\n";
        }
        return 0;
    }

    if (opt_snapshot) {
        std::cout << ">>> Running Snapshotting Mechanics Comparative Study..." << std::endl;
        auto results = Optimizer::evaluate_snapshotting_impact(sim, iterations);
        std::cout << "\n--- SNAPSHOTTING DELTA ANALYSIS ---" << std::endl;
        for (const auto& r : results) {
            std::cout << "Rank #" << r.rank << " " << std::left << std::setw(48) << r.name
                      << " -> " << std::fixed << std::setprecision(1) << r.mean_dps << " DPS\n";
        }
        return 0;
    }

    // Default single batch run
    std::cout << "Executing " << iterations << " Discrete Event Simulations across " << threads << " threads..." << std::endl;
    BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations, threads, nullptr, base_seed);

    std::cout << "\n>>> SIMULATION RESULTS <<<\n"
              << "Throughput:     " << batch.total_iterations << " iterations in " 
              << std::fixed << std::setprecision(3) << batch.total_sim_time_seconds << "s ("
              << std::setprecision(0) << batch.iterations_per_second << " sims/sec)\n"
              << "Mean DPS:       " << std::setprecision(1) << batch.mean_dps << " +/- " << batch.std_dev_dps << "\n"
              << "Median (P50):   " << batch.p50_dps << "\n"
              << "P5 - P95 Range: [" << batch.p5_dps << " - " << batch.p95_dps << "]\n"
              << "Min - Max DPS:  [" << batch.min_dps << " - " << batch.max_dps << "]\n"
              << "ISB Uptime:     " << batch.mean_isb_uptime << "%\n"
              << "Crit Rate:      " << batch.crit_percent << "%\n"
              << "Miss Rate:      " << batch.miss_percent << "%\n"
              << "Avg Life Taps:  " << batch.mean_life_taps << "\n\n";
    std::cout << "Damage Breakdown:\n";
    if (batch.pct_shadow_bolt > 0.001) std::cout << "  Shadow Bolt:   " << batch.pct_shadow_bolt << "%\n";
    if (batch.pct_corruption > 0.001)  std::cout << "  Corruption:    " << batch.pct_corruption << "%\n";
    if (batch.pct_agony > 0.001)       std::cout << "  Bane of Agony: " << batch.pct_agony << "%\n";
    if (batch.pct_doom > 0.001)        std::cout << "  Bane of Doom:  " << batch.pct_doom << "%\n";
    if (batch.pct_siphon_life > 0.001) std::cout << "  Siphon Life:   " << batch.pct_siphon_life << "%\n";
    if (batch.pct_immolate > 0.001)    std::cout << "  Immolate:      " << batch.pct_immolate << "%\n";
    if (batch.pct_shadowburn > 0.001)  std::cout << "  Shadowburn:    " << batch.pct_shadowburn << "%\n";
    if (batch.pct_conflagrate > 0.001) std::cout << "  Conflagrate:   " << batch.pct_conflagrate << "%\n";
    if (batch.pct_incinerate > 0.001)  std::cout << "  Incinerate:    " << batch.pct_incinerate << "%\n";
    if (batch.pct_searing_pain > 0.001)std::cout << "  Searing Pain:  " << batch.pct_searing_pain << "%\n";
    if (batch.pct_soul_fire > 0.001)   std::cout << "  Soul Fire:     " << batch.pct_soul_fire << "%\n";
    if (batch.pct_drain_hope > 0.001)  std::cout << "  Wrack:         " << batch.pct_drain_hope << "%\n";
    if (batch.pct_pet_imp > 0.001) {
        std::cout << "  Imp (Firebolt):" << batch.pct_pet_imp << "% (" << batch.mean_pet_dps << " DPS)\n";
    } else if (batch.pct_pet_succubus > 0.001) {
        std::cout << "  Succubus:      " << batch.pct_pet_succubus << "% (" << batch.mean_pet_dps << " DPS)\n";
    } else if (batch.pct_pet > 0.001) {
        std::cout << "  Demon (Pet):   " << batch.pct_pet << "% (" << batch.mean_pet_dps << " DPS)\n";
    }

    if (!json_output.empty()) {
        std::ofstream ofs(json_output);
        if (ofs.is_open()) {
            ofs << "{\n"
                << "  \"seed\": " << base_seed << ",\n"
                << "  \"duration\": " << sim.fight_duration << ",\n"
                << "  \"randomize_duration\": " << (sim.randomize_duration ? "true" : "false") << ",\n"
                << "  \"duration_variance\": " << sim.duration_variance << ",\n"
                << "  \"iterations\": " << batch.total_iterations << ",\n"
                << "  \"sim_time_seconds\": " << batch.total_sim_time_seconds << ",\n"
                << "  \"mean_dps\": " << batch.mean_dps << ",\n"
                << "  \"std_dev_dps\": " << batch.std_dev_dps << ",\n"
                << "  \"p5_dps\": " << batch.p5_dps << ",\n"
                << "  \"p50_dps\": " << batch.p50_dps << ",\n"
                << "  \"p95_dps\": " << batch.p95_dps << ",\n"
                << "  \"isb_uptime\": " << batch.mean_isb_uptime << ",\n"
                << "  \"crit_percent\": " << batch.crit_percent << ",\n"
                << "  \"damage_breakdown\": {\n"
                << "    \"shadow_bolt\": {\"pct_total\": " << batch.pct_shadow_bolt << ", \"mean_dps\": " << batch.mean_dps * batch.pct_shadow_bolt / 100.0 << "},\n"
                << "    \"corruption\": {\"pct_total\": " << batch.pct_corruption << ", \"mean_dps\": " << batch.mean_dps * batch.pct_corruption / 100.0 << "},\n"
                << "    \"curse\": {\"pct_total\": " << batch.pct_curse << ", \"mean_dps\": " << batch.mean_dps * batch.pct_curse / 100.0 << "},\n"
                << "    \"agony\": {\"pct_total\": " << batch.pct_agony << ", \"mean_dps\": " << batch.mean_dps * batch.pct_agony / 100.0 << "},\n"
                << "    \"doom\": {\"pct_total\": " << batch.pct_doom << ", \"mean_dps\": " << batch.mean_dps * batch.pct_doom / 100.0 << "},\n"
                << "    \"bane_of_havoc\": {\"pct_total\": " << batch.pct_bane_of_havoc << ", \"mean_dps\": " << batch.mean_dps * batch.pct_bane_of_havoc / 100.0 << "},\n"
                << "    \"siphon_life\": {\"pct_total\": " << batch.pct_siphon_life << ", \"mean_dps\": " << batch.mean_dps * batch.pct_siphon_life / 100.0 << "},\n"
                << "    \"immolate\": {\"pct_total\": " << batch.pct_immolate << ", \"mean_dps\": " << batch.mean_dps * batch.pct_immolate / 100.0 << "},\n"
                << "    \"shadowburn\": {\"pct_total\": " << batch.pct_shadowburn << ", \"mean_dps\": " << batch.mean_dps * batch.pct_shadowburn / 100.0 << "},\n"
                << "    \"conflagrate\": {\"pct_total\": " << batch.pct_conflagrate << ", \"mean_dps\": " << batch.mean_dps * batch.pct_conflagrate / 100.0 << "},\n"
                << "    \"incinerate\": {\"pct_total\": " << batch.pct_incinerate << ", \"mean_dps\": " << batch.mean_dps * batch.pct_incinerate / 100.0 << "},\n"
                << "    \"searing_pain\": {\"pct_total\": " << batch.pct_searing_pain << ", \"mean_dps\": " << batch.mean_dps * batch.pct_searing_pain / 100.0 << "},\n"
                << "    \"soul_fire\": {\"pct_total\": " << batch.pct_soul_fire << ", \"mean_dps\": " << batch.mean_dps * batch.pct_soul_fire / 100.0 << "},\n"
                << "    \"drain_hope\": {\"pct_total\": " << batch.pct_drain_hope << ", \"mean_dps\": " << batch.mean_dps * batch.pct_drain_hope / 100.0 << "},\n"
                << "    \"drain_life\": {\"pct_total\": " << batch.pct_drain_life << ", \"mean_dps\": " << batch.mean_dps * batch.pct_drain_life / 100.0 << "},\n"
                << "    \"drain_soul\": {\"pct_total\": " << batch.pct_drain_soul << ", \"mean_dps\": " << batch.mean_dps * batch.pct_drain_soul / 100.0 << "},\n"
                << "    \"pet\": {\"pct_total\": " << batch.pct_pet << ", \"mean_dps\": " << batch.mean_dps * batch.pct_pet / 100.0 << "},\n"
                << "    \"pet_imp\": {\"pct_total\": " << batch.pct_pet_imp << ", \"mean_dps\": " << batch.mean_dps * batch.pct_pet_imp / 100.0 << "},\n"
                << "    \"pet_succubus\": {\"pct_total\": " << batch.pct_pet_succubus << ", \"mean_dps\": " << batch.mean_dps * batch.pct_pet_succubus / 100.0 << "},\n"
                << "    \"pet_melee\": {\"pct_total\": " << batch.pct_pet_melee << ", \"mean_dps\": " << batch.mean_dps * batch.pct_pet_melee / 100.0 << "},\n"
                << "    \"pet_lash_of_pain\": {\"pct_total\": " << batch.pct_pet_lash_of_pain << ", \"mean_dps\": " << batch.mean_dps * batch.pct_pet_lash_of_pain / 100.0 << "},\n"
                << "    \"pet_firebolt\": {\"pct_total\": " << batch.pct_pet_firebolt << ", \"mean_dps\": " << batch.mean_dps * batch.pct_pet_firebolt / 100.0 << "},\n"
                << "    \"demonic_brand\": {\"pct_total\": " << batch.pct_demonic_brand << ", \"mean_dps\": " << batch.mean_dps * batch.pct_demonic_brand / 100.0 << "},\n"
                << "    \"touch_of_the_grave\": {\"pct_total\": " << batch.pct_touch_of_the_grave << ", \"mean_dps\": " << batch.mean_dps * batch.pct_touch_of_the_grave / 100.0 << "}\n"
                << "  }\n"
                << "}\n";
            std::cout << "\nResults exported to " << json_output << std::endl;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--headless") == 0 ||
            std::strcmp(argv[i], "--generate-surrogate-dataset") == 0 ||
            std::strcmp(argv[i], "--generate-dataset") == 0) {
            headless = true;
        }
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        }
    }

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    // Check if running in headless environment (e.g. DISPLAY not set)
    if (!headless && getenv("DISPLAY") == nullptr && getenv("WAYLAND_DISPLAY") == nullptr) {
        std::cout << "No graphical display detected ($DISPLAY / $WAYLAND_DISPLAY unset). Defaulting to headless CLI mode.\n"
                  << "Pass --help for available CLI options.\n\n";
        headless = true;
    }
#endif

    if (headless) {
        return run_headless(argc, argv);
    } else {
        WarlockSimApp app;
        app.run_gui();
        return 0;
    }
}
