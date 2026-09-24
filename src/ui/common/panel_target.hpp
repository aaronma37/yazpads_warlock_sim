#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"
#include "src/sim/stats.hpp"
#include "src/sim/warlock/parallel_runner.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/priest/priest_sim.hpp"
#include "src/sim/priest/parallel_runner.hpp"
#include "src/ui/common/damage_breakdown_view.hpp"
#include "src/ui/common/panel_results.hpp"
#include "src/ui/priest/panel_results.hpp"
#include <array>
#include <algorithm>
#include <thread>
#include <type_traits>

namespace warlock
{

template <typename SimType, typename BatchResultType, typename RunnerType>
inline void render_panel_sim_config(SimType& sim,
                                    int& iterations,
                                    int& thread_count,
                                    BatchResultType& last_result,
                                    bool& is_running,
                                    float& progress,
                                    const char* button_text = "RUN DES SIMULATION")
{
  float total_avail_w = ImGui::GetContentRegionAvail().x;
  float left_w = std::min(400.0f, std::max(320.0f, total_avail_w * 0.38f));

  // =========================================================================
  // LEFT SIDE: Sim Configuration
  // =========================================================================
  ImGui::BeginGroup();
  ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Sim Configuration");
  ImGui::Spacing();

  auto render_input_double = [](const char* label, const char* id, double* val, const char* fmt = "%.0f", float extra_w = 14.0f) {
    WowResetTextBaseline();
    ImGui::TextColored(ImVec4(0.92f, 0.85f, 0.72f, 1.0f), "%s", label);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.5f));
    float w = std::max(70.0f, ImGui::CalcTextSize(label).x + extra_w);
    ImGui::SetNextItemWidth(w);
    WowInputDouble(id, val, 0.0, 0.0, fmt);
    ImGui::PopStyleVar();
  };

  auto render_input_int = [](const char* label, const char* id, int* val, int min_v = 1, int max_v = 1000000, float extra_w = 14.0f) {
    WowResetTextBaseline();
    ImGui::TextColored(ImVec4(0.92f, 0.85f, 0.72f, 1.0f), "%s", label);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.5f));
    float w = std::max(70.0f, ImGui::CalcTextSize(label).x + extra_w);
    ImGui::SetNextItemWidth(w);
    WowInputInt(id, val, 1, 5);
    if (*val < min_v) *val = min_v;
    if (*val > max_v) *val = max_v;
    ImGui::PopStyleVar();
  };

  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, 2.0f));
  if (ImGui::BeginTable("SimConfigGrid", 2, ImGuiTableFlags_None, ImVec2(left_w, 0)))
  {
    ImGui::TableNextRow(ImGuiTableRowFlags_None, 36.0f);
    ImGui::TableNextColumn();
    render_input_double("Fight Duration (s)", "##FightDuration", &sim.fight_duration, "%.0f", 20.0f);
    ImGui::TableNextColumn();
    render_input_double("Variance (+/- s)", "##DurVariance", &sim.duration_variance, "%.0f", 20.0f);

    ImGui::TableNextRow(ImGuiTableRowFlags_None, 36.0f);
    ImGui::TableNextColumn();
    render_input_int("Iterations", "##SimIterations", &iterations, 100, 1000000, 20.0f);
    ImGui::TableNextColumn();
    render_input_int("Worker Threads", "##SimThreads", &thread_count, 1, 64, 20.0f);

    ImGui::EndTable();
  }
  ImGui::PopStyleVar();

  if (sim.duration_variance > 0.0)
  {
    sim.randomize_duration = true;
  }

  ImGui::Spacing();
  if (WowButton(button_text, ImVec2(left_w, 32.0f), !is_running))
  {
    is_running = true;
    progress = 0.0f;
    last_result = RunnerType::run_batch(sim, iterations, thread_count, [&](float p) { progress = p; });
    is_running = false;
    progress = 1.0f;
  }

  if (is_running)
  {
    ImGui::Spacing();
    ImGui::ProgressBar(progress, ImVec2(left_w, 6));
  }

  ImGui::EndGroup();

  ImGui::SameLine(0.0f, 24.0f);

  // =========================================================================
  // RIGHT SIDE: Simulation Results & Damage Breakdown
  // =========================================================================
  ImGui::BeginGroup();
  ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Simulation Results");
  ImGui::Spacing();

  if (last_result.total_iterations > 0)
  {
    ImGui::TextDisabled("MEAN DPS");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.40f, 1.0f), "%.1f DPS", last_result.mean_dps);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "(Min: %.0f  Max: %.0f  ±%.1f)",
                       last_result.min_dps, last_result.max_dps, last_result.std_dev_dps);

    ImGui::TextDisabled("Median: %.1f (P5: %.1f - P95: %.1f)  |  Crit: %.1f%%  |  Miss: %.1f%%",
                       last_result.p50_dps, last_result.p5_dps, last_result.p95_dps,
                       last_result.crit_percent, last_result.miss_percent);

    ImGui::Spacing();
    static bool open_modal = false;
    if (WowButton("🔍 Detailed View", ImVec2(140.0f, 24.0f)))
    {
      open_modal = true;
      ImGui::OpenPopup("Detailed Simulation Results##UnderTalentsModal");
    }

    ImGui::SetNextWindowSize(ImVec2(860, 620), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Detailed Simulation Results##UnderTalentsModal", &open_modal, ImGuiWindowFlags_None))
    {
      if constexpr (std::is_same_v<SimType, WarlockSimulator>)
      {
        render_panel_results(last_result);
      }
      else
      {
        priest::render_priest_panel_results(last_result);
      }
      ImGui::Spacing();
      if (WowButton("Close", ImVec2(100, 28)))
      {
        open_modal = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.92f, 0.85f, 0.72f, 1.0f), "Damage Breakdown:");
    float right_avail_w = ImGui::GetContentRegionAvail().x;
    float label_offset = 120.0f;
    float bar_w = std::max(60.0f, right_avail_w - label_offset - 12.0f);
    if constexpr (std::is_same_v<SimType, WarlockSimulator>)
    {
      render_damage_breakdown_bars(last_result, bar_w, label_offset);
    }
    else
    {
      priest::render_priest_damage_breakdown_bars(last_result, bar_w, label_offset);
    }
  }
  else
  {
    ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "DPS: -- (Ready to simulate. Click '%s' to run)", button_text);
  }

  ImGui::EndGroup();
}

