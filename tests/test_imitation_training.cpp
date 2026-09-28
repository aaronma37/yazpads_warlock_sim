#include "test_framework.hpp"
#include "src/sim/warlock/imitation_training.hpp"
#include "src/sim/warlock/build_export.hpp"
#include "src/sim/warlock/parallel_runner.hpp"
#include "src/cli_config.hpp"
#include <fstream>
#include <chrono>
#include <set>

TEST_CASE(ImitationTraining, bounded_nested_executor_and_exception_drain) {
    sim::ParallelExecutor executor(4);
    std::atomic<int> active{0}, peak{0}, completed{0};
    std::mutex ids_mutex;
    std::set<std::thread::id> ids;
    executor.parallel_for(6, [&](size_t) {
        executor.parallel_for(12, [&](size_t) {
            const int running = ++active;
            int previous = peak.load();
            while (previous < running && !peak.compare_exchange_weak(previous, running)) {}
            { std::lock_guard lock(ids_mutex); ids.insert(std::this_thread::get_id()); }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            --active;
            ++completed;
        });
    });
    CHECK_EQ(completed.load(), 72);
    CHECK(peak.load() > 1);
    CHECK(peak.load() <= 4);
    CHECK(ids.size() > 1 && ids.size() <= 4);
    std::atomic<int> drained{0};
    bool rejected = false;
    try {
        executor.parallel_for(20, [&](size_t index) {
            if (index == 5) throw std::runtime_error("Test worker failure");
            ++drained;
        });
    } catch (const std::runtime_error&) { rejected = true; }
    CHECK(rejected);
    CHECK_EQ(drained.load(), 19);
    executor.parallel_for(3, [&](size_t) { ++drained; });
    CHECK_EQ(drained.load(), 22);
}

TEST_CASE(ImitationTraining, exact_state_future_resampling_and_execution) {
    warlock::WarlockSimulator simulator;
    simulator.fight_duration = 18;
    simulator.randomize_duration = true;
    simulator.duration_variance = 2;
    std::vector<sim::SimObservation> observations;
    simulator.decision_controller = [&](const auto& state, size_t, auto&) {
        observations.push_back(state);
        return warlock::WarlockSimulator::DecisionSelection{};
    };
    warlock::FastRNG rng(971);
    auto baseline = simulator.run_single_simulation(rng);
    CHECK(observations.size() > 2);
    simulator.decision_controller = {};
    std::vector<warlock::WarlockSimulator::DecisionSelection> prefix(2);
    std::atomic<bool> cancel{false};
    auto first = warlock::search_imitation_actions(simulator, 971, prefix, observations[2], 3, 9871, {}, cancel);
    auto second = warlock::search_imitation_actions(simulator, 971, prefix, observations[2], 3, 9871, {}, cancel);
    CHECK(!first.values.empty());
    CHECK_EQ(first.values.size(), second.values.size());
    CHECK(first.replay_count > 0);
    for (size_t i = 0; i < first.values.size(); ++i) {
        CHECK(first.values[i].action == second.values[i].action);
        CHECK_NEAR(first.values[i].dps, second.values[i].dps, 1e-12);
        CHECK(std::isfinite(first.values[i].stderr_dps));
        CHECK_EQ(first.values[i].trials, 3);
    }
    auto wrong_state = observations[2];
    wrong_state.player_mana_pct += 0.01f;
    bool rejected = false;
    try { warlock::search_imitation_actions(simulator, 971, prefix, wrong_state, 2, 9871, {}, cancel); }
    catch (const std::runtime_error&) { rejected = true; }
    CHECK(rejected);
    CHECK(baseline.dps > 0);
}

