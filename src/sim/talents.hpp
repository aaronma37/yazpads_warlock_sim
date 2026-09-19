#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

namespace warlock {

struct TalentNodeDef {
    const char* id;
    const char* name;
    int row;
    int col;
    int max_points;
    const char* icon;
    const char* req;
    const char* desc[5];
};

struct AfflictionTalents {
    int improved_life_tap = 0; // max: 2, R1C1
    int suppression = 0; // max: 5, R1C2
    int improved_corruption = 0; // max: 5, R1C3
    int malediction = 0; // max: 5, R2C1
    int soul_harvesting = 0; // max: 2, R2C2
    int improved_drains = 0; // max: 3, R2C3
    int improved_bane_of_agony = 0; // max: 2, R3C1
    int fel_concentration = 0; // max: 3, R3C2
    int amplify_curse = 0; // max: 1, R3C3
    int pandemic = 0; // max: 3, R3C4
    int malevolence = 0; // max: 5, R4C1
    int nightfall = 0; // max: 2, R4C2
    int curse_of_exhaustion = 0; // max: 1, R4C3
    int siphon_life = 0; // max: 1, R5C2
    int soul_siphon = 0; // max: 3, R5C3
    int shadow_mastery = 0; // max: 5, R6C3
    int drain_hope = 0; // max: 1, R7C2

    int total_points() const {
        return improved_life_tap + 
               suppression + 
               improved_corruption + 
               malediction + 
               soul_harvesting + 
               improved_drains + 
               improved_bane_of_agony + 
               fel_concentration + 
               amplify_curse + 
               pandemic + 
               malevolence + 
               nightfall + 
               curse_of_exhaustion + 
               siphon_life + 
               soul_siphon + 
               shadow_mastery + 
               drain_hope;
    }

    int& get_points_by_index(size_t idx) {
        switch (idx) {
            case 0: return improved_life_tap;
            case 1: return suppression;
            case 2: return improved_corruption;
            case 3: return malediction;
            case 4: return soul_harvesting;
            case 5: return improved_drains;
            case 6: return improved_bane_of_agony;
            case 7: return fel_concentration;
            case 8: return amplify_curse;
            case 9: return pandemic;
            case 10: return malevolence;
            case 11: return nightfall;
            case 12: return curse_of_exhaustion;
            case 13: return siphon_life;
            case 14: return soul_siphon;
            case 15: return shadow_mastery;
            case 16: return drain_hope;
            default: return improved_life_tap;
        }
    }

    int get_points_by_index(size_t idx) const {
        switch (idx) {
            case 0: return improved_life_tap;
            case 1: return suppression;
            case 2: return improved_corruption;
            case 3: return malediction;
            case 4: return soul_harvesting;
            case 5: return improved_drains;
            case 6: return improved_bane_of_agony;
            case 7: return fel_concentration;
            case 8: return amplify_curse;
            case 9: return pandemic;
            case 10: return malevolence;
            case 11: return nightfall;
            case 12: return curse_of_exhaustion;
            case 13: return siphon_life;
            case 14: return soul_siphon;
            case 15: return shadow_mastery;
            case 16: return drain_hope;
            default: return 0;
        }
    }
};

struct DemonologyTalents {
    int improved_health_funnel = 0; // max: 2, R1C1
    int improved_imp = 0; // max: 3, R1C2
    int demonic_embrace = 0; // max: 5, R1C3
    int unholy_power = 0; // max: 5, R1C4
    int demonic_aegis = 0; // max: 2, R2C1
    int improved_voidwalker = 0; // max: 3, R2C2
    int fel_vitality = 0; // max: 3, R2C3
    int demonic_energies = 0; // max: 2, R2C4
    int improved_sayaad = 0; // max: 3, R3C1
    int demonic_sacrifice = 0; // max: 1, R3C2
    int master_summoner = 0; // max: 2, R3C3
    int decimation = 0; // max: 2, R4C1
    int fel_domination = 0; // max: 1, R4C3
    int demonic_brand = 0; // max: 3, R4C4
    int improved_felhunter = 0; // max: 3, R5C1
    int soul_link = 0; // max: 1, R5C2
    int demonic_knowledge = 0; // max: 3, R5C3
    int master_demonologist = 0; // max: 5, R6C3
    int demonic_pact = 0; // max: 1, R7C2

    int total_points() const {
        return improved_health_funnel + 
               improved_imp + 
               demonic_embrace + 
               unholy_power + 
               demonic_aegis + 
               improved_voidwalker + 
               fel_vitality + 
               demonic_energies + 
               improved_sayaad + 
               demonic_sacrifice + 
               master_summoner + 
               decimation + 
               fel_domination + 
               demonic_brand + 
               improved_felhunter + 
               soul_link + 
               demonic_knowledge + 
               master_demonologist + 
               demonic_pact;
    }

    int& get_points_by_index(size_t idx) {
        switch (idx) {
            case 0: return improved_health_funnel;
            case 1: return improved_imp;
            case 2: return demonic_embrace;
            case 3: return unholy_power;
            case 4: return demonic_aegis;
            case 5: return improved_voidwalker;
            case 6: return fel_vitality;
            case 7: return demonic_energies;
            case 8: return improved_sayaad;
            case 9: return demonic_sacrifice;
            case 10: return master_summoner;
            case 11: return decimation;
            case 12: return fel_domination;
            case 13: return demonic_brand;
            case 14: return improved_felhunter;
            case 15: return soul_link;
            case 16: return demonic_knowledge;
            case 17: return master_demonologist;
            case 18: return demonic_pact;
            default: return improved_health_funnel;
        }
    }

    int get_points_by_index(size_t idx) const {
        switch (idx) {
            case 0: return improved_health_funnel;
            case 1: return improved_imp;
            case 2: return demonic_embrace;
            case 3: return unholy_power;
            case 4: return demonic_aegis;
            case 5: return improved_voidwalker;
            case 6: return fel_vitality;
            case 7: return demonic_energies;
            case 8: return improved_sayaad;
            case 9: return demonic_sacrifice;
            case 10: return master_summoner;
            case 11: return decimation;
            case 12: return fel_domination;
            case 13: return demonic_brand;
            case 14: return improved_felhunter;
            case 15: return soul_link;
            case 16: return demonic_knowledge;
            case 17: return master_demonologist;
            case 18: return demonic_pact;
            default: return 0;
        }
    }
};

struct DestructionTalents {
    int destructive_reach = 0; // max: 2, R1C1
    int improved_shadow_bolt = 0; // max: 5, R1C2
    int bane = 0; // max: 5, R1C3
    int molten_skin = 0; // max: 5, R2C1
    int cataclysm = 0; // max: 3, R2C2
    int aftermath = 0; // max: 5, R2C3
    int ruin = 0; // max: 5, R3C2
    int shadowburn = 0; // max: 1, R3C3
    int intensity = 0; // max: 3, R4C1
    int agonizing_flames = 0; // max: 3, R4C2
    int conflagrate = 0; // max: 1, R4C3
    int pyroclasm = 0; // max: 2, R5C1
    int bane_of_havoc = 0; // max: 1, R5C2
    int fire_and_brimstone = 0; // max: 3, R5C3
    int shadow_and_flame = 0; // max: 5, R6C3
    int incinerate = 0; // max: 1, R7C2

    int total_points() const {
        return destructive_reach + 
               improved_shadow_bolt + 
               bane + 
               molten_skin + 
               cataclysm + 
               aftermath + 
               ruin + 
               shadowburn + 
               intensity + 
               agonizing_flames + 
               conflagrate + 
               pyroclasm + 
               bane_of_havoc + 
               fire_and_brimstone + 
               shadow_and_flame + 
               incinerate;
    }

    int& get_points_by_index(size_t idx) {
        switch (idx) {
            case 0: return destructive_reach;
            case 1: return improved_shadow_bolt;
            case 2: return bane;
            case 3: return molten_skin;
            case 4: return cataclysm;
            case 5: return aftermath;
            case 6: return ruin;
            case 7: return shadowburn;
            case 8: return intensity;
            case 9: return agonizing_flames;
            case 10: return conflagrate;
            case 11: return pyroclasm;
            case 12: return bane_of_havoc;
            case 13: return fire_and_brimstone;
            case 14: return shadow_and_flame;
            case 15: return incinerate;
            default: return destructive_reach;
        }
    }

