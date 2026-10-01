#pragma once
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include "src/sim/warlock/warlock_sim.hpp"
#include "src/sim/warlock/parallel_runner.hpp"
#include "webgpu_sim_runner.hpp"

namespace warlock {

// Represents a comparison metric between CPU DES and WebGPU
struct ParityMetric {
    std::string name;
    std::string category;
    double cpu_val = 0.0;
    double webgpu_val = 0.0;
    double delta = 0.0;           // webgpu_val - cpu_val
    double pct_diff = 0.0;        // (webgpu_val - cpu_val) / cpu_val * 100.0
    double tolerance_pct = 1.5;   // Allowable relative % tolerance
    double tolerance_abs = 0.5;   // Allowable absolute delta tolerance (for small metrics or %)
    bool passed = true;
    std::string note;
};

// Granular spell-level statistics diff
struct SpellParityDiff {
    SpellID spell_id = SpellID::NONE;
    std::string spell_name;
    
    // DPS & Share
    double cpu_dps = 0.0;
    double webgpu_dps = 0.0;
    double dps_delta = 0.0;
    double dps_pct_diff = 0.0;
    double cpu_share_pct = 0.0;
    double webgpu_share_pct = 0.0;

    // Casts, Hits, Crits, Misses
    double cpu_casts = 0.0;
    double webgpu_casts = 0.0;
    double cpu_hits = 0.0;
    double webgpu_hits = 0.0;
    double cpu_crits = 0.0;
    double webgpu_crits = 0.0;
    double cpu_misses = 0.0;
    double webgpu_misses = 0.0;

    // Derived Spell Metrics
    double cpu_avg_hit = 0.0;
    double webgpu_avg_hit = 0.0;
    double avg_hit_pct_diff = 0.0;
    double cpu_crit_pct = 0.0;
    double webgpu_crit_pct = 0.0;
    double cpu_miss_pct = 0.0;
    double webgpu_miss_pct = 0.0;

    bool passed = true;
    std::vector<std::string> diagnostics;
};

// Complete parity report for a single simulation scenario
struct ParityReport {
    std::string scenario_name;
    std::string description;
    int iterations = 10000;
    uint32_t seed = 42;
    double duration = 180.0;

    BatchSimResult cpu_result;
    BatchSimResult webgpu_result;

    // Overall Metrics
    ParityMetric mean_dps_metric;
    ParityMetric std_dev_dps_metric;
    ParityMetric median_dps_metric;
    ParityMetric p5_dps_metric;
    ParityMetric p95_dps_metric;
    ParityMetric isb_uptime_metric;
    ParityMetric life_taps_metric;
    ParityMetric crit_rate_metric;
    ParityMetric miss_rate_metric;

    // Granular Breakdowns
    std::vector<ParityMetric> breakdown_metrics;
    std::vector<SpellParityDiff> spell_diffs;

    // Automated Root-Cause Diagnostics
    std::vector<std::string> root_cause_diagnostics;
    
    bool overall_passed = true;
    double max_discrepancy_pct = 0.0;

    std::string to_markdown() const;
    std::string to_json() const;
    void print_summary(std::ostream& os = std::cout, bool verbose = false) const;
};

// Test scenario specification
struct ParityScenario {
    std::string name;
    std::string category;
    std::string description;
    WarlockSimulator sim_config;
    double dps_tolerance_pct = 1.5;
    double breakdown_tolerance_pct = 2.0;
};

class ParityComparator {
public:
    // Compares CPU DES vs WebGPU for any arbitrary WarlockSimulator configuration
    static ParityReport compare(
        const std::string& name,
        const std::string& description,
        const WarlockSimulator& sim,
        int iterations = 10000,
        uint32_t seed = 42,
        double dps_tolerance_pct = 1.5,
        double breakdown_tolerance_pct = 2.0
    );

    // Runs a vector of scenarios and returns all reports
    static std::vector<ParityReport> run_suite(
        const std::vector<ParityScenario>& scenarios,
        int iterations = 10000,
        uint32_t seed = 42
    );

    // Returns standard comprehensive benchmark matrix
    static std::vector<ParityScenario> get_standard_suite();

    // Returns parameter sweep scenarios
    static std::vector<ParityScenario> get_spell_power_sweep(double hit = 12.0, double crit = 15.0);
    static std::vector<ParityScenario> get_hit_sweep(double sp = 500.0, double crit = 15.0);
    static std::vector<ParityScenario> get_crit_sweep(double sp = 500.0, double hit = 12.0);
    static std::vector<ParityScenario> get_haste_sweep(double sp = 500.0);
    static std::vector<ParityScenario> get_duration_sweep(double sp = 500.0);
    static std::vector<ParityScenario> get_spec_preset_suite();
};

} // namespace warlock
