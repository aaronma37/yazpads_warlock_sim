#pragma once
#include <cstdint>
#include <string>
#include "spells.hpp"

namespace priest {

enum class RotationChoice : uint8_t {
    SHADOW_PRIEST = 0,
    SMITE_PRIEST,
    HOLY_FIRE_WEAVING
};

inline const char* rotation_choice_to_string(RotationChoice r) {
    switch (r) {
        case RotationChoice::SHADOW_PRIEST:       return "Shadow (SW:P -> MB -> MF)";
        case RotationChoice::SMITE_PRIEST:        return "Smite DPS (Holy Fire -> Smite)";
        case RotationChoice::HOLY_FIRE_WEAVING:   return "Holy Fire Weaving";
        default:                                  return "Shadow";
    }
}

struct PolicyConfig {
    RotationChoice rotation = RotationChoice::SHADOW_PRIEST;

    // DoT & Ability toggles
    bool maintain_swp = true;              // Keep Shadow Word: Pain active
    bool cast_mind_blast = true;           // Cast Mind Blast on cooldown
    bool cast_sw_death = true;             // Cast Shadow Word: Death
    bool execute_sw_death_only = false;    // Only cast SW:D under 20% HP (synergizes with Early Demise)
    bool cast_devouring_plague = true;     // Cast Devouring Plague when off CD
    bool cast_vampiric_embrace = false;    // Keep Vampiric Embrace active (costs a GCD)

    // Mind Flay Channel Clipping
    // If true, will clip Mind Flay after Tick 2 if Mind Blast or SW:P refresh is ready
    bool clip_mind_flay_for_mb = true;

    // Cooldowns
    bool use_inner_focus = true;           // Pop Inner Focus on CD (pairs with Mind Blast or Devouring Plague)
    bool use_power_infusion = true;        // Self-cast Power Infusion on CD

    // Consumables & Mana management
    double mana_potion_threshold = 0.35;   // Drink Major Mana Potion below 35% mana
    double demonic_rune_threshold = 0.50;  // Use Demonic Rune below 50% mana
};

} // namespace priest
