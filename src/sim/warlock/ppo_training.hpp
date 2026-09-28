#pragma once
#include "warlock_sim.hpp"
#include <atomic>
#include <memory>
#include <functional>

namespace warlock {
struct PPOTrainingConfig {
    int updates = 100;
    unsigned seed = 1337;
};
struct PPOTrainingProgress {
    int updates = 0;
    int transitions = 0;
    double mean_reward = 0;
    double elapsed_seconds = 0;
};
struct PPOEvaluation {
    double policy_dps = 0;
    double baseline_dps = 0;
};
class PPOTrainer {
public:
    explicit PPOTrainer(const WarlockSimulator& simulator, unsigned seed);
    ~PPOTrainer();
    PPOTrainer(const PPOTrainer&) = delete;
    PPOTrainer& operator=(const PPOTrainer&) = delete;
    void train(int updates, const std::atomic<bool>& cancel,
               const std::function<void(PPOTrainingProgress)>& progress);
    PPOEvaluation evaluate(int episodes, unsigned seed);
    // Independent actor snapshot with fresh recurrent memory. Create one for
    // each fight; a controller must not be shared between concurrent fights.
    NeuralDecision make_policy();
    void save_policy(const std::string& path);
    void load_policy(const std::string& path);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
