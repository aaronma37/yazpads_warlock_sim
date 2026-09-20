#pragma once
#include "imgui.h"
#include "src/sim/stats.hpp"
#include "src/sim/warlock_sim.hpp"
#include <array>

namespace warlock
{

inline void render_panel_target(TargetConfig& target,
                                double& fight_duration,
                                bool& randomize_duration,
                                double& duration_variance)
{
  if (ImGui::CollapsingHeader("Target & Encounter Configuration", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Indent(8.0f);

    // 1. Fight Duration
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Fight Duration:");
    double dur_min = 10.0, dur_max = 600.0;
    ImGui::SliderScalar("##FightDurationSlider", ImGuiDataType_Double, &fight_duration, &dur_min, &dur_max, "%.0f seconds");

    // 1a. Randomize Fight Length Toggle
    ImGui::Spacing();
    ImGui::Checkbox("Randomize Fight Duration", &randomize_duration);
    if (randomize_duration)
    {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(140);
      double var_min = 1.0, var_max = 60.0;
      ImGui::SliderScalar("Variance (+/- s)##DurVariance", ImGuiDataType_Double, &duration_variance, &var_min, &var_max, "+/- %.0fs");
      double min_d = std::max(5.0, fight_duration - duration_variance);
      double max_d = fight_duration + duration_variance;
      ImGui::TextDisabled("  -> Actual range: [%.0fs - %.0fs] uniform distribution per fight", min_d, max_d);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 1b. Target Count
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Encounter Target Count:");
    ImGui::SliderInt("##TargetCountSlider", &target.target_count, 1, 5, "%d Target(s)");
    if (target.target_count > 1)
    {
      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f),
                         "  -> Multi-Target Active (%d targets). Cleave & multi-DoTs enabled.",
                         target.target_count);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Target Level
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Target Level:");
    const char* level_presets[] = {"Level 60 (Equal Level - 4% Base Miss)",
                                   "Level 61 (+1 Level - 5% Base Miss)",
                                   "Level 62 (+2 Level - 6% Base Miss)",
                                   "Level 63 (Raid Boss - 17% Base Miss - Default)"};
    int current_lvl_idx = (target.level >= 60 && target.level <= 63) ? (target.level - 60) : 3;
    if (ImGui::Combo("Preset Level##TargetLevelCombo", &current_lvl_idx, level_presets, IM_ARRAYSIZE(level_presets)))
    {
      target.level = 60 + current_lvl_idx;
    }
    ImGui::SliderInt("Custom Level", &target.level, 55, 65, "Level %d");

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Creature Type (Beast for Troll Beast Slaying, etc.)
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Target Creature Type:");
    const char* creature_types[] = {"Humanoid",
                                    "Beast (Troll Beast Slaying +5% Dmg)",
                                    "Demon",
                                    "Undead",
                                    "Dragonkin",
                                    "Elemental",
                                    "Giant",
                                    "Mechanical",
                                    "Other"};
    int current_type_idx = static_cast<int>(target.creature_type);
    if (current_type_idx < 0 || current_type_idx >= IM_ARRAYSIZE(creature_types))
      current_type_idx = 0;
    if (ImGui::Combo("Creature Type##Combo", &current_type_idx, creature_types, IM_ARRAYSIZE(creature_types)))
    {
      target.creature_type = static_cast<CreatureType>(current_type_idx);
      target.is_beast = (target.creature_type == CreatureType::BEAST);
    }

    if (target.creature_type == CreatureType::BEAST || target.is_beast)
    {
      ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.80f, 1.0f),
                         "  -> Beast target: Troll racial Beast Slaying grants +5%% damage bonus.");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 4. Target Base Resistances
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Base Innate Resistances (Uncursed):");
    float shadow_res = (float)target.base_shadow_resistance;
    if (ImGui::SliderFloat("Shadow Resistance", &shadow_res, 0.0f, 300.0f, "%.0f res"))
    {
      target.base_shadow_resistance = shadow_res;
    }
    float fire_res = (float)target.base_fire_resistance;
    if (ImGui::SliderFloat("Fire Resistance", &fire_res, 0.0f, 300.0f, "%.0f res"))
    {
      target.base_fire_resistance = fire_res;
    }

    ImGui::Unindent(8.0f);
  }
}

}  // namespace warlock
