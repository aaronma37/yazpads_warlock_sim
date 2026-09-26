#pragma once
#include <cstdint>
#include <string>
#include <ostream>

namespace warlock {

enum class SpellID : uint8_t {
    NONE = 0,
    SHADOW_BOLT = 1,
    CORRUPTION,
    CURSE_OF_SHADOWS,
    CURSE_OF_ELEMENTS,
    CURSE_OF_AGONY,
    CURSE_OF_DOOM,
    BANE_OF_HAVOC,
    IMMOLATE,
    SEARING_PAIN,
    SHADOWBURN,
    CONFLAGRATE,
    INCINERATE,
    SOUL_FIRE,
    DRAIN_HOPE,
    DRAIN_LIFE,
    DRAIN_SOUL,
    SIPHON_LIFE,
    LIFE_TAP,
    DEATH_COIL,
    PET_FIREBOLT,
    PET_LASH_OF_PAIN,
    PET_MELEE, // Succubus melee swings (stats key, never cast through the queue)
    POTION_MANA,
    DEMONIC_RUNE,
    TRINKET_USE,
    RACIAL_EUREKA,
    RACIAL_BLOOD_FURY,
    RACIAL_BERSERKING,
    AMPLIFY_CURSE,
    TOUCH_OF_THE_GRAVE,
    DEMONIC_BRAND,

    COUNT // Number of spell ids; must stay last (sizes per-spell stat arrays)
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
        s.min_dmg = 253.0;
        s.max_dmg = 283.0;
        s.direct_coefficient = 3.0 / 3.5; // ~0.8571
        return s;
    }

    static inline SpellDefinition corruption_rank7() {
        SpellDefinition s;
        s.id = SpellID::CORRUPTION;
        s.name = "Corruption";
        s.school = School::SHADOW;
        s.base_cast_time = 2.0; // Reduced to 0 by Imp Corruption 5/5
        s.mana_cost = 340.0;
        s.is_dot = true;
        s.dot_duration = 18.0;
        s.dot_tick_interval = 3.0;
        s.num_ticks = 6;
        s.dot_base_dmg_per_tick = 73.0;        // 438.0 total (73 every 3s)
        s.dot_coeff_per_tick = 0.20;          // 20% per tick (120% total coefficient)
        return s;
    }

    static inline SpellDefinition curse_of_agony_rank6() {
        SpellDefinition s;
        s.id = SpellID::CURSE_OF_AGONY;
        s.name = "Bane of Agony";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 215.0;
        s.is_dot = true;
        s.dot_duration = 24.0;
        s.dot_tick_interval = 2.0;
        s.num_ticks = 12;
        s.dot_base_dmg_per_tick = 552.0 / 12.0; // 46.0 base avg per tick (552 total)
        s.dot_coeff_per_tick = 1.596 / 12.0;    // 13.3% per tick (159.6% total coefficient)
        return s;
    }

