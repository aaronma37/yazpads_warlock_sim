#pragma once
#include <cstdint>
#include <string>

namespace warlock {

enum class SpellID : uint8_t {
    NONE = 0,
    SHADOW_BOLT = 1,
    CORRUPTION,
    CURSE_OF_SHADOWS,
    CURSE_OF_ELEMENTS,
    CURSE_OF_AGONY,
    CURSE_OF_DOOM,
    IMMOLATE,
    SEARING_PAIN,
    SHADOWBURN,
    CONFLAGRATE,
    INCINERATE,
    SOUL_FIRE,
    DRAIN_HOPE,
    LIFE_TAP,
    DEATH_COIL,
    PET_FIREBOLT,
    PET_LASH_OF_PAIN,
    POTION_MANA,
    DEMONIC_RUNE,
    TRINKET_USE
};

enum class School : uint8_t {
    SHADOW = 0,
    FIRE,
    PHYSICAL
};

struct SpellDefinition {
    SpellID id = SpellID::NONE;
    const char* name = "None";
    School school = School::SHADOW;

    double base_cast_time = 0.0;    // seconds
    double mana_cost = 0.0;
    double cooldown = 0.0;          // seconds

    // Direct damage
    double min_dmg = 0.0;
    double max_dmg = 0.0;
    double direct_coefficient = 0.0; // Spell power scaling ratio

    // Periodic (DoT)
    bool is_dot = false;
    double dot_duration = 0.0;      // seconds
    double dot_tick_interval = 0.0; // seconds
    int num_ticks = 0;
    double dot_base_dmg_per_tick = 0.0;
    double dot_coeff_per_tick = 0.0;

    bool is_channeled = false;
    bool is_binary = false;         // Binary spells (like Curses) don't have partial resists
};

class SpellBook {
public:
    static inline SpellDefinition shadow_bolt_rank10() {
        SpellDefinition s;
        s.id = SpellID::SHADOW_BOLT;
        s.name = "Shadow Bolt";
        s.school = School::SHADOW;
        s.base_cast_time = 3.0; // Reduced by Bane
        s.mana_cost = 380.0;
        s.min_dmg = 482.0;
        s.max_dmg = 538.0;
        s.direct_coefficient = 3.0 / 3.5; // ~0.8571
        return s;
    }

    static inline SpellDefinition corruption_rank7() {
        SpellDefinition s;
        s.id = SpellID::CORRUPTION;
        s.name = "Corruption";
        s.school = School::SHADOW;
        s.base_cast_time = 2.0; // Reduced to 0 by Imp Corruption 5/5
        s.mana_cost = 290.0;
        s.is_dot = true;
        s.dot_duration = 18.0;
        s.dot_tick_interval = 3.0;
        s.num_ticks = 6;
        s.dot_base_dmg_per_tick = 822.0 / 6.0; // 137.0
        s.dot_coeff_per_tick = 1.0 / 6.0;      // 100% total coefficient
        return s;
    }

    static inline SpellDefinition curse_of_agony_rank6() {
        SpellDefinition s;
        s.id = SpellID::CURSE_OF_AGONY;
        s.name = "Curse of Agony";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 215.0;
        s.is_dot = true;
        s.dot_duration = 24.0;
        s.dot_tick_interval = 2.0;
        s.num_ticks = 12;
        s.dot_base_dmg_per_tick = 1044.0 / 12.0; // 87.0 base avg per tick
        s.dot_coeff_per_tick = 1.0 / 12.0;       // 100% total coefficient
        return s;
    }

    static inline SpellDefinition curse_of_doom_rank1() {
        SpellDefinition s;
        s.id = SpellID::CURSE_OF_DOOM;
        s.name = "Curse of Doom";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 300.0;
        s.cooldown = 60.0;
        s.is_dot = true;
        s.dot_duration = 60.0;
        s.dot_tick_interval = 60.0;
        s.num_ticks = 1;
        s.dot_base_dmg_per_tick = 3200.0;
        s.dot_coeff_per_tick = 2.0; // 200% coefficient
        return s;
    }

    static inline SpellDefinition curse_of_shadows_rank2() {
        SpellDefinition s;
        s.id = SpellID::CURSE_OF_SHADOWS;
        s.name = "Curse of Shadows";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 175.0;
        s.is_binary = true;
        return s;
    }

    static inline SpellDefinition immolate_rank8() {
        SpellDefinition s;
        s.id = SpellID::IMMOLATE;
        s.name = "Immolate";
        s.school = School::FIRE;
        s.base_cast_time = 2.0; // Reduced by Bane to 1.5s
        s.mana_cost = 380.0;
        s.min_dmg = 258.0;
        s.max_dmg = 306.0;
        s.direct_coefficient = 0.20; // 20% direct
        s.is_dot = true;
        s.dot_duration = 15.0;
        s.dot_tick_interval = 3.0;
        s.num_ticks = 5;
        s.dot_base_dmg_per_tick = 485.0 / 5.0; // 97.0
        s.dot_coeff_per_tick = 0.65 / 5.0;     // 65% total DoT
        return s;
    }

    static inline SpellDefinition searing_pain_rank6() {
        SpellDefinition s;
        s.id = SpellID::SEARING_PAIN;
        s.name = "Searing Pain";
        s.school = School::FIRE;
        s.base_cast_time = 1.5;
        s.mana_cost = 168.0;
        s.min_dmg = 204.0;
        s.max_dmg = 240.0;
        s.direct_coefficient = 1.5 / 3.5; // ~0.4286
        return s;
    }

