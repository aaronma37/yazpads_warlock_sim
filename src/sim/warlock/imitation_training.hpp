#pragma once
#include "warlock_sim.hpp"
#include <atomic>
#include <filesystem>

namespace warlock {
struct ImitationConfig {
    int threads = 0; // 0 selects available hardware threads.
    int rounds = 3;
    int episodes = 8;
    int sample_stride = 4;
    int samples_per_episode = 32;
    int rollouts = 16;
    int search_depth = 1;
    double exploration = 0.25;
    int reference_rollouts = 32;
    int evaluation_episodes = 8;
    int trees = 100;
    int max_depth = 6;
    double min_child_weight = 5;
    double l2_reg = 1;
    double learning_rate = 0.05;
    double teacher_mix = 0.5;
    unsigned seed = 1337;
};
struct SearchActionValue {
    PriorityAction action;
    double dps = 0;
    double stderr_dps = 0;
    int trials = 0;
};
struct SearchResult {
    std::vector<SearchActionValue> values;
    int replay_count = 0;
};
// Replays the identical history/seed, then resamples only the future RNG.
// With an executor, continuation must support concurrent calls (e.g. immutable inference).
SearchResult search_imitation_actions(const WarlockSimulator& simulator, uint64_t episode_seed,
    const std::vector<WarlockSimulator::DecisionSelection>& prefix,
    const sim::SimObservation& state, int rollouts, uint64_t future_seed,
    const WarlockSimulator::DecisionController& continuation, const std::atomic<bool>& cancel,
    int search_depth = 1, double exploration = 0.25, sim::ParallelExecutor* executor = nullptr);

class SearchImitationPolicy {
public:
    struct Node { int feature, left, right; double threshold, value; };
    struct Tree { size_t class_index; std::vector<Node> nodes; };
    std::vector<PriorityAction> classes;
    std::vector<Tree> trees;
    void fit(const std::vector<sim::GBDTMultiActionQPolicy::QSample>& samples, const sim::GBDTConfig& config,
        sim::ParallelExecutor* executor = nullptr, const std::atomic<bool>* cancel = nullptr);
    void save(const std::string& path) const;
    void load(const std::string& path);
    std::string serialize() const;
    void deserialize(const std::string& text);
    std::vector<std::pair<PriorityAction, double>> ranked_values(const sim::SimObservation& state) const;
    WarlockSimulator::DecisionSelection select(const sim::SimObservation& state) const;
};
// Copies are immutable and can be shared by all batch simulation workers.
void activate_imitation_policy(WarlockSimulator&, std::shared_ptr<const SearchImitationPolicy>, const std::string& name);
void activate_imitation_policy(PolicyConfig&, std::shared_ptr<const SearchImitationPolicy>, const std::string& name);
void load_imitation_policy(WarlockSimulator&, const std::string& path);
struct ImitationRound {
    int round = 0;
    int samples = 0;
    int search_replays = 0;
    double policy_dps = 0, apl_dps = 0, teacher_dps = 0;
    double policy_minus_teacher = 0, gap_ci95 = 0;
    double agreement_pct = 0, regret_dps = 0;
    double validation_accuracy_pct = 0;
    double collection_seconds = 0, fit_seconds = 0, evaluation_seconds = 0;
};
struct ImitationProgress {
    std::string phase;
    int round = 0, episodes_done = 0, samples = 0, search_replays = 0;
    unsigned threads = 1;
    double elapsed_seconds = 0, phase_seconds = 0;
    double fights_per_second = 0, search_replays_per_second = 0;
    double collection_seconds = 0, fit_seconds = 0, evaluation_seconds = 0;
    std::vector<ImitationRound> history;
};
class ImitationTrainer {
public:
    explicit ImitationTrainer(const WarlockSimulator& simulator);
    ~ImitationTrainer();
    void train(const ImitationConfig&, const std::atomic<bool>&,
        const std::function<void(const ImitationProgress&)>&);
    void save_policy(const std::string& path) const;
    bool has_policy() const { return !model.classes.empty(); }
    std::shared_ptr<const SearchImitationPolicy> policy_snapshot() const;
private:
    WarlockSimulator snapshot;
    SearchImitationPolicy model;
    std::filesystem::path directory;
};
}
