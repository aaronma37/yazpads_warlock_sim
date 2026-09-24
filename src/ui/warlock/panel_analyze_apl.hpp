#pragma once
#include "asset_manager.hpp"
#include "damage_breakdown_view.hpp"
#include "wow_widgets.hpp"
#include "imgui.h"
#include "src/sim/warlock/apl_analyzer.hpp"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace warlock
{

// Background worker state for asynchronous APL vs MCTS trace analysis
struct APLAnalyzerWorkerState
{
  std::thread worker;
  std::mutex mtx;
  std::atomic<bool> is_running{false};
  std::atomic<float> progress{0.0f};
  std::string current_status;
  APLAnalysisReport live_report;
  bool has_result = false;
};

inline APLAnalyzerWorkerState& get_apl_analyzer_worker_state()
{
  static APLAnalyzerWorkerState state;
  return state;
}

// Color palette for spell timeline Gantt blocks
inline ImVec4 get_spell_block_color(SpellID id, const std::string& tag)
{
  switch (id)
  {
    case SpellID::SHADOW_BOLT:
      return ImVec4(0.32f, 0.16f, 0.54f, 0.95f); // Deep Shadow Purple
    case SpellID::CORRUPTION:
    case SpellID::CURSE_OF_AGONY:
    case SpellID::SIPHON_LIFE:
    case SpellID::CURSE_OF_DOOM:
    case SpellID::CURSE_OF_SHADOWS:
    case SpellID::CURSE_OF_ELEMENTS:
      return ImVec4(0.18f, 0.48f, 0.22f, 0.95f); // Affliction DoT Green
    case SpellID::DRAIN_HOPE:
    case SpellID::DRAIN_LIFE:
    case SpellID::DRAIN_SOUL:
      return ImVec4(0.15f, 0.42f, 0.52f, 0.95f); // Drain Teal
    case SpellID::IMMOLATE:
    case SpellID::CONFLAGRATE:
    case SpellID::INCINERATE:
    case SpellID::SOUL_FIRE:
    case SpellID::SEARING_PAIN:
      return ImVec4(0.68f, 0.28f, 0.08f, 0.95f); // Destro Fire Orange
    case SpellID::LIFE_TAP:
      return ImVec4(0.15f, 0.48f, 0.72f, 0.95f); // Mana Blue
    case SpellID::RACIAL_EUREKA:
    case SpellID::RACIAL_BLOOD_FURY:
    case SpellID::RACIAL_BERSERKING:
    case SpellID::AMPLIFY_CURSE:
      return ImVec4(0.70f, 0.52f, 0.12f, 0.95f); // Cooldown Amber
    default:
      return ImVec4(0.35f, 0.35f, 0.35f, 0.95f);
  }
}

inline void render_panel_analyze_apl(const WarlockSimulator& sim, AppTab* switch_tab = nullptr)
{
  auto& worker = get_apl_analyzer_worker_state();

  static int num_runs_to_analyze = 5;
  static int rollouts_per_action = 256;
  static int selected_run_tab = 0; // 0 = Summary, 1..N = Run #1..#N
  static int run_sub_tab = 0;       // 0 = Unified Timeline & Graphs, 1 = Discrepancy Matrix Table
  static float timeline_zoom_px_per_sec = 16.0f;
  static char text_filter[64] = "";
  static char summary_filter[64] = "";
  static int expanded_event_idx = -1;

  // Handle worker completion
  {
    std::lock_guard<std::mutex> lock(worker.mtx);
    if (!worker.is_running.load() && worker.worker.joinable())
    {
      worker.worker.join();
    }
  }

  bool is_busy = worker.is_running.load();
  float current_progress = worker.progress.load();
  std::string current_status = worker.current_status;

  // -------------------------------------------------------------------------
  // Header / Control Bar
  // -------------------------------------------------------------------------
  BeginWowChild("APLAnalysisControlBar", ImVec2(0, 84.0f), true);
  {
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "APL Analysis Using Optimal Policy From MCTS");

    ImGui::Spacing();

    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Runs:");
    ImGui::SetNextItemWidth(80.0f);
    if (is_busy) ImGui::BeginDisabled();
    WowInputInt("##NumRunsInput", &num_runs_to_analyze, 1, 5);
    if (num_runs_to_analyze < 1) num_runs_to_analyze = 1;
    if (num_runs_to_analyze > 50) num_runs_to_analyze = 50;
    ImGui::EndGroup();

    ImGui::SameLine(0, 14.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("MCTS Rollouts / Action:");
    ImGui::SetNextItemWidth(120.0f);
    WowInputInt("##RolloutsPerAction", &rollouts_per_action, 32, 128);
    if (rollouts_per_action < 16) rollouts_per_action = 16;
    if (rollouts_per_action > 2048) rollouts_per_action = 2048;
    if (is_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 16.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 14.0f);

    if (is_busy)
    {
      ImGui::ProgressBar(current_progress, ImVec2(320, 26), current_status.c_str());
    }
    else
    {
      if (WowButton("Run", ImVec2(100, 26)))
      {
        worker.is_running = true;
        worker.progress = 0.05f;
        worker.current_status = "Launching parallel MCTS forward rollout workers...";
        worker.has_result = false;

        if (worker.worker.joinable())
        {
          worker.worker.join();
        }

        WarlockSimulator sim_copy = sim;
        int runs_cnt = num_runs_to_analyze;
        int r_cnt = rollouts_per_action;

        worker.worker = std::thread([sim_copy, runs_cnt, r_cnt]() {
          auto& w = get_apl_analyzer_worker_state();
          APLAnalysisReport res = APLAnalyzer::run_analysis(
              sim_copy,
              runs_cnt,
              r_cnt,
              [&](float p, const std::string& status) {
                w.progress = p;
                w.current_status = status;
              },
              1337);

          {
            std::lock_guard<std::mutex> lock(w.mtx);
            w.live_report = std::move(res);
            w.has_result = true;
            w.is_running = false;
          }
        });
      }
    }
  }
  EndWowChild();

  ImGui::Spacing();

  // If no results yet
  if (!worker.has_result)
  {
    BeginWowChild("APLAnalysisEmptyState", ImVec2(0, 0), true);
    {
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "Monte Carlo Tree Search (MCTS) Strategy & Policy Analyzer");
      ImGui::Spacing();
      ImGui::TextWrapped(
          "This tool compares your active priority list against an optimal policy generated by Monte Carlo Tree Search.");
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Instructions:");
      ImGui::Spacing();

      auto render_tab_link = [&](const char* label, AppTab target) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::CalcTextSize(label);
        ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "%s", label);
        if (ImGui::IsItemHovered()) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
          ImDrawList* dl = ImGui::GetWindowDrawList();
          dl->AddLine(ImVec2(pos.x, pos.y + size.y), ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(115, 200, 255, 255), 1.0f);
          if (ImGui::IsMouseClicked(0) && switch_tab) {
            *switch_tab = target;
          }
        }
      };

      // Step 1:
      ImGui::Text("1) Select desired preset and APL from");
      ImGui::SameLine();
      render_tab_link("Presets", AppTab::PRESETS);
      ImGui::SameLine(0, 0);
      ImGui::Text(",");
      ImGui::SameLine();
      render_tab_link("Compare Standard Specs", AppTab::COMPARE_STANDARD_SPECS);
      ImGui::SameLine(0, 0);
      ImGui::Text(", or");
      ImGui::SameLine();
      render_tab_link("Constrained Spec Search", AppTab::CONSTRAINED_SPEC_SEARCH);
      ImGui::SameLine(0, 0);
      ImGui::Text(".");

      ImGui::Spacing();

      // Step 2:
      ImGui::Text("2) Modify APL if wanted in");
      ImGui::SameLine();
      render_tab_link("Presets", AppTab::PRESETS);
      ImGui::SameLine(0, 0);
      ImGui::Text(".");

      ImGui::Spacing();

      // Step 3:
      ImGui::Text("3) Hit Run.");

      ImGui::Spacing();

      // Step 4:
      ImGui::Text("4) View APL Blunders or combat timeline to compare to MCTS policy.");

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      ImGui::TextDisabled("Click 'Run' above to begin.");
    }
    EndWowChild();
    return;
  }

  const auto& report = worker.live_report;

  // -------------------------------------------------------------------------
  // Top KPI Summary Cards
  // -------------------------------------------------------------------------
  float avail_w = ImGui::GetContentRegionAvail().x;
  float card_w = (avail_w - 3.0f * 12.0f) / 4.0f;
  float card_h = 72.0f;

  // Card 1: Active APL Baseline DPS
  BeginWowChild("MetricCard1", ImVec2(card_w, card_h), true);
  {
    ImGui::TextDisabled("ACTIVE APL MEAN DPS");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "%.1f DPS", report.avg_apl_dps);
    ImGui::TextDisabled("Current priority list throughput");
  }
  EndWowChild();

  ImGui::SameLine(0, 12.0f);

  // Card 2: MCTS Optimal Policy Ceiling
  BeginWowChild("MetricCard2", ImVec2(card_w, card_h), true);
  {
    ImGui::TextDisabled("MCTS OPTIMAL POLICY CEILING");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%.1f DPS", report.avg_mcts_dps);
    ImGui::TextDisabled("Empirical forward rollout maximum");
  }
  EndWowChild();

  ImGui::SameLine(0, 12.0f);

  // Card 3: Realized Regret / Delta DPS
  BeginWowChild("MetricCard3", ImVec2(card_w, card_h), true);
  {
    ImGui::TextDisabled("DPS GAP (OPTIMIZATION POTENTIAL)");
    ImGui::Spacing();
    ImVec4 gap_color = (report.avg_dps_loss < 3.0) ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
    ImGui::TextColored(gap_color, "+%.1f DPS (+%.1f%%)", report.avg_dps_loss, report.avg_dps_loss_pct);
    ImGui::TextDisabled("Potential rotational yield");
  }
  EndWowChild();

  ImGui::SameLine(0, 12.0f);

  // Card 4: Agreement Fidelity & Divergence Count
  BeginWowChild("MetricCard4", ImVec2(card_w, card_h), true);
  {
    ImGui::TextDisabled("AGREEMENT FIDELITY");
    ImGui::Spacing();
    ImVec4 agree_col = (report.overall_agreement_pct >= 90.0) ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) :
                       (report.overall_agreement_pct >= 75.0) ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
    ImGui::TextColored(agree_col, "%.1f%% Agreement", report.overall_agreement_pct);
    ImGui::TextDisabled("%zu diffs / %zu GCDs", report.total_divergences, report.total_decisions_evaluated);
  }
  EndWowChild();

  ImGui::Spacing();

  // -------------------------------------------------------------------------
  // Run Navigation Tabs
  // -------------------------------------------------------------------------
  BeginWowChild("APLRunsNavBar", ImVec2(0, 42.0f), true);
  {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Select View:");
    ImGui::SameLine();

    bool is_summary_selected = (selected_run_tab == 0);
    if (WowTabButton("All Runs Summary", is_summary_selected, 140.0f, 26.0f))
    {
      selected_run_tab = 0;
    }

    for (size_t i = 0; i < report.runs.size(); ++i)
    {
      ImGui::SameLine(0, 6.0f);
      std::string label = "Run #" + std::to_string(i + 1) + " (" + std::to_string(report.runs[i].divergence_count) + " diffs)";
      bool is_run_selected = (selected_run_tab == static_cast<int>(i + 1));
      if (WowTabButton(label.c_str(), is_run_selected, 140.0f, 26.0f))
      {
        selected_run_tab = static_cast<int>(i + 1);
        expanded_event_idx = -1;
      }
    }
  }
  EndWowChild();

  ImGui::Spacing();

  // -------------------------------------------------------------------------
  // Main Content: All Runs Summary vs. Individual Run Detailed Unified Timeline
  // -------------------------------------------------------------------------
  if (selected_run_tab == 0)
  {
    // ALL RUNS SUMMARY VIEW
    BeginWowChild("APLSummaryView", ImVec2(0, 0), true);
    {
      struct MasterBlunderEntry {
        size_t run_idx;
        const APLDivergenceEvent* ev;
      };

      std::vector<MasterBlunderEntry> all_blunders;
      for (size_t r_i = 0; r_i < report.runs.size(); ++r_i) {
        for (const auto& ev : report.runs[r_i].events) {
          all_blunders.push_back({r_i, &ev});
        }
      }

      std::sort(all_blunders.begin(), all_blunders.end(), [](const auto& a, const auto& b) {
        if (std::abs(a.ev->delta_dps - b.ev->delta_dps) > 0.01)
          return a.ev->delta_dps > b.ev->delta_dps;
        return a.ev->confidence_pct > b.ev->confidence_pct;
      });

      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Combined Master APL Blunder Log (All Episodes Combined)");
      ImGui::SameLine();
      ImGui::TextDisabled("| %zu total discrepancies detected across %zu episodes, ranked by DPS loss", all_blunders.size(), report.runs.size());

      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220.0f);
      ImGui::SetNextItemWidth(220.0f);
      ImGui::InputTextWithHint("##SummaryBlunderFilter", "Search spell or rationale...", summary_filter, IM_ARRAYSIZE(summary_filter));

      ImGui::Spacing();

      if (all_blunders.empty())
      {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Optimal APL! No statistically significant rotational discrepancies detected between APL and MCTS across all episodes.");
      }
      else
      {
        std::string filter_str = summary_filter;
        std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

        static ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("CombinedBlundersTable", 9, table_flags, ImVec2(0, 280.0f)))
        {
          ImGui::TableSetupColumn("Run", ImGuiTableColumnFlags_WidthFixed, 85.0f);
          ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 105.0f);
          ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 60.0f);
          ImGui::TableSetupColumn("Combat Context", ImGuiTableColumnFlags_WidthFixed, 160.0f);
          ImGui::TableSetupColumn("APL Move Made", ImGuiTableColumnFlags_WidthFixed, 160.0f);
          ImGui::TableSetupColumn("MCTS Optimal Choice", ImGuiTableColumnFlags_WidthFixed, 160.0f);
          ImGui::TableSetupColumn("DPS Loss (95% CI)", ImGuiTableColumnFlags_WidthFixed, 130.0f);
          ImGui::TableSetupColumn("Confidence", ImGuiTableColumnFlags_WidthFixed, 85.0f);
          ImGui::TableSetupColumn("Strategic Rationale & Alternative Values", ImGuiTableColumnFlags_WidthStretch);
          ImGui::TableHeadersRow();

          size_t display_rank = 1;
          for (const auto& item : all_blunders)
          {
            const auto& ev = *item.ev;

            if (!filter_str.empty())
            {
              std::string combined = ev.apl_action_name + " " + ev.mcts_action_name + " " + ev.rationale + " " + ev.top_alternatives_summary;
              std::transform(combined.begin(), combined.end(), combined.begin(), ::tolower);
              if (combined.find(filter_str) == std::string::npos)
                continue;
            }

            ImGui::TableNextRow();

            // Col 0: Run # (Click to Inspect)
            ImGui::TableNextColumn();
            std::string btn_lbl = "Run #" + std::to_string(item.run_idx + 1);
            if (ImGui::SmallButton(btn_lbl.c_str()))
            {
              selected_run_tab = static_cast<int>(item.run_idx + 1);
              expanded_event_idx = -1;
            }
            if (ImGui::IsItemHovered())
            {
              ImGui::SetTooltip("Jump to Run #%zu timeline", item.run_idx + 1);
            }

            // Col 1: Severity
            ImGui::TableNextColumn();
            ImVec4 sev_col;
            const char* sev_tag;
            if (ev.delta_dps >= 15.0)
            {
              sev_col = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
              sev_tag = "[CRITICAL]";
            }
            else if (ev.delta_dps >= 5.0)
            {
              sev_col = ImVec4(1.0f, 0.65f, 0.2f, 1.0f);
              sev_tag = "[MAJOR]";
            }
            else
            {
              sev_col = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);
              sev_tag = "[MINOR]";
            }
            ImGui::TextColored(sev_col, "#%zu %s", display_rank++, sev_tag);

            // Col 2: Time
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%04.1fs", ev.timestamp);

            // Col 3: Combat Context
            ImGui::TableNextColumn();
            ImGui::Text("Mana: %d%% | HP: %d%%",
                        static_cast<int>(ev.state.player_mana_pct * 100.0f),
                        static_cast<int>(ev.state.target_hp_pct * 100.0f));
            
            std::string dots_summary;
            if (ev.state.dot_corruption_rem_sec > 0.0f) dots_summary += "Corr:" + std::to_string(static_cast<int>(ev.state.dot_corruption_rem_sec)) + "s ";
            if (ev.state.dot_immolate_rem_sec > 0.0f) dots_summary += "Immo:" + std::to_string(static_cast<int>(ev.state.dot_immolate_rem_sec)) + "s ";
            if (ev.state.dot_doom_rem_sec > 0.0f) dots_summary += "Doom:" + std::to_string(static_cast<int>(ev.state.dot_doom_rem_sec)) + "s ";
            if (ev.state.dot_agony_rem_sec > 0.0f) dots_summary += "Agony:" + std::to_string(static_cast<int>(ev.state.dot_agony_rem_sec)) + "s ";
            if (ev.state.isb_charges_rem > 0.0f) dots_summary += "ISB(" + std::to_string(static_cast<int>(ev.state.isb_charges_rem)) + ") ";
            if (ev.state.nightfall_proc_active > 0.5f) dots_summary += "Trance ";
            if (dots_summary.empty()) dots_summary = "No DoTs active";
            ImGui::TextDisabled("%s", dots_summary.c_str());

            // Col 4: APL Move Made
            ImGui::TableNextColumn();
            const auto& apl_icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(ev.apl_action)));
            rlImGuiImageSize(&apl_icon, 16, 16);
            ImGui::SameLine(0, 4.0f);
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", ev.apl_action_name.c_str());
            ImGui::TextDisabled("%.1f ± %.1f DPS", ev.apl_expected_dps, ev.apl_std_error);

            // Col 5: MCTS Optimal Choice
            ImGui::TableNextColumn();
            const auto& mcts_icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(ev.mcts_action)));
            rlImGuiImageSize(&mcts_icon, 16, 16);
            ImGui::SameLine(0, 4.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", ev.mcts_action_name.c_str());
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 0.8f), "%.1f ± %.1f DPS", ev.mcts_expected_dps, ev.mcts_std_error);

            // Col 6: DPS Loss (95% CI)
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "-%.1f DPS", ev.delta_dps);
            ImGui::TextDisabled("CI: [%.1f, %.1f]", ev.delta_dps_ci_lower, ev.delta_dps_ci_upper);

            // Col 7: Confidence
            ImGui::TableNextColumn();
            ImVec4 conf_col = (ev.confidence_pct >= 95.0) ? ImVec4(0.2f, 0.9f, 0.4f, 1.0f) :
                              (ev.confidence_pct >= 85.0) ? ImVec4(1.0f, 0.85f, 0.3f, 1.0f) : ImVec4(0.9f, 0.6f, 0.2f, 1.0f);
            ImGui::TextColored(conf_col, "%.1f%%", ev.confidence_pct);
            ImGui::TextDisabled("Z = %.1f", ev.z_score);

            // Col 8: Strategic Rationale & Alternatives
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", ev.rationale.c_str());
            if (!ev.top_alternatives_summary.empty())
            {
              ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 0.9f), "Other Viable Actions: %s", ev.top_alternatives_summary.c_str());
            }
          }
          ImGui::EndTable();
        }
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Per-Run Breakdown Table
      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "All Simulated Episodes (Detailed Run List):");
      ImGui::Spacing();

      static ImGuiTableFlags run_table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
      if (ImGui::BeginTable("AllRunsTable", 6, run_table_flags, ImVec2(0, 0)))
      {
        ImGui::TableSetupColumn("Run Index", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("APL DPS", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("MCTS DPS", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("DPS Gap", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Rotational Divergences", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < report.runs.size(); ++i)
        {
          const auto& r = report.runs[i];
          ImGui::TableNextRow();

          ImGui::TableNextColumn();
          if (ImGui::SmallButton(("Inspect #" + std::to_string(i + 1)).c_str()))
          {
            selected_run_tab = static_cast<int>(i + 1);
            expanded_event_idx = -1;
          }
          ImGui::SameLine(0, 4.0f);
          ImGui::Text("Run #%zu", i + 1);

          ImGui::TableNextColumn();
          ImGui::Text("%.1fs", r.fight_duration);

          ImGui::TableNextColumn();
          ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "%.1f DPS", r.apl_dps);

          ImGui::TableNextColumn();
          ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%.1f DPS", r.mcts_dps);

          ImGui::TableNextColumn();
          ImVec4 diff_col = (r.dps_difference <= 0.0) ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
          ImGui::TextColored(diff_col, "+%.1f DPS", r.dps_difference);

          ImGui::TableNextColumn();
          ImGui::Text("%zu divergences / %zu GCDs (%.1f%% agreement)", r.divergence_count, r.total_decisions, r.agreement_rate_pct);
        }
        ImGui::EndTable();
      }
    }
    EndWowChild();
  }
  else
  {
    // INDIVIDUAL RUN DETAILED VIEW
    size_t run_idx = static_cast<size_t>(selected_run_tab - 1);
    if (run_idx < report.runs.size())
    {
      const auto& current_run = report.runs[run_idx];

      BeginWowChild("IndividualRunPane", ImVec2(0, 0), true);
      {
        // Run Header
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Run #%zu Detailed Inspection: APL vs True MCTS Rollout Policy", run_idx + 1);
        ImGui::SameLine();
        ImGui::TextDisabled("| APL: %.1f DPS | MCTS: %.1f DPS | Gap: +%.1f DPS | %zu Discrepancies (%.1f%% Agreement)",
                            current_run.apl_dps, current_run.mcts_dps, current_run.dps_difference,
                            current_run.divergence_count, current_run.agreement_rate_pct);

        ImGui::Spacing();

        // Sub-Navigation Tabs: [Unified Combat Timeline & Graphs] vs [Decision Discrepancy Matrix Table]
        BeginWowChild("RunSubNavBar", ImVec2(0, 36.0f), true);
        {
          if (WowTabButton("Unified Combat Timeline", run_sub_tab == 0, 190.0f, 24.0f))
          {
            run_sub_tab = 0;
          }
          ImGui::SameLine(0, 8.0f);
          std::string matrix_lbl = "APL Blunder Matrix & Judge (" + std::to_string(current_run.events.size()) + ")";
          if (WowTabButton(matrix_lbl.c_str(), run_sub_tab == 1, 240.0f, 24.0f))
          {
            run_sub_tab = 1;
          }
        }
        EndWowChild();

        ImGui::Spacing();

        if (run_sub_tab == 0)
        {
          // -----------------------------------------------------------------
          // SUB-VIEW 1: UNIFIED SYNCHRONIZED TIMELINE (GANTT TRACKS + DPS + MANA)
          // -----------------------------------------------------------------
          ImGui::AlignTextToFramePadding();
          ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Synchronized Combat Timeline (Zoomable & Scrollable):");
          ImGui::SameLine();
          ImGui::TextDisabled("| All spell icons, divergence markers, DPS curves, and Mana pools stay time-aligned");

          ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220.0f);
          ImGui::BeginGroup();
          WowResetTextBaseline();
          ImGui::Text("Zoom (px/s):");
          ImGui::SetNextItemWidth(100.0f);
          WowInputFloat("##ZoomInput", &timeline_zoom_px_per_sec, 2.0f, 10.0f, "%.0f");
          if (timeline_zoom_px_per_sec < 4.0f) timeline_zoom_px_per_sec = 4.0f;
          if (timeline_zoom_px_per_sec > 100.0f) timeline_zoom_px_per_sec = 100.0f;
          ImGui::EndGroup();

          ImGui::Spacing();

          // Unified Timeline Dimensions
          float px_per_sec = timeline_zoom_px_per_sec;
          float canvas_w = static_cast<float>(current_run.fight_duration) * px_per_sec + 100.0f;
          // Track layout offsets (relative to p0.y)
          float ruler_y = 4.0f;
          float lane1_y = 26.0f;  // APL Spell Track
          float lane2_y = 74.0f;  // MCTS Optimal Track
          float marker_y = 118.0f; // Divergence Markers
          float dps_plot_y = 150.0f; // Cumulative DPS Curve (Height: 160px)
          float mana_plot_y = 328.0f; // Player Mana Curve (Height: 160px)
          float canvas_h = 510.0f;

          // Scrollable window containing the entire unified canvas
          ImGui::BeginChild("UnifiedTimelineScroll", ImVec2(-1, -1), true, ImGuiWindowFlags_HorizontalScrollbar);
          {
            // PROPER IMGUI LAYOUT: Allocate exact dummy size to prevent SetCursorPos extension warnings
            ImGui::Dummy(ImVec2(canvas_w, canvas_h));

            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p0 = ImGui::GetItemRectMin();
            ImVec2 p1 = ImVec2(p0.x + canvas_w, p0.y + canvas_h);

            // Canvas Background
            dl->AddRectFilled(p0, p1, IM_COL32(18, 18, 22, 255));

            // Time Ruler Ticks & Vertical Grid Lines across all tracks
            for (double t = 0.0; t <= current_run.fight_duration + 0.1; t += 5.0)
            {
              float x = p0.x + static_cast<float>(t) * px_per_sec;
              bool is_major = (std::fmod(t, 10.0) < 0.01);

              // Grid line extending down through spell tracks and graphs
              dl->AddLine(ImVec2(x, p0.y + ruler_y), ImVec2(x, p0.y + canvas_h - 10.0f),
                          is_major ? IM_COL32(65, 65, 75, 160) : IM_COL32(38, 38, 48, 100));

              // Time label
              if (is_major || t == 0.0 || t >= current_run.fight_duration - 1.0)
              {
                std::string t_str = std::to_string(static_cast<int>(t)) + "s";
                dl->AddText(ImVec2(x + 3.0f, p0.y + ruler_y), IM_COL32(190, 190, 190, 255), t_str.c_str());
              }
            }

            // Track Labels
            dl->AddText(ImVec2(p0.x + 6.0f, p0.y + lane1_y - 14.0f), IM_COL32(100, 190, 255, 255), "Active APL Cast Sequence Track:");
            dl->AddText(ImVec2(p0.x + 6.0f, p0.y + lane2_y - 14.0f), IM_COL32(255, 215, 0, 255), "MCTS Optimal Policy Cast Track:");
            dl->AddText(ImVec2(p0.x + 6.0f, p0.y + dps_plot_y - 14.0f), IM_COL32(255, 220, 120, 255), "Cumulative DPS Curves (APL Cyan vs MCTS Gold):");
            dl->AddText(ImVec2(p0.x + 6.0f, p0.y + mana_plot_y - 14.0f), IM_COL32(200, 150, 255, 255), "Player Mana Pools (APL Cyan vs MCTS Purple):");

            ImVec2 mouse_pos = ImGui::GetMousePos();
            bool is_canvas_hovered = (mouse_pos.x >= p0.x && mouse_pos.x <= p1.x && mouse_pos.y >= p0.y && mouse_pos.y <= p1.y);
            double hovered_time = is_canvas_hovered ? std::clamp(static_cast<double>((mouse_pos.x - p0.x) / px_per_sec), 0.0, current_run.fight_duration) : -1.0;

            const TimelineSpellBlock* hovered_apl_block = nullptr;
            const TimelineSpellBlock* hovered_mcts_block = nullptr;
            const APLDivergenceEvent* hovered_divergence = nullptr;

            // -------------------------------------------------------------
            // Track 1: Active APL Cast Icons (Uniform Size, No Background)
            // -------------------------------------------------------------
            const float icon_sz = 30.0f;
            for (const auto& sp : current_run.apl_spells)
            {
              float ix0 = p0.x + static_cast<float>(sp.start_time) * px_per_sec;
              float iy0 = p0.y + lane1_y + 2.0f;
              float ix1 = ix0 + icon_sz;
              float iy1 = iy0 + icon_sz;

              // Render Spell Icon Texture (Always uniform size)
              Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(sp.spell_id));
              dl->AddImage((ImTextureID)(uintptr_t)icon.id, ImVec2(ix0, iy0), ImVec2(ix1, iy1));

              // Clean 1px border
              ImU32 col_border = sp.is_crit ? IM_COL32(255, 215, 0, 255) : IM_COL32(60, 60, 75, 220);
              dl->AddRect(ImVec2(ix0, iy0), ImVec2(ix1, iy1), col_border, 2.0f, 0, sp.is_crit ? 2.0f : 1.0f);

              // Check hover
              if (is_canvas_hovered && mouse_pos.x >= ix0 && mouse_pos.x <= ix1 && mouse_pos.y >= iy0 && mouse_pos.y <= iy1)
              {
                hovered_apl_block = &sp;
                dl->AddRect(ImVec2(ix0 - 1.0f, iy0 - 1.0f), ImVec2(ix1 + 1.0f, iy1 + 1.0f), IM_COL32(255, 255, 255, 255), 2.0f, 0, 2.0f);
              }
            }

            // -------------------------------------------------------------
            // Track 2: MCTS Optimal Cast Icons (Uniform Size, No Background)
            // -------------------------------------------------------------
            for (const auto& sp : current_run.mcts_spells)
            {
              float ix0 = p0.x + static_cast<float>(sp.start_time) * px_per_sec;
              float iy0 = p0.y + lane2_y + 2.0f;
              float ix1 = ix0 + icon_sz;
              float iy1 = iy0 + icon_sz;

              Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(sp.spell_id));
              dl->AddImage((ImTextureID)(uintptr_t)icon.id, ImVec2(ix0, iy0), ImVec2(ix1, iy1));

              ImU32 col_border = sp.is_crit ? IM_COL32(255, 215, 0, 255) : IM_COL32(210, 180, 70, 220);
              dl->AddRect(ImVec2(ix0, iy0), ImVec2(ix1, iy1), col_border, 2.0f, 0, sp.is_crit ? 2.0f : 1.0f);

              if (is_canvas_hovered && mouse_pos.x >= ix0 && mouse_pos.x <= ix1 && mouse_pos.y >= iy0 && mouse_pos.y <= iy1)
              {
                hovered_mcts_block = &sp;
                dl->AddRect(ImVec2(ix0 - 1.0f, iy0 - 1.0f), ImVec2(ix1 + 1.0f, iy1 + 1.0f), IM_COL32(255, 255, 255, 255), 2.0f, 0, 2.0f);
              }
            }

            // -------------------------------------------------------------
            // Track 3: Rotational Divergence Markers
            // -------------------------------------------------------------
            for (const auto& ev : current_run.events)
            {
              float mx = p0.x + static_cast<float>(ev.timestamp) * px_per_sec;

              // Glowing vertical divergence line across tracks
              dl->AddLine(ImVec2(mx, p0.y + lane1_y), ImVec2(mx, p0.y + marker_y + 12.0f), IM_COL32(255, 215, 0, 180), 2.0f);

              // Marker Triangle Flag
              ImVec2 flag_a(mx - 6.0f, p0.y + marker_y);
              ImVec2 flag_b(mx + 6.0f, p0.y + marker_y);
              ImVec2 flag_c(mx, p0.y + marker_y + 10.0f);
              dl->AddTriangleFilled(flag_a, flag_b, flag_c, IM_COL32(255, 215, 0, 255));

              if (is_canvas_hovered && std::abs(mouse_pos.x - mx) <= 8.0f && mouse_pos.y >= p0.y + lane1_y && mouse_pos.y <= p0.y + marker_y + 20.0f)
              {
                hovered_divergence = &ev;
              }
            }

            // -------------------------------------------------------------
            // Track 4: Synchronized Cumulative DPS Graph
            // -------------------------------------------------------------
            float dps_h = 160.0f;
            ImVec2 dps_box_min(p0.x + 4.0f, p0.y + dps_plot_y);
            ImVec2 dps_box_max(p0.x + canvas_w - 40.0f, p0.y + dps_plot_y + dps_h);

            dl->AddRectFilled(dps_box_min, dps_box_max, IM_COL32(12, 14, 18, 240), 4.0f);
            dl->AddRect(dps_box_min, dps_box_max, IM_COL32(50, 50, 60, 200), 4.0f);

            // Compute Max DPS for Y-Scaling
            double max_dps_val = 100.0;
            for (double v : current_run.ts_mcts_dps) max_dps_val = std::max(max_dps_val, v);
            for (double v : current_run.ts_apl_dps) max_dps_val = std::max(max_dps_val, v);
            max_dps_val = std::ceil(max_dps_val / 100.0) * 100.0 + 50.0;

            // DPS Horizontal Grid Lines & Scale Labels
            for (int d = 100; d < static_cast<int>(max_dps_val); d += 100)
            {
              float gy = dps_box_max.y - static_cast<float>(d / max_dps_val) * (dps_h - 10.0f);
              dl->AddLine(ImVec2(dps_box_min.x, gy), ImVec2(dps_box_max.x, gy), IM_COL32(40, 45, 55, 120));
              std::string label = std::to_string(d) + " DPS";
              dl->AddText(ImVec2(dps_box_min.x + 6.0f, gy - 12.0f), IM_COL32(140, 140, 150, 180), label.c_str());
            }

            // Draw DPS Lines
            if (current_run.ts_time.size() >= 2)
            {
              for (size_t i = 0; i < current_run.ts_time.size() - 1; ++i)
              {
                float x_a = p0.x + static_cast<float>(current_run.ts_time[i]) * px_per_sec;
                float x_b = p0.x + static_cast<float>(current_run.ts_time[i + 1]) * px_per_sec;

                float apl_y_a = dps_box_max.y - static_cast<float>(current_run.ts_apl_dps[i] / max_dps_val) * (dps_h - 10.0f);
                float apl_y_b = dps_box_max.y - static_cast<float>(current_run.ts_apl_dps[i + 1] / max_dps_val) * (dps_h - 10.0f);

                float mcts_y_a = dps_box_max.y - static_cast<float>(current_run.ts_mcts_dps[i] / max_dps_val) * (dps_h - 10.0f);
                float mcts_y_b = dps_box_max.y - static_cast<float>(current_run.ts_mcts_dps[i + 1] / max_dps_val) * (dps_h - 10.0f);

                // Area fill between APL and MCTS where MCTS is higher
                if (mcts_y_a < apl_y_a || mcts_y_b < apl_y_b)
                {
                  ImVec2 poly[4] = {ImVec2(x_a, apl_y_a), ImVec2(x_b, apl_y_b), ImVec2(x_b, mcts_y_b), ImVec2(x_a, mcts_y_a)};
                  dl->AddConvexPolyFilled(poly, 4, IM_COL32(255, 215, 0, 40));
                }

                // APL DPS Line (Cyan)
                dl->AddLine(ImVec2(x_a, apl_y_a), ImVec2(x_b, apl_y_b), IM_COL32(79, 195, 247, 255), 2.0f);

                // MCTS DPS Line (Gold)
                dl->AddLine(ImVec2(x_a, mcts_y_a), ImVec2(x_b, mcts_y_b), IM_COL32(255, 215, 0, 255), 2.5f);
              }
            }

            // -------------------------------------------------------------
            // Track 5: Synchronized Player Mana Graph
            // -------------------------------------------------------------
            float mana_h = 160.0f;
            ImVec2 mana_box_min(p0.x + 4.0f, p0.y + mana_plot_y);
            ImVec2 mana_box_max(p0.x + canvas_w - 40.0f, p0.y + mana_plot_y + mana_h);

            dl->AddRectFilled(mana_box_min, mana_box_max, IM_COL32(12, 14, 18, 240), 4.0f);
            dl->AddRect(mana_box_min, mana_box_max, IM_COL32(50, 50, 60, 200), 4.0f);

            double max_mana_val = 1000.0;
            for (double v : current_run.ts_mcts_mana) max_mana_val = std::max(max_mana_val, v);
            for (double v : current_run.ts_apl_mana) max_mana_val = std::max(max_mana_val, v);
            max_mana_val = std::ceil(max_mana_val / 500.0) * 500.0;

            // Mana Grid Lines
            for (int m = 1000; m < static_cast<int>(max_mana_val); m += 1000)
            {
              float gy = mana_box_max.y - static_cast<float>(m / max_mana_val) * (mana_h - 10.0f);
              dl->AddLine(ImVec2(mana_box_min.x, gy), ImVec2(mana_box_max.x, gy), IM_COL32(40, 45, 55, 120));
              std::string label = std::to_string(m) + " Mana";
              dl->AddText(ImVec2(mana_box_min.x + 6.0f, gy - 12.0f), IM_COL32(140, 140, 150, 180), label.c_str());
            }

            // Draw Mana Lines
            if (current_run.ts_time.size() >= 2)
            {
              for (size_t i = 0; i < current_run.ts_time.size() - 1; ++i)
              {
                float x_a = p0.x + static_cast<float>(current_run.ts_time[i]) * px_per_sec;
                float x_b = p0.x + static_cast<float>(current_run.ts_time[i + 1]) * px_per_sec;

                float apl_m_a = mana_box_max.y - static_cast<float>(current_run.ts_apl_mana[i] / max_mana_val) * (mana_h - 10.0f);
                float apl_m_b = mana_box_max.y - static_cast<float>(current_run.ts_apl_mana[i + 1] / max_mana_val) * (mana_h - 10.0f);

                float mcts_m_a = mana_box_max.y - static_cast<float>(current_run.ts_mcts_mana[i] / max_mana_val) * (mana_h - 10.0f);
                float mcts_m_b = mana_box_max.y - static_cast<float>(current_run.ts_mcts_mana[i + 1] / max_mana_val) * (mana_h - 10.0f);

                // APL Mana (Cyan)
                dl->AddLine(ImVec2(x_a, apl_m_a), ImVec2(x_b, apl_m_b), IM_COL32(79, 195, 247, 255), 2.0f);

                // MCTS Mana (Purple)
                dl->AddLine(ImVec2(x_a, mcts_m_a), ImVec2(x_b, mcts_m_b), IM_COL32(206, 147, 216, 255), 2.5f);
              }
            }

            // -------------------------------------------------------------
            // Synchronized Global Mouse Time Cursor & Integrated Tooltip
            // -------------------------------------------------------------
            if (is_canvas_hovered && hovered_time >= 0.0)
            {
              float cx = p0.x + static_cast<float>(hovered_time) * px_per_sec;
              dl->AddLine(ImVec2(cx, p0.y + ruler_y), ImVec2(cx, p0.y + canvas_h - 10.0f), IM_COL32(255, 255, 255, 180), 1.5f);

              // Interpolate Time Series at Hovered Cursor Time
              double cur_apl_dps = 0.0;
              double cur_mcts_dps = 0.0;
              double cur_apl_mana = 0.0;
              double cur_mcts_mana = 0.0;

              if (!current_run.ts_time.empty())
              {
                size_t ts_idx = std::min(static_cast<size_t>(hovered_time * 2.0), current_run.ts_time.size() - 1);
                cur_apl_dps = current_run.ts_apl_dps[ts_idx];
                cur_mcts_dps = current_run.ts_mcts_dps[ts_idx];
                cur_apl_mana = current_run.ts_apl_mana[ts_idx];
                cur_mcts_mana = current_run.ts_mcts_mana[ts_idx];
              }

              ImGui::BeginTooltip();
              ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Combat Timeline Cursor: %.1fs / %.1fs", hovered_time, current_run.fight_duration);
              ImGui::Separator();

              // Hovered Spell details
              if (hovered_apl_block)
              {
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "APL Cast: %s (%.1fs -> %.1fs)",
                                   hovered_apl_block->spell_name.c_str(), hovered_apl_block->start_time, hovered_apl_block->end_time);
                if (hovered_apl_block->damage > 0.0)
                {
                  ImGui::TextColored(hovered_apl_block->is_crit ? ImVec4(1.0f, 0.85f, 0.0f, 1.0f) : ImVec4(0.4f, 0.9f, 0.4f, 1.0f),
                                     "  Damage: %.0f %s (%s)", hovered_apl_block->damage, hovered_apl_block->is_crit ? "[CRIT]" : "", hovered_apl_block->tag.c_str());
                }
              }

              if (hovered_mcts_block)
              {
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "MCTS Cast: %s (%.1fs -> %.1fs)",
                                   hovered_mcts_block->spell_name.c_str(), hovered_mcts_block->start_time, hovered_mcts_block->end_time);
                if (hovered_mcts_block->damage > 0.0)
                {
                  ImGui::TextColored(hovered_mcts_block->is_crit ? ImVec4(1.0f, 0.85f, 0.0f, 1.0f) : ImVec4(0.4f, 0.9f, 0.4f, 1.0f),
                                     "  Damage: %.0f %s (%s)", hovered_mcts_block->damage, hovered_mcts_block->is_crit ? "[CRIT]" : "", hovered_mcts_block->tag.c_str());
                }
              }

              if (hovered_divergence)
              {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "⚡ Rotational Divergence Detected at %.1fs:", hovered_divergence->timestamp);
                ImGui::Text("  APL:  %s (%.1f DPS)", hovered_divergence->apl_action_name.c_str(), hovered_divergence->apl_expected_dps);
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "  MCTS: %s (%.1f DPS)  |  +%.1f DPS (%.1f%% conf)",
                                   hovered_divergence->mcts_action_name.c_str(), hovered_divergence->mcts_expected_dps,
                                   hovered_divergence->delta_dps, hovered_divergence->confidence_pct);
                ImGui::TextWrapped("  %s", hovered_divergence->rationale.c_str());
              }

              ImGui::Spacing();
              ImGui::Separator();
              ImGui::Text("Throughput at %.1fs:", hovered_time);
              ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "  APL:  %.1f DPS  |  %.0f Mana", cur_apl_dps, cur_apl_mana);
              ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "  MCTS: %.1f DPS  |  %.0f Mana  (ΔDPS: +%.1f)",
                                 cur_mcts_dps, cur_mcts_mana, std::max(0.0, cur_mcts_dps - cur_apl_dps));

              ImGui::EndTooltip();
            }
          }
          ImGui::EndChild();
        }
        else
        {
          // -----------------------------------------------------------------
          // SUB-VIEW 2: APL BLUNDER MATRIX & DECISION JUDGE TABLE
          // -----------------------------------------------------------------
          ImGui::AlignTextToFramePadding();
          ImGui::Text("Filter:");
          ImGui::SameLine();
          ImGui::SetNextItemWidth(180.0f);
          ImGui::InputTextWithHint("##TraceFilter", "Search rationale or spell...", text_filter, IM_ARRAYSIZE(text_filter));

          ImGui::SameLine(0, 16.0f);
          ImGui::TextDisabled("Showing %zu detected APL blunders for this episode, ranked from worst move to least severe.", current_run.events.size());

          ImGui::Spacing();

          if (current_run.events.empty())
          {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Optimal Run! The active APL made 100%% optimal choices matching MCTS forward rollouts throughout this entire episode.");
          }
          else
          {
            static ImGuiTableFlags trace_table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                                       ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
            if (ImGui::BeginTable("BlunderMatrixTable", 8, trace_table_flags, ImVec2(0, 0)))
            {
              ImGui::TableSetupColumn("Rank / Severity", ImGuiTableColumnFlags_WidthFixed, 110.0f);
              ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 65.0f);
              ImGui::TableSetupColumn("Combat Context", ImGuiTableColumnFlags_WidthFixed, 170.0f);
              ImGui::TableSetupColumn("APL Move Made", ImGuiTableColumnFlags_WidthFixed, 160.0f);
              ImGui::TableSetupColumn("MCTS Optimal Move", ImGuiTableColumnFlags_WidthFixed, 160.0f);
              ImGui::TableSetupColumn("DPS Loss (95% CI)", ImGuiTableColumnFlags_WidthFixed, 130.0f);
              ImGui::TableSetupColumn("Confidence", ImGuiTableColumnFlags_WidthFixed, 85.0f);
              ImGui::TableSetupColumn("Strategic Rationale & Alternative Values", ImGuiTableColumnFlags_WidthStretch);
              ImGui::TableHeadersRow();

              std::string filter_str = text_filter;
              std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

              for (size_t ev_idx = 0; ev_idx < current_run.events.size(); ++ev_idx)
              {
                const auto& ev = current_run.events[ev_idx];

                // Filter check
                if (!filter_str.empty())
                {
                  std::string combined = ev.apl_action_name + " " + ev.mcts_action_name + " " + ev.rationale + " " + ev.top_alternatives_summary;
                  std::transform(combined.begin(), combined.end(), combined.begin(), ::tolower);
                  if (combined.find(filter_str) == std::string::npos)
                    continue;
                }

                ImGui::TableNextRow();

                // Col 0: Rank / Severity
                ImGui::TableNextColumn();
                ImVec4 sev_col;
                const char* sev_tag;
                if (ev.delta_dps >= 15.0)
                {
                  sev_col = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
                  sev_tag = "[CRITICAL]";
                }
                else if (ev.delta_dps >= 5.0)
                {
                  sev_col = ImVec4(1.0f, 0.65f, 0.2f, 1.0f);
                  sev_tag = "[MAJOR]";
                }
                else
                {
                  sev_col = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);
                  sev_tag = "[MINOR]";
                }
                ImGui::TextColored(sev_col, "#%zu %s", ev.blunder_rank, sev_tag);

                // Col 1: Time
                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%04.1fs", ev.timestamp);
                ImGui::TextDisabled("(%.0f%%)", (ev.timestamp / std::max(1.0, current_run.fight_duration)) * 100.0);

                // Col 2: Combat Context
                ImGui::TableNextColumn();
                ImGui::Text("Mana: %d%% | HP: %d%%",
                            static_cast<int>(ev.state.player_mana_pct * 100.0f),
                            static_cast<int>(ev.state.target_hp_pct * 100.0f));
                
                std::string dots_summary;
                if (ev.state.dot_corruption_rem_sec > 0.0f) dots_summary += "Corr:" + std::to_string(static_cast<int>(ev.state.dot_corruption_rem_sec)) + "s ";
                if (ev.state.dot_immolate_rem_sec > 0.0f) dots_summary += "Immo:" + std::to_string(static_cast<int>(ev.state.dot_immolate_rem_sec)) + "s ";
                if (ev.state.dot_doom_rem_sec > 0.0f) dots_summary += "Doom:" + std::to_string(static_cast<int>(ev.state.dot_doom_rem_sec)) + "s ";
                if (ev.state.dot_agony_rem_sec > 0.0f) dots_summary += "Agony:" + std::to_string(static_cast<int>(ev.state.dot_agony_rem_sec)) + "s ";
                if (ev.state.isb_charges_rem > 0.0f) dots_summary += "ISB(" + std::to_string(static_cast<int>(ev.state.isb_charges_rem)) + ") ";
                if (ev.state.nightfall_proc_active > 0.5f) dots_summary += "Trance ";
                if (dots_summary.empty()) dots_summary = "No DoTs active";
                ImGui::TextDisabled("%s", dots_summary.c_str());

                // Col 3: APL Move Made
                ImGui::TableNextColumn();
                const auto& apl_icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(ev.apl_action)));
                rlImGuiImageSize(&apl_icon, 16, 16);
                ImGui::SameLine(0, 4.0f);
                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", ev.apl_action_name.c_str());
                ImGui::TextDisabled("%.1f ± %.1f DPS", ev.apl_expected_dps, ev.apl_std_error);

                // Col 4: MCTS Optimal Move
                ImGui::TableNextColumn();
                const auto& mcts_icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(ev.mcts_action)));
                rlImGuiImageSize(&mcts_icon, 16, 16);
                ImGui::SameLine(0, 4.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", ev.mcts_action_name.c_str());
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 0.8f), "%.1f ± %.1f DPS", ev.mcts_expected_dps, ev.mcts_std_error);

                // Col 5: DPS Loss (95% CI)
                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "-%.1f DPS", ev.delta_dps);
                ImGui::TextDisabled("CI: [%.1f, %.1f]", ev.delta_dps_ci_lower, ev.delta_dps_ci_upper);

                // Col 6: Confidence
                ImGui::TableNextColumn();
                ImVec4 conf_col = (ev.confidence_pct >= 95.0) ? ImVec4(0.2f, 0.9f, 0.4f, 1.0f) :
                                  (ev.confidence_pct >= 85.0) ? ImVec4(1.0f, 0.85f, 0.3f, 1.0f) : ImVec4(0.9f, 0.6f, 0.2f, 1.0f);
                ImGui::TextColored(conf_col, "%.1f%%", ev.confidence_pct);
                ImGui::TextDisabled("Z = %.1f", ev.z_score);

                // Col 7: Strategic Rationale & Top Alternatives
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", ev.rationale.c_str());

                if (!ev.top_alternatives_summary.empty())
                {
                  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 0.9f), "Other Viable Actions: %s", ev.top_alternatives_summary.c_str());
                }

                if (!ev.candidate_evals.empty())
                {
                  std::string btn_id = (expanded_event_idx == static_cast<int>(ev_idx)) ? "Hide Full Branch Rankings##" : "Inspect All Candidate Action Values##";
                  btn_id += std::to_string(ev_idx);
                  if (ImGui::SmallButton(btn_id.c_str()))
                  {
                    expanded_event_idx = (expanded_event_idx == static_cast<int>(ev_idx)) ? -1 : static_cast<int>(ev_idx);
                  }

                  if (expanded_event_idx == static_cast<int>(ev_idx))
                  {
                    ImGui::Indent(12.0f);
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "MCTS Branch Rollout Comparison from this Exact State (%zu rollouts/branch):",
                                       ev.candidate_evals[0].rollout_count);

                    for (size_t c_idx = 0; c_idx < ev.candidate_evals.size(); ++c_idx)
                    {
                      const auto& cand = ev.candidate_evals[c_idx];
                      bool is_optimal = (cand.action == ev.mcts_action);
                      bool is_apl = (cand.action == ev.apl_action);

                      ImVec4 c_col = is_optimal ? ImVec4(1.0f, 0.85f, 0.3f, 1.0f) :
                                     is_apl ? ImVec4(0.4f, 0.75f, 1.0f, 1.0f) : ImVec4(0.75f, 0.75f, 0.75f, 1.0f);

                      double diff_vs_apl = cand.mean_dps - ev.apl_expected_dps;

                      const auto& cand_icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(cand.action)));
                      rlImGuiImageSize(&cand_icon, 14, 14);
                      ImGui::SameLine(0, 4.0f);

                      ImGui::TextColored(c_col, "#%zu %s: %.1f DPS (±%.1f, 95%% CI: [%.1f, %.1f]) | Δ vs APL: %s%.1f DPS %s%s",
                                         c_idx + 1,
                                         cand.name.c_str(),
                                         cand.mean_dps,
                                         cand.std_error,
                                         cand.ci_lower_95,
                                         cand.ci_upper_95,
                                         (diff_vs_apl >= 0.0 ? "+" : ""),
                                         diff_vs_apl,
                                         is_optimal ? " [OPTIMAL]" : "",
                                         is_apl ? " [APL MOVE]" : "");
                    }
                    ImGui::Unindent(12.0f);
                  }
                }
              }
              ImGui::EndTable();
            }
          }
        }
      }
      EndWowChild();
    }
  }
}

} // namespace warlock
