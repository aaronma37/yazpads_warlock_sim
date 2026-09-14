#pragma once
#include "imgui.h"
#include "asset_manager.hpp"
#include "src/sim/policy.hpp"
#include "src/sim/talents.hpp"
#include "src/sim/warlock_sim.hpp"
#include <vector>
#include <algorithm>

namespace warlock {

inline void render_priority_chain_subpane(const std::vector<PriorityRule>& rules) {
    ImGui::TextColored(ImVec4(0.40f, 0.85f, 1.0f, 1.0f), "Action Priority Chain:");

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.08f, 0.15f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.25f, 0.70f, 0.80f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));

    // Dynamic height child with auto-wrapping onto multiple lines
    if (ImGui::BeginChild("PriorityRuleChainBox", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
        float avail_width = ImGui::GetContentRegionAvail().x;
        float cur_x = 0.0f;
        const float icon_sz = 18.0f;
        const float arrow_w = 16.0f;

        for (size_t i = 0; i < rules.size(); ++i) {
            const auto& rule = rules[i];
            ImGui::PushID(static_cast<int>(i));

            Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(rule.spell_id));
            const float badge_w = 26.0f;
            const float badge_h = 26.0f;
            const float icon_sz = 20.0f;
            float total_w = badge_w + ((i + 1 < rules.size()) ? (arrow_w + 6.0f) : 0.0f);

            // Wrap to next line if badge + arrow would exceed available width
            if (i > 0) {
                if (cur_x + total_w > avail_width && cur_x > 0.0f) {
                    ImGui::NewLine();
                    cur_x = 0.0f;
                } else {
                    ImGui::SameLine(0, 4);
                    cur_x += 4.0f;
                }
            }

            // Colors based on action type
            ImU32 bg_col = IM_COL32(56, 46, 76, 240);
            ImU32 border_col = IM_COL32(140, 90, 204, 230);
            if (rule.action == PriorityAction::LIFE_TAP) {
                bg_col = IM_COL32(89, 30, 30, 240);
                border_col = IM_COL32(230, 76, 76, 230);
            } else if (rule.action == PriorityAction::NIGHTFALL_SHADOW_BOLT || rule.action == PriorityAction::DECIMATION_SOUL_FIRE) {
                bg_col = IM_COL32(89, 64, 20, 240);
                border_col = IM_COL32(255, 204, 51, 230);
            } else if (rule.action == PriorityAction::CONFLAGRATE || rule.action == PriorityAction::INCINERATE_FILLER) {
                bg_col = IM_COL32(82, 46, 25, 240);
                border_col = IM_COL32(255, 140, 51, 230);
            } else if (rule.action == PriorityAction::RACIAL_EUREKA || rule.action == PriorityAction::RACIAL_BLOOD_FURY || rule.action == PriorityAction::RACIAL_BERSERKING) {
                bg_col = IM_COL32(25, 65, 85, 240);
                border_col = IM_COL32(80, 200, 240, 230);
            }

            // Reserve item space with InvisibleButton
            std::string btn_label = "##RuleBtn_" + std::to_string(i);
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton(btn_label.c_str(), ImVec2(badge_w, badge_h));
            bool is_hovered = ImGui::IsItemHovered();
            ImVec2 p1 = ImVec2(p0.x + badge_w, p0.y + badge_h);

            // Draw custom badge background, border, and centered icon
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImU32 fill_col = is_hovered ? IM_COL32(80, 70, 110, 250) : bg_col;
            draw_list->AddRectFilled(p0, p1, fill_col, 4.0f);
            draw_list->AddRect(p0, p1, is_hovered ? IM_COL32(255, 255, 255, 255) : border_col, 4.0f, 0, 1.0f);

            // Draw icon centered
            float icon_x = p0.x + (badge_w - icon_sz) * 0.5f;
            float icon_y = p0.y + (badge_h - icon_sz) * 0.5f;
            draw_list->AddImage((ImTextureID)(uintptr_t)icon.id, ImVec2(icon_x, icon_y), ImVec2(icon_x + icon_sz, icon_y + icon_sz));

            if (is_hovered) {
                ImGui::BeginTooltip();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.30f, 1.0f));
                ImGui::Text("Priority Rank #%d: %s", (int)i + 1, rule.name.c_str());
                ImGui::PopStyleColor();
                ImGui::Separator();

                ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Trigger Condition:");
                ImGui::TextWrapped("%s", rule.trigger_condition.c_str());

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Rule Purpose & Mechanics:");
                ImGui::TextWrapped("%s", rule.rule_explanation.c_str());

                ImGui::Separator();
                ImGui::TextDisabled("Base Spell: %s | Condition: %s", spell_id_to_name(rule.spell_id), rule.condition_summary.c_str());
                ImGui::EndTooltip();
            }

            cur_x += badge_w;

            // Render arrow separator
            if (i + 1 < rules.size()) {
                ImGui::SameLine(0, 4);
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.20f, 0.9f), ">");
                cur_x += arrow_w + 4.0f;
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}


inline void render_panel_policy_controls(PolicyConfig& policy, const Talents& talents, Race race = Race::UNDEAD) {
    ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Combat Policy & Priority Rules:");
    ImGui::Separator();

    // 0. Rotation Choice / Strategy
    ImGui::Text("Active Spell Rotation Preset:");
    int rot_idx = static_cast<int>(policy.rotation);
    const char* rot_names[] = {
        "5/11/35 DS/AF (Conflag Weave + SB Spam)",
        "5/11/35 Fire Destro (Incinerate + Conflagrate)",
        "0/31/20 DP/AF Fire (Searing Pain + Immolate + Conflag)",
        "40/11/0 Deep Affliction (DS Imp / Drain Hope)",
        "32/0/19 SM/AF (Corruption + CoA + SB Spam)",
        "0/31/20 Demo Execute (Decimation Soul Fire + SB)",
        "Pure Shadow Bolt (No DoTs / Classic Limit)",
        "20/0/31 Multi-DoT Hybrid (Agony + Corr + Immo)"
    };
    ImGui::SetNextItemWidth(450);
    if (ImGui::Combo("##RotationCombo", &rot_idx, rot_names, IM_ARRAYSIZE(rot_names))) {
        policy.rotation = static_cast<RotationChoice>(rot_idx);
    }

    ImGui::TextDisabled("%s", rotation_choice_description(policy.rotation));

    ImGui::Spacing();

    // 1. Dynamic Rule-Based Priority Chain Subpane (<Spell> > <Spell> > <Spell>)
    std::vector<PriorityRule> rules = policy.get_priority_rules(talents, race);
    render_priority_chain_subpane(rules);
}

inline void render_panel_policy(PolicyConfig& policy, const Talents& talents, Race race = Race::UNDEAD) {
    render_panel_policy_controls(policy, talents, race);
}

inline void render_panel_policy(WarlockSimulator& sim) {
    render_panel_policy_controls(sim.policy, sim.talents, sim.race);
}

} // namespace warlock
