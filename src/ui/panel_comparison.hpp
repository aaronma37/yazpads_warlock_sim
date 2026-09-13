#pragma once
#include "imgui.h"
#include "implot.h"
#include "src/sim/optimizer.hpp"
#include <vector>
#include <string>

namespace warlock {

inline void render_panel_comparison(
    const WarlockSimulator& base_sim,
    std::vector<CandidateResult>& comparison_results,
    bool& is_comparing,
    float& compare_progress,
    std::string& current_task_name
) {
    ImGui::TextColored(ImVec4(0.8f, 0.7f, 1.0f, 1.0f), "Comparative Theorycrafting Analysis:");
    ImGui::TextWrapped("Run side-by-side comparisons of talent builds, phase gear sets, or toggle mechanics (such as snapshotting).");
    ImGui::Separator();

    if (is_comparing) ImGui::BeginDisabled();
    if (ImGui::Button("Compare Specs (DS/Ruin vs SM vs Fire vs MD)")) {
        is_comparing = true;
        compare_progress = 0.0f;
        comparison_results = Optimizer::optimize_talents(base_sim, 2500, [&](float p, const std::string& name) {
            compare_progress = p;
            current_task_name = name;
        });
        is_comparing = false;
        compare_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Compare Phase Gear (P1 -> P6)")) {
        is_comparing = true;
        compare_progress = 0.0f;
        comparison_results = Optimizer::optimize_gear(base_sim, 2500, [&](float p, const std::string& name) {
            compare_progress = p;
            current_task_name = name;
        });
        is_comparing = false;
        compare_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Compare Consumables (T0 -> T3)")) {
        is_comparing = true;
        compare_progress = 0.0f;
        comparison_results = Optimizer::compare_consumable_tiers(base_sim, 2500, [&](float p, const std::string& name) {
            compare_progress = p;
            current_task_name = name;
        });
        is_comparing = false;
        compare_progress = 1.0f;
    }

    ImGui::Spacing();
    if (ImGui::Button("Compare Stat Values (EP / Weights)")) {
        is_comparing = true;
        compare_progress = 0.0f;
        comparison_results = Optimizer::compare_stat_values(base_sim, 2500, [&](float p, const std::string& name) {
            compare_progress = p;
            current_task_name = name;
        });
        is_comparing = false;
        compare_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Combinatorial Talents (15 Specs)")) {
        is_comparing = true;
        compare_progress = 0.0f;
        comparison_results = Optimizer::explore_combinatorial_talents(base_sim, 2000, [&](float p, const std::string& name) {
            compare_progress = p;
            current_task_name = name;
        });
        is_comparing = false;
        compare_progress = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Compare Snapshotting (ON vs OFF)")) {
        is_comparing = true;
        compare_progress = 0.0f;
        comparison_results = Optimizer::evaluate_snapshotting_impact(base_sim, 2500, [&](float p, const std::string& name) {
            compare_progress = p;
            current_task_name = name;
        });
        is_comparing = false;
        compare_progress = 1.0f;
    }
    if (is_comparing) ImGui::EndDisabled();

    if (is_comparing) {
        ImGui::Text("Analyzing: %s...", current_task_name.c_str());
        ImGui::ProgressBar(compare_progress, ImVec2(-1, 6));
    }

    ImGui::Separator();

    if (!comparison_results.empty()) {
        // ImPlot Bar Chart comparing builds
        std::vector<double> xs(comparison_results.size());
        std::vector<double> ys(comparison_results.size());
        std::vector<double> errs(comparison_results.size());

        for (size_t i = 0; i < comparison_results.size(); ++i) {
            xs[i] = static_cast<double>(i);
            ys[i] = comparison_results[i].mean_dps;
            errs[i] = comparison_results[i].std_dev_dps;
        }

        if (ImPlot::BeginPlot("DPS Comparison Chart (Mean +/- StdDev)", ImVec2(-1, 260))) {
            ImPlot::SetupAxes("Configuration / Spec", "Mean DPS");
            ImPlot::SetupAxisTicks(ImAxis_X1, 0, static_cast<int>(comparison_results.size()) - 1, static_cast<int>(comparison_results.size()));
            ImPlot::PlotBars("Mean DPS", xs.data(), ys.data(), static_cast<int>(xs.size()), 0.5);
            ImPlot::PlotErrorBars("StdDev", xs.data(), ys.data(), errs.data(), static_cast<int>(xs.size()));
            ImPlot::EndPlot();
        }

        ImGui::Separator();

        // Comparison Data Table
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Detailed Comparison Table:");
        if (ImGui::BeginTable("ComparisonTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 45);
            ImGui::TableSetupColumn("Configuration Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Mean DPS", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("+/- StdDev", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Min - Max", ImGuiTableColumnFlags_WidthFixed, 110);
            ImGui::TableSetupColumn("ISB Uptime", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableHeadersRow();

            for (const auto& r : comparison_results) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (r.rank == 1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#%d [BiS]", r.rank);
                } else {
                    ImGui::Text("#%d", r.rank);
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", r.name.c_str());

                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.1f", r.mean_dps);

                ImGui::TableNextColumn();
                ImGui::TextDisabled("+/- %.1f", r.std_dev_dps);

                ImGui::TableNextColumn();
                ImGui::Text("%.0f - %.0f", r.min_dps, r.max_dps);

                ImGui::TableNextColumn();
                ImGui::Text("%.1f%%", r.isb_uptime);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::TextDisabled("Run one of the comparisons above to generate comparison graphs.");
    }
}

} // namespace warlock
