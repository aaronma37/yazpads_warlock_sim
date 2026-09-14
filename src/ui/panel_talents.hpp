#pragma once
#include "imgui.h"
#include "rlImGui.h"
#include "src/sim/talents.hpp"
#include "src/ui/asset_manager.hpp"
#include <string>
#include <cstring>
#include <array>
#include <cmath>
#include <algorithm>

namespace warlock {

// Helper to check prerequisite for a node in a tree
template<size_t N>
inline bool is_prereq_fulfilled(const std::array<TalentNodeDef, N>& nodes, const TalentNodeDef& node, auto get_pts_func) {
    if (!node.req) return true;
    for (size_t i = 0; i < N; ++i) {
        if (strcmp(nodes[i].name, node.req) == 0) {
            return get_pts_func(i) == nodes[i].max_points;
        }
    }
    return true;
}

// Helper to check if removing 1 point from a node is permitted
template<size_t N>
inline bool can_unlearn_talent(const std::array<TalentNodeDef, N>& nodes, size_t node_idx, auto get_pts_func) {
    const auto& node = nodes[node_idx];
    if (get_pts_func(node_idx) <= 0) return false;

    // 1. Check if any talent in this tree that currently has >0 points depends on this talent as a prerequisite
    for (size_t i = 0; i < N; ++i) {
        if (nodes[i].req && strcmp(nodes[i].req, node.name) == 0) {
            if (get_pts_func(i) > 0) return false;
        }
    }

    // 2. Check tier requirements for all higher tiers with points
    for (int R = node.row + 1; R <= 7; ++R) {
        int pts_at_or_above_R = 0;
        int pts_below_R = 0;
        for (size_t i = 0; i < N; ++i) {
            if (nodes[i].row >= R) pts_at_or_above_R += get_pts_func(i);
            if (nodes[i].row < R) pts_below_R += get_pts_func(i);
        }
        // If there are points at or above tier R, removing 1 point below tier R cannot drop total below (R - 1) * 5
        if (pts_at_or_above_R > 0 && pts_below_R <= (R - 1) * 5) {
            return false;
        }
    }

    return true;
}

// Helper to render one arrow between prerequisite and dependent talent
inline void draw_talent_arrow(ImDrawList* draw_list, ImVec2 A, ImVec2 B, float icon_sz, bool active) {
    ImU32 col = active ? IM_COL32(255, 210, 30, 230) : IM_COL32(110, 110, 120, 160);
    float line_thickness = active ? 2.5f : 2.0f;
    float arrow_sz = 6.0f;

    // Same column: direct vertical arrow down
    if (std::abs(A.x - B.x) < 4.0f) {
        ImVec2 start_pt(A.x, A.y + icon_sz * 0.5f + 1.0f);
        ImVec2 end_pt(B.x, B.y - icon_sz * 0.5f - 4.0f);
        draw_list->AddLine(start_pt, end_pt, col, line_thickness);
        draw_list->AddTriangleFilled(
            ImVec2(end_pt.x, end_pt.y + 3.0f),
            ImVec2(end_pt.x - arrow_sz, end_pt.y - arrow_sz),
            ImVec2(end_pt.x + arrow_sz, end_pt.y - arrow_sz),
            col
        );
    }
    // Same row: direct horizontal arrow right
    else if (std::abs(A.y - B.y) < 4.0f) {
        float dir = (B.x > A.x) ? 1.0f : -1.0f;
        ImVec2 start_pt(A.x + dir * (icon_sz * 0.5f + 1.0f), A.y);
        ImVec2 end_pt(B.x - dir * (icon_sz * 0.5f + 4.0f), B.y);
        draw_list->AddLine(start_pt, end_pt, col, line_thickness);
        draw_list->AddTriangleFilled(
            ImVec2(end_pt.x + dir * 3.0f, end_pt.y),
            ImVec2(end_pt.x - dir * arrow_sz, end_pt.y - arrow_sz),
            ImVec2(end_pt.x - dir * arrow_sz, end_pt.y + arrow_sz),
            col
        );
    }
    // Multi-row and multi-column: stepped / elbow arrow
    else {
        ImVec2 p0(A.x, A.y + icon_sz * 0.5f + 1.0f);
        float mid_y = B.y - icon_sz * 0.5f - 14.0f;
        ImVec2 p1(A.x, mid_y);
        ImVec2 p2(B.x, mid_y);
        ImVec2 end_pt(B.x, B.y - icon_sz * 0.5f - 4.0f);

        draw_list->AddLine(p0, p1, col, line_thickness);
        draw_list->AddLine(p1, p2, col, line_thickness);
        draw_list->AddLine(p2, end_pt, col, line_thickness);

        draw_list->AddTriangleFilled(
            ImVec2(end_pt.x, end_pt.y + 3.0f),
            ImVec2(end_pt.x - arrow_sz, end_pt.y - arrow_sz),
            ImVec2(end_pt.x + arrow_sz, end_pt.y - arrow_sz),
            col
        );
    }
}

// Render a single interactive talent tree column with authentic background art
template<size_t N>
inline void render_tree_column(
    const char* tree_name,
    const char* bg_filename,
    const ImVec4& title_color,
    const std::array<TalentNodeDef, N>& nodes,
    auto get_pts_func,
    auto get_pts_ref_func,
    auto reset_tree_func,
    int tree_total_points,
    int all_total_points,
    float col_w,
    float col_h
) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    std::string child_id = std::string("TreeChild_") + tree_name;
    ImGui::BeginChild(child_id.c_str(), ImVec2(col_w, col_h), true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();
    ImVec2 win_max = ImVec2(win_pos.x + win_size.x, win_pos.y + win_size.y);

    // 1. Draw Authentic Talent Background Image
    const Texture2D& bg_tex = AssetManager::get().get_icon(bg_filename);
    if (bg_tex.id > 0) {
        draw_list->AddImage(
            ImTextureID(bg_tex.id),
            win_pos,
            win_max,
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f),
            IM_COL32(255, 255, 255, 210)
        );
        // Subtle dark ambient overlay so icons, text, and lines are sharp and readable
        draw_list->AddRectFilled(
            win_pos,
            win_max,
            IM_COL32(10, 12, 18, 90)
        );
    } else {
        // Fallback dark gradient
        draw_list->AddRectFilled(win_pos, win_max, IM_COL32(20, 20, 26, 255));
    }

