#pragma once
#include "asset_manager.hpp"
#include "imgui.h"
#include "wow_widgets.hpp"
#include "src/sim/policy.hpp"
#include "src/sim/talents.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/ui/common/panel_policy.hpp"
#include <algorithm>
#include <vector>

namespace warlock
{

inline void render_priority_chain_subpane(const std::vector<PriorityRule>& rules)
{
  std::vector<CommonPriorityRule> common_rules;
  common_rules.reserve(rules.size());
  for (const auto& r : rules)
  {
    ImU32 bg_col = IM_COL32(56, 46, 76, 240);
    ImU32 border_col = IM_COL32(140, 90, 204, 230);
    if (r.action == PriorityAction::LIFE_TAP)
    {
      bg_col = IM_COL32(89, 30, 30, 240);
      border_col = IM_COL32(230, 76, 76, 230);
    }
    else if (r.action == PriorityAction::NIGHTFALL_SHADOW_BOLT ||
             r.action == PriorityAction::DECIMATION_SOUL_FIRE)
    {
      bg_col = IM_COL32(89, 64, 20, 240);
      border_col = IM_COL32(255, 204, 51, 230);
    }
    else if (r.action == PriorityAction::DECIMATION_SEARING_PAIN ||
             r.action == PriorityAction::DEMONIC_BRAND_SEARING_PAIN)
    {
      bg_col = IM_COL32(90, 45, 65, 240);
      border_col = IM_COL32(230, 90, 150, 230);
    }
    else if (r.action == PriorityAction::CONFLAGRATE || r.action == PriorityAction::INCINERATE_FILLER)
    {
      bg_col = IM_COL32(82, 46, 25, 240);
      border_col = IM_COL32(255, 140, 51, 230);
    }
    else if (r.action == PriorityAction::RACIAL_EUREKA || r.action == PriorityAction::RACIAL_BLOOD_FURY ||
             r.action == PriorityAction::RACIAL_BERSERKING)
    {
      bg_col = IM_COL32(25, 65, 85, 240);
      border_col = IM_COL32(80, 200, 240, 230);
    }
    else if (r.action == PriorityAction::AMPLIFY_CURSE)
    {
      bg_col = IM_COL32(65, 30, 85, 240);
      border_col = IM_COL32(190, 90, 230, 230);
    }

    common_rules.push_back({
        spell_id_to_icon(r.spell_id),
        r.name,
        std::string("Base Spell: ") + spell_id_to_name(r.spell_id) + " | Condition: " + r.condition_summary,
        r.trigger_condition,
        r.rule_explanation,
        bg_col,
        border_col
    });
  }

  render_common_priority_chain(common_rules);
}

inline void render_panel_policy_controls(PolicyConfig& policy, const Talents& talents, Race race = Race::UNDEAD)
{
  ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Combat Policy & Priority Rules:");
  ImGui::Separator();

  // 0. Rotation Choice / Strategy
  ImGui::Text("Active Spell Rotation Preset:");
  int rot_idx = static_cast<int>(policy.rotation);
  const char* rot_names[] = {
      "Shadow Destro - Conflag Weave + Decimation",                   // 0  SHADOW_DESTRO
      "Shadow Destro - Conflag Weave",                                // 1  SHADOW_DESTRO_2
      "Fire Destro - Incinerate + Conflag",                           // 2  FIRE_DESTRO
      "Demonology Shadow - Corruption + Bane + SB",                   // 3  DP_AF_SHADOW
      "Demonology Fire - Searing Pain",                               // 4  DP_RUIN_FIRE
      "Deep Affliction - Wrack",                                      // 5  DEEP_AFFLICTION
      "Shadow Mastery - DoTs + SB",                                   // 6  SM_RUIN
      "Demo Execute - Decimation Soul Fire",                          // 7  DEMONOLOGY_EXECUTE
      "Pure Shadow Bolt - No DoTs",                                   // 8  PURE_SHADOW_BOLT
      "Affliction Hybrid - Multi-DoT",                                // 9  AFFLICTION_HYBRID_DOTS
      "Shadow & Flame Fire - Incinerate + Conflag",                   // 10 SHADOW_AND_FLAME_FIRE_2
      "Demonology Shadow - Bane + SB",                                // 11 DP_AF_SHADOW_NO_CORRUPTION
      "Fire Destro - Incinerate + Conflag (No Corruption)",           // 12 FIRE_DESTRO_NO_CORRUPTION
      "Demonology Shadow - Corruption + Bane + SB (No Soul Fire)",    // 13 DP_AF_SHADOW_NO_SOUL_FIRE
      "Demonology Shadow - Corruption + SB (No Bane)",                // 14 DP_AF_SHADOW_NO_BANE
      "Demonology Shadow - Corruption + SB (No Soul Fire, No Bane)",  // 15 DP_AF_SHADOW_NO_SOUL_FIRE_NO_BANE
      "Deep Affliction - Wrack (SB Filler)",                          // 16 DEEP_AFFLICTION_SB
      "Deep Affliction - Wrack (SB Filler, No Siphon Life)",          // 17 DEEP_AFFLICTION_SB_NO_SL
      "Shadow & Flame Fire - Incinerate + Conflag + Bane",            // 18 SHADOW_AND_FLAME_FIRE_BANE
      "Demonology Shadow - Demonic Brand Weave",                      // 19 DP_AF_SHADOW_BRAND
  };
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::Combo("##RotationCombo", &rot_idx, rot_names, IM_ARRAYSIZE(rot_names)))
  {
    policy.rotation = static_cast<RotationChoice>(rot_idx);
  }


