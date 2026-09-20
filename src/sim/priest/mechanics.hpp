#pragma once
#include <cstdint>
#include <string>
#include "src/sim/common/combat_mechanics.hpp"

namespace priest {

// Priest-specific combat mechanics extending universal WoW combat mechanics
struct MechanicsConfig : public sim::CombatMechanicsConfig {
    // 1. Shadow Weaving
    // In WoW Forever: 3-point talent, 100% chance per shadow damage spell to apply +2% Shadow damage, stacking up to 5 times (+10% total).
    // In Classic 1.12: 5-point talent, 20-100% chance to apply +3% Shadow damage up to 5 stacks (+15% total).
    bool shadow_weaving_personal = true;
    double shadow_weaving_per_stack = 0.02; // 2% per stack in Forever
    int max_shadow_weaving_stacks = 5;

    // 2. Mind Flay Clipping & Tick Scheduling
    // True: Mind Flay channel can be clipped immediately following a damage tick (e.g. at 2.0s after tick 2)
    // to cast Mind Blast or refresh SW:P with minimum latency.
    bool allow_mind_flay_clipping = true;
    double clip_tolerance_window_ms = 100.0;

    // 3. Shadowform
    // Increases Shadow damage by 10%, reduces mana cost of shadow spells by 50%, and increases spell crit bonus to 2.0x (100% bonus).
    bool shadowform_enabled = true;
    double shadowform_damage_bonus = 0.10;
    double shadowform_mana_cost_reduction = 0.50;
    double shadowform_crit_bonus_multiplier = 2.0; // 2.0x crit multiplier

    // 4. Spirit Tap & Meditation Mana Regen
    // Meditation allows 50% spirit regen while casting at 3/3 in Forever.
    double meditation_casting_regen_ratio = 0.50; // At 3/3
    bool spirit_tap_enabled = false;              // Active on mob death (boss encounters usually false unless adds)
    double spirit_tap_spirit_multiplier = 1.00;   // +100% spirit
    double spirit_tap_casting_regen = 0.50;       // 50% casting regen

    // 5. Shadow Word: Death Self-Damage
    // In TBC/Forever: SW:D deals backlash damage to the Priest equal to damage dealt (unless target dies).
    bool sw_death_backlash = false;

    // 6. Vampiric Embrace Healing
    double vampiric_embrace_healing_percent = 0.20; // 20% of shadow spell damage dealt heals party
};

} // namespace priest
