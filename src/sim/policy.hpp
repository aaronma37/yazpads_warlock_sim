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

// Demon summon icon asset used for the spec-rank table pet / sacrifice icon columns.
// Returns "" when there is no demon to depict (none / sacrificed).
inline const char* pet_choice_to_icon(PetChoice p) {
    switch (p) {
        case PetChoice::SUCCUBUS: return "Spell_Shadow_SummonSuccubus.png";
        case PetChoice::IMP: return "Spell_Shadow_SummonImp.png";
        default: return "";
    }
}

enum class RotationChoice : uint8_t {
    SHADOW_DESTRO = 0,      // Shadow Destro: Corruption + Shadowburn + SB filler (no Immolate/Conflag)
    SHADOW_DESTRO_2,      // Shadow Destro: Corruption + Shadowburn + SB filler (no Immolate/Conflag)
    FIRE_DESTRO,            // Fire Destro: Immolate (+25% Incinerate dmg) + Conflagrate + Shadowburn (10% Fire buff) + Incinerate filler
    DP_AF_SHADOW,           // Demonology Shadow: Corruption + Bane of Agony + SB filler
    DP_RUIN_FIRE,           // DP/Ruin Fire: Immolate + Conflagrate + Searing Pain / Decimation Soul Fire (<35% HP)
    DEEP_AFFLICTION,        // Deep Affliction: Corruption + Bane of Agony + Drain Hope (20s CD) + Nightfall procs + SB filler
    SM_RUIN,                // SM/Ruin: Corruption + Bane of Agony + Nightfall + Shadowburn + Shadow Bolt filler
    DEMONOLOGY_EXECUTE,     // Demo Execute: Corruption + Bane of Agony + Decimation Soul Fire (<35% HP execute) + SB filler
    PURE_SHADOW_BOLT,       // Pure Shadow Bolt: SB spam only (0 DoTs, classic 16 debuff limit)
    AFFLICTION_HYBRID_DOTS, // Multi-DoT Hybrid: Agony + Corruption + Immolate + Drain Hope + SB filler
    SHADOW_AND_FLAME_FIRE_2,// Shadow & Flame Fire 2: Immolate + Conflag + Corruption + SBurn + Incinerate (No Searing Pain / Soul Fire)
    DP_AF_SHADOW_NO_CORRUPTION, // Demonology Shadow (No Corruption): Bane of Agony only + SB filler
    FIRE_DESTRO_NO_CORRUPTION,  // Fire Destro (No Corruption): Immolate + Conflagrate + Shadowburn + Incinerate filler, no Corruption
    DP_AF_SHADOW_NO_SOUL_FIRE,        // Demonology Shadow (No Soul Fire): Corruption + Bane of Agony + SB filler, no Decimation Soul Fire
    DP_AF_SHADOW_NO_BANE,             // Demonology Shadow (No Bane): Corruption only + SB filler, no Bane of Agony
    DP_AF_SHADOW_NO_SOUL_FIRE_NO_BANE,// Demonology Shadow (No Soul Fire, No Bane): Corruption only + SB filler

    // Backward compatibility aliases
    SHADOW_BOLT_PRIMARY = SHADOW_DESTRO,
    INCINERATE_FIRE = FIRE_DESTRO
};