    // Border frame
    draw_list->AddRect(win_pos, win_max, IM_COL32(85, 75, 55, 255), 4.0f, 0, 1.5f);

    // 2. Tree Header (Title, Points, Reset button)
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::TextColored(title_color, "%s", tree_name);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "(%d)", tree_total_points);

    // Reset button on the top right
    float reset_btn_x = col_w - 38.0f;
    ImGui::SameLine(reset_btn_x);
    std::string reset_btn_id = std::string("##Reset_") + tree_name;
    if (ImGui::SmallButton(("↺" + reset_btn_id).c_str())) {
        reset_tree_func();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset all points in %s", tree_name);
    }
    ImGui::PopFont();

    ImGui::Separator();

    // 3. Grid Positioning Parameters
    const float icon_sz = 34.0f;
    const float cell_w = 42.0f;
    const float col_spacing = 12.0f;
    const float total_grid_w = 4.0f * cell_w + 3.0f * col_spacing;
    const float start_x = std::max(4.0f, (col_w - total_grid_w) * 0.5f);
    const float grid_start_y = 38.0f;
    const float row_height = 68.0f;

    std::array<ImVec2, N> node_centers;
    std::array<bool, N> node_rendered;
    node_rendered.fill(false);

    // Subtle horizontal tier guidelines
    for (int r = 1; r <= 7; ++r) {
        float tier_y = win_pos.y + grid_start_y + (r - 1) * row_height;
        draw_list->AddLine(
            ImVec2(win_pos.x + 8.0f, tier_y - 2.0f),
            ImVec2(win_max.x - 8.0f, tier_y - 2.0f),
            IM_COL32(255, 255, 255, 14),
            1.0f
        );
    }

    // 4. Render 7x4 Grid of Talents
    for (int row = 1; row <= 7; ++row) {
        bool tier_unlocked = (tree_total_points >= (row - 1) * 5);
        float cursor_y = grid_start_y + (row - 1) * row_height;

        for (int col = 1; col <= 4; ++col) {
            // Find talent node at (row, col)
            int found_idx = -1;
            for (size_t i = 0; i < N; ++i) {
                if (nodes[i].row == row && nodes[i].col == col) {
                    found_idx = static_cast<int>(i);
                    break;
                }
            }

            if (found_idx < 0) continue;

            const auto& node = nodes[found_idx];
            int current_pts = get_pts_func(found_idx);
            bool prereq_met = is_prereq_fulfilled(nodes, node, get_pts_func);
            bool can_learn = (all_total_points < 51) && (current_pts < node.max_points) && tier_unlocked && prereq_met;
            bool can_unlearn = can_unlearn_talent(nodes, found_idx, get_pts_func);

            float cell_x = start_x + (col - 1) * (cell_w + col_spacing) + (cell_w - icon_sz) * 0.5f;
            ImGui::SetCursorPos(ImVec2(cell_x, cursor_y));

            ImVec2 icon_screen_pos = ImGui::GetCursorScreenPos();
            node_centers[found_idx] = ImVec2(icon_screen_pos.x + icon_sz * 0.5f, icon_screen_pos.y + icon_sz * 0.5f);
            node_rendered[found_idx] = true;

            // Border color: Gold if maxed, Vivid Green if partial, Light gray if learnable, Dim if locked
            ImVec4 border_col = ImVec4(0.25f, 0.25f, 0.25f, 0.85f);
            if (current_pts == node.max_points) {
                border_col = ImVec4(1.0f, 0.82f, 0.15f, 1.0f); // Gold
            } else if (current_pts > 0) {
                border_col = ImVec4(0.2f, 0.95f, 0.2f, 1.0f);  // Vivid Green
            } else if (tier_unlocked && prereq_met) {
                border_col = ImVec4(0.75f, 0.75f, 0.75f, 1.0f); // White/Silver available
            }

            const Texture2D& tex = AssetManager::get().get_icon(node.icon);

            ImGui::PushID(node.id);
            ImGui::PushStyleColor(ImGuiCol_Border, border_col);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.45f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.35f, 0.70f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.40f, 0.40f, 0.55f, 0.85f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, (current_pts > 0) ? 2.0f : 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 1.0f));

            if (rlImGuiImageButtonSize(node.id, &tex, Vector2{ icon_sz, icon_sz })) {
                if (can_learn) {
                    get_pts_ref_func(found_idx)++;
                }
            }

            // Right click decrements point
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                if (can_unlearn) {
                    get_pts_ref_func(found_idx)--;
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);

            // If locked, draw semi-transparent dark shade over icon
            if (!tier_unlocked || !prereq_met) {
                draw_list->AddRectFilled(
                    icon_screen_pos,
                    ImVec2(icon_screen_pos.x + icon_sz + 2.0f, icon_screen_pos.y + icon_sz + 2.0f),
                    IM_COL32(0, 0, 0, 155),
                    2.0f
                );
            }

            // Authentic WoW Tooltip on Hover
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", node.name);
                ImGui::TextDisabled("Rank %d / %d", current_pts, node.max_points);
                ImGui::Separator();

                // Current rank description
                int desc_idx = (current_pts > 0) ? (current_pts - 1) : 0;
                if (node.desc[desc_idx]) {
                    ImGui::TextWrapped("%s", node.desc[desc_idx]);
                }

                // Next rank description
                if (current_pts > 0 && current_pts < node.max_points && node.desc[current_pts]) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Next Rank:");
                    ImGui::TextWrapped("%s", node.desc[current_pts]);
                }

                // Requirements warnings
                if (!tier_unlocked) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Requires %d points in %s Talents", (row - 1) * 5, tree_name);
                }
                if (!prereq_met && node.req) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Requires max points in %s", node.req);
                }

                ImGui::Spacing();
                ImGui::TextDisabled("Left-click: Learn | Right-click: Unlearn");
                ImGui::EndTooltip();
            }

            // Rank Badge below the icon (e.g. "5/5")
            ImVec4 badge_col = (current_pts == node.max_points) ? ImVec4(1.0f, 0.82f, 0.15f, 1.0f) :
                               (current_pts > 0) ? ImVec4(0.2f, 0.95f, 0.2f, 1.0f) :
                               (tier_unlocked && prereq_met) ? ImVec4(0.85f, 0.85f, 0.85f, 1.0f) :
                               ImVec4(0.40f, 0.40f, 0.40f, 1.0f);

            char badge_str[16];
            snprintf(badge_str, sizeof(badge_str), "%d/%d", current_pts, node.max_points);
            float badge_text_w = ImGui::CalcTextSize(badge_str).x;
            ImGui::SetCursorPos(ImVec2(cell_x + (icon_sz - badge_text_w) * 0.5f, cursor_y + icon_sz + 3.0f));
            ImGui::TextColored(badge_col, "%s", badge_str);

            ImGui::PopID();
        }
    }

    // 5. Draw Prerequisite Arrows
    for (size_t i = 0; i < N; ++i) {
        if (!nodes[i].req) continue;

        int prereq_idx = -1;
        for (size_t p = 0; p < N; ++p) {
            if (strcmp(nodes[p].name, nodes[i].req) == 0) {
                prereq_idx = static_cast<int>(p);
                break;
            }
        }

        if (prereq_idx >= 0 && node_rendered[prereq_idx] && node_rendered[i]) {
            bool active = (get_pts_func(prereq_idx) == nodes[prereq_idx].max_points);
            draw_talent_arrow(draw_list, node_centers[prereq_idx], node_centers[i], icon_sz, active);
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

// Master Panel: Render All 3 Talent Trees Side by Side on the Same Pane
inline void render_panel_talents(WarlockSimulator& sim) {
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "Talents");
    ImGui::Separator();

    // Summary Header
    int total_pts = sim.talents.total_points();
    ImVec4 pt_color = (total_pts <= 51) ? ImVec4(0.4f, 0.95f, 0.4f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);

    ImGui::Text("Active Build: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "[%d / %d / %d]",
        sim.talents.aff.total_points(), sim.talents.demo.total_points(), sim.talents.destro.total_points());
    ImGui::SameLine();
    ImGui::Text("  Points: ");
    ImGui::SameLine();
    ImGui::TextColored(pt_color, "%d / 51", total_pts);
    ImGui::SameLine();
    ImGui::TextDisabled("(Left-click icon to add point, Right-click to remove)");

    ImGui::Spacing();

    // Presets Row
    ImGui::Text("Presets:");
    ImGui::SameLine();
    if (ImGui::SmallButton("DS/AF")) {
        sim.talents = Talents::create_forever_ds_af();
        sim.buffs.sacrifice_succubus = false;
        sim.buffs.sacrifice_imp = true;
        sim.policy.pet = PetChoice::NONE;
        sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("DS/Incinerate")) {
        sim.talents = Talents::create_forever_ds_incinerate();
        sim.buffs.sacrifice_succubus = true;
        sim.buffs.sacrifice_imp = false;
        sim.policy.maintain_immolate = true;
        sim.policy.rotation = RotationChoice::FIRE_DESTRO;
        sim.policy.pet = PetChoice::NONE;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("DS/Searing Pain")) {
        sim.talents = Talents::create_forever_ds_searing_pain();
        sim.buffs.sacrifice_succubus = true;
        sim.buffs.sacrifice_imp = false;
        sim.policy.maintain_immolate = true;
        sim.policy.rotation = RotationChoice::FIRE_DESTRO;
        sim.policy.pet = PetChoice::NONE;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("DP/AF Shadow")) {
        sim.talents = Talents::create_forever_dp_af_shadow();
        sim.buffs.sacrifice_succubus = false;
        sim.buffs.sacrifice_imp = true;
        sim.policy.pet = PetChoice::SUCCUBUS;
        sim.policy.rotation = RotationChoice::DP_AF_SHADOW;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("DP/AF Fire")) {
        sim.talents = Talents::create_forever_dp_af_fire();
        sim.buffs.sacrifice_succubus = true;
        sim.buffs.sacrifice_imp = false;
        sim.policy.pet = PetChoice::IMP;
        sim.policy.rotation = RotationChoice::DP_RUIN_FIRE;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Deep Affliction")) {
        sim.talents = Talents::create_forever_deep_affliction();
        sim.buffs.sacrifice_succubus = false;
        sim.buffs.sacrifice_imp = true;
        sim.policy.rotation = RotationChoice::DEEP_AFFLICTION;
        sim.policy.pet = PetChoice::NONE;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Fire Destro+Decim")) {
        sim.talents = Talents::create_forever_fire_destro_decimation();
        sim.buffs.sacrifice_succubus = true;
        sim.buffs.sacrifice_imp = false;
        sim.policy.maintain_immolate = true;
        sim.policy.rotation = RotationChoice::FIRE_DESTRO;
        sim.policy.pet = PetChoice::NONE;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Shadow and Flame")) {
        sim.talents = Talents::create_forever_shadow_and_flame();
        sim.buffs.sacrifice_succubus = false;
        sim.buffs.sacrifice_imp = false;
        sim.policy.pet = PetChoice::IMP;
        sim.policy.maintain_immolate = true;
        sim.policy.rotation = RotationChoice::FIRE_DESTRO;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Shadow and Flame (Shadow)")) {
        sim.talents = Talents::create_forever_shadow_and_flame_shadow();
        sim.buffs.sacrifice_succubus = false;
        sim.buffs.sacrifice_imp = false;
        sim.policy.pet = PetChoice::IMP;
        sim.policy.maintain_immolate = true;
        sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("NF/DS/Ruin")) {
        sim.talents = Talents::create_forever_nf_ds_ruin();
        sim.buffs.sacrifice_succubus = false;
        sim.buffs.sacrifice_imp = true;
        sim.policy.pet = PetChoice::NONE;
        sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset All")) {
        sim.talents = Talents();
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Calculate dimensions for the 3 side-by-side trees
    float avail_w = ImGui::GetContentRegionAvail().x;
    float col_spacing = 8.0f;
    float col_w = (avail_w - 2.0f * col_spacing) / 3.0f;
    if (col_w < 250.0f) col_w = 250.0f;

    float avail_h = ImGui::GetContentRegionAvail().y;
    float col_h = std::max(530.0f, avail_h - 8.0f);

    // --- Column 1: Affliction Tree ---
    render_tree_column(
        "Affliction",
        "affliction_bg.png",
        ImVec4(0.45f, 0.75f, 1.0f, 1.0f),
        FOREVER_AFFLICTION_NODES,
        [&](size_t i) { return sim.talents.aff.get_points_by_index(i); },
        [&](size_t i) -> int& { return sim.talents.aff.get_points_by_index(i); },
        [&]() { sim.talents.aff = AfflictionTalents(); },
        sim.talents.aff.total_points(),
        total_pts,
        col_w,
        col_h
    );

    ImGui::SameLine(0.0f, col_spacing);

    // --- Column 2: Demonology Tree ---
    render_tree_column(
        "Demonology",
        "demonology_bg.png",
        ImVec4(0.95f, 0.45f, 0.85f, 1.0f),
        FOREVER_DEMONOLOGY_NODES,
        [&](size_t i) { return sim.talents.demo.get_points_by_index(i); },
        [&](size_t i) -> int& { return sim.talents.demo.get_points_by_index(i); },
        [&]() { sim.talents.demo = DemonologyTalents(); },
        sim.talents.demo.total_points(),
        total_pts,
        col_w,
        col_h
    );

    ImGui::SameLine(0.0f, col_spacing);

    // --- Column 3: Destruction Tree ---
    render_tree_column(
        "Destruction",
        "destruction_bg.png",
        ImVec4(1.0f, 0.60f, 0.20f, 1.0f),
        FOREVER_DESTRUCTION_NODES,
        [&](size_t i) { return sim.talents.destro.get_points_by_index(i); },
        [&](size_t i) -> int& { return sim.talents.destro.get_points_by_index(i); },
        [&]() { sim.talents.destro = DestructionTalents(); },
        sim.talents.destro.total_points(),
        total_pts,
        col_w,
        col_h
    );
}

inline void render_panel_talents(Talents& talents) {
    WarlockSimulator temp_sim;
    temp_sim.talents = talents;
    render_panel_talents(temp_sim);
    talents = temp_sim.talents;
}

} // namespace warlock
