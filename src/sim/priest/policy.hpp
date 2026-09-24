#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "spells.hpp"
#include "talents.hpp"
#include "src/sim/common/stats.hpp"

namespace priest {

enum class RotationChoice : uint8_t {
    SHADOW_PRIEST = 0,
    SHADOW_NO_MB,
    SHADOW_SWP_ONLY,
    SMITE_PRIEST,
    PURE_SMITE,
    HOLY_FIRE_WEAVING,
    DISC_INQUISITOR,
    COUNT
};

inline const char* rotation_choice_to_string(RotationChoice r) {
    switch (r) {
        case RotationChoice::SHADOW_PRIEST:       return "Shadow (SW:P -> MB -> MF)";
        case RotationChoice::SHADOW_NO_MB:        return "Shadow - No Mind Blast (SW:P -> MF)";
        case RotationChoice::SHADOW_SWP_ONLY:     return "Shadow - SW:P & Flay (Mana Conserve)";
        case RotationChoice::SMITE_PRIEST:        return "Smite DPS (Holy Fire -> Smite)";
        case RotationChoice::PURE_SMITE:          return "Pure Smite (Smite Spam + Penance)";
        case RotationChoice::HOLY_FIRE_WEAVING:   return "Holy Fire Weaving (Holy Fire -> SW:P -> Smite)";
        case RotationChoice::DISC_INQUISITOR:     return "Inquisitor (Holy Fire -> SW:P -> SW:D -> Smite)";
        default:                                  return "Shadow";
    }
}

inline const char* rotation_choice_description(RotationChoice r) {
    switch (r) {
        case RotationChoice::SHADOW_PRIEST:
            return "Standard Shadow Priest priority: DP, SW:P, Mind Blast on CD, SW:Death, and Mind Flay filler.";
        case RotationChoice::SHADOW_NO_MB:
            return "Threat- and mana-reducing Shadow priority: skips Mind Blast, maintaining DP, SW:P, and SW:Death with Mind Flay filler.";
        case RotationChoice::SHADOW_SWP_ONLY:
            return "Maximum mana conservation: maintains only SW:P with execute SW:D and Mind Flay filler, skipping expensive DP and MB.";
        case RotationChoice::SMITE_PRIEST:
            return "Holy/Disc Smite DPS: maintains Holy Fire for Power in Light (+10% damage), Penance on CD, and Smite filler.";
        case RotationChoice::PURE_SMITE:
            return "Direct Holy burst: skips Holy Fire's long cast time, focusing on Penance on CD and rapid Smite spam.";
        case RotationChoice::HOLY_FIRE_WEAVING:
            return "Hybrid Holy-Shadow DoT weaving: maintains both Holy Fire and SW:P, with Penance and Smite filler.";
        case RotationChoice::DISC_INQUISITOR:
            return "Aggressive Discipline/Inquisitor burst: weaves Holy Fire, SW:P, Penance, and instant Shadow Word: Death into Smite filler.";
        default:
            return "";
    }
}

struct PriorityRule {
    SpellID spell_id = SpellID::MIND_FLAY;
    std::string name;
    std::string condition_summary;
    std::string trigger_condition;
    bool enabled = true;
};

struct PolicyConfig {
    RotationChoice rotation = RotationChoice::SHADOW_PRIEST;

    // DoT & Ability toggles
    bool maintain_swp = true;              // Keep Shadow Word: Pain active
    bool cast_mind_blast = true;           // Cast Mind Blast on cooldown
    bool cast_sw_death = true;             // Cast Shadow Word: Death
    bool execute_sw_death_only = false;    // Only cast SW:D under 20% HP (synergizes with Early Demise)
    bool cast_devouring_plague = true;     // Cast Devouring Plague when off CD
    bool cast_vampiric_embrace = false;    // Keep Vampiric Embrace active (costs a GCD)

    // Holy / Smite / Disc toggles
    bool cast_holy_fire = true;            // Maintain Holy Fire for Power in Light (+10% Smite/Penance)
    bool cast_penance = true;              // Cast Penance on cooldown if talented
    bool cast_holy_nova_on_proc = true;    // Cast instant free Holy Nova when Clearcasting procs