inline const char* rotation_choice_to_string(RotationChoice r) {
    switch (r) {
        case RotationChoice::SHADOW_DESTRO: return "Shadow Destro — Conflag Weave + Decimation";
        case RotationChoice::SHADOW_DESTRO_2: return "Shadow Destro — Conflag Weave";
        case RotationChoice::FIRE_DESTRO: return "Fire Destro — Incinerate + Conflag";
        case RotationChoice::DP_AF_SHADOW: return "Demonology Shadow — Corruption + Bane + SB";
        case RotationChoice::DP_RUIN_FIRE: return "Demonology Fire — Searing Pain";
        case RotationChoice::DEEP_AFFLICTION: return "Deep Affliction — Drain Hope";
        case RotationChoice::SM_RUIN: return "Shadow Mastery — DoTs + SB";
        case RotationChoice::DEMONOLOGY_EXECUTE: return "Demo Execute — Decimation Soul Fire";
        case RotationChoice::PURE_SHADOW_BOLT: return "Pure Shadow Bolt — No DoTs";
        case RotationChoice::AFFLICTION_HYBRID_DOTS: return "Affliction Hybrid — Multi-DoT";
        case RotationChoice::SHADOW_AND_FLAME_FIRE_2: return "Shadow & Flame Fire — Incinerate + Conflag";
        case RotationChoice::DP_AF_SHADOW_NO_CORRUPTION: return "Demonology Shadow — Bane + SB";
        case RotationChoice::FIRE_DESTRO_NO_CORRUPTION: return "Fire Destro — Incinerate + Conflag (No Corruption)";
        case RotationChoice::DP_AF_SHADOW_NO_SOUL_FIRE: return "Demonology Shadow — Corruption + Bane + SB (No Soul Fire)";
        case RotationChoice::DP_AF_SHADOW_NO_BANE: return "Demonology Shadow — Corruption + SB (No Bane)";
        case RotationChoice::DP_AF_SHADOW_NO_SOUL_FIRE_NO_BANE: return "Demonology Shadow — Corruption + SB (No Soul Fire, No Bane)";
        default: return "Shadow Destro";
    }
}

