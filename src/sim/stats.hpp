#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cmath>

namespace warlock {

enum class Race : uint8_t {
    UNDEAD = 0,
    ORC = 1,
    TROLL = 2,
    HUMAN = 3,
    GNOME = 4
};

inline const char* race_to_string(Race r) {
    switch (r) {
        case Race::UNDEAD: return "Undead";
        case Race::ORC: return "Orc";
        case Race::TROLL: return "Troll";
        case Race::HUMAN: return "Human";
        case Race::GNOME: return "Gnome";
        default: return "Undead";
    }
}

inline const char* race_faction(Race r) {
    switch (r) {
        case Race::HUMAN:
        case Race::GNOME:
            return "Alliance";
        default:
            return "Horde";
    }
}

// Small race head icon asset used for the spec-rank table race icon column.
inline const char* race_to_icon(Race r) {
    switch (r) {
        case Race::UNDEAD: return "INV_Misc_Head_Undead_01.png";
        case Race::ORC: return "INV_Misc_Head_Orc_01.png";
        case Race::TROLL: return "INV_Misc_Head_Troll_01.png";
        case Race::HUMAN: return "INV_Misc_Head_Human_01.png";
        case Race::GNOME: return "INV_Misc_Head_Gnome_01.png";
        default: return "INV_Misc_Head_Undead_01.png";
    }
}

// Character baseline attributes (Classic Level 60 Undead/Gnome/Orc/Human/Troll Warlock)
struct BaseAttributes {
    double stamina = 120.0;
    double intellect = 135.0;
    double spirit = 140.0;
    double base_mana = 1393.0;
    double base_health = 1414.0;
    double base_spell_crit = 1.70; // Base warlock spell crit % at 60 (without intellect)
};

inline BaseAttributes get_base_attributes_for_race(Race r) {
    BaseAttributes b;
    switch (r) {
        case Race::UNDEAD:
            b.stamina = 120.0;
            b.intellect = 135.0;
            b.spirit = 140.0;
            b.base_mana = 1393.0;
            b.base_health = 1414.0;
            b.base_spell_crit = 1.70;
            break;
        case Race::ORC:
            b.stamina = 121.0;
            b.intellect = 133.0;
            b.spirit = 138.0;
            b.base_mana = 1393.0;
            b.base_health = 1424.0;
            b.base_spell_crit = 1.70;
            break;
        case Race::TROLL:
            b.stamina = 120.0;
            b.intellect = 132.0;
            b.spirit = 140.0;
            b.base_mana = 1393.0;
            b.base_health = 1414.0;
            b.base_spell_crit = 1.70;
            break;
        case Race::HUMAN:
            b.stamina = 119.0;
            b.intellect = 136.0;
            b.spirit = 147.0; // +5% Spirit from The Human Spirit
            b.base_mana = 1393.0;
            b.base_health = 1404.0;
            b.base_spell_crit = 1.70;
            break;
        case Race::GNOME:
            b.stamina = 118.0;
            b.intellect = 142.0; // High base Int
            b.spirit = 139.0;
            b.base_mana = 1463.0; // +5% Mana from Expansive Mind
            b.base_health = 1394.0;
            b.base_spell_crit = 1.70;
            break;
    }
    return b;
}

// Player combat stats aggregated from base stats, gear, buffs, enchants
struct Stats {
    double stamina = 0.0;
    double intellect = 0.0;
    double spirit = 0.0;

    double max_mana = 0.0;
    double max_health = 0.0;

    double spell_power = 0.0;       // Generic +damage and healing
    double shadow_power = 0.0;      // +Shadow spell damage
    double fire_power = 0.0;        // +Fire spell damage

    double spell_hit_percent = 0.0; // Extra spell hit % from gear/talents/buffs
    double spell_crit_percent = 0.0;// Extra spell crit % from gear/buffs
    double spell_haste_percent = 0.0; // Spell haste % (if enabled)
    double mp5 = 0.0;               // Mana regenerated per 5 seconds