    int get_points_by_index(size_t idx) const {
        switch (idx) {
            case 0: return destructive_reach;
            case 1: return improved_shadow_bolt;
            case 2: return bane;
            case 3: return molten_skin;
            case 4: return cataclysm;
            case 5: return aftermath;
            case 6: return ruin;
            case 7: return shadowburn;
            case 8: return intensity;
            case 9: return agonizing_flames;
            case 10: return conflagrate;
            case 11: return pyroclasm;
            case 12: return bane_of_havoc;
            case 13: return fire_and_brimstone;
            case 14: return shadow_and_flame;
            case 15: return incinerate;
            default: return 0;
        }
    }
};

inline const std::array<TalentNodeDef, 17> FOREVER_AFFLICTION_NODES = {{
    {"improved_life_tap", "Improved Life Tap", 1, 1, 2, "spell_shadow_burningspirit.png", nullptr, {"Increases the amount of Mana awarded by your Life Tap spell by 10%.", "Increases the amount of Mana awarded by your Life Tap spell by 20%.", nullptr, nullptr, nullptr}},
    {"suppression", "Suppression", 1, 2, 5, "spell_shadow_unsummonbuilding.png", nullptr, {"Improves your chance to hit by 1% and reduces all threat you generate by 4%.", "Improves your chance to hit by 2% and reduces all threat you generate by 8%.", "Improves your chance to hit by 3% and reduces all threat you generate by 12%.", "Improves your chance to hit by 4% and reduces all threat you generate by 16%.", "Improves your chance to hit by 5% and reduces all threat you generate by 20%."}},
    {"improved_corruption", "Improved Corruption", 1, 3, 5, "spell_shadow_abominationexplosion.png", nullptr, {"Reduces the casting time of your Corruption spell by 0.4 sec and increases the damage it deals by 2%.", "Reduces the casting time of your Corruption spell by 0.8 sec and increases the damage it deals by 4%.", "Reduces the casting time of your Corruption spell by 1.2 sec and increases the damage it deals by 6%.", "Reduces the casting time of your Corruption spell by 1.6 sec and increases the damage it deals by 8%.", "Reduces the casting time of your Corruption spell by 2 sec and increases the damage it deals by 10%."}},
    {"malediction", "Malediction", 2, 1, 5, "spell_shadow_curseofachimonde.png", nullptr, {"Increases all periodic damage done by your Warlock spells by 1%.", "Increases all periodic damage done by your Warlock spells by 2%.", "Increases all periodic damage done by your Warlock spells by 3%.", "Increases all periodic damage done by your Warlock spells by 4%.", "Increases all periodic damage done by your Warlock spells by 5%."}},
    {"soul_harvesting", "Soul Harvesting", 2, 2, 2, "inv_elemental_primal_shadow.png", nullptr, {"You gain Soul Harvest for 10 sec if a victim is killed while afflicted with your Drain Soul. Soul Harvest allows your Mana to regenerate at 50% of normal speed while you are casting spells, and grants a 50% increase to your Mana regeneration.", "You gain Soul Harvest for 10 sec if a victim is killed while afflicted with your Drain Soul. Soul Harvest allows your Mana to regenerate at 100% of normal speed while you are casting spells, and grants a 100% increase to your Mana regeneration.", nullptr, nullptr, nullptr}},
    {"improved_drains", "Improved Drains", 2, 3, 3, "spell_shadow_haunting.png", nullptr, {"Increases health drained or damage done by your Drain Life, Drain Soul, and Wrack spells by 7%.", "Increases health drained or damage done by your Drain Life, Drain Soul, and Wrack spells by 13%.", "Increases health drained or damage done by your Drain Life, Drain Soul, and Wrack spells by 20%.", nullptr, nullptr}},
    {"improved_bane_of_agony", "Improved Bane of Agony", 3, 1, 2, "spell_shadow_curseofsargeras.png", nullptr, {"Increases the damage done by your Bane of Agony by 5%.", "Increases the damage done by your Bane of Agony by 10%.", nullptr, nullptr, nullptr}},
    {"fel_concentration", "Fel Concentration", 3, 2, 3, "spell_shadow_fingerofdeath.png", nullptr, {"Gives you a 23% chance to avoid interruption caused by damage while channeling or casting your Drain Life, Drain Mana, Drain Soul, or Wrack spells.", "Gives you a 47% chance to avoid interruption caused by damage while channeling or casting your Drain Life, Drain Mana, Drain Soul, or Wrack spells.", "Gives you a 70% chance to avoid interruption caused by damage while channeling or casting your Drain Life, Drain Mana, Drain Soul, or Wrack spells.", nullptr, nullptr}},
    {"amplify_curse", "Amplify Curse", 3, 3, 1, "spell_shadow_contagion.png", nullptr, {"Increases the effect of your next Curse of Weakness or Bane of Agony by 50%, or your next Curse of Exhaustion by 20%. Lasts 30 sec.", nullptr, nullptr, nullptr, nullptr}},
    {"pandemic", "Pandemic", 3, 4, 3, "spell_shadow_unstableaffliction_2.png", nullptr, {"Increases the critical strike damage bonus of your Corruption, Bane of Agony, Bane of Doom, Drain Soul, Drain Life, Siphon Life, and Wrack spells by 33%.", "Increases the critical strike damage bonus of your Corruption, Bane of Agony, Bane of Doom, Drain Soul, Drain Life, Siphon Life, and Wrack spells by 67%.", "Increases the critical strike damage bonus of your Corruption, Bane of Agony, Bane of Doom, Drain Soul, Drain Life, Siphon Life, and Wrack spells by 100%.", nullptr, nullptr}},
    {"malevolence", "Malevolence", 4, 1, 5, "spell_shadow_focusedpower.png", nullptr, {"Increases the critical effect chance of your Shadow spells by 1%.", "Increases the critical effect chance of your Shadow spells by 2%.", "Increases the critical effect chance of your Shadow spells by 3%.", "Increases the critical effect chance of your Shadow spells by 4%.", "Increases the critical effect chance of your Shadow spells by 5%."}},
    {"nightfall", "Nightfall", 4, 2, 2, "spell_shadow_twilight.png", nullptr, {"Gives your Corruption, Drain Soul, Drain Life, and Wrack spells a 2% chance to cause you to enter a Shadow Trance after damaging the opponent. The Shadow Trance reduces the casting time of your next Shadow Bolt spell by 100%.", "Gives your Corruption, Drain Soul, Drain Life, and Wrack spells a 4% chance to cause you to enter a Shadow Trance after damaging the opponent. The Shadow Trance reduces the casting time of your next Shadow Bolt spell by 100%.", nullptr, nullptr, nullptr}},
    {"curse_of_exhaustion", "Curse of Exhaustion", 4, 3, 1, "spell_shadow_grimward.png", "Amplify Curse", {"Reduces the target's movement speed by 30% for 12 sec. Only one Curse per Warlock can be active on any one target.", nullptr, nullptr, nullptr, nullptr}},
    {"siphon_life", "Siphon Life", 5, 2, 1, "spell_shadow_requiem.png", nullptr, {"Transfers 41 health from the target to the caster every 3 sec. Lasts 30 sec.", nullptr, nullptr, nullptr, nullptr}},
    {"soul_siphon", "Soul Siphon", 5, 3, 3, "spell_shadow_lifedrain02.png", nullptr, {"Increases the damage done or health drained by your Drain Life, Drain Soul, and Wrack spells by 4% per each of your other Affliction effects active on the target, up to a maximum increase of 12%.", "Increases the damage done or health drained by your Drain Life, Drain Soul, and Wrack spells by 8% per each of your other Affliction effects active on the target, up to a maximum increase of 24%.", "Increases the damage done or health drained by your Drain Life, Drain Soul, and Wrack spells by 12% per each of your other Affliction effects active on the target, up to a maximum increase of 36%.", nullptr, nullptr}},
    {"shadow_mastery", "Shadow Mastery", 6, 3, 5, "spell_shadow_shadetruesight.png", nullptr, {"Increases the damage dealt or life drained by your Shadow spells by 1%.", "Increases the damage dealt or life drained by your Shadow spells by 2%.", "Increases the damage dealt or life drained by your Shadow spells by 3%.", "Increases the damage dealt or life drained by your Shadow spells by 4%.", "Increases the damage dealt or life drained by your Shadow spells by 5%."}},
    {"wrack", "Wrack", 7, 2, 1, "ability_deathknight_hemorrhagicfever.png", "Siphon Life", {"Tears the target apart from within, dealing 36 Shadow damage every 1 sec and increasing the damage they take from your other Shadow damage over time effects by 10%. Lasts 6 sec.", nullptr, nullptr, nullptr, nullptr}},
}};

inline const std::array<TalentNodeDef, 19> FOREVER_DEMONOLOGY_NODES = {{
    {"improved_health_funnel", "Improved Health Funnel", 1, 1, 2, "spell_shadow_lifedrain.png", nullptr, {"Increases the amount of health transferred by your Health Funnel spell by 20%, reduces its health cost by 15%, and reduces all threat your Health Funnel generates by 50%. Allows Health Funnel to be used regardless of your demon's health.", "Increases the amount of health transferred by your Health Funnel spell by 40%, reduces its health cost by 30%, and reduces all threat your Health Funnel generates by 100%. Allows Health Funnel to be used regardless of your demon's health.", nullptr, nullptr, nullptr}},
    {"improved_imp", "Improved Imp", 1, 2, 3, "spell_shadow_summonimp.png", nullptr, {"Increases the damage of your Imp's Firebolt spell by 10% and the effect of its Fire Shield spell by 10%.", "Increases the damage of your Imp's Firebolt spell by 20% and the effect of its Fire Shield spell by 20%.", "Increases the damage of your Imp's Firebolt spell by 30% and the effect of its Fire Shield spell by 30%.", nullptr, nullptr}},
    {"demonic_embrace", "Demonic Embrace", 1, 3, 5, "spell_shadow_metamorphosis.png", nullptr, {"Increases your total Stamina by 3%.", "Increases your total Stamina by 6%.", "Increases your total Stamina by 9%.", "Increases your total Stamina by 12%.", "Increases your total Stamina by 15%."}},
    {"unholy_power", "Unholy Power", 1, 4, 5, "spell_shadow_shadowworddominate.png", nullptr, {"Increases all damage done by your Imp, Voidwalker, Succubus, Incubus, and Felhunter pets by 2%.", "Increases all damage done by your Imp, Voidwalker, Succubus, Incubus, and Felhunter pets by 4%.", "Increases all damage done by your Imp, Voidwalker, Succubus, Incubus, and Felhunter pets by 6%.", "Increases all damage done by your Imp, Voidwalker, Succubus, Incubus, and Felhunter pets by 8%.", "Increases all damage done by your Imp, Voidwalker, Succubus, Incubus, and Felhunter pets by 10%."}},
    {"demonic_aegis", "Demonic Aegis", 2, 1, 2, "spell_shadow_ragingscream.png", nullptr, {"Increases the effectiveness of your Demon Skin and Demon Armor spells by 15%.", "Increases the effectiveness of your Demon Skin and Demon Armor spells by 30%.", nullptr, nullptr, nullptr}},
    {"improved_voidwalker", "Improved Voidwalker", 2, 2, 3, "spell_shadow_summonvoidwalker.png", nullptr, {"Increases the effectiveness of your Voidwalker's Torment, Consume Shadows, Sacrifice, and Suffering spells by 10%.", "Increases the effectiveness of your Voidwalker's Torment, Consume Shadows, Sacrifice, and Suffering spells by 20%.", "Increases the effectiveness of your Voidwalker's Torment, Consume Shadows, Sacrifice, and Suffering spells by 30%.", nullptr, nullptr}},
    {"fel_vitality", "Fel Vitality", 2, 3, 3, "spell_shadow_demonictactics.png", nullptr, {"Increases the maximum health and Mana of your Imp, Voidwalker, Succubus, Incubus, and Felhunter by 5%, and increases your maximum Mana by 5%.", "Increases the maximum health and Mana of your Imp, Voidwalker, Succubus, Incubus, and Felhunter by 10%, and increases your maximum Mana by 10%.", "Increases the maximum health and Mana of your Imp, Voidwalker, Succubus, Incubus, and Felhunter by 15%, and increases your maximum Mana by 15%.", nullptr, nullptr}},
    {"demonic_energies", "Demonic Energies", 2, 4, 2, "spell_shadow_felmending.png", nullptr, {"You heal your pet for 8% of all spell damage you deal. When you gain Mana from Life Tap, your summoned demon gains 50% of the Mana you gain.", "You heal your pet for 15% of all spell damage you deal. When you gain Mana from Life Tap, your summoned demon gains 100% of the Mana you gain.", nullptr, nullptr, nullptr}},
    {"improved_sayaad", "Improved Sayaad", 3, 1, 3, "ability_warlock_randomizesuccubusincubus.png", nullptr, {"Increases the effect of your Succubus' and Incubus' Lash of Pain and Soothing Kiss spells by 10%, and increases the duration of your Succubus' and Incubus' Seduction and Lesser Invisibility spells by 10%.", "Increases the effect of your Succubus' and Incubus' Lash of Pain and Soothing Kiss spells by 20%, and increases the duration of your Succubus' and Incubus' Seduation and Lesser Invisibility spells by 20%.", "Increases the effect of your Succubus' and Incubus' Lash of Pain and Soothing Kiss spells by 30%, and increases the duration of your Succubus' and Incubus' Seduction and Lesser Invisibility spells by 30%.", nullptr, nullptr}},
    {"demonic_sacrifice", "Demonic Sacrifice", 3, 2, 1, "spell_shadow_psychicscream.png", nullptr, {"When activated, sacrifices your summoned Demon to enhance the opposing aspect of your power, granting you an effect that lasts 120 min. The effect is canceled if any Demon is summoned.\\n\\nImp: Increases your Shadow damage by 15%.\\n\\nVoidwalker: Restores 2% of your total Mana every 4 sec.\\n\\nSuccubus/Incubus: Increases your Fire damage by 15%.\\n\\nFelhunter: Restores 3% of your total Health every 4 sec.", nullptr, nullptr, nullptr, nullptr}},
    {"master_summoner", "Master Summoner", 3, 3, 2, "spell_shadow_impphaseshift.png", nullptr, {"Reduces the casting time of your Imp, Voidwalker, Succubus, Incubus, and Felhunter Summoning spells by 2 sec and the Mana cost by 20%.", "Reduces the casting time of your Imp, Voidwalker, Succubus, Incubus, and Felhunter Summoning spells by 4 sec and the Mana cost by 40%.", nullptr, nullptr, nullptr}},
    {"decimation", "Decimation", 4, 1, 2, "spell_fire_fireball02.png", nullptr, {"Reduces the cooldown of your Soul Fire spell by 45%. When you cast Shadow Bolt or Searing Pain on an enemy below 35% health, they deal 3% increased damage, and for the next 10 sec your Soul Fire spell has its cast time reduced by 20% and costs no Soul Shards.", "Reduces the cooldown of your Soul Fire spell by 90%. When you cast Shadow Bolt or Searing Pain on an enemy below 35% health, they deal 6% increased damage, and for the next 10 sec your Soul Fire spell has its cast time reduced by 40% and costs no Soul Shards.", nullptr, nullptr, nullptr}},
    {"fel_domination", "Fel Domination", 4, 3, 1, "spell_nature_removecurse.png", "Master Summoner", {"Your next Imp, Voidwalker, Succubus, Incubus, or Felhunter Summon spell has its casting time reduced by 5.5 sec and its Mana cost reduced by 50%.", nullptr, nullptr, nullptr, nullptr}},
    {"demonic_brand", "Demonic Brand", 4, 4, 3, "ability_demonhunter_chaoticimprint_fire.png", nullptr, {"Your Searing Pain generates 17% less threat and brands the target for 10 sec. Your pet's next 2 attacks against the target generate high threat and deal 65 to 68 Fire or Shadow damage based on the pet.", "Your Searing Pain generates 33% less threat and brands the target for 10 sec. Your pet's next 4 attacks against the target generate high threat and deal 65 to 68 Fire or Shadow damage based on the pet.", "Your Searing Pain generates 50% less threat and brands the target for 10 sec. Your pet's next 6 attacks against the target generate high threat and deal 65 to 68 Fire or Shadow damage based on the pet.", nullptr, nullptr}},
    {"improved_felhunter", "Improved Felhunter", 5, 1, 3, "spell_shadow_summonfelhunter.png", nullptr, {"Increases the Attack Power reduction of your Felhunter's Tainted Blood, the healing of its Devour Magic, and the detection level of its Paranoia by 10%, and reduces the cooldown of its Spell Lock by 2 sec.", "Increases the Attack Power reduction of your Felhunter's Tainted Blood, the healing of its Devour Magic, and the detection level of its Paranoia by 20%, and reduces the cooldown of its Spell Lock by 4 sec.", "Increases the Attack Power reduction of your Felhunter's Tainted Blood, the healing of its Devour Magic, and the detection level of its Paranoia by 30%, and reduces the cooldown of its Spell Lock by 6 sec.", nullptr, nullptr}},
    {"soul_link", "Soul Link", 5, 2, 1, "spell_shadow_gathershadows.png", "Demonic Sacrifice", {"When active, 30% of all damage taken by the caster is taken by your Imp, Voidwalker, Succubus, Incubus, or Felhunter Demon instead. In addition, both the Demon and the master will inflict 3% more damage. Lasts as long as the Demon is active.", nullptr, nullptr, nullptr, nullptr}},
    {"demonic_knowledge", "Demonic Knowledge", 5, 3, 3, "spell_shadow_improvedvampiricembrace.png", nullptr, {"Increases your spell damage and your Demon pet's spell damage by up to 33% of your level while you have a summoned Demon pet active.", "Increases your spell damage and your Demon pet's spell damage by up to 67% of your level while you have a summoned Demon pet active.", "Increases your spell damage and your Demon pet's spell damage by up to 100% of your level while you have a summoned Demon pet active.", nullptr, nullptr}},
    {"master_demonologist", "Master Demonologist", 6, 3, 5, "spell_shadow_shadowpact.png", nullptr, {"Grants both the Warlock and the summoned demon an effect as long as that demon is active.\\n\\nImp - Increases Fire damage done by 2%.\\n\\nVoidwalker - Reduces Physical damage taken by 2%.\\n\\nSuccubus/Incubus - Increases Shadow damage done by 2%.\\n\\nFelhunter - Reduces Magic damage taken by 2%.", "Grants both the Warlock and the summoned demon an effect as long as that demon is active.\\n\\nImp - Increases Fire damage done by 4%.\\n\\nVoidwalker - Reduces Physical damage taken by 4%.\\n\\nSuccubus/Incubus - Increases Shadow damage done by 4%.\\n\\nFelhunter - Reduces Magic damage taken by 4%.", "Grants both the Warlock and the summoned demon an effect as long as that demon is active.\\n\\nImp - Increases Fire damage done by 6%.\\n\\nVoidwalker - Reduces Physical damage taken by 6%.\\n\\nSuccubus/Incubus - Increases Shadow damage done by 6%.\\n\\nFelhunter - Reduces Magic damage taken by 6%.", "Grants both the Warlock and the summoned demon an effect as long as that demon is active.\\n\\nImp - Increases Fire damage done by 8%.\\n\\nVoidwalker - Reduces Physical damage taken by 8%.\\n\\nSuccubus/Incubus - Increases Shadow damage done by 8%.\\n\\nFelhunter - Reduces Magic damage taken by 8%.", "Grants both the Warlock and the summoned demon an effect as long as that demon is active.\\n\\nImp - Increases Fire damage done by 10%.\\n\\nVoidwalker - Reduces Physical damage taken by 10%.\\n\\nSuccubus/Incubus - Increases Shadow damage done by 10%.\\n\\nFelhunter - Reduces Magic damage taken by 10%."}},
    {"demonic_pact", "Demonic Pact", 7, 2, 1, "inv_ability_soulharvesterwarlock_demonicsoul.png", "Soul Link", {"Your Demonic Sacrifice effect is no longer cancelled by summoning a different Demon pet. Resummoning the sacrificed pet will still cancel the effect.", nullptr, nullptr, nullptr, nullptr}},
}};

inline const std::array<TalentNodeDef, 16> FOREVER_DESTRUCTION_NODES = {{
    {"destructive_reach", "Destructive Reach", 1, 1, 2, "spell_shadow_corpseexplode.png", nullptr, {"Increases the range of your damaging spells by 10%.", "Increases the range of your damaging spells by 20%.", nullptr, nullptr, nullptr}},
    {"improved_shadow_bolt", "Improved Shadow Bolt", 1, 2, 5, "spell_shadow_shadowbolt.png", nullptr, {"Your Shadow Bolt critical strikes increase Shadow damage taken by the target from your attacks by 4% for 12 sec.", "Your Shadow Bolt critical strikes increase Shadow damage taken by the target from your attacks by 8% for 12 sec.", "Your Shadow Bolt critical strikes increase Shadow damage taken by the target from your attacks by 12% for 12 sec.", "Your Shadow Bolt critical strikes increase Shadow damage taken by the target from your attacks by 16% for 12 sec.", "Your Shadow Bolt critical strikes increase Shadow damage taken by the target from your attacks by 20% for 12 sec."}},
    {"bane", "Bane", 1, 3, 5, "spell_shadow_deathpact.png", nullptr, {"Reduces the casting time of your Shadow Bolt, Immolate, and Incinerate spells by 0.1 sec and your Soul Fire spell by 0.4 sec.", "Reduces the casting time of your Shadow Bolt, Immolate, and Incinerate spells by 0.2 sec and your Soul Fire spell by 0.8 sec.", "Reduces the casting time of your Shadow Bolt, Immolate, and Incinerate spells by 0.3 sec and your Soul Fire spell by 1.2 sec.", "Reduces the casting time of your Shadow Bolt, Immolate, and Incinerate spells by 0.4 sec and your Soul Fire spell by 1.6 sec.", "Reduces the casting time of your Shadow Bolt, Immolate, and Incinerate spells by 0.5 sec and your Soul Fire spell by 2 sec."}},
    {"molten_skin", "Molten Skin", 2, 1, 5, "ability_mage_moltenarmor.png", nullptr, {"Reduces all damage taken by 2%.", "Reduces all damage taken by 4%.", "Reduces all damage taken by 6%.", "Reduces all damage taken by 8%.", "Reduces all damage taken by 10%."}},
    {"cataclysm", "Cataclysm", 2, 2, 3, "spell_fire_windsofwoe.png", nullptr, {"Reduces the Mana cost of your Destruction spells by 3%.", "Reduces the Mana cost of your Destruction spells by 6%.", "Reduces the Mana cost of your Destruction spells by 10%.", nullptr, nullptr}},
    {"aftermath", "Aftermath", 2, 3, 5, "spell_fire_fire.png", nullptr, {"Increases the initial damage of your Immolate spell by 10% and your Conflagrate spell has a 20% chance to Daze the target, reducing the target's movement speed by 50% for 5 sec.", "Increases the initial damage of your Immolate spell by 20% and your Conflagrate spell has a 40% chance to Daze the target, reducing the target's movement speed by 50% for 5 sec.", "Increases the initial damage of your Immolate spell by 30% and your Conflagrate spell has a 60% chance to Daze the target, reducing the target's movement speed by 50% for 5 sec.", "Increases the initial damage of your Immolate spell by 40% and your Conflagrate spell has a 80% chance to Daze the target, reducing the target's movement speed by 50% for 5 sec.", "Increases the initial damage of your Immolate spell by 50% and your Conflagrate spell has a 100% chance to Daze the target, reducing the target's movement speed by 50% for 5 sec."}},
    {"ruin", "Ruin", 3, 2, 5, "spell_shadow_shadowwordpain.png", nullptr, {"Increases the critical strike damage bonus of your Destruction spells by 20%.", "Increases the critical strike damage bonus of your Destruction spells by 40%.", "Increases the critical strike damage bonus of your Destruction spells by 60%.", "Increases the critical strike damage bonus of your Destruction spells by 80%.", "Increases the critical strike damage bonus of your Destruction spells by 100%."}},
    {"shadowburn", "Shadowburn", 3, 3, 1, "spell_shadow_scourgebuild.png", nullptr, {"Instantly blasts the target for 65 to 74 Shadow damage. If a non-trivial target dies within 8 sec of being hit with Shadowburn, the caster gains a Soul Shard.", nullptr, nullptr, nullptr, nullptr}},
    {"intensity", "Intensity", 4, 1, 3, "spell_fire_lavaspawn.png", nullptr, {"Gives you a 23% chance to resist interruption caused by damage while casting or channeling any Destruction spell.", "Gives you a 47% chance to resist interruption caused by damage while casting or channeling any Destruction spell.", "Gives you a 70% chance to resist interruption caused by damage while casting or channeling any Destruction spell.", nullptr, nullptr}},
    {"agonizing_flames", "Agonizing Flames", 4, 2, 3, "spell_fire_soulburn.png", nullptr, {"Increases the critical strike chance of your Searing Pain spell by 3% and the damage done by all your Destruction spells by 3%.", "Increases the critical strike chance of your Searing Pain spell by 7% and the damage done by all your Destruction spells by 7%.", "Increases the critical strike chance of your Searing Pain spell by 10% and the damage done by all your Destruction spells by 10%.", nullptr, nullptr}},
    {"conflagrate", "Conflagrate", 4, 3, 1, "spell_fire_fireball.png", nullptr, {"Ignites a target that is already afflicted by your Immolate spell, dealing 88 to 111 Fire damage and consuming your Immolate effect.", nullptr, nullptr, nullptr, nullptr}},
    {"pyroclasm", "Pyroclasm", 5, 1, 2, "spell_fire_volcano.png", "Intensity", {"Gives your Soul Fire spell a 13% chance to Stun the target for 3 sec, and your Rain of Fire and Hellfire spells a 13% chance over their duration to Stun targets they damage for 3 sec.", "Gives your Soul Fire spell a 26% chance to Stun the target for 3 sec, and your Rain of Fire and Hellfire spells a 26% chance over their duration to Stun targets they damage for 3 sec.", nullptr, nullptr, nullptr}},
    {"bane_of_havoc", "Bane of Havoc", 5, 2, 1, "ability_warlock_baneofhavoc.png", nullptr, {"Afflicts the target for 5 min, causing 15% of all damage done by the Warlock to other targets to also be dealt to the cursed target. Bane of Havoc is limited to 1 target, and only one Bane per Warlock can be active on any one target.", nullptr, nullptr, nullptr, nullptr}},
    {"fire_and_brimstone", "Fire and Brimstone", 5, 3, 3, "spell_fire_meteorstorm.png", "Conflagrate", {"Increases the critical strike chance of your Conflagrate spell by 8%.", "Increases the critical strike chance of your Conflagrate spell by 17%.", "Increases the critical strike chance of your Conflagrate spell by 25%.", nullptr, nullptr}},
    {"shadow_and_flame", "Shadow and Flame", 6, 3, 5, "spell_fire_playingwithfire.png", nullptr, {"Hitting an enemy with Conflagrate increases all Shadow damage you deal by 2% for 20 sec, and hitting an enemy with Shadowburn increases all Fire damage you deal by 2% for 20 sec. In addition, Conflagrate has a 20% chance not to consume Immolate, and Shadowburn has a 20% chance to instantly refund a Soul Shard.", "Hitting an enemy with Conflagrate increases all Shadow damage you deal by 4% for 20 sec, and hitting an enemy with Shadowburn increases all Fire damage you deal by 4% for 20 sec. In addition, Conflagrate has a 40% chance not to consume Immolate, and Shadowburn has a 40% chance to instantly refund a Soul Shard.", "Hitting an enemy with Conflagrate increases all Shadow damage you deal by 6% for 20 sec, and hitting an enemy with Shadowburn increases all Fire damage you deal by 6% for 20 sec. In addition, Conflagrate has a 60% chance not to consume Immolate, and Shadowburn has a 60% chance to instantly refund a Soul Shard.", "Hitting an enemy with Conflagrate increases all Shadow damage you deal by 8% for 20 sec, and hitting an enemy with Shadowburn increases all Fire damage you deal by 8% for 20 sec. In addition, Conflagrate has a 80% chance not to consume Immolate, and Shadowburn has a 80% chance to instantly refund a Soul Shard.", "Hitting an enemy with Conflagrate increases all Shadow damage you deal by 10% for 20 sec, and hitting an enemy with Shadowburn increases all Fire damage you deal by 10% for 20 sec. In addition, Conflagrate has a 100% chance not to consume Immolate, and Shadowburn has a 100% chance to instantly refund a Soul Shard."}},
    {"incinerate", "Incinerate", 7, 2, 1, "spell_fire_burnout.png", "Bane of Havoc", {"Deals 100 to 114 Fire damage to your target and an additional 25% damage if the target is afflicted by Immolate.", nullptr, nullptr, nullptr, nullptr}},
}};

struct Talents {
    AfflictionTalents aff;
    DemonologyTalents demo;
    DestructionTalents destro;

