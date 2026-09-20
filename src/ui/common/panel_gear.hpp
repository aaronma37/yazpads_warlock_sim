#pragma once
#include "asset_manager.hpp"
#include "imgui.h"
#include "rlImGui.h"
#include "src/sim/build_export.hpp"
#include "src/sim/gear.hpp"
#include "src/sim/stats.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/common/mana_regen.hpp"
#include <sstream>
#include <string>
#include <vector>

namespace warlock
{

inline ImVec4 get_item_color(ItemQuality q)
{
  switch (q)
  {
    case ItemQuality::EPIC:
      return ImVec4(0.75f, 0.35f, 1.00f, 1.0f);  // Epic purple
    case ItemQuality::RARE:
      return ImVec4(0.20f, 0.60f, 1.00f, 1.0f);  // Rare blue
    case ItemQuality::UNCOMMON:
      return ImVec4(0.20f, 1.00f, 0.20f, 1.0f);
    default:
      return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
  }
}

inline void render_gear_dropdown_table(GearLoadout& gear)
{
  const Slot all_slots[] = {Slot::HEAD,
                            Slot::NECK,
                            Slot::SHOULDERS,
                            Slot::BACK,
                            Slot::CHEST,
                            Slot::WRISTS,
                            Slot::HANDS,
                            Slot::WAIST,
                            Slot::LEGS,
                            Slot::FEET,
                            Slot::RING1,
                            Slot::RING2,
                            Slot::TRINKET1,
                            Slot::TRINKET2,
                            Slot::MAIN_HAND,
                            Slot::OFF_HAND,
                            Slot::RANGED};

  if (ImGui::BeginTable("GearDropdownTable",
                        2,
                        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
  {
    ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 82);
    ImGui::TableSetupColumn("Equipped Item (Select to Change)", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    for (Slot slot : all_slots)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "%s", slot_to_name(slot));

      ImGui::TableNextColumn();
      const Item& equipped = gear.get(slot);
      std::string preview_name = equipped.name.empty() ? "(Empty Slot)" : equipped.name;
      ImVec4 item_col = equipped.name.empty() ? ImVec4(0.50f, 0.50f, 0.55f, 1.0f) : get_item_color(equipped.quality);

      ImGui::PushID(static_cast<int>(slot));
      ImGui::PushStyleColor(ImGuiCol_Text, item_col);

      if (ImGui::BeginCombo("##SlotCombo", preview_name.c_str()))
      {
        // Option 1: None / Empty
        if (ImGui::Selectable("(Empty Slot)", equipped.name.empty()))
        {
          gear.equip(slot, Item{});
        }

        auto available_items = ItemDatabase::get_items_for_slot(slot);
        for (const auto& it : available_items)
        {
          bool is_selected = (equipped.id == it.id || (equipped.name == it.name && !it.name.empty()));
          ImGui::PushStyleColor(ImGuiCol_Text, get_item_color(it.quality));
          if (ImGui::Selectable(it.name.c_str(), is_selected))
          {
            gear.equip(slot, it);
          }
          ImGui::PopStyleColor();

          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::TextColored(get_item_color(it.quality), "%s (Phase %d)", it.name.c_str(), it.phase);
            ImGui::Separator();
            if (it.spell_power > 0)
              ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "+%.0f Spell Power", it.spell_power);
            if (it.shadow_power > 0)
              ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.0f, 1.0f), "+%.0f Shadow Power", it.shadow_power);
            if (it.fire_power > 0)
              ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "+%.0f Fire Power", it.fire_power);
            if (it.spell_hit > 0)
              ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "+%.0f%% Spell Hit", it.spell_hit);
            if (it.spell_crit > 0)
              ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "+%.0f%% Spell Crit", it.spell_crit);
            if (it.stamina > 0 || it.intellect > 0 || it.spirit > 0)
            {
              ImGui::Text("Attributes: %.0f Stam, %.0f Int, %.0f Spr", it.stamina, it.intellect, it.spirit);
            }
            if (!it.set_name.empty())
            {
              ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Set: %s", it.set_name.c_str());
            }
            if (it.has_on_use)
            {
              ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                                 "Use: +%.0f SP for %.0fs (%.0fs CD)",
                                 it.on_use_spell_power,
                                 it.on_use_duration,
                                 it.on_use_cooldown);
            }
            ImGui::EndTooltip();
          }

          if (is_selected)
          {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      ImGui::PopStyleColor();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
}

