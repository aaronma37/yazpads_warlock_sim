#pragma once
#include "raylib.h"
#include "imgui.h"
#include "implot.h"
#include "rlImGui.h"

#include "asset_manager.hpp"
#include "ui_theme.hpp"
#include "panel_gear.hpp"
#include "panel_target.hpp"
#include "panel_sim_control.hpp"
#include "panel_talents.hpp"
#include "panel_mechanics.hpp"
#include "panel_buffs.hpp"
#include "panel_policy.hpp"
#include "panel_results.hpp"
#include "panel_comparison.hpp"
#include "panel_optimizer.hpp"
#include "panel_spellbook.hpp"
#include "panel_mechanics_tab.hpp"
#include "panel_known_issues.hpp"

#include "src/sim/warlock_sim.hpp"
#include "src/sim/parallel_runner.hpp"
#include "src/sim/optimizer.hpp"

namespace warlock {

class WarlockSimApp {
public:
    WarlockSimulator sim;
    BatchSimResult last_result;
    std::vector<CandidateResult> comparison_results;
    std::vector<CandidateResult> optimizer_results;

    int iterations = 10000;
    int thread_count = 0;

    bool is_sim_running = false;
    float sim_progress = 0.0f;

    bool is_comparing = false;
    float compare_progress = 0.0f;
    std::string compare_task_name;

    bool is_optimizing = false;
    float opt_progress = 0.0f;
    std::string opt_task_name;

    bool request_switch_to_preset = false;

    std::string character_name = "Grimmortis";
    int selected_model_idx = 3; // 3 = Human (0 = Undead, 1 = Orc, 2 = Troll, 3 = Human, 4 = Gnome)

    WarlockSimApp() {
        thread_count = static_cast<int>(std::thread::hardware_concurrency());
        if (thread_count <= 0) thread_count = 4;
    }

