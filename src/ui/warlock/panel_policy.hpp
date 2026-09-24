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

inline std::vector<PriorityRule> get_available_warlock_actions(const Talents& talents, Race race)
{
  std::vector<PriorityRule> list;
  list.push_back({PriorityAction::SHADOW_BOLT_FILLER, SpellID::SHADOW_BOLT, "Shadow Bolt", "Always / Filler", "Trigger when: No higher priority spells are ready.", "Main Shadow direct damage spell."});
  list.push_back({PriorityAction::INCINERATE_FILLER, SpellID::INCINERATE, "Incinerate", "Always / Filler", "Trigger when: No higher priority spells are ready.", "Main Fire direct damage spell (+25% bonus against Immolated targets)."});
  list.push_back({PriorityAction::SEARING_PAIN_FILLER, SpellID::SEARING_PAIN, "Searing Pain", "Always / Filler", "Trigger when: No higher priority spells are ready.", "Fast 1.5s cast Fire damage spell."});
  list.push_back({PriorityAction::DRAIN_SOUL_FILLER, SpellID::DRAIN_SOUL, "Drain Soul", "Always / Filler", "Trigger when: No higher priority spells are ready.", "Channeled Affliction shadow drain filler."});
  list.push_back({PriorityAction::LIFE_TAP, SpellID::LIFE_TAP, "Life Tap", "Mana <= 30% & HP > 800", "Trigger when: Current Mana <= 30% and Health > 800.", "Converts health into mana on global cooldown."});
  list.push_back({PriorityAction::CORRUPTION, SpellID::CORRUPTION, "Corruption", "DoT Expired / Missing", "Trigger when: Target does not have active Corruption.", "Maintains 18s ticking Shadow DoT."});
  list.push_back({PriorityAction::IMMOLATE, SpellID::IMMOLATE, "Immolate", "DoT Expired / Missing", "Trigger when: Target does not have active Immolate.", "Maintains 15s ticking Fire DoT and enables Incinerate/Conflagrate."});
  list.push_back({PriorityAction::CURSE_OF_AGONY, SpellID::CURSE_OF_AGONY, "Bane of Agony", "DoT Expired / Missing", "Trigger when: Target does not have active Bane of Agony.", "Maintains 24s ramping Shadow DoT."});
  list.push_back({PriorityAction::CURSE_OF_DOOM, SpellID::CURSE_OF_DOOM, "Curse of Doom", "Target Missing Curse & >60s Left", "Trigger when: Target has no curse and >60s remain in combat.", "Deals massive delayed Shadow damage after 60s."});
  if (talents.destro.conflagrate > 0)
    list.push_back({PriorityAction::CONFLAGRATE, SpellID::CONFLAGRATE, "Conflagrate", "Immolate Active & CD Ready", "Trigger when: Target is Immolated and Conflagrate CD is ready (10s).", "Consumes Immolate for instant Fire burst damage."});
  if (talents.destro.shadowburn > 0)
    list.push_back({PriorityAction::SHADOWBURN, SpellID::SHADOWBURN, "Shadowburn", "CD Ready & Soul Shard Available", "Trigger when: Shadowburn CD is ready (15s).", "Instant cast Shadow burst spell."});
  if (talents.aff.siphon_life > 0)
    list.push_back({PriorityAction::SIPHON_LIFE, SpellID::SIPHON_LIFE, "Siphon Life", "DoT Expired / Missing", "Trigger when: Target does not have active Siphon Life.", "Maintains 30s ticking Shadow DoT."});
  if (talents.aff.amplify_curse > 0)
    list.push_back({PriorityAction::AMPLIFY_CURSE, SpellID::AMPLIFY_CURSE, "Amplify Curse", "CD Ready & Curse Cast", "Trigger when: Amplify Curse CD is ready (180s).", "Boosts next Bane of Agony base damage by 50%."});
  if (talents.demo.decimation > 0)
    list.push_back({PriorityAction::DECIMATION_SOUL_FIRE, SpellID::SOUL_FIRE, "Decimation: Soul Fire", "Target < 35% HP & Decimation Active", "Trigger when: Target HP < 35% and Decimation buff is active.", "Spams fast cast Soul Fire during execute phase."});
  if (talents.aff.drain_hope > 0)
    list.push_back({PriorityAction::DRAIN_HOPE, SpellID::DRAIN_HOPE, "Drain Hope", "Target < 20% HP", "Trigger when: Target HP < 20%.", "Channels execute drain on low health targets."});
  if (race == Race::ORC)
    list.push_back({PriorityAction::RACIAL_BLOOD_FURY, SpellID::NONE, "Blood Fury", "On Cooldown", "Trigger when: Blood Fury CD is ready (120s).", "Racial ability: Increases base spell damage for 15s."});
  else if (race == Race::TROLL)
    list.push_back({PriorityAction::RACIAL_BERSERKING, SpellID::NONE, "Berserking", "On Cooldown", "Trigger when: Berserking CD is ready (180s).", "Racial ability: Increases spell casting speed for 10s."});
  else if (race == Race::GNOME)
    list.push_back({PriorityAction::RACIAL_EUREKA, SpellID::NONE, "Eureka", "On Cooldown", "Trigger when: Eureka CD is ready (120s).", "Racial ability: Restores mana and grants spell power."});
  return list;
}