    // Racial ability toggles
    bool cast_starshards = true;           // Cast Starshards on CD (Night Elf)
    bool cast_chastise = true;             // Cast Chastise on CD (Dwarf)
    bool use_dark_sacrifice = true;        // Use Dark Sacrifice when mana < 60% (Undead)
    bool use_berserking = true;            // Pop Berserking on CD (Troll)

    // Mind Flay Channel Clipping
    // If true, will clip Mind Flay after Tick 2 if Mind Blast or SW:P refresh is ready
    bool clip_mind_flay_for_mb = true;

    // Cooldowns
    bool use_inner_focus = true;           // Pop Inner Focus on CD (pairs with Mind Blast or Devouring Plague)
    bool use_power_infusion = true;        // Self-cast Power Infusion on CD

    // Consumables & Mana management
    double mana_potion_threshold = 0.35;   // Drink Major Mana Potion below 35% mana
    double demonic_rune_threshold = 0.50;  // Use Demonic Rune below 50% mana

    // Dynamic & Custom APL properties
    bool use_custom_apl = false;
    std::vector<PriorityRule> custom_rules;

    // Constructs the ordered priority rule list for display and execution
    std::vector<PriorityRule> get_priority_rules(const Talents& talents, sim::Race race = sim::Race::HUMAN) const;
    std::vector<PriorityRule> build_preset_rules(const Talents& talents, sim::Race race = sim::Race::HUMAN) const;

    void enable_custom_apl(const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        if (custom_rules.empty()) {
            custom_rules = build_preset_rules(talents, race);
        }
        use_custom_apl = true;
    }

    void reset_to_preset(const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        custom_rules.clear();
        use_custom_apl = false;
    }

    size_t rule_count(const Talents& talents, sim::Race race = sim::Race::HUMAN) const {
        return get_priority_rules(talents, race).size();
    }

    bool move_rule_up(size_t index, const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        enable_custom_apl(talents, race);
        if (index > 0 && index < custom_rules.size()) {
            std::swap(custom_rules[index], custom_rules[index - 1]);
            return true;
        }
        return false;
    }

    bool move_rule_down(size_t index, const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        enable_custom_apl(talents, race);
        if (index + 1 < custom_rules.size()) {
            std::swap(custom_rules[index], custom_rules[index + 1]);
            return true;
        }
        return false;
    }

    bool swap_rules(size_t i, size_t j, const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        enable_custom_apl(talents, race);
        if (i < custom_rules.size() && j < custom_rules.size()) {
            std::swap(custom_rules[i], custom_rules[j]);
            return true;
        }
        return false;
    }

    bool set_rule_enabled(size_t index, bool enabled, const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        enable_custom_apl(talents, race);
        if (index < custom_rules.size()) {
            custom_rules[index].enabled = enabled;
            return true;
        }
        return false;
    }

    bool set_rule(size_t index, const PriorityRule& rule, const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        enable_custom_apl(talents, race);
        if (index < custom_rules.size()) {
            custom_rules[index] = rule;
            return true;
        }
        return false;
    }

    bool insert_rule(size_t index, const PriorityRule& rule, const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        enable_custom_apl(talents, race);
        if (index <= custom_rules.size()) {
            custom_rules.insert(custom_rules.begin() + index, rule);
            return true;
        }
        return false;
    }

