#pragma once
#include <cstdint>
#include <string>
#include "src/sim/common/combat_mechanics.hpp"

namespace warlock {

// Warlock toggleable mechanics configuration
// Extends common combat mechanics with warlock-specific mechanics (ISB, Nightfall, Pets, etc.)
struct MechanicsConfig : public sim::CombatMechanicsConfig {
    // 6. Improved Shadow Bolt (ISB)
    // In WoW Forever: ISB is a pure 12-second debuff window (chargeless).
    // In Classic 1.12: 4 charges consumed by damaging shadow attacks.
    bool isb_has_charges = false; // Default: 12-second timed window (chargeless)
    bool isb_all_shadow_sources = false;

    // 8. Nightfall / Shadow Trance
    // 4% chance per Corruption tick to grant Shadow Trance (instant Shadow Bolt).
    bool nightfall_enabled = true;
    double nightfall_proc_chance = 0.04;

    // 11. Wrack Spell Type
    // True: Wrack is an instant cast 6-second DoT (1.5s GCD floor), allowing filler casts during its duration.
    // False: Wrack is a channeled spell (player is locked channeling for its duration).
    bool instant_drain_hope = false;

    // 12. Corruption Spell Power Coefficient
    // Default 1.0 (100% total SP over 6 ticks / 18s). Custom toggle/setting allows 1.2 (120% total SP).
    double corruption_sp_coefficient = 1.0;

    // 13. Pet Stat Scaling & Mana Modeling
    // True: Pets inherit master's Spell Power (10 SP = 1 Pet SP -> 10.0%, 6 SP = 1 Pet AP -> ~16.67%).
    // False: Static base damage (Classic 1.12).
    bool pet_scaling = true; // Default: ON (WoW Forever)
    double pet_sp_ratio = 0.10; // 10 SP = 1 Pet SP (10.0% SP inheritance to pet spell damage)
    double pet_ap_ratio = 1.0 / 6.0; // 6 SP = 1 Pet AP (~16.67% SP inheritance to pet Attack Power)

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
