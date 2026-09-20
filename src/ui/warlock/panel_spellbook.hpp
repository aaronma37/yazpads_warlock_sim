#pragma once
#include "asset_manager.hpp"
#include "imgui.h"
#include "src/sim/spells.hpp"
#include "src/sim/talents.hpp"
#include "src/ui/common/panel_spellbook.hpp"
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
       "100.0% Spirit",
       "Mana Returned = (430 + 1.0 * Spirit) * (1.0 + 0.10 * Imp_Life_Tap_Points)",
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
       "spell_shadow_requiem"},
      {SpellID::NONE,
       "Blood Pact",
       "Rank 5",
       "Passive",
       "Instant",
       "0 Mana",
       "Increases party members' Stamina by 54.",
       "None",
       "—",
       "Stamina +54 (Imp Active Aura)",
       "spell_shadow_bloodboil"}};
  return entries;
}

inline void render_panel_spellbook()
{
  static char search_filter[64] = "";
  static std::vector<CommonSpellBookEntry> common_entries;
  if (common_entries.empty())
  {
    for (const auto& e : get_all_spellbook_entries())
    {
      common_entries.push_back({e.name, e.rank, e.school_str, e.base_cast_str,
                                e.base_mana_str, e.direct_dmg_str, e.dot_dmg_str,
                                e.coeff_str, e.sim_formula, e.icon_name});
    }
  }

  render_unified_spellbook_table("SpellbookTable",
                                 "Warlock Spellbook & Base Spell Data",
                                 "Search Spells (e.g. Shadow Bolt, Fire)...",
                                 search_filter, sizeof(search_filter),
                                 common_entries);
}

}  // namespace warlock
