#include "parity_comparator.hpp"
#include "src/sim/warlock/spec_presets.hpp"
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace warlock {

static std::string format_float(double val, int precision = 1) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << val;
    return ss.str();
}

static std::string format_pct(double val, int precision = 1) {
    std::ostringstream ss;
    if (val >= 0.0) ss << "+";
    ss << std::fixed << std::setprecision(precision) << val << "%";
    return ss.str();
}

static ParityMetric make_metric(
    const std::string& name,
    const std::string& category,
    double cpu_val,
    double webgpu_val,
    double tol_pct = 1.5,
    double tol_abs = 0.5
) {
    ParityMetric m;
    m.name = name;
    m.category = category;
    m.cpu_val = cpu_val;
    m.webgpu_val = webgpu_val;
    m.delta = webgpu_val - cpu_val;
    m.tolerance_pct = tol_pct;
    m.tolerance_abs = tol_abs;
    
    if (std::abs(cpu_val) > 1e-4) {
        m.pct_diff = (m.delta / cpu_val) * 100.0;
    } else {
        m.pct_diff = 0.0;
    }

    // Check pass/fail: within relative % tolerance OR within absolute tolerance
    if (std::abs(m.pct_diff) <= tol_pct || std::abs(m.delta) <= tol_abs) {
        m.passed = true;
    } else {
        m.passed = false;
    }
    return m;
}

