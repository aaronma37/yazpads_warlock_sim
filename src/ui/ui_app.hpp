#pragma once
#include "imgui.h"
#include "implot.h"
#include "raylib.h"
#include "rlImGui.h"

#include "asset_manager.hpp"
#include "panel_buffs.hpp"
#include "panel_gear.hpp"
#include "panel_imp_analysis.hpp"
#include "panel_isb_analysis.hpp"
#include "panel_known_issues.hpp"
#include "panel_mechanics.hpp"
#include "panel_mechanics_tab.hpp"
#include "panel_optimizer.hpp"
#include "panel_policy.hpp"
#include "panel_results.hpp"
#include "panel_sim_control.hpp"
#include "panel_spellbook.hpp"
#include "panel_talents.hpp"
#include "panel_target.hpp"
#include "panel_theorycrafting.hpp"
#include "ui_theme.hpp"

#include "src/sim/build_export.hpp"
#include "src/sim/optimizer.hpp"
#include "src/sim/parallel_runner.hpp"
#include "src/sim/spec_presets.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/common/player_class.hpp"
#include "src/sim/priest/priest_sim.hpp"
#include "src/sim/priest/parallel_runner.hpp"
#include "src/sim/priest/spec_presets.hpp"
#include "src/sim/priest/talents.hpp"
#include "src/sim/priest/spells.hpp"
#include "src/ui/priest/panel_talents.hpp"
#include "src/ui/priest/panel_policy.hpp"
#include "src/ui/priest/panel_spellbook.hpp"
#include "src/ui/priest/panel_mechanics.hpp"
#include "src/ui/priest/panel_sim_control.hpp"
#include "src/ui/priest/panel_results.hpp"
#include "src/sim/priest/optimizer.hpp"
#include "src/ui/priest/panel_optimizer.hpp"

namespace warlock
{

class WarlockSimApp
{
 public:
  sim::PlayerClass active_class = sim::PlayerClass::WARLOCK;

  WarlockSimulator sim;
  BatchSimResult last_result;
  std::vector<CandidateResult> optimizer_results;

  int iterations = 10000;
  int thread_count = 0;

  bool is_sim_running = false;
  float sim_progress = 0.0f;

  bool is_optimizing = false;
  float opt_progress = 0.0f;
  std::string opt_task_name;

  bool request_switch_to_preset = false;

  float build_copied_timer = 0.0f;

  std::string character_name = "Grimmortis";
  int selected_model_idx = 3;  // 3 = Human (0 = Undead, 1 = Orc, 2 = Troll, 3 = Human, 4 = Gnome)

  std::string priest_character_name = "Benedictus";
  int priest_model_idx = 0;  // 0 = Human (0 = Human, 1 = Dwarf, 2 = Night Elf, 3 = Undead, 4 = Troll)
  float priest_build_copied_timer = 0.0f;

  // Priest simulator instance & state
  priest::PriestSimulator priest_sim;
  priest::BatchSimResult priest_last_result;
  int priest_iterations = 10000;
  bool is_priest_sim_running = false;
  float priest_sim_progress = 0.0f;

  std::vector<priest::CandidateResult> priest_optimizer_results;
  bool is_priest_optimizing = false;
  float priest_opt_progress = 0.0f;
  std::string priest_opt_task_name;
  bool request_priest_switch_to_preset = false;

  WarlockSimApp()
  {
    thread_count = static_cast<int>(std::thread::hardware_concurrency());
    if (thread_count <= 0)
      thread_count = 4;
    sim.fight_duration = 180.0;
    sim.randomize_duration = true;
    sim.duration_variance = 30.0;

    priest_sim.fight_duration = 180.0;
    priest_sim.randomize_duration = true;
    priest_sim.duration_variance = 30.0;
  }

  void run_gui()
  {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1650, 960, "Classic WoW Warlock DES Simulator & Multi-Threaded Armory");
    SetTargetFPS(60);

    rlImGuiSetup(true);
    ImPlot::CreateContext();
    AssetManager::get().init();
    apply_warlock_theme();

    // Initial baseline runs
    last_result = ParallelSimRunner::run_batch(sim, 5000, thread_count);
    priest_last_result = priest::ParallelSimRunner::run_batch(priest_sim, 2000, thread_count);

