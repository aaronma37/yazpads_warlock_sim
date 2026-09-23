#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"
#include "src/sim/parallel_runner.hpp"
#include "src/sim/warlock_sim.hpp"
#include <thread>
#include <algorithm>

namespace warlock
{

template <typename SimType, typename BatchResultType, typename RunnerType>
inline void render_common_sim_control(SimType& sim,
                                      int& iterations,
                                      int& thread_count,
                                      BatchResultType& last_result,
                                      bool& is_running,
                                      float& progress,
                                      const char* button_text = "RUN DES SIMULATION")
{
  if (WowButton(button_text, ImVec2(-1, 38), !is_running))
  {
    is_running = true;
    progress = 0.0f;
    last_result = RunnerType::run_batch(sim, iterations, thread_count, [&](float p) { progress = p; });
    is_running = false;
    progress = 1.0f;
  }

  if (is_running)
  {
    ImGui::ProgressBar(progress, ImVec2(-1, 6));
  }

  if (last_result.total_iterations > 0)
  {
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                       "Completed %d sims in %.3fs (%.0f sims/sec)",
                       last_result.total_iterations,
                       last_result.total_sim_time_seconds,
                       last_result.iterations_per_second);
  }

  ImGui::Separator();

  // Encounter & Simulation Parameters
  ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Encounter & Simulation Parameters:");
  double dur_min = 10.0, dur_max = 600.0;
  WowInputDouble("Fight Duration (s)",  &sim.fight_duration, 0, 0, "%.0f seconds");

  WowCheckbox("Randomize Fight Duration", &sim.randomize_duration);
  if (sim.randomize_duration)
  {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    double var_min = 1.0, var_max = 60.0;
    WowInputDouble("Variance (+/- s)", &sim.duration_variance, 0,0, "+/- %.0fs");
  }

  // Target Level & Type
  const char* level_presets[] = {
      "Level 60 (Equal Lvl)", "Level 61 (+1 Lvl)", "Level 62 (+2 Lvl)", "Level 63 (Raid Boss)"};
  int current_lvl_idx =
      (sim.target_config.level >= 60 && sim.target_config.level <= 63) ? (sim.target_config.level - 60) : 3;
  ImGui::SetNextItemWidth(180);
  if (ImGui::Combo("Target Level", &current_lvl_idx, level_presets, IM_ARRAYSIZE(level_presets)))
  {
    sim.target_config.level = 60 + current_lvl_idx;
  }

  const char* creature_types[] = {
      "Humanoid", "Beast (Troll +5%)", "Demon", "Undead", "Dragonkin", "Elemental", "Giant", "Mechanical", "Other"};
  int current_type_idx = static_cast<int>(sim.target_config.creature_type);
  if (current_type_idx < 0 || current_type_idx >= IM_ARRAYSIZE(creature_types))
    current_type_idx = 0;
  ImGui::SameLine();
  ImGui::SetNextItemWidth(180);
  if (ImGui::Combo("Target Type", &current_type_idx, creature_types, IM_ARRAYSIZE(creature_types)))
  {
    sim.target_config.creature_type = static_cast<sim::CreatureType>(current_type_idx);
    sim.target_config.is_beast = (sim.target_config.creature_type == sim::CreatureType::BEAST);
  }

  WowSliderInt("Iterations", &iterations, 1000, 100000, "%d fights");

#if !defined(__EMSCRIPTEN__)
  int max_threads = static_cast<int>(std::thread::hardware_concurrency());
  if (max_threads <= 0)
    max_threads = 4;
  WowSliderInt("Worker Threads", &thread_count, 1, max_threads, "%d threads");
#endif
}

inline void render_panel_sim_control(WarlockSimulator& sim,
                                     int& iterations,
                                     int& thread_count,
                                     BatchSimResult& last_result,
                                     bool& is_running,
                                     float& progress)
{
  render_common_sim_control<WarlockSimulator, BatchSimResult, ParallelSimRunner>(
      sim, iterations, thread_count, last_result, is_running, progress, "RUN DES SIMULATION");
}

}  // namespace warlock