    int total_points() const {
        return aff.total_points() + demo.total_points() + destro.total_points();
    }

    bool is_valid() const {
        return total_points() <= 51;
    }

    // =========================================================================
    // Build Presets
    // =========================================================================

    // 1. 5/11/35 DS/AF DS-Imp
    static Talents create_forever_shadow_destro() {
        Talents t;
        // Affliction: 5 points
        t.aff.suppression = 5;             // 5/5 (+5% hit in Forever, -20% threat)

        // Demonology: 11 points (Sac Imp -> +15% Shadow!)
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.demonic_sacrifice = 1;      // Sac Imp -> +15% Shadow!

        // Destruction: 35 points
        t.destro.destructive_reach = 2;
        t.destro.improved_shadow_bolt = 5; // 20% Shadow vuln on crit
        t.destro.bane = 5;                 // -0.5s SB cast time
        t.destro.cataclysm = 3;            // -9% mana cost
        t.destro.aftermath = 2;
        t.destro.ruin = 5;                 // +100% crit damage bonus (2.0x total)
        t.destro.shadowburn = 1;           // instant shadow finisher
        t.destro.agonizing_flames = 3;     // +9% all Destruction spell damage!
        t.destro.conflagrate = 1;
        t.destro.fire_and_brimstone = 3;
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Conflag buffs Shadow by 10%!
        return t;
    }

