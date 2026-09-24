#pragma once
#include "asset_manager.hpp"
#include "imgui.h"
#include "rlImGui.h"
#include "ui_theme.hpp"
#include "wow_widgets.hpp"
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
    case ItemQuality::COMMON:
      return wow_colors::QualityCommon;
    case ItemQuality::UNCOMMON:
      return wow_colors::QualityUncommon;
    case ItemQuality::RARE:
      return wow_colors::QualityRare;
    case ItemQuality::EPIC:
      return wow_colors::QualityEpic;
    case ItemQuality::LEGENDARY:
      return wow_colors::QualityLegendary;
    default:
      return wow_colors::ParchmentText;
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
                                int& selected_model_idx,
                                float& build_copied_timer,
                                sim::PlayerClass player_class = sim::PlayerClass::WARLOCK)
{
  GearLoadout& gear = sim.gear;

  // --- CHARACTER RACE, PET, DEMONIC SACRIFICE & RACIAL ICONS ---
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.10f, 0.16f, 0.90f));
  ImGui::BeginChild("ArmoryHeader", ImVec2(0, 78), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

  static bool race_selector_expanded = false;
  static bool pet_selector_expanded = false;
  static bool ds_selector_expanded = false;

  constexpr float kIconSize = 34.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

  auto draw_slot_btn = [&](const char* id, const char* icon_file, bool is_none, bool is_sel, const char* tooltip) {
    if (is_sel)
    {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.18f, 0.12f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.00f, 0.82f, 0.20f, 1.0f));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    }
    else
    {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.09f, 0.07f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.28f, 0.18f, 0.7f));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }

    bool clicked = false;
    if (is_none || !icon_file || icon_file[0] == '\0')
    {
      clicked = ImGui::Button(id, ImVec2(kIconSize, kIconSize));
    }
    else
    {
      const Texture2D& tex = AssetManager::get().get_icon(icon_file);
      clicked = rlImGuiImageButtonSize(id, &tex, Vector2{kIconSize, kIconSize});
    }

    if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip("%s", tooltip);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    return clicked;
  };

  if (race_selector_expanded)
  {
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Select Race");

    const char* warlock_races[] = {"Undead", "Orc", "Troll", "Human", "Gnome"};
    const Race warlock_race_vals[] = {Race::UNDEAD, Race::ORC, Race::TROLL, Race::HUMAN, Race::GNOME};
    const char* priest_races[] = {"Human", "Dwarf", "Night Elf", "Gnome", "Undead", "Troll"};
    const Race priest_race_vals[] = {Race::HUMAN, Race::DWARF, Race::NIGHT_ELF, Race::GNOME, Race::UNDEAD, Race::TROLL};

    const char** race_names = (player_class == sim::PlayerClass::PRIEST) ? priest_races : warlock_races;
    const Race* race_vals = (player_class == sim::PlayerClass::PRIEST) ? priest_race_vals : warlock_race_vals;
    int num_races = (player_class == sim::PlayerClass::PRIEST) ? 6 : 5;

    for (int i = 0; i < num_races; ++i)
    {
      if (i > 0)
        ImGui::SameLine();
      bool is_sel = (sim.race == race_vals[i]);
      const std::string race_btn_id = std::string("##RaceOpt_") + race_names[i];
      if (draw_slot_btn(race_btn_id.c_str(), race_to_icon(race_vals[i]), false, is_sel, race_names[i]))
      {
        sim.race = race_vals[i];
        selected_model_idx = i;
        sim.base_attrs = sim::get_base_attributes_for_class_and_race(player_class, sim.race);
        race_selector_expanded = false;
      }
    }
  }
  else if (pet_selector_expanded)
  {
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Select Pet");

    if constexpr (requires { sim.policy.pet; sim.buffs.sacrifice_imp; })
    {
      struct PetDef { PetChoice pet; const char* icon; const char* name; const char* desc; };
      PetDef pet_opts[] = {
        {PetChoice::NONE, nullptr, "None", "No active demon pet summoned (Empty Slot)"},
        {PetChoice::IMP, "Spell_Shadow_SummonImp.png", "Imp", "Active Imp (Firebolt auto-cast)"},
        {PetChoice::SUCCUBUS, "Spell_Shadow_SummonSuccubus.png", "Succubus", "Active Succubus (Melee swings + Lash of Pain)"}
      };

      for (int i = 0; i < 3; ++i)
      {
        if (i > 0)
          ImGui::SameLine();
        bool is_sel = (sim.policy.pet == pet_opts[i].pet);
        bool is_none = (pet_opts[i].pet == PetChoice::NONE);
        const std::string btn_id = std::string("##PetOpt_") + pet_opts[i].name;
        std::string tip = std::string(pet_opts[i].name) + "\n" + pet_opts[i].desc;
        if (draw_slot_btn(btn_id.c_str(), pet_opts[i].icon, is_none, is_sel, tip.c_str()))
        {
          sim.policy.pet = pet_opts[i].pet;
          // Mutual exclusion constraint: Pet and DS cannot be the same demon
          if (sim.policy.pet == PetChoice::IMP && sim.buffs.sacrifice_imp)
          {
            sim.buffs.sacrifice_imp = false;
          }
          else if (sim.policy.pet == PetChoice::SUCCUBUS && sim.buffs.sacrifice_succubus)
          {
            sim.buffs.sacrifice_succubus = false;
          }
          pet_selector_expanded = false;
        }
      }
    }
  }
  else if (ds_selector_expanded)
  {
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Select Demonic Sacrifice");

    if constexpr (requires { sim.buffs.sacrifice_imp; sim.buffs.sacrifice_succubus; sim.policy.pet; })
    {
      struct DsDef { int id; const char* icon; const char* name; const char* desc; };
      DsDef ds_opts[] = {
        {0, nullptr, "None", "No Demonic Sacrifice active (Empty Slot)"},
        {1, "Spell_Shadow_SummonImp.png", "Sacrifice Imp", "+15% Shadow Damage in Forever (Sacrificed Imp)"},
        {2, "Spell_Shadow_SummonSuccubus.png", "Sacrifice Succubus", "+15% Fire Damage in Forever (Sacrificed Succubus)"}
      };

      for (int i = 0; i < 3; ++i)
      {
        if (i > 0)
          ImGui::SameLine();
        bool is_sel = (i == 0 && !sim.buffs.sacrifice_imp && !sim.buffs.sacrifice_succubus) ||
                      (i == 1 && sim.buffs.sacrifice_imp) ||
                      (i == 2 && sim.buffs.sacrifice_succubus);
        bool is_none = (i == 0);
        const std::string btn_id = std::string("##DsOpt_") + ds_opts[i].name;
        std::string tip = std::string(ds_opts[i].name) + "\n" + ds_opts[i].desc;
        if (draw_slot_btn(btn_id.c_str(), ds_opts[i].icon, is_none, is_sel, tip.c_str()))
        {
          if (i == 0) {
            sim.buffs.sacrifice_imp = false;
            sim.buffs.sacrifice_succubus = false;
          } else if (i == 1) {
            sim.buffs.sacrifice_imp = true;
            sim.buffs.sacrifice_succubus = false;
            // Mutual exclusion constraint: Pet and DS cannot be the same demon
            if (sim.policy.pet == PetChoice::IMP)
            {
              sim.policy.pet = PetChoice::NONE;
            }
          } else if (i == 2) {
            sim.buffs.sacrifice_imp = false;
            sim.buffs.sacrifice_succubus = true;
            // Mutual exclusion constraint: Pet and DS cannot be the same demon
            if (sim.policy.pet == PetChoice::SUCCUBUS)
            {
              sim.policy.pet = PetChoice::NONE;
            }
          }
          ds_selector_expanded = false;
        }
      }
    }
  }
  else
  {
    // Collapsed standard view: Race, Pet, and DS side-by-side | Divider | Racials
    // 1. Race
    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Race");
    std::string race_tip = std::string("Race: ") + race_to_string(sim.race) + " (Click to change)";
    if (draw_slot_btn("##SelectedRace", race_to_icon(sim.race), false, true, race_tip.c_str()))
    {
      race_selector_expanded = true;
      pet_selector_expanded = false;
      ds_selector_expanded = false;
    }
    ImGui::EndGroup();

    if constexpr (requires { sim.policy.pet; sim.buffs.sacrifice_imp; })
    {
      if (player_class == sim::PlayerClass::WARLOCK)
      {
        ImGui::SameLine(0.0f, 12.0f);

        // 2. Pet
        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Pet");
        bool pet_is_none = (sim.policy.pet == PetChoice::NONE);
        const char* pet_icon = pet_is_none ? nullptr : (sim.policy.pet == PetChoice::IMP ? "Spell_Shadow_SummonImp.png" : "Spell_Shadow_SummonSuccubus.png");
        std::string pet_tip = std::string("Active Pet: ") + pet_choice_to_string(sim.policy.pet) + " (Click to change)";
        if (draw_slot_btn("##SelectedPet", pet_icon, pet_is_none, !pet_is_none, pet_tip.c_str()))
        {
          pet_selector_expanded = true;
          race_selector_expanded = false;
          ds_selector_expanded = false;
        }
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 12.0f);

        // 3. DS (Demonic Sacrifice)
        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "DS");
        bool ds_is_none = (!sim.buffs.sacrifice_imp && !sim.buffs.sacrifice_succubus);
        const char* ds_icon = ds_is_none ? nullptr : (sim.buffs.sacrifice_imp ? "Spell_Shadow_SummonImp.png" : "Spell_Shadow_SummonSuccubus.png");
        const char* ds_name = ds_is_none ? "None" : (sim.buffs.sacrifice_imp ? "Sacrificed Imp (+15% Shadow Damage)" : "Sacrificed Succubus (+15% Fire Damage)");
        std::string ds_tip = std::string("Demonic Sacrifice: ") + ds_name + " (Click to change)";
        if (draw_slot_btn("##SelectedDS", ds_icon, ds_is_none, !ds_is_none, ds_tip.c_str()))
        {
          ds_selector_expanded = true;
          race_selector_expanded = false;
          pet_selector_expanded = false;
        }
        ImGui::EndGroup();
      }
    }

    // Vertical Divider Line
    ImGui::SameLine(0.0f, 12.0f);
    ImVec2 div_p0 = ImGui::GetCursorScreenPos();
    float div_h = kIconSize + 16.0f;
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(div_p0.x, div_p0.y - 2.0f),
        ImVec2(div_p0.x, div_p0.y + div_h),
        IM_COL32(90, 72, 40, 200), 1.5f);
    ImGui::Dummy(ImVec2(1.5f, div_h));

    ImGui::SameLine(0.0f, 12.0f);

    // 4. Racials
    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.0f, 1.0f), "Racials");

    struct RacialTraitInfo {
      const char* name;
      const char* icon;
      const char* desc;
    };

    std::vector<RacialTraitInfo> racials;
    if (player_class == sim::PlayerClass::PRIEST)
    {
      switch (sim.race)
      {
        case Race::HUMAN:
          racials.push_back({"The Human Spirit", "Spell_Holy_MagicalSentry.png", "+5% Spirit"});
          racials.push_back({"Divine Grace", "Spell_Holy_Restoration.png", "Emergency Heal <50% HP"});
          racials.push_back({"Feedback", "Spell_Shadow_ManaBurn.png", "Anti-Magic Mana Burn & Dmg"});
          break;
        case Race::DWARF:
          racials.push_back({"Stoneform", "Spell_Shadow_UnholyStrength.png", "+10% Armor, Bleed/Poison Immune"});
          racials.push_back({"Chastise", "INV_Misc_QuestionMark.png", "272-306 Holy Dmg, 2s Immobilize"});
          racials.push_back({"Desperate Prayer", "Spell_Holy_Restoration.png", "Instant self-heal 1285-1513"});
          break;
        case Race::NIGHT_ELF:
          racials.push_back({"Starshards", "Spell_Arcane_StarFire.png", "1800 Arcane Dmg over 6s (30s CD)"});
          racials.push_back({"Elune's Grace", "Spell_Holy_ElunesGrace.png", "-50% Attack Hit Chance for 15s"});
          racials.push_back({"Shadowmeld", "INV_Misc_QuestionMark.png", "Stealth | Quickness (+1% Dodge)"});
          break;
        case Race::GNOME:
          racials.push_back({"Expansive Mind", "INV_Enchant_EssenceEternalLarge.png", "+5% Mana"});
          racials.push_back({"Contingency Plan", "Spell_Holy_PowerWordShield.png", "Emergency Shield & Heal"});
          racials.push_back({"Confounding Flash", "Spell_Shadow_MindSteal.png", "AoE Confuse 5 Enemies for 3s"});
          break;
        case Race::UNDEAD:
          racials.push_back({"Dark Sacrifice", "Ability_Racial_Cannibalize.png", "Cannibalize 1600 HP -> 1600 Mana"});
          racials.push_back({"Touch of the Grave", "Spell_Shadow_LifeDrain02.png", "Drain up to 5% Max HP"});
          racials.push_back({"Will of the Forsaken", "Spell_Shadow_RaiseDead.png", "Charm/Fear/Sleep Immune"});
          break;
        case Race::TROLL:
          racials.push_back({"Berserking", "Racial_Troll_Berserk.png", "+10% Haste for 10s"});
          racials.push_back({"Shadowguard", "Spell_Shadow_ManaBurn.png", "3 Charges: 96 Shadow Dmg Retaliation"});
          racials.push_back({"Beast Slaying", "Ability_Hunter_BeastSoothe.png", "+5% vs Beasts"});
          break;
        default:
          break;
      }
    }
    else
    {
      switch (sim.race)
      {
        case Race::HUMAN:
          racials.push_back({"Sword Spec", "INV_Sword_27.png", "+2% Crit w/ Swords"});
          racials.push_back({"The Human Spirit", "Spell_Holy_MagicalSentry.png", "+5% Spirit"});
          break;
        case Race::GNOME:
          racials.push_back({"Expansive Mind", "INV_Enchant_EssenceEternalLarge.png", "+5% Mana"});
          racials.push_back({"Eureka!", "Spell_Nature_WispSplode.png", "-50% Mana, +10% Dmg for 3 casts"});
          break;
        case Race::ORC:
          racials.push_back({"Blood Fury", "Racial_Orc_BerserkerStrength.png", "+10% SP for 15s"});
          break;
        case Race::UNDEAD:
          racials.push_back({"Touch of the Grave", "Spell_Shadow_LifeDrain02.png", "10% chance to drain up to 5% Max HP"});
          racials.push_back({"Will of the Forsaken", "Spell_Shadow_RaiseDead.png", "Charm/Fear/Sleep Immune"});
          break;
        case Race::TROLL:
          racials.push_back({"Berserking", "Racial_Troll_Berserk.png", "+10% Haste for 10s"});
          racials.push_back({"Beast Slaying", "Ability_Hunter_BeastSoothe.png", "+5% vs Beasts"});
          break;
        case Race::DWARF:
          racials.push_back({"Stoneform", "Spell_Shadow_UnholyStrength.png", "+10% Armor, Bleed/Poison Immune"});
          break;
        case Race::NIGHT_ELF:
          racials.push_back({"Shadowmeld", "INV_Misc_QuestionMark.png", "Stealth | Quickness (+1% Dodge)"});
          break;
      }
    }

    for (size_t i = 0; i < racials.size(); ++i)
    {
      if (i > 0) ImGui::SameLine(0.0f, 6.0f);
      const Texture2D& r_tex = AssetManager::get().get_icon(racials[i].icon);
      const std::string r_id = std::string("##Racial_") + racials[i].name;

      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.09f, 0.07f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 0.32f, 0.20f, 0.7f));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

      rlImGuiImageButtonSize(r_id.c_str(), &r_tex, Vector2{kIconSize, kIconSize});
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("%s\n%s", racials[i].name, racials[i].desc);
      }

      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);
    }
    ImGui::EndGroup();
  }
  ImGui::PopStyleVar(2);  // FrameRounding + FramePadding

  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();

  // Mode Switcher: Equipped Gear vs Direct Stats
  float avail_w = ImGui::GetContentRegionAvail().x;
  float mode_btn_w = (avail_w - 6.0f) * 0.5f;

  bool in_raw_mode = sim.use_raw_stats;
  if (WowButton(in_raw_mode ? "DIRECT STATS" : "DIRECT STAT VALUES", ImVec2(mode_btn_w, 28)))
  {
    sim.use_raw_stats = true;
    if (sim.raw_stats.spell_power == 0.0 && sim.raw_stats.shadow_power == 0.0)
    {
      sim.raw_stats = sim.gear.calculate_stats();
    }
  }

  ImGui::SameLine();

  bool in_gear_mode = !sim.use_raw_stats;
  if (WowButton("EQUIPPED ITEMS", ImVec2(mode_btn_w, 28)))
  {
    sim.use_raw_stats = false;
  }

  ImGui::Spacing();

  if (!sim.use_raw_stats)
  {
    // Quick tier buttons for equipped gear
    if (WowButton("Pre-Raid"))
      gear = GearLoadout::create_preraid_bis();
    ImGui::SameLine();
    if (WowButton("Phase 3/4"))
      gear = GearLoadout::create_phase3_bis();
    ImGui::SameLine();
    if (WowButton("Phase 5"))
      gear = GearLoadout::create_phase5_bis();
    ImGui::SameLine();
    if (WowButton("Phase 6 BiS"))
      gear = GearLoadout::create_phase6_bis();

    ImGui::Spacing();
    render_gear_dropdown_table(gear);
  }
  else
  {
    // Direct Stats Mode
    ImGui::Separator();
    ImGui::Spacing();

    auto render_stat_entry = [](const char* label, const char* id, double* val, const char* fmt = "%.0f") {
      WowResetTextBaseline();
      ImGui::TextColored(ImVec4(0.92f, 0.85f, 0.72f, 1.0f), "%s", label);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.5f));
      ImGui::SetNextItemWidth(-1.0f);
      WowInputDouble(id, val, 0.0, 0.0, fmt);
      ImGui::PopStyleVar();
    };

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 2.0f));
    if (ImGui::BeginTable("DirectStatsGrid", 2, ImGuiTableFlags_SizingStretchSame))
    {
      ImGui::TableNextRow(ImGuiTableRowFlags_None, 38.0f);
      ImGui::TableNextColumn();
      render_stat_entry("Spell Power", "##RawSpellPower", &sim.raw_stats.spell_power, "%.0f");
      ImGui::TableNextColumn();
      render_stat_entry("Shadow Power", "##RawShadowPower", &sim.raw_stats.shadow_power, "%.0f");

      ImGui::TableNextRow(ImGuiTableRowFlags_None, 38.0f);
      ImGui::TableNextColumn();
      if (player_class == sim::PlayerClass::PRIEST)
      {
        render_stat_entry("Holy Power", "##RawHolyPower", &sim.raw_stats.holy_power, "%.0f");
      }
      else
      {
        render_stat_entry("Fire Power", "##RawFirePower", &sim.raw_stats.fire_power, "%.0f");
      }
      ImGui::TableNextColumn();
      render_stat_entry("Spell Hit %", "##RawSpellHit", &sim.raw_stats.spell_hit_percent, "%.1f%%");

      ImGui::TableNextRow(ImGuiTableRowFlags_None, 38.0f);
      ImGui::TableNextColumn();
      render_stat_entry("Spell Crit %", "##RawSpellCrit", &sim.raw_stats.spell_crit_percent, "%.1f%%");
      ImGui::TableNextColumn();
      render_stat_entry("Spell Haste %", "##RawSpellHaste", &sim.raw_stats.spell_haste_percent, "%.1f%%");

      ImGui::TableNextRow(ImGuiTableRowFlags_None, 38.0f);
      ImGui::TableNextColumn();
      render_stat_entry("Intellect", "##RawIntellect", &sim.raw_stats.intellect, "%.0f");
      ImGui::TableNextColumn();
      render_stat_entry("Stamina", "##RawStamina", &sim.raw_stats.stamina, "%.0f");

      ImGui::TableNextRow(ImGuiTableRowFlags_None, 38.0f);
      ImGui::TableNextColumn();
      render_stat_entry("Spirit", "##RawSpirit", &sim.raw_stats.spirit, "%.0f");
      ImGui::TableNextColumn();
      render_stat_entry("MP5", "##RawMP5", &sim.raw_stats.mp5, "%.0f");

      ImGui::EndTable();
    }
    ImGui::PopStyleVar();
  }
}

template <typename SimType>
inline void render_armory_panel(SimType& sim,
                                int& selected_model_idx,
                                sim::PlayerClass player_class = sim::PlayerClass::WARLOCK)
{
  float dummy_timer = 0.0f;
  Stats dummy_stats;
  BaseAttributes dummy_attrs;
  render_armory_panel(sim, dummy_stats, dummy_attrs, selected_model_idx, dummy_timer, player_class);
}

template <typename SimType>
inline void render_combat_stats_summary(SimType& sim,
                                        const Stats& total_stats,
                                        const BaseAttributes& base_attrs,
                                        float& build_copied_timer,
                                        sim::PlayerClass player_class = sim::PlayerClass::WARLOCK)
{
  if (WowCollapsingHeader("Combat Stats Summary", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::Indent(8.0f);

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
  if (total_stats.spell_haste_percent > 0.0)
  {
    ImGui::Text("Spell Haste: %.1f%%", total_stats.spell_haste_percent);
  }
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

    ImGui::Unindent(8.0f);
  }
}

}  // namespace warlock
