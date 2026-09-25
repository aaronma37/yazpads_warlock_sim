#pragma once
#include "asset_manager.hpp"
#include "damage_breakdown_view.hpp"
#include "wow_widgets.hpp"
#include "imgui.h"
#include "panel_policy.hpp"
#include "src/sim/optimizer.hpp"
#include "src/sim/talent_graph.hpp"
#include "src/sim/warlock/apl_optimizer.hpp"
#include "src/sim/warlock/genetic_optimizer.hpp"
#include "src/sim/warlock/surrogate_evaluator.hpp"
#include "src/sim/warlock/viper_oracle.hpp"
#include "src/sim/build_export.hpp"
#include "src/sim/common/zip_writer.hpp"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace warlock
{

#if defined(__EMSCRIPTEN__)
inline GeneticOptimizerSession& get_emscripten_ga_session()
{
  static GeneticOptimizerSession session;
  return session;
}
#endif

// Background worker state for live asynchronous optimization
struct OptimizerWorkerState
{
  std::thread worker;
  std::mutex mtx;
  std::atomic<bool> is_running{false};
  std::atomic<bool> stop_requested{false};
  std::atomic<float> progress{0.0f};
  std::string current_status;
  std::vector<CandidateResult> live_results;
  bool has_new_results = false;
};

inline OptimizerWorkerState& get_opt_worker_state()
{
  static OptimizerWorkerState state;
  return state;
}

// Background worker state for Genetic APL Optimization
struct APLWorkerState
{
  std::thread worker;
  std::mutex mtx;
  std::atomic<bool> is_running{false};
  std::atomic<float> progress{0.0f};
  std::string current_status;
  APLOptimizationResult live_result;
  bool has_result = false;
};

inline APLWorkerState& get_apl_worker_state()
{
  static APLWorkerState state;
  return state;
}

// Background worker state for VIPER policy extraction (legacy)
struct VIPERWorkerState
{
  std::thread worker;
  std::mutex mtx;
  std::atomic<bool> is_running{false};
  std::atomic<float> progress{0.0f};
  std::string current_status;
  VIPEROracle::VIPERExtractionResult live_result;
  bool has_result = false;
};

inline VIPERWorkerState& get_viper_worker_state()
{
  static VIPERWorkerState state;
  return state;
}



inline void render_panel_optimizer(WarlockSimulator& sim,
                                   std::vector<CandidateResult>& optimizer_results,
                                   bool& is_optimizing,
                                   float& opt_progress,
                                   std::string& current_opt_target,
                                   bool* request_switch_to_preset = nullptr,
                                   int opt_mode = 1)
{
#if defined(__EMSCRIPTEN__)
  auto& em_session = get_emscripten_ga_session();
  if (em_session.is_running())
  {
    is_optimizing = true;
    std::vector<CandidateResult> step_elites;
    float step_progress = 0.0f;
    std::string step_status;
    bool more = em_session.step(step_elites, step_progress, step_status);
    if (!step_elites.empty())
    {
      optimizer_results = step_elites;
    }
    opt_progress = step_progress;
    current_opt_target = step_status;

    if (!more)
    {
      optimizer_results = em_session.finish([&](float p, const std::string& status) {
        opt_progress = p;
        current_opt_target = status;
      });
      is_optimizing = false;
      opt_progress = 1.0f;
    }
  }
  else if (is_optimizing && em_session.is_finished())
  {
    is_optimizing = false;
    opt_progress = 1.0f;
  }
#else
  auto& worker = get_opt_worker_state();

  // Check if background worker has new live generation results
  {
    std::lock_guard<std::mutex> lock(worker.mtx);
    if (worker.has_new_results)
    {
      optimizer_results = worker.live_results;
      worker.has_new_results = false;
    }
    if (worker.is_running.load())
    {
      is_optimizing = true;
      opt_progress = worker.progress.load();
      current_opt_target = worker.current_status;
    }
    else if (is_optimizing)
    {
      is_optimizing = false;
      opt_progress = 1.0f;
      if (worker.worker.joinable())
      {
        worker.worker.join();
      }
    }
  }
#endif

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
  static int ga_forced_pet_mode = -1;
#if defined(__EMSCRIPTEN__)
  static int ga_threads = 1;
#else
  static int ga_threads = static_cast<int>(std::thread::hardware_concurrency());
#endif

  static int iters_per_candidate = 3000;
  static bool compare_all_races = false;
  static bool calculate_stat_weights = false;

  if (opt_mode == 0)
  {
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Generations:");
    ImGui::SetNextItemWidth(110);
    if (WowInputInt("##WarlockGAGenerations", &ga_generations))
    {
      if (ga_generations < 1)
        ga_generations = 1;
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 16.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Population:");
    ImGui::SetNextItemWidth(110);
    if (WowInputInt("##WarlockGAPopulation", &ga_pop_size))
    {
      if (ga_pop_size < 2)
        ga_pop_size = 2;
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 16.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Screening Sims:");
    ImGui::SetNextItemWidth(125);
    if (WowInputInt("##WarlockGAScreeningSims", &ga_screening_sims))
    {
      if (ga_screening_sims < 10)
        ga_screening_sims = 10;
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 16.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Final Precision:");
    ImGui::SetNextItemWidth(125);
    if (WowInputInt("##WarlockGAFinalSims", &ga_final_sims))
    {
      if (ga_final_sims < 10)
        ga_final_sims = 10;
    }
    ImGui::EndGroup();

#if !defined(__EMSCRIPTEN__)
    ImGui::SameLine(0.0f, 16.0f);
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Threads:");
    ImGui::SetNextItemWidth(90);
    int max_threads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    if (WowInputInt("##WarlockGAThreads", &ga_threads, 1, 2))
    {
      if (ga_threads < 1)
        ga_threads = 1;
      if (ga_threads > max_threads)
        ga_threads = max_threads;
    }
    ImGui::EndGroup();
#endif

    ImGui::Spacing();
    WowCheckbox("Seed with standard presets", &ga_seed_presets);
    ImGui::SameLine(0.0f, 24.0f);
    WowCheckbox("Optimize Race", &ga_optimize_race);
    ImGui::SameLine(0.0f, 24.0f);
    WowCheckbox("Advanced Convergence Tuning", &show_advanced_tuning);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.3f, 1.0f), "Build Constraints:");

    // Helper lambda for talent selection combo
    auto render_talent_combo = [](const char* label, int& selected_idx)
    {
      const auto& graph = TalentGraph::get();
      std::string preview = "[None]";
      if (selected_idx >= 0 && selected_idx < static_cast<int>(TOTAL_TALENT_NODES))
      {
        const auto& n = graph.node(selected_idx);
        const char* tree_name = (n.tree_idx == 0) ? "Aff" : ((n.tree_idx == 1) ? "Demo" : "Destro");
        preview = std::string(n.name) + " (" + tree_name + ")";
      }

      ImGui::BeginGroup();
      WowResetTextBaseline();
      ImGui::Text("%s:", label);
      ImGui::SetNextItemWidth(200);
      std::string combo_id = std::string("##") + label;
      if (ImGui::BeginCombo(combo_id.c_str(), preview.c_str()))
      {
        if (ImGui::Selectable("[None]", selected_idx == -1))
        {
          selected_idx = -1;
        }
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i)
        {
          const auto& n = graph.node(i);
          const char* tree_name = (n.tree_idx == 0) ? "Aff" : ((n.tree_idx == 1) ? "Demo" : "Destro");
          std::string item_name = std::string(n.name) + " (" + tree_name + ")";
          bool is_selected = (selected_idx == static_cast<int>(i));
          if (ImGui::Selectable(item_name.c_str(), is_selected))
          {
            selected_idx = static_cast<int>(i);
          }
          if (is_selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::EndGroup();
    };

    render_talent_combo("Req Talent 1", ga_req_talent1);
    ImGui::SameLine(0.0f, 16.0f);
    render_talent_combo("Req Talent 2", ga_req_talent2);
    ImGui::SameLine(0.0f, 16.0f);
    render_talent_combo("Req Talent 3", ga_req_talent3);

    ImGui::SameLine(0.0f, 16.0f);
    // Race constraint combo
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Locked Race:");
    ImGui::SetNextItemWidth(150);
    const char* race_names[] = {"[Any / Evolve]", "Undead", "Orc", "Troll", "Human", "Gnome"};
    int current_race_idx = (ga_forced_race >= 0 && ga_forced_race < 5) ? (ga_forced_race + 1) : 0;
    if (ImGui::Combo("##LockedRace", &current_race_idx, race_names, IM_ARRAYSIZE(race_names)))
    {
      ga_forced_race = (current_race_idx == 0) ? -1 : (current_race_idx - 1);
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 16.0f);
    // Rotation constraint combo
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Locked Rotation:");
    ImGui::SetNextItemWidth(210);
    const char* rot_preview = "[Auto / Adaptive]";
    if (ga_forced_rotation >= 0)
    {
      rot_preview = rotation_choice_to_string(static_cast<RotationChoice>(ga_forced_rotation));
    }
    if (ImGui::BeginCombo("##LockedRotation", rot_preview))
    {
      if (ImGui::Selectable("[Auto / Adaptive]", ga_forced_rotation == -1))
      {
        ga_forced_rotation = -1;
      }
      for (int r = 0; r <= static_cast<int>(RotationChoice::DP_AF_SHADOW_BRAND); ++r)
      {
        RotationChoice rc = static_cast<RotationChoice>(r);
        const char* r_str = rotation_choice_to_string(rc);
        bool is_sel = (ga_forced_rotation == r);
        if (ImGui::Selectable(r_str, is_sel))
        {
          ga_forced_rotation = r;
        }
        if (is_sel)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 16.0f);
    // Pet / Demonic Sacrifice constraint combo
    ImGui::BeginGroup();
    WowResetTextBaseline();
    ImGui::Text("Locked Pet / DS:");
    ImGui::SetNextItemWidth(210);
    const char* pet_preview = "[Auto / Adaptive]";
    if (ga_forced_pet_mode >= 0)
    {
      pet_preview = pet_constraint_to_string(static_cast<PetConstraint>(ga_forced_pet_mode));
    }
    if (ImGui::BeginCombo("##LockedPetDS", pet_preview))
    {
      if (ImGui::Selectable("[Auto / Adaptive]", ga_forced_pet_mode == -1))
      {
        ga_forced_pet_mode = -1;
      }
      for (int p = 0; p <= static_cast<int>(PetConstraint::NO_PET); ++p)
      {
        PetConstraint pc = static_cast<PetConstraint>(p);
        const char* p_str = pet_constraint_to_string(pc);
        bool is_sel = (ga_forced_pet_mode == p);
        if (ImGui::Selectable(p_str, is_sel))
        {
          ga_forced_pet_mode = p;
        }
        if (is_sel)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::EndGroup();

    if (show_advanced_tuning)
    {
      ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.18f, 0.6f));
      ImGui::BeginChild("GATuningBox", ImVec2(-1, 88), true);
      
      ImGui::BeginGroup();
      WowResetTextBaseline();
      ImGui::Text("Mutation Rate:");
      ImGui::SetNextItemWidth(130);
      WowInputFloat("##WarlockGAMutationRate", &ga_mutation_rate, 0.05f, 0.1f, "%.2f");
      ImGui::EndGroup();

      ImGui::SameLine(0.0f, 20.0f);
      ImGui::BeginGroup();
      WowResetTextBaseline();
      ImGui::Text("Start Exploration Rate:");
      ImGui::SetNextItemWidth(160);
      WowInputFloat("##WarlockGAInitExplore", &ga_initial_explore, 0.05f, 0.1f, "%.2f");
      ImGui::EndGroup();

      ImGui::SameLine(0.0f, 20.0f);
      ImGui::BeginGroup();
      WowResetTextBaseline();
      ImGui::Text("End Exploration Rate:");
      ImGui::SetNextItemWidth(160);
      WowInputFloat("##WarlockGAMinExplore", &ga_min_explore, 0.05f, 0.1f, "%.2f");
      ImGui::EndGroup();

      ImGui::TextDisabled(
          "Controls simulated annealing schedule: high early exploration prevents getting stuck in local optima.");
      ImGui::EndChild();
      ImGui::PopStyleColor();
    }
  }
  else if (opt_mode == 1)
  {
    ImGui::BeginGroup();
    ImGui::Text("Number of Simulations:");
    ImGui::SetNextItemWidth(180);
    WowInputInt("##WarlockItersPerCandidate", &iters_per_candidate, 500, 2000);
    if (iters_per_candidate < 100) iters_per_candidate = 100;
    ImGui::EndGroup();

    ImGui::SameLine(220);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing()));
    WowCheckbox("Compare across all races", &compare_all_races);
    ImGui::SameLine(0.0f, 20.0f);
    WowCheckbox("Calculate Stat Weights", &calculate_stat_weights);
    ImGui::EndGroup();
  }
#if 0  // Genetic APL search hidden/disabled for now
  else if (opt_mode == 2)
  {
    static int apl_pop_size = 32;
    static int apl_generations = 25;
    static int apl_eval_sims = 80;
    static int apl_bench_sims = 500;
    static float apl_mutation_rate = 0.40f;
    static bool apl_allow_dual_tap = true;
    static bool apl_seed_from_current = true;
    static bool apl_use_simulated_annealing = false;
    static bool apl_applied_notification = false;

    auto& a_worker = get_apl_worker_state();

    // Check background worker completion
    if (!a_worker.is_running.load() && a_worker.worker.joinable())
    {
      a_worker.worker.join();
    }

    ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "Action Priority List (APL) Policy Optimization:");
    ImGui::TextDisabled("Directly evolves discrete rule priorities and tunes continuous activation levers (Life Tap, Bane of Doom, Pandemic DoT windows) under strict structural constraints.");
    ImGui::Spacing();

    bool is_busy = a_worker.is_running.load();

    ImGui::BeginDisabled(is_busy);
    ImGui::BeginGroup();
    ImGui::Text("Pop Size / Epoch Budget:");
    ImGui::SetNextItemWidth(130);
    WowInputInt("##APLPopSize", &apl_pop_size, 4, 16);
    ImGui::EndGroup();

    ImGui::SameLine(180);
    ImGui::BeginGroup();
    ImGui::Text("Generations / Epochs:");
    ImGui::SetNextItemWidth(130);
    WowInputInt("##APLGenerations", &apl_generations, 5, 20);
    ImGui::EndGroup();

    ImGui::SameLine(360);
    ImGui::BeginGroup();
    ImGui::Text("Eval Precision:");
    ImGui::SetNextItemWidth(130);
    WowInputInt("##APLEvalSims", &apl_eval_sims, 20, 100);
    ImGui::EndGroup();

    ImGui::SameLine(540);
    ImGui::BeginGroup();
    ImGui::Text("Benchmark Precision:");
    ImGui::SetNextItemWidth(130);
    WowInputInt("##APLBenchSims", &apl_bench_sims, 50, 200);
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::BeginGroup();
    ImGui::Text("Mutation Rate:");
    ImGui::SetNextItemWidth(130);
    WowInputFloat("##APLMutationRate", &apl_mutation_rate, 0.05f, 0.1f, "%.2f");
    ImGui::EndGroup();

    ImGui::SameLine(180);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 18.0f);
    WowCheckbox("Allow Dual Life Tap", &apl_allow_dual_tap);
    ImGui::SameLine(360);
    WowCheckbox("Seed from Current APL", &apl_seed_from_current);
    ImGui::SameLine(560);
    WowCheckbox("Use Simulated Annealing (SA)", &apl_use_simulated_annealing);
    ImGui::EndDisabled();

    ImGui::Spacing();

    if (!is_busy)
    {
      std::string btn_label = apl_use_simulated_annealing ? "Run Simulated Annealing APL Optimization" : "Run Genetic APL Optimization";
      if (WowButton(btn_label.c_str(), ImVec2(300, 28)))
      {
        apl_applied_notification = false;
        a_worker.has_result = false;
        a_worker.progress.store(0.0f);
        a_worker.current_status = apl_use_simulated_annealing ? "Initializing Simulated Annealing APL optimizer..." : "Initializing Genetic APL optimizer...";
        a_worker.is_running.store(true);

        WarlockSimulator sim_copy = sim;
        APLOptimizerConfig cfg;
        cfg.population_size = static_cast<size_t>(apl_pop_size);
        cfg.generations = static_cast<size_t>(apl_generations);
        cfg.eval_simulations = static_cast<size_t>(apl_eval_sims);
        cfg.benchmark_simulations = static_cast<size_t>(apl_bench_sims);
        cfg.mutation_rate = apl_mutation_rate;
        cfg.allow_dual_life_tap = apl_allow_dual_tap;
        cfg.seed_from_current_apl = apl_seed_from_current;
        cfg.use_simulated_annealing = apl_use_simulated_annealing;
        if (apl_seed_from_current) {
          cfg.custom_seed_rules = sim.policy.get_priority_rules(sim.talents, sim.race);
        }
        cfg.seed = 42;

        if (a_worker.worker.joinable())
        {
          a_worker.worker.join();
        }

        a_worker.worker = std::thread([sim_copy, cfg]() mutable {
          auto& w = get_apl_worker_state();
          auto res = APLOptimizer::optimize_apl(
              sim_copy,
              cfg,
              [&](float p, const std::string& status) {
                w.progress.store(p);
                std::lock_guard<std::mutex> lock(w.mtx);
                w.current_status = status;
              });

          {
            std::lock_guard<std::mutex> lock(w.mtx);
            w.live_result = res;
            w.has_result = true;
          }
          w.is_running.store(false);
          w.progress.store(1.0f);
        });
      }

      ImGui::SameLine();
      if (WowButton("Seed from Current APL & Optimize", ImVec2(280, 28)))
      {
        apl_applied_notification = false;
        a_worker.has_result = false;
        a_worker.progress.store(0.0f);
        a_worker.current_status = "Seeding from active APL and optimizing...";
        a_worker.is_running.store(true);

        WarlockSimulator sim_copy = sim;
        APLOptimizerConfig cfg;
        cfg.population_size = static_cast<size_t>(apl_pop_size);
        cfg.generations = static_cast<size_t>(apl_generations);
        cfg.eval_simulations = static_cast<size_t>(apl_eval_sims);
        cfg.benchmark_simulations = static_cast<size_t>(apl_bench_sims);
        cfg.mutation_rate = apl_mutation_rate;
        cfg.allow_dual_life_tap = apl_allow_dual_tap;
        cfg.seed_from_current_apl = true;
        cfg.use_simulated_annealing = apl_use_simulated_annealing;
        cfg.custom_seed_rules = sim.policy.get_priority_rules(sim.talents, sim.race);
        cfg.seed = 42;

        if (a_worker.worker.joinable())
        {
          a_worker.worker.join();
        }

        a_worker.worker = std::thread([sim_copy, cfg]() mutable {
          auto& w = get_apl_worker_state();
          auto res = APLOptimizer::optimize_apl(
              sim_copy,
              cfg,
              [&](float p, const std::string& status) {
                w.progress.store(p);
                std::lock_guard<std::mutex> lock(w.mtx);
                w.current_status = status;
              });

          {
            std::lock_guard<std::mutex> lock(w.mtx);
            w.live_result = res;
            w.has_result = true;
          }
          w.is_running.store(false);
          w.progress.store(1.0f);
        });
      }
    }
    else
    {
      // Live execution progress bar & status
      float prog = a_worker.progress.load();
      std::string status_msg;
      {
        std::lock_guard<std::mutex> lock(a_worker.mtx);
        status_msg = a_worker.current_status;
      }

      WowProgressBar(prog, ImVec2(360, 24), "");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%s", status_msg.c_str());
    }

    // Results Dashboard
    if (a_worker.has_result)
    {
      APLOptimizationResult res;
      {
        std::lock_guard<std::mutex> lock(a_worker.mtx);
        res = a_worker.live_result;
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Top Header Metrics Banner
      ImGui::TextColored(ImVec4(0.30f, 0.95f, 0.40f, 1.0f), "Genetic APL Optimization Results & Expected Value Analysis");
      ImGui::TextDisabled("Evaluated across %zu generations with %zu-iteration high-precision benchmark | Talent-Constrained & Deduplicated",
                          res.evolution_history.size(), static_cast<size_t>(apl_bench_sims));

      ImGui::Spacing();

      // 3 Large Expected Value Comparison KPI Cards
      float card_w = (ImGui::GetContentRegionAvail().x - 24) / 3.0f;
      if (card_w < 200.0f) card_w = 200.0f;

      // Card 1: Baseline Policy Expected Value
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.14f, 0.18f, 0.90f));
      ImGui::BeginChild("BaselineCard", ImVec2(card_w, 95), true);
      ImGui::TextDisabled("1. BASELINE APL");
      ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%.1f DPS", res.baseline_dps);
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "stddev: +/-%.1f | [%.0f - %.0f]", res.baseline_stddev, res.baseline_min_dps, res.baseline_max_dps);
      ImGui::TextDisabled("Default Preset Rotation");
      ImGui::EndChild();
      ImGui::PopStyleColor();

      ImGui::SameLine();

      // Card 2: Optimized Genetic APL Expected Value
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.19f, 0.14f, 0.90f));
      ImGui::BeginChild("OptimizedAPLCard", ImVec2(card_w, 95), true);
      ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.45f, 1.0f), "2. OPTIMIZED GENETIC APL");
      ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.45f, 1.0f), "%.1f DPS", res.optimized_dps);
      ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.50f, 1.0f), "stddev: +/-%.1f | [%.0f - %.0f]", res.optimized_stddev, res.optimized_min_dps, res.optimized_max_dps);
      ImGui::TextDisabled("Evolved Sequence & Tuned Levers");
      ImGui::EndChild();
      ImGui::PopStyleColor();

      ImGui::SameLine();

      // Card 3: Net Realized Gain
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.16f, 0.22f, 0.90f));
      ImGui::BeginChild("GainCard", ImVec2(card_w, 95), true);
      ImGui::TextColored(ImVec4(0.40f, 0.85f, 1.0f, 1.0f), "3. NET REALIZED GAIN");
      if (res.dps_gain >= 0.0) {
        ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.45f, 1.0f), "+%.1f DPS", res.dps_gain);
        ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.50f, 1.0f), "+%.2f%% Gain (%zu Gens)", res.dps_gain_pct, res.evolution_history.size());
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%.1f DPS", res.dps_gain);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%.2f%% Gain (%zu Gens)", res.dps_gain_pct, res.evolution_history.size());
      }
      ImGui::TextDisabled("Direct Policy Search Result");
      ImGui::EndChild();
      ImGui::PopStyleColor();

      ImGui::Spacing();

      // Action buttons
      if (WowButton("Apply Optimized APL to Active Sim Policy", ImVec2(340, 28)))
      {
        sim.policy.custom_rules = res.optimized_rules;
        sim.policy.use_custom_apl = true;
        apl_applied_notification = true;
      }
      if (apl_applied_notification)
      {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Optimized Genetic APL applied! Check the Rotation tab to view or edit.");
      }

      ImGui::Spacing();

      // Detailed Analysis Tabs
      if (ImGui::BeginTabBar("APLDetailTabBar", ImGuiTabBarFlags_None))
      {
        // Tab 1: Top Candidate Policies Leaderboard Table
        if (ImGui::BeginTabItem("Top Candidate Policies"))
        {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.30f, 0.95f, 0.40f, 1.0f), "Evolved Candidate Policies Leaderboard (%zu Candidates):", res.top_candidates.size());
          ImGui::TextDisabled("Ranked high-precision evaluation of unique genetic chromosome solutions against baseline:");

          ImGui::Spacing();
          if (ImGui::BeginTable("APLLeaderboardTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
          {
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("Policy Variant Name", ImGuiTableColumnFlags_WidthFixed, 230.0f);
            ImGui::TableSetupColumn("Action Priority Chain", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Expected DPS", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Gain vs Baseline", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 65.0f);
            ImGui::TableHeadersRow();

            for (size_t c_idx = 0; c_idx < res.top_candidates.size(); ++c_idx)
            {
              const auto& cand = res.top_candidates[c_idx];
              ImGui::TableNextRow();

              // Rank
              ImGui::TableSetColumnIndex(0);
              if (cand.rank == 1) {
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "#1");
              } else if (cand.rank == 2) {
                ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "#2");
              } else if (cand.rank == 3) {
                ImGui::TextColored(ImVec4(0.80f, 0.50f, 0.20f, 1.0f), "#3");
              } else {
                ImGui::Text("#%d", cand.rank);
              }

              // Variant Name
              ImGui::TableSetColumnIndex(1);
              if (cand.rank == 1) {
                ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.45f, 1.0f), "%s (Champion)", cand.name.c_str());
              } else if (cand.name.find("Baseline") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.85f, 1.0f), "%s", cand.name.c_str());
              } else {
                ImGui::Text("%s", cand.name.c_str());
              }

              // Action Sequence Icons
              ImGui::TableSetColumnIndex(2);
              for (size_t r = 0; r < cand.rules.size(); ++r) {
                if (r > 0) ImGui::SameLine(0.0f, 2.0f);
                Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(APLOptimizer::get_spell_id(cand.rules[r].action)));
                ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip("#%zu: %s (%s)", r + 1, cand.rules[r].name.c_str(), cand.rules[r].condition_summary.c_str());
                }
              }

              // Expected DPS
              ImGui::TableSetColumnIndex(3);
              ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.45f, 1.0f), "%.1f DPS", cand.dps);

              // Gain %
              ImGui::TableSetColumnIndex(4);
              double delta = cand.dps - res.baseline_dps;
              if (delta > 0.05) {
                ImGui::TextColored(ImVec4(0.30f, 1.0f, 0.40f, 1.0f), "+%.1f DPS (+%.2f%%)", delta, cand.gain_pct);
              } else if (delta < -0.05) {
                ImGui::TextColored(ImVec4(1.0f, 0.50f, 0.30f, 1.0f), "%.1f DPS (%.2f%%)", delta, cand.gain_pct);
              } else {
                ImGui::TextDisabled("Baseline (0.0%%)");
              }

              // Apply Button
              ImGui::TableSetColumnIndex(5);
              char btn_id[32];
              std::snprintf(btn_id, sizeof(btn_id), "Apply##cand_%zu", c_idx);
              if (ImGui::SmallButton(btn_id)) {
                sim.policy.custom_rules = cand.rules;
                sim.policy.use_custom_apl = true;
                apl_applied_notification = true;
              }
            }
            ImGui::EndTable();
          }
          ImGui::EndTabItem();
        }

        // Tab 2: Champion Priority Chain Details
        if (ImGui::BeginTabItem("Champion Priority Chain"))
        {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Evolved Priority Sequence (%zu rules):", res.optimized_rules.size());
          ImGui::TextDisabled("Direct top-to-bottom decision chain with bounded activation conditions:");

          ImGui::Spacing();
          render_priority_chain_subpane(res.optimized_rules);

          ImGui::Spacing();
          if (ImGui::BeginTable("APLRulesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
          {
            ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Action / Spell", ImGuiTableColumnFlags_WidthFixed, 260.0f);
            ImGui::TableSetupColumn("Condition / Trigger Criteria", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < res.optimized_rules.size(); ++i)
            {
              const auto& rule = res.optimized_rules[i];
              ImGui::TableNextRow();

              // Priority
              ImGui::TableSetColumnIndex(0);
              ImGui::Text("#%zu", i + 1);

              // Action Name
              ImGui::TableSetColumnIndex(1);
              Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(APLOptimizer::get_spell_id(rule.action)));
              ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
              ImGui::SameLine();
              ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", APLOptimizer::get_action_name(rule.action));

              // Condition description
              ImGui::TableSetColumnIndex(2);
              if (rule.action == PriorityAction::LIFE_TAP) {
                if (rule.max_mana_pct <= 0.25f) {
                  ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Emergency Tap (Mana <= %.0f%%)", rule.max_mana_pct * 100.0f);
                } else {
                  ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.4f, 1.0f), "Maintenance Tap (Mana <= %.0f%% & Time Left >= %.0fs)", rule.max_mana_pct * 100.0f, rule.min_time_remaining);
                }
              } else if (rule.action == PriorityAction::NIGHTFALL_SHADOW_BOLT) {
                ImGui::TextColored(ImVec4(0.9f, 0.5f, 1.0f, 1.0f), "Shadow Trance proc active (Instant Cast)");
              } else if (rule.action == PriorityAction::DECIMATION_SOUL_FIRE) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Decimation active & Target HP <= %.0f%%", res.execute_hp_threshold * 100.0f);
              } else if (rule.action == PriorityAction::CONFLAGRATE) {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Immolate active on target & CD ready");
              } else if (rule.action == PriorityAction::IMMOLATE) {
                ImGui::Text("Target Immolate <= %.2fs & Fight Duration >= 3.0s", res.dot_pandemic_window);
              } else if (rule.action == PriorityAction::CORRUPTION) {
                ImGui::Text("Target Corruption <= %.2fs & Fight Duration >= 4.0s", res.dot_pandemic_window);
              } else if (rule.action == PriorityAction::CURSE_OF_DOOM) {
                ImGui::Text("Fight Time Left >= %.0fs & CD ready", res.curse_of_doom_cutoff);
              } else if (rule.action == PriorityAction::CURSE_OF_AGONY) {
                ImGui::Text("Target Agony <= %.2fs & Bane of Doom not active", res.dot_pandemic_window);
              } else {
                ImGui::TextDisabled("Rotational Filler / Resource Generation");
              }

              // Status
              ImGui::TableSetColumnIndex(3);
              ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "ACTIVE");
            }
            ImGui::EndTable();
          }
          ImGui::EndTabItem();
        }

        // Tab 2: Rule Diff (Baseline vs Optimized)
        if (ImGui::BeginTabItem("Rule Diff (Baseline vs Optimized)"))
        {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Priority Shifts Against Baseline Preset:");
          ImGui::TextDisabled("Highlights priority changes made by the genetic policy search:");

          ImGui::Spacing();
          if (ImGui::BeginTable("APLDiffTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
          {
            ImGui::TableSetupColumn("Action Name", ImGuiTableColumnFlags_WidthFixed, 260.0f);
            ImGui::TableSetupColumn("Baseline Priority", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Optimized Priority", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Shift Status", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& shift : res.rule_shifts)
            {
              ImGui::TableNextRow();

              ImGui::TableSetColumnIndex(0);
              Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(APLOptimizer::get_spell_id(shift.action)));
              ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
              ImGui::SameLine();
              ImGui::Text("%s", shift.name.c_str());

              ImGui::TableSetColumnIndex(1);
              if (shift.baseline_rank > 0) {
                ImGui::Text("#%d", shift.baseline_rank);
              } else {
                ImGui::TextDisabled("[None]");
              }

              ImGui::TableSetColumnIndex(2);
              if (shift.optimized_rank > 0) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "#%d", shift.optimized_rank);
              } else {
                ImGui::TextDisabled("[None]");
              }

              ImGui::TableSetColumnIndex(3);
              if (shift.change_type == "PROMOTED") {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "PROMOTED (+%d positions)", shift.baseline_rank - shift.optimized_rank);
              } else if (shift.change_type == "DEMOTED") {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "DEMOTED (-%d positions)", shift.optimized_rank - shift.baseline_rank);
              } else if (shift.change_type == "NEW") {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "NEW RULE ADDED");
              } else if (shift.change_type == "PRUNED") {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "PRUNED (Talent-locked / Redundant)");
              } else {
                ImGui::TextDisabled("UNCHANGED");
              }
            }
            ImGui::EndTable();
          }
          ImGui::EndTabItem();
        }

        // Tab 3: Continuous Parameter Levers
        if (ImGui::BeginTabItem("Tuned Continuous Parameters"))
        {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Continuous Activation Levers (Evolved via BLX-alpha Blend):");
          ImGui::TextDisabled("Continuous parameters tuned alongside priority permutations under strict domain constraints:");

          ImGui::Spacing();
          if (ImGui::BeginTable("APLLeversTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
          {
            ImGui::TableSetupColumn("Parameter Lever", ImGuiTableColumnFlags_WidthFixed, 240.0f);
            ImGui::TableSetupColumn("Optimized Value", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Structural Bound", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Design Rationale / Invariant", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            // Row 1: Emergency Tap
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Emergency Life Tap Mana");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%.1f%% Mana", res.emergency_tap_mana * 100.0f);
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("[10.0%% - 25.0%%]");
            ImGui::TableSetColumnIndex(3);
            ImGui::TextWrapped("Prevents OOM stalls without wasteful high-mana tapping.");

            // Row 2: Maintenance Tap
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "Maintenance Life Tap Mana");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%.1f%% Mana", res.maint_tap_mana * 100.0f);
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("[25.0%% - 50.0%%]");
            ImGui::TableSetColumnIndex(3);
            ImGui::TextWrapped("Permits opportunistic resource buffering with min 20s combat remaining.");

            // Row 3: Bane of Doom Cutoff
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.9f, 0.5f, 1.0f, 1.0f), "Bane of Doom Cutoff");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%.1fs Remaining", res.curse_of_doom_cutoff);
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("[50.0s - 75.0s]");
            ImGui::TableSetColumnIndex(3);
            ImGui::TextWrapped("Guarantees full 60s tick before combat expiration.");

            // Row 4: DoT Pandemic Window
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "DoT Refresh (Pandemic Window)");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "<= %.2fs Remaining", res.dot_pandemic_window);
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("[0.00s - 2.50s]");
            ImGui::TableSetColumnIndex(3);
            ImGui::TextWrapped("Allows smooth queueing of Corruption/Immolate without clipping tick intervals.");

            // Row 5: Decimation / Execute HP
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Execute HP Threshold");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "<= %.1f%% Boss HP", res.execute_hp_threshold * 100.0f);
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("[20.0%% - 35.0%%]");
            ImGui::TableSetColumnIndex(3);
            ImGui::TextWrapped("Controls when Decimation Soul Fire / Searing Pain execute triggers take priority.");

            ImGui::EndTable();
          }

          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "Domain Invariants Enforced:");
          ImGui::BulletText("Action Uniqueness: Damaging spells cannot appear multiple times in the priority chain.");
          ImGui::BulletText("Mandatory Fallback Filler: Exactly 1 unconditional cast filler (Shadow Bolt / Incinerate) at the end.");
          ImGui::BulletText("Talent Legality: Spells not unlocked in the active talent spec are strictly pruned.");
          ImGui::BulletText("No 95%% Tap Glitch: Continuous parameter bounds mathematically preclude high-mana tapping.");

          ImGui::EndTabItem();
        }

        // Tab 4: Evolution History
        if (!res.evolution_history.empty() && ImGui::BeginTabItem("Evolution History"))
        {
          ImGui::Spacing();
          ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Genetic Policy Convergence History (%zu Generations):", res.evolution_history.size());
          ImGui::TextDisabled("Tracks the elite candidate fitness and population average across evolutionary cycles:");

          ImGui::Spacing();
          if (ImGui::BeginTable("APLEvolutionTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
          {
            ImGui::TableSetupColumn("Generation", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Best Policy DPS", ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn("Population Mean DPS", ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn("Gain over Baseline", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& gen_stat : res.evolution_history)
            {
              ImGui::TableNextRow();

              ImGui::TableSetColumnIndex(0);
              ImGui::Text("Gen #%zu", gen_stat.generation);

              ImGui::TableSetColumnIndex(1);
              ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.45f, 1.0f), "%.1f DPS", gen_stat.best_dps);

              ImGui::TableSetColumnIndex(2);
              ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%.1f DPS", gen_stat.avg_dps);

              ImGui::TableSetColumnIndex(3);
              double delta = gen_stat.best_dps - res.baseline_dps;
              if (delta >= 0.0) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "+%.1f DPS (+%.2f%%)", delta, (delta / std::max(1.0, res.baseline_dps)) * 100.0);
              } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%.1f DPS (%.2f%%)", delta, (delta / std::max(1.0, res.baseline_dps)) * 100.0);
              }
            }
            ImGui::EndTable();
          }
          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      }
    }
  }