    static inline SpellDefinition curse_of_doom_rank1() {
        SpellDefinition s;
        s.id = SpellID::CURSE_OF_DOOM;
        s.name = "Bane of Doom";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 300.0;
        s.cooldown = 60.0;
        s.is_dot = true;
        s.dot_duration = 60.0;
        s.dot_tick_interval = 60.0;
        s.num_ticks = 1;
        s.dot_base_dmg_per_tick = 1742.0;       // 1,742 base damage after 60s
        s.dot_coeff_per_tick = 4.0;            // 400% coefficient
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

    static inline SpellDefinition bane_of_havoc() {
        SpellDefinition s;
        s.id = SpellID::BANE_OF_HAVOC;
        s.name = "Bane of Havoc";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 150.0;
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
        s.min_dmg = 158.0;
        s.max_dmg = 158.0;
        s.direct_coefficient = 0.20; // 20% direct
        s.is_dot = true;
        s.dot_duration = 15.0;
        s.dot_tick_interval = 3.0;
        s.num_ticks = 5;
        s.dot_base_dmg_per_tick = 55.0; // 55 every 3s (275 DoT + 158 initial = 433 total)
        s.dot_coeff_per_tick = 0.13;    // 13% per tick (65% total DoT -> 85% full duration)
        return s;
    }

    static inline SpellDefinition searing_pain_rank6() {
        SpellDefinition s;
        s.id = SpellID::SEARING_PAIN;
        s.name = "Searing Pain";
        s.school = School::FIRE;
        s.base_cast_time = 1.5;
        s.mana_cost = 168.0;
        s.min_dmg = 108.0;
        s.max_dmg = 127.0;
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
        s.cooldown = 15.0;
        s.min_dmg = 259.0;
        s.max_dmg = 289.0;
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
        s.min_dmg = 430.0; // Health cost: 430
        s.max_dmg = 430.0; // Base Mana return (430 + Spirit)
        s.direct_coefficient = 0.0; // Scales 100% with Spirit
        return s;
    }

    static inline SpellDefinition incinerate_rank1() {
        SpellDefinition s;
        s.id = SpellID::INCINERATE;
        s.name = "Incinerate";
        s.school = School::FIRE;
        s.base_cast_time = 2.5; // Reduced by Bane
        s.mana_cost = 325.0;
        s.min_dmg = 201.0;
        s.max_dmg = 233.0;
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
        s.min_dmg = 306.0;
        s.max_dmg = 374.0;
        s.direct_coefficient = 1.5 / 3.5; // 0.4286
        return s;
    }

    static inline SpellDefinition soul_fire_rank5() {
        SpellDefinition s;
        s.id = SpellID::SOUL_FIRE;
        s.name = "Soul Fire";
        s.school = School::FIRE;
        s.base_cast_time = 6.0; // 4.0s with 5/5 Bane; reduced further by Decimation
        s.mana_cost = 335.0;
        s.cooldown = 60.0;      // Reduced by Decimation by 90% -> 6.0s
        s.min_dmg = 383.0;
        s.max_dmg = 479.0;
        s.direct_coefficient = 1.0;
        return s;
    }

    static inline SpellDefinition wrack_rank3() {
        SpellDefinition s;
        s.id = SpellID::DRAIN_HOPE;
        s.name = "Wrack";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 240.0;
        s.cooldown = 0.0;
        s.is_channeled = false;
        s.is_dot = true;
        s.dot_duration = 6.0;
        s.dot_tick_interval = 1.0;
        s.num_ticks = 6;
        s.dot_base_dmg_per_tick = 216.0 / 6.0; // 36.0
        s.dot_coeff_per_tick = 0.858 / 6.0;    // 0.143 (85.8% total coefficient)
        return s;
    }
    static inline SpellDefinition drain_hope_rank3() { return wrack_rank3(); }
    static inline SpellDefinition drain_hope_rank1() { return wrack_rank3(); }
    static inline SpellDefinition drain_life_rank6() {
        SpellDefinition s;
        s.id = SpellID::DRAIN_LIFE;
        s.name = "Drain Life";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 300.0;
        s.is_channeled = true;
        s.dot_duration = 5.0;
        s.dot_tick_interval = 1.0;
        s.num_ticks = 5;
        s.dot_base_dmg_per_tick = 51.0; // 255 base across 5 sec (5 ticks of 51)
        s.dot_coeff_per_tick = 0.10;   // 50% total SP coefficient (10% per tick)
        return s;
    }
    static inline SpellDefinition drain_soul_rank4() {
        SpellDefinition s;
        s.id = SpellID::DRAIN_SOUL;
        s.name = "Drain Soul";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 290.0;
        s.is_channeled = true;
        s.dot_duration = 15.0;
        s.dot_tick_interval = 3.0;
        s.num_ticks = 5;
        s.dot_base_dmg_per_tick = 84.0; // 420 base across 15 sec (5 ticks of 84)
        s.dot_coeff_per_tick = 0.10;   // 50% total SP coefficient (10% per tick)
        return s;
    }

    static inline SpellDefinition siphon_life_rank4() {
        SpellDefinition s;
        s.id = SpellID::SIPHON_LIFE;
        s.name = "Siphon Life";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 365.0;
        s.is_dot = true;
        s.dot_duration = 30.0;
        s.dot_tick_interval = 3.0;
        s.num_ticks = 10;
        s.dot_base_dmg_per_tick = 41.0; // 410.0 base across 30 sec (10 ticks of 41)
        s.dot_coeff_per_tick = 0.05;   // 5% per tick (50% total SP coefficient)
        return s;
    }

    static inline SpellDefinition amplify_curse() {
        SpellDefinition s;
        s.id = SpellID::AMPLIFY_CURSE;
        s.name = "Amplify Curse";
        s.school = School::SHADOW;
        s.base_cast_time = 0.0;
        s.mana_cost = 0.0;
        s.cooldown = 180.0;
        return s;
    }
};

inline const char* spell_id_to_name(SpellID id) {
    switch (id) {
        case SpellID::SHADOW_BOLT: return "Shadow Bolt";
        case SpellID::CORRUPTION: return "Corruption";
        case SpellID::CURSE_OF_SHADOWS: return "Curse of Shadows";
        case SpellID::CURSE_OF_ELEMENTS: return "Curse of the Elements";
        case SpellID::CURSE_OF_AGONY: return "Bane of Agony";
        case SpellID::CURSE_OF_DOOM: return "Bane of Doom";
        case SpellID::BANE_OF_HAVOC: return "Bane of Havoc";
        case SpellID::IMMOLATE: return "Immolate";
        case SpellID::SEARING_PAIN: return "Searing Pain";
        case SpellID::SHADOWBURN: return "Shadowburn";
        case SpellID::CONFLAGRATE: return "Conflagrate";
        case SpellID::INCINERATE: return "Incinerate";
        case SpellID::SOUL_FIRE: return "Soul Fire";
        case SpellID::DRAIN_HOPE: return "Wrack";
        case SpellID::DRAIN_LIFE: return "Drain Life";
        case SpellID::DRAIN_SOUL: return "Drain Soul";
        case SpellID::SIPHON_LIFE: return "Siphon Life";
        case SpellID::LIFE_TAP: return "Life Tap";
        case SpellID::PET_FIREBOLT: return "Firebolt (Pet)";
        case SpellID::PET_LASH_OF_PAIN: return "Lash of Pain (Pet)";
        case SpellID::PET_MELEE: return "Melee (Pet)";
        case SpellID::RACIAL_EUREKA: return "Eureka!";
        case SpellID::RACIAL_BLOOD_FURY: return "Blood Fury";
        case SpellID::RACIAL_BERSERKING: return "Berserking";
        case SpellID::AMPLIFY_CURSE: return "Amplify Curse";
        case SpellID::TOUCH_OF_THE_GRAVE: return "Touch of the Grave";
        case SpellID::DEMONIC_BRAND: return "Demonic Brand";
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
        case SpellID::BANE_OF_HAVOC: return "ability_warlock_baneofhavoc.png";
        case SpellID::IMMOLATE: return "spell_fire_immolation.png";
        case SpellID::SEARING_PAIN: return "spell_fire_soulburn.png";
        case SpellID::SHADOWBURN: return "spell_shadow_scourgebuild.png";
        case SpellID::CONFLAGRATE: return "spell_fire_fireball.png";
        case SpellID::INCINERATE: return "spell_fire_burnout.png";
        case SpellID::SOUL_FIRE: return "spell_fire_fireball02.png";
        case SpellID::DRAIN_HOPE: return "ability_deathknight_hemorrhagicfever.png";
        case SpellID::DRAIN_LIFE: return "spell_shadow_lifedrain02.png";
        case SpellID::DRAIN_SOUL: return "spell_shadow_soulgem.png";
        case SpellID::SIPHON_LIFE: return "spell_shadow_requiem.png";
        case SpellID::LIFE_TAP: return "spell_shadow_burningspirit.png";
        case SpellID::PET_FIREBOLT: return "spell_fire_firebolt.png";
        case SpellID::PET_LASH_OF_PAIN: return "spell_shadow_curse.png";
        case SpellID::PET_MELEE: return "Ability_MeleeDamage.png";
        case SpellID::RACIAL_EUREKA: return "spell_nature_astralrecalgroup.png";
        case SpellID::RACIAL_BLOOD_FURY: return "racial_orc_berserkerstrength.png";
        case SpellID::RACIAL_BERSERKING: return "racial_troll_berserk.png";
        case SpellID::AMPLIFY_CURSE: return "spell_shadow_contagion.png";
        case SpellID::TOUCH_OF_THE_GRAVE: return "spell_shadow_chilltouch.png";
        case SpellID::DEMONIC_BRAND: return "ability_demonhunter_chaoticimprint_fire.png";
        default: return "spell_shadow_shadowbolt.png";
    }
}

inline std::ostream& operator<<(std::ostream& os, SpellID id) {
    return os << spell_id_to_name(id);
}

} // namespace warlock
