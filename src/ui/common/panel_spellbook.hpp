#pragma once
#include "asset_manager.hpp"
#include "imgui.h"
#include <algorithm>
#include <string>
#include <vector>

namespace warlock
{

struct CommonSpellBookEntry
{
  std::string name;
  std::string rank;
  std::string school_str;
  std::string base_cast_str;
  std::string base_mana_str;
  std::string direct_dmg_str;
  std::string dot_dmg_str;
  std::string coeff_str;
  std::string sim_formula;
  std::string icon_name;
};

inline void render_unified_spellbook_table(const char* table_id,
                                           const char* title,
                                           const char* search_hint,
                                           char* search_filter,
                                           size_t filter_buf_size,
                                           const std::vector<CommonSpellBookEntry>& entries)
{
  ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", title);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::SetNextItemWidth(300);
  ImGui::InputTextWithHint(
      "##SpellSearch", search_hint, search_filter, filter_buf_size);
  ImGui::SameLine();
  if (ImGui::Button("Clear"))
  {
    search_filter[0] = '\0';
  }

  ImGui::Spacing();

  ImGuiTableFlags flags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable(table_id, 8, flags, ImVec2(0, 0)))
  {
    ImGui::TableSetupColumn("Spell", ImGuiTableColumnFlags_WidthFixed, 180.0f);
    ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 70.0f);
    ImGui::TableSetupColumn("School", ImGuiTableColumnFlags_WidthFixed, 75.0f);
    ImGui::TableSetupColumn("Cast Time", ImGuiTableColumnFlags_WidthFixed, 105.0f);
    ImGui::TableSetupColumn("Mana Cost", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Base Damage / Effect", ImGuiTableColumnFlags_WidthFixed, 260.0f);
    ImGui::TableSetupColumn("SP Coefficient", ImGuiTableColumnFlags_WidthFixed, 115.0f);
    ImGui::TableSetupColumn("Formula", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    std::string query = search_filter;
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    for (const auto& sp : entries)
    {
      std::string name_lower = sp.name;
      std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
      std::string school_lower = sp.school_str;
      std::transform(school_lower.begin(), school_lower.end(), school_lower.begin(), ::tolower);

      if (!query.empty() && name_lower.find(query) == std::string::npos &&
          school_lower.find(query) == std::string::npos)
      {
        continue;
      }

      ImGui::TableNextRow();

      // Col 0: Icon + Name
      ImGui::TableSetColumnIndex(0);
      Texture2D icon = AssetManager::get().get_icon(sp.icon_name);
      if (icon.id > 0)
      {
        ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(20, 20));
        ImGui::SameLine(0, 6);
      }
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.70f, 1.0f), "%s", sp.name.c_str());

      // Col 1: Rank
      ImGui::TableSetColumnIndex(1);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.80f, 1.0f), "%s", sp.rank.c_str());

      // Col 2: School
      ImGui::TableSetColumnIndex(2);
      ImGui::AlignTextToFramePadding();
      if (sp.school_str == "Shadow")
      {
        ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "Shadow");
      }
      else if (sp.school_str == "Fire")
      {
        ImGui::TextColored(ImVec4(1.0f, 0.50f, 0.20f, 1.0f), "Fire");
      }
      else if (sp.school_str == "Holy")
      {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "Holy");
      }
      else if (sp.school_str == "Arcane")
      {
        ImGui::TextColored(ImVec4(0.40f, 0.80f, 1.0f, 1.0f), "Arcane");
      }
      else if (sp.school_str == "Discipline")
      {
        ImGui::TextColored(ImVec4(0.80f, 0.85f, 1.0f, 1.0f), "Discipline");
      }
      else
      {
        ImGui::TextColored(ImVec4(0.80f, 0.80f, 0.80f, 1.0f), "%s", sp.school_str.c_str());
      }

      // Col 3: Cast Time
      ImGui::TableSetColumnIndex(3);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("%s", sp.base_cast_str.c_str());

      // Col 4: Mana Cost
      ImGui::TableSetColumnIndex(4);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.0f, 1.0f), "%s", sp.base_mana_str.c_str());

      // Col 5: Base Damage / Effect
      ImGui::TableSetColumnIndex(5);
      ImGui::AlignTextToFramePadding();
      if (!sp.direct_dmg_str.empty() && sp.direct_dmg_str != "None")
      {
        ImGui::Text("%s", sp.direct_dmg_str.c_str());
      }
      if (!sp.dot_dmg_str.empty() && sp.dot_dmg_str != "None")
      {
        ImGui::TextColored(ImVec4(0.50f, 0.90f, 0.50f, 1.0f), "%s", sp.dot_dmg_str.c_str());
      }

      // Col 6: SP Coefficient
      ImGui::TableSetColumnIndex(6);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.40f, 1.0f), "%s", sp.coeff_str.c_str());

      // Col 7: Formula
      ImGui::TableSetColumnIndex(7);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "%s", sp.sim_formula.c_str());
    }

    ImGui::EndTable();
  }
}

}  // namespace warlock
