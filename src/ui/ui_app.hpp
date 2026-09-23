#pragma once
#include "imgui.h"
#include "implot.h"
#include "raylib.h"
#include "rlImGui.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

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
#include "wow_widgets.hpp"

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

  int selected_model_idx = 3;  // 3 = Human (0 = Undead, 1 = Orc, 2 = Troll, 3 = Human, 4 = Gnome)

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
#if defined(__EMSCRIPTEN__)
    thread_count = 1;
#else
    thread_count = static_cast<int>(std::thread::hardware_concurrency());
    if (thread_count <= 0)
      thread_count = 4;
#endif
    sim.fight_duration = 180.0;
    sim.randomize_duration = true;
    sim.duration_variance = 30.0;

    priest_sim.fight_duration = 180.0;
    priest_sim.randomize_duration = true;
    priest_sim.duration_variance = 30.0;
  }

  enum class AppTab {
    PRESETS = 0,
    SIMULATE = 1,
    ABILITIES = 2,
    THEORYCRAFTING = 3
  };

  AppTab active_tab = AppTab::PRESETS;
  AppTab priest_active_tab = AppTab::PRESETS;

  void render_top_navigation_tabs(float target_bottom_y)
  {
    const float tab_h = 32.0f;
    const float tab_spacing = 6.0f;

    struct TabDef {
      const char* label;
      AppTab tab;
      float width;
    };

    std::vector<TabDef> tabs;
    if (active_class == sim::PlayerClass::WARLOCK) {
      tabs = {
        {"Presets", AppTab::PRESETS, 100.0f},
        {"Simulate", AppTab::SIMULATE, 100.0f},
        {"Abilities", AppTab::ABILITIES, 100.0f},
        {"Theorycrafting", AppTab::THEORYCRAFTING, 136.0f}
      };
    } else {
      tabs = {
        {"Presets", AppTab::PRESETS, 100.0f},
        {"Simulate", AppTab::SIMULATE, 100.0f},
        {"Abilities", AppTab::ABILITIES, 100.0f}
      };
    }

    float total_tabs_w = 0.0f;
    for (size_t i = 0; i < tabs.size(); ++i) {
      total_tabs_w += tabs[i].width;
      if (i > 0) total_tabs_w += tab_spacing;
    }

    float win_w = ImGui::GetWindowWidth();
    float start_x = win_w - total_tabs_w - 16.0f;
    if (start_x < 120.0f) start_x = 120.0f;

    ImGui::SameLine(start_x);
    // Align bottom of tab buttons exactly flush with the top of panels below
    ImGui::SetCursorPosY(target_bottom_y - tab_h);

    AppTab& cur_tab = (active_class == sim::PlayerClass::WARLOCK) ? active_tab : priest_active_tab;

    for (size_t i = 0; i < tabs.size(); ++i) {
      if (i > 0) {
        ImGui::SameLine(0.0f, tab_spacing);
      }
      bool is_selected = (cur_tab == tabs[i].tab);
      if (WowTabButton(tabs[i].label, is_selected, tabs[i].width, tab_h)) {
        cur_tab = tabs[i].tab;
      }
    }
  }

  void render_frame()
  {
#if defined(__EMSCRIPTEN__)
    int cur_w = EM_ASM_INT( return window.innerWidth; );
    int cur_h = EM_ASM_INT( return window.innerHeight; );
    if (cur_w > 0 && cur_h > 0 && (cur_w != GetScreenWidth() || cur_h != GetScreenHeight()))
    {
      SetWindowSize(cur_w, cur_h);
    }
#endif

    BeginDrawing();
    ClearBackground(Color{14, 12, 18, 255});

    rlImGuiBegin();

    // Fixed Fullscreen Canvas (No floating / draggable windows!)
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())));
    ImGuiWindowFlags root_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    if (ImGui::Begin("RootFixedCanvas", nullptr, root_flags))
    {
      // Draw seamless Classic WoW dark stone/parchment background
      DrawWowDialogBackdrop(ImGui::GetWindowDrawList(), ImVec2(0, 0), ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())), false);

      // -------------------------------------------------------------------------
      // Top Header Bar: Class Switcher (Left) & Right-Justified Navigation Tabs
      // -------------------------------------------------------------------------
      const Texture2D& warlock_icon = AssetManager::get().get_icon(sim::player_class_to_icon(sim::PlayerClass::WARLOCK));
      const Texture2D& priest_icon = AssetManager::get().get_icon(sim::player_class_to_icon(sim::PlayerClass::PRIEST));
      constexpr float kClassIconSize = 36.0f;
      const float top_bar_bottom_y = ImGui::GetCursorPosY() + kClassIconSize + 4.0f;

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
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
      if (rlImGuiImageButtonSize("##ClassWarlock", &warlock_icon, Vector2{kClassIconSize, kClassIconSize}))
      {
        active_class = sim::PlayerClass::WARLOCK;
      }
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Warlock");
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);

      ImGui::SameLine();

      if (active_class == sim::PlayerClass::PRIEST)
      {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.90f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
      }
      else
      {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.14f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.30f, 0.45f, 0.6f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
      }
      if (rlImGuiImageButtonSize("##ClassPriest", &priest_icon, Vector2{kClassIconSize, kClassIconSize}))
      {
        active_class = sim::PlayerClass::PRIEST;
      }
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Priest");
      ImGui::PopStyleVar();
      ImGui::PopStyleColor(2);

      ImGui::PopStyleVar(2);  // FrameRounding + FramePadding

      // Render right-justified tabs on the same row, flush with the panel below
      render_top_navigation_tabs(top_bar_bottom_y);

      // Start main view flush at top_bar_bottom_y
      ImGui::SetCursorPosY(top_bar_bottom_y);

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

  void run_gui()
  {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
#if defined(__EMSCRIPTEN__)
    int init_w = EM_ASM_INT( return window.innerWidth; );
    int init_h = EM_ASM_INT( return window.innerHeight; );
    if (init_w <= 0) init_w = 1650;
    if (init_h <= 0) init_h = 960;
    InitWindow(init_w, init_h, "Classic WoW Warlock DES Simulator & Multi-Threaded Armory");
#else
    InitWindow(1650, 960, "Classic WoW Warlock DES Simulator & Multi-Threaded Armory");
#endif
    SetTargetFPS(60);

    rlImGuiSetLoadFontsCallback(load_wow_fonts);
    rlImGuiSetup(true);
    ImPlot::CreateContext();
    AssetManager::get().init();
    apply_wow_theme();

    // Initial baseline runs
    last_result = ParallelSimRunner::run_batch(sim, 2000, thread_count);
    priest_last_result = priest::ParallelSimRunner::run_batch(priest_sim, 1000, thread_count);

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg([](void* arg) {
      static_cast<WarlockSimApp*>(arg)->render_frame();
    }, this, 0, 1);
#else
    while (!WindowShouldClose())
    {
      render_frame();
    }

    AssetManager::get().shutdown();
    ImPlot::DestroyContext();
    rlImGuiShutdown();
    CloseWindow();
#endif
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
      request_switch_to_preset = false;
      active_tab = AppTab::PRESETS;
      last_result = ParallelSimRunner::run_batch(sim, 5000, thread_count);
    }

    switch (active_tab)
    {
      case AppTab::PRESETS:
      {
        const float pane1_w = 330.0f;                   // Column 1: Left (~330px)
        const float pane2_w = 360.0f;                   // Column 2: Middle (~360px)
        const float pane_height = full_height;

        // Column 1 (Left, ~330px): Gear & Direct Stats, Combat Policy
        BeginWowChild("PresetPane_Col1", ImVec2(pane1_w, pane_height), true);
        render_armory_panel(
            sim, selected_model_idx, sim::PlayerClass::WARLOCK);
        ImGui::Spacing();
        ImGui::Separator();
        render_panel_policy(sim);
        EndWowChild();

        ImGui::SameLine();

        // Column 2 (Middle, ~360px): Combat Stats Summary collapsible header, followed by collapsible panels: Consumables & Elixirs, Raid Buffs, World Buffs, Raid Debuffs, and Game Mechanics
        BeginWowChild("PresetPane_Col2", ImVec2(pane2_w, pane_height), true);
        render_combat_stats_summary(
            sim, player_stats, sim.base_attrs, build_copied_timer, sim::PlayerClass::WARLOCK);
        ImGui::Spacing();
        render_panel_buffs(sim);
        ImGui::Spacing();
        render_panel_mechanics(sim.mechanics);
        EndWowChild();

        ImGui::SameLine();

        // Column 3 (Talents on top, Sim Config & Simulation Results sharing panel below):
        BeginWowChild("PresetPane_Col3", ImVec2(0, pane_height), true, ImGuiWindowFlags_HorizontalScrollbar);
        render_panel_talents(sim);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        render_panel_target(
            sim, iterations, thread_count, last_result, is_sim_running, sim_progress);
        EndWowChild();
        break;
      }

      case AppTab::SIMULATE:
      {
        render_panel_optimizer(
            sim, optimizer_results, is_optimizing, opt_progress, opt_task_name, &request_switch_to_preset);
        break;
      }

      case AppTab::ABILITIES:
      {
        render_panel_spellbook();
        break;
      }

      case AppTab::THEORYCRAFTING:
      {
        render_panel_theorycrafting(sim);
        break;
      }
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
      request_priest_switch_to_preset = false;
      priest_active_tab = AppTab::PRESETS;
      priest_last_result = priest::ParallelSimRunner::run_batch(priest_sim, 2500, thread_count);
    }

    switch (priest_active_tab)
    {
      case AppTab::PRESETS:
      {
        const float pane1_w = 330.0f;                   // Column 1: Left (~330px)
        const float pane2_w = 360.0f;                   // Column 2: Middle (~360px)
        const float pane_height = full_height;

        // Column 1 (Left, ~330px): Gear, Direct Stats, Combat Policy
        BeginWowChild("PriestPane_Col1", ImVec2(pane1_w, pane_height), true);
        render_armory_panel(
            priest_sim, priest_model_idx, sim::PlayerClass::PRIEST);
        ImGui::Spacing();
        ImGui::Separator();
        priest::render_priest_policy_panel(priest_sim.policy, priest_sim.talents, priest_sim.race);
        EndWowChild();

        ImGui::SameLine();

        // Column 2 (Middle, ~360px): Combat Stats Summary collapsible header, followed by collapsible panels: Consumables & Elixirs, Raid Buffs, World Buffs, Raid Debuffs, and Priest Mechanics
        BeginWowChild("PriestPane_Col2", ImVec2(pane2_w, pane_height), true);
        render_combat_stats_summary(
            priest_sim, priest_stats, priest_sim.base_attrs, priest_build_copied_timer, sim::PlayerClass::PRIEST);
        ImGui::Spacing();
        warlock::render_panel_buffs(priest_sim);
        ImGui::Spacing();
        priest::render_priest_mechanics_panel(priest_sim.mechanics);
        EndWowChild();

        ImGui::SameLine();

        // Column 3 (Talents on top, Sim Config & Simulation Results sharing panel below):
        BeginWowChild("PriestPane_Col3", ImVec2(0, pane_height), true, ImGuiWindowFlags_HorizontalScrollbar);
        priest::render_priest_talents_panel(priest_sim);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        render_panel_target<priest::PriestSimulator, priest::BatchSimResult, priest::ParallelSimRunner>(
            priest_sim, priest_iterations, thread_count, priest_last_result, is_priest_sim_running, priest_sim_progress, "RUN PRIEST DES SIMULATION");
        EndWowChild();
        break;
      }

      case AppTab::SIMULATE:
      {
        priest::render_priest_panel_optimizer(
            priest_sim, priest_optimizer_results, is_priest_optimizing, priest_opt_progress, priest_opt_task_name, &request_priest_switch_to_preset);
        break;
      }

      case AppTab::ABILITIES:
      {
        priest::render_priest_spellbook_panel();
        break;
      }

      case AppTab::THEORYCRAFTING:
      {
        priest_active_tab = AppTab::PRESETS;
        break;
      }
    }
  }
};

}  // namespace warlock
