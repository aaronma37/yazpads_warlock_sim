#pragma once
#include "imgui.h"
#include "asset_manager.hpp"
#include "src/ui/common/wow_widgets.hpp"
#include "src/sim/priest/policy.hpp"
#include "src/sim/priest/talents.hpp"
#include "src/sim/common/player_class.hpp"
#include "src/ui/common/panel_policy.hpp"
#include <vector>

namespace priest {

inline void render_priest_priority_chain_subpane(const std::vector<PriorityRule>& rules) {
    std::vector<warlock::CommonPriorityRule> common_rules;
    common_rules.reserve(rules.size());
    for (const auto& r : rules) {
        ImU32 bg_col = IM_COL32(56, 46, 76, 240);
        ImU32 border_col = IM_COL32(140, 90, 204, 230);
        if (r.spell_id == SpellID::POWER_INFUSION || r.spell_id == SpellID::INNER_FOCUS) {
            bg_col = IM_COL32(89, 64, 20, 240);
            border_col = IM_COL32(255, 204, 51, 230);
        } else if (r.spell_id == SpellID::DARK_SACRIFICE) {
            bg_col = IM_COL32(89, 30, 30, 240);
            border_col = IM_COL32(230, 76, 76, 230);
        } else if (r.spell_id == SpellID::RACIAL_BERSERKING) {
            bg_col = IM_COL32(25, 65, 85, 240);
            border_col = IM_COL32(80, 200, 240, 230);
        } else if (r.spell_id == SpellID::SMITE || r.spell_id == SpellID::HOLY_FIRE ||
                   r.spell_id == SpellID::PENANCE || r.spell_id == SpellID::HOLY_NOVA ||
                   r.spell_id == SpellID::CHASTISE) {
            bg_col = IM_COL32(82, 60, 25, 240);
            border_col = IM_COL32(255, 200, 51, 230);
        } else if (r.spell_id == SpellID::STARSHARDS) {
            bg_col = IM_COL32(25, 50, 85, 240);
            border_col = IM_COL32(80, 160, 240, 230);
        } else if (r.spell_id == SpellID::DEVOURING_PLAGUE) {
            bg_col = IM_COL32(35, 65, 40, 240);
            border_col = IM_COL32(80, 200, 100, 230);
        }

        common_rules.push_back({
            spell_id_to_icon(r.spell_id),
            r.name,
            std::string("Base Spell: ") + spell_id_to_name(r.spell_id) + " | Condition: " + r.condition_summary,
            r.trigger_condition,
            r.trigger_condition,
            bg_col,
            border_col
        });
    }
    warlock::render_common_priority_chain(common_rules);
}

inline void render_priest_policy_panel(PolicyConfig& policy, const Talents& talents = Talents(), sim::Race race = sim::Race::HUMAN) {
    ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Combat Policy & Priority Rules:");
    ImGui::Separator();

    // 0. Rotation Choice Combo
    ImGui::Text("Active Spell Rotation Preset:");
    int rot_idx = static_cast<int>(policy.rotation);
    const char* rot_names[] = {
        "Shadow (SW:P -> MB -> MF)",
        "Shadow - No Mind Blast (SW:P -> MF)",
        "Shadow - SW:P & Flay (Mana Conserve)",
        "Smite DPS (Holy Fire -> Smite)",
        "Pure Smite (Smite Spam + Penance)",
        "Holy Fire Weaving (Holy Fire -> SW:P -> Smite)",
        "Inquisitor (Holy Fire -> SW:P -> SW:D -> Smite)"
    };
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::Combo("##PriestRotationCombo", &rot_idx, rot_names, IM_ARRAYSIZE(rot_names))) {
        policy.rotation = static_cast<RotationChoice>(rot_idx);
    }
    ImGui::TextWrapped("%s", rotation_choice_description(policy.rotation));

    ImGui::Spacing();