    // 1b. 19/11/21 NF/DS/Ruin DS-Imp
    static Talents create_forever_nf_ds_ruin() {
        Talents t;
        // Affliction: 19 points (Nightfall 2/2)
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;             // 5/5 (+5% spell hit, -20% threat)
        t.aff.improved_corruption = 5;     // 5/5 Instant Corruption (+10% dmg)
        t.aff.malediction = 5;             // 5/5 (+5% periodic damage)
        t.aff.nightfall = 2;               // 2/2 Shadow Trance on Corruption ticks

        // Demonology: 11 points (Demonic Sacrifice -> Sac Imp for +15% Shadow!)
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.demonic_sacrifice = 1;      // Sac Imp -> +15% Shadow!

        // Destruction: 21 points (Ruin 5/5 + Agonizing Flames 3/3)
        t.destro.improved_shadow_bolt = 5; // 20% Shadow vuln on crit
        t.destro.bane = 5;                 // -0.5s SB cast time
        t.destro.cataclysm = 2;            // -6% mana cost
        t.destro.ruin = 5;                 // +100% crit damage bonus (2.0x total)
        t.destro.shadowburn = 1;           // instant shadow finisher
        t.destro.agonizing_flames = 3;     // +9% all Destruction spell damage!
        return t;
    }