template <typename SimType, typename BatchResultType, typename RunnerType>
inline void render_panel_target(SimType& sim,
                                int& iterations,
                                int& thread_count,
                                BatchResultType& last_result,
                                bool& is_running,
                                float& progress,
                                const char* button_text = "RUN DES SIMULATION")
{
  render_panel_sim_config<SimType, BatchResultType, RunnerType>(
      sim, iterations, thread_count, last_result, is_running, progress, button_text);
}

inline void render_panel_target(WarlockSimulator& sim,
                                int& iterations,
                                int& thread_count,
                                BatchSimResult& last_result,
                                bool& is_running,
                                float& progress)
{
  render_panel_sim_config<WarlockSimulator, BatchSimResult, ParallelSimRunner>(
      sim, iterations, thread_count, last_result, is_running, progress, "RUN DES SIMULATION");
}

inline void render_panel_target(TargetConfig& target,
                                double& fight_duration,
                                bool& randomize_duration,
                                double& duration_variance)
{
  WarlockSimulator dummy_sim;
  dummy_sim.target_config = target;
  dummy_sim.fight_duration = fight_duration;
  dummy_sim.randomize_duration = randomize_duration;
  dummy_sim.duration_variance = duration_variance;
  int iter = 10000, thr = 4;
  BatchSimResult dummy_res;
  bool running = false;
  float prog = 0.0f;
  render_panel_sim_config<WarlockSimulator, BatchSimResult, ParallelSimRunner>(
      dummy_sim, iter, thr, dummy_res, running, prog);
  target = dummy_sim.target_config;
  fight_duration = dummy_sim.fight_duration;
  randomize_duration = dummy_sim.randomize_duration;
  duration_variance = dummy_sim.duration_variance;
}

}  // namespace warlock
