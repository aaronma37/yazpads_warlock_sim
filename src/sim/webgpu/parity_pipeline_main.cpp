#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#include "parity_comparator.hpp"

using namespace warlock;

void print_help() {
    std::cout << "======================================================================\n";
    std::cout << " WebGPU vs CPU DES Parity Comparison Pipeline\n";
    std::cout << " Treats CPU Discrete Event Simulator as authoritative ground truth.\n";
    std::cout << "======================================================================\n";
    std::cout << "Usage: webgpu_parity_pipeline [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --all                 Run full standard parity matrix (default if no scenario specified)\n";
    std::cout << "  --sweep <stat>        Run parameter sweep (sp, hit, crit, haste, duration)\n";
    std::cout << "  --presets             Run standard spec preset builds (SM/Ruin, DS/Ruin, etc.)\n";
    std::cout << "  --scenario <name>     Run a specific scenario by name\n";
    std::cout << "  --iterations <N>      Number of simulation iterations per engine (default: 10000)\n";
    std::cout << "  --seed <S>            RNG seed (default: 42)\n";
    std::cout << "  --tolerance <pct>     Relative % tolerance threshold for DPS (default: 1.5)\n";
    std::cout << "  --json <file>         Export JSON comparison report to file\n";
    std::cout << "  --markdown <file>     Export Markdown comparison report to file\n";
    std::cout << "  --verbose, -v         Display detailed per-spell breakdowns and diagnostics\n";
    std::cout << "  --help, -h            Show this help message\n";
    std::cout << "======================================================================\n";
}

int run_pipeline(int argc, char** argv) {
    bool run_all = true;
    bool run_presets = false;
    std::string sweep_type = "";
    std::string target_scenario = "";
    int iterations = 10000;
    uint32_t seed = 42;
    double tolerance_pct = 1.5;
    std::string json_output_path = "";
    std::string markdown_output_path = "";
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        } else if (arg == "--all") {
            run_all = true;
        } else if (arg == "--presets") {
            run_presets = true;
            run_all = false;
        } else if (arg == "--sweep" && i + 1 < argc) {
            sweep_type = argv[++i];
            run_all = false;
        } else if (arg == "--scenario" && i + 1 < argc) {
            target_scenario = argv[++i];
            run_all = false;
        } else if (arg == "--iterations" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--tolerance" && i + 1 < argc) {
            tolerance_pct = std::stod(argv[++i]);
        } else if (arg == "--json" && i + 1 < argc) {
            json_output_path = argv[++i];
        } else if (arg == "--markdown" && i + 1 < argc) {
            markdown_output_path = argv[++i];
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        }
    }

    std::vector<ParityScenario> scenarios;

    if (!target_scenario.empty()) {
        auto all = ParityComparator::get_standard_suite();
        auto presets = ParityComparator::get_spec_preset_suite();
        all.insert(all.end(), presets.begin(), presets.end());
        for (const auto& sc : all) {
            if (sc.name == target_scenario) {
                scenarios.push_back(sc);
                break;
            }
        }
        if (scenarios.empty()) {
            std::cerr << "Error: Unknown scenario '" << target_scenario << "'\n";
            return 1;
        }
    } else if (!sweep_type.empty()) {
        if (sweep_type == "sp") scenarios = ParityComparator::get_spell_power_sweep();
        else if (sweep_type == "hit") scenarios = ParityComparator::get_hit_sweep();
        else if (sweep_type == "crit") scenarios = ParityComparator::get_crit_sweep();
        else if (sweep_type == "haste") scenarios = ParityComparator::get_haste_sweep();
        else if (sweep_type == "duration") scenarios = ParityComparator::get_duration_sweep();
        else {
            std::cerr << "Error: Unknown sweep type '" << sweep_type << "'. Valid options: sp, hit, crit, haste, duration\n";
            return 1;
        }
    } else if (run_presets) {
        scenarios = ParityComparator::get_spec_preset_suite();
    } else {
        scenarios = ParityComparator::get_standard_suite();
    }

    std::cout << "\n>>> Starting WebGPU vs CPU Parity Pipeline (" << scenarios.size()
              << " scenarios, " << iterations << " iterations each, seed=" << seed << ") <<<\n\n";

    std::vector<ParityReport> reports;
    reports.reserve(scenarios.size());
    int total_passed = 0;
    int total_failed = 0;

    for (auto& sc : scenarios) {
        sc.dps_tolerance_pct = tolerance_pct;
        ParityReport report = ParityComparator::compare(
            sc.name,
            sc.description,
            sc.sim_config,
            iterations,
            seed,
            sc.dps_tolerance_pct,
            sc.breakdown_tolerance_pct
        );

        report.print_summary(std::cout, verbose);
        if (report.overall_passed) {
            total_passed++;
        } else {
            total_failed++;
        }
        reports.push_back(report);
    }

    std::cout << "======================================================================\n";
    std::cout << " PARITY PIPELINE SUMMARY: " << total_passed << " PASSED, " << total_failed << " FAILED ("
              << scenarios.size() << " total scenarios)\n";
    std::cout << "======================================================================\n";

    // Write Markdown Output
    if (!markdown_output_path.empty()) {
        std::ofstream md_file(markdown_output_path);
        if (md_file.is_open()) {
            md_file << "# WebGPU vs CPU DES Parity Report\n\n";
            md_file << "**Generated at**: Live Evaluation  \n";
            md_file << "**Iterations**: " << iterations << "  \n";
            md_file << "**Summary**: " << total_passed << "/" << scenarios.size() << " Passed  \n\n";
            for (const auto& r : reports) {
                md_file << r.to_markdown() << "\n---\n\n";
            }
            std::cout << "Markdown report written to: " << markdown_output_path << "\n";
        } else {
            std::cerr << "Warning: Could not open " << markdown_output_path << " for writing.\n";
        }
    }

    // Write JSON Output
    if (!json_output_path.empty()) {
        std::ofstream json_file(json_output_path);
        if (json_file.is_open()) {
            json_file << "[\n";
            for (size_t i = 0; i < reports.size(); ++i) {
                json_file << reports[i].to_json();
                if (i + 1 < reports.size()) json_file << ",\n";
            }
            json_file << "\n]\n";
            std::cout << "JSON report written to: " << json_output_path << "\n";
        } else {
            std::cerr << "Warning: Could not open " << json_output_path << " for writing.\n";
        }
    }

    return (total_failed == 0) ? 0 : 1;
}

#if defined(__EMSCRIPTEN__)
extern "C" EMSCRIPTEN_KEEPALIVE int run_web_pipeline(int iterations, uint32_t seed, int presets) {
    std::vector<std::string> args = {
        "webgpu_parity_pipeline", presets ? "--presets" : "--all",
        "--iterations", std::to_string(iterations), "--seed", std::to_string(seed),
        "--json", "/webgpu-report.json", "--markdown", "/webgpu-report.md"
    };
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (std::string& arg : args) argv.push_back(arg.data());
    return run_pipeline(static_cast<int>(argv.size()), argv.data());
}
#else
int main(int argc, char** argv) {
    return run_pipeline(argc, argv);
}
#endif