    double shadow_multiplier = 1.0; // Global multiplier for shadow damage (e.g. SM +10%, DS +15%, etc.)
    double fire_multiplier = 1.0;   // Global multiplier for fire damage
    double all_damage_multiplier = 1.0;

    double shadow_crit_bonus_multiplier = 1.5; // Base 1.5x, Ruin talent increases bonus to 2.0x
    double fire_crit_bonus_multiplier = 1.5;

    // Computed effective values
    double effective_shadow_power() const {
        return spell_power + shadow_power;
    }

    double effective_fire_power() const {
        return spell_power + fire_power;
    }

    // Classic Warlock: 60.6 Intellect = 1% Spell Crit
    double total_spell_crit(double base_crit = 1.70) const {
        return base_crit + (intellect / 60.6) + spell_crit_percent;
    }
};

enum class CreatureType : uint8_t {
    HUMANOID = 0,
    BEAST,
    DEMON,
    UNDEAD,
    DRAGONKIN,
    ELEMENTAL,
    GIANT,
    MECHANICAL,
    OTHER
};

inline const char* creature_type_to_string(CreatureType c) {
    switch (c) {
        case CreatureType::HUMANOID: return "Humanoid";
        case CreatureType::BEAST: return "Beast";
        case CreatureType::DEMON: return "Demon";
        case CreatureType::UNDEAD: return "Undead";
        case CreatureType::DRAGONKIN: return "Dragonkin";
        case CreatureType::ELEMENTAL: return "Elemental";
        case CreatureType::GIANT: return "Giant";
        case CreatureType::MECHANICAL: return "Mechanical";
        case CreatureType::OTHER: return "Other";
        default: return "Humanoid";
    }
}

// Target (Boss) characteristics
struct TargetConfig {
    int level = 63;                 // Standard raid boss level (60-63+)
    CreatureType creature_type = CreatureType::HUMANOID;
    double base_shadow_resistance = 24.0; // Boss base innate resistance (cannot be lowered below 0)
    double base_fire_resistance = 24.0;
    double current_shadow_resistance = 24.0;
    double current_fire_resistance = 24.0;
    
    // Active debuffs on target
    bool curse_of_shadows = false;   // -75 Shadow/Arcane res, +10% Shadow/Arcane dmg
    bool curse_of_elements = false;  // -75 Fire/Frost res, +10% Fire/Frost dmg
    bool shadow_weaving = false;     // 5 stacks: +15% Shadow damage from Shadow Priest (Personal only in Forever)
    bool stormstrike = false;
    bool nightfall_axe_proc = false; // Spell damage +15%
    bool is_beast = false;           // Troll racial +5% Beast Slaying bonus
    
    // Improved Shadow Bolt (ISB) tracking on target
    int isb_charges = 0;             // >0 for Classic charges; -1 for Forever chargeless buff window
    double isb_expire_time = 0.0;    // 12s duration
    double isb_bonus = 0.20;         // +20% shadow damage at 5/5
    int total_isb_applications = 0;
    double total_isb_uptime = 0.0;
    double last_isb_update = 0.0;

    void update_isb_uptime(double current_time) {
        if ((isb_charges > 0 || isb_charges == -1) && current_time <= isb_expire_time) {
            double dt = current_time - last_isb_update;
            if (dt > 0.0) total_isb_uptime += dt;
        }
        last_isb_update = current_time;
        if (current_time >= isb_expire_time) {
            isb_charges = 0;
        }
    }

    void apply_isb(double current_time, int points = 5, bool use_charges = false) {
        update_isb_uptime(current_time);
        isb_bonus = points * 0.04;
        isb_charges = use_charges ? 4 : -1;
        isb_expire_time = current_time + 12.0;
        total_isb_applications++;
    }

    bool consume_isb_charge(double current_time) {
        update_isb_uptime(current_time);
        if (current_time <= isb_expire_time) {
            if (isb_charges == -1) {
                // Unlimited attacks within the 12-second window (WoW Forever)
                return true;
            } else if (isb_charges > 0) {
                isb_charges--;
                return true;
            }
        }
        return false;
    }
};

} // namespace warlock
