#pragma once
#include "src/sim/warlock/ppo_training.hpp"
#include "src/ui/common/ui_theme.hpp"
#include <future>
#include <mutex>
#include <filesystem>

namespace warlock {
// Owned by the app: no detached worker can outlive the panel or its simulator.
struct PolicyTrainingPanel {
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
    void render(const WarlockSimulator& simulator) {
        poll();
        ImGui::BeginChild("TrainPoliciesPane", ImVec2(0, 0), true);
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
};
}
