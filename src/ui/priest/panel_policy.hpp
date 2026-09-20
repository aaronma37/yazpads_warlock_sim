#pragma once
#include "imgui.h"
#include "src/sim/priest/policy.hpp"
#include "src/sim/priest/talents.hpp"
#include "src/sim/common/player_class.hpp"
#include "src/ui/common/panel_policy.hpp"
#include <vector>

namespace priest {

inline std::vector<warlock::CommonPriorityRule> get_priest_priority_rules(
    const PolicyConfig& policy,
    const Talents& talents,
    sim::Race race)
{
    std::vector<warlock::CommonPriorityRule> rules;

    // Cooldowns
    if (policy.use_inner_focus) {
        rules.push_back({
            "spell_frost_windwalkon",
            "Inner Focus",
            "On Cooldown",
            "Off Cooldown",
            "Next spell costs 0 mana and has +25% critical strike chance.",
            IM_COL32(25, 65, 85, 240),
            IM_COL32(80, 200, 240, 230)
        });
    }

    if (policy.use_power_infusion) {
        rules.push_back({
            "spell_holy_powerinfusion",
            "Power Infusion",
            "On Cooldown",
            "Off Cooldown",
            "Infuses self with power, granting +20% spell damage and healing for 15s.",
            IM_COL32(89, 64, 20, 240),
            IM_COL32(255, 204, 51, 230)
        });
    }

    // Racial Abilities
    if (race == sim::Race::UNDEAD) {
        rules.push_back({
            "spell_holy_powerinfusion_shadow",
            "Dark Sacrifice",
            "Low Mana",
            "Mana < 40%",
            "Cannibalizes 1600 HP over 15s to restore 1600 Mana.",
            IM_COL32(89, 30, 30, 240),
            IM_COL32(230, 76, 76, 230)
        });
    } else if (race == sim::Race::NIGHT_ELF) {
        rules.push_back({
            "spell_arcane_starfire",
            "Starshards",
            "On Cooldown (30s)",
            "Off Cooldown & Target Active",
            "Rains 1,800 Arcane damage over 6s on target.",
            IM_COL32(30, 50, 90, 240),
            IM_COL32(80, 160, 240, 230)
        });
    } else if (race == sim::Race::TROLL) {
        rules.push_back({
            "racial_troll_berserk",
            "Berserking",
            "On Cooldown / Burst",
            "Off Cooldown",
            "Increases casting speed by 10% for 10s.",
            IM_COL32(25, 65, 85, 240),
            IM_COL32(80, 200, 240, 230)
        });
    }

    if (policy.rotation == RotationChoice::SHADOW_PRIEST) {
        if (policy.cast_vampiric_embrace) {
            rules.push_back({
                "spell_shadow_unsummonbuilding",
                "Vampiric Embrace",
                "DoT Expired",
                "Target Missing VE",
                "Afflicts target: 20% of shadow spell damage dealt heals party for 30s.",
                IM_COL32(65, 30, 85, 240),
                IM_COL32(190, 90, 230, 230)
            });
        }

        if (policy.maintain_swp) {
            rules.push_back({
                "spell_shadow_shadowwordpain",
                "Shadow Word: Pain",
                "DoT Missing / Refresh",
                "Remaining Duration <= 0.0s",
                "Maintains permanent 18s Shadow Word: Pain DoT (120% SP coefficient in Forever).",
                IM_COL32(56, 46, 76, 240),
                IM_COL32(140, 90, 204, 230)
            });
        }

        if (policy.cast_devouring_plague) {
            rules.push_back({
                "spell_shadow_devouringplague",
                "Devouring Plague",
                "On Cooldown (1 min)",
                "Off Cooldown & DoT Expired",
                "Afflicts target with 848 Shadow DoT over 24s and heals caster.",
                IM_COL32(45, 60, 40, 240),
                IM_COL32(90, 190, 80, 230)
            });
        }

        if (policy.cast_mind_blast) {
            rules.push_back({
                "spell_shadow_unholyfrenzy",
                "Mind Blast",
                "On Cooldown (5.5s talented)",
                "Off Cooldown",
                "Hits target for 472 - 498 base Shadow damage with high burst scaling.",
                IM_COL32(70, 35, 80, 240),
                IM_COL32(180, 80, 220, 230)
            });
        }

        if (policy.cast_sw_death) {
            rules.push_back({
                "spell_shadow_demonicfortitude",
                "Shadow Word: Death",
                policy.execute_sw_death_only ? "Execute (<20% HP)" : "On Cooldown (15s)",
                policy.execute_sw_death_only ? "Target HP <= 20%" : "Off Cooldown",
                "Inflicts 434 - 462 instant Shadow damage (+30% crit during execute).",
                IM_COL32(89, 64, 20, 240),
                IM_COL32(255, 204, 51, 230)
            });
        }

        rules.push_back({
            "spell_shadow_siphonmana",
            "Mind Flay",
            "Filler Channel",
            "Always Available",
            "Channels 3s Shadow beam for 390 base damage (50% SP coefficient in Forever).",
            IM_COL32(50, 40, 70, 240),
            IM_COL32(120, 90, 180, 230)
        });
    } else if (policy.rotation == RotationChoice::SMITE_PRIEST) {
        rules.push_back({
            "spell_holy_searinglight",
            "Holy Fire",
            "On Cooldown / DoT",
            "DoT Expired",
            "Hits for 184 - 232 Holy damage + 75 DoT over 10s. Procs free Holy Nova.",
            IM_COL32(82, 46, 25, 240),
            IM_COL32(255, 140, 51, 230)
        });

        rules.push_back({
            "spell_holy_chastise",
            "Chastise",
            "On Cooldown",
            "Off Cooldown",
            "Instant 272 - 306 Holy damage and 2s immobilize.",
            IM_COL32(75, 65, 25, 240),
            IM_COL32(230, 200, 60, 230)
        });

        rules.push_back({
            "spell_holy_holysmite",
            "Smite",
            "Filler Cast (2.0s talented)",
            "Always Available",
            "Primary Holy damage nuke (71.4% SP coefficient). Deals +10% vs Holy Fire target.",
            IM_COL32(80, 70, 30, 240),
            IM_COL32(240, 210, 80, 230)
        });
    } else { // HOLY_FIRE_WEAVING
        rules.push_back({
            "spell_holy_searinglight",
            "Holy Fire",
            "Maintain DoT",
            "DoT Expired",
            "Maintains Holy Fire 10s DoT on target.",
            IM_COL32(82, 46, 25, 240),
            IM_COL32(255, 140, 51, 230)
        });

        rules.push_back({
            "spell_shadow_shadowwordpain",
            "Shadow Word: Pain",
            "Maintain DoT",
            "Remaining Duration <= 0.0s",
            "Maintains Shadow Word: Pain DoT.",
            IM_COL32(56, 46, 76, 240),
            IM_COL32(140, 90, 204, 230)
        });

        rules.push_back({
            "spell_holy_holysmite",
            "Smite",
            "Filler Cast",
            "Always Available",
            "Smite filler between DoT refreshes.",
            IM_COL32(80, 70, 30, 240),
            IM_COL32(240, 210, 80, 230)
        });
    }

    // Mana Consumables
    if (policy.mana_potion_threshold > 0.0) {
        rules.push_back({
            "inv_potion_76",
            "Major Mana Potion",
            "Mana Below Threshold",
            "Player Mana < Threshold",
            "Restores 1350 - 2250 Mana (120s cooldown).",
            IM_COL32(25, 55, 80, 240),
            IM_COL32(70, 160, 230, 230)
        });
    }

    if (policy.demonic_rune_threshold > 0.0) {
        rules.push_back({
            "inv_misc_gem_pearl_03",
            "Demonic / Dark Rune",
            "Mana Below Threshold",
            "Player Mana < Threshold",
            "Restores 900 - 1500 Mana at cost of 600 - 1000 HP (120s cooldown).",
            IM_COL32(50, 30, 60, 240),
            IM_COL32(150, 80, 180, 230)
        });
    }

    return rules;
}

inline void render_priest_policy_panel(PolicyConfig& policy, const Talents& talents = Talents(), sim::Race race = sim::Race::HUMAN) {
    ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Combat Policy & Priority Rules:");
    ImGui::Separator();

    // 0. Rotation Choice Combo
    ImGui::Text("Active Spell Rotation Preset:");
    int rot_idx = static_cast<int>(policy.rotation);
    const char* rot_names[] = {
        "Shadow (SW:P -> MB -> MF)",
        "Smite DPS (Holy Fire -> Smite)",
        "Holy Fire Weaving (Holy Fire -> SW:P -> Smite)"
    };
    ImGui::SetNextItemWidth(450);
    if (ImGui::Combo("##PriestRotationCombo", &rot_idx, rot_names, IM_ARRAYSIZE(rot_names))) {
        policy.rotation = static_cast<RotationChoice>(rot_idx);
    }
    ImGui::TextDisabled("%s", rotation_choice_to_string(policy.rotation));

    ImGui::Spacing();

    // 1. Dynamic Action Priority Chain
    auto rules = get_priest_priority_rules(policy, talents, race);
    warlock::render_common_priority_chain(rules);

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Rotational Ability Policies
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Rotational Ability Policies:");
    if (policy.rotation == RotationChoice::SHADOW_PRIEST || policy.rotation == RotationChoice::HOLY_FIRE_WEAVING) {
        ImGui::Checkbox("Maintain Shadow Word: Pain##Policy", &policy.maintain_swp);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Maintains 100%% uptime on Shadow Word: Pain.");
        }
        ImGui::SameLine(0, 16);
        ImGui::Checkbox("Cast Mind Blast on Cooldown##Policy", &policy.cast_mind_blast);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts Mind Blast whenever off cooldown (5.5s cooldown when talented).");
        }
    }

    if (policy.rotation == RotationChoice::SHADOW_PRIEST) {
        ImGui::Checkbox("Cast Shadow Word: Death##Policy", &policy.cast_sw_death);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts Shadow Word: Death on 15s cooldown.");
        }
        if (policy.cast_sw_death) {
            ImGui::SameLine(0, 16);
            ImGui::Checkbox("Execute Only (<20% HP)##Policy", &policy.execute_sw_death_only);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Restricts SW:D to execute phase (<20%% HP) to benefit from Early Demise +30%% crit.");
            }
        }

        ImGui::Checkbox("Cast Devouring Plague on Cooldown##Policy", &policy.cast_devouring_plague);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts Devouring Plague whenever off its 1 min cooldown.");
        }
        ImGui::SameLine(0, 16);
        ImGui::Checkbox("Clip Mind Flay after Tick 2 for MB / SW:P##Policy", &policy.clip_mind_flay_for_mb);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON: Clips Mind Flay early after the second damage tick (2.0s) if Mind Blast or SW:P is ready to cast.");
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Cooldown & Consumable Thresholds
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Cooldown & Consumable Triggers:");
    ImGui::Checkbox("Use Inner Focus on Cooldown##Policy", &policy.use_inner_focus);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically activates Inner Focus for a 100%% free spell with +25%% crit.");
    }
    ImGui::SameLine(0, 16);
    ImGui::Checkbox("Use Power Infusion on Cooldown##Policy", &policy.use_power_infusion);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Pops Power Infusion for +20%% spell damage and haste for 15 sec.");
    }

    ImGui::SetNextItemWidth(200);
    float pot_thresh = static_cast<float>(policy.mana_potion_threshold * 100.0);
    if (ImGui::SliderFloat("Mana Potion Threshold##Policy", &pot_thresh, 10.0f, 80.0f, "%.0f%% Mana")) {
        policy.mana_potion_threshold = pot_thresh / 100.0;
    }
    ImGui::SameLine(0, 16);
    ImGui::SetNextItemWidth(200);
    float rune_thresh = static_cast<float>(policy.demonic_rune_threshold * 100.0);
    if (ImGui::SliderFloat("Demonic Rune Threshold##Policy", &rune_thresh, 10.0f, 80.0f, "%.0f%% Mana")) {
        policy.demonic_rune_threshold = rune_thresh / 100.0;
    }
}

} // namespace priest
