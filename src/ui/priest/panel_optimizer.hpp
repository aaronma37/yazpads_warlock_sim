#pragma once
#include "imgui.h"
#include "src/ui/common/asset_manager.hpp"
#include "src/ui/common/damage_breakdown_view.hpp"
#include "src/ui/common/panel_policy.hpp"
#include "src/ui/priest/panel_policy.hpp"
#include "src/sim/priest/optimizer.hpp"
#include "src/sim/priest/talent_graph.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace priest {

// Background worker state for live asynchronous optimization
struct PriestOptimizerWorkerState {
    std::thread worker;
    std::mutex mtx;
    std::atomic<bool> is_running{false};
    std::atomic<bool> stop_requested{false};
    std::atomic<float> progress{0.0f};
    std::string current_status;
    std::vector<CandidateResult> live_results;
    bool has_new_results = false;
};

inline PriestOptimizerWorkerState& get_priest_opt_worker_state() {
    static PriestOptimizerWorkerState state;
    return state;
}

inline void render_priest_panel_optimizer(
    PriestSimulator& sim,
    std::vector<CandidateResult>& optimizer_results,
    bool& is_optimizing,
    float& opt_progress,
    std::string& current_opt_target,
    bool* request_switch_to_preset = nullptr
) {
    auto& worker = get_priest_opt_worker_state();

    // Check if background worker has new live generation results
    {
        std::lock_guard<std::mutex> lock(worker.mtx);
        if (worker.has_new_results) {
            optimizer_results = worker.live_results;
            worker.has_new_results = false;
        }
        if (worker.is_running.load()) {
            is_optimizing = true;
            opt_progress = worker.progress.load();
            current_opt_target = worker.current_status;
        } else if (is_optimizing) {
            is_optimizing = false;
            opt_progress = 1.0f;
            if (worker.worker.joinable()) {
                worker.worker.join();
            }
        }
    }

    static int opt_mode = 1; // 0 = Genetic Search, 1 = Standard Presets Benchmark
    ImGui::RadioButton("Standard Specs Benchmark", &opt_mode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Genetic Search", &opt_mode, 0);

    ImGui::Spacing();

    static int ga_pop_size = 50;
    static int ga_generations = 400;
    static int ga_screening_sims = 400;
    static int ga_final_sims = 2500;
    static float ga_mutation_rate = 0.45f;
    static float ga_initial_explore = 0.50f;
    static float ga_min_explore = 0.15f;
    static bool ga_seed_presets = false;
    static bool ga_optimize_race = true;
    static bool show_advanced_tuning = false;
    static int ga_req_talent1 = -1;
    static int ga_req_talent2 = -1;
    static int ga_req_talent3 = -1;
    static int ga_forced_race = -1;
    static int ga_forced_rotation = -1;
    static int ga_threads = static_cast<int>(std::thread::hardware_concurrency());

    static int iters_per_candidate = 3000;
    static bool compare_all_races = false;

    if (opt_mode == 0) {
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Generations", &ga_generations)) {
            if (ga_generations < 1) ga_generations = 1;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("Population", &ga_pop_size)) {
            if (ga_pop_size < 4) ga_pop_size = 4;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110);
        if (ImGui::InputInt("Screening Sims", &ga_screening_sims)) {
            if (ga_screening_sims < 10) ga_screening_sims = 10;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110);
        if (ImGui::InputInt("Final Precision", &ga_final_sims)) {
            if (ga_final_sims < 10) ga_final_sims = 10;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        int max_threads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
        if (ImGui::SliderInt("Threads", &ga_threads, 1, max_threads, "%d")) {
            if (ga_threads < 1) ga_threads = 1;
        }

        ImGui::Spacing();
        ImGui::Checkbox("Seed with standard presets", &ga_seed_presets);
        ImGui::SameLine(460);
        ImGui::Checkbox("Optimize Race", &ga_optimize_race);
        ImGui::SameLine(750);
        ImGui::Checkbox("Advanced Convergence Tuning", &show_advanced_tuning);

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.3f, 1.0f), "Build Constraints:");

        // Helper lambda for talent selection combo
        auto render_priest_talent_combo = [](const char* label, int& selected_idx) {
            const auto& nodes = TalentGraph::get().nodes();
            std::string preview = "[None]";
            if (selected_idx >= 0 && selected_idx < static_cast<int>(nodes.size())) {
                const auto& n = nodes[selected_idx];
                const char* tree_name = (n.tree == 0) ? "Disc" : ((n.tree == 1) ? "Holy" : "Shadow");
                preview = n.name + " (" + tree_name + ")";
            }

            ImGui::SetNextItemWidth(210);
            if (ImGui::BeginCombo(label, preview.c_str())) {
                if (ImGui::Selectable("[None]", selected_idx == -1)) {
                    selected_idx = -1;
                }
                for (size_t i = 0; i < nodes.size(); ++i) {
                    const auto& n = nodes[i];
                    const char* tree_name = (n.tree == 0) ? "Disc" : ((n.tree == 1) ? "Holy" : "Shadow");
                    std::string item_name = n.name + " (" + tree_name + ")";
                    bool is_selected = (selected_idx == static_cast<int>(i));
                    if (ImGui::Selectable(item_name.c_str(), is_selected)) {
                        selected_idx = static_cast<int>(i);
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        };

        render_priest_talent_combo("Req Talent 1", ga_req_talent1);
        ImGui::SameLine();
        render_priest_talent_combo("Req Talent 2", ga_req_talent2);
        ImGui::SameLine();
        render_priest_talent_combo("Req Talent 3", ga_req_talent3);

        // Race constraint combo
        ImGui::SetNextItemWidth(170);
        const char* race_names[] = {"[Any / Evolve]", "Human", "Dwarf", "Night Elf", "Undead", "Troll"};
        const sim::Race race_enums[] = {sim::Race::HUMAN, sim::Race::HUMAN, sim::Race::DWARF, sim::Race::NIGHT_ELF, sim::Race::UNDEAD, sim::Race::TROLL};
        int current_race_idx = 0;
        if (ga_forced_race >= 0) {
            for (int r = 1; r < 6; ++r) {
                if (static_cast<int>(race_enums[r]) == ga_forced_race) {
                    current_race_idx = r;
                    break;
                }
            }
        }
        if (ImGui::Combo("Locked Race", &current_race_idx, race_names, IM_ARRAYSIZE(race_names))) {
            ga_forced_race = (current_race_idx == 0) ? -1 : static_cast<int>(race_enums[current_race_idx]);
        }

        ImGui::SameLine();
        // Rotation constraint combo
        ImGui::SetNextItemWidth(260);
        const char* rot_preview = "[Auto / Adaptive]";
        if (ga_forced_rotation >= 0) {
            rot_preview = rotation_choice_to_string(static_cast<RotationChoice>(ga_forced_rotation));
        }
        if (ImGui::BeginCombo("Locked Rotation", rot_preview)) {
            if (ImGui::Selectable("[Auto / Adaptive]", ga_forced_rotation == -1)) {
                ga_forced_rotation = -1;
            }
            for (int r = 0; r < static_cast<int>(RotationChoice::COUNT); ++r) {
                RotationChoice rc = static_cast<RotationChoice>(r);
                const char* r_str = rotation_choice_to_string(rc);
                bool is_sel = (ga_forced_rotation == r);
                if (ImGui::Selectable(r_str, is_sel)) {
                    ga_forced_rotation = r;
                }
                if (is_sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (show_advanced_tuning) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.18f, 0.6f));
            ImGui::BeginChild("PriestGATuningBox", ImVec2(-1, 68), true);
            ImGui::SetNextItemWidth(150);
            ImGui::SliderFloat("Mutation Rate", &ga_mutation_rate, 0.10f, 0.90f, "%.2f");
            ImGui::SameLine(220);
            ImGui::SetNextItemWidth(180);
            ImGui::SliderFloat("Start Exploration Rate", &ga_initial_explore, 0.10f, 0.90f, "%.2f (early gens)");
            ImGui::SameLine(480);
            ImGui::SetNextItemWidth(180);
            ImGui::SliderFloat("End Exploration Rate", &ga_min_explore, 0.05f, 0.50f, "%.2f (annealed final)");
            ImGui::TextDisabled("Controls simulated annealing schedule: high early exploration prevents getting stuck in local optima.");
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    } else {
        ImGui::SetNextItemWidth(200);
        ImGui::SliderInt("Sims Per Candidate", &iters_per_candidate, 500, 10000, "%d fights");
        ImGui::SameLine(340);
        ImGui::Checkbox("Compare across all races", &compare_all_races);
    }

    ImGui::Spacing();

    // Run / Cancel Buttons
    if (opt_mode == 0) {
        if (!worker.is_running.load()) {
            if (ImGui::Button("Run AI Genetic Optimization", ImVec2(260, 28))) {
                if (worker.worker.joinable()) worker.worker.join();
                worker.is_running = true;
                worker.stop_requested = false;
                worker.progress = 0.0f;
                worker.current_status = "Initializing Population...";
                is_optimizing = true;
                opt_progress = 0.0f;
                current_opt_target = worker.current_status;

                PriestSimulator sim_copy = sim;
                int pop_sz = ga_pop_size;
                int gens = ga_generations;
                int screen_sims = ga_screening_sims;
                int fn_sims = ga_final_sims;
                bool seed_pre = ga_seed_presets;
                bool opt_race = (ga_forced_race >= 0) ? false : ga_optimize_race;
                double mut_rate = ga_mutation_rate;
                double init_exp = ga_initial_explore;
                double min_exp = ga_min_explore;

                std::vector<int> req_talents;
                if (ga_req_talent1 >= 0) req_talents.push_back(ga_req_talent1);
                if (ga_req_talent2 >= 0 && ga_req_talent2 != ga_req_talent1) req_talents.push_back(ga_req_talent2);
                if (ga_req_talent3 >= 0 && ga_req_talent3 != ga_req_talent1 && ga_req_talent3 != ga_req_talent2) req_talents.push_back(ga_req_talent3);
                int forced_r = ga_forced_race;
                int forced_rot = ga_forced_rotation;
                int th_count = ga_threads;

                worker.worker = std::thread([sim_copy, pop_sz, gens, screen_sims, fn_sims, seed_pre, opt_race,
                                             mut_rate, init_exp, min_exp, req_talents, forced_r, forced_rot, th_count]() {
                    auto& w = get_priest_opt_worker_state();
                    auto results = Optimizer::optimize_genetic_ai(
                        sim_copy,
                        pop_sz,
                        gens,
                        screen_sims,
                        fn_sims,
                        seed_pre,
                        opt_race,
                        mut_rate,
                        init_exp,
                        min_exp,
                        req_talents,
                        forced_r,
                        forced_rot,
                        th_count,
                        [&](float p, const std::string& name) {
                            w.progress = p;
                            std::lock_guard<std::mutex> lk(w.mtx);
                            w.current_status = name;
                        },
                        [&](const std::vector<CandidateResult>& current_elites) {
                            std::lock_guard<std::mutex> lk(w.mtx);
                            w.live_results = current_elites;
                            w.has_new_results = true;
                        },
                        &w.stop_requested
                    );

                    {
                        std::lock_guard<std::mutex> lk(w.mtx);
                        w.live_results = results;
                        w.has_new_results = true;
                        w.is_running = false;
                    }
                });
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
            if (ImGui::Button("🛑 Stop Search & Keep Best", ImVec2(220, 28))) {
                worker.stop_requested = true;
            }
            ImGui::PopStyleColor();
        }
    } else {
        if (is_optimizing) ImGui::BeginDisabled();
        if (ImGui::Button("Simulate Standard Specs", ImVec2(240, 28))) {
            is_optimizing = true;
            opt_progress = 0.0f;
            optimizer_results = Optimizer::optimize_talents(
                sim,
                iters_per_candidate,
                [&](float p, const std::string& name) {
                    opt_progress = p;
                    current_opt_target = name;
                },
                compare_all_races
            );
            is_optimizing = false;
            opt_progress = 1.0f;
        }
        if (is_optimizing) ImGui::EndDisabled();
    }

    if (is_optimizing) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Optimizing: %s", current_opt_target.c_str());
        ImGui::ProgressBar(opt_progress, ImVec2(-1, 8));
    }

    ImGui::Separator();

    auto apply_candidate_config = [&](const CandidateResult& r) {
        sim.race = r.race;
        sim.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, sim.race);
        sim.use_raw_stats = r.use_raw_stats;
        sim.raw_stats = r.raw_stats;
        sim.talents = r.talents;
        sim.gear = r.gear;
        sim.buffs = r.buffs;
        sim.policy = r.policy;
        sim.mechanics = r.mechanics;
        if (request_switch_to_preset) {
            *request_switch_to_preset = true;
        }
    };

    if (!optimizer_results.empty()) {
        const auto& best = optimizer_results[0];
        static int selected_candidate_idx = 0;
        if (selected_candidate_idx >= static_cast<int>(optimizer_results.size())) {
            selected_candidate_idx = 0;
        }

        static bool show_std_dev = false;
        static bool show_pct_from_leader = false;
        int num_cols = show_std_dev ? 8 : 7;

        // Calculate active base combat stats for the header line
        sim::Stats base_stats = sim.use_raw_stats ? sim.raw_stats : sim.gear.calculate_stats();
        sim.buffs.apply_to_stats(base_stats, sim.base_attrs, true, sim.mechanics.shadow_weaving_personal);

        // Spec table header matching Warlock golden standard
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Specs");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "Base Stats:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.75f, 0.50f, 1.0f, 1.0f), "Shadow SP: %.0f", base_stats.effective_shadow_power());
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "Holy SP: %.0f", base_stats.effective_holy_power());
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Hit: %.1f%%", base_stats.spell_hit_percent);
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Crit: %.2f%%", base_stats.total_spell_crit(sim.base_attrs.base_spell_crit));
        if (base_stats.spell_haste_percent > 0.0) {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Haste: %.1f%%", base_stats.spell_haste_percent);
        }
        if (base_stats.mp5 > 0.0) {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "MP5: %.0f", base_stats.mp5);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        if (sim.randomize_duration && sim.duration_variance > 0.0) {
            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.45f, 1.0f), "Fight: %.0fs +/- %.0fs", sim.fight_duration, sim.duration_variance);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Simulated fight duration: %.0fs to %.0fs (mean %.0fs)",
                                  std::max(5.0, sim.fight_duration - sim.duration_variance),
                                  sim.fight_duration + sim.duration_variance,
                                  sim.fight_duration);
            }
        } else {
            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.45f, 1.0f), "Fight: %.0fs", sim.fight_duration);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Simulated fight duration: %.0f seconds", sim.fight_duration);
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::Checkbox("% from Leader", &show_pct_from_leader);
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::Checkbox("Show Std Dev", &show_std_dev);

        if (ImGui::BeginTable("PriestOptLeaderboardTable", num_cols, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 45);
            ImGui::TableSetupColumn("Spec Name", ImGuiTableColumnFlags_WidthFixed, 190);
            ImGui::TableSetupColumn("Race", ImGuiTableColumnFlags_WidthFixed, 42);
            ImGui::TableSetupColumn("Form / Spec", ImGuiTableColumnFlags_WidthFixed, 75);
            ImGui::TableSetupColumn("Action Priority Chain", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Damage Split", ImGuiTableColumnFlags_WidthFixed, 150);
            ImGui::TableSetupColumn(show_pct_from_leader ? "% vs Leader" : "Mean DPS", ImGuiTableColumnFlags_WidthFixed, 85);
            if (show_std_dev) {
                ImGui::TableSetupColumn("+/- StdDev", ImGuiTableColumnFlags_WidthFixed, 75);
            }
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < optimizer_results.size(); ++i) {
                const auto& r = optimizer_results[i];
                bool is_selected = (selected_candidate_idx == static_cast<int>(i));

                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                char rank_sel_id[64];
                std::snprintf(rank_sel_id, sizeof(rank_sel_id), "##priest_row_sel_%zu", i);
                if (ImGui::Selectable(rank_sel_id, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                    selected_candidate_idx = static_cast<int>(i);
                }
                ImGui::SameLine(0.0f, 0.0f);
                if (r.rank == 1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#1");
                } else {
                    ImGui::Text("#%d", r.rank);
                }

                // Spec Name: clickable link that loads candidate configuration
                ImGui::TableNextColumn();
                ImGui::PushID(static_cast<int>(i));
                ImVec2 text_size = ImGui::CalcTextSize(r.name.c_str());
                bool clicked_link = ImGui::InvisibleButton("##spec_link", text_size);
                bool hovered_link = ImGui::IsItemHovered();

                if (hovered_link) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "%s", r.name.c_str());
                    ImGui::Separator();
                    ImGui::Text("Click to load preset into configuration");
                    ImGui::EndTooltip();
                }

                ImVec2 p_min = ImGui::GetItemRectMin();
                ImVec2 p_max = ImGui::GetItemRectMax();

                ImVec4 link_col = hovered_link ? ImVec4(0.70f, 0.90f, 1.0f, 1.0f) : ImVec4(0.40f, 0.75f, 1.0f, 1.0f);
                ImU32 u32_col = ImGui::ColorConvertFloat4ToU32(link_col);

                draw_list->AddText(p_min, u32_col, r.name.c_str());

                if (hovered_link) {
                    draw_list->AddLine(ImVec2(p_min.x, p_max.y), ImVec2(p_max.x, p_max.y), u32_col, 1.2f);
                } else {
                    ImU32 dim_underline = ImGui::ColorConvertFloat4ToU32(ImVec4(0.40f, 0.75f, 1.0f, 0.40f));
                    draw_list->AddLine(ImVec2(p_min.x, p_max.y), ImVec2(p_max.x, p_max.y), dim_underline, 1.0f);
                }

                if (clicked_link) {
                    selected_candidate_idx = static_cast<int>(i);
                    apply_candidate_config(r);
                }
                ImGui::PopID();

                // Race Icon
                ImGui::TableNextColumn();
                {
                    Texture2D race_tex = warlock::AssetManager::get().get_icon(sim::race_to_icon(r.race));
                    if (race_tex.id > 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)race_tex.id, ImVec2(18, 18));
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", sim::race_to_string(r.race));
                        ImGui::EndTooltip();
                    }
                }

                // Form / Spec Icon
                ImGui::TableNextColumn();
                {
                    const char* form_icon = "spell_holy_holybolt";
                    const char* form_name = "Holy Specialization";
                    if (r.talents.shadow.shadowform > 0) {
                        form_icon = "spell_shadow_shadowform";
                        form_name = "Shadowform (+10% Shadow Dmg, -50% Mana, 2.0x Crit)";
                    } else if (r.talents.disc.power_infusion > 0) {
                        form_icon = "spell_holy_powerinfusion";
                        form_name = "Discipline / Power Infusion Spec";
                    }

                    Texture2D ftex = warlock::AssetManager::get().get_icon(form_icon);
                    if (ftex.id > 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)ftex.id, ImVec2(18, 18));
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", form_name);
                        ImGui::EndTooltip();
                    }
                }

                // Action Priority Chain
                ImGui::TableNextColumn();
                std::vector<PriorityRule> rules = r.policy.get_priority_rules(r.talents, r.race);
                for (size_t k = 0; k < rules.size(); ++k) {
                    const auto& rule = rules[k];
                    Texture2D icon = warlock::AssetManager::get().get_icon(spell_id_to_icon(rule.spell_id));
                    if (icon.id > 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(18, 18));
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", rule.name.c_str());
                        if (!rule.condition_summary.empty()) {
                            ImGui::TextDisabled("%s", rule.condition_summary.c_str());
                        }
                        if (!rule.trigger_condition.empty()) {
                            ImGui::TextWrapped("%s", rule.trigger_condition.c_str());
                        }
                        ImGui::EndTooltip();
                    }
                    if (k + 1 < rules.size()) {
                        ImGui::SameLine(0, 3);
                    }
                }

                // Damage Breakdown: 3-section horizontal bar (Shadow, Holy, Arcane)
                ImGui::TableNextColumn();
                double shadow_pct = r.batch.pct_sw_pain + r.batch.pct_mind_flay + r.batch.pct_mind_blast +
                                    r.batch.pct_sw_death + r.batch.pct_devouring_plague + r.batch.pct_shadowguard +
                                    r.batch.pct_touch_of_the_grave;
                double holy_pct = r.batch.pct_smite + r.batch.pct_holy_fire + r.batch.pct_penance +
                                  r.batch.pct_holy_nova + r.batch.pct_chastise;
                double arcane_pct = r.batch.pct_starshards;

                double total_pct = shadow_pct + holy_pct + arcane_pct;
                if (total_pct > 0.0) {
                    shadow_pct = (shadow_pct / total_pct) * 100.0;
                    holy_pct = (holy_pct / total_pct) * 100.0;
                    arcane_pct = (arcane_pct / total_pct) * 100.0;
                } else {
                    if (r.name.find("Smite") != std::string::npos || r.name.find("Holy") != std::string::npos) {
                        shadow_pct = 5.0;
                        holy_pct = 95.0;
                        arcane_pct = 0.0;
                    } else {
                        shadow_pct = 95.0;
                        holy_pct = 5.0;
                        arcane_pct = 0.0;
                    }
                }

                float col_w = ImGui::GetContentRegionAvail().x;
                float bar_w = std::max(40.0f, col_w);
                float bar_h = 16.0f;
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImVec2 p1 = ImVec2(p0.x + bar_w, p0.y + bar_h);

                std::string bar_btn_id = "##PriestDmgBar_" + std::to_string(i);
                ImGui::InvisibleButton(bar_btn_id.c_str(), ImVec2(bar_w, bar_h));
                bool is_bar_hovered = ImGui::IsItemHovered();

                draw_list->AddRectFilled(p0, p1, IM_COL32(20, 20, 26, 255), 3.0f);

                float cur_bar_x = p0.x;
                float s_w = (float)(bar_w * (shadow_pct * 0.01));
                float h_w = (float)(bar_w * (holy_pct * 0.01));
                float a_w = (float)(bar_w * (arcane_pct * 0.01));

                // 1. Shadow segment (Purple)
                if (shadow_pct > 0.5) {
                    ImVec2 b0(cur_bar_x, p0.y);
                    ImVec2 b1(std::min(p1.x, cur_bar_x + s_w), p1.y);
                    draw_list->AddRectFilled(b0, b1, IM_COL32(148, 65, 235, 235), 0.0f);
                    cur_bar_x += s_w;
                }

                // 2. Holy segment (Golden Yellow)
                if (holy_pct > 0.5) {
                    ImVec2 b0(cur_bar_x, p0.y);
                    ImVec2 b1(std::min(p1.x, cur_bar_x + h_w), p1.y);
                    draw_list->AddRectFilled(b0, b1, IM_COL32(235, 200, 60, 235), 0.0f);
                    cur_bar_x += h_w;
                }

                // 3. Arcane segment (Cyan / Blue)
                if (arcane_pct > 0.5) {
                    ImVec2 b0(cur_bar_x, p0.y);
                    ImVec2 b1(p1.x, p1.y);
                    draw_list->AddRectFilled(b0, b1, IM_COL32(80, 200, 240, 235), 0.0f);
                }

                draw_list->AddRect(
                    p0, p1, is_bar_hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(65, 65, 80, 255), 3.0f, 0, 1.0f);

                char shadow_txt[16], holy_txt[16], arcane_txt[16];
                std::snprintf(shadow_txt, sizeof(shadow_txt), "%.0f%%", shadow_pct);
                std::snprintf(holy_txt, sizeof(holy_txt), "%.0f%%", holy_pct);
                std::snprintf(arcane_txt, sizeof(arcane_txt), "%.0f%%", arcane_pct);

                float txt_y = p0.y + 1.0f;
                if (s_w >= 26.0f) {
                    ImVec2 sz = ImGui::CalcTextSize(shadow_txt);
                    float txt_x = p0.x + (s_w - sz.x) * 0.5f;
                    draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), shadow_txt);
                    draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), shadow_txt);
                }
                if (h_w >= 26.0f) {
                    ImVec2 sz = ImGui::CalcTextSize(holy_txt);
                    float txt_x = p0.x + s_w + (h_w - sz.x) * 0.5f;
                    draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), holy_txt);
                    draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), holy_txt);
                }
                if (a_w >= 26.0f) {
                    ImVec2 sz = ImGui::CalcTextSize(arcane_txt);
                    float txt_x = p0.x + s_w + h_w + (a_w - sz.x) * 0.5f;
                    draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), arcane_txt);
                    draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), arcane_txt);
                }

                if (is_bar_hovered) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Damage Share Breakdown (%s):", r.name.c_str());
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f),
                                       "■ Shadow Damage: %.1f%% (%.1f DPS)",
                                       shadow_pct,
                                       shadow_pct * 0.01 * r.mean_dps);
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f),
                                       "■ Holy Damage:   %.1f%% (%.1f DPS)",
                                       holy_pct,
                                       holy_pct * 0.01 * r.mean_dps);
                    if (arcane_pct > 0.5) {
                        ImGui::TextColored(ImVec4(0.40f, 0.85f, 1.0f, 1.0f),
                                           "■ Arcane Damage: %.1f%% (%.1f DPS)",
                                           arcane_pct,
                                           arcane_pct * 0.01 * r.mean_dps);
                    }
                    ImGui::EndTooltip();
                }

                ImGui::TableNextColumn();
                if (show_pct_from_leader) {
                    if (best.mean_dps > 0.0) {
                        double pct_diff = ((r.mean_dps - best.mean_dps) / best.mean_dps) * 100.0;
                        if (r.rank == 1) {
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "= Leader");
                        } else {
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%.2f%%", pct_diff);
                        }
                    } else {
                        ImGui::TextDisabled("-");
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.1f", r.mean_dps);
                }

                if (show_std_dev) {
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("+/- %.1f", r.std_dev_dps);
                }
            }

            ImGui::EndTable();
        }

        // Candidate Detail Inspector
        if (selected_candidate_idx >= 0 && selected_candidate_idx < static_cast<int>(optimizer_results.size())) {
            const auto& sel = optimizer_results[selected_candidate_idx];
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#%d %s", sel.rank, sel.name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                               "[%.1f Mean DPS | Median: %.1f | P5-P95: %.1f - %.1f]",
                               sel.mean_dps,
                               sel.batch.p50_dps > 0 ? sel.batch.p50_dps : sel.mean_dps,
                               sel.batch.p5_dps,
                               sel.batch.p95_dps);

            ImGui::Spacing();
            std::vector<PriorityRule> candidate_rules = sel.policy.get_priority_rules(sel.talents, sel.race);
            render_priest_priority_chain_subpane(candidate_rules);
            ImGui::Spacing();

            ImGui::Columns(2, "CandidateDetailCols", true);

            // Left Column: Damage Breakdown & Performance
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Damage Breakdown (%% of Total Damage + DPS):");
            const auto& b = sel.batch;
            render_priest_damage_breakdown_bars(b, 180.0f, 130.0f);

            ImGui::Spacing();
            ImGui::Text("Combat Performance:");
            ImGui::BulletText("Shadow Weaving Procs: %.1f", sel.sw_procs);
            ImGui::BulletText("Spell Crit Rate: %.1f%% | Miss Rate: %.1f%%", b.crit_percent, b.miss_percent);
            ImGui::BulletText("Mana Consumed / Gained: %.0f / %.0f", b.mean_mana_spent, b.mean_mana_gained);
            ImGui::BulletText("Min - Max DPS Range: [%.1f - %.1f]", sel.min_dps, sel.max_dps);

            ImGui::NextColumn();

            // Right Column: Observed Spell Cast Sequence & Combat Rotation
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Observed Combat Rotation & Cast Sequence:");

            // Obtain sample cast sequence
            std::vector<SpellCastLog> seq = b.sample_timeline.cast_sequence;
            if (seq.empty()) {
                PriestSimulator s = sim;
                s.race = sel.race;
                s.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, sel.race);
                s.talents = sel.talents;
                s.policy = sel.policy;
                s.buffs = sel.buffs;
                s.record_timeline = true;
                sim::FastRNG rng(0x13374242ULL);
                SimResult res = s.run_single_simulation(rng);
                seq = res.cast_sequence;
            }

            // 1. Opener Sequence Badges (First 16-24 Spells Cast)
            int opener_count = (int)std::min(seq.size(), (size_t)16);
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Cast Sequence (First %d Spells):", opener_count);
            ImGui::BeginChild("PriestOpenerSequenceBox", ImVec2(-1, 64), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (size_t i = 0; i < std::min(seq.size(), (size_t)24); ++i) {
                const auto& cast = seq[i];
                if (i > 0) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("->");
                    ImGui::SameLine();
                }
                ImGui::BeginGroup();
                Texture2D icon = warlock::AssetManager::get().get_icon(spell_id_to_icon(cast.spell_id));
                if (icon.id > 0) {
                    ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(20, 20));
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", spell_id_to_string(cast.spell_id));
                    ImGui::Text("Time: %.1fs  |  Cast Duration: %.1fs", cast.time, cast.cast_time);
                    ImGui::Text("Role: %s", cast.tag.c_str());
                    if (cast.damage > 0.0) {
                        ImGui::TextColored(cast.is_crit ? ImVec4(1.0f, 0.85f, 0.2f, 1.0f) : ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
                                           "Damage: %.0f %s",
                                           cast.damage,
                                           cast.is_crit ? "(CRIT!)" : "");
                    } else if (cast.is_miss) {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Result: MISS / RESIST");
                    }
                    ImGui::EndTooltip();
                }
                ImGui::TextDisabled("%.1fs", cast.time);
                ImGui::EndGroup();
            }
            ImGui::EndChild();

            // 2. Observed Spell Cast Order & Usage Table
            struct SpellStat {
                SpellID id;
                int count = 0;
                double first_cast = -1.0;
                std::string role;
            };
            std::vector<SpellStat> stats;
            int total_observed_casts = (int)seq.size();
            for (const auto& cast : seq) {
                bool found = false;
                for (auto& s : stats) {
                    if (s.id == cast.spell_id) {
                        s.count++;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    SpellStat s;
                    s.id = cast.spell_id;
                    s.count = 1;
                    s.first_cast = cast.time;
                    s.role = cast.tag;
                    stats.push_back(s);
                }
            }
            std::sort(stats.begin(), stats.end(), [](const SpellStat& a, const SpellStat& b) {
                return a.first_cast < b.first_cast;
            });

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Observed Cast Order & Role Breakdown (120s Fight):");
            if (ImGui::BeginTable("PriestObservedSpellsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 18);
                ImGui::TableSetupColumn("Spell", ImGuiTableColumnFlags_WidthFixed, 120);
                ImGui::TableSetupColumn("First Cast", ImGuiTableColumnFlags_WidthFixed, 60);
                ImGui::TableSetupColumn("Casts (Share)", ImGuiTableColumnFlags_WidthFixed, 85);
                ImGui::TableSetupColumn("Combat Role & Behavior", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                int rank = 1;
                for (const auto& st : stats) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("%d", rank++);

                    ImGui::TableNextColumn();
                    Texture2D icon = warlock::AssetManager::get().get_icon(spell_id_to_icon(st.id));
                    if (icon.id > 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
                        ImGui::SameLine();
                    }
                    ImGui::Text("%s", spell_id_to_name(st.id));

                    ImGui::TableNextColumn();
                    ImGui::Text("%.1fs", st.first_cast);

                    ImGui::TableNextColumn();
                    double share = total_observed_casts > 0 ? (100.0 * st.count / total_observed_casts) : 0.0;
                    ImGui::Text("%dx (%.1f%%)", st.count, share);

                    ImGui::TableNextColumn();
                    std::string role_desc;
                    if (st.id == SpellID::SHADOW_WORD_PAIN) {
                        role_desc = "DoT (Shadow Word: Pain; 18s-24s duration)";
                    } else if (st.id == SpellID::DEVOURING_PLAGUE) {
                        role_desc = "DoT (Devouring Plague; 1min CD, 24s disease)";
                    } else if (st.id == SpellID::MIND_BLAST) {
                        role_desc = "Instant Burst Nuke (Cast on 8s / 5.5s CD)";
                    } else if (st.id == SpellID::SHADOW_WORD_DEATH) {
                        role_desc = "Instant Burst / Execute (15s CD; 10% self-damage)";
                    } else if (st.id == SpellID::MIND_FLAY) {
                        role_desc = "Primary Channeled Filler (Clipped after tick 2)";
                    } else if (st.id == SpellID::HOLY_FIRE) {
                        role_desc = "DoT / Nuke (Empowers Smite & Penance +10%)";
                    } else if (st.id == SpellID::PENANCE) {
                        role_desc = "3-pulse Channeled Volley (10s CD)";
                    } else if (st.id == SpellID::SMITE) {
                        role_desc = "Primary Cast Filler (2.0s cast, Divine Fury)";
                    } else if (st.id == SpellID::HOLY_NOVA) {
                        role_desc = "Instant Burst (Clearcast proc free cast)";
                    } else if (st.id == SpellID::STARSHARDS) {
                        role_desc = "Arcane Channeled Racial (30s CD)";
                    } else if (st.id == SpellID::CHASTISE) {
                        role_desc = "Instant Holy Racial Burst";
                    } else if (st.id == SpellID::POWER_INFUSION) {
                        role_desc = "Major DPS Cooldown (+20% damage, -20% mana)";
                    } else if (st.id == SpellID::INNER_FOCUS) {
                        role_desc = "Cooldown (Free spell + 25% crit)";
                    } else if (st.id == SpellID::DARK_SACRIFICE) {
                        role_desc = "Resource Cannibalize (Restores 1600 mana)";
                    } else {
                        role_desc = st.role.empty() ? "Combat Ability" : st.role;
                    }
                    ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", role_desc.c_str());
                }
                ImGui::EndTable();
            }

            // 3. Setup Details
            ImGui::Spacing();
            std::string setup_desc = "Holy / Discipline Caster";
            if (sel.talents.shadow.shadowform > 0) {
                setup_desc = "Shadowform (+10% Shadow Dmg, -50% Mana, 2.0x Crit)";
            } else if (sel.talents.disc.penance > 0) {
                setup_desc = "Discipline Penance Hybrid";
            }
            ImGui::BulletText("Caster Form / Setup: %s", setup_desc.c_str());
            ImGui::BulletText("Mana Management: Potion threshold %.0f%% | Demonic Rune threshold %.0f%%",
                              sel.policy.mana_potion_threshold * 100.0, sel.policy.demonic_rune_threshold * 100.0);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.25f, 0.65f, 1.0f));
            if (ImGui::Button("▶ Load Configuration into Preset Simulation", ImVec2(320, 28))) {
                apply_candidate_config(sel);
            }
            ImGui::PopStyleColor();

            ImGui::Columns(1);
        }
    }
}

} // namespace priest
