#pragma once
#include "raylib.h"
#include "imgui.h"
#include "implot.h"
#include "rlImGui.h"

#include "asset_manager.hpp"
#include "ui_theme.hpp"
#include "panel_gear.hpp"
#include "panel_sim_control.hpp"
#include "panel_talents.hpp"
#include "panel_mechanics.hpp"
#include "panel_policy.hpp"
#include "panel_results.hpp"
#include "panel_comparison.hpp"
#include "panel_optimizer.hpp"

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

    std::string character_name = "Grimmortis";
    int selected_model_idx = 0; // 0 = Undead, 1 = Orc

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
                    ImGui::TextColored(ImVec4(0.85f, 0.55f, 1.0f, 1.0f), "⚔ WOW FOREVER WARLOCK DES SIMULATOR");
                    ImGui::Separator();

                    if (ImGui::BeginMenu("Build Presets")) {
                        if (ImGui::MenuItem("Forever Shadow Destro (0/21/30)")) {
                            sim.talents = Talents::create_forever_shadow_destro();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = true;
                            sim.policy.pet = PetChoice::NONE;
                        }
                        if (ImGui::MenuItem("Forever Fire Destro (0/11/40 - Incinerate)")) {
                            sim.talents = Talents::create_forever_fire_destro();
                            sim.buffs.sacrifice_succubus = true;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.maintain_immolate = true;
                            sim.policy.rotation = RotationChoice::INCINERATE_FIRE;
                            sim.policy.pet = PetChoice::NONE;
                        }
                        if (ImGui::MenuItem("Forever Demonic Pact + Ruin (2/31/18 - Sac Imp + Succubus)")) {
                            sim.talents = Talents::create_forever_demonic_pact();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = true;
                            sim.policy.pet = PetChoice::SUCCUBUS;
                        }
                        if (ImGui::MenuItem("Forever Deep Affliction (41/0/10 - Drain Hope)")) {
                            sim.talents = Talents::create_forever_deep_affliction();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.rotation = RotationChoice::DEEP_AFFLICTION;
                            sim.policy.pet = PetChoice::SUCCUBUS;
                        }
                        if (ImGui::MenuItem("Forever SM / Ruin (29/0/22 - 3/3 Flames)")) {
                            sim.talents = Talents::create_forever_sm_ruin();
                            sim.buffs.sacrifice_succubus = false;
                            sim.buffs.sacrifice_imp = false;
                            sim.policy.pet = PetChoice::SUCCUBUS;
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
                sim.buffs.apply_to_stats(player_stats, sim.base_attrs, true); // true = WoW Forever mechanics

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

                // Two-Pane Fixed Layout:
                // Left: Character Armory & Sheet (450px wide)
                // Right: Unified Dashboard & Tabs (Remaining width)
                const float armory_width = 450.0f;
                const float full_height = ImGui::GetContentRegionAvail().y;

                // --- LEFT PANE: CHARACTER ARMORY ---
                ImGui::BeginChild("ArmoryLeftPane", ImVec2(armory_width, full_height), true);
                render_armory_panel(sim, player_stats, sim.base_attrs, character_name, selected_model_idx);
                ImGui::EndChild();

                ImGui::SameLine();

                // --- RIGHT PANE: MAIN TOOLING TABS ---
                ImGui::BeginChild("DashboardRightPane", ImVec2(0, full_height), true);

                if (ImGui::BeginTabBar("UnifiedDashboardTabs", ImGuiTabBarFlags_None)) {

                    // Tab 1: Simulation & Results
                    if (ImGui::BeginTabItem("  ⚔ Combat Simulation & Charts  ")) {
                        ImGui::Spacing();
                        render_panel_sim_control(sim, iterations, thread_count, last_result, is_sim_running, sim_progress);
                        ImGui::Separator();
                        render_panel_results(last_result);
                        ImGui::EndTabItem();
                    }

                    // Tab 2: Talent Tree
                    if (ImGui::BeginTabItem("  🌲 Talent Tree (51 Points)  ")) {
                        ImGui::Spacing();
                        render_panel_talents(sim.talents);
                        ImGui::EndTabItem();
                    }

                    // Tab 3: Multi-Threaded Optimizer & Comparison
                    if (ImGui::BeginTabItem("  ⚡ Brute-Force Optimizer & Leaderboard  ")) {
                        ImGui::Spacing();
                        render_panel_optimizer(sim, optimizer_results, is_optimizing, opt_progress, opt_task_name);
                        ImGui::Separator();
                        render_panel_comparison(sim, comparison_results, is_comparing, compare_progress, compare_task_name);
                        ImGui::EndTabItem();
                    }

                    // Tab 4: Mechanics Toggles & Rotation Policy
                    if (ImGui::BeginTabItem("  ⚙ Mechanics Toggles & Rotation Policy  ")) {
                        ImGui::Spacing();
                        ImGui::Columns(2, "MechanicsAndPolicyCols", true);

                        render_panel_mechanics(sim.mechanics);
                        ImGui::NextColumn();

                        render_panel_policy(sim);
                        ImGui::Columns(1);

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }

                ImGui::EndChild();
            }
            ImGui::End();
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
