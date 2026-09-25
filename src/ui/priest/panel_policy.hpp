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

inline std::vector<PriorityRule> get_available_priest_actions(const Talents& talents, sim::Race race) {
    std::vector<PriorityRule> list;
    if (talents.shadow.mind_flay > 0) {
        list.push_back({SpellID::MIND_FLAY, "Mind Flay", "Primary Filler", "3.0s channeled Shadow beam (benefits from Shadowform/Darkness).", true});
    }
    list.push_back({SpellID::SMITE, "Smite", "Primary Filler", "2.0s - 2.5s Holy direct damage filler.", true});
    list.push_back({SpellID::SHADOW_WORD_PAIN, "Shadow Word: Pain", "DoT Expired / Refresh", "Maintains 100% uptime on Shadow Word: Pain DoT.", true});
    list.push_back({SpellID::MIND_BLAST, "Mind Blast", "On Cooldown (5.5s-8s)", "High burst Shadow damage on short cooldown.", true});
    list.push_back({SpellID::SHADOW_WORD_DEATH, "Shadow Word: Death", "On Cooldown (15s)", "Instant shadow nuke (10% max HP backlash self-damage).", true});
    list.push_back({SpellID::DEVOURING_PLAGUE, "Devouring Plague", "On Cooldown (1 min)", "Afflicts target with Shadow damage over 24s and heals caster.", true});
    list.push_back({SpellID::HOLY_FIRE, "Holy Fire", "Maintain DoT (10s)", "Empowers Smite & Penance (+10% damage from Power in Light).", true});
    if (talents.disc.penance > 0) {
        list.push_back({SpellID::PENANCE, "Penance", "On Cooldown (10s)", "3-pulse Holy volley channel; benefits from Power in Light.", true});
    }
    list.push_back({SpellID::HOLY_NOVA, "Holy Nova", "Clearcast Proc", "Instant free cast when Clearcasting triggers.", true});
    if (talents.shadow.vampiric_embrace > 0) {
        list.push_back({SpellID::VAMPIRIC_EMBRACE, "Vampiric Embrace", "DoT Expired (30s)", "Afflicts target: 20% of shadow spell damage heals party.", true});
    }
    if (talents.disc.inner_focus > 0) {
        list.push_back({SpellID::INNER_FOCUS, "Inner Focus", "On Cooldown (3m)", "Next spell costs 0 mana and +25% crit chance.", true});
    }
    if (talents.disc.power_infusion > 0) {
        list.push_back({SpellID::POWER_INFUSION, "Power Infusion", "On Cooldown (3m)", "Infuses self for +20% spell damage & healing for 15s.", true});
    }
    if (race == sim::Race::UNDEAD) {
        list.push_back({SpellID::DARK_SACRIFICE, "Dark Sacrifice", "Mana <= 60%", "Cannibalize / sacrifice target for 1600 mana.", true});
    } else if (race == sim::Race::TROLL) {
        list.push_back({SpellID::RACIAL_BERSERKING, "Berserking", "On Cooldown (3m)", "Increases casting speed by 10% for 10s.", true});
    } else if (race == sim::Race::NIGHT_ELF) {
        list.push_back({SpellID::STARSHARDS, "Starshards", "On Cooldown (30s)", "Night Elf racial channel dealing Arcane damage over 6s.", true});
    } else if (race == sim::Race::DWARF) {
        list.push_back({SpellID::CHASTISE, "Chastise", "On Cooldown (2 min)", "Dwarf racial instant Holy damage.", true});
    }
    return list;
}

