#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>

#include "src/sim/warlock_sim.hpp"
#include "src/sim/parallel_runner.hpp"
#include "src/sim/optimizer.hpp"
#include "src/ui/ui_app.hpp"

using namespace warlock;

void print_help() {
    std::cout << "WoW: Forever Warlock DES\n"
              << "Usage: ./warlock_sim [options]\n\n"
              << "Options:\n"
              << "  --headless                     Run in headless CLI mode (no window)\n"
              << "  --iterations <N>               Number of fight simulations (default: 10000)\n"
              << "  --duration <seconds>           Duration of each fight in seconds (default: 120)\n"
              << "  --threads <N>                  Worker threads (default: hardware concurrency)\n"
              << "  --spec <name>                  Talent spec: shadow_destro, fire_destro, demonic_pact, deep_affliction, sm_ruin, nf_af\n"
              << "  --gear <name>                  Gear preset: preraid, p3, p5, p6\n"
              << "  --race <name>                  Playable race: undead, orc, troll, human, gnome\n"
              << "  --raw-stats                    Direct stat mode (overrides gear items)\n"
              << "  --sp <value>                   Direct generic spell power value\n"
              << "  --snapshotting <0|1>           Toggle DoT snapshotting (1: Classic, 0: Forever default)\n"
              << "  --optimize-talents             Run brute-force talent optimizer\n"
              << "  --optimize-gear                Run brute-force gear optimizer\n"
              << "  --optimize-policy              Run brute-force rotation policy optimizer\n"
              << "  --snapshotting-study           Run comparative snapshotting impact study\n"
              << "  --json <file>                  Export batch summary to JSON\n"
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

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--iterations" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            sim.fight_duration = std::stod(argv[++i]);
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
        }
    }

    std::cout << "========================================================================\n"
              << "     WOW FOREVER WARLOCK DES SIMULATOR (HEADLESS MULTI-THREADED)        \n"
              << "========================================================================\n"
              << "Settings: Duration=" << sim.fight_duration << "s | Workers=" << threads
              << " | Snapshotting=" << (sim.mechanics.snapshot_dots ? "ON (Classic)" : "OFF (Forever)") << "\n\n";

    if (opt_talents) {
        std::cout << ">>> Running Brute-Force Talent Build Sweep (" << iterations << " iterations per spec)..." << std::endl;
        auto results = Optimizer::optimize_talents(sim, iterations, [](float p, const std::string& name) {
            std::cout << "  [" << static_cast<int>(p * 100) << "%] Testing: " << name << "\r" << std::flush;
        });
        std::cout << "\n\n--- TALENT SWEEP LEADERBOARD ---" << std::endl;
        for (const auto& r : results) {
            std::cout << "Rank #" << r.rank << " " << std::left << std::setw(44) << r.name
                      << " -> Mean DPS: " << std::fixed << std::setprecision(1) << r.mean_dps
                      << " +/- " << r.std_dev_dps << " (ISB: " << r.isb_uptime << "%)\n";
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
    BatchSimResult batch = ParallelSimRunner::run_batch(sim, iterations, threads);

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
    if (batch.pct_doom > 0.001)        std::cout << "  Curse of Doom: " << batch.pct_doom << "%\n";
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
                << "  \"iterations\": " << batch.total_iterations << ",\n"
                << "  \"sim_time_seconds\": " << batch.total_sim_time_seconds << ",\n"
                << "  \"mean_dps\": " << batch.mean_dps << ",\n"
                << "  \"std_dev_dps\": " << batch.std_dev_dps << ",\n"
                << "  \"p5_dps\": " << batch.p5_dps << ",\n"
                << "  \"p50_dps\": " << batch.p50_dps << ",\n"
                << "  \"p95_dps\": " << batch.p95_dps << ",\n"
                << "  \"isb_uptime\": " << batch.mean_isb_uptime << ",\n"
                << "  \"crit_percent\": " << batch.crit_percent << "\n"
                << "}\n";
            std::cout << "\nResults exported to " << json_output << std::endl;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--headless") == 0) headless = true;
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        }
    }

#ifndef _WIN32
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
