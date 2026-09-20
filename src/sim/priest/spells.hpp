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
    POTION_MANA,
    DEMONIC_RUNE,
    TRINKET_USE,
    RACIAL_BERSERKING,
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
        case SpellID::POTION_MANA:      return "Major Mana Potion";
        case SpellID::DEMONIC_RUNE:     return "Demonic Rune";
        case SpellID::TRINKET_USE:      return "Trinket Use";
        case SpellID::RACIAL_BERSERKING:return "Berserking";
        default:                        return "None";
    }
}

class SpellBook {
public:
    // Shadow Word: Pain (Rank 8: 18s duration base, 6 ticks, 3s tick interval)
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
        s.dot_base_dmg_per_tick = 114.0; // Total 684 base damage over 18s
        s.dot_coeff_per_tick = 0.1667;   // Total 100% SP over 18s (1.0 / 6 = ~0.1667)
        return s;
    }

    // Mind Flay (Rank 6: 3-second channeled spell, 3 ticks every 1.0s)
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
        s.dot_base_dmg_per_tick = 142.0; // Total 426 base over 3s
        s.dot_coeff_per_tick = 0.15;     // 45% SP over 3s (~0.15 per tick)
        return s;
    }

    // Mind Blast (Rank 9: 1.5s cast, 8.0s CD, direct shadow damage)
    static inline sim::SpellDefinition mind_blast_rank9() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::MIND_BLAST);
        s.name = "Mind Blast";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 1.5;
        s.mana_cost = 350.0;
        s.cooldown = 8.0; // Reduced by Improved Mind Blast (up to -2.5s -> 5.5s CD)
        s.min_dmg = 508.0;
        s.max_dmg = 537.0;
        s.direct_coefficient = 1.5 / 3.5; // ~0.4286
        return s;
    }

    // Shadow Word: Death (Rank 1: Instant cast, 12s CD)
    static inline sim::SpellDefinition shadow_word_death_rank1() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::SHADOW_WORD_DEATH);
        s.name = "Shadow Word: Death";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 310.0;
        s.cooldown = 12.0;
        s.min_dmg = 450.0;
        s.max_dmg = 522.0;
        s.direct_coefficient = 1.5 / 3.5;
        return s;
    }

    // Devouring Plague (Rank 6: Instant cast, 24s duration, 8 ticks every 3s, 180s CD in Classic or standard DoT in Forever)
    static inline sim::SpellDefinition devouring_plague_rank6() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::DEVOURING_PLAGUE);
        s.name = "Devouring Plague";
        s.school = sim::School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 700.0; // Reduced by Devouring Contagion (up to -50%)
        s.cooldown = 24.0;
        s.is_dot = true;
        s.dot_duration = 24.0;
        s.dot_tick_interval = 3.0;
        s.num_ticks = 8;
        s.dot_base_dmg_per_tick = 113.0;
        s.dot_coeff_per_tick = 0.10; // 80% total SP
        return s;
    }

    // Smite (Rank 8: 2.5s cast, direct Holy damage)
    static inline sim::SpellDefinition smite_rank8() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::SMITE);
        s.name = "Smite";
        s.school = sim::School::HOLY;
        s.base_cast_time = 2.5; // Reduced by Divine Fury (-0.5s -> 2.0s)
        s.mana_cost = 280.0;
        s.min_dmg = 371.0;
        s.max_dmg = 415.0;
        s.direct_coefficient = 2.5 / 3.5; // ~0.7143
        return s;
    }

    // Holy Fire (Rank 8: 3.5s cast, direct Holy + 10s DoT)
    static inline sim::SpellDefinition holy_fire_rank8() {
        sim::SpellDefinition s;
        s.id = static_cast<uint8_t>(SpellID::HOLY_FIRE);
        s.name = "Holy Fire";
        s.school = sim::School::HOLY;
        s.base_cast_time = 3.5; // Reduced by Divine Fury (-0.5s -> 3.0s)
        s.mana_cost = 255.0;
        s.min_dmg = 430.0;
        s.max_dmg = 543.0;
        s.direct_coefficient = 0.75;
        s.is_dot = true;
        s.dot_duration = 10.0;
        s.dot_tick_interval = 2.0;
        s.num_ticks = 5;
        s.dot_base_dmg_per_tick = 33.0;
        s.dot_coeff_per_tick = 0.05;
        return s;
    }
};

} // namespace priest
