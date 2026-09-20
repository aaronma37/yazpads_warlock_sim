#pragma once
#include "imgui.h"
#include "src/ui/common/asset_manager.hpp"
#include "src/sim/priest/optimizer.hpp"
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>

namespace priest {

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

    // Sync live state from background worker
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

    static int opt_mode = 1; // 0 = Genetic Search, 1 = Standard Specs Benchmark
    ImGui::RadioButton("Standard Specs Benchmark", &opt_mode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Genetic AI Search", &opt_mode, 0);

    ImGui::Spacing();

    static int iters_per_candidate = 2500;
    static bool compare_all_races = true;

    static int ga_pop_size = 40;
    static int ga_generations = 60;
    static int ga_screening_sims = 200;
    static int ga_final_sims = 1500;
    static bool ga_seed_presets = true;
    static bool ga_optimize_race = true;

    if (opt_mode == 1) {
        ImGui::SetNextItemWidth(180);
        ImGui::SliderInt("Iterations / Spec", &iters_per_candidate, 500, 10000, "%d fights");
        ImGui::SameLine(320);
        ImGui::Checkbox("Benchmark Across All Playable Races (Human, Dwarf, Night Elf, Undead, Troll)", &compare_all_races);
    } else {
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("Generations", &ga_generations);
        if (ga_generations < 1) ga_generations = 1;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("Population", &ga_pop_size);
        if (ga_pop_size < 4) ga_pop_size = 4;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110);
        ImGui::InputInt("Screening Sims", &ga_screening_sims);
        if (ga_screening_sims < 10) ga_screening_sims = 10;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110);
        ImGui::InputInt("Final Precision", &ga_final_sims);
        if (ga_final_sims < 10) ga_final_sims = 10;

        ImGui::Spacing();
        ImGui::Checkbox("Seed with standard specs", &ga_seed_presets);
        ImGui::SameLine(260);
        ImGui::Checkbox("Evolve Race in Genome", &ga_optimize_race);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Start / Cancel Controls
    if (!is_optimizing) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.75f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.55f, 0.88f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.35f, 0.65f, 1.0f));

        const char* btn_text = (opt_mode == 1) ? ">>> RUN STANDARD SPECS BENCHMARK <<<" : ">>> START GENETIC TALENT SEARCH <<<";
        if (ImGui::Button(btn_text, ImVec2(-1, 36))) {
            if (worker.worker.joinable()) {
                worker.worker.join();
            }
            worker.is_running = true;
            worker.stop_requested = false;
            worker.progress = 0.0f;
            worker.current_status = "Starting optimization...";
            is_optimizing = true;
            opt_progress = 0.0f;

            PriestSimulator sim_copy = sim;
            int mode = opt_mode;
            int iters = iters_per_candidate;
            bool cmp_races = compare_all_races;
            int pop = ga_pop_size;
            int gens = ga_generations;
            int scr = ga_screening_sims;
            int fnl = ga_final_sims;
            bool seed = ga_seed_presets;
            bool opt_r = ga_optimize_race;

            worker.worker = std::thread([sim_copy, mode, iters, cmp_races, pop, gens, scr, fnl, seed, opt_r]() {
                auto& w = get_priest_opt_worker_state();
                std::vector<CandidateResult> results;

                if (mode == 1) {
                    results = Optimizer::optimize_talents(
                        sim_copy,
                        iters,
                        [&](float p, const std::string& name) {
                            w.progress = p;
                            std::lock_guard<std::mutex> lk(w.mtx);
                            w.current_status = name;
                        },
                        cmp_races
                    );
                } else {
                    results = Optimizer::optimize_genetic_ai(
                        sim_copy,
                        pop,
                        gens,
                        scr,
                        fnl,
                        seed,
                        opt_r,
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
                }

                {
                    std::lock_guard<std::mutex> lk(w.mtx);
                    w.live_results = results;
                    w.has_new_results = true;
                    w.is_running = false;
                    w.progress = 1.0f;
                    w.current_status = "Optimization Complete";
                }
            });
        }
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.30f, 0.30f, 1.0f));
        if (ImGui::Button("CANCEL OPTIMIZATION", ImVec2(-1, 36))) {
            worker.stop_requested = true;
        }
        ImGui::PopStyleColor(2);

        ImGui::ProgressBar(opt_progress, ImVec2(-1, 8));
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Progress: %.1f%% | %s", opt_progress * 100.0f, current_opt_target.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Results Display Table
    if (optimizer_results.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No optimizer runs recorded yet. Select options above and click Run to benchmark configurations.");
        return;
    }

    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Ranked Results (%zu configurations evaluated):", optimizer_results.size());
    ImGui::Spacing();

    if (ImGui::BeginTable("PriestOptResultsTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 480))) {
        ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 45);
        ImGui::TableSetupColumn("Configuration / Spec", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableSetupColumn("Race", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Mean DPS", ImGuiTableColumnFlags_WidthFixed, 90);
        ImGui::TableSetupColumn("+/- StdDev", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("SW Procs", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Mana Spent / Gained", ImGuiTableColumnFlags_WidthFixed, 140);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < optimizer_results.size(); ++i) {
            const auto& r = optimizer_results[i];
            ImGui::TableNextRow();

            // Rank
            ImGui::TableSetColumnIndex(0);
            if (r.rank == 1) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#%d", r.rank);
            } else if (r.rank <= 3) {
                ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "#%d", r.rank);
            } else {
                ImGui::Text("#%d", r.rank);
            }

            // Name
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.70f, 1.0f), "%s", r.name.c_str());

            // Category
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("%s", r.category.c_str());

            // Race
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", sim::race_to_string(r.race));

            // Mean DPS
            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.1f", r.mean_dps);

            // StdDev
            ImGui::TableSetColumnIndex(5);
            ImGui::TextDisabled("+/- %.1f", r.std_dev_dps);

            // SW Procs
            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%.1f", r.sw_procs);

            // Mana
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%.0f / %.0f", r.mean_mana_spent, r.mean_mana_gained);

            // Action Button
            ImGui::TableSetColumnIndex(8);
            std::string btn_id = "Apply##" + std::to_string(i);
            if (ImGui::Button(btn_id.c_str(), ImVec2(-1, 20))) {
                sim.talents = r.talents;
                sim.race = r.race;
                sim.base_attrs = sim::get_base_attributes_for_class_and_race(sim::PlayerClass::PRIEST, r.race);
                sim.policy = r.policy;
                if (request_switch_to_preset) {
                    *request_switch_to_preset = true;
                }
            }
        }

        ImGui::EndTable();
    }
}

} // namespace priest
