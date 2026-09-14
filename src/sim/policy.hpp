#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>
#include "spells.hpp"
#include "talents.hpp"
#include "stats.hpp"

namespace warlock {

enum class CurseChoice : uint8_t {
    NONE = 0,
    BANE_OF_AGONY,
    CURSE_OF_DOOM,

    // Backward compatibility aliases
    CURSE_OF_AGONY = BANE_OF_AGONY
};

inline const char* curse_choice_to_string(CurseChoice c) {
    switch (c) {
        case CurseChoice::NONE: return "None";
        case CurseChoice::BANE_OF_AGONY: return "Bane of Agony (CoA)";
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
    SHADOW_DESTRO = 0,      // Shadow Destro: Corruption + Shadowburn + SB filler (no Immolate/Conflag)
    FIRE_DESTRO,            // Fire Destro: Immolate (+25% Incinerate dmg) + Conflagrate + Shadowburn (10% Fire buff) + Incinerate filler
    DP_RUIN_FIRE,           // DP/Ruin Fire: Immolate + Conflagrate + Searing Pain / Decimation Soul Fire (<35% HP)
    DEEP_AFFLICTION,        // Deep Affliction: Corruption + Bane of Agony + Drain Hope (20s CD) + Nightfall procs + SB filler
    SM_RUIN,                // SM/Ruin: Corruption + Bane of Agony + Nightfall + Shadowburn + Shadow Bolt filler
    DEMONOLOGY_EXECUTE,     // Demo Execute: Corruption + Bane of Agony + Decimation Soul Fire (<35% HP execute) + SB filler
    PURE_SHADOW_BOLT,       // Pure Shadow Bolt: SB spam only (0 DoTs, classic 16 debuff limit)
    AFFLICTION_HYBRID_DOTS, // Multi-DoT Hybrid: Agony + Corruption + Immolate + Drain Hope + SB filler

    // Backward compatibility aliases
    SHADOW_BOLT_PRIMARY = SHADOW_DESTRO,
    INCINERATE_FIRE = FIRE_DESTRO
};

inline const char* rotation_choice_to_string(RotationChoice r) {
    switch (r) {
        case RotationChoice::SHADOW_DESTRO: return "5/11/35 DS/AF (Conflag Weave + SB Spam)";
        case RotationChoice::FIRE_DESTRO: return "5/11/35 Fire Destro (Incinerate + Conflagrate)";
        case RotationChoice::DP_RUIN_FIRE: return "0/31/20 DP/AF Fire (Searing Pain + Immolate + Conflag)";
        case RotationChoice::DEEP_AFFLICTION: return "40/11/0 Deep Affliction (DS Imp / Drain Hope)";
        case RotationChoice::SM_RUIN: return "32/0/19 SM/AF (Corruption + CoA + SB Spam)";
        case RotationChoice::DEMONOLOGY_EXECUTE: return "0/31/20 Demo Execute (Decimation Soul Fire + SB)";
        case RotationChoice::PURE_SHADOW_BOLT: return "Pure Shadow Bolt (No DoTs / Classic Limit)";
        case RotationChoice::AFFLICTION_HYBRID_DOTS: return "20/0/31 Multi-DoT Hybrid (Agony + Corr + Immo)";
        default: return "DS/AF";
    }
}

inline const char* rotation_choice_description(RotationChoice r) {
    switch (r) {
        case RotationChoice::SHADOW_DESTRO:
            return "5/11/35 Demonic Sacrifice / Agonizing Flames: Sacrifices Imp (+15% Shadow damage), maintains Immolate and Conflagrate for Shadow & Flame (+10% Shadow damage for 20s), casts Shadowburn, and spams Shadow Bolt.";
        case RotationChoice::FIRE_DESTRO:
            return "Maintains Immolate for +25% Incinerate damage, casts Conflagrate on CD, weaves Shadowburn for +10% Fire buff, and spams Incinerate.";
        case RotationChoice::DP_RUIN_FIRE:
            return "Fire Demonic Pact / Agonizing Flames: Maintains Immolate, casts Conflagrate on CD, weaves Shadowburn, spams Searing Pain (benefiting from Agonizing Flames & Demonic Brand), and feeds pet mana via Demonic Energies.";
        case RotationChoice::DEEP_AFFLICTION:
            return "Maintains Corruption and Bane of Agony (CoA) on top priority for Nightfall procs and Pandemic crits, channels Drain Hope on 20s CD for +10% DoT amplification.";
        case RotationChoice::SM_RUIN:
            return "Shadow Mastery / Agonizing Flames (32/0/19): Maintains Corruption and Bane of Agony (CoA) for Nightfall instant Shadow Bolts and Pandemic crits, casts Shadowburn on cooldown, and spams Shadow Bolt.";
        case RotationChoice::DEMONOLOGY_EXECUTE:
            return "Maintains Corruption and Bane of Agony (CoA) with Shadow Bolt filler until boss reaches <35% HP, then spams rapid Decimation Soul Fires.";
        case RotationChoice::PURE_SHADOW_BOLT:
            return "Strictly spams Shadow Bolt without placing any DoTs (ideal for Classic 16 debuff limit). Raid curses are handled by raid debuffs.";
        case RotationChoice::AFFLICTION_HYBRID_DOTS:
            return "Maintains Bane of Agony, Corruption, and Immolate concurrently for maximum multi-DoT DPS, filling with Drain Hope and Shadow Bolt.";
        default:
            return "";
    }
}

enum class PriorityAction : uint8_t {
    LIFE_TAP = 0,
    RACIAL_EUREKA,
    RACIAL_BLOOD_FURY,
    RACIAL_BERSERKING,
    CURSE_OF_AGONY,
    CURSE_OF_DOOM,
    NIGHTFALL_SHADOW_BOLT,
    DECIMATION_SOUL_FIRE,
    CORRUPTION,
    SIPHON_LIFE,
    DRAIN_HOPE,
    IMMOLATE,
    CONFLAGRATE,
    SHADOWBURN,
    INCINERATE_FILLER,
    SEARING_PAIN_FILLER,
    DRAIN_LIFE_FILLER,
    DRAIN_SOUL_FILLER,
    SHADOW_BOLT_FILLER,

    // Aliases
    BANE_OF_AGONY = CURSE_OF_AGONY
};

struct PriorityRule {
    PriorityAction action = PriorityAction::SHADOW_BOLT_FILLER;
    SpellID spell_id = SpellID::SHADOW_BOLT;
    std::string name;
    std::string condition_summary;
    std::string trigger_condition;
    std::string rule_explanation;
    bool enabled = true;
};

struct PolicyConfig {
    RotationChoice rotation = RotationChoice::SHADOW_DESTRO;
    CurseChoice curse = CurseChoice::BANE_OF_AGONY;
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

    // Constructs the ordered priority rule list for display and execution
    std::vector<PriorityRule> get_priority_rules(const Talents& talents, Race race = Race::UNDEAD) const {
        std::vector<PriorityRule> rules;
        RotationChoice eff = rotation;

        // 1. Life Tap Rule (always top emergency resource)
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Mana <= %.0f%% & HP > 800", life_tap_threshold_pct);
            rules.push_back({
                PriorityAction::LIFE_TAP,
                SpellID::LIFE_TAP,
                "Life Tap",
                buf,
                "Trigger when: Current Mana <= " + std::to_string((int)life_tap_threshold_pct) + "% AND Player Health > 800.",
                "Instantly converts player health into mana on global cooldown to maintain spellcasting resources."
            });
        }

        // Helper lambdas for common rule additions
        auto add_racial = [&]() {
            if (race == Race::GNOME) {
                rules.push_back({
                    PriorityAction::RACIAL_EUREKA,
                    SpellID::RACIAL_EUREKA,
                    "Eureka! (Gnome)",
                    "CD Ready (120s) & Gnome",
                    "Trigger when: Gnome racial cooldown is ready (120s CD).",
                    "Instant off-GCD ability. Grants 3 charges of +10% direct damage bonus to the next 3 direct spell damage casts."
                });
            } else if (race == Race::ORC) {
                rules.push_back({
                    PriorityAction::RACIAL_BLOOD_FURY,
                    SpellID::RACIAL_BLOOD_FURY,
                    "Blood Fury (Orc)",
                    "CD Ready (120s) & Orc",
                    "Trigger when: Orc racial cooldown is ready (120s CD).",
                    "Instant off-GCD ability. Increases spell damage for 15s."
                });
            } else if (race == Race::TROLL) {
                rules.push_back({
                    PriorityAction::RACIAL_BERSERKING,
                    SpellID::RACIAL_BERSERKING,
                    "Berserking (Troll)",
                    "CD Ready (180s) & Troll",
                    "Trigger when: Troll racial cooldown is ready (180s CD).",
                    "Instant off-GCD ability. Increases spell casting haste for 10s."
                });
            }
        };

        auto add_nightfall = [&]() {
            if (cast_nightfall_procs) {
                rules.push_back({
                    PriorityAction::NIGHTFALL_SHADOW_BOLT,
                    SpellID::SHADOW_BOLT,
                    "Nightfall Shadow Bolt",
                    "Shadow Trance Proc Active",
                    "Trigger when: Shadow Trance buff is active (triggered by Corruption ticks).",
                    "Instantly casts Shadow Bolt with 0 cast time."
                });
            }
        };

        auto add_decimation = [&]() {
            if (talents.demo.decimation > 0 || use_decimation_soul_fire) {
                rules.push_back({
                    PriorityAction::DECIMATION_SOUL_FIRE,
                    SpellID::SOUL_FIRE,
                    "Decimation Soul Fire",
                    "Target HP < 35% (Execute)",
                    "Trigger when: Boss is below 35% health AND Decimation proc reduces Soul Fire cast time by 40% with 0 Soul Shards.",
                    "Executes the boss with rapid high-damage Soul Fire casts."
                });
            }
        };

        auto add_corruption = [&]() {
            if (corruption != DotPolicy::NEVER) {
                rules.push_back({
                    PriorityAction::CORRUPTION,
                    SpellID::CORRUPTION,
                    "Corruption",
                    "DoT Expired / Missing",
                    "Trigger when: Corruption DoT is not active on target.",
                    "Maintains ticking Shadow DoT for Nightfall Shadow Trance procs and Pandemic DoT crits."
                });
            }
        };

        auto add_agony = [&]() {
            if (curse == CurseChoice::BANE_OF_AGONY || eff == RotationChoice::DEEP_AFFLICTION || eff == RotationChoice::SM_RUIN || eff == RotationChoice::AFFLICTION_HYBRID_DOTS) {
                rules.push_back({
                    PriorityAction::CURSE_OF_AGONY,
                    SpellID::CURSE_OF_AGONY,
                    "Bane of Agony",
                    "DoT Expired / Missing",
                    "Trigger when: Bane of Agony is not active on target.",
                    "Deals ticking Shadow damage over 24 seconds alongside target curses, benefiting from Pandemic DoT crits."
                });
            } else if (curse == CurseChoice::CURSE_OF_DOOM) {
                rules.push_back({
                    PriorityAction::CURSE_OF_DOOM,
                    SpellID::CURSE_OF_DOOM,
                    "Curse of Doom",
                    "DoT Expired / Missing",
                    "Trigger when: Curse of Doom is not active on target and cooldown is ready (60s).",
                    "Deals massive delayed Shadow damage after 60 seconds."
                });
            }
        };

        auto add_siphon_life = [&]() {
            if (talents.aff.siphon_life > 0) {
                rules.push_back({
                    PriorityAction::SIPHON_LIFE,
                    SpellID::SIPHON_LIFE,
                    "Siphon Life",
                    "DoT Expired / Missing",
                    "Trigger when: Siphon Life is not active on target and talented.",
                    "Maintains 30s ticking Shadow DoT, benefiting from Shadow Mastery, Malediction, and Pandemic crits."
                });
            }
        };

        auto add_drain_hope = [&]() {
            if ((talents.aff.drain_hope > 0 || eff == RotationChoice::DEEP_AFFLICTION || eff == RotationChoice::AFFLICTION_HYBRID_DOTS) && channel_drain_hope) {
                rules.push_back({
                    PriorityAction::DRAIN_HOPE,
                    SpellID::DRAIN_HOPE,
                    "Drain Hope",
                    "CD Ready (20s) & Talented",
                    "Trigger when: Drain Hope cooldown is ready (20s).",
                    "Channels Shadow execute, dealing high ticking damage and amplifying all other Shadow DoTs by +10%."
                });
            }
        };

        auto add_immolate = [&]() {
            rules.push_back({
                PriorityAction::IMMOLATE,
                SpellID::IMMOLATE,
                "Immolate",
                "DoT Expired / Missing",
                "Trigger when: Immolate DoT is not active on target.",
                "Maintains Immolate to enable Conflagrate casts and deal periodic Fire damage."
            });
        };

        auto add_conflagrate = [&]() {
            if (talents.destro.conflagrate > 0 && use_conflagrate) {
                rules.push_back({
                    PriorityAction::CONFLAGRATE,
                    SpellID::CONFLAGRATE,
                    "Conflagrate",
                    "Immolate Active & CD Ready (10s)",
                    "Trigger when: Immolate is active on target AND Conflagrate cooldown is ready (10s).",
                    "Deals instant Fire burst and triggers Shadow & Flame (+10% damage for 20s)."
                });
            }
        };

        auto add_shadowburn = [&]() {
            if (talents.destro.shadowburn > 0 && shadowburn != ShadowburnPolicy::NEVER) {
                rules.push_back({
                    PriorityAction::SHADOWBURN,
                    SpellID::SHADOWBURN,
                    "Shadowburn",
                    shadowburn == ShadowburnPolicy::ON_COOLDOWN ? "CD Ready (8s)" : "Target HP < 20% (Execute)",
                    shadowburn == ShadowburnPolicy::ON_COOLDOWN ? "Trigger when: Shadowburn cooldown is ready (8s)." : "Trigger when: Target HP < 20% AND Shadowburn cooldown is ready (8s).",
                    "Instant Shadow burst damage. Consumes ISB charges and triggers Destruction crit bonuses."
                });
            }
        };

        auto add_drain_life_filler = [&]() {
            rules.push_back({
                PriorityAction::DRAIN_LIFE_FILLER,
                SpellID::DRAIN_LIFE,
                "Drain Life",
                "Primary Filler",
                "Trigger when: No higher priority DoT refresh or Nightfall proc is available.",
                "Channeled Shadow drain benefiting from Improved Drains (+18%), Soul Siphon (+50% tick speed), Pandemic (200% crits), and proccing Nightfall."
            });
        };

        auto add_drain_soul_filler = [&]() {
            rules.push_back({
                PriorityAction::DRAIN_SOUL_FILLER,
                SpellID::DRAIN_SOUL,
                "Drain Soul",
                "Primary Filler",
                "Trigger when: No higher priority DoT refresh or Nightfall proc is available.",
                "Channeled Shadow drain benefiting from Improved Drains (+18%, tripled <20% HP), Soul Siphon (+50% tick speed), Pandemic (200% crits), and proccing Nightfall."
            });
        };

        auto add_sb_filler = [&]() {
            rules.push_back({
                PriorityAction::SHADOW_BOLT_FILLER,
                SpellID::SHADOW_BOLT,
                "Shadow Bolt",
                "Default Filler",
                "Trigger when: No higher priority action conditions are met.",
                "Primary single-target nuke (2.5s cast with Bane). Applies Improved Shadow Bolt (ISB) debuff on crit."
            });
        };

        // Switch based on rotation choice
        switch (eff) {
            case RotationChoice::PURE_SHADOW_BOLT:
                add_racial();
                add_sb_filler();
                break;

            case RotationChoice::FIRE_DESTRO:
                add_racial();
                add_decimation();
                add_immolate();
                add_conflagrate();
                add_shadowburn();
                if (talents.destro.incinerate > 0) {
                    rules.push_back({
                        PriorityAction::INCINERATE_FILLER,
                        SpellID::INCINERATE,
                        "Incinerate",
                        "Primary Filler",
                        "Trigger when: Primary rotational fallback for Fire Destro.",
                        "Fast 2.0s Fire filler dealing massive direct damage (+25% bonus damage against Immolated targets)."
                    });
                } else {
                    rules.push_back({
                        PriorityAction::SEARING_PAIN_FILLER,
                        SpellID::SEARING_PAIN,
                        "Searing Pain",
                        "Primary Filler",
                        "Trigger when: Fire filler spell.",
                        "Fast 1.5s Fire filler spell."
                    });
                }
                break;

            case RotationChoice::DP_RUIN_FIRE:
                add_racial();
                add_immolate();
                add_conflagrate();
                rules.push_back({
                    PriorityAction::SEARING_PAIN_FILLER,
                    SpellID::SEARING_PAIN,
                    "Searing Pain",
                    "Primary Filler",
                    "Trigger when: Primary rotational fallback for DP/AF Fire.",
                    "Fast 1.5s Fire filler benefiting from Agonizing Flames and Ruin crit damage multipliers."
                });
                break;

            case RotationChoice::SHADOW_DESTRO:
                add_racial();
                if (talents.destro.shadow_and_flame > 0 && talents.destro.conflagrate > 0) {
                    add_immolate();
                    add_conflagrate();
                }
                add_nightfall();
                add_corruption();
                add_shadowburn();
                add_sb_filler();
                break;

            case RotationChoice::DEEP_AFFLICTION:
                // Corruption, CoA, and Siphon Life higher priority than Drain Hope
                add_racial();
                add_nightfall();      // Cast Shadow Bolt ONLY on Nightfall procs
                add_corruption();
                add_agony();
                add_siphon_life();
                add_drain_hope();
                add_drain_soul_filler(); // Drain Soul as filler
                break;

            case RotationChoice::SM_RUIN:
                // Corruption and CoA; no Immolate
                add_racial();
                add_nightfall();
                add_corruption();
                add_agony();
                add_shadowburn();
                add_sb_filler();
                break;

            case RotationChoice::DEMONOLOGY_EXECUTE:
                add_racial();
                add_decimation();
                add_nightfall();
                add_corruption();
                add_agony();
                add_sb_filler();
                break;

            case RotationChoice::AFFLICTION_HYBRID_DOTS:
                add_racial();
                add_nightfall();
                add_corruption();
                add_agony();
                add_siphon_life();
                add_immolate();
                add_drain_hope();
                add_shadowburn();
                add_sb_filler();
                break;
        }

        return rules;
    }
};

} // namespace warlock
