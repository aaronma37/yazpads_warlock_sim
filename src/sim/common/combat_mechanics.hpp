#pragma once
#include <cstdint>

namespace sim {

// Universal combat mechanics configuration shared across all classes
struct CombatMechanicsConfig {
    // 1. Snapshotting
    // True: DoTs snapshot spell power and % damage modifiers at cast time for their entire duration (Classic WoW).
    // False: DoTs dynamically recalculate damage on every tick based on active buffs/debuffs (WoW Forever / Modern).
    bool snapshot_dots = false;

    // 2. Spell Batching
    // If enabled, events occurring within the batch window (e.g. 400ms) resolve at the next batch boundary.
    bool spell_batching = false;
    double batch_window_ms = 400.0;

    // 3. Raid Debuff Limit
    // 0 = Infinite / Unlimited debuff slots (Default in Forever / Modern).
    // 16 = 1.12 Classic 16-slot debuff limit.
    int debuff_limit = 0;
    bool enforce_debuff_slots = false; // Default: Infinite Debuff Slots

    // 4. Personal Shadow Weaving
    // True: Shadow Weaving debuff (+15% Shadow) is personal to the Shadow Priest that applied it (Warlocks do NOT benefit).
    // False: Raid-wide shared Shadow Weaving (Classic WoW 1.12).
    bool personal_shadow_weaving = true; // Default: Personal Only (Warlocks do not benefit)

    // 5. Resistance & Partial Resists
    // True: Uses Classic 4-roll partial resist table (0%, 25%, 50%, 75%, 100% resist).
    // False: Binary resist only (miss or full hit).
    bool partial_resists_enabled = true;

    // 5b. Spell Piercing (Penetration) below 0
    // In WoW Forever: Spell Piercing reduces target resistance below 0 into negative resistance,
    // which amplifies spell damage (~0.55% - 0.60% per point of negative resistance).
    bool spell_piercing_below_zero = true;
    double spell_piercing_bonus_per_point = 0.00575; // Default: +0.575% damage per piercing point below 0


    // 6. Spell Hit Cap & Base Hit
    // Level 60 vs Level 63 boss: Base hit chance is 83% (17% miss chance, 0% miss floor in Forever Beta, so 100% hit cap with 17% hit from gear/talents).
    double base_hit_vs_boss = 0.83; // 83%
    double max_spell_hit = 1.00;    // 100% hit cap (0% minimum miss chance floor in Forever Beta)

    // 7. Critical Strike Damage Multiplier
    // Base spell crit damage is 1.5x in Classic.
    double base_spell_crit_multiplier = 1.50;

    // 8. Spell Travel Time / Projectile Speed
    // Missiles have a physical travel speed (~24 yards/sec).
    bool projectile_travel_time = true;
    double default_boss_distance_yards = 30.0;
    double projectile_speed_yards_per_sec = 24.0;

    // 9. Global Cooldown (GCD)
    double base_gcd = 1.5;
    bool haste_affects_gcd = false; // In classic, haste does not affect 1.5s GCD floor; toggleable for modern.
};

} // namespace sim

namespace warlock {
    using sim::CombatMechanicsConfig;
}
