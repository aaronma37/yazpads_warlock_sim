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

  ImGui::Text("Fight Duration (seconds):");
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x > 250.0f ? 240.0f : ImGui::GetContentRegionAvail().x);
  WowInputDouble("##FightDuration", &sim.fight_duration, 5.0, 30.0, "%.0f s");
  if (sim.fight_duration < 5.0) sim.fight_duration = 5.0;

  WowCheckbox("Randomize Fight Duration", &sim.randomize_duration);
  if (sim.randomize_duration)
  {
    ImGui::Text("Duration Variance (+/- seconds):");
    ImGui::SetNextItemWidth(140);
    WowInputDouble("##Variance", &sim.duration_variance, 1.0, 5.0, "+/- %.0f s");
    if (sim.duration_variance < 0.0) sim.duration_variance = 0.0;
  }

  // Target Level & Type
  const char* level_presets[] = {
      "Level 60 (Equal Lvl)", "Level 61 (+1 Lvl)", "Level 62 (+2 Lvl)", "Level 63 (Raid Boss)"};
  int current_lvl_idx =
      (sim.target_config.level >= 60 && sim.target_config.level <= 63) ? (sim.target_config.level - 60) : 3;
  ImGui::Text("Target Level & Creature Type:");
  ImGui::SetNextItemWidth(180);
  if (ImGui::Combo("##TargetLevel", &current_lvl_idx, level_presets, IM_ARRAYSIZE(level_presets)))
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
  if (ImGui::Combo("##TargetType", &current_type_idx, creature_types, IM_ARRAYSIZE(creature_types)))
  {
    sim.target_config.creature_type = static_cast<sim::CreatureType>(current_type_idx);
    sim.target_config.is_beast = (sim.target_config.creature_type == sim::CreatureType::BEAST);
  }

  ImGui::Text("Sim Iterations:");
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x > 250.0f ? 240.0f : ImGui::GetContentRegionAvail().x);
  WowInputInt("##SimIterations", &iterations, 500, 5000);
  if (iterations < 1) iterations = 1;

#if !defined(__EMSCRIPTEN__)
  int max_threads = static_cast<int>(std::thread::hardware_concurrency());
  if (max_threads <= 0)
    max_threads = 4;
  ImGui::Text("Worker Threads:");
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x > 250.0f ? 240.0f : ImGui::GetContentRegionAvail().x);
  WowInputInt("##WorkerThreads", &thread_count, 1, 4);
  if (thread_count < 1) thread_count = 1;
  if (thread_count > max_threads) thread_count = max_threads;
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
      sim, iterations, thread_count, last_result, is_running, progress, "RUN SIMULATIONS");
}

}  // namespace warlock
