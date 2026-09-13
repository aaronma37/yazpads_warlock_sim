#pragma once
#include <cstdint>
#include <string>

namespace warlock {

enum class CurseChoice : uint8_t {
    NONE = 0,
    CURSE_OF_SHADOWS,
    CURSE_OF_ELEMENTS,
    CURSE_OF_AGONY,
    CURSE_OF_DOOM
};

inline const char* curse_choice_to_string(CurseChoice c) {
    switch (c) {
        case CurseChoice::NONE: return "None";
        case CurseChoice::CURSE_OF_SHADOWS: return "Curse of Shadows";
        case CurseChoice::CURSE_OF_ELEMENTS: return "Curse of the Elements";
        case CurseChoice::CURSE_OF_AGONY: return "Curse of Agony";
        case CurseChoice::CURSE_OF_DOOM: return "Curse of Doom";
        default: return "Unknown";
    }
}

enum class DotPolicy : uint8_t {
    NEVER = 0,
    ALWAYS,
    ONLY_WITH_DEBUFF_SLOT
};

enum class ShadowburnPolicy : uint8_t {
    NEVER = 0,
    ON_COOLDOWN,
    EXECUTE_ONLY // Last 20% of fight
};

enum class PetChoice : uint8_t {
    NONE = 0,
    SUCCUBUS,
    IMP
};

inline const char* pet_choice_to_string(PetChoice p) {
    switch (p) {
        case PetChoice::SUCCUBUS: return "Succubus (Melee + Lash of Pain)";
        case PetChoice::IMP: return "Imp (Firebolt)";
        default: return "None / Sacrificed";
    }
}

enum class RotationChoice : uint8_t {
    AUTO = 0,               // Automatically pick best spell based on spec (Incinerate for Fire, SB for Shadow)
    SHADOW_BOLT_PRIMARY,    // Shadow Bolt filler
    INCINERATE_FIRE,        // Incinerate filler
    DEEP_AFFLICTION         // DoT prioritization + Drain Hope + Drains
};

inline const char* rotation_choice_to_string(RotationChoice r) {
    switch (r) {
        case RotationChoice::SHADOW_BOLT_PRIMARY: return "Shadow Bolt Primary";
        case RotationChoice::INCINERATE_FIRE: return "Incinerate (Fire Destro)";
        case RotationChoice::DEEP_AFFLICTION: return "Deep Affliction (Drain Hope)";
        default: return "Auto (Talent Adaptive)";
    }
}

struct PolicyConfig {
    RotationChoice rotation = RotationChoice::AUTO;
    CurseChoice curse = CurseChoice::CURSE_OF_SHADOWS;
    DotPolicy corruption = DotPolicy::ALWAYS;
    bool maintain_immolate = true;            // Maintain Immolate for Conflag/Incinerate bonuses
    ShadowburnPolicy shadowburn = ShadowburnPolicy::ON_COOLDOWN;
    PetChoice pet = PetChoice::SUCCUBUS;      // Active demon when not sacrificed

    double life_tap_threshold_pct = 25.0;     // Life Tap if mana drops below this %
    bool use_trinkets_on_cooldown = true;
    bool cast_nightfall_procs = true;

    // WoW Forever rotational abilities
    bool use_conflagrate = true;              // Cast Conflagrate on cooldown
    bool use_incinerate = true;               // Use Incinerate when talented
    bool use_decimation_soul_fire = true;     // Execute phase Soul Fire procs (<35% HP)
    bool channel_drain_hope = true;           // Channel Drain Hope on cooldown if talented
};

} // namespace warlock