TEST_CASE(ImitationTraining, portable_multiclass_inference_and_schema_validation) {
    auto path = std::filesystem::temp_directory_path() / ("warlock-lgbm-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    {
        std::ofstream file(path);
        file << "WARLOCK_GBDT_V1 24 2 2\n";
        for (auto& feature : sim::SimObservation::feature_names()) file << feature << ' ';
        file << '\n' << static_cast<int>(warlock::PriorityAction::LIFE_TAP) << ' '
             << static_cast<int>(warlock::PriorityAction::SHADOW_BOLT_FILLER) << "\n";
        file << "0 3\n0 1 2 0.3 0\n-1 -1 -1 0 5\n-1 -1 -1 0 -5\n";
        file << "1 1\n-1 -1 -1 0 1\n";
    }
    warlock::SearchImitationPolicy model;
    model.load(path.string());
    sim::SimObservation low, high;
    low.player_mana_pct = 0.2;
    high.player_mana_pct = 0.9;
    CHECK(model.select(low).rules.front().action == warlock::PriorityAction::LIFE_TAP);
    CHECK(model.select(high).rules.front().action == warlock::PriorityAction::SHADOW_BOLT_FILLER);
    CHECK(model.select(high).force_all);
    {
        std::ofstream file(path);
        file << "WARLOCK_GBDT_V1 23 2 2\n";
    }
    bool rejected = false;
    try { model.load(path.string()); } catch (const std::runtime_error&) { rejected = true; }
    CHECK(rejected);
    // Failed loads preserve the previous valid policy.
    CHECK(model.select(low).rules.front().action == warlock::PriorityAction::LIFE_TAP);
    std::filesystem::remove(path);
}

