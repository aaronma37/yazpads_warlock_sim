#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"
#include "implot.h"
#include "src/sim/pet_analysis.hpp"
#include <vector>
#include <string>

namespace warlock {

// Talent configurations plotted on the pet damage charts.
struct PetCurveDef {
    const char* label;
    int unholy_power;
    int improved_aspect; // Improved Imp (Firebolt) or Improved Sayaad (Lash of Pain)
    ImVec4 color;
};

inline const PetCurveDef* imp_curve_defs(int& out_count) {
    static const PetCurveDef defs[] = {
        {"Base Imp", 0, 0, ImVec4(0.75f, 0.75f, 0.80f, 1.0f)},
        {"Unholy Power 5/5", 5, 0, ImVec4(0.70f, 0.40f, 1.00f, 1.0f)},
        {"Improved Imp 3/3", 0, 3, ImVec4(1.00f, 0.55f, 0.20f, 1.0f)},
        {"UP 5/5 + Imp Imp 3/3", 5, 3, ImVec4(0.30f, 0.95f, 0.50f, 1.0f)},
    };
    out_count = 4;
    return defs;
}

inline const PetCurveDef* succubus_curve_defs(int& out_count) {
    static const PetCurveDef defs[] = {
        {"Base Succubus", 0, 0, ImVec4(0.75f, 0.75f, 0.80f, 1.0f)},
        {"Unholy Power 5/5", 5, 0, ImVec4(0.70f, 0.40f, 1.00f, 1.0f)},
        {"Improved Sayaad 3/3", 0, 3, ImVec4(1.00f, 0.55f, 0.20f, 1.0f)},
        {"UP 5/5 + Sayaad 3/3", 5, 3, ImVec4(0.30f, 0.95f, 0.50f, 1.0f)},
    };
    out_count = 4;
    return defs;
}

inline void render_imp_dps_chart(const char* plot_id, bool mana_limited,
                                 double fight_duration, int sp_max) {
    int curve_count = 0;
    const PetCurveDef* defs = imp_curve_defs(curve_count);

    const int n = sp_max / 20 + 1;
    std::vector<double> xs(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) xs[static_cast<size_t>(i)] = i * 20.0;

    if (ImPlot::BeginPlot(plot_id, ImVec2(-1, 300))) {
        ImPlot::SetupAxes("Master Fire Spell Power", "Imp DPS");
        for (int c = 0; c < curve_count; ++c) {
            std::vector<double> ys(static_cast<size_t>(n));
            for (int i = 0; i < n; ++i) {
                ys[static_cast<size_t>(i)] = imp_analysis::expected_dps(
                    xs[static_cast<size_t>(i)], defs[c].unholy_power,
                    defs[c].improved_aspect, mana_limited, fight_duration);
            }
            ImPlot::PlotLine(defs[c].label, xs.data(), ys.data(), n,
                             {ImPlotProp_LineColor, defs[c].color,
                              ImPlotProp_LineWeight, 2.0f});
        }
        ImPlot::EndPlot();
    }

    // Summary table at 0 SP and max SP, plus casts over the fight and slope.
    if (ImGui::BeginTable("ImpDpsSummaryTable", 5,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Configuration", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Casts in Fight", ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableSetupColumn("DPS @ 0 SP", ImGuiTableColumnFlags_WidthFixed, 100);
        char max_header[64];
        std::snprintf(max_header, sizeof(max_header), "DPS @ %d SP", sp_max);
        ImGui::TableSetupColumn(max_header, ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableSetupColumn("DPS / 10 SP", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();
        for (int c = 0; c < curve_count; ++c) {
            const double dps_0 = imp_analysis::expected_dps(
                0.0, defs[c].unholy_power, defs[c].improved_aspect,
                mana_limited, fight_duration);
            const double dps_max = imp_analysis::expected_dps(
                sp_max, defs[c].unholy_power, defs[c].improved_aspect,
                mana_limited, fight_duration);
            // DPS is linear in master SP (mana only affects cast count),
            // so the slope is exact: DPS gained per 10 spell power.
            const double slope_per_10sp = sp_max > 0 ? (dps_max - dps_0) / sp_max * 10.0 : 0.0;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(defs[c].color, "%s", defs[c].label);
            ImGui::TableNextColumn();
            ImGui::Text("%d", imp_analysis::expected_cast_count(fight_duration, mana_limited));
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", dps_0);
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.1f", dps_max);
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "+%.2f", slope_per_10sp);
        }
        ImGui::EndTable();
    }
}

inline void render_succubus_dps_chart(const char* plot_id, bool mana_limited,
                                      double fight_duration, int sp_max) {
    int curve_count = 0;
    const PetCurveDef* defs = succubus_curve_defs(curve_count);

    const int n = sp_max / 20 + 1;
    std::vector<double> xs(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) xs[static_cast<size_t>(i)] = i * 20.0;

    if (ImPlot::BeginPlot(plot_id, ImVec2(-1, 300))) {
        ImPlot::SetupAxes("Master Shadow Spell Power", "Succubus DPS (Melee + Lash of Pain)");
        for (int c = 0; c < curve_count; ++c) {
            std::vector<double> ys(static_cast<size_t>(n));
            for (int i = 0; i < n; ++i) {
                ys[static_cast<size_t>(i)] = imp_analysis::succubus_expected_dps(
                    xs[static_cast<size_t>(i)], defs[c].unholy_power,
                    defs[c].improved_aspect, mana_limited, fight_duration);
            }
            ImPlot::PlotLine(defs[c].label, xs.data(), ys.data(), n,
                             {ImPlotProp_LineColor, defs[c].color,
                              ImPlotProp_LineWeight, 2.0f});
        }
        ImPlot::EndPlot();
    }

    if (ImGui::BeginTable("SuccubusDpsSummaryTable", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Configuration", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Lash Casts", ImGuiTableColumnFlags_WidthFixed, 90);
        ImGui::TableSetupColumn("Melee Swings", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("DPS @ 0 SP", ImGuiTableColumnFlags_WidthFixed, 100);
        char max_header[64];
        std::snprintf(max_header, sizeof(max_header), "DPS @ %d SP", sp_max);
        ImGui::TableSetupColumn(max_header, ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableSetupColumn("DPS / 10 SP", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();
        for (int c = 0; c < curve_count; ++c) {
            const double dps_0 = imp_analysis::succubus_expected_dps(
                0.0, defs[c].unholy_power, defs[c].improved_aspect,
                mana_limited, fight_duration);
            const double dps_max = imp_analysis::succubus_expected_dps(
                sp_max, defs[c].unholy_power, defs[c].improved_aspect,
                mana_limited, fight_duration);
            const double slope_per_10sp = sp_max > 0 ? (dps_max - dps_0) / sp_max * 10.0 : 0.0;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(defs[c].color, "%s", defs[c].label);
            ImGui::TableNextColumn();
            ImGui::Text("%d", imp_analysis::succubus_expected_lop_casts(
                                   fight_duration, defs[c].improved_aspect, mana_limited));
            ImGui::TableNextColumn();
            ImGui::Text("%d", imp_analysis::succubus_expected_melee_swings(fight_duration));
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", dps_0);
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.1f", dps_max);
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "+%.2f", slope_per_10sp);
        }
        ImGui::EndTable();
    }
}

inline void render_panel_imp_analysis(double fight_duration) {
    ImGui::TextColored(ImVec4(1.0f, 0.70f, 0.25f, 1.0f), "Imp Damage Analysis:");
    ImGui::TextWrapped("Expected pet DPS vs the master's spell power (15%% pet SP scaling). "
                       "Fight duration follows the Presets target encounter (currently %.0f seconds).",
                       fight_duration);
    ImGui::Separator();

    static int sp_max = 800;
    ImGui::Text("Max Master Spell Power:");
    ImGui::SetNextItemWidth(180);
    WowInputInt("##MaxMasterSpellPower", &sp_max, 50, 200);
    if (sp_max < 50) sp_max = 50;

    ImGui::Spacing();
    if (WowBeginTabBar("ImpAnalysisSubTabs", ImGuiTabBarFlags_None)) {
        if (WowBeginTabItem("  Mana-Starved (Base Regen)  ")) {
            ImGui::TextDisabled("Imp spends 115 mana per Firebolt from a 1150 pool; "
                                "Succubus spends 160 mana per Lash of Pain from a 1450 pool "
                                "(melee swings are free). Both recover 45 mana every 5s, no buffs.");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "Imp (Firebolt):");
            render_imp_dps_chart("Imp DPS - Mana-Starved", true, fight_duration, sp_max);
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "Succubus (Melee + Lash of Pain):");
            render_succubus_dps_chart("Succubus DPS - Mana-Starved", true, fight_duration, sp_max);
            WowEndTabItem();
        }
        if (WowBeginTabItem("  Infinite Mana  ")) {
            ImGui::TextDisabled("Imp casts Firebolt every 1.5s; Succubus swings every 2.0s and "
                                "casts Lash of Pain on cooldown (9s, 6s with 3/3 Sayaad).");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "Imp (Firebolt):");
            render_imp_dps_chart("Imp DPS - Infinite Mana", false, fight_duration, sp_max);
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "Succubus (Melee + Lash of Pain):");
            render_succubus_dps_chart("Succubus DPS - Infinite Mana", false, fight_duration, sp_max);
            WowEndTabItem();
        }
        WowEndTabBar();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Assumptions: Imp Firebolt 44 base + (2.0/3.5) x 15%% of master Fire SP, 2.0s cast (Classic toggle: 85-98 + (1.5/3.5)x15%% SP, 1.5s cast); "
                        "Succubus melee 145-195 + (57%% Shadow SP / 14) x 2.0, x0.86 armor, 95%% hit, 5%% crit x2.0; "
                        "Lash of Pain 50 + (1.5/3.5) x 15%% of master Shadow SP, 83%% hit, 5%% crit x1.5 (12s base cd); "
                        "UP +2%%/pt, Improved Imp / Sayaad +10%%/pt; "
                        "no CoE/CoS, no resists, no mana buffs. Mirrors the DES pet engine exactly.");
}

} // namespace warlock
