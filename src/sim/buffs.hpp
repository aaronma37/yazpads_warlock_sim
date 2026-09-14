#pragma once
#include <string>
#include "stats.hpp"

namespace warlock {

struct BuffConfig {
    // Raid Buffs
    bool arcane_intellect = true;     // +31 Intellect
    bool blessing_of_kings = true;    // +10% all base attributes
    bool blessing_of_wisdom = true;   // +30 MP5
    bool mark_of_the_wild = true;     // +12 all attributes
    bool judgement_of_wisdom = true;  // 50% chance on spell hit to restore 59 mana

    // Consumables
    bool flask_of_supreme_power = true; // +150 Spell Power
    bool greater_arcane_elixir = true;  // +35 Spell Power
    bool elixir_of_shadow_power = true; // +40 Shadow Spell Power
    bool elixir_of_greater_firepower = true; // +40 Fire Spell Power
    bool brilliant_wizard_oil = true;   // +36 Spell Power, +1% Spell Crit
    bool use_mana_potions = true;       // Major Mana Potion (~1800 mana, 120s cd)
    bool use_demonic_runes = true;      // Demonic / Dark Rune (~1200 mana, 120s cd)

    // World Buffs (Default Off)
    bool rallying_cry = false;          // Dragonslayer: +10% Spell Crit
    bool songflower = false;            // +5% Spell Crit, +15 All Attributes
    bool spirit_of_zandalar = false;    // +10% All Attributes
    bool warchiefs_blessing = false;    // +300 HP, +10 MP5
    bool sayges_fortune = false;        // Darkmoon Faire: +10% Damage Dealt

    // Target Debuffs
    bool curse_of_shadows = true;       // +10% Shadow/Arcane damage, -75 Shadow Resistance
    bool curse_of_elements = true;      // +10% Fire/Frost damage, -75 Fire Resistance
    bool shadow_weaving = false;        // Priest: 5 stacks = +15% Shadow damage (Default: OFF - Personal only in Forever)
    bool nightfall_axe = false;         // Spell vulnerability: +15% spell damage

    // Demonic Sacrifice Buffs
    // In WoW Forever: Imp gives +15% Shadow, Succubus gives +15% Fire
    // In Classic 1.12: Succubus gives +15% Shadow, Imp gives +15% Fire
    bool sacrifice_succubus = false;     
    bool sacrifice_imp = true;          

    // Applies static stat and multiplier contributions
    void apply_to_stats(Stats& stats, const BaseAttributes& base, bool wow_forever = true, bool personal_shadow_weaving = true) const {
        // Base attributes modification
        double stat_multiplier = 1.0;
        if (blessing_of_kings) stat_multiplier *= 1.10;
        if (spirit_of_zandalar) stat_multiplier *= 1.10;

        double bonus_int = 0.0;
        double bonus_stam = 0.0;
        double bonus_spr = 0.0;

        if (arcane_intellect) bonus_int += 31.0;
        if (mark_of_the_wild) {
            bonus_int += 12.0;
            bonus_stam += 12.0;
            bonus_spr += 12.0;
        }
        if (songflower) {
            bonus_int += 15.0;
            bonus_stam += 15.0;
            bonus_spr += 15.0;
        }

        stats.intellect = (base.intellect + stats.intellect + bonus_int) * stat_multiplier;
        stats.stamina = (base.stamina + stats.stamina + bonus_stam) * stat_multiplier;
        stats.spirit = (base.spirit + stats.spirit + bonus_spr) * stat_multiplier;

        // Health & Mana pools (1 Int = 15 Mana, 1 Stam = 10 HP)
        stats.max_mana = base.base_mana + stats.intellect * 15.0;
        stats.max_health = base.base_health + stats.stamina * 10.0;

        // Consumables spell power
        if (flask_of_supreme_power) stats.spell_power += 150.0;
        if (greater_arcane_elixir) stats.spell_power += 35.0;
        if (elixir_of_shadow_power) stats.shadow_power += 40.0;
        if (elixir_of_greater_firepower) stats.fire_power += 40.0;
        if (brilliant_wizard_oil) {
            stats.spell_power += 36.0;
            stats.spell_crit_percent += 1.0;
        }

        // World buff crit
        if (rallying_cry) stats.spell_crit_percent += 10.0;
        if (songflower) stats.spell_crit_percent += 5.0;

        // MP5
        if (blessing_of_wisdom) stats.mp5 += 30.0;
        if (warchiefs_blessing) stats.mp5 += 10.0;

        // Damage multipliers
        if (sayges_fortune) stats.all_damage_multiplier *= 1.10;
        if (curse_of_shadows) stats.shadow_multiplier *= 1.10;
        if (curse_of_elements) stats.fire_multiplier *= 1.10;
        if (shadow_weaving && !personal_shadow_weaving) stats.shadow_multiplier *= 1.15;
        if (nightfall_axe) stats.all_damage_multiplier *= 1.15;

        if (wow_forever) {
            if (sacrifice_imp) stats.shadow_multiplier *= 1.15;
            if (sacrifice_succubus) stats.fire_multiplier *= 1.15;
        } else {
            if (sacrifice_succubus) stats.shadow_multiplier *= 1.15;
            if (sacrifice_imp) stats.fire_multiplier *= 1.15;
        }
    }
};

} // namespace warlock
