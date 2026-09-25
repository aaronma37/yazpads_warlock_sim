#pragma once
#include "asset_manager.hpp"
#include "damage_breakdown_view.hpp"
#include "wow_widgets.hpp"
#include "imgui.h"
#include "src/sim/warlock/viper_oracle.hpp"
#include "src/sim/warlock/apl_analyzer.hpp"
#include "src/sim/warlock/policy.hpp"
#include "src/sim/warlock/warlock_sim.hpp"
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

// Background worker state for APL Synthesis (VIPER + DAgger)
struct APLSynthesizerWorkerState
{
  std::thread worker;
  std::mutex mtx;
  std::atomic<bool> is_running{false};
  std::atomic<float> progress{0.0f};
  std::string current_status;
  VIPEROracle::VIPERExtractionResult live_result;
  bool has_result = false;
  float apl_applied_timer = 0.0f;

  void set_status(float p, const std::string& msg)
  {
    progress.store(p, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(mtx);
    current_status = msg;
  }

  std::string get_status()
  {
    std::lock_guard<std::mutex> lock(mtx);
    return current_status;
  }
};

inline APLSynthesizerWorkerState& get_apl_synthesizer_worker_state()
{
  static APLSynthesizerWorkerState state;
  return state;
}

inline void render_panel_synthesize_apl(WarlockSimulator& sim, AppTab* switch_tab = nullptr)
{
  auto& worker = get_apl_synthesizer_worker_state();

#if defined(__EMSCRIPTEN__)
  static int synth_threads = get_browser_max_threads();
#else
  static int synth_threads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
#endif
  static int synth_dagger_iterations = 5;
  static int synth_rollouts_per_action = 512;
  static bool synth_adaptive_rollouts = true;
  static int synth_episodes_per_pass = 30;
  static int synth_benchmark_iters = 500;
  static char rule_search_filter[64] = "";

  // Join finished thread
  {
    std::lock_guard<std::mutex> lock(worker.mtx);
    if (!worker.is_running.load() && worker.worker.joinable())
    {
      worker.worker.join();
    }
  }

  bool is_busy = worker.is_running.load();
  if (worker.apl_applied_timer > 0.0f)
  {
    worker.apl_applied_timer -= ImGui::GetIO().DeltaTime;
  }

  float full_w = ImGui::GetContentRegionAvail().x;
  float top_h = 108.0f;

  // -------------------------------------------------------------------------
  // TOP SECTION: Configuration & Launch Card
  // -------------------------------------------------------------------------
  BeginWowChild("TopCard_SynthesizeAPL", ImVec2(0, top_h), true);
  {
    // Group 0: Threads
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Threads:");
    ImGui::SetNextItemWidth(55.0f);
    if (is_busy) ImGui::BeginDisabled();
    WowInputInt("##SynthThreadsInput", &synth_threads, 1, 4);
    if (synth_threads < 1) synth_threads = 1;
#if defined(__EMSCRIPTEN__)
    int max_wasm_threads = get_browser_max_threads();
    if (synth_threads > max_wasm_threads) synth_threads = max_wasm_threads;
#else
    if (synth_threads > 128) synth_threads = 128;
#endif
    if (is_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 8.0f);

    // Group 1: DAgger Passes
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("DAgger Passes:");
    ImGui::SetNextItemWidth(65.0f);
    if (is_busy) ImGui::BeginDisabled();
    WowInputInt("##SynthDAggerItersInput", &synth_dagger_iterations, 1, 3);
    if (synth_dagger_iterations < 1) synth_dagger_iterations = 1;
    if (synth_dagger_iterations > 25) synth_dagger_iterations = 25;
    if (is_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 10.0f);

    // Group 2: Max Rollouts / Action
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text(synth_adaptive_rollouts ? "Max Rollouts:" : "Rollouts / Action:");
    ImGui::SetNextItemWidth(90.0f);
    if (is_busy) ImGui::BeginDisabled();
    WowInputInt("##SynthRolloutsInput", &synth_rollouts_per_action, 64, 128);
    if (synth_rollouts_per_action < 32) synth_rollouts_per_action = 32;
    if (synth_rollouts_per_action > 2048) synth_rollouts_per_action = 2048;
    if (is_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 8.0f);

    // Group 3: Adaptive Toggle
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Dummy(ImVec2(0, 1.0f));
    if (is_busy) ImGui::BeginDisabled();
    WowCheckbox("Adaptive", &synth_adaptive_rollouts);
    if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip("Statistically early-stops and prunes inferior candidate branches when >= 99%% confidence\nseparation is reached, hard-capped at Max Rollouts (default 512).");
    }
    if (is_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 10.0f);

    // Group 4: Episodes / Pass
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Episodes / Pass:");
    ImGui::SetNextItemWidth(75.0f);
    if (is_busy) ImGui::BeginDisabled();
    WowInputInt("##SynthEpisodesInput", &synth_episodes_per_pass, 5, 10);
    if (synth_episodes_per_pass < 8) synth_episodes_per_pass = 8;
    if (synth_episodes_per_pass > 200) synth_episodes_per_pass = 200;
    if (is_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 10.0f);

    // Group 5: Benchmark Precision
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Benchmark Iters:");
    ImGui::SetNextItemWidth(80.0f);
    if (is_busy) ImGui::BeginDisabled();
    WowInputInt("##SynthBenchmarkItersInput", &synth_benchmark_iters, 100, 250);
    if (synth_benchmark_iters < 100) synth_benchmark_iters = 100;
    if (synth_benchmark_iters > 5000) synth_benchmark_iters = 5000;
    if (is_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 16.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

    if (is_busy)
    {
      std::string s_status = worker.get_status();
      ImGui::ProgressBar(worker.progress.load(), ImVec2(220.0f, 26.0f), s_status.c_str());
    }
    else
    {
      if (WowButton("Synthesize Optimal APL", ImVec2(210.0f, 26.0f)))
      {
        worker.is_running = true;
        worker.has_result = false;
        worker.set_status(0.02f, "Initializing synthesis session...");

        if (worker.worker.joinable())
        {
          worker.worker.join();
        }

        WarlockSimulator sim_copy = sim;
        int d_iters = synth_dagger_iterations;
        int eps = synth_episodes_per_pass * synth_dagger_iterations;
        int b_iters = synth_benchmark_iters;

        worker.worker = std::thread([sim_copy, eps, b_iters, d_iters]() {
          auto& w = get_apl_synthesizer_worker_state();
          WarlockSimulator local_sim = sim_copy;

          VIPEROracle::VIPERExtractionResult res = VIPEROracle::extract_viper_apl(
              local_sim,
              eps,
              b_iters,
              d_iters,
              [&](float p, const std::string& msg) {
                w.set_status(p, msg);
              },
              1337);

          {
            std::lock_guard<std::mutex> lock(w.mtx);
            w.live_result = std::move(res);
            w.has_result = true;
            w.is_running = false;
          }
          w.set_status(1.0f, "Synthesis Complete!");
        });
      }
    }
  }
  EndWowChild();

  ImGui::Spacing();

  // -------------------------------------------------------------------------
  // BOTTOM SECTION: Synthesized Results & Multi-Tab Inspection
  // -------------------------------------------------------------------------
  BeginWowChild("BottomSynthesizeResultsPane", ImVec2(0, 0), true);
  {
    if (!worker.has_result)
    {
      ImGui::Spacing();
      ImGui::Indent(20.0f);
      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "APL Policy Synthesis Ready");
      ImGui::Spacing();
      ImGui::TextWrapped("Click 'Synthesize Optimal APL' above to launch the autonomous MCTS-guided DAgger optimization process.");
      ImGui::Spacing();
      ImGui::TextWrapped("The engine will evaluate optimal forward returns, discover continuous predicate thresholds (Life Tap mana boundaries, Bane of Doom cutoffs, Pandemic DoT windows), and iteratively aggregate recovery states over %d DAgger passes.", synth_dagger_iterations);
      ImGui::Unindent(20.0f);
    }
    else
    {
      const auto& res = worker.live_result;

      // Top KPI Cards (4 Expected Value Cards)
      float avail_w = ImGui::GetContentRegionAvail().x;
      float card_w = (avail_w - 3.0f * 10.0f) / 4.0f;
      float card_h = 72.0f;

      // Card 1: Baseline Expected DPS
      BeginWowChild("SynthKPI1", ImVec2(card_w, card_h), true);
      {
        ImGui::TextDisabled("1. BASELINE POLICY DPS");
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%.1f ± %.1f DPS", res.baseline_expected_dps, res.baseline_dps_stddev);
        ImGui::TextDisabled("Range: [%.1f - %.1f DPS]", res.baseline_min_dps, res.baseline_max_dps);
      }
      EndWowChild();

      ImGui::SameLine(0, 10.0f);

      // Card 2: Synthesized APL (Interpretable)
      BeginWowChild("SynthKPI2", ImVec2(card_w, card_h), true);
      {
        ImGui::TextDisabled("2. SYNTHESIZED APL (RULES)");
        ImVec4 gain_col = (res.viper_gain_over_baseline > 0.5) ? ImVec4(0.2f, 0.95f, 0.35f, 1.0f) : ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        ImGui::TextColored(gain_col, "%.1f ± %.1f DPS (+%.1f%%)", res.viper_expected_dps, res.viper_dps_stddev, res.viper_gain_pct);
        ImGui::TextDisabled("Captured %.1f%% Ceiling | %.1f%% Fid", res.oracle_potential_captured_pct, res.oracle_agreement_fidelity_pct);
      }
      EndWowChild();

      ImGui::SameLine(0, 10.0f);

      // Card 3: GBDT Q-Policy (LightGBM/Trees)
      BeginWowChild("SynthKPI3", ImVec2(card_w, card_h), true);
      {
        ImGui::TextDisabled("3. GBDT Q-POLICY (LIGHTGBM)");
        ImVec4 gbdt_col = (res.gbdt_policy_gain_pct > 0.5) ? ImVec4(0.3f, 0.9f, 1.0f, 1.0f) : ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        ImGui::TextColored(gbdt_col, "%.1f ± %.1f DPS (+%.1f%%)", res.gbdt_policy_expected_dps, res.gbdt_policy_dps_stddev, res.gbdt_policy_gain_pct);
        ImGui::TextDisabled("Captured %.1f%% of MCTS Ceiling", res.gbdt_potential_captured_pct);
      }
      EndWowChild();

      ImGui::SameLine(0, 10.0f);

      // Card 4: MCTS Oracle Ceiling
      BeginWowChild("SynthKPI4", ImVec2(card_w, card_h), true);
      {
        ImGui::TextDisabled("4. MCTS ORACLE CEILING");
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%.1f DPS (+%.1f%%)", res.oracle_expected_dps, res.oracle_gain_pct);
        ImGui::TextDisabled("Theoretical Upper Bound: +%.1f DPS", res.oracle_expected_gain);
      }
      EndWowChild();

      ImGui::Spacing();

      // Action Bar: Apply Synthesized APL vs Apply GBDT Q-Policy
      if (WowButton("Apply Synthesized APL to Sim", ImVec2(240.0f, 26.0f)))
      {
        sim.policy.custom_rules = res.extracted_rules;
        sim.policy.use_custom_apl = true;
        sim.policy.use_gbdt_policy = false;
        worker.apl_applied_timer = 4.0f;
      }

      ImGui::SameLine(0, 10.0f);

      if (WowButton("Apply GBDT Q-Policy to Sim", ImVec2(240.0f, 26.0f)))
      {
        sim.policy.gbdt_q_policy = std::make_shared<sim::GBDTMultiActionQPolicy>(res.gbdt_q_policy);
        sim.policy.use_gbdt_policy = true;
        sim.policy.use_custom_apl = false;
        worker.apl_applied_timer = 4.0f;
      }

      if (worker.apl_applied_timer > 0.0f)
      {
        ImGui::SameLine(0, 12.0f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.35f, 1.0f), "Policy successfully applied to active sim!");
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Tab bar for deep inspection
      if (ImGui::BeginTabBar("SynthesizedAPLTabBar", ImGuiTabBarFlags_None))
      {
        // -------------------------------------------------------------------
        // TAB 1: Synthesized Priority List (APL)
        // -------------------------------------------------------------------
        if (ImGui::BeginTabItem("Synthesized Priority List (APL)"))
        {
          ImGui::Spacing();
          ImGui::AlignTextToFramePadding();
          ImGui::Text("Filter:");
          ImGui::SameLine();
          ImGui::SetNextItemWidth(200.0f);
          ImGui::InputTextWithHint("##SynthFilter", "Search ability...", rule_search_filter, IM_ARRAYSIZE(rule_search_filter));

          ImGui::SameLine(0, 16.0f);
          ImGui::TextDisabled("Total Synthesized Priority Rules: %zu", res.extracted_rules.size());

          ImGui::Spacing();

          std::string f_str = rule_search_filter;
          std::transform(f_str.begin(), f_str.end(), f_str.begin(), ::tolower);

          static ImGuiTableFlags apl_tbl_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
          if (ImGui::BeginTable("SynthesizedRulesTable", 4, apl_tbl_flags, ImVec2(0, 0)))
          {
            ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed, 65.0f);
            ImGui::TableSetupColumn("Ability / Spell", ImGuiTableColumnFlags_WidthFixed, 220.0f);
            ImGui::TableSetupColumn("Extracted Continuous Levers & Conditions", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Rule Rationale", ImGuiTableColumnFlags_WidthFixed, 240.0f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < res.extracted_rules.size(); ++i)
            {
              const auto& r = res.extracted_rules[i];

              if (!f_str.empty())
              {
                std::string match_text = r.name + " " + r.condition_summary + " " + r.rule_explanation;
                std::transform(match_text.begin(), match_text.end(), match_text.begin(), ::tolower);
                if (match_text.find(f_str) == std::string::npos)
                  continue;
              }

              ImGui::TableNextRow();

              // Col 0: Priority Rank
              ImGui::TableNextColumn();
              ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "#%zu", i + 1);

              // Col 1: Spell Icon & Name
              ImGui::TableNextColumn();
              const auto& icon = AssetManager::get().get_icon(spell_id_to_icon(r.spell_id));
              rlImGuiImageSize(&icon, 16, 16);
              ImGui::SameLine(0, 5.0f);
              ImGui::TextColored(ImVec4(0.92f, 0.85f, 0.72f, 1.0f), "%s", r.name.c_str());

              // Col 2: Continuous Levers / Conditions
              ImGui::TableNextColumn();
              if (r.use_custom_thresholds && !r.condition_summary.empty() && r.condition_summary != "Default" && r.condition_summary != "(Always)")
              {
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", r.condition_summary.c_str());
              }
              else
              {
                ImGui::TextDisabled("(Always / Unconditional)");
              }

              // Col 3: Explanation
              ImGui::TableNextColumn();
              ImGui::TextDisabled("%s", r.rule_explanation.c_str());
            }
            ImGui::EndTable();
          }

          ImGui::EndTabItem();
        }

        // -------------------------------------------------------------------
        // TAB 2: Rule Diff & Lever Modifications
        // -------------------------------------------------------------------
        if (ImGui::BeginTabItem("Rule Diff & Priority Shifts"))
        {
          ImGui::Spacing();
          ImGui::TextDisabled("Comparison between baseline preset priority and the newly synthesized APL:");
          ImGui::Spacing();

          static ImGuiTableFlags diff_tbl_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
          if (ImGui::BeginTable("RuleDiffTable", 5, diff_tbl_flags, ImVec2(0, 0)))
          {
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Baseline Rank", ImGuiTableColumnFlags_WidthFixed, 105.0f);
            ImGui::TableSetupColumn("Synthesized Rank", ImGuiTableColumnFlags_WidthFixed, 115.0f);
            ImGui::TableSetupColumn("Shift Type", ImGuiTableColumnFlags_WidthFixed, 125.0f);
            ImGui::TableSetupColumn("Rank Delta", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableHeadersRow();

            for (const auto& shift : res.rule_shifts)
            {
              ImGui::TableNextRow();

              // Col 0: Action Name
              ImGui::TableNextColumn();
              const auto& icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(shift.action)));
              rlImGuiImageSize(&icon, 16, 16);
              ImGui::SameLine(0, 5.0f);
              ImGui::Text("%s", shift.name.c_str());

              // Col 1: Baseline Rank
              ImGui::TableNextColumn();
              if (shift.baseline_rank > 0)
                ImGui::Text("#%d", shift.baseline_rank);
              else
                ImGui::TextDisabled("N/A");

              // Col 2: Synthesized Rank
              ImGui::TableNextColumn();
              if (shift.viper_rank > 0)
                ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "#%d", shift.viper_rank);
              else
                ImGui::TextDisabled("Disabled");

              // Col 3: Shift Type Tag
              ImGui::TableNextColumn();
              if (shift.change_type == "PROMOTED")
                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.35f, 1.0f), "[PROMOTED]");
              else if (shift.change_type == "DEMOTED")
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "[DEMOTED]");
              else if (shift.change_type == "NEW")
                ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "[NEW]");
              else
                ImGui::TextDisabled("[UNCHANGED]");

              // Col 4: Rank Delta
              ImGui::TableNextColumn();
              if (shift.baseline_rank > 0 && shift.viper_rank > 0)
              {
                int delta = shift.baseline_rank - shift.viper_rank;
                if (delta > 0)
                  ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.35f, 1.0f), "+%d slots", delta);
                else if (delta < 0)
                  ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "%d slots", delta);
                else
                  ImGui::TextDisabled("0");
              }
              else
              {
                ImGui::TextDisabled("-");
              }
            }
            ImGui::EndTable();
          }

          ImGui::EndTabItem();
        }

        // -------------------------------------------------------------------
        // TAB 3: DAgger Convergence History
        // -------------------------------------------------------------------
        if (ImGui::BeginTabItem("DAgger Convergence History"))
        {
          ImGui::Spacing();
          ImGui::TextDisabled("Progression of candidate policy throughput and tree fidelity over %zu DAgger aggregation passes:", res.dagger_history.size());
          ImGui::Spacing();

          static ImGuiTableFlags dag_tbl_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
          if (ImGui::BeginTable("DAggerHistoryTable", 6, dag_tbl_flags))
          {
            ImGui::TableSetupColumn("Iteration", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Samples Added", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Dataset Size |D|", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Candidate DPS", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Tree Fidelity %", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Tree Leaves", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& log : res.dagger_history)
            {
              ImGui::TableNextRow();

              // Col 0: Pass #
              ImGui::TableNextColumn();
              ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Pass #%zu", log.iteration);

              // Col 1: Samples Added
              ImGui::TableNextColumn();
              ImGui::Text("+%zu", log.samples_added);

              // Col 2: Cumulative Dataset
              ImGui::TableNextColumn();
              ImGui::Text("%zu samples", log.total_samples);

              // Col 3: Candidate DPS
              ImGui::TableNextColumn();
              ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.35f, 1.0f), "%.1f DPS", log.candidate_dps);

              // Col 4: Tree Fidelity
              ImGui::TableNextColumn();
              ImGui::Text("%.1f%%", log.tree_weighted_fidelity_pct);

              // Col 5: Tree Leaves
              ImGui::TableNextColumn();
              ImGui::Text("%zu rules / leaves", log.tree_leaf_count);
            }
            ImGui::EndTable();
          }

          ImGui::EndTabItem();
        }

        // -------------------------------------------------------------------
        // TAB 4: Decision Tree (CART & C++ Export)
        // -------------------------------------------------------------------
        if (ImGui::BeginTabItem("Decision Tree & C++ Export"))
        {
          ImGui::Spacing();
          ImGui::Text("Fitted CART Decision Tree (Depth: %zu, Leaves: %zu, Fidelity: %.1f%%)",
                      res.tree_depth, res.tree_leaf_count, res.tree_weighted_fidelity_pct);
          ImGui::Spacing();

          if (WowButton("Copy Generated C++ Policy Code", ImVec2(240.0f, 24.0f)))
          {
            ImGui::SetClipboardText(res.generated_cpp_code.c_str());
          }

          ImGui::Spacing();

          float half_w = (ImGui::GetContentRegionAvail().x - 10.0f) * 0.5f;

          // Left Box: ASCII Tree Visualization
          BeginWowChild("TreeAsciiBox", ImVec2(half_w, 0), true);
          {
            ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "CART Decision Tree Splits & Bounds:");
            ImGui::Spacing();
            ImGui::TextUnformatted(res.tree_ascii_visualization.c_str());
          }
          EndWowChild();

          ImGui::SameLine(0, 10.0f);

          // Right Box: Transpiled C++ Code
          BeginWowChild("TreeCppBox", ImVec2(0, 0), true);
          {
            ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Transpiled C++ Decision Policy:");
            ImGui::Spacing();
            ImGui::TextUnformatted(res.generated_cpp_code.c_str());
          }
          EndWowChild();

          ImGui::EndTabItem();
        }

        // -------------------------------------------------------------------
        // TAB 5: MCTS Action Regret & Frequency
        // -------------------------------------------------------------------
        if (ImGui::BeginTabItem("MCTS Action Values & Regret"))
        {
          ImGui::Spacing();
          ImGui::TextDisabled("Empirical Oracle action distribution and regret weights across all visited states:");
          ImGui::Spacing();

          static ImGuiTableFlags act_tbl_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
          if (ImGui::BeginTable("ActionStatsTable", 4, act_tbl_flags, ImVec2(0, 0)))
          {
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Oracle Pick %", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Avg Regret Weight w(s)", ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn("Total Visited Selections", ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableHeadersRow();

            for (const auto& st : res.action_stats)
            {
              ImGui::TableNextRow();

              // Col 0: Name
              ImGui::TableNextColumn();
              const auto& icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(st.action)));
              rlImGuiImageSize(&icon, 16, 16);
              ImGui::SameLine(0, 5.0f);
              ImGui::Text("%s", st.name.c_str());

              // Col 1: Selection %
              ImGui::TableNextColumn();
              ImGui::Text("%.1f%%", st.selection_pct);

              // Col 2: Avg Regret
              ImGui::TableNextColumn();
              ImGui::Text("%.2f DPS", st.avg_regret);

              // Col 3: Count
              ImGui::TableNextColumn();
              ImGui::Text("%zu times", st.selection_count);
            }
            ImGui::EndTable();
          }

          ImGui::EndTabItem();
        }

        // -------------------------------------------------------------------
        // TAB 6: GBDT Q-Policy (LightGBM/Tree Ensemble)
        // -------------------------------------------------------------------
        if (ImGui::BeginTabItem("GBDT Q-Policy (LightGBM/Ensemble)"))
        {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f),
                             "Pure C++ Multi-Action GBDT Q-Policy Engine (LightGBM 2nd-Order Boosting)");
          ImGui::Spacing();
          ImGui::Text("Models Trained: %zu Action Estimators | Trees / Model: %zu | Max Depth: %zu | Expected DPS: %.1f DPS",
                      res.gbdt_q_policy.models().size(),
                      res.gbdt_q_policy.models().empty() ? 0 : res.gbdt_q_policy.models().begin()->second.num_trees(),
                      res.gbdt_q_policy.models().empty() ? 0 : res.gbdt_q_policy.models().begin()->second.config().max_depth,
                      res.gbdt_policy_expected_dps);
          ImGui::Spacing();

          if (WowButton("Export GBDT Model JSON to Clipboard", ImVec2(260.0f, 24.0f)))
          {
            std::ostringstream jss;
            jss << "{\n  \"model_type\": \"GBDTMultiActionQPolicy\",\n  \"models\": {\n";
            size_t m_idx = 0;
            for (const auto& [act_id, model] : res.gbdt_q_policy.models()) {
              jss << "    \"" << VIPEROracle::get_action_name(static_cast<PriorityAction>(act_id)) << "\": "
                  << model.to_json() << (m_idx + 1 < res.gbdt_q_policy.models().size() ? ",\n" : "\n");
              m_idx++;
            }
            jss << "  }\n}\n";
            ImGui::SetClipboardText(jss.str().c_str());
          }

          ImGui::Spacing();
          ImGui::TextDisabled("Global GBDT Feature Importance (Total Split Gain across all Action Models):");
          ImGui::Spacing();

          static ImGuiTableFlags feat_tbl_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
          if (ImGui::BeginTable("GBDTFeatureImportanceTable", 3, feat_tbl_flags, ImVec2(0, 0)))
          {
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Feature Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Total Split Gain", ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableHeadersRow();

            for (size_t f = 0; f < res.gbdt_feature_importances.size(); ++f)
            {
              const auto& [fname, gain] = res.gbdt_feature_importances[f];
              ImGui::TableNextRow();

              ImGui::TableNextColumn();
              ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "#%zu", f + 1);

              ImGui::TableNextColumn();
              ImGui::Text("%s", fname.c_str());

              ImGui::TableNextColumn();
              ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "%.2f", gain);
            }
            ImGui::EndTable();
          }

          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      }
    }
  }
  EndWowChild();
}

} // namespace warlock