ParityReport ParityComparator::compare(
    const std::string& name,
    const std::string& description,
    const WarlockSimulator& sim,
    int iterations,
    uint32_t seed,
    double dps_tolerance_pct,
    double breakdown_tolerance_pct
) {
    ParityReport report;
    report.scenario_name = name;
    report.description = description;
    report.iterations = iterations;
    report.seed = seed;
    report.duration = sim.fight_duration;

    // 1. Run Ground Truth: CPU DES Engine
    report.cpu_result = ParallelSimRunner::run_batch(sim, iterations, 0, nullptr, seed);

    // 2. Run Under Test: WebGPU Engine
    report.webgpu_result = WebGPUSimRunner::run_batch(sim, iterations, 0, seed);

    // 3. Compute Top-Level DPS Metrics
    report.mean_dps_metric = make_metric("Mean DPS", "Summary", report.cpu_result.mean_dps, report.webgpu_result.mean_dps, dps_tolerance_pct, 1.0);
    report.std_dev_dps_metric = make_metric("Std Dev DPS", "Summary", report.cpu_result.std_dev_dps, report.webgpu_result.std_dev_dps, 15.0, 5.0);
    report.median_dps_metric = make_metric("Median (p50) DPS", "Summary", report.cpu_result.p50_dps, report.webgpu_result.p50_dps, dps_tolerance_pct, 1.5);
    report.p5_dps_metric = make_metric("p5 DPS", "Summary", report.cpu_result.p5_dps, report.webgpu_result.p5_dps, dps_tolerance_pct * 1.5, 2.0);
    report.p95_dps_metric = make_metric("p95 DPS", "Summary", report.cpu_result.p95_dps, report.webgpu_result.p95_dps, dps_tolerance_pct * 1.5, 2.0);
    report.isb_uptime_metric = make_metric("ISB Uptime %", "Buffs/Auras", report.cpu_result.mean_isb_uptime, report.webgpu_result.mean_isb_uptime, 5.0, 2.0);
    report.life_taps_metric = make_metric("Mean Life Taps", "Resource", report.cpu_result.mean_life_taps, report.webgpu_result.mean_life_taps, 5.0, 0.5);
    report.crit_rate_metric = make_metric("Total Crit %", "Combat Table", report.cpu_result.crit_percent, report.webgpu_result.crit_percent, 5.0, 1.0);
    report.miss_rate_metric = make_metric("Total Miss %", "Combat Table", report.cpu_result.miss_percent, report.webgpu_result.miss_percent, 5.0, 0.5);

    // 4. Compute Damage Breakdown Metrics
    auto add_breakdown = [&](const std::string& spell_name, double cpu_pct, double gpu_pct) {
        if (cpu_pct > 0.05 || gpu_pct > 0.05) {
            report.breakdown_metrics.push_back(make_metric(spell_name, "Damage Breakdown %", cpu_pct, gpu_pct, breakdown_tolerance_pct, 0.8));
        }
    };

    add_breakdown("Shadow Bolt", report.cpu_result.pct_shadow_bolt, report.webgpu_result.pct_shadow_bolt);
    add_breakdown("Soul Fire", report.cpu_result.pct_soul_fire, report.webgpu_result.pct_soul_fire);
    add_breakdown("Corruption", report.cpu_result.pct_corruption, report.webgpu_result.pct_corruption);
    add_breakdown("Curse of Agony", report.cpu_result.pct_agony, report.webgpu_result.pct_agony);
    add_breakdown("Curse of Doom", report.cpu_result.pct_doom, report.webgpu_result.pct_doom);
    add_breakdown("Siphon Life", report.cpu_result.pct_siphon_life, report.webgpu_result.pct_siphon_life);
    add_breakdown("Wrack", report.cpu_result.pct_drain_hope, report.webgpu_result.pct_drain_hope);
    add_breakdown("Immolate", report.cpu_result.pct_immolate, report.webgpu_result.pct_immolate);
    add_breakdown("Incinerate", report.cpu_result.pct_incinerate, report.webgpu_result.pct_incinerate);
    add_breakdown("Conflagrate", report.cpu_result.pct_conflagrate, report.webgpu_result.pct_conflagrate);
    add_breakdown("Shadowburn", report.cpu_result.pct_shadowburn, report.webgpu_result.pct_shadowburn);
    add_breakdown("Touch of the Grave", report.cpu_result.pct_touch_of_the_grave, report.webgpu_result.pct_touch_of_the_grave);
    add_breakdown("Total Pet", report.cpu_result.pct_pet, report.webgpu_result.pct_pet);
    add_breakdown("Pet Firebolt", report.cpu_result.pct_pet_firebolt, report.webgpu_result.pct_pet_firebolt);
    add_breakdown("Pet Melee", report.cpu_result.pct_pet_melee, report.webgpu_result.pct_pet_melee);
    add_breakdown("Pet Lash of Pain", report.cpu_result.pct_pet_lash_of_pain, report.webgpu_result.pct_pet_lash_of_pain);

    // 5. Compute Detailed Per-Spell Statistics
    auto analyze_spell = [&](SpellID id, const std::string& sname) {
        const auto& cs = report.cpu_result.spell_stats[static_cast<size_t>(id)];
        const auto& gs = report.webgpu_result.spell_stats[static_cast<size_t>(id)];

        if (cs.mean_casts < 0.05 && gs.mean_casts < 0.05 && cs.mean_damage < 0.5 && gs.mean_damage < 0.5) {
            return;
        }

        SpellParityDiff sd;
        sd.spell_id = id;
        sd.spell_name = sname;

        sd.cpu_dps = report.duration > 0.0 ? cs.mean_damage / report.duration : 0.0;
        sd.webgpu_dps = report.duration > 0.0 ? gs.mean_damage / report.duration : 0.0;
        sd.dps_delta = sd.webgpu_dps - sd.cpu_dps;
        sd.dps_pct_diff = (sd.cpu_dps > 0.01) ? (sd.dps_delta / sd.cpu_dps) * 100.0 : 0.0;

        double cpu_tot_dmg = report.cpu_result.mean_dps * report.duration;
        double gpu_tot_dmg = report.webgpu_result.mean_dps * report.duration;
        sd.cpu_share_pct = (cpu_tot_dmg > 0.0) ? (cs.mean_damage / cpu_tot_dmg) * 100.0 : 0.0;
        sd.webgpu_share_pct = (gpu_tot_dmg > 0.0) ? (gs.mean_damage / gpu_tot_dmg) * 100.0 : 0.0;

        sd.cpu_casts = cs.mean_casts;
        sd.webgpu_casts = gs.mean_casts;
        sd.cpu_hits = cs.mean_hits;
        sd.webgpu_hits = gs.mean_hits;
        sd.cpu_crits = cs.mean_crits;
        sd.webgpu_crits = gs.mean_crits;
        sd.cpu_misses = cs.mean_misses;
        sd.webgpu_misses = gs.mean_misses;

        sd.cpu_avg_hit = spell_avg_hit(cs);
        sd.webgpu_avg_hit = spell_avg_hit(gs);
        sd.avg_hit_pct_diff = (sd.cpu_avg_hit > 0.01) ? ((sd.webgpu_avg_hit - sd.cpu_avg_hit) / sd.cpu_avg_hit) * 100.0 : 0.0;

        sd.cpu_crit_pct = spell_crit_pct(cs);
        sd.webgpu_crit_pct = spell_crit_pct(gs);
        sd.cpu_miss_pct = spell_miss_pct(cs);
        sd.webgpu_miss_pct = spell_miss_pct(gs);

        // Spell-level pass criteria
        bool dps_ok = std::abs(sd.dps_pct_diff) <= (dps_tolerance_pct * 1.5) || std::abs(sd.dps_delta) <= 1.0;
        bool avg_hit_ok = std::abs(sd.avg_hit_pct_diff) <= (dps_tolerance_pct * 1.5) || std::abs(sd.webgpu_avg_hit - sd.cpu_avg_hit) <= 2.0;
        bool casts_ok = std::abs(sd.webgpu_casts - sd.cpu_casts) <= std::max(1.5, sd.cpu_casts * 0.08);
        if (sd.cpu_share_pct < 3.0 && sd.webgpu_share_pct < 3.0) {
            avg_hit_ok = true;
            casts_ok = true;
        }
        sd.passed = dps_ok && avg_hit_ok && casts_ok;

        if (!sd.passed) {
            if (!casts_ok) {
                sd.diagnostics.push_back("Cast count mismatch: CPU=" + format_float(sd.cpu_casts, 2) +
                                         " vs WebGPU=" + format_float(sd.webgpu_casts, 2) +
                                         " (Check GCD/cast time/haste/clipping logic)");
            }
            if (!avg_hit_ok) {
                sd.diagnostics.push_back("Damage per hit mismatch: CPU=" + format_float(sd.cpu_avg_hit, 1) +
                                         " vs WebGPU=" + format_float(sd.webgpu_avg_hit, 1) + " (" +
                                         format_pct(sd.avg_hit_pct_diff) +
                                         ") (Check base damage/SP coeff/talents/school multiplier)");
            }
            if (std::abs(sd.webgpu_crit_pct - sd.cpu_crit_pct) > 2.0) {
                sd.diagnostics.push_back("Crit rate mismatch: CPU=" + format_float(sd.cpu_crit_pct, 1) +
                                         "% vs WebGPU=" + format_float(sd.webgpu_crit_pct, 1) + "%");
            }
            if (std::abs(sd.webgpu_miss_pct - sd.cpu_miss_pct) > 1.5) {
                sd.diagnostics.push_back("Miss rate mismatch: CPU=" + format_float(sd.cpu_miss_pct, 1) +
                                         "% vs WebGPU=" + format_float(sd.webgpu_miss_pct, 1) + "%");
            }
        }

        report.spell_diffs.push_back(sd);
    };

    analyze_spell(SpellID::SHADOW_BOLT, "Shadow Bolt");
    analyze_spell(SpellID::SOUL_FIRE, "Soul Fire");
    analyze_spell(SpellID::CORRUPTION, "Corruption");
    analyze_spell(SpellID::CURSE_OF_AGONY, "Curse of Agony");
    analyze_spell(SpellID::CURSE_OF_DOOM, "Curse of Doom");
    analyze_spell(SpellID::SIPHON_LIFE, "Siphon Life");
    analyze_spell(SpellID::DRAIN_HOPE, "Wrack");
    analyze_spell(SpellID::IMMOLATE, "Immolate");
    analyze_spell(SpellID::INCINERATE, "Incinerate");
    analyze_spell(SpellID::CONFLAGRATE, "Conflagrate");
    analyze_spell(SpellID::SHADOWBURN, "Shadowburn");
    analyze_spell(SpellID::SEARING_PAIN, "Searing Pain");
    analyze_spell(SpellID::TOUCH_OF_THE_GRAVE, "Touch of the Grave");
    analyze_spell(SpellID::PET_FIREBOLT, "Pet: Firebolt");
    analyze_spell(SpellID::PET_MELEE, "Pet: Melee");
    analyze_spell(SpellID::PET_LASH_OF_PAIN, "Pet: Lash of Pain");

    // 6. Overall Pass/Fail & Root-Cause Analysis
    report.overall_passed = report.mean_dps_metric.passed;
    report.max_discrepancy_pct = std::abs(report.mean_dps_metric.pct_diff);

    for (const auto& bm : report.breakdown_metrics) {
        if (!bm.passed) {
            report.overall_passed = false;
        }
        report.max_discrepancy_pct = std::max(report.max_discrepancy_pct, std::abs(bm.pct_diff));
    }

    for (const auto& sd : report.spell_diffs) {
        if (!sd.passed) {
            report.overall_passed = false;
            for (const auto& diag : sd.diagnostics) {
                report.root_cause_diagnostics.push_back("[" + sd.spell_name + "] " + diag);
            }
        }
    }

    if (!report.life_taps_metric.passed) {
        report.root_cause_diagnostics.push_back("[Life Tap] Tap count differs: CPU=" +
            format_float(report.life_taps_metric.cpu_val, 2) + " vs WebGPU=" +
            format_float(report.life_taps_metric.webgpu_val, 2) +
            " (Check Life Tap mana gain formula or tap threshold)");
    }

    if (!report.isb_uptime_metric.passed && sim.talents.destro.improved_shadow_bolt > 0) {
        report.root_cause_diagnostics.push_back("[ISB] ISB Uptime differs: CPU=" +
            format_float(report.isb_uptime_metric.cpu_val, 1) + "% vs WebGPU=" +
            format_float(report.isb_uptime_metric.webgpu_val, 1) + "%");
    }

    return report;
}

