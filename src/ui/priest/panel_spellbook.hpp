#pragma once
#include "imgui.h"
#include "src/ui/asset_manager.hpp"
#include "src/ui/common/panel_spellbook.hpp"
#include <string>
#include <vector>

namespace priest
{

inline const std::vector<warlock::CommonSpellBookEntry>& get_all_priest_spellbook_entries()
{
  static const std::vector<warlock::CommonSpellBookEntry> entries = {
      {"Power Word: Fortitude", "Rank 6", "Discipline", "Instant", "---", "1,695 Mana",
       "---", "---", "spell_holy_wordfortitude"},
      {"Power Word: Shield", "Rank 10", "Discipline", "Instant", "4s", "500 Mana",
       "928", "10.0%", "spell_holy_powerwordshield"},
      {"Confounding Flash", "---", "Discipline", "0.5s", "2m", "3% base",
       "---", "---", "ability_paladin_blindinglight2"},
      {"Starshards", "Rank 7", "Arcane", "Channeled", "30s", "350 Mana",
       "300 per tick every 1s", "100.0%", "spell_arcane_starfire"},
      {"Inner Fire", "Rank 6", "Discipline", "Instant", "---", "315 Mana",
       "---", "---", "spell_holy_innerfire"},
      {"Dispel Magic", "Rank 2", "Discipline", "Instant", "---", "18% base",
       "---", "---", "spell_holy_dispelmagic"},
      {"Contingency Plan", "Rank 5", "Discipline", "Instant", "10m", "0 Mana",
       "---", "---", "ability_priest_soulwarding"},
      {"Elune's Grace", "---", "Discipline", "Instant", "5m", "3% base",
       "---", "---", "spell_holy_elunesgrace"},
      {"Feedback", "Rank 5", "Discipline", "Instant", "3m", "230 Mana",
       "105", "---", "spell_shadow_ritualofsacrifice"},
      {"Shackle Undead", "Rank 3", "Discipline", "1.5s", "---", "150 Mana",
       "---", "---", "spell_nature_slow"},
      {"Mana Burn", "Rank 5", "Discipline", "3.0s", "---", "270 Mana",
       "738 - 780", "---", "spell_shadow_manaburn"},
      {"Divine Spirit", "Rank 4", "Discipline", "Instant", "---", "970 Mana",
       "---", "---", "spell_holy_divinespirit"},
      {"Penance", "Rank 4", "Discipline", "Channeled", "12s", "355 Mana",
       "131 per tick every 1s", "75.0%", "spell_holy_penance"},
      {"Levitate", "---", "Discipline", "Instant", "---", "100 Mana",
       "---", "---", "spell_holy_layonhands"},
      {"Prayer of Fortitude", "Rank 2", "Discipline", "Instant", "---", "3,400 Mana",
       "---", "---", "spell_holy_prayeroffortitude"},
      {"Prayer of Spirit", "---", "Discipline", "Instant", "---", "1,940 Mana",
       "---", "---", "spell_holy_prayerofspirit"},
      {"Lesser Heal", "Rank 3", "Holy", "2.5s", "---", "75 Mana",
       "130 - 152", "71.43%", "spell_holy_lesserheal"},
      {"Smite", "Rank 8", "Holy", "2.5s", "---", "280 Mana",
       "160 - 180", "71.43%", "spell_holy_holysmite"},
      {"Renew", "Rank 10", "Holy", "Instant", "---", "410 Mana",
       "166 per tick every 3s", "100.0%", "spell_holy_renew"},
      {"Desperate Prayer", "Rank 7", "Holy", "Instant", "10m", "0 Mana",
       "1,269 - 1,497", "42.86%", "spell_holy_restoration"},
      {"Divine Grace", "Rank 7", "Holy", "Instant", "10m", "0 Mana",
       "1,269 - 1,497", "42.86%", "ability_priest_savinggrace"},
      {"Resurrection", "Rank 5", "Holy", "10.0s", "---", "75% base",
       "---", "---", "spell_holy_resurrection"},
      {"Cure Disease", "---", "Holy", "Instant", "---", "15% base",
       "---", "---", "spell_holy_nullifydisease"},
      {"Heal", "Rank 4", "Holy", "3.0s", "---", "305 Mana",
       "611 - 691", "85.71%", "spell_holy_heal02"},
      {"Chastise", "Rank 5", "Holy", "Instant", "2m", "225 Mana",
       "272 - 306", "42.86%", "spell_holy_chastise"},
      {"Fear Ward", "---", "Holy", "Instant", "3m", "100 Mana",
       "---", "---", "spell_holy_excorcism"},
      {"Flash Heal", "Rank 7", "Holy", "1.5s", "---", "380 Mana",
       "757 - 893", "85.71%", "spell_holy_flashheal"},
      {"Holy Fire", "Rank 8", "Holy", "3.5s", "---", "255 Mana",
       "184 - 232 init, 15 per tick every 2s", "100.0%", "spell_holy_searinglight"},
      {"Holy Nova", "Rank 6", "Holy", "Instant", "---", "750 Mana",
       "174 - 200", "16.1% / 30.3%", "spell_holy_holynova"},
      {"Binding Heal", "Rank 6", "Holy", "1.5s", "---", "380 Mana",
       "757 - 893", "85.71%", "spell_holy_blindingheal"},
      {"Prayer of Healing", "Rank 5", "Holy", "3.0s", "---", "1,070 Mana",
       "631 - 667", "300.0%", "spell_holy_prayerofhealing02"},
      {"Abolish Disease", "---", "Holy", "Instant", "---", "15% base",
       "---", "---", "spell_nature_nullifydisease"},
      {"Greater Heal", "Rank 5", "Holy", "3.0s", "---", "710 Mana",
       "1,853 - 2,067", "85.71%", "spell_holy_greaterheal"},
      {"Lightwell", "Rank 3", "Holy", "1.5s", "10m", "365 Mana",
       "---", "---", "spell_holy_summonlightwell"},
      {"Prayer of Mending", "Rank 3", "Holy", "Instant", "10s", "390 Mana",
       "413", "42.86%", "spell_holy_prayerofmendingtga"},
      {"Shadow Word: Pain", "Rank 8", "Shadow", "Instant", "---", "470 Mana",
       "127 per tick every 3s", "120.0%", "spell_shadow_shadowwordpain"},
      {"Fade", "Rank 6", "Shadow", "Instant", "30s", "275 Mana",
       "---", "---", "spell_magic_lesserinvisibilty"},
      {"Hex of Weakness", "Rank 6", "Shadow", "Instant", "---", "240 Mana",
       "---", "---", "spell_shadow_fingerofdeath"},
      {"Mind Blast", "Rank 9", "Shadow", "1.5s", "8s", "350 Mana",
       "472 - 498", "42.86%", "spell_shadow_unholyfrenzy"},
      {"Touch of Weakness", "Rank 6", "Shadow", "Instant", "---", "195 Mana",
       "56", "10.0%", "spell_shadow_deadofnight"},
      {"Psychic Scream", "Rank 4", "Shadow", "Instant", "30s", "210 Mana",
       "---", "---", "spell_shadow_psychicscream"},
      {"Dark Sacrifice", "Rank 5", "Shadow", "Instant", "10m", "0 Mana",
       "---", "---", "spell_holy_powerinfusion_shadow"},
      {"Devouring Plague", "Rank 6", "Shadow", "Instant", "1m", "985 Mana",
       "106 per tick every 3s", "80.0%", "spell_shadow_devouringplague"},
      {"Mind Flay", "Rank 6", "Shadow", "Channeled 3.0s", "---", "205 Mana",
       "130 per tick every 1s", "50.0%", "spell_shadow_siphonmana"},
      {"Mind Soothe", "Rank 3", "Shadow", "Instant", "---", "90 Mana",
       "---", "---", "spell_holy_mindsooth"},
      {"Shadowguard", "Rank 6", "Shadow", "Instant", "---", "250 Mana",
       "96", "80.0%", "spell_nature_lightningshield"},
      {"Mind Vision", "Rank 2", "Shadow", "Channeled 1.0m", "---", "150 Mana",
       "---", "---", "spell_holy_mindvision"},
      {"Mind Control", "Rank 3", "Shadow", "Channeled 1.0m", "---", "750 Mana",
       "---", "---", "spell_shadow_shadowworddominate"},
      {"Shadow Protection", "Rank 3", "Shadow", "Instant", "---", "650 Mana",
       "---", "---", "spell_shadow_antishadow"},
      {"Shadow Word: Death", "Rank 4", "Shadow", "Instant", "15s", "340 Mana",
       "434 - 462", "42.86%", "spell_shadow_demonicfortitude"},
      {"Prayer of Shadow Protection", "---", "Shadow", "Instant", "---", "1,300 Mana",
       "---", "---", "spell_holy_prayerofshadowprotection"},
  };
  return entries;
}

inline void render_priest_spellbook_panel()
{
  static char search_filter[64] = "";
  const auto& entries = get_all_priest_spellbook_entries();

  warlock::render_unified_spellbook_table("PriestSpellbookTable",
                                          "Search Spells (e.g. Shadow Word, Holy)...",
                                          search_filter, sizeof(search_filter),
                                          entries);
}

}  // namespace priest
