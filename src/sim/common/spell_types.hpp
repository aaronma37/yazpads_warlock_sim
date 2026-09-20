#pragma once
#include <cstdint>
#include <string>

namespace sim {

enum class School : uint8_t {
    SHADOW = 0,
    FIRE,
    PHYSICAL,
    HOLY,
    FROST,
    ARCANE,
    NATURE
};

inline const char* school_to_string(School s) {
    switch (s) {
        case School::SHADOW: return "Shadow";
        case School::FIRE: return "Fire";
        case School::PHYSICAL: return "Physical";
        case School::HOLY: return "Holy";
        case School::FROST: return "Frost";
        case School::ARCANE: return "Arcane";
        case School::NATURE: return "Nature";
        default: return "Unknown";
    }
}

// Common definition of a spell's baseline attributes
struct SpellDefinition {
    uint8_t id = 0;
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

// Per-spell combat counters for the damage breakdown views.
// casts = completed casts / DoT applications / channel casts / pet casts & swings.
// hits = damaging impacts + ticks (ticks count as hits). damage sums final damage.
struct SpellCombatStats {
    int casts = 0;
    int hits = 0;
    int crits = 0;
    int misses = 0;
    double damage = 0.0;
};

// Per-spell fight averages for batch damage breakdown views.
struct BatchSpellStats {
    double mean_casts = 0.0;
    double mean_hits = 0.0; // damaging impacts + ticks
    double mean_crits = 0.0;
    double mean_misses = 0.0;
    double mean_damage = 0.0;
};

inline double spell_crit_pct(const BatchSpellStats& s) {
    return s.mean_hits > 0.0 ? (s.mean_crits / s.mean_hits) * 100.0 : 0.0;
}

inline double spell_miss_pct(const BatchSpellStats& s) {
    return s.mean_casts > 0.0 ? (s.mean_misses / s.mean_casts) * 100.0 : 0.0;
}

inline double spell_avg_hit(const BatchSpellStats& s) {
    return s.mean_hits > 0.0 ? s.mean_damage / s.mean_hits : 0.0;
}

} // namespace sim
