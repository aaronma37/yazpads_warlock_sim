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
  list.push_back({PriorityAction::SEARING_PAIN_FILLER, SpellID::SEARING_PAIN, "Searing Pain", "Always / Filler", "Trigger when: Fire filler spell.", "Fast 1.5s cast Fire damage spell."});
  list.push_back({PriorityAction::DRAIN_SOUL_FILLER, SpellID::DRAIN_SOUL, "Drain Soul", "Always / Filler", "Trigger when: No higher priority spells are ready.", "Channeled Affliction shadow drain filler."});
  list.push_back({PriorityAction::LIFE_TAP, SpellID::LIFE_TAP, "Life Tap", "Mana <= 25%", "Trigger when: Current Mana <= configured threshold.", "Converts health into mana on global cooldown."});
  list.push_back({PriorityAction::CORRUPTION, SpellID::CORRUPTION, "Corruption", "DoT Expired / Missing", "Trigger when: Target does not have active Corruption.", "Maintains 18s ticking Shadow DoT."});
  list.push_back({PriorityAction::IMMOLATE, SpellID::IMMOLATE, "Immolate", "DoT Expired / Missing", "Trigger when: Target does not have active Immolate.", "Maintains 15s ticking Fire DoT and enables Incinerate/Conflagrate."});
  list.push_back({PriorityAction::CURSE_OF_AGONY, SpellID::CURSE_OF_AGONY, "Bane of Agony", "DoT Expired / Missing", "Trigger when: Target does not have active Bane of Agony.", "Maintains 24s ramping Shadow DoT."});
  list.push_back({PriorityAction::CURSE_OF_DOOM, SpellID::CURSE_OF_DOOM, "Bane of Doom", "Target Missing Curse & >60s Left", "Trigger when: Target has no curse and >60s remain in combat.", "Deals massive delayed Shadow damage after 60s."});
  list.push_back({PriorityAction::NIGHTFALL_SHADOW_BOLT, SpellID::SHADOW_BOLT, "Nightfall Shadow Bolt", "Shadow Trance Active", "Trigger when: Shadow Trance buff is active.", "Instant cast Shadow Bolt on Nightfall proc."});
  if (talents.destro.conflagrate > 0)
    list.push_back({PriorityAction::CONFLAGRATE, SpellID::CONFLAGRATE, "Conflagrate", "Immolate Active & CD Ready", "Trigger when: Target is Immolated and Conflagrate CD is ready (10s).", "Consumes Immolate for instant Fire burst damage."});
  if (talents.destro.shadowburn > 0)
    list.push_back({PriorityAction::SHADOWBURN, SpellID::SHADOWBURN, "Shadowburn", "CD Ready & Soul Shard Available", "Trigger when: Shadowburn CD is ready (15s).", "Instant cast Shadow burst spell."});
  if (talents.aff.siphon_life > 0)
    list.push_back({PriorityAction::SIPHON_LIFE, SpellID::SIPHON_LIFE, "Siphon Life", "DoT Expired / Missing", "Trigger when: Target does not have active Siphon Life.", "Maintains 30s ticking Shadow DoT."});
  if (talents.aff.amplify_curse > 0)
    list.push_back({PriorityAction::AMPLIFY_CURSE, SpellID::AMPLIFY_CURSE, "Amplify Curse", "CD Ready & Curse Cast", "Trigger when: Amplify Curse CD is ready (180s).", "Boosts next Bane of Agony base damage by 50%."});
  if (talents.demo.decimation > 0)
  {
    list.push_back({PriorityAction::DECIMATION_SEARING_PAIN, SpellID::SEARING_PAIN, "Decimation Trigger (Searing Pain)", "Target HP <= 35% & Decimation Inactive", "Trigger when: Target HP <= 35% and Decimation buff is inactive.", "Fast cast Searing Pain to trigger Decimation buff."});
    list.push_back({PriorityAction::DECIMATION_SOUL_FIRE, SpellID::SOUL_FIRE, "Decimation Soul Fire", "Target HP <= 35% & Decimation Active", "Trigger when: Target HP <= 35% and Decimation buff is active.", "Spams fast cast Soul Fire during execute phase."});
  }
  if (talents.demo.demonic_brand > 0)
    list.push_back({PriorityAction::DEMONIC_BRAND_SEARING_PAIN, SpellID::DEMONIC_BRAND, "Demonic Brand (Searing Pain)", "Demonic Brand Down", "Trigger when: Target is missing Demonic Brand.", "Brands target to empower pet attacks."});
  if (talents.aff.drain_hope > 0)
    list.push_back({PriorityAction::DRAIN_HOPE, SpellID::DRAIN_HOPE, "Wrack", "DoT Expired / Missing", "Trigger when: Wrack is not active on target (refreshed every 6s).", "Tears the target apart from within, dealing ticking Shadow damage and increasing other Shadow DoT damage by 10%."});
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

  // Edit Conditions Modal State
  static bool open_edit_conditions_modal = false;
  static int editing_rule_index = -1;
  static PriorityRule editing_rule;

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

      // Col 1: Trigger Condition
      ImGui::TableSetColumnIndex(1);
      std::string cond_display = r.format_condition_summary();
      ImGui::TextUnformatted(cond_display.c_str());
      if (!r.trigger_condition.empty() && ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("%s", r.trigger_condition.c_str());
      }

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

        if (ImGui::MenuItem("⚙️ Edit Conditions..."))
        {
          open_edit_conditions_modal = true;
          editing_rule_index = static_cast<int>(i);
          editing_rule = r;
          editing_rule.use_custom_thresholds = true;

          // If the rule was using default preset triggers (no checkboxes enabled yet), initialize them intelligently:
          if (!editing_rule.check_mana && !editing_rule.check_target_hp && !editing_rule.check_dot_refresh &&
              !editing_rule.check_fight_time && !editing_rule.check_isb_debuff &&
              !editing_rule.check_shadow_trance && !editing_rule.check_decimation && !editing_rule.check_demonic_brand &&
              !editing_rule.check_doom_debuff)
          {
            if (editing_rule.action == PriorityAction::LIFE_TAP)
            {
              editing_rule.check_mana = true;
              if (editing_rule.max_mana_pct >= 0.999f)
              {
                editing_rule.max_mana_pct = static_cast<float>(policy.life_tap_threshold_pct / 100.0);
              }
            }
            else if (editing_rule.action == PriorityAction::NIGHTFALL_SHADOW_BOLT)
            {
              editing_rule.check_shadow_trance = true;
            }
            else if (editing_rule.action == PriorityAction::DECIMATION_SOUL_FIRE)
            {
              editing_rule.check_target_hp = true;
              editing_rule.max_target_hp_pct = 0.35f;
              editing_rule.check_decimation = true;
              editing_rule.require_decimation_active = true;
            }
            else if (editing_rule.action == PriorityAction::DECIMATION_SEARING_PAIN)
            {
              editing_rule.check_target_hp = true;
              editing_rule.max_target_hp_pct = 0.35f;
              editing_rule.check_decimation = true;
              editing_rule.require_decimation_active = false;
            }
            else if (editing_rule.action == PriorityAction::DEMONIC_BRAND_SEARING_PAIN)
            {
              editing_rule.check_demonic_brand = true;
              editing_rule.require_demonic_brand_missing = true;
            }
            else if (editing_rule.action == PriorityAction::SHADOWBURN && policy.shadowburn == ShadowburnPolicy::EXECUTE_ONLY)
            {
              editing_rule.check_target_hp = true;
              editing_rule.max_target_hp_pct = 0.20f;
            }
            else if (editing_rule.action == PriorityAction::CURSE_OF_DOOM)
            {
              editing_rule.check_fight_time = true;
              editing_rule.min_time_remaining = 60.0f;
            }
            else if (editing_rule.action == PriorityAction::CURSE_OF_AGONY)
            {
              editing_rule.check_dot_refresh = true;
              editing_rule.max_dot_rem_sec = 0.0f;
              editing_rule.check_doom_debuff = true;
              editing_rule.require_doom_missing = true;
            }
            else if (editing_rule.action == PriorityAction::CORRUPTION ||
                     editing_rule.action == PriorityAction::IMMOLATE ||
                     editing_rule.action == PriorityAction::SIPHON_LIFE)
            {
              editing_rule.check_dot_refresh = true;
              editing_rule.max_dot_rem_sec = 0.0f;
            }
          }
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

  // Edit Conditions Modal Dialog
  if (open_edit_conditions_modal)
  {
    ImGui::OpenPopup("Edit Rule Conditions##AplModal");
    open_edit_conditions_modal = false;
  }

  if (ImGui::BeginPopupModal("Edit Rule Conditions##AplModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
  {
    Texture2D modal_icon = AssetManager::get().get_icon(spell_id_to_icon(editing_rule.spell_id));
    if (modal_icon.id > 0)
    {
      ImGui::Image((ImTextureID)(uintptr_t)modal_icon.id, ImVec2(24, 24));
      ImGui::SameLine(0, 8);
    }
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Configure Trigger Conditions: %s", editing_rule.name.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // 1. Procs & Buff / Debuff States
    ImGui::TextColored(ImVec4(0.85f, 0.80f, 1.0f, 1.0f), "Procs & Buff States:");
    WowCheckbox("Require Shadow Trance (Nightfall Proc) Active##AplModal", &editing_rule.check_shadow_trance);
    if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip("Only casts when Nightfall's instant cast Shadow Trance buff is active.");
    }

    WowCheckbox("Check Decimation Buff State##AplModal", &editing_rule.check_decimation);
    if (editing_rule.check_decimation)
    {
      ImGui::Indent(20.0f);
      int decim_mode = editing_rule.require_decimation_active ? 0 : 1;
      const char* decim_modes[] = {"Require Decimation Buff Active (e.g. for Soul Fire)", "Require Decimation Buff Inactive (e.g. for Trigger Cast)"};
      ImGui::SetNextItemWidth(340.0f);
      if (ImGui::Combo("##DecimModeCombo", &decim_mode, decim_modes, IM_ARRAYSIZE(decim_modes)))
      {
        editing_rule.require_decimation_active = (decim_mode == 0);
      }
      ImGui::Unindent(20.0f);
    }

    WowCheckbox("Check Demonic Brand Debuff State##AplModal", &editing_rule.check_demonic_brand);
    if (editing_rule.check_demonic_brand)
    {
      ImGui::Indent(20.0f);
      int brand_mode = editing_rule.require_demonic_brand_missing ? 0 : 1;
      const char* brand_modes[] = {"Trigger when Demonic Brand is Down / Expired", "Require Demonic Brand Active"};
      ImGui::SetNextItemWidth(340.0f);
      if (ImGui::Combo("##BrandModeCombo", &brand_mode, brand_modes, IM_ARRAYSIZE(brand_modes)))
      {
        editing_rule.require_demonic_brand_missing = (brand_mode == 0);
      }
      ImGui::Unindent(20.0f);
    }

    WowCheckbox("Check Bane of Doom Debuff State##AplModal", &editing_rule.check_doom_debuff);
    if (editing_rule.check_doom_debuff)
    {
      ImGui::Indent(20.0f);
      int doom_mode = editing_rule.require_doom_missing ? 0 : 1;
      const char* doom_modes[] = {"Trigger when Bane of Doom is NOT Active (e.g. for Bane of Agony)", "Require Bane of Doom Active"};
      ImGui::SetNextItemWidth(340.0f);
      if (ImGui::Combo("##DoomModeCombo", &doom_mode, doom_modes, IM_ARRAYSIZE(doom_modes)))
      {
        editing_rule.require_doom_missing = (doom_mode == 0);
      }
      ImGui::Unindent(20.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Mana Thresholds
    ImGui::TextColored(ImVec4(0.85f, 0.80f, 1.0f, 1.0f), "Player Resources:");
    WowCheckbox("Player Mana Thresholds##AplModal", &editing_rule.check_mana);
    if (editing_rule.check_mana)
    {
      ImGui::Indent(20.0f);
      float max_m = editing_rule.max_mana_pct * 100.0f;
      if (ImGui::SliderFloat("Trigger when Mana <= X%##AplModal", &max_m, 0.0f, 100.0f, "%.0f%%"))
      {
        editing_rule.max_mana_pct = max_m / 100.0f;
      }
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Only triggers this spell if player mana is at or below this percentage (e.g. 25%% for Life Tap).");
      }

      float min_m = editing_rule.min_mana_pct * 100.0f;
      if (ImGui::SliderFloat("Trigger only if Mana >= X%##AplModal", &min_m, 0.0f, 100.0f, "%.0f%%"))
      {
        editing_rule.min_mana_pct = min_m / 100.0f;
      }
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Only triggers this spell if player mana is at or above this percentage (prevents casting expensive spells when OOM).");
      }
      ImGui::Unindent(20.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Target Health / Execute Phase
    ImGui::TextColored(ImVec4(0.85f, 0.80f, 1.0f, 1.0f), "Target & Combat States:");
    WowCheckbox("Target Health / Execute Phase##AplModal", &editing_rule.check_target_hp);
    if (editing_rule.check_target_hp)
    {
      ImGui::Indent(20.0f);
      float max_hp = editing_rule.max_target_hp_pct * 100.0f;
      if (ImGui::SliderFloat("Trigger when Target HP <= X%##AplModal", &max_hp, 0.0f, 100.0f, "%.0f%%"))
      {
        editing_rule.max_target_hp_pct = max_hp / 100.0f;
      }
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Set to 35%% for Decimation Soul Fire / Searing Pain, or 20%% for Shadowburn execute.");
      }

      float min_hp = editing_rule.min_target_hp_pct * 100.0f;
      if (ImGui::SliderFloat("Trigger only if Target HP >= X%##AplModal", &min_hp, 0.0f, 100.0f, "%.0f%%"))
      {
        editing_rule.min_target_hp_pct = min_hp / 100.0f;
      }
      ImGui::Unindent(20.0f);
    }

    ImGui::Spacing();

    // 4. DoT Duration & Refresh Window
    WowCheckbox("DoT Duration / Refresh Window##AplModal", &editing_rule.check_dot_refresh);
    if (editing_rule.check_dot_refresh)
    {
      ImGui::Indent(20.0f);
      ImGui::SliderFloat("Refresh when DoT remaining <= X sec##AplModal", &editing_rule.max_dot_rem_sec, 0.0f, 10.0f, "%.1f sec");
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("0.0s = Only refresh when DoT has expired.\n> 0.0s = Allows Pandemic pre-refresh before DoT expires.");
      }
      ImGui::Unindent(20.0f);
    }

    ImGui::Spacing();

    // 5. Combat Duration Remaining
    WowCheckbox("Combat Duration Remaining##AplModal", &editing_rule.check_fight_time);
    if (editing_rule.check_fight_time)
    {
      ImGui::Indent(20.0f);
      ImGui::SliderFloat("Require at least X sec remaining in fight##AplModal", &editing_rule.min_time_remaining, 0.0f, 120.0f, "%.0f sec");
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Only casts if at least X seconds remain before boss death (e.g. 60s for Bane of Doom).");
      }
      ImGui::Unindent(20.0f);
    }

    ImGui::Spacing();

    // 6. ISB Debuff Remaining Duration
    WowCheckbox("Improved Shadow Bolt (ISB) Debuff Remaining##AplModal", &editing_rule.check_isb_debuff);
    if (editing_rule.check_isb_debuff)
    {
      ImGui::Indent(20.0f);
      ImGui::SliderFloat("Require ISB debuff with at least X sec remaining##AplModal", &editing_rule.min_isb_rem_sec, 0.0f, 12.0f, "%.1f sec");
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("0.0s = Any active ISB debuff charges.\n> 0.0s = Requires at least X seconds of ISB debuff duration remaining on the boss.");
      }
      ImGui::Unindent(20.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Condition Summary Preview: ");
    ImGui::SameLine();
    std::string preview_str = editing_rule.format_condition_summary();
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "%s", preview_str.c_str());

    ImGui::Spacing();
    ImGui::Separator();

    if (WowBiggerButton(" Save & Apply ", ImVec2(120, 26)))
    {
      editing_rule.condition_summary = editing_rule.format_condition_summary();
      policy.set_rule(editing_rule_index, editing_rule, talents, race);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All Conditions", ImVec2(145, 26)))
    {
      editing_rule.check_shadow_trance = false;
      editing_rule.check_decimation = false;
      editing_rule.require_decimation_active = true;
      editing_rule.check_demonic_brand = false;
      editing_rule.require_demonic_brand_missing = true;
      editing_rule.check_doom_debuff = false;
      editing_rule.require_doom_missing = true;
      editing_rule.check_mana = false;
      editing_rule.max_mana_pct = 1.0f;
      editing_rule.min_mana_pct = 0.0f;
      editing_rule.check_target_hp = false;
      editing_rule.max_target_hp_pct = 1.0f;
      editing_rule.min_target_hp_pct = 0.0f;
      editing_rule.check_dot_refresh = false;
      editing_rule.max_dot_rem_sec = 0.0f;
      editing_rule.check_fight_time = false;
      editing_rule.min_time_remaining = 0.0f;
      editing_rule.max_time_remaining = 9999.0f;
      editing_rule.check_isb_debuff = false;
      editing_rule.min_isb_rem_sec = 0.0f;
      editing_rule.require_isb_active = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(80, 26)))
    {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
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
        "Align with Bane of Doom — Pop 0-6s before Doom tick, else Execute/CD"};
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