TEST_CASE(ImitationTraining, native_training_dagger_evaluation_and_bundle_roundtrip) {
    warlock::WarlockSimulator simulator;
    simulator.fight_duration = 12;
    warlock::ImitationConfig config;
    config.threads = 4;
    config.rounds = 2;
    config.episodes = 3;
    config.rollouts = 2;
    config.reference_rollouts = 2;
    config.search_depth = 3;
    config.evaluation_episodes = 2;
    config.sample_stride = 1;
    config.samples_per_episode = 2;
    config.trees = 3;
    config.max_depth = 2;
    config.min_child_weight = 0;
    std::atomic<bool> cancel{false};
    warlock::ImitationTrainer trainer(simulator);
    warlock::ImitationProgress latest;
    trainer.train(config, cancel, [&](const auto& progress) { latest = progress; });
    CHECK(trainer.has_policy());
    CHECK_EQ(latest.history.size(), size_t(2));
    CHECK(latest.history.back().samples > latest.history.front().samples);
    CHECK(latest.history.back().policy_dps > 0);
    CHECK(latest.history.back().teacher_dps > 0);
    CHECK(std::isfinite(latest.history.back().gap_ci95));
    auto bundle = std::filesystem::temp_directory_path() / ("warlock-native-bundle-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    trainer.save_policy(bundle.string());
    CHECK(std::filesystem::exists(bundle / "build.json"));
    CHECK(std::filesystem::exists(bundle / "metrics.csv"));
    CHECK(std::filesystem::exists(bundle / "deployment.json"));
    warlock::SearchImitationPolicy loaded;
    loaded.load((bundle / "policy.gbdt").string());
    simulator.decision_controller = [&](const auto& state, size_t, auto&) { return loaded.select(state); };
    warlock::FastRNG rng(1412);
    CHECK(simulator.run_single_simulation(rng).dps > 0);
    bool overwrite_rejected = false;
    try { trainer.save_policy(bundle.string()); } catch (const std::runtime_error&) { overwrite_rejected = true; }
    CHECK(overwrite_rejected);
    std::filesystem::remove_all(bundle);
}

TEST_CASE(ImitationTraining, thread_count_preserves_dagger_models_data_and_metrics) {
    warlock::WarlockSimulator simulator;
    simulator.fight_duration = 16;
    simulator.randomize_duration = true;
    simulator.duration_variance = 2;
    warlock::ImitationConfig config;
    config.rounds = 2;
    config.episodes = 3;
    config.rollouts = 6;
    config.reference_rollouts = 6;
    config.search_depth = 3;
    config.evaluation_episodes = 3;
    config.sample_stride = 1;
    config.samples_per_episode = 3;
    config.trees = 5;
    config.max_depth = 2;
    config.min_child_weight = 0;
    std::atomic<bool> cancel{false};
    warlock::ImitationTrainer serial(simulator), parallel(simulator);
    warlock::ImitationProgress serial_progress, parallel_progress;
    config.threads = 1;
    serial.train(config, cancel, [&](const auto& progress) { serial_progress = progress; });
    config.threads = 4;
    parallel.train(config, cancel, [&](const auto& progress) { parallel_progress = progress; });
    CHECK_EQ(serial_progress.threads, 1u);
    CHECK_EQ(parallel_progress.threads, 4u);
    CHECK(serial.policy_snapshot()->serialize() == parallel.policy_snapshot()->serialize());
    CHECK_EQ(serial_progress.history.size(), parallel_progress.history.size());
    for (size_t i = 0; i < serial_progress.history.size(); ++i) {
        const auto& a = serial_progress.history[i];
        const auto& b = parallel_progress.history[i];
        CHECK_EQ(a.samples, b.samples);
        CHECK_EQ(a.search_replays, b.search_replays);
        CHECK_EQ(a.policy_dps, b.policy_dps);
        CHECK_EQ(a.apl_dps, b.apl_dps);
        CHECK_EQ(a.teacher_dps, b.teacher_dps);
        CHECK_EQ(a.gap_ci95, b.gap_ci95);
        CHECK_EQ(a.agreement_pct, b.agreement_pct);
        CHECK_EQ(a.regret_dps, b.regret_dps);
        CHECK_EQ(a.validation_accuracy_pct, b.validation_accuracy_pct);
    }
    auto root = std::filesystem::temp_directory_path() / ("warlock-thread-parity-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root);
    serial.save_policy((root / "serial").string());
    parallel.save_policy((root / "parallel").string());
    auto read = [](const std::filesystem::path& path) {
        std::ifstream input(path);
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    CHECK(read(root / "serial/samples.csv") == read(root / "parallel/samples.csv"));
    std::filesystem::remove_all(root);
    CHECK(parallel_progress.collection_seconds > 0);
    CHECK(parallel_progress.fit_seconds > 0);
    CHECK(parallel_progress.evaluation_seconds > 0);
}

TEST_CASE(ImitationTraining, cancellation_joins_workers_and_preserves_fitted_policy) {
    warlock::WarlockSimulator simulator;
    simulator.fight_duration = 12;
    warlock::ImitationConfig config;
    config.threads = 4;
    config.rounds = 2;
    config.episodes = 3;
    config.rollouts = config.reference_rollouts = 2;
    config.search_depth = 3;
    config.evaluation_episodes = 2;
    config.samples_per_episode = 2;
    config.sample_stride = 1;
    config.trees = 3;
    config.min_child_weight = 0;
    std::atomic<bool> cancel{false};
    warlock::ImitationTrainer trainer(simulator);
    warlock::ImitationProgress last;
    trainer.train(config, cancel, [&](const auto& progress) {
        last = progress;
        if (progress.phase.find("Evaluating") == 0) cancel = true;
    });
    CHECK(trainer.has_policy());
    CHECK(last.phase.find("Stopped") == 0);
    CHECK(last.history.empty());
    CHECK(!trainer.policy_snapshot()->serialize().empty());
    // Already-cancelled jobs also drain promptly without creating a model.
    warlock::ImitationTrainer cancelled(simulator);
    cancelled.train(config, cancel, [&](const auto& progress) { last = progress; });
    CHECK(!cancelled.has_policy());
    CHECK(last.phase.find("Stopped") == 0);
}

TEST_CASE(ImitationTraining, configuration_deployment_trace_and_parallel_execution) {
    auto model = std::make_shared<warlock::SearchImitationPolicy>();
    model->classes = {warlock::PriorityAction::LIFE_TAP, warlock::PriorityAction::SHADOW_BOLT_FILLER};
    model->trees = {{0, {{-1, -1, -1, 0, -5}}}, {1, {{-1, -1, -1, 0, 5}}}};
    warlock::WarlockSimulator simulator;
    simulator.fight_duration = 12;
    simulator.record_timeline = true;
    auto captured_apl = simulator.policy.get_priority_rules(simulator.talents, simulator.race);
    warlock::activate_imitation_policy(simulator, model, "Test GBDT");
    warlock::FastRNG rng(42);
    auto deployed = simulator.run_single_simulation(rng);
    CHECK(!deployed.policy_decisions.empty());
    CHECK(!deployed.cast_sequence.empty());
    CHECK(deployed.decision_policy_name == "Test GBDT");
    CHECK(deployed.action_history.front() == warlock::PriorityAction::SHADOW_BOLT_FILLER);
    CHECK_EQ(deployed.policy_decisions.size(), deployed.action_history.size());
    for (size_t i = 0; i < deployed.policy_decisions.size(); ++i) {
        const auto& decision = deployed.policy_decisions[i];
        CHECK(decision.executed_action == deployed.action_history[i]);
        CHECK(decision.ranked_values == model->ranked_values(decision.state));
    }
    // A simulator copy owns its model; it does not capture a panel or trainer.
    auto copied = simulator;
    simulator.policy.imitation_policy.reset();
    auto batch = warlock::ParallelSimRunner::run_batch(copied, 6, 2, nullptr, 42);
    CHECK(batch.mean_dps > 0);
    CHECK(!batch.sample_timeline.policy_decisions.empty());
    CHECK(batch.sample_timeline.decision_policy_name == "Test GBDT");
    copied.policy.use_imitation_policy = false;
    warlock::FastRNG baseline_rng(42);
    auto baseline = copied.run_single_simulation(baseline_rng);
    CHECK(baseline.policy_decisions.empty());
    CHECK(copied.policy.imitation_policy != nullptr);
    CHECK_EQ(captured_apl.size(), copied.policy.get_priority_rules(copied.talents, copied.race).size());
}

TEST_CASE(ImitationTraining, embedded_configuration_and_relative_policy_path_roundtrip) {
    auto model = std::make_shared<warlock::SearchImitationPolicy>();
    model->classes = {warlock::PriorityAction::LIFE_TAP, warlock::PriorityAction::SHADOW_BOLT_FILLER};
    model->trees = {{0, {{-1, -1, -1, 0, -5}}}, {1, {{-1, -1, -1, 0, 5}}}};
    warlock::WarlockSimulator simulator;
    simulator.fight_duration = 12;
    warlock::activate_imitation_policy(simulator, model, "Quoted \"GBDT\" model");
    const auto json = warlock::build_export::export_build_json(simulator);
    warlock::WarlockSimulator restored;
    int iterations = 2, threads = 1;
    uint64_t seed = 42;
    warlock::cli_config::apply_config(warlock::cli_config::Parser(json).parse(), restored, iterations, threads, seed);
    CHECK(restored.policy.use_imitation_policy);
    CHECK(restored.policy.imitation_policy_name == simulator.policy.imitation_policy_name);
    CHECK(restored.policy.imitation_policy->serialize() == model->serialize());
    warlock::FastRNG rng_a(42), rng_b(42);
    CHECK_NEAR(simulator.run_single_simulation(rng_a).dps, restored.run_single_simulation(rng_b).dps, 1e-9);
    // Deployment configurations also restore gear and encounter/automation settings.
    simulator.use_raw_stats = false;
    simulator.gear = warlock::GearLoadout::create_phase6_bis();
    simulator.gear.extra_spell_power = 30;
    simulator.race = warlock::Race::ORC;
    simulator.policy.racial_policy = warlock::RacialPolicy::ON_COOLDOWN;
    simulator.policy.corruption = warlock::DotPolicy::NEVER;
    simulator.mechanics.pet_scaling = false;
    simulator.target_config.creature_type = sim::CreatureType::BEAST;
    simulator.target_config.is_beast = true;
    warlock::cli_config::apply_config(warlock::cli_config::Parser(warlock::build_export::export_build_json(simulator)).parse(),
        restored, iterations, threads, seed);
    CHECK(!restored.use_raw_stats);
    CHECK_NEAR(restored.gear.extra_spell_power, 30, 1e-12);
    CHECK(restored.policy.racial_policy == simulator.policy.racial_policy);
    CHECK(restored.target_config.creature_type == simulator.target_config.creature_type);
    warlock::FastRNG gear_rng_a(42), gear_rng_b(42);
    CHECK_NEAR(simulator.run_single_simulation(gear_rng_a).dps, restored.run_single_simulation(gear_rng_b).dps, 1e-9);
    simulator.policy.use_imitation_policy = false;
    warlock::cli_config::apply_config(warlock::cli_config::Parser(warlock::build_export::export_build_json(simulator)).parse(),
        restored, iterations, threads, seed);
    CHECK(!restored.policy.use_imitation_policy);
    CHECK(restored.policy.imitation_policy != nullptr);

    auto directory = std::filesystem::temp_directory_path() / ("warlock-deployment-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    model->save((directory / "policy.gbdt").string());
    {
        std::ofstream file(directory / "config.json");
        file << "{\"policy\":{\"trained_gbdt\":{\"path\":\"policy.gbdt\",\"enabled\":true}}}";
    }
    warlock::cli_config::load_file((directory / "config.json").string(), restored, iterations, threads, seed);
    CHECK(restored.policy.use_imitation_policy);
    CHECK(restored.policy.imitation_policy->serialize() == model->serialize());
    auto old_model = restored.policy.imitation_policy;
    bool rejected = false;
    try { warlock::load_imitation_policy(restored, (directory / "missing.gbdt").string()); }
    catch (const std::runtime_error&) { rejected = true; }
    CHECK(rejected);
    CHECK(restored.policy.imitation_policy == old_model);
    std::filesystem::remove_all(directory);
}
