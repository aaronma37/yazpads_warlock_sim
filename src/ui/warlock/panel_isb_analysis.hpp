#pragma once
#include "imgui.h"
#include "implot.h"
#include "src/sim/isb_analysis.hpp"
#include <cstdio>
#include <vector>

namespace warlock {

// Hit-rate lines plotted on the ISB uptime chart (83% = base vs a raid
// boss before +hit; higher lines add hit gear/talents).
struct IsbHitCurveDef {
    const char* label;
    double hit_chance;
    ImVec4 color;
};

inline const IsbHitCurveDef* isb_hit_curve_defs(int& out_count) {
    static const IsbHitCurveDef defs[] = {
        {"83% hit (base vs boss)", 0.83, ImVec4(0.75f, 0.75f, 0.80f, 1.0f)},
        {"89% hit (+6% hit)", 0.89, ImVec4(1.00f, 0.55f, 0.20f, 1.0f)},
        {"94% hit (+11% hit)", 0.94, ImVec4(0.30f, 0.95f, 0.50f, 1.0f)},
        {"99% hit (capped)", 0.99, ImVec4(0.30f, 0.85f, 1.00f, 1.0f)},
    };
    out_count = 4;
    return defs;
}

inline void render_panel_isb_analysis() {
    ImGui::TextColored(ImVec4(1.0f, 0.70f, 0.25f, 1.0f), "ISB Uptime Analysis:");
    ImGui::TextWrapped("Steady-state Improved Shadow Bolt uptime vs Shadow Bolt crit chance "
                       "(of landed bolts), one line per hit chance. Assumes continuous Shadow Bolt "
                       "spam with 5/5 Improved Shadow Bolt refreshing the default 12s chargeless window.");
    ImGui::Separator();

    static float cast_interval = 2.5f;
    ImGui::SetNextItemWidth(260);
    ImGui::SliderFloat("Sec per Shadow Bolt", &cast_interval, 2.0f, 3.0f, "%.2fs");
    ImGui::SameLine();
    ImGui::TextDisabled("2.5s = 5/5 Bane");

    ImGui::Spacing();

    int curve_count = 0;
    const IsbHitCurveDef* defs = isb_hit_curve_defs(curve_count);

    const int n = 41; // 0%..40% crit in 1pp steps
    std::vector<double> xs(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) xs[static_cast<size_t>(i)] = i * 1.0;

    if (ImPlot::BeginPlot("ISB Uptime vs Crit", ImVec2(-1, 300))) {
        ImPlot::SetupAxes("Shadow Bolt Crit Chance (%)", "ISB Uptime (%)");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 40, ImPlotCond_Once);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImPlotCond_Once);
        for (int c = 0; c < curve_count; ++c) {
            std::vector<double> ys(static_cast<size_t>(n));
            for (int i = 0; i < n; ++i) {
                ys[static_cast<size_t>(i)] =
                    isb_analysis::expected_uptime(xs[static_cast<size_t>(i)] / 100.0,
                                                  defs[c].hit_chance,
                                                  cast_interval) *
                    100.0;
            }
            ImPlot::PlotLine(defs[c].label, xs.data(), ys.data(), n,
                             {ImPlotProp_LineColor, defs[c].color,
                              ImPlotProp_LineWeight, 2.0f});
        }
        ImPlot::EndPlot();
    }

    // Summary table: uptime at 10/20/30% crit per hit line.
    if (ImGui::BeginTable("IsbUptimeSummaryTable", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Hit Chance", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Uptime @ 10% crit", ImGuiTableColumnFlags_WidthFixed, 130);
        ImGui::TableSetupColumn("Uptime @ 20% crit", ImGuiTableColumnFlags_WidthFixed, 130);
        ImGui::TableSetupColumn("Uptime @ 30% crit", ImGuiTableColumnFlags_WidthFixed, 130);
        ImGui::TableHeadersRow();
        for (int c = 0; c < curve_count; ++c) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(defs[c].color, "%s", defs[c].label);
            const double crits[3] = {0.10, 0.20, 0.30};
            for (int k = 0; k < 3; ++k) {
                ImGui::TableNextColumn();
                ImGui::Text("%.1f%%", isb_analysis::expected_uptime(
                                          crits[k], defs[c].hit_chance,
                                          cast_interval) *
                                      100.0);
            }
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Model: uptime = 1 - exp(-h*c*T_win/T_cast), T_win = 12s. "
                        "Mirrors the DES engine (miss rolled first, crit only on landed bolts, "
                        "each crit refreshes the window). Upper bound under Classic 4-charge rules, "
                        "where raid consumption can drain charges early.");
}

} // namespace warlock
