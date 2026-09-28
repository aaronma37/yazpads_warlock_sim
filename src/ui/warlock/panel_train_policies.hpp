#pragma once
#include "src/sim/warlock/ppo_training.hpp"
#include "src/sim/warlock/imitation_training.hpp"
#include "implot.h"
#include "src/ui/common/ui_theme.hpp"
#include <future>
#include <mutex>
#include <filesystem>

namespace warlock {
// Owned by the app: no detached worker can outlive the panel or its simulator.
struct PolicyTrainingPanel {
    int approach = 0;
    ImitationConfig imitation_config;
    ImitationProgress imitation_progress;
    std::unique_ptr<ImitationTrainer> imitation_trainer;
    char imitation_bundle[512] = "imitation_policy";
    int updates = 100;
    int seed = 1337;
    int evaluation_episodes = 20;
    char checkpoint[512] = "ppo_gru_policy.tar";
    std::atomic<bool> cancel{false};
    std::mutex mutex;
    PPOTrainingProgress progress;
    std::vector<float> rewards;
    std::future<void> worker;
    bool training_job = false;
    std::unique_ptr<PPOTrainer> trainer;
    PPOEvaluation evaluation;
    bool has_evaluation = false;
    std::string status = "Ready. Start training with the current Warlock configuration.";
    ~PolicyTrainingPanel() {
        cancel = true;
        if (worker.valid()) worker.wait();
    }
    bool busy() const { return worker.valid(); }
    void poll() {
        if (worker.valid() && worker.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            try { worker.get(); }
            catch (const std::exception& error) { status = std::string("Error: ") + error.what(); }
        }
    }
    void render(WarlockSimulator& simulator) {
        poll();
        ImGui::BeginChild("TrainPoliciesPane", ImVec2(0, 0), true);
        ImGui::BeginDisabled(busy());
        ImGui::Combo("Training approach", &approach, "PPO + GRU\0C++ GBDT + search imitation\0");
        ImGui::EndDisabled();
        if (approach == 1) {
            render_imitation(simulator);
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored(wow_colors::Gold, "Train Policies - CPU PPO + GRU");
        ImGui::TextWrapped("Train a recurrent policy from combat damage using rl-tools. Training snapshots the current gear, talents, buffs, target and fight settings.");
        ImGui::TextWrapped("GRU actor + critic: 32 units each. 4 environments x 128 decisions; 4 PPO epochs; clip 0.2; gamma 0.99; GAE lambda 0.95. CPU backend.");
        ImGui::TextWrapped("The policy learns Gaussian spell-preference vectors. The simulator tries spells in preference order using its eligibility rules. Pets, multi-target upkeep and cooldown automation retain the captured configuration.");
        ImGui::Separator();
        ImGui::BeginDisabled(busy());
        ImGui::SetNextItemWidth(160); ImGui::InputInt("Training updates", &updates);
        ImGui::SetNextItemWidth(160); ImGui::InputInt("Random seed", &seed);
        ImGui::SetNextItemWidth(160); ImGui::InputInt("Evaluation fights", &evaluation_episodes);
        updates = std::clamp(updates, 1, 100000);
        evaluation_episodes = std::clamp(evaluation_episodes, 1, 10000);
        ImGui::SetNextItemWidth(420); ImGui::InputText("Policy checkpoint", checkpoint, sizeof(checkpoint));
        if (ImGui::Button("Train new policy")) {
            training_job = true;
            cancel = false;
            has_evaluation = false;
            { std::lock_guard lock(mutex); progress = {}; rewards.clear(); }
            status = "Training...";
            const auto snapshot = simulator;
            const int requested_updates = updates;
            const auto requested_seed = static_cast<unsigned>(seed);
            worker = std::async(std::launch::async, [this, snapshot, requested_updates, requested_seed] {
                trainer = std::make_unique<PPOTrainer>(snapshot, requested_seed);
                trainer->train(requested_updates, cancel, [this](PPOTrainingProgress value) {
                    std::lock_guard lock(mutex);
                    progress = value; rewards.push_back(static_cast<float>(value.mean_reward));
                });
                // UI reads status only after future completion.
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Load policy")) {
            training_job = false;
            cancel = false;
            status = "Loading policy...";
            const auto snapshot = simulator;
            const std::string path = checkpoint;
            const auto requested_seed = static_cast<unsigned>(seed);
            worker = std::async(std::launch::async, [this, snapshot, path, requested_seed] {
                auto loaded = std::make_unique<PPOTrainer>(snapshot, requested_seed);
                loaded->load_policy(path);
                trainer = std::move(loaded);
            });
            has_evaluation = false;
        }
        ImGui::EndDisabled();
        if (busy()) {
            if (training_job && ImGui::Button("Stop after current PPO update")) cancel = true;
            ImGui::TextUnformatted(cancel ? "Stopping after the current update..." : "Working...");
        } else {
            ImGui::BeginDisabled(!trainer);
            if (ImGui::Button("Evaluate policy vs captured APL")) {
                training_job = false;
                cancel = false;
                status = "Evaluating...";
                const int count = evaluation_episodes;
                const auto eval_seed = static_cast<unsigned>(seed) ^ 0x9e3779b9u;
                worker = std::async(std::launch::async, [this, count, eval_seed] {
                    evaluation = trainer->evaluate(count, eval_seed);
                    has_evaluation = true;
                });
            }
            ImGui::SameLine();
            if (ImGui::Button("Save policy")) {
                // Avoid silently replacing a previous training run.
                if (std::filesystem::exists(checkpoint)) status = "That file already exists. Choose a new checkpoint filename.";
                else {
                    try { trainer->save_policy(checkpoint); status = std::string("Saved ") + checkpoint; }
                    catch (const std::exception& error) { status = error.what(); }
                }
            }
            ImGui::EndDisabled();
        }
        {
            std::lock_guard lock(mutex);
            ImGui::ProgressBar(static_cast<float>(progress.updates) / updates, ImVec2(420, 0));
            ImGui::Text("%d updates | %d decisions | %.1f seconds", progress.updates, progress.transitions, progress.elapsed_seconds);
            if (!rewards.empty()) ImGui::PlotLines("Mean reward / decision", rewards.data(), static_cast<int>(rewards.size()), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(620, 140));
        }
        if (!busy()) {
            if (status == "Training..." || status == "Loading policy..." || status == "Evaluating...")
                status = cancel ? "Stopped. The current policy can be evaluated or saved." : "Finished. The policy can be evaluated or saved.";
            ImGui::TextWrapped("%s", status.c_str());
            if (has_evaluation) {
                ImGui::Text("Policy: %.1f DPS | Captured APL: %.1f DPS | Difference: %+.1f DPS", evaluation.policy_dps, evaluation.baseline_dps, evaluation.policy_dps - evaluation.baseline_dps);
                ImGui::TextWrapped("Evaluation uses deterministic GRU inference, resets memory every fight, and compares the same held-out seed list. A saved file contains actor weights; it is not a resumable optimizer checkpoint.");
            }
        }
        ImGui::TextWrapped("Rollouts restart after 128 decisions; longer fights may not reach their late phases during training. Checkpoints contain the policy only, so load them with the same character build. Main-simulator APL selection is unchanged.");
        ImGui::EndChild();
    }
    void render_imitation(WarlockSimulator& simulator) {
        ImGui::TextColored(wow_colors::Gold, "C++ GBDT imitation from Monte Carlo search");
        ImGui::TextWrapped("Search tries executable actions from the identical combat state, samples future outcomes, and rolls out to fight end. Each round fits our existing C++ LightGBM-style GBDT to relative search action values, then collects new states with the learner. Gear, talents and fight settings are captured when training starts.");
        ImGui::TextWrapped("Search DPS is an empirical reference, not a certified optimum. Depth 1 uses action lookahead; depth 2+ builds an open-loop UCT MCTS tree of future action preferences, then rolls out with the APL/learner. More rollouts reduce sampling noise and grow the tree; more rounds improve the continuation policy. Multi-target and off-GCD automation use the captured settings.");
        ImGui::Separator();
        ImGui::BeginDisabled(busy());
        auto integer = [](const char* label, int& value, int minimum, int maximum) {
            ImGui::SetNextItemWidth(160);
            ImGui::InputInt(label, &value);
            value = std::clamp(value, minimum, maximum);
        };
        integer("DAgger rounds", imitation_config.rounds, 1, 100);
        integer("Training threads (0 = auto)", imitation_config.threads, 0,
            static_cast<int>(std::min(256u, std::max(1u, std::thread::hardware_concurrency()))));
        ImGui::TextWrapped("One shared thread budget covers fights, root-action search, evaluation and per-action tree fitting. Seeds and dataset ordering are independent of thread count.");
        integer("Training fights / round", imitation_config.episodes, 2, 10000);
        integer("Label every N decisions", imitation_config.sample_stride, 1, 1000);
        integer("Label cap / fight", imitation_config.samples_per_episode, 1, 10000);
        integer("Future rollouts / action", imitation_config.rollouts, 2, 4096);
        integer("Search depth in decisions", imitation_config.search_depth, 1, 32);
        ImGui::SetNextItemWidth(160);
        ImGui::InputDouble("UCT exploration (DPS / 1000)", &imitation_config.exploration, 0.05, 0.25);
        imitation_config.exploration = std::clamp(imitation_config.exploration, 0.0, 10.0);
        integer("Reference rollouts / action", imitation_config.reference_rollouts, 2, 4096);
        integer("Held-out evaluation fights", imitation_config.evaluation_episodes, 2, 10000);
        ImGui::SliderFloat("Teacher mix on labeled training states", &teacher_mix_ui, 0.0f, 1.0f);
        ImGui::TextWrapped("First round follows search at labeled states. Later rounds mix teacher and learner; set mix to 0 for learner-only DAgger collection. Evaluation searches every decision, so it can be expensive. Increase the label cap to cover late-fight states.");
        if (ImGui::CollapsingHeader("GBDT parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
            integer("Boosting iterations", imitation_config.trees, 1, 2000);
            integer("Maximum tree depth", imitation_config.max_depth, 1, 15);
            ImGui::SetNextItemWidth(160);
            ImGui::InputDouble("Minimum leaf weight", &imitation_config.min_child_weight, 1, 5);
            imitation_config.min_child_weight = std::clamp(imitation_config.min_child_weight, 0.0, 10000.0);
            ImGui::SetNextItemWidth(160);
            ImGui::InputDouble("L2 regularization", &imitation_config.l2_reg, 0.1, 1);
            imitation_config.l2_reg = std::clamp(imitation_config.l2_reg, 0.0, 10000.0);
            ImGui::SliderFloat("Learning rate", &learning_rate_ui, 0.005f, 0.5f, "%.3f", ImGuiSliderFlags_Logarithmic);
        }
        integer("Seed", seed, 0, INT_MAX);
        ImGui::SetNextItemWidth(420);
        ImGui::InputText("Save bundle directory", imitation_bundle, sizeof(imitation_bundle));
        if (ImGui::Button("Train search imitation policy")) {
            cancel = false;
            imitation_config.seed = static_cast<unsigned>(seed);
            imitation_config.teacher_mix = teacher_mix_ui;
            imitation_config.learning_rate = learning_rate_ui;
            const auto config = imitation_config;
            const auto snapshot = simulator;
            { std::lock_guard lock(mutex); imitation_progress = {}; }
            status = "Training search imitation...";
            worker = std::async(std::launch::async, [this, snapshot, config] {
                imitation_trainer = std::make_unique<ImitationTrainer>(snapshot);
                imitation_trainer->train(config, cancel, [this](const ImitationProgress& value) {
                    std::lock_guard lock(mutex);
                    imitation_progress = value;
                });
            });
        }
        ImGui::EndDisabled();
        if (busy()) {
            if (ImGui::Button("Stop training / evaluation")) cancel = true;
            ImGui::TextWrapped("Cancellation is checked at decision/rollout and boosting-iteration boundaries. Active workers finish their current step before the job stops.");
        } else {
            if (status == "Training search imitation...") {
                std::lock_guard lock(mutex);
                status = imitation_progress.phase;
            }
            ImGui::TextWrapped("%s", status.c_str());
            ImGui::BeginDisabled(!imitation_trainer || !imitation_trainer->has_policy());
            if (ImGui::Button("Use trained policy in current configuration")) {
                try {
                    activate_imitation_policy(simulator, imitation_trainer->policy_snapshot(), "Search imitation GBDT");
                    status = "Trained GBDT is active. Run a normal simulation to inspect its cast sequence and decision trace.";
                } catch (const std::exception& error) { status = error.what(); }
            }
            if (ImGui::Button("Save GBDT bundle")) {
                try { imitation_trainer->save_policy(imitation_bundle); status = std::string("Saved bundle: ") + imitation_bundle; }
                catch (const std::exception& error) { status = error.what(); }
            }
            ImGui::EndDisabled();
            if (ImGui::Button("Load bundle / policy and activate")) {
                try {
                    load_imitation_policy(simulator, imitation_bundle);
                    status = "Loaded GBDT is active in the current configuration.";
                } catch (const std::exception& error) { status = error.what(); }
            }
        }
        if (simulator.policy.use_imitation_policy)
            ImGui::TextWrapped("Active decision policy: %s", simulator.policy.imitation_policy_name.c_str());
        ImitationProgress view;
        { std::lock_guard lock(mutex); view = imitation_progress; }
        ImGui::TextWrapped("%s", view.phase.c_str());
        ImGui::Text("Round %d / %d | %d fights | %d labels | %d search replays", view.round,
            imitation_config.rounds, view.episodes_done, view.samples, view.search_replays);
        ImGui::Text("%u compute threads | %.1fs total | %.1fs in phase | %.2f fights/s | %.0f search replays/s",
            view.threads, view.elapsed_seconds, view.phase_seconds, view.fights_per_second, view.search_replays_per_second);
        ImGui::Text("Phase totals: collection %.1fs | fitting %.1fs | evaluation %.1fs",
            view.collection_seconds, view.fit_seconds, view.evaluation_seconds);
        if (!view.history.empty()) {
            std::vector<double> rounds, policies, teachers, baselines;
            for (const auto& entry : view.history) {
                rounds.push_back(entry.round); policies.push_back(entry.policy_dps);
                teachers.push_back(entry.teacher_dps); baselines.push_back(entry.apl_dps);
            }
            if (ImPlot::BeginPlot("Held-out DPS by training round", ImVec2(-1, 220))) {
                ImPlot::SetupAxes("Round", "Expected DPS estimate");
                ImPlot::PlotLine("C++ GBDT", rounds.data(), policies.data(), static_cast<int>(rounds.size()));
                ImPlot::PlotLine("Online search reference", rounds.data(), teachers.data(), static_cast<int>(rounds.size()));
                ImPlot::PlotLine("Captured APL", rounds.data(), baselines.data(), static_cast<int>(rounds.size()));
                ImPlot::EndPlot();
            }
            const auto& last = view.history.back();
            ImGui::Text("Policy %.1f DPS | APL %.1f DPS | Search %.1f DPS", last.policy_dps, last.apl_dps, last.teacher_dps);
            if (last.teacher_dps > 0) ImGui::Text("Policy / search: %.2f%%", 100 * last.policy_dps / last.teacher_dps);
            ImGui::Text("Policy minus search: %+.1f DPS | approximate paired 95%% CI [%+.1f, %+.1f]",
                last.policy_minus_teacher, last.policy_minus_teacher - last.gap_ci95, last.policy_minus_teacher + last.gap_ci95);
            ImGui::Text("Teacher-state agreement %.1f%% | mean action regret %.2f DPS | episode validation accuracy %.1f%%",
                last.agreement_pct, last.regret_dps, last.validation_accuracy_pct);
            ImGui::TextWrapped("Regret compares candidate full-fight rollout values at teacher-visited states. All three controllers use the same held-out fight seeds; search uses the current learner beyond its tree horizon. Its reference changes as the learner improves. A ratio near 100%% measures imitation of this search, not proof of global optimality. Recheck with larger reference budgets, deeper trees and fresh seeds.");
        }
        ImGui::TextWrapped("Saving creates a new bundle with the C++ policy, training data, metrics, captured build and deployment configuration. Activate the trained model here, or load a bundle / policy.gbdt. The Policy tab switches between APL and trained GBDT; normal simulation results show its actions.");
    }
    float teacher_mix_ui = 0.5f;
    float learning_rate_ui = 0.05f;
};
}