std::vector<ParityReport> ParityComparator::run_suite(
    const std::vector<ParityScenario>& scenarios,
    int iterations,
    uint32_t seed
) {
    std::vector<ParityReport> reports;
    reports.reserve(scenarios.size());
    for (const auto& sc : scenarios) {
        reports.push_back(compare(
            sc.name,
            sc.description,
            sc.sim_config,
            iterations,
            seed,
            sc.dps_tolerance_pct,
            sc.breakdown_tolerance_pct
        ));
    }
    return reports;
}

std::string ParityReport::to_markdown() const {
    std::ostringstream ss;
    ss << "### Scenario: " << scenario_name << "\n";
    ss << "**Description**: " << description << "  \n";
    ss << "**Iterations**: " << iterations << " | **Duration**: " << duration << "s | **Status**: "
       << (overall_passed ? "✅ **PARITY PASS**" : "❌ **PARITY FAIL**") << "\n\n";

    // Summary Table
    ss << "| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |\n";
    ss << "| :--- | :---: | :---: | :---: | :---: | :---: |\n";
    
    auto row = [&](const ParityMetric& m, const std::string& unit = "") {
        ss << "| " << m.name << " | " << format_float(m.cpu_val, 1) << unit << " | "
           << format_float(m.webgpu_val, 1) << unit << " | " << format_float(m.delta, 1)
           << " | " << format_pct(m.pct_diff, 2) << " | "
           << (m.passed ? "✅ PASS" : "❌ FAIL") << " |\n";
    };

    row(mean_dps_metric, " DPS");
    row(median_dps_metric, " DPS");
    row(std_dev_dps_metric, " DPS");
    row(p5_dps_metric, " DPS");
    row(p95_dps_metric, " DPS");
    row(isb_uptime_metric, "%");
    row(life_taps_metric);
    row(crit_rate_metric, "%");
    row(miss_rate_metric, "%");
    ss << "\n";

    // Damage Breakdown Table
    if (!breakdown_metrics.empty()) {
        ss << "#### Damage Breakdown\n\n";
        ss << "| Spell | CPU Share | WebGPU Share | Delta | Status |\n";
        ss << "| :--- | :---: | :---: | :---: | :---: |\n";
        for (const auto& bm : breakdown_metrics) {
            ss << "| " << bm.name << " | " << format_float(bm.cpu_val, 1) << "% | "
               << format_float(bm.webgpu_val, 1) << "% | " << format_float(bm.delta, 1)
               << "% | " << (bm.passed ? "✅ PASS" : "❌ FAIL") << " |\n";
        }
        ss << "\n";
    }

    // Detailed Spell Stats Table
    if (!spell_diffs.empty()) {
        ss << "#### Detailed Spell Performance\n\n";
        ss << "| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |\n";
        ss << "| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |\n";
        for (const auto& sd : spell_diffs) {
            ss << "| " << sd.spell_name << " | " << format_float(sd.cpu_dps, 1) << " | "
               << format_float(sd.webgpu_dps, 1) << " | " << format_float(sd.cpu_casts, 1)
               << " | " << format_float(sd.webgpu_casts, 1) << " | "
               << format_float(sd.cpu_avg_hit, 1) << " | " << format_float(sd.webgpu_avg_hit, 1)
               << " | " << format_float(sd.cpu_crit_pct, 1) << "% | "
               << format_float(sd.webgpu_crit_pct, 1) << "% | "
               << (sd.passed ? "✅ PASS" : "❌ FAIL") << " |\n";
        }
        ss << "\n";
    }

    // Root Cause Diagnostics
    if (!root_cause_diagnostics.empty()) {
        ss << "#### 🔍 Root-Cause Diagnostics & Discrepancies\n\n";
        for (const auto& diag : root_cause_diagnostics) {
            ss << "- ⚠️ " << diag << "\n";
        }
        ss << "\n";
    }

    return ss.str();
}

