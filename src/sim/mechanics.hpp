#pragma once
#include <cstdint>
#include <string>

namespace warlock {

// Toggleable mechanics configuration
// Allows simulating pure 1.12 Classic WoW as well as custom/future iterations
struct MechanicsConfig {
    // 1. Snapshotting
    // True: DoTs snapshot spell power and % damage modifiers at cast time for their entire duration (Classic WoW).
    // False: DoTs dynamically recalculate damage on every tick based on active buffs/debuffs (WoW Forever / Modern).
    bool snapshot_dots = false;

    // 2. Spell Batching
    // If enabled, events occurring within the batch window (e.g. 400ms) resolve at the next batch boundary.
    bool spell_batching = false;
    double batch_window_ms = 400.0;

    // 3. Raid Debuff Limit
    // Classic vanilla: 8 slots early on, 16 slots in 1.12.
    // Set to 0 for unlimited debuff slots.
    int debuff_limit = 16;
    bool enforce_debuff_slots = true;

    // 4. Resistance & Partial Resists
    // True: Uses Classic 4-roll partial resist table (0%, 25%, 50%, 75%, 100% resist).
    // False: Binary resist only (miss or full hit).
    bool partial_resists_enabled = true;

    // 5. Improved Shadow Bolt (ISB) consumption rules
    // isb_has_charges: True = 4 charges consumed by attacks (Classic WoW 1.12).
    //                  False = 12-second timed window without charges (WoW Forever).
    bool isb_has_charges = false;
    bool isb_all_shadow_sources = false;

    // 6. Spell Hit Cap & Base Hit
    // Level 60 vs Level 63 boss: Base hit chance is 83% (17% miss chance, 1% always misses, so 84% hit cap, 16% hit needed from gear/talents).
    double base_hit_vs_boss = 0.83; // 83%
    double max_spell_hit = 0.99;    // 1% minimum miss chance always remains in Classic

    // 7. Critical Strike Damage Multiplier
    // Base spell crit damage is 1.5x in Classic. Ruin talent increases the critical strike damage bonus by 100% (so bonus is 100% * 2 = 100%, total 2.0x).
    double base_spell_crit_multiplier = 1.50;

    // 8. Nightfall / Shadow Trance
    // 4% chance per Corruption tick to grant Shadow Trance (instant Shadow Bolt).
    bool nightfall_enabled = true;
    double nightfall_proc_chance = 0.04;

    // 9. Spell Travel Time / Projectile Speed
    // Classic Shadow Bolt has a physical missile speed (~24 yards/sec).
    // At 30yd max range: ~1.25s projectile travel time.
    bool projectile_travel_time = true;
    double default_boss_distance_yards = 30.0;
    double projectile_speed_yards_per_sec = 24.0;

    // 10. Global Cooldown (GCD)
    double base_gcd = 1.5;
    bool haste_affects_gcd = false; // In classic, haste does not affect 1.5s GCD floor; toggleable for modern.
};

} // namespace warlock
