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

enum class ActiveAnalysisView
{
  NONE = 0,
  BLUNDER = 1,
  FULL_MCTS = 2
};

// Background worker state for asynchronous analysis
struct APLAnalyzerWorkerState
{
  std::shared_ptr<APLAnalyzer::AsyncBlunderAnalysisSession> session;
  std::thread worker;
  std::mutex mtx;
  std::atomic<bool> is_running{false};
  std::atomic<bool> stop_requested{false};
  std::atomic<float> progress{0.0f};
  std::string current_status;
  APLAnalysisReport live_report;
  bool has_result = false;

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

  void request_stop()
  {
    stop_requested.store(true, std::memory_order_relaxed);
  }
};

inline APLAnalyzerWorkerState& get_blunder_analyzer_worker_state()
{
  static APLAnalyzerWorkerState state;
  return state;
}

inline APLAnalyzerWorkerState& get_full_mcts_analyzer_worker_state()
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

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
inline int get_browser_max_threads()
{
  int browser_threads = EM_ASM_INT({
    return (typeof navigator !== 'undefined' && navigator.hardwareConcurrency)
        ? navigator.hardwareConcurrency
        : 4;
  });
  // Reserve 2 threads for browser main thread and coordinator worker
  return std::max(1, browser_threads - 2);
}
#endif