#endif

  ImGui::Spacing();

  if (opt_mode == 0)
  {
#if defined(__EMSCRIPTEN__)
    bool is_busy = em_session.is_running();
#else
    bool is_busy = worker.is_running.load();
#endif
    if (!is_busy)
    {
      if (WowButton("Run AI Genetic Optimization", ImVec2(260, 28)))
      {
        WarlockSimulator sim_copy = sim;
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
        if (ga_req_talent1 >= 0)
          req_talents.push_back(ga_req_talent1);
        if (ga_req_talent2 >= 0 && ga_req_talent2 != ga_req_talent1)
          req_talents.push_back(ga_req_talent2);
        if (ga_req_talent3 >= 0 && ga_req_talent3 != ga_req_talent1 && ga_req_talent3 != ga_req_talent2)
          req_talents.push_back(ga_req_talent3);
        int forced_r = ga_forced_race;
        int forced_rot = ga_forced_rotation;
        int forced_pet = ga_forced_pet_mode;

#if defined(__EMSCRIPTEN__)
        GeneticOptimizerConfig cfg;
        cfg.population_size = pop_sz;
        cfg.generations = gens;
        cfg.screening_sims = screen_sims;
        cfg.final_sims = fn_sims;
        cfg.seed_with_presets = seed_pre;
        cfg.optimize_race = opt_race;
        cfg.mutation_rate = mut_rate;
        cfg.initial_exploration_rate = init_exp;
        cfg.min_exploration_rate = min_exp;
        cfg.required_talent_indices = req_talents;
        cfg.forced_race = forced_r;
        cfg.forced_rotation = forced_rot;
        cfg.forced_pet_mode = forced_pet;
        cfg.num_threads = 1;

        em_session.start(sim_copy, cfg);
        is_optimizing = true;
        opt_progress = 0.001f;
        current_opt_target = "Gen 0: Initializing Population...";
        optimizer_results = em_session.get_elites();
#else
        auto& w = get_opt_worker_state();
        w.is_running = true;
        w.stop_requested = false;
        w.progress = 0.0f;
        w.current_status = "Initializing Population...";
        is_optimizing = true;
        opt_progress = 0.0f;
        current_opt_target = w.current_status;
        int th_count = ga_threads;

        if (worker.worker.joinable())
          worker.worker.join();

        worker.worker = std::thread(
            [sim_copy,
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
             forced_pet,
             th_count]()
            {
              auto& w = get_opt_worker_state();
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
                  forced_pet,
                  th_count,
                  [&](float p, const std::string& name)
                  {
                    w.progress = p;
                    std::lock_guard<std::mutex> lk(w.mtx);
                    w.current_status = name;
                  },
                  [&](const std::vector<CandidateResult>& current_elites)
                  {
                    std::lock_guard<std::mutex> lk(w.mtx);
                    w.live_results = current_elites;
                    w.has_new_results = true;
                  },
                  &w.stop_requested);

              {
                std::lock_guard<std::mutex> lk(w.mtx);
                w.live_results = results;
                w.has_new_results = true;
                w.is_running = false;
              }
            });
#endif
      }
    }
    else
    {
      if (WowButton("🛑 Stop Search & Keep Best", ImVec2(220, 28)))
      {
#if defined(__EMSCRIPTEN__)
        em_session.stop();
        optimizer_results = em_session.finish();
        is_optimizing = false;
        opt_progress = 1.0f;
        current_opt_target = "Stopped by user";
#else
        worker.stop_requested = true;
#endif
      }
    }

    if (is_busy)
    {
      ImGui::SameLine(0.0f, 12.0f);
      WowProgressBar(opt_progress, ImVec2(240, 28));
      if (!current_opt_target.empty())
      {
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", current_opt_target.c_str());
      }
    }
  }
  else if (opt_mode == 1)
  {
    static float specs_export_timer = 0.0f;
    static std::string specs_export_msg = "";
    if (specs_export_timer > 0.0f)
    {
      specs_export_timer -= ImGui::GetIO().DeltaTime;
    }

    auto get_current_specs = [&]() -> std::vector<CandidateResult> {
      if (!optimizer_results.empty()) return optimizer_results;
      std::vector<CandidateResult> default_specs;
      const auto& presets = standard_spec_presets();
      for (size_t i = 0; i < presets.size(); ++i) {
        const auto& p = presets[i];
        CandidateResult cand;
        cand.rank = static_cast<int>(i + 1);
        cand.name = p.display_name;
        cand.category = "Talents";
        cand.race = sim.race;
        cand.talents = p.make_talents();
        cand.gear = sim.gear;
        cand.buffs = sim.buffs;
        cand.buffs.sacrifice_succubus = p.sac_succubus;
        cand.buffs.sacrifice_imp = p.sac_imp;
        cand.policy = sim.policy;
        cand.policy.rotation = p.rotation;
        cand.policy.pet = p.pet;
        cand.policy.maintain_immolate = p.maintain_immolate;
        cand.mechanics = sim.mechanics;
        cand.use_raw_stats = sim.use_raw_stats;
        cand.raw_stats = sim.raw_stats;
        default_specs.push_back(cand);
      }
      return default_specs;
    };

#if defined(__EMSCRIPTEN__)
    if (WowButton("Simulate Standard Specs", ImVec2(240, 28), !is_optimizing))
    {
      is_optimizing = true;
      opt_progress = 0.0f;
      optimizer_results = Optimizer::optimize_talents(
          sim,
          iters_per_candidate,
          [&](float p, const std::string& name)
          {
            opt_progress = p;
            current_opt_target = name;
          },
          compare_all_races,
          calculate_stat_weights);
      is_optimizing = false;
      opt_progress = 1.0f;
    }
#else
    auto& worker = get_opt_worker_state();
    bool is_busy = worker.is_running.load();
    if (!is_busy)
    {
      if (WowButton("Simulate Standard Specs", ImVec2(240, 28)))
      {
        WarlockSimulator sim_copy = sim;
        int iters = iters_per_candidate;
        bool all_races = compare_all_races;
        bool calc_weights = calculate_stat_weights;

        worker.is_running = true;
        worker.stop_requested = false;
        worker.progress = 0.0f;
        worker.current_status = "Starting Standard Specs Simulation...";
        is_optimizing = true;
        opt_progress = 0.0f;
        current_opt_target = worker.current_status;

        if (worker.worker.joinable())
          worker.worker.join();

        worker.worker = std::thread(
            [sim_copy, iters, all_races, calc_weights]()
            {
              auto& w = get_opt_worker_state();
              auto results = Optimizer::optimize_talents(
                  sim_copy,
                  iters,
                  [&w](float p, const std::string& name)
                  {
                    w.progress = p;
                    std::lock_guard<std::mutex> lk(w.mtx);
                    w.current_status = name;
                  },
                  all_races,
                  calc_weights);

              {
                std::lock_guard<std::mutex> lk(w.mtx);
                w.live_results = results;
                w.has_new_results = true;
                w.is_running = false;
              }
            });
      }
    }
    else
    {
      if (WowButton("Stop Simulation", ImVec2(240, 28)))
      {
        worker.stop_requested = true;
      }
    }
#endif

    ImGui::SameLine(0.0f, 12.0f);
    if (WowButton("Save All Specs (ZIP)", ImVec2(220, 28)))
    {
      auto specs = get_current_specs();
      auto zip = build_export::create_specs_batch_zip(specs, &sim);
      if (zip.save_to_file("warlock_specs_comparison.zip"))
      {
        specs_export_msg = "Saved warlock_specs_comparison.zip!";
      }
      else
      {
        specs_export_msg = "Failed to save ZIP file";
      }
      specs_export_timer = 3.0f;
    }
    if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip("Save ZIP archive containing individual JSON configs and results for all specs shown, manifest.json, and leaderboard.csv");
    }

    if (is_busy)
    {
      ImGui::SameLine(0.0f, 12.0f);
      WowProgressBar(opt_progress, ImVec2(240, 28));
      if (!current_opt_target.empty())
      {
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", current_opt_target.c_str());
      }
    }

    if (specs_export_timer > 0.0f && !specs_export_msg.empty())
    {
      ImGui::Spacing();
      ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%s", specs_export_msg.c_str());
    }
  }
  else
  {
    if (is_optimizing)
      ImGui::BeginDisabled();
    if (is_optimizing)
      ImGui::EndDisabled();
  }

  ImGui::Separator();

  auto apply_candidate_config = [&](const CandidateResult& r)
  {
    sim.race = r.race;
    sim.base_attrs = get_base_attributes_for_race(sim.race);
    sim.use_raw_stats = r.use_raw_stats;
    sim.raw_stats = r.raw_stats;
    if (r.category == "Talents" || r.category == "Combinatorial Talents" || r.category == "Genetic AI" ||
        r.category == "Diverse Spec Peak (MAP-Elites)" || r.category.find("Peak") != std::string::npos)
    {
      sim.talents = r.talents;
      sim.buffs = r.buffs;
      sim.policy = r.policy;
    }
    else if (r.category == "Gear")
    {
      sim.gear = r.gear;
      sim.use_raw_stats = false;
    }
    else if (r.category == "Consumables")
    {
      sim.buffs = r.buffs;
    }
    else if (r.category == "Stat Values (EP)")
    {
      sim.use_raw_stats = true;
      sim.raw_stats = r.raw_stats;
    }
    else if (r.category == "Policy")
    {
      sim.policy = r.policy;
    }
    else
    {
      sim.talents = r.talents;
      sim.gear = r.gear;
      sim.buffs = r.buffs;
      sim.policy = r.policy;
      sim.mechanics = r.mechanics;
      sim.use_raw_stats = r.use_raw_stats;
      sim.raw_stats = r.raw_stats;
    }
    if (request_switch_to_preset)
    {
      *request_switch_to_preset = true;
    }
  };

  if (!optimizer_results.empty())
  {
    const auto& best = optimizer_results[0];
    static int selected_candidate_idx = 0;
    if (selected_candidate_idx >= static_cast<int>(optimizer_results.size()))
    {
      selected_candidate_idx = 0;
    }

    bool show_stat_weights = false;
    for (const auto& res : optimizer_results)
    {
      if (res.stat_weights.valid)
      {
        show_stat_weights = true;
        break;
      }
    }

    int num_cols = show_stat_weights ? 14 : 8;
    static bool show_std_dev = false;
    if (!show_std_dev)
      num_cols -= 1;  // hide +/- StdDev column

    // Calculate active base combat stats for the header line
    Stats base_stats = sim.use_raw_stats ? sim.raw_stats : sim.gear.calculate_stats();
    sim.buffs.apply_to_stats(base_stats, sim.base_attrs, true, sim.mechanics.personal_shadow_weaving);
    if (sim.race == Race::HUMAN)
    {
      bool is_sword = true;
      if (!sim.use_raw_stats)
      {
        const Item& mh = sim.gear.get(Slot::MAIN_HAND);
        is_sword = (mh.name.find("Mageblade") != std::string::npos || mh.name.find("Sword") != std::string::npos ||
                    mh.name.find("Blade") != std::string::npos || mh.icon.find("sword") != std::string::npos ||
                    mh.icon.find("Sword") != std::string::npos);
      }
      if (is_sword)
        base_stats.spell_crit_percent += 2.0;
    }

    // Spec table header with active base stats matching Build Configuration
    static bool show_pct_from_leader = false;
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
    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "Fire SP: %.0f", base_stats.effective_fire_power());
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Hit: %.1f%%", base_stats.spell_hit_percent);
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::TextColored(
        ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Crit: %.2f%%", base_stats.total_spell_crit(sim.base_attrs.base_spell_crit));
    if (base_stats.spell_haste_percent > 0.0)
    {
      ImGui::SameLine();
      ImGui::TextDisabled("|");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Haste: %.1f%%", base_stats.spell_haste_percent);
    }
    if (base_stats.mp5 > 0.0)
    {
      ImGui::SameLine();
      ImGui::TextDisabled("|");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "MP5: %.0f", base_stats.mp5);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (sim.mechanics.pet_scaling)
    {
      double pet_sp_pct = sim.mechanics.pet_sp_ratio * 100.0;
      double pet_ap_pct = sim.mechanics.pet_ap_ratio * 100.0;
      ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f),
                         (std::fmod(pet_sp_pct, 1.0) == 0.0 ? "Pet SP: %.0f%%" : "Pet SP: %.1f%%"),
                         pet_sp_pct);
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Pet Spell Power inheritance: %.1f%% of master's SP (demon spells)", pet_sp_pct);
      }
      ImGui::SameLine();
      ImGui::TextDisabled("|");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f),
                         (std::fmod(pet_ap_pct, 1.0) == 0.0 ? "Pet AP: %.0f%%" : "Pet AP: %.1f%%"),
                         pet_ap_pct);
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Pet Attack Power inheritance: %.1f%% of master's SP (demon melee)", pet_ap_pct);
      }
    }
    else
    {
      ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "Pet Scaling: Off");
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Pet stat scaling is disabled (Classic 1.12 flat base damage)");
      }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (sim.randomize_duration && sim.duration_variance > 0.0)
    {
      ImGui::TextColored(
          ImVec4(0.95f, 0.75f, 0.45f, 1.0f), "Fight: %.0fs +/- %.0fs", sim.fight_duration, sim.duration_variance);
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Simulated fight duration: %.0fs to %.0fs (mean %.0fs)",
                          std::max(5.0, sim.fight_duration - sim.duration_variance),
                          sim.fight_duration + sim.duration_variance,
                          sim.fight_duration);
      }
    }
    else
    {
      ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.45f, 1.0f), "Fight: %.0fs", sim.fight_duration);
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Simulated fight duration: %.0f seconds", sim.fight_duration);
      }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    WowCheckbox("% from Leader", &show_pct_from_leader);
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    WowCheckbox("Show Std Dev", &show_std_dev);

    if (ImGui::BeginTable("OptLeaderboardTable",
                          num_cols,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
    {
      ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 45);
      ImGui::TableSetupColumn("Spec Name", ImGuiTableColumnFlags_WidthFixed, 190);
      ImGui::TableSetupColumn("Race", ImGuiTableColumnFlags_WidthFixed, 42);
      ImGui::TableSetupColumn("Pet / Sac", ImGuiTableColumnFlags_WidthFixed, 80);
      ImGui::TableSetupColumn("Action Priority Chain", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Damage Split", ImGuiTableColumnFlags_WidthFixed, 150);
      ImGui::TableSetupColumn(show_pct_from_leader ? "% vs Leader" : "Mean DPS", ImGuiTableColumnFlags_WidthFixed, 85);
      if (show_std_dev)
        ImGui::TableSetupColumn("+/- StdDev", ImGuiTableColumnFlags_WidthFixed, 75);
      if (show_stat_weights)
      {
        ImGui::TableSetupColumn("DPS/SP", ImGuiTableColumnFlags_WidthFixed, 65);
        ImGui::TableSetupColumn("DPS/Hit", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("DPS/Crit", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("DPS/Haste", ImGuiTableColumnFlags_WidthFixed, 75);
        ImGui::TableSetupColumn("DPS/Int", ImGuiTableColumnFlags_WidthFixed, 65);
        ImGui::TableSetupColumn("DPS/Spirit", ImGuiTableColumnFlags_WidthFixed, 75);
      }
      ImGui::TableHeadersRow();

      for (size_t i = 0; i < optimizer_results.size(); ++i)
      {
        const auto& r = optimizer_results[i];
        bool is_selected = (selected_candidate_idx == static_cast<int>(i));

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        char rank_sel_id[32];
        std::snprintf(rank_sel_id, sizeof(rank_sel_id), "##row_sel_%zu", i);
        if (ImGui::Selectable(
                rank_sel_id, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
        {
          selected_candidate_idx = static_cast<int>(i);
        }
        ImGui::SameLine(0.0f, 0.0f);
        if (r.rank == 1)
        {
          ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#1");
        }
        else
        {
          ImGui::Text("#%d", r.rank);
        }

        ImGui::TableNextColumn();
        ImGui::PushID(static_cast<int>(i));
        ImVec2 text_size = ImGui::CalcTextSize(r.name.c_str());
        bool clicked_link = ImGui::InvisibleButton("##spec_link", text_size);
        bool hovered_link = ImGui::IsItemHovered();

        if (hovered_link)
        {
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

        if (hovered_link)
        {
          draw_list->AddLine(ImVec2(p_min.x, p_max.y), ImVec2(p_max.x, p_max.y), u32_col, 1.2f);
        }
        else
        {
          ImU32 dim_underline = ImGui::ColorConvertFloat4ToU32(ImVec4(0.40f, 0.75f, 1.0f, 0.40f));
          draw_list->AddLine(ImVec2(p_min.x, p_max.y), ImVec2(p_max.x, p_max.y), dim_underline, 1.0f);
        }

        if (clicked_link)
        {
          selected_candidate_idx = static_cast<int>(i);
          apply_candidate_config(r);
        }
        ImGui::PopID();

        ImGui::TableNextColumn();
        {
          Texture2D race_tex = AssetManager::get().get_icon(race_to_icon(r.race));
          ImGui::Image((ImTextureID)(uintptr_t)race_tex.id, ImVec2(18, 18));
          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::Text("%s", race_to_string(r.race));
            ImGui::EndTooltip();
          }
        }

        ImGui::TableNextColumn();
        {
          // Active pet after Demonic Sacrifice is applied (mirrors WarlockSimulator:
          // without Demonic Pact any sacrifice leaves no active pet; with it, only
          // the sacrificed demon itself is gone).
          PetChoice active = r.policy.pet;
          if (r.buffs.sacrifice_succubus || r.buffs.sacrifice_imp)
          {
            if (r.talents.demo.demonic_pact > 0)
            {
              if (r.buffs.sacrifice_imp && r.policy.pet == PetChoice::IMP)
                active = PetChoice::NONE;
              else if (r.buffs.sacrifice_succubus && r.policy.pet == PetChoice::SUCCUBUS)
                active = PetChoice::NONE;
            }
            else
            {
              active = PetChoice::NONE;
            }
          }
          bool sac_imp = r.buffs.sacrifice_imp;
          bool sac_suc = r.buffs.sacrifice_succubus;

          auto draw_demon_icon = [&](PetChoice p, const char* tip, const char* sub)
          {
            Texture2D tex = AssetManager::get().get_icon(pet_choice_to_icon(p));
            ImGui::Image((ImTextureID)(uintptr_t)tex.id, ImVec2(16, 16));
            if (ImGui::IsItemHovered())
            {
              ImGui::BeginTooltip();
              ImGui::Text("%s", tip);
              ImGui::TextDisabled("%s", sub);
              ImGui::EndTooltip();
            }
          };
          auto draw_empty = [&](const char* tip)
          {
            ImGui::TextDisabled("--");
            if (ImGui::IsItemHovered())
            {
              ImGui::BeginTooltip();
              ImGui::TextDisabled("%s", tip);
              ImGui::EndTooltip();
            }
          };

          if (active == PetChoice::NONE && !sac_imp && !sac_suc)
          {
            draw_empty("No pet or sacrifice");
          }
          else
          {
            const char* pet_icon = pet_choice_to_icon(active);
            if (pet_icon[0] != '\0')
            {
              draw_demon_icon(active, pet_choice_to_string(active), "Active pet");
            }
            else
            {
              draw_empty("No active pet (sacrificed)");
            }
            ImGui::SameLine(0, 4);
            ImGui::TextDisabled("/");
            ImGui::SameLine(0, 4);
            if (!sac_imp && !sac_suc)
            {
              draw_empty("No sacrifice (active pet build)");
            }
            else
            {
              if (sac_imp)
                draw_demon_icon(PetChoice::IMP, "Sacrificed Imp (+15% Shadow damage)", "Sacrificed");
              if (sac_imp && sac_suc)
                ImGui::SameLine(0, 2);
              if (sac_suc)
                draw_demon_icon(PetChoice::SUCCUBUS, "Sacrificed Succubus (+15% Fire damage)", "Sacrificed");
            }
          }
        }

        ImGui::TableNextColumn();
        std::vector<PriorityRule> rules = r.policy.get_priority_rules(r.talents, r.race);
        for (size_t k = 0; k < rules.size(); ++k)
        {
          const auto& rule = rules[k];
          Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(rule.spell_id));
          ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(18, 18));
          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", rule.name.c_str());
            if (!rule.condition_summary.empty())
            {
              ImGui::TextDisabled("%s", rule.condition_summary.c_str());
            }
            if (!rule.trigger_condition.empty())
            {
              ImGui::TextWrapped("%s", rule.trigger_condition.c_str());
            }
            ImGui::EndTooltip();
          }
          if (k + 1 < rules.size())
          {
            ImGui::SameLine(0, 3);
          }
        }

        // Damage Breakdown: 3-section horizontal bar (Shadow, Fire, Pet)
        ImGui::TableNextColumn();
        double shadow_pct = r.batch.pct_shadow_bolt + r.batch.pct_corruption + r.batch.pct_curse +
                            r.batch.pct_siphon_life + r.batch.pct_shadowburn + r.batch.pct_drain_hope +
                            r.batch.pct_drain_life + r.batch.pct_drain_soul + r.batch.pct_bane_of_havoc;
        double fire_pct = r.batch.pct_immolate + r.batch.pct_conflagrate + r.batch.pct_incinerate +
                          r.batch.pct_searing_pain + r.batch.pct_soul_fire;
        double pet_pct = r.batch.pct_pet;

        double total_pct = shadow_pct + fire_pct + pet_pct;
        if (total_pct > 0.0)
        {
          shadow_pct = (shadow_pct / total_pct) * 100.0;
          fire_pct = (fire_pct / total_pct) * 100.0;
          pet_pct = (pet_pct / total_pct) * 100.0;
        }
        else
        {
          if (r.name.find("Fire") != std::string::npos || r.name.find("Incinerate") != std::string::npos)
          {
            shadow_pct = 5.0;
            fire_pct = 95.0;
            pet_pct = 0.0;
          }
          else if (r.name.find("DP") != std::string::npos || r.name.find("Demo") != std::string::npos ||
                   r.name.find("MD") != std::string::npos)
          {
            shadow_pct = 70.0;
            fire_pct = 5.0;
            pet_pct = 25.0;
          }
          else
          {
            shadow_pct = 98.0;
            fire_pct = 2.0;
            pet_pct = 0.0;
          }
        }

        float col_w = ImGui::GetContentRegionAvail().x;
        float bar_w = std::max(40.0f, col_w);
        float bar_h = 16.0f;
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + bar_w, p0.y + bar_h);

        std::string bar_btn_id = "##DmgBar_" + std::to_string(i);
        ImGui::InvisibleButton(bar_btn_id.c_str(), ImVec2(bar_w, bar_h));
        bool is_bar_hovered = ImGui::IsItemHovered();

        draw_list->AddRectFilled(p0, p1, IM_COL32(20, 20, 26, 255), 3.0f);

        float cur_bar_x = p0.x;
        float s_w = (float)(bar_w * (shadow_pct * 0.01));
        float f_w = (float)(bar_w * (fire_pct * 0.01));
        float p_w = (float)(bar_w * (pet_pct * 0.01));

        // 1. Shadow segment (Purple)
        if (shadow_pct > 0.5)
        {
          ImVec2 b0(cur_bar_x, p0.y);
          ImVec2 b1(std::min(p1.x, cur_bar_x + s_w), p1.y);
          draw_list->AddRectFilled(b0, b1, IM_COL32(148, 65, 235, 235), 0.0f);
          cur_bar_x += s_w;
        }

        // 2. Fire segment (Orange)
        if (fire_pct > 0.5)
        {
          ImVec2 b0(cur_bar_x, p0.y);
          ImVec2 b1(std::min(p1.x, cur_bar_x + f_w), p1.y);
          draw_list->AddRectFilled(b0, b1, IM_COL32(245, 115, 30, 235), 0.0f);
          cur_bar_x += f_w;
        }

        // 3. Pet segment (Emerald Green)
        if (pet_pct > 0.5)
        {
          ImVec2 b0(cur_bar_x, p0.y);
          ImVec2 b1(p1.x, p1.y);
          draw_list->AddRectFilled(b0, b1, IM_COL32(40, 195, 90, 235), 0.0f);
        }

        draw_list->AddRect(
            p0, p1, is_bar_hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(65, 65, 80, 255), 3.0f, 0, 1.0f);

        char shadow_txt[16], fire_txt[16], pet_txt[16];
        std::snprintf(shadow_txt, sizeof(shadow_txt), "%.0f%%", shadow_pct);
        std::snprintf(fire_txt, sizeof(fire_txt), "%.0f%%", fire_pct);
        std::snprintf(pet_txt, sizeof(pet_txt), "%.0f%%", pet_pct);

        float txt_y = p0.y + 1.0f;
        if (s_w >= 26.0f)
        {
          ImVec2 sz = ImGui::CalcTextSize(shadow_txt);
          float txt_x = p0.x + (s_w - sz.x) * 0.5f;
          draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), shadow_txt);
          draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), shadow_txt);
        }
        if (f_w >= 26.0f)
        {
          ImVec2 sz = ImGui::CalcTextSize(fire_txt);
          float txt_x = p0.x + s_w + (f_w - sz.x) * 0.5f;
          draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), fire_txt);
          draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), fire_txt);
        }
        if (p_w >= 26.0f)
        {
          ImVec2 sz = ImGui::CalcTextSize(pet_txt);
          float txt_x = p0.x + s_w + f_w + (p_w - sz.x) * 0.5f;
          draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), pet_txt);
          draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), pet_txt);
        }

        if (is_bar_hovered)
        {
          ImGui::BeginTooltip();
          ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Damage Share Breakdown (%s):", r.name.c_str());
          ImGui::Separator();
          ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f),
                             "■ Shadow Damage: %.1f%% (%.1f DPS)",
                             shadow_pct,
                             shadow_pct * 0.01 * r.mean_dps);
          ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.20f, 1.0f),
                             "■ Fire Damage:   %.1f%% (%.1f DPS)",
                             fire_pct,
                             fire_pct * 0.01 * r.mean_dps);
          ImGui::TextColored(ImVec4(0.30f, 0.95f, 0.50f, 1.0f),
                             "■ Pet Damage:    %.1f%% (%.1f DPS)",
                             pet_pct,
                             pet_pct * 0.01 * r.mean_dps);
          ImGui::EndTooltip();
        }

        ImGui::TableNextColumn();
        if (show_pct_from_leader)
        {
          if (best.mean_dps > 0.0)
          {
            double pct_diff = ((r.mean_dps - best.mean_dps) / best.mean_dps) * 100.0;
            if (r.rank == 1)
            {
              ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "= Leader");
            }
            else
            {
              ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%.2f%%", pct_diff);
            }
          }
          else
          {
            ImGui::TextDisabled("-");
          }
        }
        else
        {
          ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%.1f", r.mean_dps);
        }

        if (show_std_dev)
        {
          ImGui::TableNextColumn();
          ImGui::TextDisabled("+/- %.1f", r.std_dev_dps);
        }

        if (show_stat_weights)
        {
          ImGui::TableNextColumn();
          if (r.stat_weights.valid)
          {
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "+%.2f", r.stat_weights.dps_per_sp);
          }
          else
          {
            ImGui::TextDisabled("-");
          }

          ImGui::TableNextColumn();
          if (r.stat_weights.valid)
          {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "+%.1f", r.stat_weights.dps_per_hit);
          }
          else
          {
            ImGui::TextDisabled("-");
          }

          ImGui::TableNextColumn();
          if (r.stat_weights.valid)
          {
            ImGui::TextColored(ImVec4(0.9f, 0.4f, 1.0f, 1.0f), "+%.1f", r.stat_weights.dps_per_crit);
          }
          else
          {
            ImGui::TextDisabled("-");
          }

          ImGui::TableNextColumn();
          if (r.stat_weights.valid)
          {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.8f, 1.0f), "+%.1f", r.stat_weights.dps_per_haste);
          }
          else
          {
            ImGui::TextDisabled("-");
          }

          ImGui::TableNextColumn();
          if (r.stat_weights.valid)
          {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "+%.2f", r.stat_weights.dps_per_int);
          }
          else
          {
            ImGui::TextDisabled("-");
          }

          ImGui::TableNextColumn();
          if (r.stat_weights.valid)
          {
            ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.8f, 1.0f), "+%.2f", r.stat_weights.dps_per_spirit);
          }
          else
          {
            ImGui::TextDisabled("-");
          }
        }
      }

      ImGui::EndTable();
    }

    // Candidate Detail Inspector
    if (selected_candidate_idx >= 0 && selected_candidate_idx < static_cast<int>(optimizer_results.size()))
    {
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
      std::vector<PriorityRule> candidate_rules = sel.policy.get_priority_rules(sel.talents, sim.race);
      render_priority_chain_subpane(candidate_rules);
      ImGui::Spacing();

      ImGui::Columns(2, "CandidateDetailCols", true);

      // Left Column: Damage Breakdown & Performance
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Damage Breakdown (%% of Total Damage + DPS):");
      const auto& b = sel.batch;
      render_damage_breakdown_bars(b, 180.0f, 130.0f);

      ImGui::Spacing();
      ImGui::Text("Combat Performance:");
      ImGui::BulletText("ISB Vulnerability Uptime: %.1f%%", sel.isb_uptime);
      ImGui::BulletText("Spell Crit Rate: %.1f%% | Miss Rate: %.1f%%", b.crit_percent, b.miss_percent);
      ImGui::BulletText("Life Taps per fight: %.1f (Mana spent: %.0f)", b.mean_life_taps, b.mean_mana_spent);
      ImGui::BulletText("Min - Max DPS Range: [%.1f - %.1f]", sel.min_dps, sel.max_dps);

      if (sel.stat_weights.valid)
      {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Local Stat Sensitivity / Weights (DPS per +1 Stat):");
        ImGui::BulletText("+1 Spell Power:  %.2f DPS", sel.stat_weights.dps_per_sp);
        ImGui::BulletText(
            "+1%% Spell Hit:   %.1f DPS (EP: %.1f SP)",
            sel.stat_weights.dps_per_hit,
            sel.stat_weights.dps_per_sp > 0 ? (sel.stat_weights.dps_per_hit / sel.stat_weights.dps_per_sp) : 0.0);
        ImGui::BulletText(
            "+1%% Spell Crit:  %.1f DPS (EP: %.1f SP)",
            sel.stat_weights.dps_per_crit,
            sel.stat_weights.dps_per_sp > 0 ? (sel.stat_weights.dps_per_crit / sel.stat_weights.dps_per_sp) : 0.0);
        ImGui::BulletText(
            "+1%% Spell Haste: %.1f DPS (EP: %.1f SP)",
            sel.stat_weights.dps_per_haste,
            sel.stat_weights.dps_per_sp > 0 ? (sel.stat_weights.dps_per_haste / sel.stat_weights.dps_per_sp) : 0.0);
        ImGui::BulletText("+1 Intellect:     %.2f DPS", sel.stat_weights.dps_per_int);
        ImGui::BulletText(
            "+1 Spirit:        %.2f DPS (EP: %.2f SP)",
            sel.stat_weights.dps_per_spirit,
            sel.stat_weights.dps_per_sp > 0 ? (sel.stat_weights.dps_per_spirit / sel.stat_weights.dps_per_sp) : 0.0);
      }

      ImGui::NextColumn();

      // Right Column: Observed Spell Cast Sequence & Combat Rotation
      static bool opt_show_all_damage_instances = false;
      ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Observed Combat Rotation & Cast Sequence:");
      ImGui::SameLine();
      WowCheckbox("Show All Damage Instances (vs Casts)##WarlockOptDamageToggle", &opt_show_all_damage_instances);

      // Obtain sample sequence
      std::vector<SpellCastLog> seq;
      if (opt_show_all_damage_instances)
      {
        if (b.sample_timeline.timeline.empty())
        {
          WarlockSimulator s = sim;
          s.talents = sel.talents;
          s.policy = sel.policy;
          s.buffs = sel.buffs;
          s.record_timeline = true;
          FastRNG rng(0x13374242ULL);
          SimResult res = s.run_single_simulation(rng);
          seq = res.get_damage_sequence();
        }
        else
        {
          seq = b.sample_timeline.get_damage_sequence();
        }
      }
      else
      {
        seq = b.sample_timeline.cast_sequence;
        if (seq.empty())
        {
          WarlockSimulator s = sim;
          s.talents = sel.talents;
          s.policy = sel.policy;
          s.buffs = sel.buffs;
          s.record_timeline = true;
          FastRNG rng(0x13374242ULL);
          SimResult res = s.run_single_simulation(rng);
          seq = res.cast_sequence;
        }
      }

      // 1. Opener Sequence Badges (First 16-24 Spells Cast / Damage Events)
      int opener_count = (int)std::min(seq.size(), (size_t)16);
      if (opt_show_all_damage_instances)
      {
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Damage Sequence (First %d Events):", opener_count);
      }
      else
      {
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Opener Cast Sequence (First %d Spells):", opener_count);
      }
      ImGui::BeginChild("OpenerSequenceBox", ImVec2(-1, 64), true, ImGuiWindowFlags_HorizontalScrollbar);
      for (size_t i = 0; i < std::min(seq.size(), (size_t)24); ++i)
      {
        const auto& cast = seq[i];
        if (i > 0)
        {
          ImGui::SameLine();
          ImGui::TextDisabled("->");
          ImGui::SameLine();
        }
        ImGui::BeginGroup();
        Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(cast.spell_id));
        ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(20, 20));
        if (ImGui::IsItemHovered())
        {
          ImGui::BeginTooltip();
          ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", spell_id_to_name(cast.spell_id));
          if (opt_show_all_damage_instances)
          {
            ImGui::Text("Time: %.1fs", cast.time);
            ImGui::Text("Type: %s", cast.tag.c_str());
          }
          else
          {
            ImGui::Text("Time: %.1fs  |  Cast Duration: %.1fs", cast.time, cast.cast_time);
            ImGui::Text("Role: %s", cast.tag.c_str());
          }
          if (cast.damage > 0.0)
          {
            ImGui::TextColored(cast.is_crit ? ImVec4(1.0f, 0.85f, 0.2f, 1.0f) : ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
                               "Damage: %.0f %s",
                               cast.damage,
                               cast.is_crit ? "(CRIT!)" : "");
          }
          else if (cast.is_miss)
          {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Result: MISS / RESIST");
          }
          ImGui::EndTooltip();
        }
        ImGui::TextDisabled("%.1fs", cast.time);
        ImGui::EndGroup();
      }
      ImGui::EndChild();

      // 2. Observed Spell Cast Order & Usage Table
      struct SpellStat
      {
        SpellID id;
        int count = 0;
        double first_cast = -1.0;
        std::string role;
      };
      std::vector<SpellStat> stats;
      int total_observed_casts = (int)seq.size();
      for (const auto& cast : seq)
      {
        bool found = false;
        for (auto& s : stats)
        {
          if (s.id == cast.spell_id)
          {
            s.count++;
            found = true;
            break;
          }
        }
        if (!found)
        {
          SpellStat s;
          s.id = cast.spell_id;
          s.count = 1;
          s.first_cast = cast.time;
          s.role = cast.tag;
          stats.push_back(s);
        }
      }
      std::sort(stats.begin(),
                stats.end(),
                [](const SpellStat& a, const SpellStat& b) { return a.first_cast < b.first_cast; });

      ImGui::Spacing();
      if (opt_show_all_damage_instances)
      {
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Observed Damage Events Breakdown (120s Fight):");
      }
      else
      {
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Observed Cast Order & Role Breakdown (120s Fight):");
      }
      if (ImGui::BeginTable("ObservedSpellsTable",
                            5,
                            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
      {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 18);
        ImGui::TableSetupColumn(opt_show_all_damage_instances ? "Spell / Source" : "Spell", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn(opt_show_all_damage_instances ? "First Hit" : "First Cast", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn(opt_show_all_damage_instances ? "Hits (Share)" : "Casts (Share)", ImGuiTableColumnFlags_WidthFixed, 85);
        ImGui::TableSetupColumn(opt_show_all_damage_instances ? "Event Type" : "Combat Role & Behavior", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        int rank = 1;
        for (const auto& st : stats)
        {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TextDisabled("%d", rank++);

          ImGui::TableNextColumn();
          Texture2D icon = AssetManager::get().get_icon(spell_id_to_icon(st.id));
          ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(16, 16));
          ImGui::SameLine();
          ImGui::Text("%s", spell_id_to_name(st.id));

          ImGui::TableNextColumn();
          ImGui::Text("%.1fs", st.first_cast);

          ImGui::TableNextColumn();
          double share = total_observed_casts > 0 ? (100.0 * st.count / total_observed_casts) : 0.0;
          ImGui::Text("%dx (%.1f%%)", st.count, share);

          ImGui::TableNextColumn();
          std::string role_desc;
          if (opt_show_all_damage_instances)
          {
            role_desc = st.role;
          }
          else if (st.id == SpellID::CURSE_OF_AGONY)
          {
            role_desc = "DoT (Bane of Agony; maintained every 24s)";
          }
          else if (st.id == SpellID::CURSE_OF_DOOM)
          {
            role_desc = "Curse (Cast on 60s cooldown)";
          }
          else if (st.id == SpellID::CURSE_OF_SHADOWS || st.id == SpellID::CURSE_OF_ELEMENTS)
          {
            role_desc = "Raid Debuff";
          }
          else if (st.id == SpellID::CORRUPTION)
          {
            role_desc = "DoT (Maintained every 18s; Nightfall)";
          }
          else if (st.id == SpellID::IMMOLATE)
          {
            role_desc = "DoT (Maintained every 15s; buffs Destro)";
          }
          else if (st.id == SpellID::SOUL_FIRE)
          {
            role_desc = "Decimation Execute (Spammed <35% HP)";
          }
          else if (st.id == SpellID::SHADOWBURN)
          {
            role_desc = "Instant Burst (Cast on 8s cooldown)";
          }
          else if (st.id == SpellID::CONFLAGRATE)
          {
            role_desc = "Instant Burst (Cast on 10s cooldown)";
          }
          else if (st.id == SpellID::SHADOW_BOLT)
          {
            role_desc = "Primary Cast Filler (2.5s cast, Bane)";
          }
          else if (st.id == SpellID::INCINERATE)
          {
            role_desc = "Primary Cast Filler (2.0s cast, Bane)";
          }
          else if (st.id == SpellID::DRAIN_HOPE)
          {
            role_desc = "Channeled Execute (6.0s channel)";
          }
          else if (st.id == SpellID::LIFE_TAP)
          {
            role_desc = "Resource Tap (Cast when mana drops low)";
          }
          else
          {
            role_desc = st.role;
          }
          ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", role_desc.c_str());
        }
        ImGui::EndTable();
      }

      // 3. Pet & Setup Details
      ImGui::Spacing();
      std::string pet_desc;
      if (sel.buffs.sacrifice_imp && sel.buffs.sacrifice_succubus)
      {
        pet_desc = "Double Sacrificed";
      }
      else if (sel.talents.demo.demonic_pact > 0)
      {
        pet_desc = "Demonic Pact: Sac Imp (+15% Shadow) + Active Succubus (+10% MD, +60 SP, +3% SL)";
      }
      else if (sel.buffs.sacrifice_imp)
      {
        pet_desc = "Sacrificed Imp (+15% Shadow Damage)";
      }
      else if (sel.buffs.sacrifice_succubus)
      {
        pet_desc = "Sacrificed Succubus (+15% Fire Damage)";
      }
      else
      {
        pet_desc = pet_choice_to_string(sel.policy.pet);
      }
      ImGui::BulletText("Demon Pet / Sacrifice: %s", pet_desc.c_str());
      ImGui::BulletText("Mana Conservation: Life Tap below %.0f%% Mana", sel.policy.life_tap_threshold_pct);

      ImGui::Spacing();
      ImGui::Separator();
      if (WowButton("▶ Load Configuration into Preset Simulation", ImVec2(320, 28)))
      {
        apply_candidate_config(sel);
      }

      ImGui::Columns(1);
    }
  }
  else {}
}

}  // namespace warlock
