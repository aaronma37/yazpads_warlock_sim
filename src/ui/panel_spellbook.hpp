#pragma once
#include "asset_manager.hpp"
#include "imgui.h"
#include "src/sim/spells.hpp"
#include "src/sim/talents.hpp"
#include <string>
#include <vector>

namespace warlock
{

struct SpellBookEntry
{
  SpellID id;
  std::string name;
  std::string rank;
  std::string school_str;
  std::string base_cast_str;
  std::string base_mana_str;
  std::string direct_dmg_str;
  std::string dot_dmg_str;
  std::string coeff_str;
  std::string sim_formula;
  std::string icon_name;
};

inline const std::vector<SpellBookEntry>& get_all_spellbook_entries()
{
  static const std::vector<SpellBookEntry> entries = {
      {SpellID::SHADOW_BOLT,
       "Shadow Bolt",
       "Rank 10",
       "Shadow",
       "3.0s",
       "380 Mana",
       "253 - 283 Direct Damage",
       "None",
       "85.71%",
       "Damage = Roll(253, 283) + (SpellPower + ShadowSpellPower) * 0.85714 * Multipliers",
       "spell_shadow_shadowbolt"},
      {SpellID::CORRUPTION,
       "Corruption",
       "Rank 7",
       "Shadow",
       "2.0s",
       "340 Mana",
       "None",
       "438 Base DoT over 18s (6 ticks of 73.0 every 3s)",
       "120.0%",
       "Tick Damage = (73.0 + (SpellPower + ShadowSpellPower) * 0.20) * Multipliers",
       "spell_shadow_abominationexplosion"},
      {SpellID::IMMOLATE,
       "Immolate",
       "Rank 8",
       "Fire",
       "2.0s",
       "380 Mana",
       "158 Initial Direct Damage",
       "275 Base DoT over 15s (5 ticks of 55.0 every 3s, 433 Total)",
       "85.0%",
       "Direct = 158.0 + SP * 0.20 | Tick = 55.0 + SP * 0.13",
       "spell_fire_immolation"},
      {SpellID::CURSE_OF_AGONY,
       "Bane of Agony",
       "Rank 6",
       "Shadow",
       "Instant",
       "215 Mana",
       "None",
       "552 Base DoT over 24s (12 ticks of 46.0 avg every 2s, ramping)",
       "159.6%",
       "Tick Damage = ((552.0 / 12.0) + (SpellPower + ShadowSpellPower) * (1.596 / 12.0)) * Ramp * Multipliers",
       "spell_shadow_curseofsargeras"},
      {SpellID::CURSE_OF_DOOM,
       "Curse of Doom",
       "Rank 1",
       "Shadow",
       "Instant",
       "300 Mana",
       "None",
       "1,742 Base Shadow Damage after 60s",
       "400.0%",
       "Damage = (1742.0 + (SpellPower + ShadowSpellPower) * 4.0) * Multipliers",
       "spell_shadow_auraofdarkness"},
      {SpellID::CURSE_OF_SHADOWS,
       "Curse of Shadows",
       "Rank 2",
       "Shadow",
       "Instant",
       "175 Mana",
       "None (Debuff)",
       "None",
       "—",
       "Target Modifier: +10% Shadow & Arcane damage taken, -75 Shadow/Arcane resistance",
       "spell_shadow_curseofachimonde"},
      {SpellID::CURSE_OF_ELEMENTS,
       "Curse of Elements",
       "Rank 2",
       "Shadow",
       "Instant",
       "175 Mana",
       "None (Debuff)",
       "None",
       "—",
       "Target Modifier: +10% Fire & Frost damage taken, -75 Fire/Frost resistance",
       "spell_shadow_chilltouch"},
      {SpellID::SHADOWBURN,
       "Shadowburn",
       "Rank 6",
       "Shadow",
       "Instant",
       "365 Mana",
       "238 - 266 Direct Damage",
       "None",
       "42.86%",
       "Damage = Roll(238, 266) + (SpellPower + ShadowSpellPower) * 0.42857 * Multipliers",
       "spell_shadow_scourgebuild"},
      {SpellID::CONFLAGRATE,
       "Conflagrate",
       "Rank 4",
       "Fire",
       "Instant",
       "265 Mana",
       "306 - 374 Direct Damage",
       "Consumes active Immolate on target",
       "42.86%",
       "Damage = Roll(306, 374) + (SpellPower + FireSpellPower) * 0.42857 * Multipliers",
       "spell_fire_fireball"},
      {SpellID::INCINERATE,
       "Incinerate",
       "Rank 3",
       "Fire",
       "2.5s",
       "325 Mana",
       "201 - 233 Direct Damage (+25% bonus vs Immolated target)",
       "None",
       "71.43%",
       "Damage = (Roll(201, 233) + SP * 0.71429) * (Immolate_Active ? 1.25 : 1.0) * Multipliers",
       "spell_fire_burnout"},
      {SpellID::SOUL_FIRE,
       "Soul Fire",
       "Rank 5",
       "Fire",
       "6.0s",
       "335 Mana",
       "383 - 479 Direct Damage",
       "None",
       "100.0%",
       "Damage = Roll(383, 479) + (SpellPower + FireSpellPower) * 1.0 * Multipliers",
       "spell_fire_fireball02"},
      {SpellID::DRAIN_HOPE,
       "Wrack",
       "Rank 3",
       "Shadow",
       "Channeled 6.0s",
       "240 Mana",
       "None",
       "212 Total Shadow Damage (6 ticks of 35.33 every 1s)",
       "100.0%",
       "Tick Damage = (35.33 + (SpellPower + ShadowSpellPower) * 0.1667) * Multipliers",
       "spell_shadow_lifedrain02"},
      {SpellID::DRAIN_SOUL,
       "Drain Soul",
       "Rank 4",
       "Shadow",
       "Channeled 15.0s",
       "290 Mana",
       "None",
       "420 Total Shadow Damage (5 ticks of 84.0 every 3s)",
       "100.0%",
       "Tick Damage = (84.0 + (SpellPower + ShadowSpellPower) * 0.20) * Multipliers",
       "spell_shadow_soulgem"},
      {SpellID::SEARING_PAIN,
       "Searing Pain",
       "Rank 6",
       "Fire",
       "1.5s",
       "168 Mana",
       "108 - 127 Direct Damage",
       "None",
       "42.86%",
       "Damage = Roll(108, 127) + (SpellPower + FireSpellPower) * 0.42857 * Multipliers",
       "spell_fire_soulburn"},
      {SpellID::LIFE_TAP,
       "Life Tap",
       "—",
       "Shadow",
       "Instant",
       "0 Mana (Cost: 430 HP)",
       "430 Base Mana Restored",
       "None",
       "5.0% Spirit",
       "Mana Returned = (430 + 0.05 * Spirit) * (1.0 + 0.10 * Imp_Life_Tap_Points)",
       "spell_shadow_burningspirit"},
      {SpellID::SIPHON_LIFE,
       "Siphon Life",
       "Rank 4",
       "Shadow",
       "Instant",
       "365 Mana",
       "None",
       "410 Base DoT over 30s (10 ticks of 41.0 every 3s)",
       "50.0%",
       "Tick Damage = (41.0 + (SpellPower + ShadowSpellPower) * 0.05) * Multipliers",
       "spell_shadow_requiem"}};
  return entries;
}

inline void render_panel_spellbook()
{
  ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Warlock Spellbook & Base Spell Data");

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  static char search_filter[64] = "";
  ImGui::SetNextItemWidth(300);
  ImGui::InputTextWithHint(
      "##SpellSearch", "Search Spells (e.g. Shadow Bolt, Fire)...", search_filter, sizeof(search_filter));
  ImGui::SameLine();
  if (ImGui::Button("Clear"))
  {
    search_filter[0] = '\0';
  }

  ImGui::Spacing();

  const auto& entries = get_all_spellbook_entries();

  ImGuiTableFlags flags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable("SpellbookTable", 8, flags, ImVec2(0, 0)))
  {
    ImGui::TableSetupColumn("Spell", ImGuiTableColumnFlags_WidthFixed, 180.0f);
    ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 70.0f);
    ImGui::TableSetupColumn("School", ImGuiTableColumnFlags_WidthFixed, 75.0f);
    ImGui::TableSetupColumn("Cast Time", ImGuiTableColumnFlags_WidthFixed, 105.0f);
    ImGui::TableSetupColumn("Mana Cost", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Base Damage / Effect", ImGuiTableColumnFlags_WidthFixed, 260.0f);
    ImGui::TableSetupColumn("SP Coefficient", ImGuiTableColumnFlags_WidthFixed, 115.0f);
    ImGui::TableSetupColumn("Formula", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    std::string query = search_filter;
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    for (const auto& sp : entries)
    {
      std::string name_lower = sp.name;
      std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
      std::string school_lower = sp.school_str;
      std::transform(school_lower.begin(), school_lower.end(), school_lower.begin(), ::tolower);

      if (!query.empty() && name_lower.find(query) == std::string::npos &&
          school_lower.find(query) == std::string::npos)
      {
        continue;
      }

      ImGui::TableNextRow();

      // Col 0: Icon + Name
      ImGui::TableSetColumnIndex(0);
      Texture2D icon = AssetManager::get().get_icon(sp.icon_name);
      ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(20, 20));
      ImGui::SameLine(0, 6);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.70f, 1.0f), "%s", sp.name.c_str());