    // 2. 9/11/31 Fire Destro+Suppression DS-Succ
    static Talents create_forever_ds_incinerate() {
        Talents t;
        // Affliction: 9 points
        t.aff.suppression = 5;             // 5/5 (+5% spell hit, -20% threat)
        t.aff.improved_corruption = 4;     // 4/5 (0.4s cast time Corruption)

        // Demonology: 11 points (Sac Succubus -> +15% Fire!)
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.demonic_sacrifice = 1;      // Sac Succubus -> +15% Fire!

        // Destruction: 31 points
        t.destro.bane = 5;                 // -0.5s Incinerate (2.0s cast!)
        t.destro.cataclysm = 1;
        t.destro.aftermath = 5;            // 1/5 Aftermath
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // triggers +10% Fire buff from Shadow & Flame!
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;
        t.destro.bane_of_havoc = 1;        // Prerequisite for Incinerate
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Shadowburn buffs Fire by 10%
        t.destro.incinerate = 1;           // Fire filler spell (2.0s cast, +25% dmg with Immolate)
        return t;
    }

    // Compatibility alias
    static Talents create_forever_fire_destro() {
        return create_forever_ds_incinerate();
    }

    // 2a. 7/11/33 Fire Destro+Suppression DS-Succ (No Corruption)
    // Variation of create_forever_ds_incinerate: drops Improved Corruption
    // (4/5 -> 0/5) and moves those points into Improved Life Tap (+2) and
    // Cataclysm (+2, 1/3 -> 3/3). Paired with the Fire Destro no-Corruption
    // rotation (Immolate + Conflagrate + Shadowburn + Incinerate filler).
    static Talents create_forever_ds_incinerate_no_corruption() {
        Talents t;
        // Affliction: 7 points
        t.aff.suppression = 5;             // 5/5 (+5% spell hit, -20% threat)
        t.aff.improved_life_tap = 2;       // 2/2 (+20% mana from Life Tap)
        t.aff.improved_corruption = 0;     // 0/5 (Corruption unused)

        // Demonology: 11 points (Sac Succubus -> +15% Fire!)
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.demonic_sacrifice = 1;      // Sac Succubus -> +15% Fire!

        // Destruction: 33 points
        t.destro.bane = 5;                 // -0.5s Incinerate (2.0s cast!)
        t.destro.cataclysm = 3;            // -9% mana cost
        t.destro.aftermath = 5;            // 1/5 Aftermath
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // triggers +10% Fire buff from Shadow & Flame!
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;
        t.destro.bane_of_havoc = 1;        // Prerequisite for Incinerate
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Shadowburn buffs Fire by 10%
        t.destro.incinerate = 1;           // Fire filler spell (2.0s cast, +25% dmg with Immolate)
        return t;
    }