template <typename SimType>
inline void render_armory_panel(SimType& sim,
                                const Stats& total_stats,
                                const BaseAttributes& base_attrs,
                                std::string& character_name,
                                int& selected_model_idx,
                                float& build_copied_timer,
                                sim::PlayerClass player_class = sim::PlayerClass::WARLOCK)
{
  GearLoadout& gear = sim.gear;

  // --- CHARACTER RACE SELECTION (SINGLE DROPDOWN) ---
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.10f, 0.16f, 0.90f));
  ImGui::BeginChild("ArmoryHeader", ImVec2(0, 80), true);

  ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "Race:");
  ImGui::SameLine();

  const char* warlock_races[] = {"Undead", "Orc", "Troll", "Human", "Gnome"};
  const Race warlock_race_vals[] = {Race::UNDEAD, Race::ORC, Race::TROLL, Race::HUMAN, Race::GNOME};

  const char* priest_races[] = {"Human", "Dwarf", "Night Elf", "Undead", "Troll"};
  const Race priest_race_vals[] = {Race::HUMAN, Race::DWARF, Race::NIGHT_ELF, Race::UNDEAD, Race::TROLL};

  const char** race_names = (player_class == sim::PlayerClass::PRIEST) ? priest_races : warlock_races;
  const Race* race_vals = (player_class == sim::PlayerClass::PRIEST) ? priest_race_vals : warlock_race_vals;
  int num_races = 5;

  int current_race_idx = 0;
  for (int i = 0; i < num_races; ++i)
  {
    if (sim.race == race_vals[i])
    {
      current_race_idx = i;
      break;
    }
  }

  ImGui::SetNextItemWidth(140);
  if (ImGui::Combo("##RaceSelectCombo", &current_race_idx, race_names, num_races))
  {
    sim.race = race_vals[current_race_idx];
    selected_model_idx = current_race_idx;
    sim.base_attrs = get_base_attributes_for_race(sim.race);
  }

  switch (sim.race)
  {
    case Race::HUMAN:
      ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "Sword Spec (+2%% Crit w/ Swords)");
      ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "Spirit (+5%%)");
      break;
    case Race::GNOME:
      ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Expansive Mind (+5%% Mana)");
      ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Eureka! (-50%% Mana, +10%% Dmg 3 casts)");
      break;
    case Race::ORC:
      ImGui::TextColored(ImVec4(1.0f, 0.50f, 0.30f, 1.0f), "Blood Fury (+10%% SP for 15s)");
      break;
    case Race::UNDEAD:
      ImGui::TextColored(ImVec4(0.70f, 0.90f, 0.60f, 1.0f),
                         "Touch of the Grave (10%% chance to drain up to 5%% Max HP)");
      break;
    case Race::TROLL:
      ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.80f, 1.0f), "Berserking (+10%% Haste for 10s)");
      ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.80f, 1.0f), "Beast Slaying (+5%% vs Beasts)");
      break;
    case Race::DWARF:
      ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "Stoneform (+10%% Armor, Bleed/Poison Immune)");
      ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.40f, 1.0f), "Frost Resistance (+10)");
      break;
    case Race::NIGHT_ELF:
      ImGui::TextColored(ImVec4(0.50f, 0.80f, 1.0f, 1.0f), "Quickness (+1%% Dodge)");
      ImGui::TextColored(ImVec4(0.50f, 0.80f, 1.0f, 1.0f), "Shadowmeld (Stealth)");
      break;
  }

  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();

  // Mode Switcher: Equipped Gear vs Direct Stats
  float avail_w = ImGui::GetContentRegionAvail().x;
  float mode_btn_w = (avail_w - 6.0f) * 0.5f;

  bool in_raw_mode = sim.use_raw_stats;
  if (in_raw_mode)
  {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 1.0f, 0.60f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
  }
  else
  {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.12f, 0.18f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.28f, 0.45f, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  }
  if (ImGui::Button("DIRECT STAT VALUES", ImVec2(mode_btn_w, 28)))
  {
    sim.use_raw_stats = true;
    if (sim.raw_stats.spell_power == 0.0 && sim.raw_stats.shadow_power == 0.0)
    {
      sim.raw_stats = sim.gear.calculate_stats();
    }
  }
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);

  ImGui::SameLine();

  bool in_gear_mode = !sim.use_raw_stats;
  if (in_gear_mode)
  {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42f, 0.20f, 0.68f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.85f, 0.20f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
  }
  else
  {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.12f, 0.18f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.28f, 0.45f, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  }
  if (ImGui::Button("EQUIPPED ITEMS", ImVec2(mode_btn_w, 28)))
  {
    sim.use_raw_stats = false;
  }
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);

  ImGui::Spacing();

  if (!sim.use_raw_stats)
  {
    // Quick tier buttons for equipped gear
    if (ImGui::SmallButton("Pre-Raid"))
      gear = GearLoadout::create_preraid_bis();
    ImGui::SameLine();
    if (ImGui::SmallButton("Phase 3/4"))
      gear = GearLoadout::create_phase3_bis();
    ImGui::SameLine();
    if (ImGui::SmallButton("Phase 5"))
      gear = GearLoadout::create_phase5_bis();
    ImGui::SameLine();
    if (ImGui::SmallButton("Phase 6 BiS"))
      gear = GearLoadout::create_phase6_bis();

    ImGui::Spacing();
    render_gear_dropdown_table(gear);
  }
  else
  {
    // Direct Stats Mode
    ImGui::Text("Stat Presets:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Pre-Raid"))
    {
      sim.raw_stats = Stats();
      if (player_class == sim::PlayerClass::PRIEST)
      {
        sim.raw_stats.spell_power = 300.0;
        sim.raw_stats.shadow_power = 80.0;
        sim.raw_stats.spell_hit_percent = 3.0;
        sim.raw_stats.spell_crit_percent = 5.0;
        sim.raw_stats.intellect = 140.0;
        sim.raw_stats.spirit = 180.0;
        sim.raw_stats.stamina = 120.0;
        sim.raw_stats.mp5 = 15.0;
      }
      else
      {
        sim.raw_stats.spell_power = 320.0;
        sim.raw_stats.spell_hit_percent = 3.0;
        sim.raw_stats.spell_crit_percent = 4.0;
        sim.raw_stats.intellect = 110.0;
        sim.raw_stats.stamina = 130.0;
        sim.raw_stats.spirit = 50.0;
      }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Phase 3/4"))
    {
      sim.raw_stats = Stats();
      if (player_class == sim::PlayerClass::PRIEST)
      {
        sim.raw_stats.spell_power = 500.0;
        sim.raw_stats.shadow_power = 120.0;
        sim.raw_stats.spell_hit_percent = 6.0;
        sim.raw_stats.spell_crit_percent = 10.0;
        sim.raw_stats.intellect = 180.0;
        sim.raw_stats.spirit = 240.0;
        sim.raw_stats.stamina = 150.0;
        sim.raw_stats.mp5 = 30.0;
      }
      else
      {
        sim.raw_stats.spell_power = 520.0;
        sim.raw_stats.spell_hit_percent = 8.0;
        sim.raw_stats.spell_crit_percent = 10.0;
        sim.raw_stats.intellect = 140.0;
        sim.raw_stats.stamina = 160.0;
        sim.raw_stats.spirit = 60.0;
      }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Phase 6 BiS"))
    {
      sim.raw_stats = Stats();
      if (player_class == sim::PlayerClass::PRIEST)
      {
        sim.raw_stats.spell_power = 750.0;
        sim.raw_stats.shadow_power = 150.0;
        sim.raw_stats.spell_hit_percent = 16.0;
        sim.raw_stats.spell_crit_percent = 16.0;
        sim.raw_stats.spell_haste_percent = 6.0;
        sim.raw_stats.intellect = 220.0;
        sim.raw_stats.spirit = 300.0;
        sim.raw_stats.stamina = 180.0;
        sim.raw_stats.mp5 = 45.0;
      }
      else
      {
        sim.raw_stats.spell_power = 780.0;
        sim.raw_stats.spell_hit_percent = 16.0;
        sim.raw_stats.spell_crit_percent = 18.0;
        sim.raw_stats.spell_haste_percent = 8.0;
        sim.raw_stats.intellect = 180.0;
        sim.raw_stats.stamina = 200.0;
        sim.raw_stats.spirit = 75.0;
      }
    }

    if (ImGui::Button("Import From Gear", ImVec2(130, 22)))
    {
      sim.raw_stats = sim.gear.calculate_stats();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to 0", ImVec2(90, 22)))
    {
      sim.raw_stats = Stats();
    }

    ImGui::Separator();
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("Spell Power##Raw", &sim.raw_stats.spell_power, 10.0, 50.0, "%.0f");
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("Shadow Power##Raw", &sim.raw_stats.shadow_power, 10.0, 50.0, "%.0f");
    if (player_class == sim::PlayerClass::PRIEST)
    {
      ImGui::SetNextItemWidth(100);
      ImGui::InputDouble("Holy Power##Raw", &sim.raw_stats.holy_power, 10.0, 50.0, "%.0f");
    }
    else
    {
      ImGui::SetNextItemWidth(100);
      ImGui::InputDouble("Fire Power##Raw", &sim.raw_stats.fire_power, 10.0, 50.0, "%.0f");
    }
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("Spell Hit %##Raw", &sim.raw_stats.spell_hit_percent, 1.0, 2.0, "%.1f%%");
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("Spell Crit %##Raw", &sim.raw_stats.spell_crit_percent, 1.0, 2.0, "%.1f%%");
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("Spell Haste %##Raw", &sim.raw_stats.spell_haste_percent, 1.0, 2.0, "%.1f%%");
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("MP5##Raw", &sim.raw_stats.mp5, 5.0, 10.0, "%.0f");
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("Intellect##Raw", &sim.raw_stats.intellect, 10.0, 25.0, "%.0f");
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("Stamina##Raw", &sim.raw_stats.stamina, 10.0, 25.0, "%.0f");
    ImGui::SetNextItemWidth(100);
    ImGui::InputDouble("Spirit##Raw", &sim.raw_stats.spirit, 10.0, 25.0, "%.0f");
  }

  // Character Attributes & Spell Stats Summary
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Combat Stats Summary:");

  ImGui::Text("Shadow SP: %.0f", total_stats.effective_shadow_power());
  if (player_class == sim::PlayerClass::PRIEST)
  {
    ImGui::Text("Holy SP: %.0f", total_stats.effective_holy_power());
  }
  else
  {
    ImGui::Text("Fire SP: %.0f", total_stats.effective_fire_power());
  }
  ImGui::Text("Spell Hit: %.1f%% (Cap: 16%%)", total_stats.spell_hit_percent);
  ImGui::Text("Spell Crit: %.2f%%", total_stats.total_spell_crit(base_attrs.base_spell_crit));
  ImGui::Text("Max Mana: %.0f", total_stats.max_mana);
  ImGui::Text("Max Health: %.0f", total_stats.max_health);
  ImGui::Text("MP5: %.0f", total_stats.mp5);
  ImGui::Text("Int: %.0f", total_stats.intellect);
  ImGui::Text("Stamina: %.0f", total_stats.stamina);
  ImGui::Text("Spirit: %.0f", total_stats.spirit);
  ImGui::Text("Shadow Mult: %.3fx", total_stats.shadow_multiplier * total_stats.all_damage_multiplier);
  if (player_class == sim::PlayerClass::PRIEST)
  {
    ImGui::Text("Holy Mult: %.3fx", total_stats.holy_multiplier * total_stats.all_damage_multiplier);

    // 5SR Mana Regen breakdown for Priest
    double spirit_tick = sim::ManaRegenCalculator::calculate_spirit_regen_per_tick(
        total_stats.intellect, total_stats.spirit, sim::PlayerClass::PRIEST);
    double outside_5sr = spirit_tick / 2.0;
    double med_ratio = 0.15;
    if constexpr (requires { sim.mechanics.meditation_casting_regen_ratio; }) {
      med_ratio = sim.mechanics.meditation_casting_regen_ratio;
    }
    double inside_5sr = (spirit_tick * med_ratio) / 2.0;
    double mp5_mps = total_stats.mp5 / 5.0;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "5-Second Rule Mana Regeneration:");
    ImGui::Text("Outside 5SR: %.1f mps (%.0f / 5s)", outside_5sr + mp5_mps, (outside_5sr + mp5_mps) * 5.0);
    ImGui::Text("Inside 5SR (Meditation): %.1f mps (%.0f / 5s)", inside_5sr + mp5_mps, (inside_5sr + mp5_mps) * 5.0);
    ImGui::Text("MP5 Contribution: %.1f mps", mp5_mps);
  }
  else
  {
    ImGui::Text("Fire Mult: %.3fx", total_stats.fire_multiplier * total_stats.all_damage_multiplier);
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  float avail_btn_w = ImGui::GetContentRegionAvail().x;
  if (ImGui::Button("Copy Build to Clipboard", ImVec2(avail_btn_w, 26)))
  {
    if constexpr (std::is_same_v<SimType, WarlockSimulator>)
    {
      ImGui::SetClipboardText(build_export::export_build_json(sim).c_str());
      build_copied_timer = 3.0f;
    }
    else
    {
      std::ostringstream json;
      json << "{\n";
      json << "  \"format\": \"priest-build/1\",\n";
      json << "  \"race\": \"" << race_to_string(sim.race) << "\",\n";
      json << "  \"fight_duration\": " << sim.fight_duration << ",\n";
      json << "  \"stats_mode\": \"" << (sim.use_raw_stats ? "raw" : "gear") << "\"\n";
      json << "}\n";
      ImGui::SetClipboardText(json.str().c_str());
      build_copied_timer = 3.0f;
    }
  }
  if (build_copied_timer > 0.0f)
  {
    build_copied_timer -= ImGui::GetIO().DeltaTime;
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Build copied to clipboard!");
  }
}

}  // namespace warlock