      // Col 1: Rank
      ImGui::TableSetColumnIndex(1);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.80f, 1.0f), "%s", sp.rank.c_str());

      // Col 2: School
      ImGui::TableSetColumnIndex(2);
      ImGui::AlignTextToFramePadding();
      if (sp.school_str == "Shadow")
      {
        ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "Shadow");
      }
      else if (sp.school_str == "Fire")
      {
        ImGui::TextColored(ImVec4(1.0f, 0.50f, 0.20f, 1.0f), "Fire");
      }
      else
      {
        ImGui::TextColored(ImVec4(0.80f, 0.80f, 0.80f, 1.0f), "%s", sp.school_str.c_str());
      }

      // Col 3: Cast Time
      ImGui::TableSetColumnIndex(3);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("%s", sp.base_cast_str.c_str());

      // Col 4: Mana Cost
      ImGui::TableSetColumnIndex(4);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.0f, 1.0f), "%s", sp.base_mana_str.c_str());

      // Col 5: Base Damage
      ImGui::TableSetColumnIndex(5);
      ImGui::AlignTextToFramePadding();
      if (sp.direct_dmg_str != "None")
      {
        ImGui::Text("%s", sp.direct_dmg_str.c_str());
      }
      if (sp.dot_dmg_str != "None")
      {
        ImGui::TextColored(ImVec4(0.50f, 0.90f, 0.50f, 1.0f), "%s", sp.dot_dmg_str.c_str());
      }

      // Col 6: SP Coefficient
      ImGui::TableSetColumnIndex(6);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.40f, 1.0f), "%s", sp.coeff_str.c_str());

      // Col 7: Formula
      ImGui::TableSetColumnIndex(7);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "%s", sp.sim_formula.c_str());
    }

    ImGui::EndTable();
  }
}

}  // namespace warlock
