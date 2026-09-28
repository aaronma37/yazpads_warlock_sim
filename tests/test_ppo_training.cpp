#include "test_framework.hpp"
#include "src/sim/warlock/ppo_training.hpp"
#include <filesystem>
#include <fstream>
#include <map>
#include <chrono>

namespace {
struct Checkpoints {
    std::filesystem::path directory = std::filesystem::temp_directory_path() /
        ("warlock-ppo-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Checkpoints() { std::filesystem::create_directory(directory); }
    ~Checkpoints() { std::filesystem::remove_all(directory); }
    std::string path(const char* name) const { return (directory / name).string(); }
};
// Compare actual recurrent parameters, not optimizer moments or activations.
std::map<std::string, std::string> recurrent_weights(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    std::map<std::string, std::string> result;
    std::array<char, 512> header{};
    while (input.read(header.data(), header.size()) && header[0]) {
        const std::string name(header.begin(), std::find(header.begin(), header.begin() + 100, '\0'));
        const auto size = std::stoull(std::string(header.data() + 124, 11), nullptr, 8);
        std::string data(size, '\0');
        input.read(data.data(), static_cast<std::streamsize>(size));
        if (name.find("weights_hidden/parameters/data") != std::string::npos)
            result.emplace(name, data);
        input.ignore(static_cast<std::streamsize>((512 - size % 512) % 512));
    }
    return result;
}
}

TEST_CASE(PPOTraining, recurrent_update_checkpoint_evaluation_and_cancel) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 20;
    sim.randomize_duration = false;
    Checkpoints files;
    warlock::PPOTrainer trainer(sim, 77);
    trainer.save_policy(files.path("before.tar"));
    const auto before = recurrent_weights(files.path("before.tar"));
    CHECK(!before.empty());
    std::atomic<bool> cancel{false};
    int callbacks = 0;
    trainer.train(2, cancel, [&](warlock::PPOTrainingProgress progress) {
        ++callbacks;
        CHECK_EQ(progress.updates, callbacks);
        CHECK_EQ(progress.transitions, callbacks * 512);
        CHECK(std::isfinite(progress.mean_reward));
        CHECK(progress.mean_reward > 0);
    });
    CHECK_EQ(callbacks, 2);
    trainer.save_policy(files.path("after.tar"));
    const auto after = recurrent_weights(files.path("after.tar"));
    CHECK_EQ(before.size(), after.size());
    CHECK(before != after);
    // A recurrent actor must retain memory even when observations are identical.
    // A newly created controller must reproduce the first decision exactly.
    warlock::NeuralObservation observation{};
    observation.fill(0.25f);
    auto controller = trainer.make_policy();
    const auto first_action = controller(observation, 0, 0);
    const auto second_action = controller(observation, 0, 1);
    CHECK(first_action != second_action);
    CHECK(first_action == trainer.make_policy()(observation, 0, 0));
    auto result = trainer.evaluate(3, 12345);
    CHECK(std::isfinite(result.policy_dps));
    CHECK(result.policy_dps > 0);
    CHECK(result.baseline_dps > 0);
    auto repeated = trainer.evaluate(3, 12345);
    CHECK_EQ(result.policy_dps, repeated.policy_dps);
    warlock::PPOTrainer loaded(sim, 999);
    loaded.load_policy(files.path("after.tar"));
    auto roundtrip = loaded.evaluate(3, 12345);
    CHECK_EQ(result.policy_dps, roundtrip.policy_dps);
    CHECK_EQ(result.baseline_dps, roundtrip.baseline_dps);
    cancel = true;
    trainer.train(10, cancel, [&](auto) { CHECK(false); });
    trainer.save_policy(files.path("cancelled.tar"));
    CHECK(after == recurrent_weights(files.path("cancelled.tar")));
    { std::ofstream invalid(files.path("invalid.tar")); invalid << "invalid policy"; }
    bool rejected = false;
    try { loaded.load_policy(files.path("invalid.tar")); }
    catch (const std::exception&) { rejected = true; }
    CHECK(rejected);
    CHECK_EQ(loaded.evaluate(3, 12345).policy_dps, result.policy_dps);
}

TEST_CASE(PPOTraining, neural_controller_is_independent_of_oracle_and_apl) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 15;
    sim.policy.use_custom_apl = true;
    sim.policy.custom_rules.clear();
    int decisions = 0;
    sim.neural_decision = [&](const warlock::NeuralObservation& obs, double damage, double time) {
        ++decisions;
        CHECK(std::isfinite(damage)); CHECK(time >= 0);
        for (auto value : obs) CHECK(std::isfinite(value));
        warlock::NeuralAction action{};
        action.back() = 10; // Shadow Bolt preference, independent of APL.
        return action;
    };
    warlock::FastRNG rng(42);
    auto result = sim.run_single_simulation(rng);
    CHECK(decisions > 0);
    CHECK(result.shadow_bolt_casts > 0);
    sim.use_oracle_execution_policy = true;
    sim.policy.use_oracle_execution_policy = true;
    warlock::FastRNG same_seed(42);
    CHECK_EQ(result.total_damage, sim.run_single_simulation(same_seed).total_damage);
}