inline void render_panel_policy_controls(PolicyConfig& policy, const Talents& talents, Race race = Race::UNDEAD)
{
  ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Action Priority List");
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
    policy.use_custom_apl = false;
    policy.custom_rules.clear();
  }

  ImGui::Spacing();

  if (policy.use_custom_apl)
  {
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Custom APL Active (Modified)");
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset to Preset Defaults"))
    {
      policy.reset_to_preset(talents, race);
    }
  }

  if (ImGui::BeginTable("WarlockAplTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
  {
    ImGui::TableSetupColumn("Action / Spell", ImGuiTableColumnFlags_WidthStretch, 0.44f);
    ImGui::TableSetupColumn("Trigger Condition", ImGuiTableColumnFlags_WidthStretch, 0.56f);
    ImGui::TableSetupColumn("##ModifyCol", ImGuiTableColumnFlags_WidthFixed, 26.0f);
    ImGui::TableHeadersRow();

    std::vector<PriorityRule> current_rules = policy.get_priority_rules(talents, race);
    std::vector<PriorityRule> available_rules = get_available_warlock_actions(talents, race);

    for (size_t i = 0; i < current_rules.size(); ++i)
    {
      const auto& r = current_rules[i];
      ImGui::TableNextRow();
      ImGui::PushID(static_cast<int>(i));

      // Col 0: Action / Spell Icon + Name
      ImGui::TableSetColumnIndex(0);
      Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(r.spell_id));
      if (icon.id > 0)
      {
        ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
        ImGui::SameLine(0, 4);
      }
      ImGui::Text("%s", r.name.c_str());

      // Col 1: Condition Summary
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(r.condition_summary.c_str());

      // Col 2: Modify button & popup
      ImGui::TableSetColumnIndex(2);
      std::string btn_label = "##RuleMod_" + std::to_string(i);
      if (WowBiggerButton(btn_label.c_str(), ImVec2(20, 20)))
      {
        ImGui::OpenPopup("ModifyRulePopup");
      }

      if (ImGui::BeginPopup("ModifyRulePopup"))
      {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "#%zu %s", i + 1, r.name.c_str());
        ImGui::Separator();

        if (i > 0)
        {
          if (ImGui::MenuItem("▲ Move Up"))
          {
            policy.move_rule_up(i, talents, race);
          }
        }
        else
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem("▲ Move Up");
          ImGui::EndDisabled();
        }

        if (i + 1 < current_rules.size())
        {
          if (ImGui::MenuItem("▼ Move Down"))
          {
            policy.move_rule_down(i, talents, race);
          }
        }
        else
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem("▼ Move Down");
          ImGui::EndDisabled();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("➕ Add Rule Above"))
        {
          for (const auto& avail : available_rules)
          {
            if (ImGui::MenuItem(avail.name.c_str()))
            {
              policy.insert_rule(i, avail, talents, race);
            }
          }
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("➕ Add Rule Below"))
        {
          for (const auto& avail : available_rules)
          {
            if (ImGui::MenuItem(avail.name.c_str()))
            {
              policy.insert_rule(i + 1, avail, talents, race);
            }
          }
          ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("❌ Remove Rule"))
        {
          policy.remove_rule(i, talents, race);
        }

        ImGui::EndPopup();
      }

      ImGui::PopID();
    }
    ImGui::EndTable();
  }

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

  // 3. Multi-Target Combat Policy
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
