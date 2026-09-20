#pragma once
#include "imgui.h"
#include "src/sim/priest/parallel_runner.hpp"
#include "src/sim/priest/priest_sim.hpp"

namespace priest {

inline void render_priest_sim_control(PriestSimulator& sim,
                                      int& iterations,
                                      int& thread_count,
                                      BatchSimResult& last_result,
                                      bool& is_running,
                                      float& progress)
{
  // Big prominent RUN button
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.15f, 0.65f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.22f, 0.85f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.30f, 1.0f, 1.0f));

  if (is_running)
    ImGui::BeginDisabled();
  if (ImGui::Button(">>> RUN PRIEST DES SIMULATION <<<", ImVec2(-1, 38)))
  {
    is_running = true;
    progress = 0.0f;
    last_result = ParallelSimRunner::run_batch(sim, iterations, thread_count, [&](float p) { progress = p; });
    is_running = false;
    progress = 1.0f;
  }
  if (is_running)
    ImGui::EndDisabled();
  ImGui::PopStyleColor(3);

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
  ImGui::SliderScalar("Fight Duration (s)##Priest", ImGuiDataType_Double, &sim.fight_duration, &dur_min, &dur_max, "%.0f seconds");

  ImGui::Checkbox("Randomize Fight Duration##Priest", &sim.randomize_duration);
  if (sim.randomize_duration)
  {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140);
    double var_min = 1.0, var_max = 60.0;
    ImGui::SliderScalar("Variance (+/- s)##Priest", ImGuiDataType_Double, &sim.duration_variance, &var_min, &var_max, "+/- %.0fs");
  }

  // Target Level & Type
  const char* level_presets[] = {
      "Level 60 (Equal Lvl)", "Level 61 (+1 Lvl)", "Level 62 (+2 Lvl)", "Level 63 (Raid Boss)"};
  int current_lvl_idx =
      (sim.target_config.level >= 60 && sim.target_config.level <= 63) ? (sim.target_config.level - 60) : 3;
  ImGui::SetNextItemWidth(180);
  if (ImGui::Combo("Target Level##Priest", &current_lvl_idx, level_presets, IM_ARRAYSIZE(level_presets)))
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
  if (ImGui::Combo("Target Type##Priest", &current_type_idx, creature_types, IM_ARRAYSIZE(creature_types)))
  {
    sim.target_config.creature_type = static_cast<sim::CreatureType>(current_type_idx);
    sim.target_config.is_beast = (sim.target_config.creature_type == sim::CreatureType::BEAST);
  }

  ImGui::SliderInt("Iterations##Priest", &iterations, 1000, 100000, "%d fights");

  int max_threads = static_cast<int>(std::thread::hardware_concurrency());
  if (max_threads <= 0)
    max_threads = 4;
  ImGui::SliderInt("Worker Threads##Priest", &thread_count, 1, max_threads, "%d threads");
}

} // namespace priest