    static inline SpellDefinition shadowburn_rank6() {
        SpellDefinition s;
        s.id = SpellID::SHADOWBURN;
        s.name = "Shadowburn";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 365.0;
        s.cooldown = 8.0;
        s.min_dmg = 450.0;
        s.max_dmg = 502.0;
        s.direct_coefficient = 1.5 / 3.5; // 0.4286
        return s;
    }

    static inline SpellDefinition life_tap_rank6() {
        SpellDefinition s;
        s.id = SpellID::LIFE_TAP;
        s.name = "Life Tap";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 0.0;
        s.min_dmg = 580.0; // Health cost
        s.max_dmg = 580.0; // Base Mana return (scales with 80% spell power)
        s.direct_coefficient = 0.80;
        return s;
    }

    static inline SpellDefinition incinerate_rank1() {
        SpellDefinition s;
        s.id = SpellID::INCINERATE;
        s.name = "Incinerate";
        s.school = School::FIRE;
        s.base_cast_time = 2.5; // Reduced by Bane
        s.mana_cost = 355.0;
        s.min_dmg = 445.0;
        s.max_dmg = 515.0;
        s.direct_coefficient = 2.5 / 3.5; // 0.7143
        return s;
    }

    static inline SpellDefinition conflagrate_rank4() {
        SpellDefinition s;
        s.id = SpellID::CONFLAGRATE;
        s.name = "Conflagrate";
        s.school = School::FIRE;
        s.base_cast_time = 0.0;
        s.mana_cost = 265.0;
        s.cooldown = 10.0;
        s.min_dmg = 578.0;
        s.max_dmg = 704.0;
        s.direct_coefficient = 1.5 / 3.5; // 0.4286
        return s;
    }

    static inline SpellDefinition soul_fire_rank5() {
        SpellDefinition s;
        s.id = SpellID::SOUL_FIRE;
        s.name = "Soul Fire";
        s.school = School::FIRE;
        s.base_cast_time = 4.0; // Reduced by Bane to 2.0s; reduced by Decimation
        s.mana_cost = 335.0;
        s.cooldown = 60.0;      // Reduced by Decimation by 90% -> 6.0s
        s.min_dmg = 715.0;
        s.max_dmg = 895.0;
        s.direct_coefficient = 1.0;
        return s;
    }

    static inline SpellDefinition drain_hope_rank1() {
        SpellDefinition s;
        s.id = SpellID::DRAIN_HOPE;
        s.name = "Drain Hope";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 240.0;
        s.cooldown = 20.0;
        s.is_channeled = true;
        s.dot_duration = 6.0;
        s.dot_tick_interval = 1.0;
        s.num_ticks = 6;
        s.dot_base_dmg_per_tick = 52.0;
        s.dot_coeff_per_tick = 0.166;
        return s;
    }
};

inline const char* spell_id_to_name(SpellID id) {
    switch (id) {
        case SpellID::SHADOW_BOLT: return "Shadow Bolt";
        case SpellID::CORRUPTION: return "Corruption";
        case SpellID::CURSE_OF_SHADOWS: return "Curse of Shadows";
        case SpellID::CURSE_OF_ELEMENTS: return "Curse of the Elements";
        case SpellID::CURSE_OF_AGONY: return "Curse of Agony";
        case SpellID::CURSE_OF_DOOM: return "Curse of Doom";
        case SpellID::IMMOLATE: return "Immolate";
        case SpellID::SEARING_PAIN: return "Searing Pain";
        case SpellID::SHADOWBURN: return "Shadowburn";
        case SpellID::CONFLAGRATE: return "Conflagrate";
        case SpellID::INCINERATE: return "Incinerate";
        case SpellID::SOUL_FIRE: return "Soul Fire";
        case SpellID::DRAIN_HOPE: return "Drain Hope";
        case SpellID::LIFE_TAP: return "Life Tap";
        case SpellID::PET_FIREBOLT: return "Firebolt (Pet)";
        case SpellID::PET_LASH_OF_PAIN: return "Lash of Pain (Pet)";
        default: return "Spell";
    }
}

inline const char* spell_id_to_icon(SpellID id) {
    switch (id) {
        case SpellID::SHADOW_BOLT: return "spell_shadow_shadowbolt.png";
        case SpellID::CORRUPTION: return "spell_shadow_abominationexplosion.png";
        case SpellID::CURSE_OF_SHADOWS: return "spell_shadow_curseofachimonde.png";
        case SpellID::CURSE_OF_ELEMENTS: return "spell_shadow_chilltouch.png";
        case SpellID::CURSE_OF_AGONY: return "spell_shadow_curseofsargeras.png";
        case SpellID::CURSE_OF_DOOM: return "spell_shadow_auraofdarkness.png";
        case SpellID::IMMOLATE: return "spell_fire_immolation.png";
        case SpellID::SEARING_PAIN: return "spell_fire_soulburn.png";
        case SpellID::SHADOWBURN: return "spell_shadow_scourgebuild.png";
        case SpellID::CONFLAGRATE: return "spell_fire_fireball.png";
        case SpellID::INCINERATE: return "spell_fire_burnout.png";
        case SpellID::SOUL_FIRE: return "spell_fire_fireball02.png";
        case SpellID::DRAIN_HOPE: return "spell_shadow_haunting.png";
        case SpellID::LIFE_TAP: return "spell_shadow_burningspirit.png";
        case SpellID::PET_FIREBOLT: return "spell_fire_firebolt.png";
        case SpellID::PET_LASH_OF_PAIN: return "spell_shadow_curse.png";
        default: return "spell_shadow_shadowbolt.png";
    }
}

} // namespace warlock