    // 2b. 5/11/35 DS/Searing Pain DS-Succ
    static Talents create_forever_ds_searing_pain() {
        Talents t;
        // Affliction: 5 points
        t.aff.suppression = 5;             // +5% spell hit, -20% threat

        // Demonology: 11 points (Sac Succubus -> +15% Fire!)
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.demonic_sacrifice = 1;      // Sac Succubus -> +15% Fire!

        // Destruction: 35 points
        t.destro.destructive_reach = 2;
        t.destro.improved_shadow_bolt = 2;
        t.destro.bane = 5;                 // -0.5s Immolate / -2.0s Soul Fire
        t.destro.cataclysm = 3;
        t.destro.aftermath = 5;            // Immolate initial direct damage +50%!
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // triggers +10% Fire buff from Shadow & Flame!
        t.destro.agonizing_flames = 3;     // +9% Destruction damage, +9% Searing Pain crit!
        t.destro.conflagrate = 1;
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Shadowburn buffs Fire by 10%
        return t;
    }

    // 2c. 3/17/31 Shadow and Flame Fire DS-Succ (Decimation + DS-Succ)
    static Talents create_forever_shadow_and_flame_fire_ds_succ() {
        Talents t;
        // Affliction: 3 points
        t.aff.suppression = 3;             // +3% spell hit

        // Demonology: 17 points (Demonic Sacrifice + 2/2 Decimation)
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.demonic_sacrifice = 1;      // Sac Succubus -> +15% Fire!
        t.demo.decimation = 2;             // Soul Fire execute <35% HP (+6% Searing Pain/SB, -40% SF cast time, 6s CD)
        t.demo.master_summoner = 2;
        t.demo.improved_imp = 2;

        // Destruction: 31 points
        t.destro.destructive_reach = 1;
        t.destro.bane = 5;                 // -0.5s Incinerate / -2.0s Soul Fire
        t.destro.cataclysm = 3;
        t.destro.aftermath = 2;            // Immolate initial direct damage +20%
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // triggers +10% Fire buff from Shadow & Flame!
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;
        t.destro.bane_of_havoc = 1;        // Prerequisite for Incinerate
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Shadowburn buffs Fire by 10%
        t.destro.incinerate = 1;           // Fire filler spell (2.0s cast, +25% dmg with Immolate)
        return t;
    }

    // Compatibility alias
    static Talents create_forever_fire_destro_decimation() {
        return create_forever_shadow_and_flame_fire_ds_succ();
    }

    // 2d. 1/17/33 Shadow and Flame Fire
    static Talents create_forever_shadow_and_flame() {
        Talents t;
        // Affliction: 1 point
        t.aff.suppression = 1;             // +1% spell hit

        // Demonology: 17 points
        t.demo.improved_imp = 3;           // +30% Imp Firebolt damage
        t.demo.unholy_power = 5;           // +10% Imp damage
        t.demo.demonic_aegis = 2;          // +30% Armor/Fel Armor effects
        t.demo.fel_vitality = 3;           // +15% Max Mana
        t.demo.demonic_energies = 2;       // Life Tap restores pet mana
        t.demo.decimation = 2;             // Soul Fire execute <35% HP

        // Destruction: 33 points
        t.destro.improved_shadow_bolt = 5; // +20% Shadow vulnerability on crit
        t.destro.bane = 5;                 // -0.5s Incinerate / -2.0s Soul Fire
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // Instant finisher, triggers +10% Fire buff from Shadow & Flame!
        t.destro.intensity = 3;            // 70% pushback resistance
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;          // Conflagrate (never consumes Immolate, buffs Shadow by 10%)
        t.destro.bane_of_havoc = 1;        // Prerequisite for Incinerate
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Shadowburn buffs Fire by 10%
        t.destro.incinerate = 1;           // Fire filler spell (2.0s cast, +25% dmg with Immolate)
        return t;
    }

    // 2d2. 10/10/31 Shadow and Flame Fire 2 (5/5 Suppression, 3/5 Imp Corruption, 2/2 Imp Life Tap)
    static Talents create_forever_shadow_and_flame_fire_2() {
        Talents t;
        // Affliction: 10 points
        t.aff.improved_life_tap = 2;       // 2/2 (+20% mana from Life Tap)
        t.aff.suppression = 5;             // 5/5 (+5% spell hit, -20% threat)
        t.aff.improved_corruption = 3;     // 3/5 (-1.2s cast time, +6% damage)

        // Demonology: 10 points
        t.demo.improved_imp = 3;           // +30% Imp Firebolt damage
        t.demo.unholy_power = 5;           // +10% Imp damage
        t.demo.demonic_energies = 2;       // Life Tap restores pet mana

        // Destruction: 31 points
        t.destro.improved_shadow_bolt = 0; // 3/5 Improved Shadow Bolt
        t.destro.bane = 5;                 // -0.5s Incinerate / -2.0s Soul Fire
        t.destro.cataclysm = 1;            // 2/3 (-6% mana cost)
        t.destro.aftermath = 5;            // 1/5 Aftermath (+10% initial Immolate damage)
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // Instant finisher, triggers +10% Fire buff from Shadow & Flame!
        t.destro.intensity = 0;            // 0/3 Intensity
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;          // Conflagrate (never consumes Immolate, buffs Shadow by 10%)
        t.destro.bane_of_havoc = 1;        // Prerequisite for Incinerate
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Shadowburn buffs Fire by 10%
        t.destro.incinerate = 1;           // Fire filler spell (2.0s cast, +25% dmg with Immolate)
        return t;
    }

    // 2d3. 7/13/31 Incinerate - Suppression + Succubus (5/5 Suppression, 2/5 Imp Corruption,
    //      5/5 Unholy Power, 3/3 Fel Vitality, 2/2 Demonic Energies, 3/3 Imp Sayaad; active Succubus)
    static Talents create_forever_shadow_and_flame_fire_2_succubus() {
        Talents t;
        // Affliction: 7 points
        t.aff.suppression = 5;             // 5/5 (+5% spell hit, -20% threat)
        t.aff.improved_corruption = 2;     // 2/5 (-0.8s cast time, +4% damage)

        // Demonology: 13 points
        t.demo.unholy_power = 5;           // +10% pet damage
        t.demo.fel_vitality = 3;           // +15% Max Mana
        t.demo.demonic_energies = 2;       // Life Tap restores pet mana
        t.demo.improved_sayaad = 3;        // +30% Succubus Lash of Pain, -3s Lash cooldown

        // Destruction: 31 points
        t.destro.bane = 5;                 // -0.5s Incinerate / -2.0s Soul Fire
        t.destro.cataclysm = 1;            // 1/3 (-3% mana cost)
        t.destro.aftermath = 5;            // 5/5 Aftermath
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // Instant finisher, triggers +10% Fire buff from Shadow & Flame!
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;          // Conflagrate (never consumes Immolate, buffs Shadow by 10%)
        t.destro.bane_of_havoc = 1;        // Prerequisite for Incinerate
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Shadowburn buffs Fire by 10%
        t.destro.incinerate = 1;           // Fire filler spell (2.0s cast, +25% dmg with Immolate)
        return t;
    }