    // 1. Dynamic Action Priority Chain
    std::vector<PriorityRule> rules = policy.get_priority_rules(talents, race);
    render_priest_priority_chain_subpane(rules);

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Action Priority List (APL) Order & Customization", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (policy.use_custom_apl) {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Custom APL Active (Modified)");
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset to Preset Defaults")) {
                policy.reset_to_preset(talents, race);
            }
        } else {
            ImGui::TextDisabled("Using standard preset ordering. Reorder or toggle any rule below to customize.");
        }

        if (ImGui::BeginTable("PriestAplTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 24.0f);
            ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_WidthFixed, 56.0f);
            ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 44.0f);
            ImGui::TableSetupColumn("Action / Spell", ImGuiTableColumnFlags_WidthStretch, 0.45f);
            ImGui::TableSetupColumn("Trigger Condition", ImGuiTableColumnFlags_WidthStretch, 0.55f);
            ImGui::TableHeadersRow();

            std::vector<PriorityRule> current_rules = policy.get_priority_rules(talents, race);
            for (size_t i = 0; i < current_rules.size(); ++i) {
                const auto& r = current_rules[i];
                ImGui::TableNextRow();
                ImGui::PushID(static_cast<int>(i));

                // Col 0: Index
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("%d", (int)i + 1);

                // Col 1: Up / Down move buttons
                ImGui::TableSetColumnIndex(1);
                if (i > 0) {
                    if (ImGui::SmallButton("^")) {
                        policy.move_rule_up(i, talents, race);
                    }
                } else {
                    ImGui::Dummy(ImVec2(16, 16));
                }
                ImGui::SameLine(0, 2);
                if (i + 1 < current_rules.size()) {
                    if (ImGui::SmallButton("v")) {
                        policy.move_rule_down(i, talents, race);
                    }
                }

                // Col 2: Active checkbox
                ImGui::TableSetColumnIndex(2);
                bool enabled = r.enabled;
                if (ImGui::Checkbox("##Enabled", &enabled)) {
                    policy.set_rule_enabled(i, enabled, talents, race);
                }

                // Col 3: Action / Spell Icon + Name
                ImGui::TableSetColumnIndex(3);
                Texture2D icon = warlock::AssetManager::get().get_icon(spell_id_to_icon(r.spell_id));
                if (icon.id > 0) {
                    ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
                    ImGui::SameLine(0, 4);
                }
                if (enabled)
                    ImGui::Text("%s", r.name.c_str());
                else
                    ImGui::TextDisabled("%s (Disabled)", r.name.c_str());

                // Col 4: Condition Summary
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(r.condition_summary.c_str());

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Rotational Ability Policies
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Rotational Ability Policies:");

    bool uses_swp = (policy.rotation == RotationChoice::SHADOW_PRIEST ||
                     policy.rotation == RotationChoice::SHADOW_NO_MB ||
                     policy.rotation == RotationChoice::SHADOW_SWP_ONLY ||
                     policy.rotation == RotationChoice::HOLY_FIRE_WEAVING ||
                     policy.rotation == RotationChoice::DISC_INQUISITOR);

    bool uses_mb = (policy.rotation == RotationChoice::SHADOW_PRIEST);

    bool uses_swd = (policy.rotation == RotationChoice::SHADOW_PRIEST ||
                     policy.rotation == RotationChoice::SHADOW_NO_MB ||
                     policy.rotation == RotationChoice::SHADOW_SWP_ONLY ||
                     policy.rotation == RotationChoice::DISC_INQUISITOR);

    bool uses_dp = (policy.rotation == RotationChoice::SHADOW_PRIEST ||
                    policy.rotation == RotationChoice::SHADOW_NO_MB);

    bool uses_hf = (policy.rotation == RotationChoice::SMITE_PRIEST ||
                    policy.rotation == RotationChoice::HOLY_FIRE_WEAVING ||
                    policy.rotation == RotationChoice::DISC_INQUISITOR);

    bool uses_penance = (policy.rotation == RotationChoice::SMITE_PRIEST ||
                         policy.rotation == RotationChoice::PURE_SMITE ||
                         policy.rotation == RotationChoice::HOLY_FIRE_WEAVING ||
                         policy.rotation == RotationChoice::DISC_INQUISITOR);

    bool uses_mf = (policy.rotation == RotationChoice::SHADOW_PRIEST ||
                    policy.rotation == RotationChoice::SHADOW_NO_MB ||
                    policy.rotation == RotationChoice::SHADOW_SWP_ONLY);

    if (uses_swp) {
        warlock::WowCheckbox("Maintain Shadow Word: Pain##Policy", &policy.maintain_swp);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Maintains 100%% uptime on Shadow Word: Pain.");
        }
    }

    if (uses_mb) {
        if (uses_swp) ImGui::SameLine(0, 16);
        warlock::WowCheckbox("Cast Mind Blast on Cooldown##Policy", &policy.cast_mind_blast);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts Mind Blast whenever off cooldown (5.5s cooldown when talented).");
        }
    }

    if (uses_swd) {
        warlock::WowCheckbox("Cast Shadow Word: Death##Policy", &policy.cast_sw_death);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts Shadow Word: Death on 15s cooldown.");
        }
        if (policy.cast_sw_death) {
            ImGui::SameLine(0, 16);
            warlock::WowCheckbox("Execute Only (<20% HP)##Policy", &policy.execute_sw_death_only);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Restricts SW:D to execute phase (<20%% HP) to benefit from Early Demise +30%% crit.");
            }
        }
    }

    if (uses_dp) {
        warlock::WowCheckbox("Cast Devouring Plague on Cooldown##Policy", &policy.cast_devouring_plague);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts Devouring Plague whenever off its 1 min cooldown.");
        }
    }

    if (uses_hf) {
        warlock::WowCheckbox("Maintain Holy Fire DoT##Policy", &policy.cast_holy_fire);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts Holy Fire to maintain DoT and trigger Power in Light (+10%% Smite/Penance damage).");
        }
    }

    if (uses_penance && talents.disc.penance > 0) {
        warlock::WowCheckbox("Cast Penance on Cooldown##Policy", &policy.cast_penance);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Channels Penance whenever off cooldown.");
        }
    }

    if (uses_mf) {
        warlock::WowCheckbox("Clip Mind Flay after Tick 2 for MB / SW:P##Policy", &policy.clip_mind_flay_for_mb);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON: Clips Mind Flay early after the second damage tick (2.0s) if Mind Blast or SW:P is ready to cast.");
        }
    }

    if (talents.shadow.vampiric_embrace > 0) {
        warlock::WowCheckbox("Maintain Vampiric Embrace##Policy", &policy.cast_vampiric_embrace);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts Vampiric Embrace every 30s to heal the party for 20%% of Shadow spell damage.");
        }
    }

    if (policy.rotation == RotationChoice::SMITE_PRIEST || policy.rotation == RotationChoice::PURE_SMITE) {
        warlock::WowCheckbox("Cast Free Holy Nova on Clearcast Proc##Policy", &policy.cast_holy_nova_on_proc);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Consumes Searing Light clearcast procs with an instant free Holy Nova.");
        }
    }

    if (race == sim::Race::NIGHT_ELF) {
        warlock::WowCheckbox("Cast Starshards on Cooldown (Night Elf)##Policy", &policy.cast_starshards);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Channels racial Starshards on 30s cooldown for heavy Arcane damage.");
        }
    } else if (race == sim::Race::DWARF) {
        warlock::WowCheckbox("Cast Chastise on Cooldown (Dwarf)##Policy", &policy.cast_chastise);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Casts racial Chastise on 2 min cooldown for instant Holy damage.");
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Cooldown & Consumable Thresholds
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Cooldown & Consumable Triggers:");
    warlock::WowCheckbox("Use Inner Focus on Cooldown##Policy", &policy.use_inner_focus);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically activates Inner Focus for a 100%% free spell with +25%% crit.");
    }
    ImGui::SameLine(0, 16);
    warlock::WowCheckbox("Use Power Infusion on Cooldown##Policy", &policy.use_power_infusion);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Pops Power Infusion for +20%% spell damage and haste for 15 sec.");
    }

    if (race == sim::Race::UNDEAD) {
        ImGui::SameLine(0, 16);
        warlock::WowCheckbox("Dark Sacrifice (<60% Mana)##Policy", &policy.use_dark_sacrifice);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Cannibalizes 1600 HP over 15s to restore 1600 Mana when under 60%% Mana.");
        }
    } else if (race == sim::Race::TROLL) {
        ImGui::SameLine(0, 16);
        warlock::WowCheckbox("Berserking on Cooldown##Policy", &policy.use_berserking);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Pops Troll racial Berserking on 3 min cooldown.");
        }
    }

    ImGui::SetNextItemWidth(200);
    float pot_thresh = static_cast<float>(policy.mana_potion_threshold * 100.0);
    if (warlock::WowSliderFloat("Mana Potion Threshold##Policy", &pot_thresh, 10.0f, 80.0f, "%.0f%% Mana")) {
        policy.mana_potion_threshold = pot_thresh / 100.0;
    }
    ImGui::SameLine(0, 16);
    ImGui::SetNextItemWidth(200);
    float rune_thresh = static_cast<float>(policy.demonic_rune_threshold * 100.0);
    if (warlock::WowSliderFloat("Demonic Rune Threshold##Policy", &rune_thresh, 10.0f, 80.0f, "%.0f%% Mana")) {
        policy.demonic_rune_threshold = rune_thresh / 100.0;
    }
}

} // namespace priest
