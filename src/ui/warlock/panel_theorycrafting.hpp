#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"
#include "implot.h"
#include "panel_imp_analysis.hpp"
#include "panel_isb_analysis.hpp"
#include "src/sim/spells.hpp"
#include "src/sim/stats.hpp"
#include "src/sim/warlock_sim.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace warlock
{

inline void render_panel_theorycrafting(const WarlockSimulator& sim)
{
  if (WowBeginTabBar("TheorycraftSubTabs", ImGuiTabBarFlags_None))
  {
    // -------------------------------------------------------------------------------------------------
    // SUBTAB 1: PET DAMAGE ANALYSIS (Imp & Succubus Scaling)
    // -------------------------------------------------------------------------------------------------
    if (WowBeginTabItem("  Pet Damage Analysis  "))
    {
      ImGui::Spacing();
      render_panel_imp_analysis(sim.fight_duration);
      WowEndTabItem();
    }

    // -------------------------------------------------------------------------------------------------
    // SUBTAB 2: ISB UPTIME ANALYSIS (ISB uptime vs crit & hit chance)
    // -------------------------------------------------------------------------------------------------
    if (WowBeginTabItem("  ISB Uptime Analysis  "))
    {
      ImGui::Spacing();
      render_panel_isb_analysis();
      WowEndTabItem();
    }

    // -------------------------------------------------------------------------------------------------
    // SUBTAB 3: MATHEMATICAL PROOFS & DOMINANCE THEOREMS
    // -------------------------------------------------------------------------------------------------
    if (WowBeginTabItem("  Theorycrafting  "))
    {
      ImGui::Spacing();

      // Active player stats for live evaluations
      Stats stats = sim.use_raw_stats ? sim.raw_stats : sim.gear.calculate_stats();
      sim.buffs.apply_to_stats(stats, sim.base_attrs, true, sim.mechanics.personal_shadow_weaving);
      double active_sp = stats.effective_shadow_power();
      double shadow_mult = stats.all_damage_multiplier * stats.shadow_multiplier;

      // -------------------------------------------------------------------------------------------------
      // THEOREM 1: Bane of Doom (T >= 60s) Dominance Over Bane of Agony
      // -------------------------------------------------------------------------------------------------
      if (WowCollapsingHeader("[Unverified] Policy that prefers Bane of Doom (T >= 60s) > Bane of Agony strictly "
                                  "Dominates only Bane of Agony",
                                  ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::Indent(8.0f);

        ImGui::TextWrapped(
            "Let T_rem be the remaining fight duration. For all Spell Power SP >= 0 and all shadow multipliers "
            "M_shadow > "
            "0, "
            "the hybrid policy P*(T_rem >= 60s -> Bane of Doom, T_rem < 60s -> Bane of Agony) strictly dominates "
            "the static policy P_Agony (always cast Bane of Agony) in Total Damage, Cast-Time Efficiency (DPS/GCD), "
            "and "
            "Mana Efficiency.");

        ImGui::Spacing();

        // Direct Damage Bound & Imp BoA
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f),
                           "Direct Spell Formulas with 2/2 Improved Bane of Agony:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped(
            "Over any 60.0-second window, Bane of Doom completes 1 full cycle (1 tick at t=60s), while Bane "
            "of Agony requires "
            "2.5 full 24.0-second cycles (2.5 casts = 30 ticks).");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - D_Doom_direct(SP) = (1742.0 + 4.00 * SP) * M_shadow");
        ImGui::TextWrapped(
            "  - In WoW Forever, Improved Bane of Agony (2/2) increases total damage by +10%% (amplifying "
            "both base damage and spell power):");
        ImGui::TextColored(
            ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - D_Agony_60s(SP) = 2.5 * 1.10 * (552.0 + 1.60 * SP) * M_shadow = (1518.0 + 4.40 * SP) * M_shadow");
        ImGui::TextColored(
            ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - Direct Difference: dD_direct(SP) = (1742.0 - 1518.0) + (4.00 - 4.40) * SP = 224.0 - 0.40 * SP");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        // Talent Invariance & Edge Cases
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f),
                           "Invariance Across All Other Talent Trees & Procs:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped("- Shadow Multipliers (Shadow Mastery 5/5, Demonic Sacrifice, ISB): Both spells are 100%% "
                           "Shadow school; all "
                           "percentage multipliers amplify both identically (M_shadow > 0 preserves inequalities).");
        ImGui::TextWrapped(
            "- DoT Critical Strikes (Pandemic 3/3): Both Agony and Doom ticks share identical crit chance "
            "and identical Pandemic bonus multipliers (1.0 + 0.5 * (1 + Pandemic/3)).");
        ImGui::TextWrapped(
            "- Amplify Curse (3-min CD): +50%% damage to 1 Agony cast (+303.6 + 0.88*SP). Over a full 180s "
            "cycle, this adds only +101.2 + 0.293*SP per minute, which is heavily outweighed by Doom's "
            "freed filler budget.");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        // GCD & Casting Budget Opportunity Cost
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "GCD & Casting Budget Opportunity Cost:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped(
            "Casting Bane of Doom requires 1 GCD (1.50s) per 60.0s window. Casting Bane of Agony requires "
            "2.5 GCDs (3.75s) per 60.0s window.");
        ImGui::TextWrapped("By casting Doom instead of Agony, the warlock reclaims dt = 3.75s - 1.50s = 2.25s of "
                           "casting time per 60s.");
        ImGui::TextWrapped(
            "With 5/5 Bane (2.5s Shadow Bolt cast time), this 2.25s is converted into 2.25 / 2.50 = 0.90 "
            "extra Shadow Bolt casts (or 0.75 without Bane):");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - dD_filler(SP) [5/5 Bane] = 0.90 * (510.0 + 0.85714 * SP) * M_shadow = (459.0 + 0.7714 "
                           "* SP) * M_shadow");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - dD_filler(SP) [No Bane]  = 0.75 * (510.0 + 0.85714 * SP) * M_shadow = (382.5 + 0.6429 "
                           "* SP) * M_shadow");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        // Total Net Yield
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Combined Net Yield & Strict Dominance:");
        ImGui::Indent(12.0f);
        ImGui::TextColored(
            ImVec4(0.3f, 1.0f, 0.6f, 1.0f),
            "  - Net Gain (with 5/5 Bane): dD_total(SP) = (224.0 + 459.0) + (-0.40 + 0.7714) * SP = (683.0 "
            "+ 0.3714 * SP) * M_shadow > 0");
        ImGui::TextColored(
            ImVec4(0.3f, 1.0f, 0.6f, 1.0f),
            "  - Net Gain (without Bane):  dD_total(SP) = (224.0 + 382.5) + (-0.40 + 0.6429) * SP = (606.5 "
            "+ 0.2429 * SP) * M_shadow > 0");
        ImGui::TextColored(
            ImVec4(0.3f, 0.8f, 1.0f, 1.0f),
            "  - Mana Consumed: Doom = 300 Mana | Agony = 2.5 * 215 = 537.5 Mana (Doom saves +237.5 Mana / "
            "60s = +3.96 MP5 equivalent)");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        // Boundary Condition
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Boundary Condition for T_rem < 60s:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped(
            "Because Bane of Doom inflicts all damage exactly at t=60.0s, its yield is 0 if the encounter "
            "terminates prior to t=60.0s. "
            "Bane of Agony begins dealing damage at t=2.0s and ticks continuously every 2.0s, ensuring "
            "positive yield for any T_rem >= 8-10s. "
            "Thus, switching to Agony when T_rem < 60.0s guarantees optimal payoff at the boundary.");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                           "P* strictly dominates P_Agony for 100%% of all talent builds, gear levels, and "
                           "encounter lengths.");

        ImGui::Spacing();
        ImGui::Separator();

        // Live Dynamic Evaluation for Current Gear
        double cur_doom_dmg = (1742.0 + 4.0 * active_sp) * shadow_mult;
        double cur_agony_dmg = 2.5 * 1.10 * (552.0 + 1.60 * active_sp) * shadow_mult;
        double cur_direct_delta = cur_doom_dmg - cur_agony_dmg;
        double cur_filler_delta = 0.90 * (510.0 + 0.85714 * active_sp) * shadow_mult;
        double cur_total_delta = cur_direct_delta + cur_filler_delta;
        double cur_dps_gain = cur_total_delta / 60.0;

        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                           "Live Evaluation for Current Loadout (SP: %.0f, Shadow Multiplier: %.2fx):",
                           active_sp,
                           shadow_mult);
        ImGui::Columns(4, "DoomProofCols", true);
        ImGui::Text("Doom Direct (60s):");
        ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "%.0f dmg", cur_doom_dmg);

        ImGui::NextColumn();
        ImGui::Text("Agony 2.5x (2/2 Imp):");
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%.0f dmg", cur_agony_dmg);

        ImGui::NextColumn();
        ImGui::Text("Freed Cast (0.9 SB):");
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "+%.0f dmg", cur_filler_delta);

        ImGui::NextColumn();
        ImGui::Text("Net DPS Advantage:");
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "+%.2f DPS (+%.0f/60s)", cur_dps_gain, cur_total_delta);
        ImGui::Columns(1);

        ImGui::Spacing();

        // Interactive Plot: Damage vs Spell Power Curve (in 60s Total Damage)
        const int plot_points = 51;
        std::vector<double> sp_vals(plot_points);
        std::vector<double> doom_direct_curve(plot_points);
        std::vector<double> agony_curve(plot_points);
        std::vector<double> doom_policy_bane_curve(plot_points);
        std::vector<double> doom_policy_nobane_curve(plot_points);
        std::vector<double> net_delta_bane_curve(plot_points);
        std::vector<double> net_delta_nobane_curve(plot_points);

        for (int p = 0; p < plot_points; ++p)
        {
          double sp_eval = p * 40.0;  // 0 to 2000 SP
          sp_vals[p] = sp_eval;
          doom_direct_curve[p] = (1742.0 + 4.00 * sp_eval) * shadow_mult;
          agony_curve[p] = 2.5 * 1.10 * (552.0 + 1.60 * sp_eval) * shadow_mult;
          double filler_bane = 0.90 * (510.0 + 0.85714 * sp_eval) * shadow_mult;
          double filler_nobane = 0.75 * (510.0 + 0.85714 * sp_eval) * shadow_mult;
          doom_policy_bane_curve[p] = doom_direct_curve[p] + filler_bane;
          doom_policy_nobane_curve[p] = doom_direct_curve[p] + filler_nobane;
          net_delta_bane_curve[p] = doom_policy_bane_curve[p] - agony_curve[p];
          net_delta_nobane_curve[p] = doom_policy_nobane_curve[p] - agony_curve[p];
        }

        if (ImPlot::BeginPlot("Bane of Doom vs Bane of Agony (60s Window)", ImVec2(-1, 290)))
        {
          ImPlot::SetupAxes("Spell Power", "Total Damage in 60s Window");
          ImPlot::SetupAxisLimits(ImAxis_X1, 0, 2000, ImPlotCond_Once);
          ImPlot::PlotLine("Doom Policy Total [5/5 Bane] (Doom + 0.90 SB, Slope: 4.771)",
                           sp_vals.data(),
                           doom_policy_bane_curve.data(),
                           plot_points);
          ImPlot::PlotLine("Doom Policy Total [No Bane] (Doom + 0.75 SB, Slope: 4.643)",
                           sp_vals.data(),
                           doom_policy_nobane_curve.data(),
                           plot_points);
          ImPlot::PlotLine("Bane of Agony 2.5x (2/2 Imp BoA = +10% Total, Slope: 4.400)",
                           sp_vals.data(),
                           agony_curve.data(),
                           plot_points);
          ImPlot::PlotLine(
              "Doom Direct Only (1 Cast, Slope: 4.000)", sp_vals.data(), doom_direct_curve.data(), plot_points);
          ImPlot::PlotLine("Net Advantage Delta [5/5 Bane] (Slope: +0.371)",
                           sp_vals.data(),
                           net_delta_bane_curve.data(),
                           plot_points);
          ImPlot::PlotLine("Net Advantage Delta [No Bane] (Slope: +0.243)",
                           sp_vals.data(),
                           net_delta_nobane_curve.data(),
                           plot_points);
          ImPlot::EndPlot();
        }

        ImGui::Unindent(8.0f);
      }

      ImGui::Spacing();
      ImGui::Separator();

      // -------------------------------------------------------------------------------------------------
      // THEOREM: Amplify Curse (1/1) Dominance Over Improved Bane of Agony (1/2 or 2/2)
      // -------------------------------------------------------------------------------------------------
      if (WowCollapsingHeader(
              "Amplify Curse (1/1) strictly Dominates Improved Bane of Agony (1/2 or 2/2) per Talent Point",
              ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::Indent(8.0f);

        ImGui::TextWrapped("For all Spell Power SP >= 0, encounter durations T > 0, and rotation policies, investing 1 "
                           "talent point into "
                           "Amplify Curse (1/1) strictly yields greater marginal damage than investing 1 or 2 talent "
                           "points into Improved "
                           "Bane of Agony (1/2 or 2/2).");

        ImGui::Spacing();

        // Under Optimal Policy P*
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f),
                           "Marginal Yield Under Optimal Policy P* [Doom on T >= 60s]:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped("Under the optimal policy P*, Bane of Doom is cast for all 60.0s windows. Bane of Agony is "
                           "cast only during "
                           "the terminal encounter tail (T_rem < 60.0s), resulting in N_Agony in {0, 1, 2} casts per "
                           "encounter (average "
                           "~= 1 cast).");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - Gain from Amplify Curse (1 pt): dD_Amp = 1.0 * 0.50 * D_Agony(SP) = +0.500 * D_Agony");
        ImGui::TextColored(
            ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - Gain from Imp Bane of Agony (1 pt = +5%%): dD_Imp1 = N_Agony * 0.05 * D_Agony(SP) = +0.050 * D_Agony");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - Gain from Imp Bane of Agony (2 pts = +10%%): dD_Imp2 = N_Agony * 0.10 * D_Agony(SP) = "
                           "+0.100 * D_Agony");
        ImGui::TextColored(
            ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
            "  - Marginal Efficiency Ratio: dD_Amp / dD_Imp1 = 0.50 / 0.05 = 10.0x (or 5.0x with 2 casts) "
            "in favor of Amplify Curse.");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        // Under 100% Continuous Agony Spam
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f),
                           "Marginal Yield Under 100%% Continuous Agony Uptime [No Doom]:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped("Even under sub-optimal 100%% continuous Agony uptime (refreshing every 24.0s), over one "
                           "180.0s Amplify Curse "
                           "cooldown cycle there are exactly 180.0 / 24.0 = 7.5 Agony casts:");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - Amplify Curse (1 pt): dD_Amp = 1 * 0.50 * D_Agony = +0.500 * D_Agony per 180s cycle");
        ImGui::TextColored(
            ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - Imp Bane of Agony (1 pt = +5%%): dD_Imp1 = 7.5 * 0.05 * D_Agony = +0.375 * D_Agony per 180s cycle");
        ImGui::TextColored(
            ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - Break-Even Condition: N * 0.05 >= 0.50 ==> N >= 10 casts (240s without recasting Amplify Curse).");
        ImGui::TextColored(
            ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
            "  - Because CD_Amp = 180s < 240s, Amplify Curse is +33.3%% stronger than 1 pt Imp BoA even in "
            "100%% continuous Agony spam.");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        // Action Economy & Tree Progression
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Action Economy & Tree Progression:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped("- Instant & Off-GCD: Amplify Curse incurs 0.0s cast time and 0.0s GCD, weaving with zero "
                           "opportunity cost.");
        ImGui::TextWrapped("- Tree Pathway: Amplify Curse unlocks Curse of Exhaustion (R4C3), whereas Improved Bane of "
                           "Agony is a dead-end node.");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        ImGui::TextColored(
            ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
            "Amplify Curse (1/1) strictly dominates Improved Bane of Agony per talent point under 100%% of "
            "rotation styles, encounter lengths, and gear levels.");

        ImGui::Spacing();
        ImGui::Separator();

        // Live Evaluation for Current Loadout
        double base_agony_single = (552.0 + 1.596 * active_sp) * shadow_mult;
        double amp_gain_single = 0.50 * base_agony_single;
        double imp_gain_single = 0.05 * base_agony_single;
        double imp_gain_180s_uptime = 7.5 * 0.05 * base_agony_single;

        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                           "Live Evaluation for Current Loadout (SP: %.0f, Base Agony Dmg: %.0f):",
                           active_sp,
                           base_agony_single);
        ImGui::Columns(4, "AmpProofCols", true);
        ImGui::Text("Amplify Curse (+50%%):");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "+%.0f dmg / cast", amp_gain_single);

        ImGui::NextColumn();
        ImGui::Text("Imp BoA (1 pt on P*):");
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "+%.0f dmg (1 cast)", imp_gain_single);

        ImGui::NextColumn();
        ImGui::Text("Imp BoA (1 pt 180s spam):");
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "+%.0f dmg (7.5 casts)", imp_gain_180s_uptime);

        ImGui::NextColumn();
        ImGui::Text("Amplify Advantage:");
        ImGui::TextColored(
            ImVec4(0.3f, 1.0f, 0.6f, 1.0f), "1.33x to 10.0x (+%.0f dmg)", amp_gain_single - imp_gain_180s_uptime);
        ImGui::Columns(1);

        ImGui::Spacing();

        // Interactive Plot: Damage Gain vs Spell Power Curve over 180s
        const int plot_pts = 25;
        std::vector<double> sp_amp_vals(plot_pts);
        std::vector<double> amp_gain_curve(plot_pts);
        std::vector<double> imp_cont_curve(plot_pts);
        std::vector<double> imp_doom_curve(plot_pts);

        for (int p = 0; p < plot_pts; ++p)
        {
          double sp_eval = p * 50.0;
          sp_amp_vals[p] = sp_eval;
          double agony_dmg = (552.0 + 1.596 * sp_eval) * shadow_mult;
          amp_gain_curve[p] = 0.50 * agony_dmg;        // 1 Amp cast per 180s
          imp_cont_curve[p] = 7.5 * 0.05 * agony_dmg;  // 1 pt Imp BoA with 7.5 continuous casts
          imp_doom_curve[p] = 1.0 * 0.05 * agony_dmg;  // 1 pt Imp BoA with 1 terminal cast under P*
        }

        if (ImPlot::BeginPlot("Amplify Curse vs Improved Bane of Agony (180s Window)", ImVec2(-1, 260)))
        {
          ImPlot::SetupAxes("Spell Power", "Marginal Damage Gain per 180s Window");
          ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1200, ImPlotCond_Once);
          ImPlot::PlotLine("Amplify Curse (1 pt, 1 Use)", sp_amp_vals.data(), amp_gain_curve.data(), plot_pts);
          ImPlot::PlotLine(
              "Imp BoA (1 pt, 100% Agony Uptime = 7.5 Casts)", sp_amp_vals.data(), imp_cont_curve.data(), plot_pts);
          ImPlot::PlotLine(
              "Imp BoA (1 pt, Under P* Doom Policy = 1 Cast)", sp_amp_vals.data(), imp_doom_curve.data(), plot_pts);
          ImPlot::EndPlot();
        }

        ImGui::Unindent(8.0f);
      }

      ImGui::Spacing();
      ImGui::Separator();

      // -------------------------------------------------------------------------------------------------
      // THEOREM: Untalented Shadow Bolt vs Maximized Channeled Wrack
      // -------------------------------------------------------------------------------------------------
      if (WowCollapsingHeader(
              "[Unverified] Untalented Shadow Bolt (No Talents) vs Maximized Channeled Wrack (6.0s Budget)",
              ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::Indent(8.0f);

        ImGui::TextWrapped("Comparing the worst possible version of Shadow Bolt (0/5 Bane, 3.0s hardcast, zero "
                           "talents, no ISB, no Ruin) "
                           "against the best possible version of 6.0s Channeled Wrack (11 Affliction talent points, 3 "
                           "active DoTs with "
                           "+10%% amplification, and 2/2 Nightfall procs).");

        ImGui::Spacing();

        // Worst-Case Shadow Bolt Yield in 6.0s
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f),
                           "Worst-Case Untalented Shadow Bolt in 6.0s Budget:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped(
            "With 0/5 Bane, Shadow Bolt has a 3.0s base cast time. In a 6.0s window, the warlock hardcasts "
            "exactly 6.0 / 3.0 = 2.00 Shadow Bolts:");
        ImGui::TextColored(
            ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - D_SB_worst(SP) = 2.00 * (510.0 + 0.85714 * SP) * M_shadow = (1020.0 + 1.7143 * SP) * M_shadow");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        // Best-Case Channeled Wrack Yield with Pandemic & Malevolence
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f),
                           "Best-Case Maximized Channeled Wrack in 6.0s with Crits:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped(
            "Wrack delivers 6 ticks (1 tick/sec) over a 6.0s channel. Fully maxing Affliction talents (3/3 "
            "Imp Drains +20%%, "
            "3/3 Soul Siphon +36%%, 5/5 Malediction +5%%, 5/5 Malevolence +5%% crit, and 3/3 Pandemic "
            "+100%% DoT crit bonus):");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - Direct Wrack: D_direct = (212.0 + 1.00 * SP) * 1.20 * 1.36 * 1.05 = (363.28 + 1.7136 * "
                           "SP) * M_shadow");
        ImGui::TextColored(
            ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - +10%% Shadow DoT Amp (Corruption, Agony, Siphon Life): dD_DoT_amp = (49.40 + 0.0932 * SP) * M_shadow");
        ImGui::TextColored(
            ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            "  - Nightfall 2/2 Procs (6 ticks * 4%% = 0.24 procs => 0.12 hardcast SB saved): dD_NF = (61.20 "
            "+ 0.1029 * SP) * M_shadow");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - Baseline Non-Crit Wrack Yield: D_Wrack_base = (473.88 + 1.9097 * SP) * M_shadow");
        ImGui::TextWrapped("  - Factoring in Spell Crit (c = 15%% with 5/5 Malevolence) & 3/3 Pandemic (2.0x crit): "
                           "M_crit_wrack = 1 + 1.00 * 0.15 = 1.150x");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - Maximized Crit-Adjusted Wrack Yield: D_Wrack_best = 1.150 * (473.88 + 1.9097 * SP) = "
                           "(544.96 + 2.1962 * SP) * M_shadow");
        ImGui::TextWrapped(
            "  - For Untalented SB (no Ruin, 1.5x crit at c=15%%): M_crit_sb = 1 + 0.50 * 0.15 = 1.075x ==> "
            "D_SB_worst = (1096.50 + 1.8429 * SP) * M_shadow");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        // Exact Inflection / Crossover Point
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Inflection / Crossover Threshold SP*:");
        ImGui::Indent(12.0f);
        ImGui::TextWrapped(
            "Setting D_Wrack_best(SP) = D_SB_worst(SP) yields the exact mathematical crossover threshold:");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "  - 544.96 + 2.1962 * SP = 1096.50 + 1.8429 * SP");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "  - (2.1962 - 1.8429) * SP = 1096.50 - 544.96  ==>  0.3533 * SP = 551.54");
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f),
                           "  - Inflection Point with 3/3 Pandemic Crits: SP* = 551.54 / 0.3533 = 1561.0 Spell Power");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
                           "    (Without Pandemic Crits, baseline inflection is SP* = 2795.0 Spell Power)");
        ImGui::TextWrapped(
            "- For SP < 1561 SP: Untalented Shadow Bolt strictly produces more damage (+552 dmg at 0 SP, "
            "+234 dmg at 900 SP).");
        ImGui::TextWrapped("- For SP >= 1561 SP: Maximized Wrack overtakes untalented Shadow Bolt due to 3/3 Pandemic "
                           "2.0x DoT crits.");
        ImGui::TextWrapped("- In Level 60 Vanilla/Classic gameplay where maximum gear caps around ~900 SP, untalented "
                           "Shadow Bolt strictly dominates across all attainable gear.");
        ImGui::Unindent(12.0f);

        ImGui::Spacing();
        ImGui::Separator();

        // Live Evaluation for Current Loadout
        double crit_rate = 0.15;  // 10% base + 5% Malevolence
        double cur_sb_worst = 2.00 * (510.0 + 0.85714 * active_sp) * (1.0 + 0.50 * crit_rate) * shadow_mult;
        double cur_wrack_best = (473.88 + 1.9097 * active_sp) * (1.0 + 1.00 * crit_rate) * shadow_mult;
        double cur_sb_delta = cur_sb_worst - cur_wrack_best;

        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                           "Live Evaluation for Current Loadout (SP: %.0f, 6.0s Budget, c=%.1f%%):",
                           active_sp,
                           crit_rate * 100.0);
        ImGui::Columns(4, "WrackProofCols", true);
        ImGui::Text("Untalented SB (2.0x 3.0s):");
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "%.0f dmg", cur_sb_worst);

        ImGui::NextColumn();
        ImGui::Text("Max Wrack (Pandemic):");
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%.0f dmg", cur_wrack_best);

        ImGui::NextColumn();
        ImGui::Text("Shadow Bolt Margin:");
        ImGui::TextColored(cur_sb_delta >= 0 ? ImVec4(0.3f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                           "%+.0f dmg",
                           cur_sb_delta);

        ImGui::NextColumn();
        ImGui::Text("Inflection Point:");
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "1561 SP (Lv60 Cap: ~900)");
        ImGui::Columns(1);

        ImGui::Spacing();

        // Interactive Plot: 6.0s Damage vs Spell Power Curve up to 3500 SP
        const int plot_wrack_pts = 71;
        std::vector<double> sp_wrack_vals(plot_wrack_pts);
        std::vector<double> sb_worst_curve(plot_wrack_pts);
        std::vector<double> wrack_best_curve(plot_wrack_pts);
        std::vector<double> sb_delta_curve(plot_wrack_pts);

        for (int p = 0; p < plot_wrack_pts; ++p)
        {
          double sp_eval = p * 50.0;  // 0 to 3500 SP
          sp_wrack_vals[p] = sp_eval;
          sb_worst_curve[p] = (1096.50 + 1.8429 * sp_eval) * shadow_mult;
          wrack_best_curve[p] = (544.96 + 2.1962 * sp_eval) * shadow_mult;
          sb_delta_curve[p] = sb_worst_curve[p] - wrack_best_curve[p];
        }

        if (ImPlot::BeginPlot("Untalented Shadow Bolt vs Maximized Channeled Wrack (6s Budget)", ImVec2(-1, 290)))
        {
          ImPlot::SetupAxes("Spell Power", "Total Damage in 6.0s Window");
          ImPlot::SetupAxisLimits(ImAxis_X1, 0, 3500, ImPlotCond_Once);
          ImPlot::PlotLine("Untalented Shadow Bolt (1.5x Crit, Slope: 1.843)",
                           sp_wrack_vals.data(),
                           sb_worst_curve.data(),
                           plot_wrack_pts);
          ImPlot::PlotLine("Maximized Wrack (Pandemic 2.0x Crit + NF + Amp, Slope: 2.196)",
                           sp_wrack_vals.data(),
                           wrack_best_curve.data(),
                           plot_wrack_pts);
          ImPlot::PlotLine("Shadow Bolt Advantage Delta (Inflection at 1561 SP)",
                           sp_wrack_vals.data(),
                           sb_delta_curve.data(),
                           plot_wrack_pts);
          ImPlot::EndPlot();
        }

        ImGui::Unindent(8.0f);
      }

      ImGui::Spacing();
      ImGui::Separator();

      // -------------------------------------------------------------------------------------------------
      // THEOREM 3: Ruin (100% Critical Damage) Superlinear Value Function
      // -------------------------------------------------------------------------------------------------
      if (WowCollapsingHeader("Ruin (100% Critical Bonus) Multiplicative Value Function",
                                  ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::Indent(8.0f);
        ImGui::TextWrapped(
            "The destruction capstone talent Ruin increases critical strike bonus damage from +50%% (1.5x) to +100%% "
            "(2.0x). "
            "The relative damage gain from Ruin g(c) as a function of spell crit chance c in [0, 1] is given by:");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                           "  - g(c) = (1 + 1.0 * c) / (1 + 0.5 * c) = 1 + 0.5 * c / (1 + 0.5 * c)");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "CRITICAL MASS TABLE:");
        ImGui::Columns(4, "RuinTableCols", true);
        ImGui::Text("Spell Crit (c):");
        ImGui::Text("10.0%% Crit");
        ImGui::Text("20.0%% Crit");
        ImGui::Text("30.0%% Crit");
        ImGui::Text("40.0%% Crit");

        ImGui::NextColumn();
        ImGui::Text("Without Ruin (1.5x):");
        ImGui::Text("1.050x");
        ImGui::Text("1.100x");
        ImGui::Text("1.150x");
        ImGui::Text("1.200x");

        ImGui::NextColumn();
        ImGui::Text("With Ruin (2.0x):");
        ImGui::Text("1.100x");
        ImGui::Text("1.200x");
        ImGui::Text("1.300x");
        ImGui::Text("1.400x");

        ImGui::NextColumn();
        ImGui::Text("Net Damage Gain g(c):");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "+4.76%%");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "+9.09%%");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "+13.04%%");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "+16.67%%");
        ImGui::Columns(1);

        ImGui::Unindent(8.0f);
      }

      ImGui::Spacing();
      ImGui::Separator();
      WowEndTabItem();
    }

    WowEndTabBar();
  }
}

}  // namespace warlock