    bool remove_rule(size_t index, const Talents& talents, sim::Race race = sim::Race::HUMAN) {
        enable_custom_apl(talents, race);
        if (index < custom_rules.size()) {
            custom_rules.erase(custom_rules.begin() + index);
            return true;
        }
        return false;
    }
};

inline std::vector<PriorityRule> PolicyConfig::get_priority_rules(const Talents& talents, sim::Race race) const {
    if (use_custom_apl && !custom_rules.empty()) {
        return custom_rules;
    }
    return build_preset_rules(talents, race);
}

inline std::vector<PriorityRule> PolicyConfig::build_preset_rules(const Talents& talents, sim::Race race) const {
    std::vector<PriorityRule> rules;

    // 1. Off-GCD Cooldowns & Emergency Mana
    if (talents.disc.inner_focus > 0 && use_inner_focus) {
        rules.push_back({SpellID::INNER_FOCUS, "Inner Focus", "On Cooldown (3m)", "Next spell costs 0 mana and +25% crit chance", true});
    }
    if (talents.disc.power_infusion > 0 && use_power_infusion) {
        rules.push_back({SpellID::POWER_INFUSION, "Power Infusion", "On Cooldown (3m)", "Infuses self for +20% spell damage & healing for 15s", true});
    }
    if (race == sim::Race::UNDEAD && use_dark_sacrifice) {
        rules.push_back({SpellID::DARK_SACRIFICE, "Dark Sacrifice", "Mana <= 60%", "Cannibalize / sacrifice target for 1600 mana", true});
    }
    if (race == sim::Race::TROLL && use_berserking) {
        rules.push_back({SpellID::RACIAL_BERSERKING, "Berserking", "On Cooldown (3m)", "Increases casting speed by 10% for 10s", true});
    }

    // 2. Rotational Spells by Rotation Choice
    if (rotation == RotationChoice::SHADOW_PRIEST) {
        // 1. Devouring Plague (1 min CD, 24s DoT)
        if (cast_devouring_plague) {
            rules.push_back({SpellID::DEVOURING_PLAGUE, "Devouring Plague", "On Cooldown (1 min)", "Afflicts target with 848 Shadow damage over 24s and heals caster", true});
        }
        // 2. Shadow Word: Pain maintenance
        if (maintain_swp) {
            rules.push_back({SpellID::SHADOW_WORD_PAIN, "Shadow Word: Pain", "DoT Expired / Refresh", "Maintains 100% uptime on Shadow Word: Pain DoT", true});
        }
        // 3. Mind Blast on cooldown
        if (cast_mind_blast) {
            rules.push_back({SpellID::MIND_BLAST, "Mind Blast", "On Cooldown (5.5s-8s)", "High burst Shadow damage on short cooldown", true});
        }
        // 4. Shadow Word: Death
        if (cast_sw_death) {
            if (execute_sw_death_only) {
                rules.push_back({SpellID::SHADOW_WORD_DEATH, "Shadow Word: Death", "Target < 20% HP", "Execute burst; benefits from Early Demise +30% crit", true});
            } else {
                rules.push_back({SpellID::SHADOW_WORD_DEATH, "Shadow Word: Death", "On Cooldown (15s)", "Instant shadow nuke (10% max HP backlash self-damage)", true});
            }
        }
        // 5. Vampiric Embrace (if selected)
        if (talents.shadow.vampiric_embrace > 0 && cast_vampiric_embrace) {
            rules.push_back({SpellID::VAMPIRIC_EMBRACE, "Vampiric Embrace", "DoT Expired (30s)", "Afflicts target: 20% of shadow spell damage heals party", true});
        }
        // 6. Starshards (Night Elf Racial)
        if (race == sim::Race::NIGHT_ELF && cast_starshards) {
            rules.push_back({SpellID::STARSHARDS, "Starshards", "On Cooldown (30s)", "Night Elf racial channel dealing Arcane damage over 6s", true});
        }
        // 7. Mind Flay / Smite Filler
        if (talents.shadow.mind_flay > 0) {
            std::string mf_desc = clip_mind_flay_for_mb
                ? "3.0s channeled Shadow beam; clipped after tick 2 if Mind Blast / SW:P is ready"
                : "3.0s channeled Shadow beam (channels full 3 ticks)";
            rules.push_back({SpellID::MIND_FLAY, "Mind Flay", "Primary Filler", mf_desc, true});
        } else {
            rules.push_back({SpellID::SMITE, "Smite", "Primary Filler", "2.0s - 2.5s Holy cast filler", true});
        }
    } else if (rotation == RotationChoice::SHADOW_NO_MB) {
        // Shadow - No Mind Blast (Threat / Mana Conserve)
        if (cast_devouring_plague) {
            rules.push_back({SpellID::DEVOURING_PLAGUE, "Devouring Plague", "On Cooldown (1 min)", "Afflicts target with 848 Shadow damage over 24s and heals caster", true});
        }
        if (maintain_swp) {
            rules.push_back({SpellID::SHADOW_WORD_PAIN, "Shadow Word: Pain", "DoT Expired / Refresh", "Maintains 100% uptime on Shadow Word: Pain DoT", true});
        }
        if (cast_sw_death) {
            if (execute_sw_death_only) {
                rules.push_back({SpellID::SHADOW_WORD_DEATH, "Shadow Word: Death", "Target < 20% HP", "Execute burst; benefits from Early Demise +30% crit", true});
            } else {
                rules.push_back({SpellID::SHADOW_WORD_DEATH, "Shadow Word: Death", "On Cooldown (15s)", "Instant shadow nuke (10% max HP backlash self-damage)", true});
            }
        }
        if (talents.shadow.vampiric_embrace > 0 && cast_vampiric_embrace) {
            rules.push_back({SpellID::VAMPIRIC_EMBRACE, "Vampiric Embrace", "DoT Expired (30s)", "Afflicts target: 20% of shadow spell damage heals party", true});
        }
        if (race == sim::Race::NIGHT_ELF && cast_starshards) {
            rules.push_back({SpellID::STARSHARDS, "Starshards", "On Cooldown (30s)", "Night Elf racial channel dealing Arcane damage over 6s", true});
        }
        if (talents.shadow.mind_flay > 0) {
            std::string mf_desc = clip_mind_flay_for_mb
                ? "3.0s channeled Shadow beam; clipped after tick 2 if SW:P refresh is ready"
                : "3.0s channeled Shadow beam (channels full 3 ticks)";
            rules.push_back({SpellID::MIND_FLAY, "Mind Flay", "Primary Filler", mf_desc, true});
        } else {
            rules.push_back({SpellID::SMITE, "Smite", "Primary Filler", "2.0s - 2.5s Holy cast filler", true});
        }
    } else if (rotation == RotationChoice::SHADOW_SWP_ONLY) {
        // Shadow - SW:P & Mind Flay (Maximum Mana Conservation)
        if (maintain_swp) {
            rules.push_back({SpellID::SHADOW_WORD_PAIN, "Shadow Word: Pain", "DoT Expired / Refresh", "Maintains 100% uptime on Shadow Word: Pain DoT", true});
        }
        if (cast_sw_death) {
            rules.push_back({SpellID::SHADOW_WORD_DEATH, "Shadow Word: Death", "Target < 20% HP", "High-efficiency execute burst under 20% HP", true});
        }
        if (race == sim::Race::NIGHT_ELF && cast_starshards) {
            rules.push_back({SpellID::STARSHARDS, "Starshards", "On Cooldown (30s)", "Night Elf racial channel dealing Arcane damage over 6s", true});
        }
        if (talents.shadow.mind_flay > 0) {
            rules.push_back({SpellID::MIND_FLAY, "Mind Flay", "Primary Filler", "3.0s channeled Shadow beam (unclipped 3 ticks for FSR regen)", true});
        } else {
            rules.push_back({SpellID::SMITE, "Smite", "Primary Filler", "2.0s - 2.5s Holy cast filler", true});
        }
    } else if (rotation == RotationChoice::SMITE_PRIEST) {
        // 1. Chastise (Dwarf Racial)
        if (race == sim::Race::DWARF && cast_chastise) {
            rules.push_back({SpellID::CHASTISE, "Chastise", "On Cooldown (2 min)", "Dwarf racial instant Holy damage", true});
        }
        // 2. Clearcast Holy Nova
        if (cast_holy_nova_on_proc) {
            rules.push_back({SpellID::HOLY_NOVA, "Holy Nova", "Clearcast Proc", "Instant free cast when Clearcasting triggers from Holy Fire", true});
        }
        // 3. Holy Fire maintenance
        if (cast_holy_fire) {
            rules.push_back({SpellID::HOLY_FIRE, "Holy Fire", "Maintain DoT (10s)", "Empowers Smite & Penance (+10% damage from Power in Light)", true});
        }
        // 4. Penance on cooldown
        if (talents.disc.penance > 0 && cast_penance) {
            rules.push_back({SpellID::PENANCE, "Penance", "On Cooldown (10s)", "3-pulse Holy volley channel; benefits from Power in Light", true});
        }
        // 5. Smite filler
        rules.push_back({SpellID::SMITE, "Smite", "Primary Filler", "2.0s Holy cast (reduced by Divine Fury)", true});
    } else if (rotation == RotationChoice::PURE_SMITE) {
        // Pure Smite (Skips Holy Fire cast time)
        if (race == sim::Race::DWARF && cast_chastise) {
            rules.push_back({SpellID::CHASTISE, "Chastise", "On Cooldown (2 min)", "Dwarf racial instant Holy damage", true});
        }
        if (cast_holy_nova_on_proc) {
            rules.push_back({SpellID::HOLY_NOVA, "Holy Nova", "Clearcast Proc", "Instant free cast when Clearcasting triggers", true});
        }
        if (talents.disc.penance > 0 && cast_penance) {
            rules.push_back({SpellID::PENANCE, "Penance", "On Cooldown (10s)", "3-pulse Holy volley channel", true});
        }
        rules.push_back({SpellID::SMITE, "Smite", "Primary Filler", "2.0s Holy cast (reduced by Divine Fury)", true});
    } else if (rotation == RotationChoice::HOLY_FIRE_WEAVING) {
        // Holy Fire + SW:P Weaving
        if (race == sim::Race::DWARF && cast_chastise) {
            rules.push_back({SpellID::CHASTISE, "Chastise", "On Cooldown (2 min)", "Dwarf racial instant Holy damage", true});
        }
        if (cast_holy_nova_on_proc) {
            rules.push_back({SpellID::HOLY_NOVA, "Holy Nova", "Clearcast Proc", "Instant free cast when Clearcasting triggers from Holy Fire", true});
        }
        if (cast_holy_fire) {
            rules.push_back({SpellID::HOLY_FIRE, "Holy Fire", "Maintain DoT (10s)", "Empowers Smite & Penance (+10% damage from Power in Light)", true});
        }
        if (maintain_swp) {
            rules.push_back({SpellID::SHADOW_WORD_PAIN, "Shadow Word: Pain", "DoT Expired / Refresh", "Maintains 100% uptime on Shadow Word: Pain DoT", true});
        }
        if (talents.disc.penance > 0 && cast_penance) {
            rules.push_back({SpellID::PENANCE, "Penance", "On Cooldown (10s)", "3-pulse Holy volley channel; benefits from Power in Light", true});
        }
        rules.push_back({SpellID::SMITE, "Smite", "Primary Filler", "2.0s Holy cast (reduced by Divine Fury)", true});
    } else if (rotation == RotationChoice::DISC_INQUISITOR) {
        // Inquisitor Hybrid (Holy Fire + SW:P + Penance + SW:Death + Smite)
        if (race == sim::Race::DWARF && cast_chastise) {
            rules.push_back({SpellID::CHASTISE, "Chastise", "On Cooldown (2 min)", "Dwarf racial instant Holy damage", true});
        }
        if (cast_holy_nova_on_proc) {
            rules.push_back({SpellID::HOLY_NOVA, "Holy Nova", "Clearcast Proc", "Instant free cast when Clearcasting triggers from Holy Fire", true});
        }
        if (cast_holy_fire) {
            rules.push_back({SpellID::HOLY_FIRE, "Holy Fire", "Maintain DoT (10s)", "Empowers Smite & Penance (+10% damage from Power in Light)", true});
        }
        if (maintain_swp) {
            rules.push_back({SpellID::SHADOW_WORD_PAIN, "Shadow Word: Pain", "DoT Expired / Refresh", "Maintains 100% uptime on Shadow Word: Pain DoT", true});
        }
        if (talents.disc.penance > 0 && cast_penance) {
            rules.push_back({SpellID::PENANCE, "Penance", "On Cooldown (10s)", "3-pulse Holy volley channel; benefits from Power in Light", true});
        }
        if (cast_sw_death) {
            if (execute_sw_death_only) {
                rules.push_back({SpellID::SHADOW_WORD_DEATH, "Shadow Word: Death", "Target < 20% HP", "Instant shadow execute burst", true});
            } else {
                rules.push_back({SpellID::SHADOW_WORD_DEATH, "Shadow Word: Death", "On Cooldown (15s)", "Instant shadow burst nuke on cooldown", true});
            }
        }
        rules.push_back({SpellID::SMITE, "Smite", "Primary Filler", "2.0s Holy cast (reduced by Divine Fury)", true});
    }

    return rules;
}

} // namespace priest
