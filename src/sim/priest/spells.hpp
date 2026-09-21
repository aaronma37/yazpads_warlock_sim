#pragma once
#include <cstdint>
#include <string>
#include "src/sim/common/spell_types.hpp"

namespace priest {

enum class SpellID : uint8_t {
    NONE = 0,
    SHADOW_WORD_PAIN = 1,
    MIND_FLAY,
    MIND_BLAST,
    SHADOW_WORD_DEATH,
    DEVOURING_PLAGUE,
    VAMPIRIC_EMBRACE,
    SHADOWFORM,
    INNER_FOCUS,
    POWER_INFUSION,
    SMITE,
    HOLY_FIRE,
    HOLY_NOVA,
    PENANCE,
    STARSHARDS,
    CHASTISE,
    SHADOWGUARD,
    POTION_MANA,
    DEMONIC_RUNE,
    TRINKET_USE,
    RACIAL_BERSERKING,
    DARK_SACRIFICE,
    TOUCH_OF_THE_GRAVE,
    COUNT
};

inline const char* spell_id_to_string(SpellID id) {
    switch (id) {
        case SpellID::SHADOW_WORD_PAIN: return "Shadow Word: Pain";
        case SpellID::MIND_FLAY:        return "Mind Flay";
        case SpellID::MIND_BLAST:       return "Mind Blast";
        case SpellID::SHADOW_WORD_DEATH:return "Shadow Word: Death";
        case SpellID::DEVOURING_PLAGUE: return "Devouring Plague";
        case SpellID::VAMPIRIC_EMBRACE: return "Vampiric Embrace";
        case SpellID::SHADOWFORM:       return "Shadowform";
        case SpellID::INNER_FOCUS:      return "Inner Focus";
        case SpellID::POWER_INFUSION:   return "Power Infusion";
        case SpellID::SMITE:            return "Smite";
        case SpellID::HOLY_FIRE:        return "Holy Fire";
        case SpellID::HOLY_NOVA:        return "Holy Nova";
        case SpellID::PENANCE:          return "Penance";
        case SpellID::STARSHARDS:       return "Starshards";
        case SpellID::CHASTISE:         return "Chastise";
        case SpellID::SHADOWGUARD:      return "Shadowguard";
        case SpellID::POTION_MANA:      return "Major Mana Potion";
        case SpellID::DEMONIC_RUNE:     return "Demonic Rune";
        case SpellID::TRINKET_USE:      return "Trinket Use";
        case SpellID::RACIAL_BERSERKING:return "Berserking";
        case SpellID::DARK_SACRIFICE:   return "Dark Sacrifice";
        case SpellID::TOUCH_OF_THE_GRAVE: return "Touch of the Grave";
        default:                        return "None";
    }
}

inline const char* spell_id_to_name(SpellID id) {
    return spell_id_to_string(id);
}

inline const char* spell_id_to_icon(SpellID id) {
    switch (id) {
        case SpellID::SHADOW_WORD_PAIN: return "spell_shadow_shadowwordpain";
        case SpellID::MIND_FLAY:        return "spell_shadow_siphonmana";
        case SpellID::MIND_BLAST:       return "spell_shadow_unholyfrenzy";
        case SpellID::SHADOW_WORD_DEATH:return "spell_shadow_demonicfortitude";
        case SpellID::DEVOURING_PLAGUE: return "spell_shadow_devouringplague";
        case SpellID::VAMPIRIC_EMBRACE: return "spell_shadow_unsummonbuilding";
        case SpellID::SHADOWFORM:       return "spell_shadow_shadowform";
        case SpellID::INNER_FOCUS:      return "spell_frost_windwalkon";
        case SpellID::POWER_INFUSION:   return "spell_holy_powerinfusion";
        case SpellID::SMITE:            return "spell_holy_holysmite";
        case SpellID::HOLY_FIRE:        return "spell_holy_searinglight";
        case SpellID::HOLY_NOVA:        return "spell_holy_holynova";
        case SpellID::PENANCE:          return "spell_holy_penance";
        case SpellID::STARSHARDS:       return "spell_arcane_starfire";
        case SpellID::CHASTISE:         return "spell_holy_chastise";
        case SpellID::SHADOWGUARD:      return "spell_nature_lightningshield";
        case SpellID::POTION_MANA:      return "inv_potion_76";
        case SpellID::DEMONIC_RUNE:     return "inv_misc_gem_pearl_03";
        case SpellID::TRINKET_USE:      return "inv_misc_gem_pearl_04";
        case SpellID::RACIAL_BERSERKING:return "racial_troll_berserk";
        case SpellID::DARK_SACRIFICE:   return "spell_holy_powerinfusion_shadow";
        case SpellID::TOUCH_OF_THE_GRAVE: return "spell_shadow_chilltouch";
        default:                        return "";
    }
}

class SpellBook {
public:
    // Shadow Word: Pain (Rank 8: 18s duration base, 6 ticks, 3s tick interval, 762 base damage)
    static inline sim::SpellDefinition shadow_word_pain_rank8() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::SHADOW_WORD_PAIN);
        s.name = "Shadow Word: Pain";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 0.0; // Instant cast (1.5s GCD)
        s.mana_cost = 470.0;
        s.is_dot = true;
        s.dot_duration = 18.0;   // Extended to 24s by Imp SW:P (2/2)
        s.dot_tick_interval = 3.0;
        s.num_ticks = 6;         // 8 ticks with Imp SW:P
        s.dot_base_dmg_per_tick = 762.0 / 6.0; // 127.0 base per tick (762 total over 18s)
        s.dot_coeff_per_tick = 0.20;           // 20% SP per tick (120% total over 18s in Forever)
        return s;
    }

    // Mind Flay (Rank 6: 3-second channeled spell, 3 ticks every 1.0s, 390 base damage)
    static inline sim::SpellDefinition mind_flay_rank6() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::MIND_FLAY);
        s.name = "Mind Flay";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 3.0; // Channeled
        s.mana_cost = 205.0;
        s.is_channeled = true;
        s.dot_duration = 3.0;
        s.dot_tick_interval = 1.0;
        s.num_ticks = 3;
        s.dot_base_dmg_per_tick = 390.0 / 3.0; // 130.0 base over 3s
        s.dot_coeff_per_tick = 0.1667;         // 16.7% SP per tick (50% total in Forever)
        return s;
    }

    // Mind Blast (Rank 9: 1.5s cast, 8.0s CD, 472 to 498 direct shadow damage)
    static inline sim::SpellDefinition mind_blast_rank9() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::MIND_BLAST);
        s.name = "Mind Blast";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 1.5;
        s.mana_cost = 350.0;
        s.cooldown = 8.0; // Reduced by Improved Mind Blast (up to -2.5s -> 5.5s CD)
        s.min_dmg = 472.0;
        s.max_dmg = 498.0;
        s.direct_coefficient = 1.5 / 3.5; // ~0.4286
        return s;
    }

    // Shadow Word: Death (Rank 4: Level 56, Instant cast, 15s CD, 434 to 462 shadow damage)
    static inline sim::SpellDefinition shadow_word_death_rank4() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::SHADOW_WORD_DEATH);
        s.name = "Shadow Word: Death";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 340.0;
        s.cooldown = 15.0;
        s.min_dmg = 434.0;
        s.max_dmg = 462.0;
        s.direct_coefficient = 1.5 / 3.5;
        return s;
    }

    // Devouring Plague (Rank 6: Instant cast, 24s duration, 8 ticks every 3s, 848 base damage, 1 min CD in Forever)
    static inline sim::SpellDefinition devouring_plague_rank6() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::DEVOURING_PLAGUE);
        s.name = "Devouring Plague";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 985.0; // Reduced by Devouring Contagion (up to -50%)
        s.cooldown = 60.0;  // 1 min cooldown in Forever
        s.is_dot = true;
        s.dot_duration = 24.0;
        s.dot_tick_interval = 3.0;
        s.num_ticks = 8;
        s.dot_base_dmg_per_tick = 848.0 / 8.0; // 106.0 base per tick
        s.dot_coeff_per_tick = 0.10;           // 80% total SP
        return s;
    }

    // Smite (Rank 8: 2.5s cast, 160 to 180 direct Holy damage)
    static inline sim::SpellDefinition smite_rank8() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::SMITE);
        s.name = "Smite";
        s.school = sim::School::HOLY;
        s.base_cast_time = 2.5; // Reduced by Divine Fury (-0.5s -> 2.0s)
        s.mana_cost = 280.0;
        s.min_dmg = 160.0;
        s.max_dmg = 180.0;
        s.direct_coefficient = 2.5 / 3.5; // ~0.7143
        return s;
    }

    // Holy Fire (Rank 8: 3.5s cast, 184 to 232 direct Holy + 75 DoT over 10s)
    static inline sim::SpellDefinition holy_fire_rank8() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::HOLY_FIRE);
        s.name = "Holy Fire";
        s.school = sim::School::HOLY;
        s.base_cast_time = 3.5; // Reduced by Divine Fury (-0.5s -> 3.0s)
        s.mana_cost = 255.0;
        s.min_dmg = 184.0;
        s.max_dmg = 232.0;
        s.direct_coefficient = 0.75;
        s.is_dot = true;
        s.dot_duration = 10.0;
        s.dot_tick_interval = 2.0;
        s.num_ticks = 5;
        s.dot_base_dmg_per_tick = 75.0 / 5.0; // 15.0 base per tick
        s.dot_coeff_per_tick = 0.05;          // 25% total SP
        return s;
    }

    // Starshards (Rank 7: Night Elf Racial, Channeled 6s, 30s CD, 1800 Arcane damage over 6s)
    static inline sim::SpellDefinition starshards_rank7() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::STARSHARDS);
        s.name = "Starshards";
        s.school = sim::School::ARCANE;
        s.base_cast_time = 6.0;
        s.mana_cost = 350.0;
        s.cooldown = 30.0;
        s.is_channeled = true;
        s.dot_duration = 6.0;
        s.dot_tick_interval = 1.0;
        s.num_ticks = 6;
        s.dot_base_dmg_per_tick = 1800.0 / 6.0; // 300.0 base per tick
        s.dot_coeff_per_tick = 0.1667;          // 16.7% SP per tick (100% total over 6s)
        return s;
    }

    // Holy Nova (Rank 6: Level 60, Instant, 750 Mana, 174 to 200 Holy damage, 10yd radius)
    static inline sim::SpellDefinition holy_nova_rank6() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::HOLY_NOVA);
        s.name = "Holy Nova";
        s.school = sim::School::HOLY;
        s.base_cast_time = 0.0;
        s.mana_cost = 750.0;
        s.min_dmg = 174.0;
        s.max_dmg = 200.0;
        s.direct_coefficient = 0.107; // ~10.7% SP
        return s;
    }

    // Penance (Rank 4: 2s Channeled, 12s CD, 131 Holy damage per tick, 3 ticks)
    static inline sim::SpellDefinition penance_rank4() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::PENANCE);
        s.name = "Penance";
        s.school = sim::School::HOLY;
        s.base_cast_time = 2.0;
        s.mana_cost = 355.0;
        s.cooldown = 12.0;
        s.is_channeled = true;
        s.dot_duration = 2.0;
        s.dot_tick_interval = 1.0;
        s.num_ticks = 3;
        s.dot_base_dmg_per_tick = 131.0;
        s.dot_coeff_per_tick = 0.285; // 28.5% SP per tick as per Forever tooltip
        return s;
    }

    // Chastise (Rank 5: Dwarf Racial, Instant, 2 min CD, 272 to 306 Holy damage)
    static inline sim::SpellDefinition chastise_rank5() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::CHASTISE);
        s.name = "Chastise";
        s.school = sim::School::HOLY;
        s.base_cast_time = 0.0;
        s.mana_cost = 225.0;
        s.cooldown = 120.0;
        s.min_dmg = 272.0;
        s.max_dmg = 306.0;
        s.direct_coefficient = 1.5 / 3.5;
        return s;
    }

    // Shadowguard (Rank 6: Troll Racial, Instant, 3 charges, 96 Shadow damage per hit)
    static inline sim::SpellDefinition shadowguard_rank6() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::SHADOWGUARD);
        s.name = "Shadowguard";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 250.0;
        s.min_dmg = 96.0;
        s.max_dmg = 96.0;
        s.direct_coefficient = 0.27; // ~27% per charge (80% over 3 charges)
        return s;
    }
};

} // namespace priest