// ============================================================================
// MAIN ENTRY POINT: 2 Columns on Top + 1 Shared Full-Width Column Below
// ============================================================================
inline void render_panel_analyze_apl(const WarlockSimulator& sim, AppTab* switch_tab = nullptr)
{
  auto& blunder_worker = get_blunder_analyzer_worker_state();
  auto& mcts_worker = get_full_mcts_analyzer_worker_state();

  static ActiveAnalysisView active_view = ActiveAnalysisView::NONE;
#if defined(__EMSCRIPTEN__)
  static int blunder_threads = get_browser_max_threads();
#else
  static int blunder_threads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
#endif
  static bool blunder_adaptive_rollouts = true;
  static int blunder_rollouts_per_action = 512;
  static char blunder_filter[64] = "";

  static int mcts_num_runs = 5;
  static bool mcts_continuous = false;
  static bool mcts_adaptive_rollouts = true;
  static int mcts_rollouts_per_step = 256;
  static int selected_mcts_run_idx = 0;
  static float timeline_zoom_px_per_sec = 16.0f;

  // Handle blunder worker completions
  {
    std::lock_guard<std::mutex> lock(blunder_worker.mtx);
    if (!blunder_worker.is_running.load() && blunder_worker.worker.joinable())
    {
      blunder_worker.worker.join();
    }
  }

  // Handle mcts worker completions
  {
    std::lock_guard<std::mutex> lock(mcts_worker.mtx);
    if (!mcts_worker.is_running.load() && mcts_worker.worker.joinable())
    {
      mcts_worker.worker.join();
    }
  }

  bool is_blunder_busy = blunder_worker.is_running.load();
  bool is_mcts_busy = mcts_worker.is_running.load();
  bool is_any_busy = is_blunder_busy || is_mcts_busy;

  float full_w = ImGui::GetContentRegionAvail().x;
  float top_h = 104.0f;
  float col_w = std::floor((full_w - 12.0f) * 0.5f);

  // -------------------------------------------------------------------------
  // TOP SECTION: 2 Side-by-Side Control Cards
  // -------------------------------------------------------------------------

  // Card 1: APL Blunder Action Analysis
  BeginWowChild("TopCard_BlunderAnalysis", ImVec2(col_w, top_h), true);
  {
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "APL Blunder Action Analysis");

    ImGui::Spacing();

    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Threads:");
    ImGui::SetNextItemWidth(55.0f);
    if (is_any_busy) ImGui::BeginDisabled();
    WowInputInt("##BlunderThreadsInput", &blunder_threads, 1, 4);
    if (blunder_threads < 1) blunder_threads = 1;
#if defined(__EMSCRIPTEN__)
    int max_wasm_threads = get_browser_max_threads();
    if (blunder_threads > max_wasm_threads) blunder_threads = max_wasm_threads;
#else
    if (blunder_threads > 128) blunder_threads = 128;
#endif
    if (is_any_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 8.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text(blunder_adaptive_rollouts ? "Max Rollouts:" : "Rollouts/Action:");
    ImGui::SetNextItemWidth(95.0f);
    if (is_any_busy) ImGui::BeginDisabled();
    WowInputInt("##BlunderRolloutsInput", &blunder_rollouts_per_action, 64, 128);
    if (blunder_rollouts_per_action < 32) blunder_rollouts_per_action = 32;
    if (blunder_rollouts_per_action > 2048) blunder_rollouts_per_action = 2048;
    if (is_any_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 8.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Dummy(ImVec2(0, 1.0f));
    if (is_any_busy) ImGui::BeginDisabled();
    WowCheckbox("Adaptive", &blunder_adaptive_rollouts);
    if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip("Runs pilot rollouts (min 16) and statistically early-stops / prunes inferior\nbranches once >= 99%% confidence separation is reached, hard-capped at Max Rollouts.");
    }
    if (is_any_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 10.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

    if (is_blunder_busy)
    {
      std::string b_status = blunder_worker.get_status();
      ImGui::ProgressBar(blunder_worker.progress.load(), ImVec2(180.0f, 26.0f), b_status.c_str());
    }
    else
    {
      if (is_any_busy) ImGui::BeginDisabled();
      if (WowButton("Run Blunder Analysis", ImVec2(180.0f, 26.0f)))
      {
        active_view = ActiveAnalysisView::BLUNDER;
        blunder_worker.has_result = false;
        blunder_worker.is_running = true;
        blunder_worker.set_status(0.05f, "Initializing simulation episode...");

        if (blunder_worker.worker.joinable())
        {
          blunder_worker.worker.join();
        }

        WarlockSimulator sim_copy = sim;
        int rollouts_cnt = blunder_rollouts_per_action;
        bool adapt = blunder_adaptive_rollouts;
        size_t user_threads = (blunder_threads > 0) ? static_cast<size_t>(blunder_threads) : 1;

        blunder_worker.worker = std::thread([sim_copy, rollouts_cnt, adapt, user_threads]() {
          auto& w = get_blunder_analyzer_worker_state();
          printf("[BlunderCoordinator] Thread started. Initializing session with %d rollouts...\n", rollouts_cnt);
          fflush(stdout);

          auto sess = std::make_shared<APLAnalyzer::AsyncBlunderAnalysisSession>();
          sess->init(sim_copy, rollouts_cnt, adapt, 1337);

#if defined(__EMSCRIPTEN__)
          // In WebAssembly, reserve 1 thread for the outer coordinator worker to avoid pthread pool deadlock
          size_t avail_threads = (user_threads > 1) ? (user_threads - 1) : 1;
#else
          size_t avail_threads = user_threads;
#endif
          size_t num_workers = std::min(avail_threads, sess->total_decision_steps);
          printf("[BlunderCoordinator] Total decision steps: %zu. Spawning %zu worker threads (avail: %zu)...\n",
                 sess->total_decision_steps, num_workers, avail_threads);
          fflush(stdout);

          if (num_workers <= 1)
          {
            printf("[BlunderCoordinator] Running single-threaded evaluation path...\n");
            fflush(stdout);
            for (size_t d = 0; d < sess->total_decision_steps; ++d)
            {
              sess->eval_decision(d);
              float p = 0.05f + 0.90f * (static_cast<float>(d + 1) / static_cast<float>(sess->total_decision_steps));
              std::string msg = "Evaluating APL Decision #" + std::to_string(d + 1) + " / " + std::to_string(sess->total_decision_steps) + "...";
              w.set_status(p, msg);
            }
          }
          else
          {
            std::vector<std::thread> workers;
            workers.reserve(num_workers);

            for (size_t t = 0; t < num_workers; ++t)
            {
              workers.emplace_back([sess, &w, t]() {
                printf("[BlunderWorker #%zu] Worker thread launched.\n", t);
                fflush(stdout);
                while (true)
                {
                  size_t d = sess->next_d.fetch_add(1, std::memory_order_relaxed);
                  if (d >= sess->total_decision_steps) break;
                  sess->eval_decision(d);
                  size_t done = sess->completed_d.fetch_add(1, std::memory_order_relaxed) + 1;
                  float p = 0.05f + 0.90f * (static_cast<float>(done) / static_cast<float>(sess->total_decision_steps));
                  std::string msg = "Evaluating APL Decision #" + std::to_string(done) + " / " + std::to_string(sess->total_decision_steps) + "...";
                  w.set_status(p, msg);
                }
                printf("[BlunderWorker #%zu] Finished all assigned tasks.\n", t);
                fflush(stdout);
              });
            }

            for (size_t t = 0; t < workers.size(); ++t)
            {
              if (workers[t].joinable())
              {
                printf("[BlunderCoordinator] Joining worker #%zu...\n", t);
                fflush(stdout);
                workers[t].join();
                printf("[BlunderCoordinator] Worker #%zu successfully joined!\n", t);
                fflush(stdout);
              }
            }
          }

          printf("[BlunderCoordinator] Finalizing report...\n");
          fflush(stdout);
          APLAnalysisReport res = sess->finalize();
          {
            std::lock_guard<std::mutex> lock(w.mtx);
            w.live_report = std::move(res);
            w.has_result = true;
            w.is_running = false;
          }
          w.set_status(1.0f, "Analysis Complete!");
          printf("[BlunderCoordinator] Analysis complete and published to UI!\n");
          fflush(stdout);
        });
      }
      if (is_any_busy) ImGui::EndDisabled();
    }
  }
  EndWowChild();

  ImGui::SameLine(0, 12.0f);

  // Card 2: APL vs Full MCTS Optimal Policy Trace
  BeginWowChild("TopCard_FullMCTSAnalysis", ImVec2(0, top_h), true);
  {
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "APL vs. Full MCTS Optimal Policy Trace");

    ImGui::Spacing();

    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Runs:");
    ImGui::SetNextItemWidth(55.0f);
    if (is_any_busy || mcts_continuous) ImGui::BeginDisabled();
    WowInputInt("##FullMCTSNumRunsInput", &mcts_num_runs, 1, 5);
    if (mcts_num_runs < 1) mcts_num_runs = 1;
    if (mcts_num_runs > 500) mcts_num_runs = 500;
    if (is_any_busy || mcts_continuous) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 8.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Dummy(ImVec2(0, 1.0f));
    if (is_any_busy) ImGui::BeginDisabled();
    WowCheckbox("Run Until Stop", &mcts_continuous);
    if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip("Continuously simulates MCTS episodes and streams live statistical updates\n(confidence intervals, optimality ratio, DPS gap) until you click Stop.");
    }
    if (is_any_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 8.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text(mcts_adaptive_rollouts ? "Max Rollouts:" : "Rollouts / Step:");
    ImGui::SetNextItemWidth(95.0f);
    if (is_any_busy) ImGui::BeginDisabled();
    WowInputInt("##FullMCTSRolloutsInput", &mcts_rollouts_per_step, 64, 128);
    if (mcts_rollouts_per_step < 32) mcts_rollouts_per_step = 32;
    if (mcts_rollouts_per_step > 2048) mcts_rollouts_per_step = 2048;
    if (is_any_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 8.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Dummy(ImVec2(0, 1.0f));
    if (is_any_busy) ImGui::BeginDisabled();
    WowCheckbox("Adaptive", &mcts_adaptive_rollouts);
    if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip("Statistically early-stops and prunes branches when a clear winner is found,\ncapped at Max Rollouts.");
    }
    if (is_any_busy) ImGui::EndDisabled();
    ImGui::EndGroup();

    ImGui::SameLine(0, 10.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

    if (is_mcts_busy)
    {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.22f, 0.22f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.28f, 0.28f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.16f, 0.16f, 1.0f));
      if (WowButton(mcts_worker.stop_requested.load() ? "Stopping..." : "Stop Analysis", ImVec2(130.0f, 26.0f)))
      {
        mcts_worker.request_stop();
        mcts_worker.set_status(mcts_worker.progress.load(), "Stopping after current episodes finish...");
      }
      ImGui::PopStyleColor(3);

      ImGui::SameLine(0, 8.0f);
      std::string m_status = mcts_worker.get_status();
      ImGui::ProgressBar(mcts_worker.progress.load(), ImVec2(180.0f, 26.0f), m_status.c_str());
    }
    else
    {
      if (is_any_busy) ImGui::BeginDisabled();
      const char* btn_label = mcts_continuous ? "Start Continuous Trace" : "Run Full MCTS Analysis";
      if (WowButton(btn_label, ImVec2(180.0f, 26.0f)))
      {
        mcts_worker.is_running = true;
        mcts_worker.stop_requested = false;
        mcts_worker.has_result = false;
        {
          std::lock_guard<std::mutex> lock(mcts_worker.mtx);
          mcts_worker.live_report = APLAnalysisReport();
        }
        mcts_worker.set_status(0.02f, "Launching autonomous MCTS rollout workers...");
        active_view = ActiveAnalysisView::FULL_MCTS;
        selected_mcts_run_idx = 0;

        if (mcts_worker.worker.joinable())
        {
          mcts_worker.worker.join();
        }

        WarlockSimulator sim_copy = sim;
        int target_runs = mcts_continuous ? -1 : mcts_num_runs;
        int r_cnt = mcts_rollouts_per_step;
        bool adapt = mcts_adaptive_rollouts;

        mcts_worker.worker = std::thread([sim_copy, target_runs, r_cnt, adapt]() {
          auto& w = get_full_mcts_analyzer_worker_state();

          unsigned int hw_threads = std::max(1u, std::thread::hardware_concurrency());
#if defined(__EMSCRIPTEN__)
          size_t avail_threads = (hw_threads > 1) ? (hw_threads - 1) : 1;
#else
          size_t avail_threads = hw_threads;
#endif
          bool is_continuous = (target_runs <= 0);
          size_t num_workers = is_continuous ? avail_threads : std::min(avail_threads, static_cast<size_t>(target_runs));
          if (num_workers < 1) num_workers = 1;

          std::atomic<size_t> next_run_idx{0};
          std::atomic<size_t> completed_count{0};
          uint64_t base_seed = 1337;

          auto worker_loop = [&](size_t /*worker_id*/) {
            while (!w.stop_requested.load(std::memory_order_relaxed)) {
              size_t run_idx = next_run_idx.fetch_add(1, std::memory_order_relaxed);
              if (!is_continuous && run_idx >= static_cast<size_t>(target_runs)) {
                break;
              }

              uint64_t run_seed = base_seed + run_idx * 1337 + 7;
              APLAnalysisRun single_run = APLAnalyzer::analyze_single_run(
                  sim_copy,
                  run_idx + 1,
                  run_seed,
                  r_cnt,
                  AnalysisMode::FULL_MCTS_ONLY,
                  adapt,
                  nullptr,
                  1
              );

              if (w.stop_requested.load(std::memory_order_relaxed) && single_run.apl_dps <= 0.0) {
                break;
              }

              // Safely add to live report under lock
              {
                std::lock_guard<std::mutex> lock(w.mtx);
                w.live_report.add_run(std::move(single_run));
                w.has_result = true;
              }

              size_t done = completed_count.fetch_add(1, std::memory_order_relaxed) + 1;
              float prog = 0.0f;
              std::string msg;
              if (is_continuous) {
                prog = 0.5f;
                std::ostringstream ss;
                ss << "Run #" << done << " finished (Running continuously)...";
                msg = ss.str();
              } else {
                prog = 0.05f + 0.95f * (static_cast<float>(done) / static_cast<float>(target_runs));
                std::ostringstream ss;
                ss << "Completed Run #" << done << " / " << target_runs << "...";
                msg = ss.str();
              }
              w.set_status(prog, msg);
            }
          };

          std::vector<std::thread> workers;
          workers.reserve(num_workers);
          for (size_t t = 0; t < num_workers; ++t) {
            workers.emplace_back(worker_loop, t);
          }

          for (auto& worker : workers) {
            if (worker.joinable()) {
              worker.join();
            }
          }

          {
            std::lock_guard<std::mutex> lock(w.mtx);
            w.live_report.is_valid = !w.live_report.runs.empty();
            w.is_running = false;
          }
          w.set_status(1.0f, w.stop_requested.load() ? "Analysis Stopped" : "Analysis Complete!");
        });
      }
      if (is_any_busy) ImGui::EndDisabled();
    }
  }
  EndWowChild();

  ImGui::Spacing();

  // -------------------------------------------------------------------------
  // BOTTOM SECTION: Shared Results Panel (Displays whatever was run last)
  // -------------------------------------------------------------------------
  BeginWowChild("BottomSharedResultsPane", ImVec2(0, 0), true);
  {
    if (active_view == ActiveAnalysisView::BLUNDER && blunder_worker.has_result)
    {
      // ---------------------------------------------------------------------
      // VIEW A: BLUNDER ANALYSIS RESULTS (FULL WIDTH)
      // ---------------------------------------------------------------------
      const auto& report = blunder_worker.live_report;

      std::vector<const APLDivergenceEvent*> all_blunders;
      if (!report.runs.empty())
      {
        for (const auto& ev : report.runs[0].events)
        {
          all_blunders.push_back(&ev);
        }
      }

      std::sort(all_blunders.begin(), all_blunders.end(), [](const auto* a, const auto* b) {
        if (std::abs(a->delta_dps - b->delta_dps) > 0.01)
          return a->delta_dps > b->delta_dps;
        return a->confidence_pct > b->confidence_pct;
      });

      // Top KPI Cards
      float avail_w = ImGui::GetContentRegionAvail().x;
      float card_w = (avail_w - 3.0f * 10.0f) / 4.0f;
      float card_h = 66.0f;

      BeginWowChild("BlunderKPI1", ImVec2(card_w, card_h), true);
      {
        ImGui::TextDisabled("TOTAL BLUNDERS DETECTED");
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%zu blunders", all_blunders.size());
        ImGui::TextDisabled("%zu total GCDs evaluated", report.total_decisions_evaluated);
      }
      EndWowChild();

      ImGui::SameLine(0, 10.0f);

      BeginWowChild("BlunderKPI2", ImVec2(card_w, card_h), true);
      {
        ImGui::TextDisabled("AGREEMENT FIDELITY");
        ImVec4 agree_col = (report.overall_agreement_pct >= 90.0) ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) :
                           (report.overall_agreement_pct >= 75.0) ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
        ImGui::TextColored(agree_col, "%.1f%% Agreement", report.overall_agreement_pct);
        ImGui::TextDisabled("APL matching optimal policy");
      }
      EndWowChild();

      ImGui::SameLine(0, 10.0f);

      BeginWowChild("BlunderKPI3", ImVec2(card_w, card_h), true);
      {
        ImGui::TextDisabled("WORST SINGLE BLUNDER");
        double worst_loss = all_blunders.empty() ? 0.0 : all_blunders.front()->delta_dps;
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "-%.1f DPS Loss", worst_loss);
        ImGui::TextDisabled("Highest individual regret");
      }
      EndWowChild();

      ImGui::SameLine(0, 10.0f);

      BeginWowChild("BlunderKPI4", ImVec2(card_w, card_h), true);
      {
        ImGui::TextDisabled("ROLLOUTS / GCD");
        if (blunder_adaptive_rollouts) {
          ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "%d (Adaptive Cap)", blunder_rollouts_per_action);
          ImGui::TextDisabled("Auto early-stop & branch pruning");
        } else {
          ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "%d Rollouts", blunder_rollouts_per_action);
          ImGui::TextDisabled("Fixed CRN rollout branches");
        }
      }
      EndWowChild();

      ImGui::Spacing();

      // Filter & Table
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Filter:");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(220.0f);
      ImGui::InputTextWithHint("##BlunderFilter", "Search spell or rationale...", blunder_filter, IM_ARRAYSIZE(blunder_filter));

      ImGui::SameLine(0, 12.0f);
      ImGui::TextDisabled("Showing %zu detected APL blunders ranked from highest DPS loss to least severe", all_blunders.size());

      ImGui::Spacing();

      if (all_blunders.empty())
      {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Optimal APL! No rotational discrepancies detected between APL and MCTS forward rollouts.");
      }
      else
      {
        std::string filter_str = blunder_filter;
        std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

        static ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("CombinedBlundersTable", 7, table_flags, ImVec2(0, 0)))
        {
          ImGui::TableSetupColumn("Rank / Severity", ImGuiTableColumnFlags_WidthFixed, 115.0f);
          ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 55.0f);
          ImGui::TableSetupColumn("Combat Context", ImGuiTableColumnFlags_WidthFixed, 220.0f);
          ImGui::TableSetupColumn("APL Move Made", ImGuiTableColumnFlags_WidthFixed, 160.0f);
          ImGui::TableSetupColumn("MCTS Optimal Choice & Branch Evaluations", ImGuiTableColumnFlags_WidthStretch);
          ImGui::TableSetupColumn("DPS Loss (95% CI)", ImGuiTableColumnFlags_WidthFixed, 130.0f);
          ImGui::TableSetupColumn("Confidence", ImGuiTableColumnFlags_WidthFixed, 85.0f);
          ImGui::TableHeadersRow();

          size_t display_rank = 1;
          for (const auto* ev_ptr : all_blunders)
          {
            const auto& ev = *ev_ptr;

            if (!filter_str.empty())
            {
              std::string combined = ev.apl_action_name + " " + ev.mcts_action_name + " " + ev.top_alternatives_summary;
              std::transform(combined.begin(), combined.end(), combined.begin(), ::tolower);
              if (combined.find(filter_str) == std::string::npos)
                continue;
            }

            ImGui::TableNextRow();

            // Col 0: Severity & Rank
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

            // Col 1: Time
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%04.1fs", ev.timestamp);

            // Col 2: Combat Context (Comma-separated, uniform color)
            ImGui::TableNextColumn();
            std::ostringstream ctx_ss;
            ctx_ss << std::fixed << std::setprecision(0);
            ctx_ss << "Target HP: " << (ev.state.target_hp_pct * 100.0f) << "%, "
                   << "Rem: " << std::setprecision(1) << ev.state.time_remaining_sec << "s, "
                   << "Mana: " << std::setprecision(0) << (ev.state.player_mana_pct * 100.0f) << "%";
            
            if (ev.state.dot_corruption_rem_sec > 0.0f) ctx_ss << ", Corr: " << static_cast<int>(ev.state.dot_corruption_rem_sec) << "s";
            if (ev.state.dot_immolate_rem_sec > 0.0f) ctx_ss << ", Immo: " << static_cast<int>(ev.state.dot_immolate_rem_sec) << "s";
            if (ev.state.dot_doom_rem_sec > 0.0f) ctx_ss << ", Doom: " << static_cast<int>(ev.state.dot_doom_rem_sec) << "s";
            if (ev.state.dot_agony_rem_sec > 0.0f) ctx_ss << ", Agony: " << static_cast<int>(ev.state.dot_agony_rem_sec) << "s";
            if (ev.state.dot_siphon_life_rem_sec > 0.0f) ctx_ss << ", SL: " << static_cast<int>(ev.state.dot_siphon_life_rem_sec) << "s";
            if (ev.state.isb_charges_rem > 0.0f) ctx_ss << ", ISB(" << static_cast<int>(ev.state.isb_charges_rem) << ")";
            if (ev.state.nightfall_proc_active > 0.5f) ctx_ss << ", Trance";
            if (ev.state.decimation_rem_sec > 0.0f) ctx_ss << ", Decimate: " << static_cast<int>(ev.state.decimation_rem_sec) << "s";
            ImGui::TextWrapped("%s", ctx_ss.str().c_str());

            // Col 3: APL Move Made (Clean name & DPS on same line)
            ImGui::TableNextColumn();
            const auto& apl_icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(ev.apl_action)));
            rlImGuiImageSize(&apl_icon, 16, 16);
            ImGui::SameLine(0, 4.0f);
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", APLAnalyzer::get_action_clean_name(ev.apl_action));
            ImGui::SameLine(0, 5.0f);
            ImGui::TextDisabled("(%.1f ± %.1f DPS)", ev.apl_expected_dps, ev.apl_std_error);

            // Col 4: MCTS Optimal Choice (Clean name & DPS on same line, expandable)
            ImGui::TableNextColumn();
            const auto& mcts_icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(ev.mcts_action)));
            rlImGuiImageSize(&mcts_icon, 16, 16);
            ImGui::SameLine(0, 4.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", APLAnalyzer::get_action_clean_name(ev.mcts_action));
            ImGui::SameLine(0, 5.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 0.8f), "(%.1f ± %.1f DPS)", ev.mcts_expected_dps, ev.mcts_std_error);

            char expand_label[64];
            snprintf(expand_label, sizeof(expand_label), "View %zu Candidate Actions##cand_%zu", ev.candidate_evals.size(), ev.decision_step);
            if (ImGui::TreeNode(expand_label))
            {
              ImGui::Spacing();
              size_t cand_rank = 1;
              for (const auto& cand : ev.candidate_evals)
              {
                if (cand.is_skipped)
                {
                  // Skipped Action Row
                  ImGui::TextDisabled(" - ");
                  ImGui::SameLine(0, 4.0f);

                  const auto& icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(cand.action)));
                  rlImGuiImageSize(&icon, 13, 13);
                  ImGui::SameLine(0, 4.0f);

                  ImGui::TextDisabled("%s", APLAnalyzer::get_action_clean_name(cand.action));
                  ImGui::SameLine(0, 6.0f);
                  ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 0.8f), "[Skipped: %s]", cand.skip_reason.c_str());
                }
                else
                {
                  // Evaluated Action Row
                  bool is_optimal = (cand.action == ev.mcts_action);
                  bool is_chosen = (cand.action == ev.apl_action);

                  // Rank
                  if (is_optimal) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "#%zu", cand_rank++);
                  } else if (is_chosen) {
                    ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "#%zu", cand_rank++);
                  } else {
                    ImGui::TextDisabled("#%zu", cand_rank++);
                  }
                  ImGui::SameLine(0, 4.0f);

                  // Icon
                  const auto& icon = AssetManager::get().get_icon(spell_id_to_icon(APLAnalyzer::get_action_spell_id(cand.action)));
                  rlImGuiImageSize(&icon, 13, 13);
                  ImGui::SameLine(0, 4.0f);

                  // Spell Name & Tag
                  if (is_optimal) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s (MCTS Best)", APLAnalyzer::get_action_clean_name(cand.action));
                  } else if (is_chosen) {
                    ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "%s (APL Pick)", APLAnalyzer::get_action_clean_name(cand.action));
                  } else {
                    ImGui::Text("%s", APLAnalyzer::get_action_clean_name(cand.action));
                  }

                  // Expected DPS & Delta vs APL
                  double delta = cand.mean_dps - ev.apl_expected_dps;
                  ImGui::SameLine(0, 6.0f);
                  if (delta > 0.05) {
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "[%.1f DPS | +%.1f]", cand.mean_dps, delta);
                  } else if (delta < -0.05) {
                    ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 0.9f), "[%.1f DPS | %.1f]", cand.mean_dps, delta);
                  } else {
                    ImGui::TextDisabled("[%.1f DPS | 0.0]", cand.mean_dps);
                  }

                  if (cand.rollout_count > 0) {
                    ImGui::SameLine(0, 4.0f);
                    ImGui::TextDisabled("(%zu rolls)", cand.rollout_count);
                  }
                }
              }
              ImGui::TreePop();
            }

            // Col 5: DPS Loss (95% CI on same line)
            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "-%.1f DPS", ev.delta_dps);
            ImGui::SameLine(0, 5.0f);
            ImGui::TextDisabled("[%.1f, %.1f]", ev.delta_dps_ci_lower, ev.delta_dps_ci_upper);

            // Col 6: Confidence & Z-Score on same line
            ImGui::TableNextColumn();
            ImVec4 conf_col = (ev.confidence_pct >= 95.0) ? ImVec4(0.2f, 0.9f, 0.4f, 1.0f) :
                              (ev.confidence_pct >= 85.0) ? ImVec4(1.0f, 0.85f, 0.3f, 1.0f) : ImVec4(0.9f, 0.6f, 0.2f, 1.0f);
            ImGui::TextColored(conf_col, "%.1f%%", ev.confidence_pct);
            ImGui::SameLine(0, 4.0f);
            ImGui::TextDisabled("(Z=%.1f)", ev.z_score);
          }
          ImGui::EndTable();
        }
      }
    }
    else if (active_view == ActiveAnalysisView::FULL_MCTS && (mcts_worker.has_result || is_mcts_busy))
    {
      // ---------------------------------------------------------------------
      // VIEW B: FULL MCTS OPTIMAL POLICY TRACE RESULTS (FULL WIDTH)
      // ---------------------------------------------------------------------
      APLAnalysisReport report;
      {
        std::lock_guard<std::mutex> lock(mcts_worker.mtx);
        report = mcts_worker.live_report;
      }

      if (report.runs.empty())
      {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "MCTS Autonomous Simulation Initializing...");
        ImGui::Spacing();
        ImGui::TextDisabled("Simulating initial episodes and evaluating forward rollouts in background...");
        ImGui::ProgressBar(mcts_worker.progress.load(), ImVec2(-1, 24.0f), mcts_worker.get_status().c_str());
      }
      else
      {
        // Top KPI Cards (4 side-by-side cards)
        float avail_w = ImGui::GetContentRegionAvail().x;
        float card_w = (avail_w - 3.0f * 10.0f) / 4.0f;
        float card_h = 76.0f;

        // Card 1: Active APL Mean DPS
        BeginWowChild("MCTSKPI1", ImVec2(card_w, card_h), true);
        {
          ImGui::TextDisabled("ACTIVE APL MEAN DPS");
          ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "%.1f DPS", report.avg_apl_dps);
          if (report.runs.size() > 1) {
            ImGui::TextDisabled("95%% CI: [%.1f, %.1f] (±%.1f)", report.apl_dps_ci_lower, report.apl_dps_ci_upper, report.apl_dps_stderr * 1.96);
          } else {
            ImGui::TextDisabled("Single episode baseline");
          }
        }
        EndWowChild();

        ImGui::SameLine(0, 10.0f);

        // Card 2: MCTS Optimal Ceiling
        BeginWowChild("MCTSKPI2", ImVec2(card_w, card_h), true);
        {
          ImGui::TextDisabled("MCTS OPTIMAL CEILING");
          ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%.1f DPS", report.avg_mcts_dps);
          if (report.runs.size() > 1) {
            ImGui::TextDisabled("95%% CI: [%.1f, %.1f] (±%.1f)", report.mcts_dps_ci_lower, report.mcts_dps_ci_upper, report.mcts_dps_stderr * 1.96);
          } else {
            ImGui::TextDisabled("Empirical rollout ceiling");
          }
        }
        EndWowChild();

        ImGui::SameLine(0, 10.0f);

        // Card 3: DPS Gap (Optimization Potential)
        BeginWowChild("MCTSKPI3", ImVec2(card_w, card_h), true);
        {
          ImGui::TextDisabled("DPS GAP (OPTIMIZATION POTENTIAL)");
          ImVec4 gap_color = (report.avg_dps_loss < 5.0) ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) :
                             (report.avg_dps_loss < 25.0) ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
          ImGui::TextColored(gap_color, "+%.1f DPS (+%.1f%%)", report.avg_dps_loss, report.avg_dps_loss_pct);
          if (report.runs.size() > 1) {
            ImGui::TextDisabled("95%% CI: [%+.1f, %+.1f] DPS", report.dps_loss_ci_lower, report.dps_loss_ci_upper);
          } else {
            ImGui::TextDisabled("Potential rotational yield");
          }
        }
        EndWowChild();

        ImGui::SameLine(0, 10.0f);

        // Card 4: APL % of Optimal Ceiling (Prominent & Bold with 95% Confidence Interval)
        BeginWowChild("MCTSKPI4", ImVec2(card_w, card_h), true);
        {
          ImGui::TextDisabled("APL %% OF OPTIMAL");
          ImVec4 opt_color = (report.optimality_pct >= 97.0) ? ImVec4(0.25f, 1.0f, 0.35f, 1.0f) :
                             (report.optimality_pct >= 90.0) ? ImVec4(1.0f, 0.85f, 0.2f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
          ImGui::TextColored(opt_color, "%.2f%%", report.optimality_pct);
          if (report.runs.size() > 1) {
            double margin = (report.avg_mcts_dps > 0.0) ? ((1.96 * report.dps_loss_stderr / report.avg_mcts_dps) * 100.0) : 0.0;
            ImGui::TextDisabled("95%% CI: [%.2f%%, %.2f%%] (±%.2f%%)", report.optimality_pct_ci_lower, report.optimality_pct_ci_upper, margin);
            if (ImGui::IsItemHovered())
            {
              ImGui::BeginTooltip();
              ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "Statistical Confidence in %% of Optimal:");
              ImGui::Separator();
              ImGui::Text(" • Estimated APL Optimality: %.2f%% of MCTS ceiling", report.optimality_pct);
              ImGui::Text(" • 95%% Confidence Interval: [%.2f%%, %.2f%%]", report.optimality_pct_ci_lower, report.optimality_pct_ci_upper);
              ImGui::Text(" • Margin of Error: ±%.2f%% across %zu sample runs", margin, report.runs.size());
              ImGui::Spacing();
              ImGui::TextDisabled("As more runs complete, the confidence interval narrows tighter,\nincreasing precision in your exact APL rating.");
              ImGui::EndTooltip();
            }
          } else {
            ImGui::TextDisabled("Single episode (Run more for CI)");
          }
        }
        EndWowChild();

        ImGui::Spacing();

        // Multi-Run Summary Table
        if (report.runs.size() > 1)
        {
          ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Multi-Episode Trace Comparison (%zu Episodes%s):", 
                             report.runs.size(), is_mcts_busy ? " - Live Updating..." : "");
          if (ImGui::BeginTable("MCTSRunsSummaryTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY, ImVec2(0, 150.0f)))
          {
            ImGui::TableSetupColumn("Episode", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("RNG Seed", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("APL DPS", ImGuiTableColumnFlags_WidthFixed, 95.0f);
            ImGui::TableSetupColumn("MCTS DPS Ceiling", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Optimization Delta", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("% of Optimal", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < report.runs.size(); ++i)
            {
              const auto& r = report.runs[i];
              ImGui::TableNextRow();

              // Col 0: Episode
              ImGui::TableNextColumn();
              bool is_sel = (selected_mcts_run_idx == static_cast<int>(i));
              char sel_label[32];
              snprintf(sel_label, sizeof(sel_label), "Run #%zu%s", i + 1, is_sel ? " *" : "");
              if (ImGui::Selectable(sel_label, is_sel, ImGuiSelectableFlags_SpanAllColumns))
              {
                selected_mcts_run_idx = static_cast<int>(i);
              }

              // Col 1: Seed
              ImGui::TableNextColumn();
              ImGui::TextDisabled("0x%llX", (unsigned long long)r.seed);

              // Col 2: Duration
              ImGui::TableNextColumn();
              ImGui::Text("%.1fs", r.fight_duration);

              // Col 3: APL DPS
              ImGui::TableNextColumn();
              ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "%.1f DPS", r.apl_dps);

              // Col 4: MCTS DPS
              ImGui::TableNextColumn();
              ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%.1f DPS", r.mcts_dps);

              // Col 5: Delta
              ImGui::TableNextColumn();
              double diff = r.mcts_dps - r.apl_dps;
              double pct = (r.apl_dps > 0) ? (diff / r.apl_dps * 100.0) : 0.0;
              if (diff > 0.1)
              {
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "+%.1f DPS (+%.1f%%)", diff, pct);
              }
              else if (diff < -0.1)
              {
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "%.1f DPS (%.1f%%)", diff, pct);
              }
              else
              {
                ImGui::TextDisabled("%s", "0.0 DPS (0.0%)");
              }

              // Col 6: % of Optimal (Clean number only, without redundant text)
              ImGui::TableNextColumn();
              double opt_ratio = (r.mcts_dps > 0.0) ? ((r.apl_dps / r.mcts_dps) * 100.0) : 100.0;
              ImVec4 row_opt_c = (opt_ratio >= 97.0) ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) :
                                 (opt_ratio >= 90.0) ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
              ImGui::TextColored(row_opt_c, "%.1f%%", opt_ratio);
            }
            ImGui::EndTable();
          }
          ImGui::Spacing();
        }

        // Clamp selected episode index
        if (selected_mcts_run_idx >= static_cast<int>(report.runs.size()))
        {
          selected_mcts_run_idx = static_cast<int>(report.runs.size()) - 1;
        }
        if (selected_mcts_run_idx < 0) selected_mcts_run_idx = 0;

        // Episode Selector & Zoom Toolbar
        BeginWowChild("MCTSNavBar", ImVec2(0, 38.0f), true);
        {
          ImGui::AlignTextToFramePadding();
          ImGui::Text("Timeline Episode (%zu):", report.runs.size());
          ImGui::SameLine();

          if (report.runs.size() <= 8)
          {
            for (size_t i = 0; i < report.runs.size(); ++i)
            {
              if (i > 0) ImGui::SameLine(0, 6.0f);
              std::string label = "Run #" + std::to_string(i + 1);
              bool is_selected = (selected_mcts_run_idx == static_cast<int>(i));
              if (WowTabButton(label.c_str(), is_selected, 75.0f, 24.0f))
              {
                selected_mcts_run_idx = static_cast<int>(i);
              }
            }
          }
          else
          {
            ImGui::SetNextItemWidth(170.0f);
            std::string combo_label = "Run #" + std::to_string(selected_mcts_run_idx + 1) + " (" +
                                      std::to_string(static_cast<int>(report.runs[selected_mcts_run_idx].apl_dps)) + " DPS)";
            if (ImGui::BeginCombo("##SelectedMCTSEpisodeCombo", combo_label.c_str()))
            {
              for (size_t i = 0; i < report.runs.size(); ++i)
              {
                bool is_selected = (selected_mcts_run_idx == static_cast<int>(i));
                std::string item_label = "Run #" + std::to_string(i + 1) + " (" +
                                         std::to_string(static_cast<int>(report.runs[i].apl_dps)) + " vs " +
                                         std::to_string(static_cast<int>(report.runs[i].mcts_dps)) + " DPS)";
                if (ImGui::Selectable(item_label.c_str(), is_selected))
                {
                  selected_mcts_run_idx = static_cast<int>(i);
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
              }
              ImGui::EndCombo();
            }

            ImGui::SameLine(0, 6.0f);
            if (WowButton("<", ImVec2(32.0f, 24.0f)) && selected_mcts_run_idx > 0)
            {
              selected_mcts_run_idx--;
            }
            ImGui::SameLine(0, 4.0f);
            if (WowButton(">", ImVec2(32.0f, 24.0f)) && selected_mcts_run_idx + 1 < static_cast<int>(report.runs.size()))
            {
              selected_mcts_run_idx++;
            }
          }

          ImGui::SameLine(ImGui::GetContentRegionAvail().x - 180.0f);
          ImGui::BeginGroup();
          WowResetTextBaseline();
          ImGui::Text("Zoom (px/s):");
          ImGui::SameLine();
          ImGui::SetNextItemWidth(75.0f);
          WowInputFloat("##MCTSZoomInput", &timeline_zoom_px_per_sec, 2.0f, 10.0f, "%.0f");
          if (timeline_zoom_px_per_sec < 4.0f) timeline_zoom_px_per_sec = 4.0f;
          if (timeline_zoom_px_per_sec > 100.0f) timeline_zoom_px_per_sec = 100.0f;
          ImGui::EndGroup();
        }
        EndWowChild();
      }

      ImGui::Spacing();

      // Timeline Canvas
      if (selected_mcts_run_idx >= 0 && selected_mcts_run_idx < static_cast<int>(report.runs.size()))
      {
        const auto& current_run = report.runs[selected_mcts_run_idx];

        float px_per_sec = timeline_zoom_px_per_sec;
        float canvas_w = static_cast<float>(current_run.fight_duration) * px_per_sec + 100.0f;
        float ruler_y = 4.0f;
        float lane1_y = 26.0f;   // APL Track
        float lane2_y = 74.0f;   // MCTS Track
        float dps_plot_y = 125.0f; // Cumulative DPS Plot (150px)
        float mana_plot_y = 295.0f; // Mana Plot (150px)
        float canvas_h = 470.0f;

        ImGui::BeginChild("MCTSTimelineScroll", ImVec2(-1, -1), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
          ImGui::Dummy(ImVec2(canvas_w, canvas_h));

          ImDrawList* dl = ImGui::GetWindowDrawList();
          ImVec2 p0 = ImGui::GetItemRectMin();
          ImVec2 p1 = ImVec2(p0.x + canvas_w, p0.y + canvas_h);

          dl->AddRectFilled(p0, p1, IM_COL32(18, 18, 22, 255));

          // Time Ruler Ticks & Vertical Grid Lines
          for (double t = 0.0; t <= current_run.fight_duration + 0.1; t += 5.0)
          {
            float x = p0.x + static_cast<float>(t) * px_per_sec;
            bool is_major = (std::fmod(t, 10.0) < 0.01);

            dl->AddLine(ImVec2(x, p0.y + ruler_y), ImVec2(x, p0.y + canvas_h - 10.0f),
                        is_major ? IM_COL32(65, 65, 75, 160) : IM_COL32(38, 38, 48, 100));

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

          // Track 1: Active APL Cast Icons
          const float icon_sz = 30.0f;
          for (const auto& sp : current_run.apl_spells)
          {
            float ix0 = p0.x + static_cast<float>(sp.start_time) * px_per_sec;
            float iy0 = p0.y + lane1_y + 2.0f;
            float ix1 = ix0 + icon_sz;
            float iy1 = iy0 + icon_sz;

            Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(sp.spell_id));
            dl->AddImage((ImTextureID)(uintptr_t)icon.id, ImVec2(ix0, iy0), ImVec2(ix1, iy1));

            ImU32 col_border = sp.is_crit ? IM_COL32(255, 215, 0, 255) : IM_COL32(60, 60, 75, 220);
            dl->AddRect(ImVec2(ix0, iy0), ImVec2(ix1, iy1), col_border, 2.0f, 0, sp.is_crit ? 2.0f : 1.0f);

            if (is_canvas_hovered && mouse_pos.x >= ix0 && mouse_pos.x <= ix1 && mouse_pos.y >= iy0 && mouse_pos.y <= iy1)
            {
              hovered_apl_block = &sp;
              dl->AddRect(ImVec2(ix0 - 1.0f, iy0 - 1.0f), ImVec2(ix1 + 1.0f, iy1 + 1.0f), IM_COL32(255, 255, 255, 255), 2.0f, 0, 2.0f);
            }
          }

          // Track 2: MCTS Optimal Cast Icons
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

          // Track 3: Cumulative DPS Plot
          float dps_h = 150.0f;
          ImVec2 dps_box_min(p0.x + 4.0f, p0.y + dps_plot_y);
          ImVec2 dps_box_max(p0.x + canvas_w - 40.0f, p0.y + dps_plot_y + dps_h);

          dl->AddRectFilled(dps_box_min, dps_box_max, IM_COL32(12, 14, 18, 240), 4.0f);
          dl->AddRect(dps_box_min, dps_box_max, IM_COL32(50, 50, 60, 200), 4.0f);

          double max_dps_val = 100.0;
          for (double v : current_run.ts_mcts_dps) max_dps_val = std::max(max_dps_val, v);
          for (double v : current_run.ts_apl_dps) max_dps_val = std::max(max_dps_val, v);
          max_dps_val = std::ceil(max_dps_val / 100.0) * 100.0 + 50.0;

          for (int d = 100; d < static_cast<int>(max_dps_val); d += 100)
          {
            float gy = dps_box_max.y - static_cast<float>(d / max_dps_val) * (dps_h - 10.0f);
            dl->AddLine(ImVec2(dps_box_min.x, gy), ImVec2(dps_box_max.x, gy), IM_COL32(40, 45, 55, 120));
            std::string label = std::to_string(d) + " DPS";
            dl->AddText(ImVec2(dps_box_min.x + 6.0f, gy - 12.0f), IM_COL32(140, 140, 150, 180), label.c_str());
          }

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

              if (mcts_y_a < apl_y_a || mcts_y_b < apl_y_b)
              {
                ImVec2 poly[4] = {ImVec2(x_a, apl_y_a), ImVec2(x_b, apl_y_b), ImVec2(x_b, mcts_y_b), ImVec2(x_a, mcts_y_a)};
                dl->AddConvexPolyFilled(poly, 4, IM_COL32(255, 215, 0, 40));
              }

              dl->AddLine(ImVec2(x_a, apl_y_a), ImVec2(x_b, apl_y_b), IM_COL32(79, 195, 247, 255), 2.0f);
              dl->AddLine(ImVec2(x_a, mcts_y_a), ImVec2(x_b, mcts_y_b), IM_COL32(255, 215, 0, 255), 2.5f);
            }
          }

          // Track 4: Player Mana Plot
          float mana_h = 150.0f;
          ImVec2 mana_box_min(p0.x + 4.0f, p0.y + mana_plot_y);
          ImVec2 mana_box_max(p0.x + canvas_w - 40.0f, p0.y + mana_plot_y + mana_h);

          dl->AddRectFilled(mana_box_min, mana_box_max, IM_COL32(12, 14, 18, 240), 4.0f);
          dl->AddRect(mana_box_min, mana_box_max, IM_COL32(50, 50, 60, 200), 4.0f);

          double max_mana_val = 1000.0;
          for (double v : current_run.ts_mcts_mana) max_mana_val = std::max(max_mana_val, v);
          for (double v : current_run.ts_apl_mana) max_mana_val = std::max(max_mana_val, v);
          max_mana_val = std::ceil(max_mana_val / 500.0) * 500.0;

          for (int m = 1000; m < static_cast<int>(max_mana_val); m += 1000)
          {
            float gy = mana_box_max.y - static_cast<float>(m / max_mana_val) * (mana_h - 10.0f);
            dl->AddLine(ImVec2(mana_box_min.x, gy), ImVec2(mana_box_max.x, gy), IM_COL32(40, 45, 55, 120));
            std::string label = std::to_string(m) + " Mana";
            dl->AddText(ImVec2(mana_box_min.x + 6.0f, gy - 12.0f), IM_COL32(140, 140, 150, 180), label.c_str());
          }

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

              dl->AddLine(ImVec2(x_a, apl_m_a), ImVec2(x_b, apl_m_b), IM_COL32(79, 195, 247, 255), 2.0f);
              dl->AddLine(ImVec2(x_a, mcts_m_a), ImVec2(x_b, mcts_m_b), IM_COL32(206, 147, 216, 255), 2.5f);
            }
          }

          // Global Mouse Time Cursor & Tooltip
          if (is_canvas_hovered && hovered_time >= 0.0)
          {
            float cx = p0.x + static_cast<float>(hovered_time) * px_per_sec;
            dl->AddLine(ImVec2(cx, p0.y + ruler_y), ImVec2(cx, p0.y + canvas_h - 10.0f), IM_COL32(255, 255, 255, 180), 1.5f);

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
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Combat Timeline: %.1fs / %.1fs", hovered_time, current_run.fight_duration);
            ImGui::Separator();

            if (hovered_apl_block)
            {
              ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "APL Cast: %s (%.1fs -> %.1fs)",
                                 hovered_apl_block->spell_name.c_str(), hovered_apl_block->start_time, hovered_apl_block->end_time);
              if (hovered_apl_block->damage > 0.0)
              {
                ImGui::TextColored(hovered_apl_block->is_crit ? ImVec4(1.0f, 0.85f, 0.0f, 1.0f) : ImVec4(0.4f, 0.9f, 0.4f, 1.0f),
                                   "  Damage: %.0f %s", hovered_apl_block->damage, hovered_apl_block->is_crit ? "[CRIT]" : "");
              }
            }

            if (hovered_mcts_block)
            {
              ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "MCTS Cast: %s (%.1fs -> %.1fs)",
                                 hovered_mcts_block->spell_name.c_str(), hovered_mcts_block->start_time, hovered_mcts_block->end_time);
              if (hovered_mcts_block->damage > 0.0)
              {
                ImGui::TextColored(hovered_mcts_block->is_crit ? ImVec4(1.0f, 0.85f, 0.0f, 1.0f) : ImVec4(0.4f, 0.9f, 0.4f, 1.0f),
                                   "  Damage: %.0f %s", hovered_mcts_block->damage, hovered_mcts_block->is_crit ? "[CRIT]" : "");
              }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "  APL:  %.1f DPS  |  %.0f Mana", cur_apl_dps, cur_apl_mana);
            ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "  MCTS: %.1f DPS  |  %.0f Mana", cur_mcts_dps, cur_mcts_mana);

            ImGui::EndTooltip();
          }
        }
        ImGui::EndChild();
      }
    }
    else
    {
      // ---------------------------------------------------------------------
      // VIEW C: IDLE INSTRUCTIONS (BEFORE EITHER IS RUN)
      // ---------------------------------------------------------------------
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "APL Strategy & Policy Trace Analyzer");
      ImGui::Spacing();
      ImGui::TextWrapped(
          "Choose an analysis tool above to evaluate your active priority list against dynamic Monte Carlo Tree Search rollouts:\n\n"
          " • APL Blunder Action Analysis: Single-episode evaluation across every decision step using Common Random Numbers (CRN) to isolate rotational blunders.\n"
          " • APL vs. Full MCTS Policy Trace: Simulates an autonomous MCTS AI play-through from start to finish to construct the optimal throughput ceiling and full combat timeline.");
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

      ImGui::Text("1) Select desired preset and APL from");
      ImGui::SameLine();
      render_tab_link("Current Configuration", AppTab::PRESETS);
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

      ImGui::Text("2) Modify APL if wanted in");
      ImGui::SameLine();
      render_tab_link("Current Configuration", AppTab::PRESETS);
      ImGui::SameLine(0, 0);
      ImGui::Text(".");

      ImGui::Spacing();

      ImGui::Text("3) Click 'Run Blunder Analysis' for instant decision-point flaw analysis, or 'Run Full MCTS Analysis' for synchronized timeline benchmarking.");

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      ImGui::TextDisabled("Select an analysis above to begin.");
    }
  }
  EndWowChild();
}

} // namespace warlock
