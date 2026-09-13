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
    AUTO = 0,               // Automatically pick best rotation based on active talents
    SHADOW_DESTRO,          // Shadow Destro: Immolate + Conflagrate (Shadow & Flame +10% Shadow) + Shadowburn + SB filler
    FIRE_DESTRO,            // Fire Destro: Immolate (+25% Incinerate dmg) + Conflagrate + Shadowburn (10% Fire buff) + Incinerate filler
    DEEP_AFFLICTION,        // Deep Affliction: Agony + Corruption + Drain Hope (20s CD) + Nightfall procs + SB filler
    SM_RUIN,                // SM / Ruin: Corruption (Nightfall + Pandemic) + Shadowburn + Shadow Bolt filler
    DEMONOLOGY_EXECUTE,     // Demo Execute: Curse + Corruption + Decimation Soul Fire (<35% HP execute) + SB filler
    PURE_SHADOW_BOLT,       // Pure Shadow Bolt: Curse + SB spam only (0 DoTs, classic 16 debuff limit)
    AFFLICTION_HYBRID_DOTS, // Multi-DoT Hybrid: Agony + Corruption + Immolate + Drain Hope + SB filler

    // Backward compatibility aliases
    SHADOW_BOLT_PRIMARY = SHADOW_DESTRO,
    INCINERATE_FIRE = FIRE_DESTRO
};

inline const char* rotation_choice_to_string(RotationChoice r) {
    switch (r) {
        case RotationChoice::AUTO: return "Auto (Talent Adaptive)";
        case RotationChoice::SHADOW_DESTRO: return "Shadow Destro (Conflagrate + Shadow & Flame)";
        case RotationChoice::FIRE_DESTRO: return "Fire Destro (Incinerate + Conflagrate)";
        case RotationChoice::DEEP_AFFLICTION: return "Deep Affliction (Drain Hope + Multi-DoT)";
        case RotationChoice::SM_RUIN: return "SM / Ruin (Corruption + SB Spam)";
        case RotationChoice::DEMONOLOGY_EXECUTE: return "Demo Execute (Decimation Soul Fire + SB)";
        case RotationChoice::PURE_SHADOW_BOLT: return "Pure Shadow Bolt (No DoTs / Classic Limit)";
        case RotationChoice::AFFLICTION_HYBRID_DOTS: return "Multi-DoT Hybrid (Agony + Corr + Immo)";
        default: return "Auto (Talent Adaptive)";
    }
}

inline const char* rotation_choice_description(RotationChoice r) {
    switch (r) {
        case RotationChoice::AUTO:
            return "Analyzes active talent points and automatically selects the highest-synergy rotation.";
        case RotationChoice::SHADOW_DESTRO:
            return "Maintains Immolate for Conflagrate to trigger Shadow & Flame (+10% Shadow damage for 20s), casting Shadowburn and Shadow Bolt.";
        case RotationChoice::FIRE_DESTRO:
            return "Maintains Immolate for +25% Incinerate damage, casts Conflagrate on CD, weaves Shadowburn for +10% Fire buff, and spams Incinerate.";
        case RotationChoice::DEEP_AFFLICTION:
            return "Maintains Curse of Agony/Doom and Corruption for Nightfall procs and Pandemic crits, channels Drain Hope on 20s CD for +10% DoT amplification.";
        case RotationChoice::SM_RUIN:
            return "Maintains Corruption for Nightfall instant Shadow Bolts and Pandemic crits, casts Shadowburn on cooldown, and spams Shadow Bolt.";
        case RotationChoice::DEMONOLOGY_EXECUTE:
            return "Maintains Curse/Corruption with Shadow Bolt filler until boss reaches <35% HP, then spams rapid Decimation Soul Fires.";
        case RotationChoice::PURE_SHADOW_BOLT:
            return "Maintains Curse of Shadows and strictly spams Shadow Bolt without placing any DoTs (ideal for Classic 16 debuff limit).";
        case RotationChoice::AFFLICTION_HYBRID_DOTS:
            return "Maintains Curse of Agony, Corruption, and Immolate concurrently for maximum multi-DoT DPS, filling with Drain Hope and Shadow Bolt.";
        default:
            return "";
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