inline const char* rotation_choice_description(RotationChoice r) {
    switch (r) {
        case RotationChoice::SHADOW_DESTRO:
            return "5/11/35 Demonic Sacrifice / Agonizing Flames: Sacrifices Imp (+15% Shadow damage), maintains Immolate and Conflagrate for Shadow & Flame (+10% Shadow damage for 20s), casts Shadowburn, and spams Shadow Bolt.";
        case RotationChoice::SHADOW_DESTRO_2:
            return "maintains Immolate and Conflagrate for Shadow & Flame (+10% Shadow damage for 20s), casts Shadowburn, and spams Shadow Bolt.";
        case RotationChoice::FIRE_DESTRO:
            return "Maintains Immolate for +25% Incinerate damage, casts Conflagrate on CD, weaves Shadowburn for +10% Fire buff, and spams Incinerate.";
        case RotationChoice::DP_AF_SHADOW:
            return "Demonic Pact / Agonizing Flames Shadow: Sacrifices Imp (+15% Shadow) and summons Succubus (+10% Shadow from Master Demonologist + 60 SP from Demonic Knowledge). Maintains Corruption and Bane of Agony (CoA), and spams Shadow Bolt.";
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
        case RotationChoice::SHADOW_AND_FLAME_FIRE_2:
            return "Shadow and Flame Fire 2: Maintains Immolate for +25% Incinerate damage, casts Conflagrate on CD, maintains Corruption, weaves Shadowburn for +10% Fire buff, and spams Incinerate as filler with active Imp (no Searing Pain or Soul Fire).";
        case RotationChoice::DP_AF_SHADOW_NO_CORRUPTION:
            return "Demonology Shadow variant that skips Corruption entirely. Maintains only Bane of Agony and spams Shadow Bolt as filler — useful when Corruption's debuff slot is not available or its DPS contribution is outweighed by omitting it.";
        case RotationChoice::FIRE_DESTRO_NO_CORRUPTION:
            return "Fire Destro variant that drops Corruption. Maintains Immolate, casts Conflagrate on CD, weaves Shadowburn for +10% Fire buff, and spams Incinerate — useful when the Corruption debuff slot is not available.";
        case RotationChoice::DP_AF_SHADOW_NO_SOUL_FIRE:
            return "Demonology Shadow variant without Soul Fire execute. Maintains Corruption and Bane of Agony, and spams Shadow Bolt as filler — identical to the base DP/AF Shadow rotation but without the Decimation Soul Fire execute phase.";
        case RotationChoice::DP_AF_SHADOW_NO_BANE:
            return "Demonology Shadow variant without Bane of Agony. Maintains only Corruption and spams Shadow Bolt as filler — useful when the Bane of Agony debuff slot is not available or is better used by another class.";
        case RotationChoice::DP_AF_SHADOW_NO_SOUL_FIRE_NO_BANE:
            return "Demonology Shadow variant without Bane of Agony or Soul Fire execute. Maintains only Corruption and spams Shadow Bolt as filler — a pure single-DoT Shadow Bolt spam for highly constrained debuff environments.";
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
    DECIMATION_SEARING_PAIN,
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

        auto add_decimation = [&](bool include_searing_pain_trigger = true) {
            if (talents.demo.decimation > 0 || use_decimation_soul_fire) {
                if (include_searing_pain_trigger) {
                    rules.push_back({
                        PriorityAction::DECIMATION_SEARING_PAIN,
                        SpellID::SEARING_PAIN,
                        "Decimation Trigger (Searing Pain)",
                        "Target HP < 35% & Decimation Inactive",
                        "Trigger when: Boss is below 35% health AND Decimation 10s buff is inactive, casting Searing Pain to activate Decimation.",
                        "Fast Searing Pain cast on execute to activate Decimation's Soul Fire cast time reduction and 0-shard cost."
                    });
                }
                rules.push_back({
                    PriorityAction::DECIMATION_SOUL_FIRE,
                    SpellID::SOUL_FIRE,
                    "Decimation Soul Fire",
                    "Target HP < 35% & Decimation Active",
                    "Trigger when: Boss is below 35% health AND Decimation buff is active on player AND Soul Fire CD is ready.",
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
            if (curse == CurseChoice::BANE_OF_AGONY || eff == RotationChoice::DEEP_AFFLICTION || eff == RotationChoice::SM_RUIN || eff == RotationChoice::AFFLICTION_HYBRID_DOTS || eff == RotationChoice::DP_AF_SHADOW || eff == RotationChoice::DP_AF_SHADOW_NO_SOUL_FIRE) {
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

            case RotationChoice::DP_AF_SHADOW:
                add_racial();
                // No Searing Pain trigger: the Shadow Bolt filler refreshes
                // the 10s Decimation buff on every cast in execute (<35% HP),
                // so a dedicated trigger cast is redundant. Soul Fire execute
                // is still scheduled below via add_decimation.
                add_decimation(false);
                add_corruption();
                add_agony();
                add_sb_filler();
                break;

            case RotationChoice::DP_AF_SHADOW_NO_CORRUPTION:
                add_racial();
                add_decimation();
                add_agony();
                add_sb_filler();
                break;

            case RotationChoice::FIRE_DESTRO:
                add_racial();
                add_decimation();
                add_immolate();
                add_conflagrate();
                add_corruption();
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

            case RotationChoice::FIRE_DESTRO_NO_CORRUPTION:
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
                        "Trigger when: Primary rotational fallback for Fire Destro (No Corruption).",
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

            case RotationChoice::SHADOW_AND_FLAME_FIRE_2:
                add_racial();
                add_immolate();
                add_conflagrate();
                add_corruption();
                add_shadowburn();
                rules.push_back({
                    PriorityAction::INCINERATE_FILLER,
                    SpellID::INCINERATE,
                    "Incinerate",
                    "Primary Filler",
                    "Trigger when: Primary rotational fallback for Shadow and Flame Fire 2.",
                    "Fast 2.0s Fire filler dealing massive direct damage (+25% bonus damage against Immolated targets)."
                });
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
                add_decimation();
                if (talents.destro.shadow_and_flame > 0 && talents.destro.conflagrate > 0) {
                    add_immolate();
                    add_conflagrate();
                }
                add_nightfall();
                add_corruption();
                add_shadowburn();
                add_sb_filler();
                break;

            case RotationChoice::SHADOW_DESTRO_2:
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

            case RotationChoice::DP_AF_SHADOW_NO_SOUL_FIRE:
                // Like DP_AF_SHADOW but without Decimation Soul Fire execute
                add_racial();
                add_corruption();
                add_agony();
                add_sb_filler();
                break;

            case RotationChoice::DP_AF_SHADOW_NO_BANE:
                // Like DP_AF_SHADOW but without Bane of Agony; uses Decimation Soul Fire
                add_racial();
                add_decimation(false);
                add_corruption();
                add_sb_filler();
                break;

            case RotationChoice::DP_AF_SHADOW_NO_SOUL_FIRE_NO_BANE:
                // Corruption only + SB filler; no Bane of Agony, no Soul Fire execute
                add_racial();
                add_corruption();
                add_sb_filler();
                break;
        }

        return rules;
    }
};

} // namespace warlock
