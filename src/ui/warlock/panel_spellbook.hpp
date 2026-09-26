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
  std::string base_cd_str;
  std::string base_mana_str;
  std::string damage_effect_str;
  std::string coeff_str;
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
       "---",
       "380 Mana",
       "253 - 283",
       "85.71%",
       "spell_shadow_shadowbolt"},
      {SpellID::CORRUPTION,
       "Corruption",
       "Rank 7",
       "Shadow",
       "2.0s",
       "---",
       "340 Mana",
       "73 per tick every 3s",
       "120.0%",
       "spell_shadow_abominationexplosion"},
      {SpellID::IMMOLATE,
       "Immolate",
       "Rank 8",
       "Fire",
       "2.0s",
       "---",
       "380 Mana",
       "158 init, 55 per tick every 3s",
       "85.0%",
       "spell_fire_immolation"},
      {SpellID::CURSE_OF_AGONY,
       "Bane of Agony",
       "Rank 6",
       "Shadow",
       "Instant",
       "---",
       "215 Mana",
       "46 per tick every 2s",
       "159.6%",
       "spell_shadow_curseofsargeras"},
      {SpellID::CURSE_OF_DOOM,
       "Bane of Doom",
       "Rank 1",
       "Shadow",
       "Instant",
       "1m",
       "300 Mana",
       "1742",
       "400.0%",
       "spell_shadow_auraofdarkness"},
      {SpellID::CURSE_OF_SHADOWS,
       "Curse of Shadows",
       "Rank 2",
       "Shadow",
       "Instant",
       "---",
       "175 Mana",
       "---",
       "---",
       "spell_shadow_curseofachimonde"},
      {SpellID::CURSE_OF_ELEMENTS,
       "Curse of Elements",
       "Rank 2",
       "Shadow",
       "Instant",
       "---",
       "175 Mana",
       "---",
       "---",
       "spell_shadow_chilltouch"},
      {SpellID::SHADOWBURN,
       "Shadowburn",
       "Rank 6",
       "Shadow",
       "Instant",
       "15s",
       "365 Mana",
       "259 - 289",
       "42.86%",
       "spell_shadow_scourgebuild"},
      {SpellID::CONFLAGRATE,
       "Conflagrate",
       "Rank 4",
       "Fire",
       "Instant",
       "10s",
       "265 Mana",
       "306 - 374",
       "42.86%",
       "spell_fire_fireball"},
      {SpellID::INCINERATE,
       "Incinerate",
       "Rank 3",
       "Fire",
       "2.5s",
       "---",
       "325 Mana",
       "201 - 233",
       "71.43%",
       "spell_fire_burnout"},
      {SpellID::SOUL_FIRE,
       "Soul Fire",
       "Rank 5",
       "Fire",
       "6.0s",
       "1m",
       "335 Mana",
       "383 - 479",
       "100.0%",
       "spell_fire_fireball02"},
      {SpellID::DRAIN_HOPE,
       "Wrack",
       "Rank 3",
       "Shadow",
       "Channeled 6.0s",
       "---",
       "240 Mana",
       "36 per tick every 1s",
       "85.8%",
       "ability_deathknight_hemorrhagicfever"},
      {SpellID::DRAIN_SOUL,
       "Drain Soul",
       "Rank 4",
       "Shadow",
       "Channeled 15.0s",
       "---",
       "290 Mana",
       "84 per tick every 3s",
       "50.0%",
       "spell_shadow_soulgem"},
      {SpellID::SEARING_PAIN,
       "Searing Pain",
       "Rank 6",
       "Fire",
       "1.5s",
       "---",
       "168 Mana",
       "108 - 127",
       "42.86%",
       "spell_fire_soulburn"},
      {SpellID::LIFE_TAP,
       "Life Tap",
       "---",
       "Shadow",
       "Instant",
       "---",
       "0 Mana (Cost: 430 HP)",
       "430",
       "100.0% Spirit",
       "spell_shadow_burningspirit"},
      {SpellID::SIPHON_LIFE,
       "Siphon Life",
       "Rank 4",
       "Shadow",
       "Instant",
       "---",
       "365 Mana",
       "41 per tick every 3s",
       "50.0%",
       "spell_shadow_requiem"},
      {SpellID::NONE,
       "Blood Pact",
       "Rank 5",
       "Passive",
       "Instant",
       "---",
       "0 Mana",
       "---",
       "---",
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
                                e.base_cd_str, e.base_mana_str, e.damage_effect_str,
                                e.coeff_str, e.icon_name});
    }
  }

  render_unified_spellbook_table("SpellbookTable",
                                 "Search Spells (e.g. Shadow Bolt, Fire)...",
                                 search_filter, sizeof(search_filter),
                                 common_entries);
}

}  // namespace warlock