inline void render_priest_policy_panel(PolicyConfig& policy, const Talents& talents = Talents(), sim::Race race = sim::Race::HUMAN) {
    ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Action Priority List");
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
        policy.use_custom_apl = false;
        policy.custom_rules.clear();
    }
    ImGui::TextWrapped("%s", rotation_choice_description(policy.rotation));

    ImGui::Spacing();

    if (policy.use_custom_apl) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Custom APL Active (Modified)");
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset to Preset Defaults")) {
            policy.reset_to_preset(talents, race);
        }
    }

    if (ImGui::BeginTable("PriestAplTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Action / Spell", ImGuiTableColumnFlags_WidthStretch, 0.44f);
        ImGui::TableSetupColumn("Trigger Condition", ImGuiTableColumnFlags_WidthStretch, 0.56f);
        ImGui::TableSetupColumn("##ModifyCol", ImGuiTableColumnFlags_WidthFixed, 26.0f);
        ImGui::TableHeadersRow();

        std::vector<PriorityRule> current_rules = policy.get_priority_rules(talents, race);
        std::vector<PriorityRule> available_rules = get_available_priest_actions(talents, race);

        for (size_t i = 0; i < current_rules.size(); ++i) {
            const auto& r = current_rules[i];
            ImGui::TableNextRow();
            ImGui::PushID(static_cast<int>(i));

            // Col 0: Action / Spell Icon + Name
            ImGui::TableSetColumnIndex(0);
            Texture2D icon = warlock::AssetManager::get().get_icon(spell_id_to_icon(r.spell_id));
            if (icon.id > 0) {
                ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
                ImGui::SameLine(0, 4);
            }
            ImGui::Text("%s", r.name.c_str());

            // Col 1: Condition Summary
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(r.condition_summary.c_str());

            // Col 2: Modify button & popup
            ImGui::TableSetColumnIndex(2);
            std::string btn_label = "##PriestRuleMod_" + std::to_string(i);
            if (warlock::WowBiggerButton(btn_label.c_str(), ImVec2(20, 20))) {
                ImGui::OpenPopup("ModifyRulePopup");
            }

            if (ImGui::BeginPopup("ModifyRulePopup")) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "#%zu %s", i + 1, r.name.c_str());
                ImGui::Separator();

                if (i > 0) {
                    if (ImGui::MenuItem("▲ Move Up")) {
                        policy.move_rule_up(i, talents, race);
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::MenuItem("▲ Move Up");
                    ImGui::EndDisabled();
                }

                if (i + 1 < current_rules.size()) {
                    if (ImGui::MenuItem("▼ Move Down")) {
                        policy.move_rule_down(i, talents, race);
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::MenuItem("▼ Move Down");
                    ImGui::EndDisabled();
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Add Rule Above")) {
                    for (const auto& avail : available_rules) {
                        if (ImGui::MenuItem(avail.name.c_str())) {
                            policy.insert_rule(i, avail, talents, race);
                        }
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Add Rule Below")) {
                    for (const auto& avail : available_rules) {
                        if (ImGui::MenuItem(avail.name.c_str())) {
                            policy.insert_rule(i + 1, avail, talents, race);
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Remove Rule")) {
                    policy.remove_rule(i, talents, race);
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
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

    ImGui::Text("Mana Potion Threshold (%% Mana):");
    ImGui::SetNextItemWidth(160);
    double pot_thresh = policy.mana_potion_threshold * 100.0;
    if (warlock::WowInputDouble("##ManaPotionThreshold", &pot_thresh, 1.0, 5.0, "%.0f%%")) {
        if (pot_thresh < 5.0) pot_thresh = 5.0;
        if (pot_thresh > 95.0) pot_thresh = 95.0;
        policy.mana_potion_threshold = pot_thresh / 100.0;
    }
    ImGui::SameLine(0, 16);
    ImGui::BeginGroup();
    ImGui::Text("Demonic Rune Threshold (%% Mana):");
    ImGui::SetNextItemWidth(160);
    double rune_thresh = policy.demonic_rune_threshold * 100.0;
    if (warlock::WowInputDouble("##DemonicRuneThreshold", &rune_thresh, 1.0, 5.0, "%.0f%%")) {
        if (rune_thresh < 5.0) rune_thresh = 5.0;
        if (rune_thresh > 95.0) rune_thresh = 95.0;
        policy.demonic_rune_threshold = rune_thresh / 100.0;
    }
    ImGui::EndGroup();
}

} // namespace priest