    void run_gui() {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(1650, 960, "Classic WoW Warlock DES Simulator & Multi-Threaded Armory");
        SetTargetFPS(60);

        rlImGuiSetup(true);
        ImPlot::CreateContext();
        AssetManager::get().init();
        apply_warlock_theme();

        // Initial baseline run
        last_result = ParallelSimRunner::run_batch(sim, 5000, thread_count);

        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(Color{ 14, 12, 18, 255 });

            rlImGuiBegin();

            // Fixed Fullscreen Canvas (No floating / draggable windows!)
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())));
            ImGuiWindowFlags root_flags = ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoCollapse |
                                         ImGuiWindowFlags_MenuBar |
                                         ImGuiWindowFlags_NoBringToFrontOnFocus;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

            if (ImGui::Begin("RootFixedCanvas", nullptr, root_flags)) {

                // Top Menu Bar
                if (ImGui::BeginMenuBar()) {
                    ImGui::TextColored(ImVec4(0.85f, 0.55f, 1.0f, 1.0f), "WOW FOREVER WARLOCK DES SIMULATOR");
                    ImGui::Separator();

                    if (ImGui::BeginMenu("Build Presets")) {
                        if (ImGui::MenuItem("5/11/35 DS/AF DS-Imp")) {
                            sim.talents = Talents::create_forever_ds_af();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = true;
                            sim.policy.pet = PetChoice::NONE;
                            sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
                        }
                        if (ImGui::MenuItem("9/11/31 Fire Destro+Suppression DS-Succ")) {
                            sim.talents = Talents::create_forever_ds_incinerate();
                            sim.buffs.sacrifice_succubus = true;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.maintain_immolate = true;
                            sim.policy.rotation = RotationChoice::FIRE_DESTRO;
                            sim.policy.pet = PetChoice::NONE;
                        }
                        if (ImGui::MenuItem("5/11/35 DS/Searing Pain DS-Succ")) {
                            sim.talents = Talents::create_forever_ds_searing_pain();
                            sim.buffs.sacrifice_succubus = true;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.maintain_immolate = true;
                            sim.policy.rotation = RotationChoice::FIRE_DESTRO;
                            sim.policy.pet = PetChoice::NONE;
                        }
                        if (ImGui::MenuItem("2/31/18 DP/AF Shadow DS-Imp")) {
                            sim.talents = Talents::create_forever_dp_af_shadow();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = true;
                            sim.policy.pet = PetChoice::SUCCUBUS;
                            sim.policy.rotation = RotationChoice::DP_AF_SHADOW;
                        }
                        if (ImGui::MenuItem("0/31/20 DP/AF Fire DS-Succ")) {
                            sim.talents = Talents::create_forever_dp_af_fire();
                            sim.buffs.sacrifice_succubus = true;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::IMP;
                            sim.policy.rotation = RotationChoice::DP_RUIN_FIRE;
                        }
                        if (ImGui::MenuItem("40/11/0 Deep Affliction DS-Imp")) {
                            sim.talents = Talents::create_forever_deep_affliction();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = true;
                            sim.policy.rotation = RotationChoice::DEEP_AFFLICTION;
                            sim.policy.pet = PetChoice::NONE;
                        }
                        if (ImGui::MenuItem("32/0/19 SM/AF")) {
                            sim.talents = Talents::create_forever_sm_af();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::IMP;
                            sim.policy.rotation = RotationChoice::SM_RUIN;
                        }
                        if (ImGui::MenuItem("23/10/18 NF/AF")) {
                            sim.talents = Talents::create_forever_nf_af();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::IMP;
                            sim.policy.rotation = RotationChoice::SM_RUIN;
                        }
                        if (ImGui::MenuItem("1/17/33 Shadow and Flame Fire")) {
                            sim.talents = Talents::create_forever_shadow_and_flame();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::IMP;
                            sim.policy.maintain_immolate = true;
                            sim.policy.rotation = RotationChoice::FIRE_DESTRO;
                        }
                        if (ImGui::MenuItem("3/17/31 Shadow and Flame Fire DS-Succ")) {
                            sim.talents = Talents::create_forever_shadow_and_flame_fire_ds_succ();
                            sim.buffs.sacrifice_succubus = true;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::NONE;
                            sim.policy.maintain_immolate = true;
                            sim.policy.rotation = RotationChoice::FIRE_DESTRO;
                        }
                        if (ImGui::MenuItem("10/10/31 Shadow and Flame Fire 2")) {
                            sim.talents = Talents::create_forever_shadow_and_flame_fire_2();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::IMP;
                            sim.policy.maintain_immolate = true;
                            sim.policy.rotation = RotationChoice::SHADOW_AND_FLAME_FIRE_2;
                        }
                        if (ImGui::MenuItem("2/17/32 Shadow and Flame Shadow")) {
                            sim.talents = Talents::create_forever_shadow_and_flame_shadow();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::IMP;
                            sim.policy.maintain_immolate = true;
                            sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
                        }
                        if (ImGui::MenuItem("2/17/32 Shadow and Flame Shadow 2")) {
                            sim.talents = Talents::create_forever_shadow_and_flame_shadow_2();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::IMP;
                            sim.policy.maintain_immolate = true;
                            sim.policy.rotation = RotationChoice::SHADOW_DESTRO_2;
                        }
                        if (ImGui::MenuItem("19/11/21 NF/DS/Ruin DS-Imp")) {
                            sim.talents = Talents::create_forever_nf_ds_ruin();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = true;
                            sim.policy.pet = PetChoice::NONE;
                            sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Phase 6 BiS (Naxxramas)")) {
                            sim.gear = GearLoadout::create_phase6_bis();
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Simulation")) {
                        if (ImGui::MenuItem("Run Full Simulation (10,000 fights)")) {
                            last_result = ParallelSimRunner::run_batch(sim, iterations, thread_count);
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::SameLine(ImGui::GetWindowWidth() - 360);
                    if (last_result.iterations_per_second > 0.0) {
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "DES Engine: %.0f sims/sec (%d Threads)",
                            last_result.iterations_per_second, thread_count);
                    } else {
                        ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), "DES Engine: Ready (%d Threads)", thread_count);
                    }

                    ImGui::EndMenuBar();
                }

                // Ensure base_attrs is synced to sim.race
                sim.base_attrs = get_base_attributes_for_race(sim.race);

                // Compute player combat stats from gear (or raw manual stats) and buffs
                Stats player_stats = sim.use_raw_stats ? sim.raw_stats : sim.gear.calculate_stats();
                sim.buffs.apply_to_stats(player_stats, sim.base_attrs, true, sim.mechanics.personal_shadow_weaving); // true = WoW Forever mechanics

                // Gnome Expansive Mind (+5% Mana)
                if (sim.race == Race::GNOME) {
                    player_stats.max_mana *= 1.05;
                }

                // Human Sword Specialization (+2% crit)
                bool is_sword = false;
                if (!sim.use_raw_stats) {
                    const Item& mh = sim.gear.get(Slot::MAIN_HAND);
                    if (mh.name.find("Mageblade") != std::string::npos ||
                        mh.name.find("Sword") != std::string::npos ||
                        mh.name.find("Blade") != std::string::npos ||
                        mh.icon.find("sword") != std::string::npos ||
                        mh.icon.find("Sword") != std::string::npos) {
                        is_sword = true;
                    }
                } else {
                    is_sword = true;
                }
                if (sim.race == Race::HUMAN && is_sword) {
                    player_stats.spell_crit_percent += 2.0;
                }

                // Orc Axe Specialization (+1% crit)
                bool is_axe = false;
                if (!sim.use_raw_stats) {
                    const Item& mh = sim.gear.get(Slot::MAIN_HAND);
                    if (mh.name.find("Axe") != std::string::npos || mh.icon.find("axe") != std::string::npos) {
                        is_axe = true;
                    }
                }
                if (sim.race == Race::ORC && is_axe) {
                    player_stats.spell_crit_percent += 1.0;
                }

                // If candidate configuration was applied from combinatorial sim, refresh baseline
                if (request_switch_to_preset) {
                    last_result = ParallelSimRunner::run_batch(sim, 5000, thread_count);
                }

                const float full_height = ImGui::GetContentRegionAvail().y;

                // =========================================================================
                // TOP-LEVEL HIERARCHICAL TABS
                // =========================================================================
                if (ImGui::BeginTabBar("TopLayerTabs", ImGuiTabBarFlags_None)) {

                    // -----------------------------------------------------------------
                    // 1. PRESET SIMULATION (Inspector, Talents, Gear, Buffs, APL, Sim)
                    // -----------------------------------------------------------------
                    ImGuiTabItemFlags preset_flags = 0;
                    if (request_switch_to_preset) {
                        preset_flags |= ImGuiTabItemFlags_SetSelected;
                    }

                    if (ImGui::BeginTabItem("  PRESET SIMULATION  ", nullptr, preset_flags)) {
                        if (request_switch_to_preset) {
                            request_switch_to_preset = false;
                        }

                        if (ImGui::BeginTabBar("PresetSubTabs", ImGuiTabBarFlags_None)) {

                            // -------------------------------------------------------------
                            // SUBTAB 1: BUILD CONFIGURATION (Gear, Talents, Buffs, Rotation)
                            // -------------------------------------------------------------
                            if (ImGui::BeginTabItem("  Build Configuration  ")) {
                                const float pane1_w = 320.0f; // Gear & Direct Stats
                                const float pane2_w = 830.0f; // Talents Tree (51 Points - All 3 Trees Visible)
                                const float pane_height = full_height - 40.0f;

                                // Pane 1: Gear & Direct Stats
                                ImGui::BeginChild("PresetPane_Gear", ImVec2(pane1_w, pane_height), true);
                                render_armory_panel(sim, player_stats, sim.base_attrs, character_name, selected_model_idx);
                                ImGui::EndChild();

                                ImGui::SameLine();

                                // Pane 2: Talent Tree (51 Points)
                                ImGui::BeginChild("PresetPane_Talents", ImVec2(pane2_w, pane_height), true, ImGuiWindowFlags_HorizontalScrollbar);
                                render_panel_talents(sim);
                                ImGui::EndChild();

                                ImGui::SameLine();

                                // Pane 3: Target Encounter, Consumables, Buffs, Rotation Policy & Mechanics
                                ImGui::BeginChild("PresetPane_BuffsPolicy", ImVec2(0, pane_height), true);
                                render_panel_target(sim.target_config, sim.fight_duration);
                                ImGui::Spacing();
                                ImGui::Separator();
                                render_panel_buffs(sim.buffs);
                                ImGui::Spacing();
                                ImGui::Separator();
                                render_panel_policy(sim);
                                ImGui::Spacing();
                                ImGui::Separator();
                                render_panel_mechanics(sim.mechanics);
                                ImGui::EndChild();

                                ImGui::EndTabItem();
                            }

                            // -------------------------------------------------------------
                            // SUBTAB 2: COMBAT SIMULATION & RESULTS (Controls, Histograms, Logs)
                            // -------------------------------------------------------------
                            if (ImGui::BeginTabItem("  Combat Simulation & Results  ")) {
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

                    // -----------------------------------------------------------------
                    // 2. COMBINATORIAL SIMULATION (Brute-Force Optimizer & Comparison)
                    // -----------------------------------------------------------------
                    if (ImGui::BeginTabItem("  COMBINATORIAL SIMULATION  ")) {
                        if (ImGui::BeginTabBar("CombinatorialSubTabs", ImGuiTabBarFlags_None)) {

                            // Subtab 1: Optimizer & Leaderboard
                            if (ImGui::BeginTabItem("  Brute-Force Optimizer & Leaderboard  ")) {
                                ImGui::Spacing();
                                render_panel_optimizer(sim, optimizer_results, is_optimizing, opt_progress, opt_task_name, &request_switch_to_preset);
                                ImGui::EndTabItem();
                            }

                            // Subtab 2: Theorycrafting Comparison
                            if (ImGui::BeginTabItem("  Side-by-Side Comparison  ")) {
                                ImGui::Spacing();
                                render_panel_comparison(sim, comparison_results, is_comparing, compare_progress, compare_task_name);
                                ImGui::EndTabItem();
                            }

                            ImGui::EndTabBar();
                        }

                        ImGui::EndTabItem();
                    }

                    // -----------------------------------------------------------------
                    // 3. SPELLBOOK (Spell Database, Ranks, Base Stats, Coefficients)
                    // -----------------------------------------------------------------
                    if (ImGui::BeginTabItem("  SPELLBOOK  ")) {
                        ImGui::Spacing();
                        render_panel_spellbook();
                        ImGui::EndTabItem();
                    }

                    // -----------------------------------------------------------------
                    // 4. MECHANICS & CUSTOM RULES (ISB, DoT Crits, SW, DP, Vanilla Differences)
                    // -----------------------------------------------------------------
                    if (ImGui::BeginTabItem("  MECHANICS  ")) {
                        ImGui::Spacing();
                        render_panel_mechanics_tab();
                        ImGui::EndTabItem();
                    }

                    // -----------------------------------------------------------------
                    // 5. KNOWN ISSUES & ROADMAP (Proc Gear Backlog, Downranking, Scope)
                    // -----------------------------------------------------------------
                    if (ImGui::BeginTabItem("  KNOWN ISSUES  ")) {
                        ImGui::Spacing();
                        render_panel_known_issues();
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
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
};

} // namespace warlock
