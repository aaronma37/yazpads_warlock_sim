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

  // Priest simulator instance & state
  priest::PriestSimulator priest_sim;
  priest::BatchSimResult priest_last_result;
  int priest_iterations = 10000;

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
    priest_sim.base_attrs = get_base_attributes_for_race(priest_sim.race);

    // Compute player stats from gear or raw manual stats
    Stats priest_stats = priest_sim.use_raw_stats ? priest_sim.raw_stats : priest_sim.gear.calculate_stats();
    priest_sim.buffs.apply_to_stats(
        priest_stats, priest_sim.base_attrs, true, priest_sim.mechanics.shadow_weaving_personal);

    // 5SR Mana Regen calculations
    double spirit_tick = sim::ManaRegenCalculator::calculate_spirit_regen_per_tick(
        priest_stats.intellect, priest_stats.spirit, sim::PlayerClass::PRIEST);
    double outside_5sr = spirit_tick / 2.0;
    double inside_5sr = (spirit_tick * priest_sim.mechanics.meditation_casting_regen_ratio) / 2.0;
    double mp5_mps = priest_stats.mp5 / 5.0;

    if (ImGui::BeginTabBar("PriestTopLayerTabs", ImGuiTabBarFlags_None))
    {
      // 1. Presets / Configuration
      if (ImGui::BeginTabItem("  Presets  "))
      {
        if (ImGui::BeginTabBar("PriestPresetSubTabs", ImGuiTabBarFlags_None))
        {
          // SubTab 1: Build Configuration
          if (ImGui::BeginTabItem("  Build Configuration  "))
          {
            const float pane1_w = 320.0f;
            const float pane2_w = 830.0f;
            const float pane_height = full_height - 35.0f;

            // Pane 1: Stats & Race
            ImGui::BeginChild("PriestPane_Stats", ImVec2(pane1_w, pane_height), true);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Priest Character & Stats");
            ImGui::Separator();

            // Race selection
            const char* race_names[] = { "Human", "Dwarf", "Night Elf", "Undead", "Troll" };
            const Race race_values[] = { Race::HUMAN, Race::DWARF, Race::NIGHT_ELF, Race::UNDEAD, Race::TROLL };
            int cur_race_idx = 0;
            for (int r = 0; r < 5; ++r) {
                if (priest_sim.race == race_values[r]) { cur_race_idx = r; break; }
            }
            if (ImGui::Combo("Race", &cur_race_idx, race_names, 5)) {
                priest_sim.race = race_values[cur_race_idx];
                priest_sim.base_attrs = get_base_attributes_for_race(priest_sim.race);
            }

            ImGui::Spacing();
            ImGui::Text("Stat Presets:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Pre-Raid##Priest")) {
                priest_sim.raw_stats = Stats();
                priest_sim.raw_stats.spell_power = 300.0;
                priest_sim.raw_stats.shadow_power = 80.0;
                priest_sim.raw_stats.spell_hit_percent = 3.0;
                priest_sim.raw_stats.spell_crit_percent = 5.0;
                priest_sim.raw_stats.intellect = 140.0;
                priest_sim.raw_stats.spirit = 180.0;
                priest_sim.raw_stats.stamina = 120.0;
                priest_sim.raw_stats.mp5 = 15.0;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Phase 3/4##Priest")) {
                priest_sim.raw_stats = Stats();
                priest_sim.raw_stats.spell_power = 500.0;
                priest_sim.raw_stats.shadow_power = 120.0;
                priest_sim.raw_stats.spell_hit_percent = 6.0;
                priest_sim.raw_stats.spell_crit_percent = 10.0;
                priest_sim.raw_stats.intellect = 180.0;
                priest_sim.raw_stats.spirit = 240.0;
                priest_sim.raw_stats.stamina = 150.0;
                priest_sim.raw_stats.mp5 = 30.0;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Phase 6 BiS##Priest")) {
                priest_sim.raw_stats = Stats();
                priest_sim.raw_stats.spell_power = 750.0;
                priest_sim.raw_stats.shadow_power = 150.0;
                priest_sim.raw_stats.spell_hit_percent = 16.0;
                priest_sim.raw_stats.spell_crit_percent = 16.0;
                priest_sim.raw_stats.spell_haste_percent = 6.0;
                priest_sim.raw_stats.intellect = 220.0;
                priest_sim.raw_stats.spirit = 300.0;
                priest_sim.raw_stats.stamina = 180.0;
                priest_sim.raw_stats.mp5 = 45.0;
            }

            ImGui::Separator();
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Spell Power##PRaw", &priest_sim.raw_stats.spell_power, 10.0, 50.0, "%.0f");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Shadow Power##PRaw", &priest_sim.raw_stats.shadow_power, 10.0, 50.0, "%.0f");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Holy Power##PRaw", &priest_sim.raw_stats.holy_power, 10.0, 50.0, "%.0f");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Spell Hit %##PRaw", &priest_sim.raw_stats.spell_hit_percent, 1.0, 2.0, "%.1f%%");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Spell Crit %##PRaw", &priest_sim.raw_stats.spell_crit_percent, 1.0, 2.0, "%.1f%%");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Spell Haste %##PRaw", &priest_sim.raw_stats.spell_haste_percent, 1.0, 2.0, "%.1f%%");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("MP5##PRaw", &priest_sim.raw_stats.mp5, 5.0, 10.0, "%.0f");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Intellect##PRaw", &priest_sim.raw_stats.intellect, 10.0, 25.0, "%.0f");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Stamina##PRaw", &priest_sim.raw_stats.stamina, 10.0, 25.0, "%.0f");
            ImGui::SetNextItemWidth(100);
            ImGui::InputDouble("Spirit##PRaw", &priest_sim.raw_stats.spirit, 10.0, 25.0, "%.0f");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Priest Combat Stats Summary:");
            ImGui::Text("Shadow SP: %.0f", priest_stats.effective_shadow_power());
            ImGui::Text("Holy SP: %.0f", priest_stats.effective_holy_power());
            ImGui::Text("Spell Hit: %.1f%% (Cap: 16%%)", priest_stats.spell_hit_percent);
            ImGui::Text("Spell Crit: %.2f%%", priest_stats.total_spell_crit(priest_sim.base_attrs.base_spell_crit));
            ImGui::Text("Max Mana: %.0f", priest_stats.max_mana);
            ImGui::Text("Max Health: %.0f", priest_stats.max_health);
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "5-Second Rule Mana Regeneration:");
            ImGui::Text("Outside 5SR: %.1f mps (%.0f / 5s)", outside_5sr + mp5_mps, (outside_5sr + mp5_mps) * 5.0);
            ImGui::Text("Inside 5SR (Meditation): %.1f mps (%.0f / 5s)", inside_5sr + mp5_mps, (inside_5sr + mp5_mps) * 5.0);
            ImGui::Text("MP5 Contribution: %.1f mps", mp5_mps);
            ImGui::EndChild();

            ImGui::SameLine();

            // Pane 2: Priest Talents
            ImGui::BeginChild("PriestPane_Talents", ImVec2(pane2_w, pane_height), true);
            priest::render_priest_talents_panel(priest_sim.talents);
            ImGui::EndChild();

            ImGui::SameLine();

            // Pane 3: Policy, Target, Buffs, Mechanics
            ImGui::BeginChild("PriestPane_BuffsPolicy", ImVec2(0, pane_height), true);
            priest::render_priest_policy_panel(priest_sim.policy);
            ImGui::Spacing();
            ImGui::Separator();
            render_panel_target(
                priest_sim.target_config, priest_sim.fight_duration, priest_sim.randomize_duration, priest_sim.duration_variance);
            ImGui::Spacing();
            ImGui::Separator();
            render_panel_buffs(priest_sim.buffs);
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
            // Controls
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.30f, 1.0f));
            if (ImGui::Button("RUN PRIEST SIMULATION", ImVec2(220, 36))) {
                priest_last_result = priest::ParallelSimRunner::run_batch(priest_sim, priest_iterations, thread_count);
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SetNextItemWidth(160);
            ImGui::SliderInt("Iterations##Priest", &priest_iterations, 1000, 50000);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(140);
            float p_dur = static_cast<float>(priest_sim.fight_duration);
            if (ImGui::SliderFloat("Duration (s)##Priest", &p_dur, 30.0f, 300.0f, "%.0fs")) {
                priest_sim.fight_duration = p_dur;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            ImGui::SliderInt("Threads##Priest", &thread_count, 1, 32);

            ImGui::Separator();
            ImGui::Spacing();

            // Results Card
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Priest Simulation Results (%d iterations):", priest_last_result.total_iterations);
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "MEAN DPS: %.1f", priest_last_result.mean_dps);
            ImGui::SameLine(250);
            ImGui::Text("Min: %.1f | Max: %.1f | StdDev: %.1f", priest_last_result.min_dps, priest_last_result.max_dps, priest_last_result.std_dev_dps);
            ImGui::Text("p5: %.1f | Median (p50): %.1f | p95: %.1f", priest_last_result.p5_dps, priest_last_result.p50_dps, priest_last_result.p95_dps);
            ImGui::Text("Shadow Weaving Mean Procs: %.1f | Mana Spent: %.0f | Mana Gained: %.0f",
                priest_last_result.mean_sw_weaving_procs, priest_last_result.mean_mana_spent, priest_last_result.mean_mana_gained);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Damage Breakdown:");

            if (ImGui::BeginTable("PriestDmgBreakdown", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("Spell / Ability", ImGuiTableColumnFlags_WidthFixed, 220);
                ImGui::TableSetupColumn("Damage Share (%)", ImGuiTableColumnFlags_WidthFixed, 140);
                ImGui::TableSetupColumn("Visual Bar", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                auto add_breakdown_row = [](const char* name, double pct) {
                    if (pct <= 0.0) return;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(name);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.2f%%", pct * 100.0);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::ProgressBar(static_cast<float>(pct), ImVec2(-1, 16));
                };

                add_breakdown_row("Shadow Word: Pain", priest_last_result.pct_sw_pain);
                add_breakdown_row("Mind Flay", priest_last_result.pct_mind_flay);
                add_breakdown_row("Mind Blast", priest_last_result.pct_mind_blast);
                add_breakdown_row("Shadow Word: Death", priest_last_result.pct_sw_death);
                add_breakdown_row("Devouring Plague", priest_last_result.pct_devouring_plague);
                add_breakdown_row("Smite", priest_last_result.pct_smite);
                add_breakdown_row("Holy Fire", priest_last_result.pct_holy_fire);

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
          }

          ImGui::EndTabBar();
        }
        ImGui::EndTabItem();
      }

      // 2. Abilities (Spellbook)
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