    // 2e. 2/17/32 Shadow and Flame Shadow
    static Talents create_forever_shadow_and_flame_shadow() {
        Talents t;
        // Affliction: 2 points
        t.aff.suppression = 2;             // +2% spell hit

        // Demonology: 17 points
        t.demo.improved_imp = 3;           // +30% Imp Firebolt damage
        t.demo.unholy_power = 5;           // +10% Imp damage
        t.demo.demonic_aegis = 2;          // +30% Armor/Fel Armor effects
        t.demo.fel_vitality = 3;           // +15% Max Mana
        t.demo.demonic_energies = 2;       // Life Tap restores pet mana
        t.demo.decimation = 2;             // Soul Fire execute <35% HP

        // Destruction: 32 points
        t.destro.improved_shadow_bolt = 5; // +20% Shadow vulnerability on crit
        t.destro.bane = 5;                 // -0.5s Shadow Bolt / -2.0s Soul Fire
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // Instant finisher
        t.destro.intensity = 3;            // 70% pushback resistance
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;          // Conflagrate (buffs Shadow damage by +10% via Shadow & Flame)
        t.destro.bane_of_havoc = 1;        // Prerequisite
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate & buffs Shadow by 10%
        // Note: 0 points in incinerate
        return t;
    }

    // 2f. 8/13/30 Shadow and Flame Shadow (8 Aff / 13 Demo / 30 Destro)
    static Talents create_forever_shadow_and_flame_shadow_2() {
        Talents t;
        // Affliction: 8 points
        t.aff.improved_life_tap = 1;
        t.aff.suppression = 5;
        t.aff.improved_corruption = 2;

        // Demonology: 13 points
        t.demo.unholy_power = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_energies = 2;
        t.demo.improved_sayaad = 3;

        // Destruction: 30 points
        t.destro.improved_shadow_bolt = 5; // +20% Shadow vulnerability on crit
        t.destro.bane = 5;                 // -0.5s Shadow Bolt / -2.0s Soul Fire
        t.destro.cataclysm = 2;            // -6% mana cost
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // Instant finisher
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;          // Conflagrate (buffs Shadow damage by +10% via Shadow & Flame)
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate & buffs Shadow by 10%
        // Note: 0 points in incinerate
        return t;
    }


    // 3. 2/31/18 DP/AF Shadow DS-Imp
    static Talents create_forever_demonic_pact() {
        Talents t;
        // Affliction: 2 points
        t.aff.suppression = 2;             // +4% spell hit

        // Demonology: 31 points (Capstone: Demonic Pact!)
        t.demo.demonic_embrace = 5;
        t.demo.unholy_power = 5;
        t.demo.fel_vitality = 3;
        t.demo.decimation = 2;             // 2/2 Decimation (Execute SF + 6% SB buff <35% HP)
        t.demo.demonic_energies = 2;       // 2/2 Demonic Energies (Life Tap shares mana with pet + pet heal)
        t.demo.improved_sayaad = 3;
        t.demo.demonic_sacrifice = 1;      // Sac Imp for +15% Shadow!
        t.demo.soul_link = 1;              // +3% all damage
        t.demo.demonic_knowledge = 3;      // +60 Spell Power while pet is out!
        t.demo.master_demonologist = 5;    // +10% Shadow damage from Succubus!
        t.demo.demonic_pact = 1;           // KEEP Demonic Sacrifice WHILE SUMMONING SUCCUBUS!

        // Destruction: 18 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;
        t.destro.ruin = 5;                 // +100% spell crit bonus!
        t.destro.agonizing_flames = 3;     // +9% Destruction damage!
        return t;
    }

    // 3a. 2/31/18 DP/AF Shadow Corruption (2/2 Improved Corruption over Suppression)
    static Talents create_forever_dp_af_shadow_corruption() {
        Talents t;
        // Affliction: 2 points
        t.aff.suppression = 0;               // 0/5 (hit capped from gear)
        t.aff.improved_corruption = 2;       // 2/5 (-0.8s cast time, +4% damage)

        // Demonology: 31 points (Capstone: Demonic Pact!)
        t.demo.demonic_embrace = 5;
        t.demo.unholy_power = 5;
        t.demo.fel_vitality = 3;
        t.demo.decimation = 2;               // 2/2 Decimation (Execute SF + 6% SB buff <35% HP)
        t.demo.demonic_energies = 2;         // 2/2 Demonic Energies (Life Tap shares mana with pet + pet heal)
        t.demo.improved_sayaad = 3;
        t.demo.demonic_sacrifice = 1;        // Sac Imp for +15% Shadow!
        t.demo.soul_link = 1;                // +3% all damage
        t.demo.demonic_knowledge = 3;        // +60 Spell Power while pet is out!
        t.demo.master_demonologist = 5;      // +10% Shadow damage from Succubus!
        t.demo.demonic_pact = 1;             // KEEP Demonic Sacrifice WHILE SUMMONING SUCCUBUS!

        // Destruction: 18 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;
        t.destro.ruin = 5;                   // +100% spell crit bonus!
        t.destro.agonizing_flames = 3;       // +9% Destruction damage!
        return t;
    }

    // 3b. 0/31/20 DP/AF Fire DS-Succ
    static Talents create_forever_demonic_pact_fire() {
        Talents t;
        // Affliction: 0 points

        // Demonology: 31 points (Capstone: Demonic Pact!)
        t.demo.demonic_embrace = 4;        // 4/5 Demonic Embrace
        t.demo.improved_imp = 3;           // +30% Imp Firebolt damage
        t.demo.unholy_power = 5;           // 5/5 Unholy Power (+10% pet damage)
        t.demo.fel_vitality = 3;
        t.demo.demonic_energies = 2;       // 2/2 Demonic Energies (Life Tap shares mana with pet + pet heal)
        t.demo.demonic_sacrifice = 1;      // Sac Succubus for +15% Fire!
        t.demo.demonic_brand = 3;          // 3/3 Demonic Brand
        t.demo.soul_link = 1;              // +3% all damage
        t.demo.demonic_knowledge = 3;      // +60 Spell Power while pet is out!
        t.demo.master_demonologist = 5;    // +10% Fire damage from active Imp!
        t.demo.demonic_pact = 1;           // KEEP Demonic Sacrifice WHILE SUMMONING IMP!

        // Destruction: 20 points
        t.destro.aftermath = 2;            // +20% initial Immolate damage
        t.destro.bane = 5;                 // -0.5s Immolate / -2.0s Soul Fire
        t.destro.cataclysm = 3;
        t.destro.ruin = 5;                 // +100% spell crit bonus!
        t.destro.shadowburn = 1;           // Instant finisher
        t.destro.conflagrate = 1;          // Conflagrate
        t.destro.agonizing_flames = 3;     // +9% Destruction damage!
        return t;
    }

    // 4. 40/11/0 Deep Affliction DS-Imp
    static Talents create_forever_deep_affliction() {
        Talents t;
        // Affliction: 40 points
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;             // +5% spell hit, -20% threat
        t.aff.improved_corruption = 5;     // Instant, +10% damage
        t.aff.improved_drains = 3;         // +20% Drain Life, Drain Soul, Wrack damage
        t.aff.malediction = 5;             // +5% periodic damage
        t.aff.pandemic = 3;                // +100% DoT crit damage bonus!
        t.aff.malevolence = 5;             // +5% Shadow spell crit
        t.aff.siphon_life = 1;
        t.aff.soul_siphon = 3;             // +12%/affliction effect (up to +36%) to Drains & Wrack
        t.aff.shadow_mastery = 5;          // +5% Shadow damage
        t.aff.drain_hope = 1;              // +10% Shadow DoT amplification!
        t.aff.amplify_curse = 1;
        t.aff.curse_of_exhaustion = 1;

        // Demonology: 11 points (Demonic Sacrifice Imp -> +15% Shadow damage!)
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.demonic_sacrifice = 1;

        // Destruction: 0 points
        return t;
    }

