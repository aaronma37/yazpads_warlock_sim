#pragma once
#include "imgui.h"
#include "rlImGui.h"
#include "src/sim/priest/talents.hpp"
#include "src/sim/priest/spec_presets.hpp"
#include "src/ui/common/asset_manager.hpp"
#include <algorithm>
#include <array>
#include <string>

namespace priest {

inline void render_priest_talents_panel(Talents& talents) {
    ImGui::BeginChild("PriestTalentsPanel", ImVec2(0, 0), true);

    int total_spent = talents.total_points();
    int remaining = 51 - total_spent;

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Priest Talent Trees");
    ImGui::SameLine();
    ImGui::Text("Points Left: %d / 51", remaining);
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset All")) {
        talents = Talents{};
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Shadow (13/0/38)")) {
        talents = Talents::create_forever_shadow();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Smite (14/37/0)")) {
        talents = Talents::create_forever_smite();
    }

    ImGui::Separator();

    auto render_tree_column = [&](const char* title, const auto& nodes, auto get_pts, auto set_pts, int tree_pts) {
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.5f, 1.0f), "%s (%d pts)", title, tree_pts);
        ImGui::BeginChild(title, ImVec2(0, 360), true);
        for (size_t i = 0; i < nodes.size(); ++i) {
            const auto& n = nodes[i];
            int pts = get_pts(i);
            ImGui::PushID(static_cast<int>(i));

            std::string label = std::string(n.name) + " (" + std::to_string(pts) + "/" + std::to_string(n.max_points) + ")";
            if (ImGui::Button("-") && pts > 0) {
                set_pts(i, pts - 1);
            }
            ImGui::SameLine();
            if (ImGui::Button("+") && pts < n.max_points && remaining > 0) {
                set_pts(i, pts + 1);
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(label.c_str());

            if (ImGui::IsItemHovered() && pts <= n.max_points && pts > 0 && n.desc[pts - 1]) {
                ImGui::SetTooltip("%s", n.desc[pts - 1]);
            } else if (ImGui::IsItemHovered() && n.desc[0]) {
                ImGui::SetTooltip("%s", n.desc[0]);
            }

            ImGui::PopID();
        }
        ImGui::EndChild();
    };

    if (ImGui::BeginTable("PriestTalentColumns", 3, ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextColumn();
        render_tree_column(
            "Discipline",
            get_disc_nodes(),
            [&](size_t i){ return talents.disc.get_points_by_index(i); },
            [&](size_t i, int v){ talents.disc.get_points_by_index(i) = v; },
            talents.disc.total_points()
        );

        ImGui::TableNextColumn();
        render_tree_column(
            "Holy",
            get_holy_nodes(),
            [&](size_t i){ return talents.holy.get_points_by_index(i); },
            [&](size_t i, int v){ talents.holy.get_points_by_index(i) = v; },
            talents.holy.total_points()
        );

        ImGui::TableNextColumn();
        render_tree_column(
            "Shadow",
            get_shadow_nodes(),
            [&](size_t i){ return talents.shadow.get_points_by_index(i); },
            [&](size_t i, int v){ talents.shadow.get_points_by_index(i) = v; },
            talents.shadow.total_points()
        );

        ImGui::EndTable();
    }

    ImGui::EndChild();
}

} // namespace priest