    while (!WindowShouldClose())
    {
      BeginDrawing();
      ClearBackground(Color{14, 12, 18, 255});

      rlImGuiBegin();

      // Fixed Fullscreen Canvas (No floating / draggable windows!)
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())));
      ImGuiWindowFlags root_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

      if (ImGui::Begin("RootFixedCanvas", nullptr, root_flags))
      {
        // -------------------------------------------------------------------------
        // Top Header Bar: Class Switcher & Character Profile
        // -------------------------------------------------------------------------
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        if (active_class == sim::PlayerClass::WARLOCK)
        {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.20f, 0.65f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.80f, 0.50f, 1.0f, 1.0f));
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
        }
        else
        {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.14f, 0.20f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.30f, 0.45f, 0.6f));
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        }
        if (ImGui::Button("WARLOCK", ImVec2(120, 26)))
        {
          active_class = sim::PlayerClass::WARLOCK;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        if (active_class == sim::PlayerClass::PRIEST)
        {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.90f, 0.95f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.10f, 0.10f, 0.15f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
        }
        else
        {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.14f, 0.20f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.90f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.30f, 0.45f, 0.6f));
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        }
        if (ImGui::Button("PRIEST", ImVec2(120, 26)))
        {
          active_class = sim::PlayerClass::PRIEST;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar();  // FrameRounding

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        if (active_class == sim::PlayerClass::WARLOCK)
        {
          ImGui::TextColored(ImVec4(0.75f, 0.55f, 1.0f, 1.0f), "Class: Warlock (%s)", character_name.c_str());
        }
        else
        {
          ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.80f, 1.0f), "Class: Priest (WoW Forever / Hyjal)");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const float full_height = ImGui::GetContentRegionAvail().y;

        if (active_class == sim::PlayerClass::WARLOCK)
        {
          render_warlock_view(full_height);
        }
        else
        {
          render_priest_view(full_height);
        }

        ImGui::End();
      }
      ImGui::PopStyleVar(3);

      rlImGuiEnd();
      EndDrawing();
    }

    AssetManager::get().shutdown();
    ImPlot::DestroyContext();
    rlImGuiShutdown();
    CloseWindow();
  }

  void render_warlock_view(float full_height)
  {
    // Ensure base_attrs is synced to sim.race
    sim.base_attrs = get_base_attributes_for_race(sim.race);

    // Compute player combat stats from gear (or raw manual stats) and buffs
    Stats player_stats = sim.use_raw_stats ? sim.raw_stats : sim.gear.calculate_stats();
    sim.buffs.apply_to_stats(
        player_stats, sim.base_attrs, true, sim.mechanics.personal_shadow_weaving);  // true = WoW Forever mechanics

    // Demonic Embrace (+3% Total Stamina per point, up to +15%)
    if (sim.talents.demo.demonic_embrace > 0)
    {
      double stam_bonus_mult = 1.0 + sim.talents.demo.demonic_embrace * 0.03;
      player_stats.stamina *= stam_bonus_mult;
      player_stats.max_health = sim.base_attrs.base_health + player_stats.stamina * 10.0;
      if (sim.buffs.flask_of_the_titans)
        player_stats.max_health += 1200.0;
    }

    // Gnome Expansive Mind (+5% Mana)
    if (sim.race == Race::GNOME)
    {
      player_stats.max_mana *= 1.05;
    }

    // Fel Vitality (+5% Max Mana per point)
    if (sim.talents.demo.fel_vitality > 0)
    {
      player_stats.max_mana *= (1.0 + sim.talents.demo.fel_vitality * 0.05);
    }

    // Blood Pact (Rank 5: +54 Stamina when Imp is active)
    PetChoice ui_active_pet = sim.policy.pet;
    if (sim.buffs.sacrifice_succubus || sim.buffs.sacrifice_imp)
    {
      if (sim.talents.demo.demonic_pact > 0)
      {
        if (sim.buffs.sacrifice_imp && sim.policy.pet == PetChoice::IMP)
          ui_active_pet = PetChoice::NONE;
        else if (sim.buffs.sacrifice_succubus && sim.policy.pet == PetChoice::SUCCUBUS)
          ui_active_pet = PetChoice::NONE;
      }
      else
      {
        ui_active_pet = PetChoice::NONE;
      }
    }
    if (ui_active_pet == PetChoice::IMP)
    {
      double blood_pact_stamina = 54.0;
      if (sim.talents.demo.demonic_embrace > 0)
      {
        blood_pact_stamina *= (1.0 + sim.talents.demo.demonic_embrace * 0.03);
      }
      player_stats.stamina += blood_pact_stamina;
      player_stats.max_health += blood_pact_stamina * 10.0;
    }

    // Human Sword Specialization (+2% crit)
    bool is_sword = false;
    if (!sim.use_raw_stats)
    {
      const Item& mh = sim.gear.get(Slot::MAIN_HAND);
      if (mh.name.find("Mageblade") != std::string::npos || mh.name.find("Sword") != std::string::npos ||
          mh.name.find("Blade") != std::string::npos || mh.icon.find("sword") != std::string::npos ||
          mh.icon.find("Sword") != std::string::npos)
      {
        is_sword = true;
      }
    }
    else
    {
      is_sword = true;
    }
    if (sim.race == Race::HUMAN && is_sword)
    {
      player_stats.spell_crit_percent += 2.0;
    }

    // Orc Axe Specialization (+1% crit)
    bool is_axe = false;
    if (!sim.use_raw_stats)
    {
      const Item& mh = sim.gear.get(Slot::MAIN_HAND);
      if (mh.name.find("Axe") != std::string::npos || mh.icon.find("axe") != std::string::npos)
      {
        is_axe = true;
      }
    }
    if (sim.race == Race::ORC && is_axe)
    {
      player_stats.spell_crit_percent += 1.0;
    }

    // If candidate configuration was applied from combinatorial sim, refresh baseline
    if (request_switch_to_preset)
    {
      last_result = ParallelSimRunner::run_batch(sim, 5000, thread_count);
    }

    // =========================================================================
    // TOP-LEVEL HIERARCHICAL TABS
    // =========================================================================
    if (ImGui::BeginTabBar("TopLayerTabs", ImGuiTabBarFlags_None))
    {
      // 1. PRESET SIMULATION (Inspector, Talents, Gear, Buffs, APL, Sim)
      ImGuiTabItemFlags preset_flags = 0;
      if (request_switch_to_preset)
      {
        preset_flags |= ImGuiTabItemFlags_SetSelected;
      }

      if (ImGui::BeginTabItem("  Presets  ", nullptr, preset_flags))
      {
        if (request_switch_to_preset)
        {
          request_switch_to_preset = false;
        }

        if (ImGui::BeginTabBar("PresetSubTabs", ImGuiTabBarFlags_None))
        {
          // SUBTAB 1: BUILD CONFIGURATION (Gear, Talents, Buffs, Rotation)
          if (ImGui::BeginTabItem("  Build Configuration  "))
          {
            const float pane1_w = 320.0f;                   // Gear & Direct Stats
            const float pane2_w = 830.0f;                   // Talents Tree (51 Points - All 3 Trees Visible)
            const float pane_height = full_height - 35.0f;  // Available content height

            // Pane 1: Gear & Direct Stats
            ImGui::BeginChild("PresetPane_Gear", ImVec2(pane1_w, pane_height), true);
            render_armory_panel(
                sim, player_stats, sim.base_attrs, character_name, selected_model_idx, build_copied_timer);
            ImGui::EndChild();

            ImGui::SameLine();

            // Pane 2: Talent Tree (51 Points)
            ImGui::BeginChild(
                "PresetPane_Talents", ImVec2(pane2_w, pane_height), true, ImGuiWindowFlags_HorizontalScrollbar);
            render_panel_talents(sim);
            ImGui::EndChild();

            ImGui::SameLine();

            // Pane 3: Rotation Policy, Target Encounter, Consumables & Buffs, Mechanics
            ImGui::BeginChild("PresetPane_BuffsPolicy", ImVec2(0, pane_height), true);
            render_panel_policy(sim);
            ImGui::Spacing();
            ImGui::Separator();
            render_panel_target(
                sim.target_config, sim.fight_duration, sim.randomize_duration, sim.duration_variance);
            ImGui::Spacing();
            ImGui::Separator();
            render_panel_buffs(sim);
            ImGui::Spacing();
            ImGui::Separator();
            render_panel_mechanics(sim.mechanics);
            ImGui::EndChild();

            ImGui::EndTabItem();
          }

          // SUBTAB 2: COMBAT SIMULATION & RESULTS (Controls, Histograms, Logs)
          if (ImGui::BeginTabItem("  Combat Simulation & Results  "))
          {
            ImGui::Spacing();
            render_panel_sim_control(sim, iterations, thread_count, last_result, is_sim_running, sim_progress);
            ImGui::Separator();
            render_panel_results(last_result);
            ImGui::EndTabItem();
          }

          ImGui::EndTabBar();
        }

        ImGui::EndTabItem();
      }

      // 2. SIMULATE (Brute-Force Optimizer)
      if (ImGui::BeginTabItem("  Simulate  "))
      {
        ImGui::Spacing();
        render_panel_optimizer(
            sim, optimizer_results, is_optimizing, opt_progress, opt_task_name, &request_switch_to_preset);
        ImGui::EndTabItem();
      }

      // 3. SPELLBOOK (Spell Database, Ranks, Base Stats, Coefficients)
      if (ImGui::BeginTabItem("  Abilities  "))
      {
        ImGui::Spacing();
        render_panel_spellbook();
        ImGui::EndTabItem();
      }

      // 4. THEORYCRAFTING (Mathematical Proofs & Dominance Theorems)
      if (ImGui::BeginTabItem("  Theorycrafting  "))
      {
        ImGui::Spacing();
        render_panel_theorycrafting(sim);
        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }

  void render_priest_view(float full_height)
  {
    // Ensure base_attrs is synced to priest_sim.race
    priest_sim.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, priest_sim.race);

    // Compute player stats from gear or raw manual stats
    Stats priest_stats = priest_sim.use_raw_stats ? priest_sim.raw_stats : priest_sim.gear.calculate_stats();
    priest_sim.buffs.apply_to_stats(
        priest_stats, priest_sim.base_attrs, true, priest_sim.mechanics.shadow_weaving_personal);

    // If candidate configuration was applied from optimizer, refresh baseline
    if (request_priest_switch_to_preset)
    {
      priest_last_result = priest::ParallelSimRunner::run_batch(priest_sim, 2500, thread_count);
    }

    if (ImGui::BeginTabBar("PriestTopLayerTabs", ImGuiTabBarFlags_None))
    {
      // 1. Presets / Configuration
      ImGuiTabItemFlags preset_flags = 0;
      if (request_priest_switch_to_preset)
      {
        preset_flags |= ImGuiTabItemFlags_SetSelected;
      }

      if (ImGui::BeginTabItem("  Presets  ", nullptr, preset_flags))
      {
        if (request_priest_switch_to_preset)
        {
          request_priest_switch_to_preset = false;
        }

        if (ImGui::BeginTabBar("PriestPresetSubTabs", ImGuiTabBarFlags_None))
        {
          // SubTab 1: Build Configuration
          if (ImGui::BeginTabItem("  Build Configuration  "))
          {
            const float pane1_w = 320.0f;
            const float pane2_w = 830.0f;
            const float pane_height = full_height - 35.0f;

            // Pane 1: Gear & Direct Stats
            ImGui::BeginChild("PriestPane_Stats", ImVec2(pane1_w, pane_height), true);
            render_armory_panel(
                priest_sim, priest_stats, priest_sim.base_attrs, priest_character_name, priest_model_idx, priest_build_copied_timer, sim::PlayerClass::PRIEST);
            ImGui::EndChild();

            ImGui::SameLine();

            // Pane 2: Priest Talents
            ImGui::BeginChild(
                "PriestPane_Talents", ImVec2(pane2_w, pane_height), true, ImGuiWindowFlags_HorizontalScrollbar);
            priest::render_priest_talents_panel(priest_sim);
            ImGui::EndChild();

            ImGui::SameLine();

            // Pane 3: Policy, Target, Buffs, Mechanics
            ImGui::BeginChild("PriestPane_BuffsPolicy", ImVec2(0, pane_height), true);
            priest::render_priest_policy_panel(priest_sim.policy, priest_sim.talents, priest_sim.race);
            ImGui::Spacing();
            ImGui::Separator();
            render_panel_target(
                priest_sim.target_config, priest_sim.fight_duration, priest_sim.randomize_duration, priest_sim.duration_variance);
            ImGui::Spacing();
            ImGui::Separator();
            render_panel_buffs(priest_sim);
            ImGui::Spacing();
            ImGui::Separator();
            priest::render_priest_mechanics_panel(priest_sim.mechanics);
            ImGui::EndChild();

            ImGui::EndTabItem();
          }

          // SubTab 2: Combat Simulation & Results
          if (ImGui::BeginTabItem("  Combat Simulation & Results  "))
          {
            ImGui::Spacing();
            priest::render_priest_sim_control(
                priest_sim, priest_iterations, thread_count, priest_last_result, is_priest_sim_running, priest_sim_progress);
            ImGui::Separator();
            priest::render_priest_panel_results(priest_last_result);
            ImGui::EndTabItem();
          }

          ImGui::EndTabBar();
        }
        ImGui::EndTabItem();
      }

      // 2. Simulate (Optimizer)
      if (ImGui::BeginTabItem("  Simulate  "))
      {
        ImGui::Spacing();
        priest::render_priest_panel_optimizer(
            priest_sim, priest_optimizer_results, is_priest_optimizing, priest_opt_progress, priest_opt_task_name, &request_priest_switch_to_preset);
        ImGui::EndTabItem();
      }

      // 3. Abilities (Spellbook)
      if (ImGui::BeginTabItem("  Abilities  "))
      {
        ImGui::Spacing();
        priest::render_priest_spellbook_panel();
        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }
};

}  // namespace warlock