    // 4b. 35/6/10 Deep Affliction Imp
    static Talents create_forever_deep_affliction_imp() {
        Talents t;
        // Affliction: 35 points
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;
        t.aff.improved_corruption = 5;
        t.aff.improved_drains = 3;
        t.aff.pandemic = 3;
        t.aff.malevolence = 5;
        t.aff.nightfall = 2;
        t.aff.siphon_life = 1;
        t.aff.soul_siphon = 3;
        t.aff.shadow_mastery = 5;
        t.aff.drain_hope = 1; // Wrack

        // Demonology: 6 points
        t.demo.improved_imp = 3;
        t.demo.unholy_power = 2;
        t.demo.demonic_energies = 1;

        // Destruction: 10 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;

        return t;
    }

    // 5. Forever SM/AF (32/0/19)
    static Talents create_forever_sm_ruin() {
        Talents t;
        // Affliction: 32 points
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;
        t.aff.improved_corruption = 5;
        t.aff.malediction = 5;
        t.aff.pandemic = 3;
        t.aff.malevolence = 5;
        t.aff.nightfall = 2;
        t.aff.shadow_mastery = 5;             // 5/5 SM (+5% Shadow dmg)

        // Destruction: 19 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;
        t.destro.ruin = 5;
        t.destro.shadowburn = 1;
        t.destro.agonizing_flames = 3;         // 3/3 Agonizing Flames (+9% Destro spell dmg!)
        return t;
    }

    // 5a. Forever SM/AF alias
    static Talents create_forever_sm_ruin_pure() {
        return create_forever_sm_ruin();
    }

    // 5b. Forever SM/AF Max Flames alias
    static Talents create_forever_sm_ruin_max_flames() {
        return create_forever_sm_ruin();
    }

    // 5c. Forever NF/AF (23/10/18 - Nightfall + active Imp + Ruin)
    static Talents create_forever_nf_af() {
        Talents t;
        // Affliction: 23 points
        t.aff.improved_life_tap = 2;         // 2/2 (+20% mana from Life Tap)
        t.aff.suppression = 5;               // 5/5 (+5% spell hit, -20% threat)
        t.aff.improved_corruption = 5;       // 5/5 (Instant Corruption, +10% damage)
        t.aff.malediction = 4;               // 4/5 (+4% periodic damage)
        t.aff.malevolence = 5;               // 5/5 (+5% Shadow spell crit)
        t.aff.nightfall = 2;                 // 2/2 (Shadow Trance on Corruption ticks)

        // Demonology: 10 points (active Imp)
        t.demo.improved_imp = 3;             // 3/3 (+30% Imp Firebolt damage)
        t.demo.unholy_power = 5;             // 5/5 (+10% pet damage)
        t.demo.demonic_energies = 2;         // 2/2 (Life Tap shares mana with pet + pet heal)

        // Destruction: 18 points
        t.destro.improved_shadow_bolt = 5;   // 5/5 (20% Shadow vuln on crit)
        t.destro.bane = 5;                   // 5/5 (-0.5s Shadow Bolt cast time)
        t.destro.ruin = 5;                   // 5/5 (+100% crit damage bonus, 2.0x total)
        t.destro.agonizing_flames = 3;       // 3/3 (+9% Destruction spell damage)
        return t;
    }

    // 6. Forever MD / Ruin (0/31/20 - 5/5 Master Demo + 5/5 Ruin)
    static Talents create_forever_md_ruin() {
        Talents t;
        // Demonology: 31 points
        t.demo.demonic_embrace = 5;
        t.demo.improved_imp = 3;
        t.demo.unholy_power = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.improved_sayaad = 3;
        t.demo.master_summoner = 2;
        t.demo.decimation = 2;
        t.demo.soul_link = 1;
        t.demo.demonic_knowledge = 3;
        t.demo.master_demonologist = 5;

        // Destruction: 20 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;
        t.destro.cataclysm = 3;
        t.destro.ruin = 5;
        t.destro.shadowburn = 1;
        t.destro.destructive_reach = 1;
        return t;
    }

    // 7. Forever Aff Incinerate (13/7/31)
    static Talents create_forever_aff_incinerate() {
        Talents t;
        // Affliction: 13 points
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;
        t.aff.improved_corruption = 2;
        t.aff.malediction = 1;
        t.aff.improved_bane_of_agony = 2;
        t.aff.amplify_curse = 1;

        // Demonology: 7 points
        t.demo.improved_imp = 1;
        t.demo.unholy_power = 4;
        t.demo.fel_vitality = 2;

        // Destruction: 31 points
        t.destro.bane = 5;
        t.destro.cataclysm = 3;
        t.destro.aftermath = 3;
        t.destro.ruin = 5;
        t.destro.shadowburn = 1;
        t.destro.agonizing_flames = 3;
        t.destro.conflagrate = 1;
        t.destro.bane_of_havoc = 1;
        t.destro.fire_and_brimstone = 3;
        t.destro.shadow_and_flame = 5;
        t.destro.incinerate = 1;
        return t;
    }

    // 8. 12/31/8 Aff/DP (12 Aff / 31 Demo / 8 Destro)
    static Talents create_forever_aff_dp() {
        Talents t;
        // Affliction: 12 points
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;
        t.aff.improved_corruption = 4;
        t.aff.amplify_curse = 1;

        // Demonology: 31 points (Capstone: Demonic Pact!)
        t.demo.demonic_embrace = 5;
        t.demo.unholy_power = 5;
        t.demo.fel_vitality = 2;
        t.demo.improved_sayaad = 3;
        t.demo.demonic_sacrifice = 1;
        t.demo.master_summoner = 2;
        t.demo.decimation = 2;
        t.demo.fel_domination = 1;
        t.demo.soul_link = 1;
        t.demo.demonic_knowledge = 3;
        t.demo.master_demonologist = 5;
        t.demo.demonic_pact = 1;

        // Destruction: 8 points
        t.destro.improved_shadow_bolt = 3;
        t.destro.bane = 5;
        return t;
    }

    // Compatibility aliases and standard methods
    static Talents create_aff_dp() { return create_forever_aff_dp(); }
    static Talents create_forever_ds_af() { return create_forever_shadow_destro(); }
    static Talents create_forever_dp_af_shadow() { return create_forever_demonic_pact(); }
    static Talents create_forever_dp_af_fire() { return create_forever_demonic_pact_fire(); }
    static Talents create_forever_sm_af() { return create_forever_sm_ruin_max_flames(); }
    static Talents create_ds_ruin() { return create_forever_shadow_destro(); }
    static Talents create_ds_af() { return create_forever_shadow_destro(); }
    static Talents create_sm_ruin() { return create_forever_sm_ruin_max_flames(); }
    static Talents create_sm_af() { return create_forever_sm_ruin_max_flames(); }
    static Talents create_dp_af() { return create_forever_demonic_pact(); }
    static Talents create_fire_destro() { return create_forever_ds_incinerate(); }
    static Talents create_ds_incinerate() { return create_forever_ds_incinerate(); }
    static Talents create_ds_searing_pain() { return create_forever_ds_searing_pain(); }
    static Talents create_md_ruin() { return create_forever_md_ruin(); }
    static Talents create_forever_nf_ds() { return create_forever_nf_ds_ruin(); }
    static Talents create_nf_ds_ruin() { return create_forever_nf_ds_ruin(); }
    static Talents create_nf_ds() { return create_forever_nf_ds_ruin(); }
    static Talents create_nf_af() { return create_forever_nf_af(); }
    static Talents create_fire_destro_decimation() { return create_forever_fire_destro_decimation(); }
    static Talents create_shadow_and_flame() { return create_forever_shadow_and_flame(); }
    static Talents create_shadow_and_flame_shadow() { return create_forever_shadow_and_flame_shadow(); }
};

} // namespace warlock