std::string ParityReport::to_json() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"scenario\": \"" << scenario_name << "\",\n";
    ss << "  \"description\": \"" << description << "\",\n";
    ss << "  \"iterations\": " << iterations << ",\n";
    ss << "  \"duration\": " << duration << ",\n";
    ss << "  \"passed\": " << (overall_passed ? "true" : "false") << ",\n";
    ss << "  \"max_discrepancy_pct\": " << max_discrepancy_pct << ",\n";
    ss << "  \"summary\": {\n";
    ss << "    \"cpu_mean_dps\": " << cpu_result.mean_dps << ",\n";
    ss << "    \"webgpu_mean_dps\": " << webgpu_result.mean_dps << ",\n";
    ss << "    \"dps_delta\": " << mean_dps_metric.delta << ",\n";
    ss << "    \"dps_pct_diff\": " << mean_dps_metric.pct_diff << ",\n";
    ss << "    \"cpu_median_dps\": " << cpu_result.p50_dps << ",\n";
    ss << "    \"webgpu_median_dps\": " << webgpu_result.p50_dps << ",\n";
    ss << "    \"cpu_std_dev\": " << cpu_result.std_dev_dps << ",\n";
    ss << "    \"webgpu_std_dev\": " << webgpu_result.std_dev_dps << ",\n";
    ss << "    \"cpu_isb_uptime\": " << cpu_result.mean_isb_uptime << ",\n";
    ss << "    \"webgpu_isb_uptime\": " << webgpu_result.mean_isb_uptime << ",\n";
    ss << "    \"cpu_life_taps\": " << cpu_result.mean_life_taps << ",\n";
    ss << "    \"webgpu_life_taps\": " << webgpu_result.mean_life_taps << "\n";
    ss << "  },\n";

    ss << "  \"breakdowns\": [\n";
    for (size_t i = 0; i < breakdown_metrics.size(); ++i) {
        const auto& bm = breakdown_metrics[i];
        ss << "    {\"spell\": \"" << bm.name << "\", \"cpu_pct\": " << bm.cpu_val
           << ", \"webgpu_pct\": " << bm.webgpu_val << ", \"passed\": "
           << (bm.passed ? "true" : "false") << "}"
           << (i + 1 < breakdown_metrics.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    ss << "  \"spells\": [\n";
    for (size_t i = 0; i < spell_diffs.size(); ++i) {
        const auto& sd = spell_diffs[i];
        ss << "    {\n";
        ss << "      \"name\": \"" << sd.spell_name << "\",\n";
        ss << "      \"cpu_dps\": " << sd.cpu_dps << ",\n";
        ss << "      \"webgpu_dps\": " << sd.webgpu_dps << ",\n";
        ss << "      \"cpu_casts\": " << sd.cpu_casts << ",\n";
        ss << "      \"webgpu_casts\": " << sd.webgpu_casts << ",\n";
        ss << "      \"cpu_avg_hit\": " << sd.cpu_avg_hit << ",\n";
        ss << "      \"webgpu_avg_hit\": " << sd.webgpu_avg_hit << ",\n";
        ss << "      \"cpu_crit_pct\": " << sd.cpu_crit_pct << ",\n";
        ss << "      \"webgpu_crit_pct\": " << sd.webgpu_crit_pct << ",\n";
        ss << "      \"passed\": " << (sd.passed ? "true" : "false") << "\n";
        ss << "    }" << (i + 1 < spell_diffs.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    ss << "  \"diagnostics\": [\n";
    for (size_t i = 0; i < root_cause_diagnostics.size(); ++i) {
        ss << "    \"" << root_cause_diagnostics[i] << "\""
           << (i + 1 < root_cause_diagnostics.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

void ParityReport::print_summary(std::ostream& os, bool verbose) const {
    os << "======================================================================\n";
    os << " SCENARIO: " << scenario_name << "\n";
    os << " " << description << "\n";
    os << " WebGPU execution: " << (webgpu_result.gpu_used ? "native GPU" : "CPU fallback") << "\n";
    os << " Status: " << (overall_passed ? "[PASS]" : "[FAIL]")
       << " | DPS Diff: " << format_pct(mean_dps_metric.pct_diff, 2)
       << " (" << format_float(mean_dps_metric.delta, 1) << " DPS)\n";
    os << "----------------------------------------------------------------------\n";
    os << std::left << std::setw(20) << "Metric"
       << std::right << std::setw(14) << "CPU (DES)"
       << std::setw(14) << "WebGPU"
       << std::setw(12) << "Delta"
       << std::setw(12) << "% Diff"
       << std::setw(10) << "Status" << "\n";
    os << "----------------------------------------------------------------------\n";

    auto print_line = [&](const ParityMetric& m, const std::string& suffix = "") {
        os << std::left << std::setw(20) << m.name
           << std::right << std::setw(14) << (format_float(m.cpu_val, 1) + suffix)
           << std::setw(14) << (format_float(m.webgpu_val, 1) + suffix)
           << std::setw(12) << format_float(m.delta, 1)
           << std::setw(12) << format_pct(m.pct_diff, 2)
           << std::setw(10) << (m.passed ? "PASS" : "FAIL") << "\n";
    };

    print_line(mean_dps_metric, " DPS");
    print_line(median_dps_metric, " DPS");
    print_line(std_dev_dps_metric, " DPS");
    print_line(isb_uptime_metric, "%");
    print_line(life_taps_metric);
    print_line(crit_rate_metric, "%");
    print_line(miss_rate_metric, "%");

    if (verbose) {
        os << "\n-- Damage Breakdowns & Spell Stats --\n";
        os << std::left << std::setw(18) << "Spell"
           << std::right << std::setw(10) << "CPU DPS"
           << std::setw(10) << "GPU DPS"
           << std::setw(10) << "CPU Cast"
           << std::setw(10) << "GPU Cast"
           << std::setw(10) << "CPU Hit"
           << std::setw(10) << "GPU Hit"
           << std::setw(8) << "Crit%"
           << std::setw(8) << "Status" << "\n";
        os << "----------------------------------------------------------------------\n";

        for (const auto& sd : spell_diffs) {
            os << std::left << std::setw(18) << sd.spell_name
               << std::right << std::setw(10) << format_float(sd.cpu_dps, 1)
               << std::setw(10) << format_float(sd.webgpu_dps, 1)
               << std::setw(10) << format_float(sd.cpu_casts, 1)
               << std::setw(10) << format_float(sd.webgpu_casts, 1)
               << std::setw(10) << format_float(sd.cpu_avg_hit, 1)
               << std::setw(10) << format_float(sd.webgpu_avg_hit, 1)
               << std::setw(7) << format_float(sd.cpu_crit_pct, 0) << "%"
               << std::setw(8) << (sd.passed ? "PASS" : "FAIL") << "\n";
        }
    }

    if (!root_cause_diagnostics.empty()) {
        os << "\n-- Root Cause Diagnostics --\n";
        for (const auto& diag : root_cause_diagnostics) {
            os << "  * " << diag << "\n";
        }
    }
    os << "======================================================================\n\n";
}

// ---------------------------------------------------------------------------
// Standard Parity Matrix Suites
// ---------------------------------------------------------------------------

std::vector<ParityScenario> ParityComparator::get_standard_suite() {
    std::vector<ParityScenario> suite;

    // 1. Pure Shadow Bolt baseline
    {
        ParityScenario sc;
        sc.name = "pure_sb_baseline";
        sc.category = "Direct Spells";
        sc.description = "Pure Shadow Bolt filler with 500 SP, 12% Hit, 15% Crit (180s)";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 500.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 15.0;
        sc.sim_config.raw_stats.mp5 = 20.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }

    // 2. ISB Classic 4-Charge Debuff
    {
        ParityScenario sc;
        sc.name = "isb_classic_charges";
        sc.category = "Mechanics";
        sc.description = "5/5 Improved Shadow Bolt with Classic 4-Charge consumption";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 600.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 25.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        sc.sim_config.talents.destro.improved_shadow_bolt = 5;
        sc.sim_config.mechanics.isb_has_charges = true;
        suite.push_back(sc);
    }

    // 3. ISB WoW Forever 12s Chargeless Window
    {
        ParityScenario sc;
        sc.name = "isb_forever_window";
        sc.category = "Mechanics";
        sc.description = "5/5 Improved Shadow Bolt with WoW Forever 12s chargeless debuff window";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 600.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 25.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        sc.sim_config.talents.destro.improved_shadow_bolt = 5;
        sc.sim_config.mechanics.isb_has_charges = false;
        suite.push_back(sc);
    }

    // 4. Corruption & Nightfall Proc Weaving
    {
        ParityScenario sc;
        sc.name = "corruption_nightfall";
        sc.category = "DoTs & Procs";
        sc.description = "Maintained Corruption with 2/2 Nightfall Shadow Trance instant procs";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 550.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 10.0;
        sc.sim_config.raw_stats.spell_crit_percent = 15.0;
        sc.sim_config.policy.rotation = RotationChoice::SM_RUIN;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.curse = CurseChoice::NONE;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.aff.improved_corruption = 5;
        sc.sim_config.talents.aff.nightfall = 2;
        sc.sim_config.talents.aff.shadow_mastery = 5;
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }

    // 5. Full Multi-DoT APL (Agony + Corruption + Siphon Life + Immolate)
    {
        ParityScenario sc;
        sc.name = "full_multidot_affliction";
        sc.category = "APLs";
        sc.description = "Full Multi-DoT APL: Bane of Agony (ramp ticks) + Corruption + Siphon Life + Immolate";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 650.0;
        sc.sim_config.raw_stats.max_mana = 7000.0;
        sc.sim_config.raw_stats.spell_hit_percent = 14.0;
        sc.sim_config.raw_stats.spell_crit_percent = 20.0;
        sc.sim_config.policy.rotation = RotationChoice::AFFLICTION_HYBRID_DOTS;
        sc.sim_config.policy.curse = CurseChoice::BANE_OF_AGONY;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.maintain_immolate = true;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.aff.siphon_life = 1;
        sc.sim_config.talents.aff.improved_bane_of_agony = 2;
        sc.sim_config.talents.aff.improved_corruption = 5;
        sc.sim_config.talents.aff.shadow_mastery = 5;
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }

    // 6. Curse of Doom Burst APL
    {
        ParityScenario sc;
        sc.name = "curse_of_doom_burst";
        sc.category = "APLs";
        sc.description = "Bane of Doom (60s delayed burst) + Corruption + Shadow Bolt";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 600.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 18.0;
        sc.sim_config.policy.rotation = RotationChoice::SM_RUIN;
        sc.sim_config.policy.curse = CurseChoice::BANE_OF_DOOM;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.maintain_immolate = false;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.aff.shadow_mastery = 5;
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }

    // 7. Active Imp with Firebolt Scaling
    {
        ParityScenario sc;
        sc.name = "pet_active_imp";
        sc.category = "Pets";
        sc.description = "Active Imp Pet with modern Firebolt scaling and +30% Improved Imp talent";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::ORC;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 500.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 10.0;
        sc.sim_config.raw_stats.spell_crit_percent = 15.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::IMP;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        sc.sim_config.talents.demo.improved_imp = 3;
        sc.sim_config.talents.demo.unholy_power = 5;
        suite.push_back(sc);
    }

    // 8. Active Succubus with Melee and Lash of Pain
    {
        ParityScenario sc;
        sc.name = "pet_active_succubus";
        sc.category = "Pets";
        sc.description = "Active Succubus with Melee swings, Lash of Pain shadow nuke, and armor reduction";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 550.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 10.0;
        sc.sim_config.raw_stats.spell_crit_percent = 15.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::SUCCUBUS;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        sc.sim_config.talents.demo.improved_sayaad = 2;
        sc.sim_config.talents.demo.unholy_power = 5;
        suite.push_back(sc);
    }

    // 9. Demonic Sacrifice: Succubus (+15% Shadow Damage)
    {
        ParityScenario sc;
        sc.name = "demonic_sacrifice_succubus";
        sc.category = "Buffs & Sacrifices";
        sc.description = "Demonic Sacrifice: Succubus granting flat +15% Shadow Damage";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 500.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 15.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.buffs.sacrifice_succubus = true;
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.demo.demonic_sacrifice = 1;
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }

    // 10. Long Mana-Stress Fight (600s with frequent Life Taps)
    {
        ParityScenario sc;
        sc.name = "mana_stress_600s";
        sc.category = "Endurance & Mana";
        sc.description = "600s prolonged endurance fight testing Life Tap weaving & MP5 regeneration";
        sc.sim_config.fight_duration = 600.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 600.0;
        sc.sim_config.raw_stats.max_mana = 5500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 15.0;
        sc.sim_config.raw_stats.mp5 = 30.0;
        sc.sim_config.policy.rotation = RotationChoice::SM_RUIN;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.curse = CurseChoice::BANE_OF_AGONY;
        sc.sim_config.policy.life_tap_threshold_pct = 30.0;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.aff.improved_life_tap = 2;
        sc.sim_config.talents.aff.shadow_mastery = 5;
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }

    // 11. Undead Racial: Touch of the Grave
    {
        ParityScenario sc;
        sc.name = "undead_touch_of_the_grave";
        sc.category = "Racials";
        sc.description = "Undead racial Touch of the Grave (10% proc chance on direct spells/ticks, 1s ICD)";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::UNDEAD;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 600.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 20.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        sc.dps_tolerance_pct = 2.0;
        suite.push_back(sc);
    }

    // 12. Orc Racial: Blood Fury
    {
        ParityScenario sc;
        sc.name = "orc_blood_fury";
        sc.category = "Racials";
        sc.description = "Orc racial Blood Fury (+10% base spell power for 15s on 120s cooldown)";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::ORC;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 600.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 20.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.racial_policy = RacialPolicy::ON_COOLDOWN;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.dps_tolerance_pct = 2.0;
        suite.push_back(sc);
    }

    // 13. Troll Racial: Berserking
    {
        ParityScenario sc;
        sc.name = "troll_berserking";
        sc.category = "Racials";
        sc.description = "Troll racial Berserking (+10% spell haste for 10s on 180s cooldown)";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::TROLL;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 600.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 20.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.racial_policy = RacialPolicy::ON_COOLDOWN;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        sc.dps_tolerance_pct = 2.0;
        suite.push_back(sc);
    }

    // 14. Gnome Racial: Expansive Mind & Eureka!
    {
        ParityScenario sc;
        sc.name = "gnome_expansive_mind";
        sc.category = "Racials";
        sc.description = "Gnome racial Expansive Mind (+5% Mana) & Eureka! (3 charges of -10% Mana / +10% Dmg)";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::GNOME;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = 600.0;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 20.0;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.racial_policy = RacialPolicy::ON_COOLDOWN;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        sc.dps_tolerance_pct = 2.0;
        suite.push_back(sc);
    }

    return suite;
}




std::vector<ParityScenario> ParityComparator::get_spell_power_sweep(double hit, double crit) {
    std::vector<ParityScenario> suite;
    const std::vector<double> sp_values = { 0.0, 250.0, 500.0, 750.0, 1000.0, 1300.0 };
    for (double sp : sp_values) {
        ParityScenario sc;
        sc.name = "sp_sweep_" + std::to_string(static_cast<int>(sp));
        sc.category = "Spell Power Sweep";
        sc.description = "Pure Shadow Bolt scaling at " + std::to_string(static_cast<int>(sp)) + " Spell Power";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = sp;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = hit;
        sc.sim_config.raw_stats.spell_crit_percent = crit;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }
    return suite;
}

std::vector<ParityScenario> ParityComparator::get_hit_sweep(double sp, double crit) {
    std::vector<ParityScenario> suite;
    const std::vector<double> hit_values = { 0.0, 4.0, 8.0, 12.0, 16.0 };
    for (double hit : hit_values) {
        ParityScenario sc;
        sc.name = "hit_sweep_" + std::to_string(static_cast<int>(hit)) + "pct";
        sc.category = "Spell Hit Sweep";
        sc.description = "Pure Shadow Bolt at " + std::to_string(static_cast<int>(hit)) + "% Spell Hit";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = sp;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = hit;
        sc.sim_config.raw_stats.spell_crit_percent = crit;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }
    return suite;
}

std::vector<ParityScenario> ParityComparator::get_crit_sweep(double sp, double hit) {
    std::vector<ParityScenario> suite;
    const std::vector<double> crit_values = { 5.0, 15.0, 25.0, 35.0, 50.0 };
    for (double crit : crit_values) {
        ParityScenario sc;
        sc.name = "crit_sweep_" + std::to_string(static_cast<int>(crit)) + "pct";
        sc.category = "Spell Crit Sweep";
        sc.description = "Pure Shadow Bolt with Ruin at " + std::to_string(static_cast<int>(crit)) + "% Spell Crit";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = sp;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = hit;
        sc.sim_config.raw_stats.spell_crit_percent = crit;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }
    return suite;
}

std::vector<ParityScenario> ParityComparator::get_haste_sweep(double sp) {
    std::vector<ParityScenario> suite;
    const std::vector<double> haste_values = { 0.0, 10.0, 20.0, 30.0 };
    for (double haste : haste_values) {
        ParityScenario sc;
        sc.name = "haste_sweep_" + std::to_string(static_cast<int>(haste)) + "pct";
        sc.category = "Spell Haste Sweep";
        sc.description = "Pure Shadow Bolt with " + std::to_string(static_cast<int>(haste)) + "% Spell Haste";
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = sp;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 15.0;
        sc.sim_config.raw_stats.spell_haste_percent = haste;
        sc.sim_config.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sc.sim_config.policy.corruption = DotPolicy::NEVER;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }
    return suite;
}

std::vector<ParityScenario> ParityComparator::get_duration_sweep(double sp) {
    std::vector<ParityScenario> suite;
    const std::vector<double> durations = { 30.0, 60.0, 120.0, 180.0, 300.0, 600.0 };
    for (double d : durations) {
        ParityScenario sc;
        sc.name = "duration_sweep_" + std::to_string(static_cast<int>(d)) + "s";
        sc.category = "Fight Duration Sweep";
        sc.description = "SM/Ruin fight length variation at " + std::to_string(static_cast<int>(d)) + "s";
        sc.sim_config.fight_duration = d;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.use_raw_stats = true;
        sc.sim_config.raw_stats.spell_power = sp;
        sc.sim_config.raw_stats.max_mana = 6500.0;
        sc.sim_config.raw_stats.spell_hit_percent = 12.0;
        sc.sim_config.raw_stats.spell_crit_percent = 15.0;
        sc.sim_config.policy.rotation = RotationChoice::SM_RUIN;
        sc.sim_config.policy.corruption = DotPolicy::ALWAYS;
        sc.sim_config.policy.curse = CurseChoice::BANE_OF_AGONY;
        sc.sim_config.policy.pet = PetChoice::NONE;
        sc.sim_config.buffs = BuffConfig{};
        sc.sim_config.talents = Talents{};
        sc.sim_config.talents.aff.shadow_mastery = 5;
        sc.sim_config.talents.aff.improved_corruption = 5;
        sc.sim_config.talents.destro.bane = 5;
        sc.sim_config.talents.destro.ruin = 5;
        suite.push_back(sc);
    }
    return suite;
}

std::vector<ParityScenario> ParityComparator::get_spec_preset_suite() {
    std::vector<ParityScenario> suite;
    const auto& presets = standard_spec_presets();
    suite.reserve(presets.size());

    for (const auto& p : presets) {
        ParityScenario sc;
        sc.name = std::string("preset_") + p.id;
        sc.category = "Spec Presets";
        sc.description = p.display_name;
        sc.sim_config.fight_duration = 180.0;
        sc.sim_config.race = Race::HUMAN;
        sc.sim_config.gear = GearLoadout::create_preraid_bis();
        sc.sim_config.use_raw_stats = false;
        apply_spec_preset(sc.sim_config, p);
        suite.push_back(sc);
    }

    return suite;
}

} // namespace warlock
