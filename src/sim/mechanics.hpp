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

    // 6. Improved Shadow Bolt (ISB)
    // In WoW Forever: ISB is a pure 12-second debuff window (chargeless).
    // In Classic 1.12: 4 charges consumed by damaging shadow attacks.
    bool isb_has_charges = false; // Default: 12-second timed window (chargeless)
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

    // 11. Drain Hope Spell Type
    // True: Drain Hope (Wrack) is an instant cast 6-second DoT (1.5s GCD floor), allowing filler casts during its duration.
    // False: Drain Hope is a channeled spell (player is locked channeling for its duration).
    bool instant_drain_hope = false;

    // 12. Corruption Spell Power Coefficient
    // Default 1.0 (100% total SP over 6 ticks / 18s). Custom toggle/setting allows 1.2 (120% total SP).
    double corruption_sp_coefficient = 1.0;

    // 13. Pet Stat Scaling & Mana Modeling
    // True: Pets inherit master's Spell Power (15% to pet spell damage, 57% to pet Attack Power).
    // False: Static base damage (Classic 1.12).
    bool pet_scaling = true; // Default: ON (WoW Forever)
    double pet_sp_ratio = 0.15; // 15% SP inheritance to pet spell damage
    double pet_ap_ratio = 0.57; // 57% SP inheritance to pet Attack Power (melee)

    // Pet Mana Management
    // When enabled, Imp and Succubus track mana pools, cast costs, and regen.
    bool pet_mana_management = true;
    double imp_base_mana = 1150.0;
    double succubus_base_mana = 1450.0;
    double imp_firebolt_cost = 115.0;     // Rank 7 Firebolt
    double succubus_lop_cost = 160.0;     // Rank 6 Lash of Pain
    double pet_base_mp5 = 45.0;           // Base passive pet regen per 5s

    // 14. Imp Firebolt Scaling Version
    // True: Modern/Custom Scaling (Default: 57.1% (2.0/3.5) of Master Fire SP, 2.0s cast interval).
    // False: Classic 1.12 Scaling (85-98 flat base damage + 15% pet SP inheritance at 1.5/3.5 coeff, 1.5s cast interval).
    bool imp_firebolt_modern_scaling = true;
};

} // namespace warlock