  ImGui::Spacing();

  // 1. Dynamic Rule-Based Priority Chain Subpane (<Spell> > <Spell> > <Spell>)
  std::vector<PriorityRule> rules = policy.get_priority_rules(talents, race);
  render_priority_chain_subpane(rules);

  // 2. Racial Ability Strategy
  if (race == Race::GNOME || race == Race::ORC || race == Race::TROLL)
  {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Racial Ability Strategy:");
    int racial_idx = static_cast<int>(policy.racial_policy);
    const char* racial_names[] = {
        "Execute Phase (<35% HP) — Save for execute abilities burst",
        "On Cooldown (Opener) — Fire at combat start and on CD",
        "Smart Execute Alignment — Opener if fight length allows recast in execute, else <35% HP",
        "Align with Curse of Doom — Pop 0-6s before Doom tick, else Execute/CD"};
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::Combo("##RacialPolicyCombo", &racial_idx, racial_names, IM_ARRAYSIZE(racial_names)))
    {
      policy.racial_policy = static_cast<RacialPolicy>(racial_idx);
    }
  }

  ImGui::Spacing();
  ImGui::Separator();

  // 3. Spell & Rotational Policy Ability Toggles
  ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Rotational Ability Policies:");
  WowCheckbox("Cast Conflagrate on Cooldown##Policy", &policy.use_conflagrate);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("When talented and Immolate is active, casts Conflagrate on 10s cooldown.");
  }
  WowCheckbox("Decimation Soul Fire (<35% HP)##Policy", &policy.use_decimation_soul_fire);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("When talented and in execute phase (<35%% HP), spams Soul Fire during Decimation buff.");
  }

  ImGui::Spacing();
  ImGui::Separator();

  // 4. Multi-Target Combat Policy
  ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Multi-Target Combat Policy:");
  WowCheckbox("Multi-DoT Corruption##Policy", &policy.multi_dot_corruption);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("When 2+ targets are configured, automatically maintains Corruption on secondary targets.");
  }
  WowCheckbox("Auto-Apply Bane of Havoc##Policy", &policy.auto_bane_of_havoc);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip(
        "When 2+ targets are configured, places Bane of Havoc on secondary target to duplicate 15%% damage dealt.");
  }
}

inline void render_panel_policy(PolicyConfig& policy, const Talents& talents, Race race = Race::UNDEAD)
{
  render_panel_policy_controls(policy, talents, race);
}

inline void render_panel_policy(WarlockSimulator& sim)
{
  render_panel_policy_controls(sim.policy, sim.talents, sim.race);
}

}  // namespace warlock
