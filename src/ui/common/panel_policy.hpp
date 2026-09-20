#pragma once
#include "asset_manager.hpp"
#include "imgui.h"
#include <string>
#include <vector>

namespace warlock
{

struct CommonPriorityRule
{
  std::string icon_name;
  std::string name;
  std::string condition_summary;
  std::string trigger_condition;
  std::string rule_explanation;
  ImU32 bg_col = IM_COL32(56, 46, 76, 240);
  ImU32 border_col = IM_COL32(140, 90, 204, 230);
};

inline void render_common_priority_chain(const std::vector<CommonPriorityRule>& rules)
{
  ImGui::TextColored(ImVec4(0.40f, 0.85f, 1.0f, 1.0f), "Action Priority Chain:");

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.08f, 0.15f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.25f, 0.70f, 0.80f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));

  if (ImGui::BeginChild("CommonPriorityRuleChainBox",
                        ImVec2(0, 0),
                        ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar))
  {
    float avail_width = ImGui::GetContentRegionAvail().x;
    float cur_x = 0.0f;
    const float badge_w = 26.0f;
    const float badge_h = 26.0f;
    const float icon_sz = 20.0f;
    const float arrow_w = 16.0f;

    for (size_t i = 0; i < rules.size(); ++i)
    {
      const auto& rule = rules[i];
      ImGui::PushID(static_cast<int>(i));

      Texture2D icon = AssetManager::get().get_icon(rule.icon_name);
      float total_w = badge_w + ((i + 1 < rules.size()) ? (arrow_w + 6.0f) : 0.0f);

      if (i > 0)
      {
        if (cur_x + total_w > avail_width && cur_x > 0.0f)
        {
          ImGui::NewLine();
          cur_x = 0.0f;
        }
        else
        {
          ImGui::SameLine(0, 4);
          cur_x += 4.0f;
        }
      }

      std::string btn_label = "##RuleBtn_" + std::to_string(i);
      ImVec2 p0 = ImGui::GetCursorScreenPos();
      ImGui::InvisibleButton(btn_label.c_str(), ImVec2(badge_w, badge_h));
      bool is_hovered = ImGui::IsItemHovered();
      ImVec2 p1 = ImVec2(p0.x + badge_w, p0.y + badge_h);

      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      ImU32 fill_col = is_hovered ? IM_COL32(80, 70, 110, 250) : rule.bg_col;
      draw_list->AddRectFilled(p0, p1, fill_col, 4.0f);
      draw_list->AddRect(p0, p1, is_hovered ? IM_COL32(255, 255, 255, 255) : rule.border_col, 4.0f, 0, 1.0f);

      float icon_x = p0.x + (badge_w - icon_sz) * 0.5f;
      float icon_y = p0.y + (badge_h - icon_sz) * 0.5f;
      if (icon.id > 0)
      {
        draw_list->AddImage(
            (ImTextureID)(uintptr_t)icon.id, ImVec2(icon_x, icon_y), ImVec2(icon_x + icon_sz, icon_y + icon_sz));
      }

      if (is_hovered)
      {
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

        if (!rule.condition_summary.empty())
        {
          ImGui::Separator();
          ImGui::TextDisabled("Condition: %s", rule.condition_summary.c_str());
        }
        ImGui::EndTooltip();
      }

      cur_x += badge_w;

      if (i + 1 < rules.size())
      {
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

}  // namespace warlock
