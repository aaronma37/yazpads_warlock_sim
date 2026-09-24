#include "test_framework.hpp"
#include "src/sim/common/sim_state_vector.hpp"
#include "src/sim/common/decision_tree.hpp"
#include "src/sim/warlock/warlock_sim.hpp"
#include "src/sim/warlock/viper_oracle.hpp"
#include "src/sim/warlock/apl_optimizer.hpp"
#include "src/sim/warlock/spec_presets.hpp"
#include <fstream>

using namespace warlock;

TEST_CASE(VIPERPipeline, SimObservationStructureAndArray) {
    sim::SimObservation obs;
    obs.player_mana_pct = 0.75f;
    obs.player_hp_pct = 0.90f;
    obs.dot_corruption_rem_sec = 12.5f;
    obs.isb_charges_rem = 3.0f;

    auto arr = obs.to_array();
    CHECK_EQ(arr.size(), sim::SimObservation::FEATURE_COUNT);
    CHECK_NEAR(arr[0], 0.75f, 0.001f);
    CHECK_NEAR(arr[1], 0.90f, 0.001f);
    CHECK_NEAR(arr[13], 12.5f, 0.001f); // dot_corruption_rem_sec index
    CHECK_NEAR(arr[19], 3.0f, 0.001f);  // isb_charges_rem index

    auto names = sim::SimObservation::feature_names();
    CHECK_EQ(names.size(), sim::SimObservation::FEATURE_COUNT);
    CHECK_EQ(names[0], std::string("player_mana_pct"));
    CHECK_EQ(names[13], std::string("dot_corruption_rem_sec"));
}

TEST_CASE(VIPERPipeline, StateCollectionAndCsvExport) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;
    sim.record_viper_samples = true;
    sim.viper_dataset.clear();

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(sim.viper_dataset.size() > 10);

    // Verify first recorded step
    const auto& first_step = sim.viper_dataset.samples[0];
    CHECK(first_step.state.player_mana_pct > 0.0f);
    CHECK(first_step.state.player_hp_pct > 0.0f);
    CHECK(first_step.state.time_remaining_sec <= 60.0f);
    CHECK(first_step.sample_weight > 0.0f);

    std::string csv = sim.viper_dataset.to_csv();
    CHECK(!csv.empty());
    CHECK(csv.find("player_mana_pct") != std::string::npos);
    CHECK(csv.find("oracle_action") != std::string::npos);
    CHECK(csv.find("sample_weight") != std::string::npos);

    // Save sample CSV for Python trainer verification
    std::ofstream out("build/test_viper_dataset.csv");
    out << csv;
    out.close();
}

TEST_CASE(VIPERPipeline, OracleLegalityChecks) {
    sim::SimObservation obs;
    Talents talents = Talents::create_forever_shadow_destro();

    // Life Tap is always legal regardless of HP (healers cover HP cost)
    obs.player_hp_pct = 0.05f;
    CHECK(VIPEROracle::is_action_legal(PriorityAction::LIFE_TAP, obs, talents));

    obs.player_hp_pct = 0.50f;
    CHECK(VIPEROracle::is_action_legal(PriorityAction::LIFE_TAP, obs, talents));

    // Conflagrate requires Immolate active
    obs.dot_immolate_rem_sec = 0.0f;
    obs.cd_conflagrate_sec = 0.0f;
    CHECK(!VIPEROracle::is_action_legal(PriorityAction::CONFLAGRATE, obs, talents));

    obs.dot_immolate_rem_sec = 10.0f;
    CHECK(VIPEROracle::is_action_legal(PriorityAction::CONFLAGRATE, obs, talents));

    // Shadowburn (ISB Active) requires ISB active on target
    obs.cd_shadowburn_sec = 0.0f;
    obs.isb_charges_rem = 0.0f;
    CHECK(VIPEROracle::is_action_legal(PriorityAction::SHADOWBURN, obs, talents));
    CHECK(!VIPEROracle::is_action_legal(PriorityAction::SHADOWBURN_ISB, obs, talents));

    obs.isb_charges_rem = 4.0f;
    CHECK(VIPEROracle::is_action_legal(PriorityAction::SHADOWBURN_ISB, obs, talents));
}

TEST_CASE(VIPERPipeline, PureCppDecisionTreeTrainingAndSplits) {
    sim::VIPERDataset synthetic_data;

    // Create a cleanly separable synthetic dataset:
    // When player_mana_pct <= 0.25 -> LIFE_TAP (action 0)
    // When player_mana_pct > 0.25 and dot_immolate_rem_sec <= 0.5 -> IMMOLATE (action 15)
    // When player_mana_pct > 0.25 and dot_immolate_rem_sec > 0.5 -> SHADOW_BOLT_FILLER (action 22)
    for (int i = 0; i < 100; ++i) {
        sim::VIPERStep s;
        s.state.player_mana_pct = 0.05f + static_cast<float>(i % 20) * 0.01f; // 0.05 .. 0.24
        s.state.dot_immolate_rem_sec = 5.0f;
        s.oracle_action = static_cast<uint8_t>(PriorityAction::LIFE_TAP);
        s.action_name = "Life Tap";
        s.sample_weight = 2.0f;
        synthetic_data.add_sample(s);
    }

    for (int i = 0; i < 100; ++i) {
        sim::VIPERStep s;
        s.state.player_mana_pct = 0.50f + static_cast<float>(i % 30) * 0.01f; // 0.50 .. 0.79
        s.state.dot_immolate_rem_sec = 0.0f;
        s.oracle_action = static_cast<uint8_t>(PriorityAction::IMMOLATE);
        s.action_name = "Immolate";
        s.sample_weight = 3.0f;
        synthetic_data.add_sample(s);
    }

    for (int i = 0; i < 100; ++i) {
        sim::VIPERStep s;
        s.state.player_mana_pct = 0.60f + static_cast<float>(i % 30) * 0.01f; // 0.60 .. 0.89
        s.state.dot_immolate_rem_sec = 8.0f;
        s.oracle_action = static_cast<uint8_t>(PriorityAction::SHADOW_BOLT_FILLER);
        s.action_name = "Shadow Bolt Filler";
        s.sample_weight = 1.0f;
        synthetic_data.add_sample(s);
    }

    sim::DecisionTreeConfig cfg;
    cfg.max_depth = 4;
    cfg.min_samples_leaf = 5;
    cfg.min_samples_split = 10;
    sim::DecisionTreeClassifier tree(cfg);
    tree.set_class_name(static_cast<uint8_t>(PriorityAction::LIFE_TAP), "Life Tap");
    tree.set_class_name(static_cast<uint8_t>(PriorityAction::IMMOLATE), "Immolate");
    tree.set_class_name(static_cast<uint8_t>(PriorityAction::SHADOW_BOLT_FILLER), "Shadow Bolt Filler");

    tree.fit(synthetic_data, 24);

    CHECK(tree.get_depth() >= 2);
    CHECK(tree.get_leaf_count() >= 3);
    CHECK_NEAR(tree.score_unweighted(synthetic_data), 1.0, 0.001);
    CHECK_NEAR(tree.score_weighted(synthetic_data), 1.0, 0.001);

    // Predict test points
    sim::SimObservation low_mana;
    low_mana.player_mana_pct = 0.12f;
    low_mana.dot_immolate_rem_sec = 10.0f;
    CHECK_EQ(tree.predict(low_mana), static_cast<uint8_t>(PriorityAction::LIFE_TAP));

    sim::SimObservation need_immo;
    need_immo.player_mana_pct = 0.70f;
    need_immo.dot_immolate_rem_sec = 0.0f;
    CHECK_EQ(tree.predict(need_immo), static_cast<uint8_t>(PriorityAction::IMMOLATE));

    sim::SimObservation filler_state;
    filler_state.player_mana_pct = 0.75f;
    filler_state.dot_immolate_rem_sec = 9.0f;
    CHECK_EQ(tree.predict(filler_state), static_cast<uint8_t>(PriorityAction::SHADOW_BOLT_FILLER));

    // Verify C++ code generation
    std::string cpp_code = tree.to_cpp("evaluate_test_policy");
    CHECK(cpp_code.find("inline PriorityAction evaluate_test_policy") != std::string::npos);
    CHECK(cpp_code.find("player_mana_pct") != std::string::npos);
    CHECK(cpp_code.find("return PriorityAction::LIFE_TAP") != std::string::npos);

    // Verify ASCII text tree
    std::string ascii_tree = tree.to_text_tree();
    CHECK(!ascii_tree.empty());
    CHECK(ascii_tree.find("[Split]") != std::string::npos);
    CHECK(ascii_tree.find("[Leaf]") != std::string::npos);

    // Verify rule extraction
    auto extracted_rules = tree.extract_rules();
    CHECK(extracted_rules.size() >= 3);
    for (const auto& r : extracted_rules) {
        CHECK(!r.conditions.empty());
        CHECK(!r.action_name.empty());
        CHECK(r.confidence > 0.90);
    }
}

TEST_CASE(VIPERPipeline, QWeightedSampleLossDomination) {
    // If a conflicting state exists with different sample weights,
    // the higher weighted sample MUST dominate the tree's split decision
    sim::VIPERDataset conflicting_data;

    // 10 low-weight samples choosing Action A (weight = 0.1 each, total weight = 1.0)
    for (int i = 0; i < 10; ++i) {
        sim::VIPERStep s;
        s.state.player_mana_pct = 0.50f;
        s.oracle_action = static_cast<uint8_t>(PriorityAction::DRAIN_LIFE_FILLER);
        s.sample_weight = 0.1f;
        conflicting_data.add_sample(s);
    }

    // 2 high-weight samples choosing Action B (weight = 10.0 each, total weight = 20.0)
    for (int i = 0; i < 2; ++i) {
        sim::VIPERStep s;
        s.state.player_mana_pct = 0.50f;
        s.oracle_action = static_cast<uint8_t>(PriorityAction::SHADOW_BOLT_FILLER);
        s.sample_weight = 10.0f;
        conflicting_data.add_sample(s);
    }

    sim::DecisionTreeConfig cfg;
    cfg.max_depth = 1;
    cfg.min_samples_leaf = 1;
    cfg.min_samples_split = 2;
    sim::DecisionTreeClassifier tree(cfg);
    tree.fit(conflicting_data, 24);

    sim::SimObservation test_obs;
    test_obs.player_mana_pct = 0.50f;
    // High-weighted action must be chosen
    CHECK_EQ(tree.predict(test_obs), static_cast<uint8_t>(PriorityAction::SHADOW_BOLT_FILLER));
}

TEST_CASE(VIPERPipeline, ExpectedValueExtractionAndRuleDiff) {
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    int progress_calls = 0;
    auto res = VIPEROracle::extract_viper_apl(sim, 10, 50, [&](float p, const std::string& status) {
        progress_calls++;
        CHECK(p >= 0.0f && p <= 1.0f);
        CHECK(!status.empty());
    });

    CHECK(progress_calls >= 4);
    CHECK(res.baseline_expected_dps > 0.0);
    CHECK(res.oracle_expected_dps >= res.baseline_expected_dps);
    CHECK(res.viper_expected_dps > 0.0);
    CHECK(res.extracted_rules.size() > 0);
    CHECK(res.rule_shifts.size() > 0);
    CHECK(res.action_stats.size() > 0);
    CHECK(res.total_samples_collected > 0);

    // CART Decision Tree verification
    CHECK(res.tree_leaf_count > 0);
    CHECK(res.tree_weighted_fidelity_pct > 0.0);
    CHECK(!res.generated_cpp_code.empty());
    CHECK(!res.tree_ascii_visualization.empty());
    CHECK(res.generated_cpp_code.find("evaluate_viper_policy") != std::string::npos);
}

TEST_CASE(VIPERPipeline, ParameterizedContinuousThresholdExecution) {
    // Verify that the sim engine correctly respects custom threshold parameters on PriorityRule
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    // Build custom APL with an aggressive Life Tap rule (tap whenever mana <= 80%)
    PriorityRule aggro_tap;
    aggro_tap.action = PriorityAction::LIFE_TAP;
    aggro_tap.use_custom_thresholds = true;
    aggro_tap.max_mana_pct = 0.80f; // 80% mana
    aggro_tap.min_hp_pct = 0.20f;
    aggro_tap.enabled = true;

    PriorityRule sb_filler;
    sb_filler.action = PriorityAction::SHADOW_BOLT_FILLER;
    sb_filler.enabled = true;

    sim.policy.custom_rules = { aggro_tap, sb_filler };
    sim.policy.use_custom_apl = true;

    FastRNG rng(42);
    SimResult res = sim.run_single_simulation(rng);

    // With an aggressive 80% mana threshold, simulator should trigger multiple life taps
    CHECK(res.life_taps > 2);
    CHECK(res.total_damage > 0.0);
}

TEST_CASE(VIPERPipeline, MultiInstanceRuleAPLExecution) {
    // Verify that the simulator supports multiple instances of the same action with distinct conditions (multi-length APL)
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    // Priority #1: Emergency Life Tap if mana <= 15%
    PriorityRule emergency_tap;
    emergency_tap.action = PriorityAction::LIFE_TAP;
    emergency_tap.name = "Emergency Tap";
    emergency_tap.use_custom_thresholds = true;
    emergency_tap.max_mana_pct = 0.15f;
    emergency_tap.enabled = true;

    // Priority #2: Immolate DoT upkeep
    PriorityRule immo;
    immo.action = PriorityAction::IMMOLATE;
    immo.enabled = true;

    // Priority #3: Maintenance Life Tap if mana <= 50% and boss has >= 30s remaining
    PriorityRule maint_tap;
    maint_tap.action = PriorityAction::LIFE_TAP;
    maint_tap.name = "Maintenance Tap";
    maint_tap.use_custom_thresholds = true;
    maint_tap.max_mana_pct = 0.50f;
    maint_tap.min_time_remaining = 30.0f;
    maint_tap.enabled = true;

    // Priority #4: Shadow Bolt filler
    PriorityRule sb;
    sb.action = PriorityAction::SHADOW_BOLT_FILLER;
    sb.enabled = true;

    sim.policy.custom_rules = { emergency_tap, immo, maint_tap, sb };
    sim.policy.use_custom_apl = true;

    FastRNG rng(42);
    SimResult res = sim.run_single_simulation(rng);

    CHECK(res.life_taps > 0);
    CHECK(res.total_damage > 0.0);
    CHECK(sim.policy.custom_rules.size() == 4);
}

TEST_CASE(VIPERPipeline, LiveOnlineMCTSControllerExecution) {
    // Verify that the Live Online Greedy MCTS / Oracle Controller executes actions live at every GCD
    FastRNG rng(777);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;
    sim.use_oracle_execution_policy = true;
    sim.record_timeline = true;
    sim.record_viper_samples = true;

    SimResult res = sim.run_single_simulation(rng);

    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 100.0);
    CHECK(res.cast_sequence.size() > 10);
    CHECK(sim.viper_dataset.size() > 5);

    // Verify that actions like Immolate / Conflagrate or fillers were cast by oracle decisions
    bool cast_damage_spells = false;
    for (const auto& log : res.cast_sequence) {
        if (log.spell_id == SpellID::IMMOLATE || log.spell_id == SpellID::SHADOW_BOLT || log.spell_id == SpellID::CONFLAGRATE) {
            cast_damage_spells = true;
            break;
        }
    }
    CHECK(cast_damage_spells);
}

TEST_CASE(VIPERPipeline, StandaloneLiveMCTSOracleBenchmark) {
    // Verify that benchmark_live_mcts_oracle computes statistical expectation of the greedy oracle policy
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    auto stats = VIPEROracle::benchmark_live_mcts_oracle(sim, 50, 42);

    CHECK(stats.mean_dps > 100.0);
    CHECK(stats.stddev_dps > 0.0);
    CHECK(stats.min_dps > 0.0);
    CHECK(stats.max_dps >= stats.mean_dps);
}

TEST_CASE(VIPERPipeline, MultiIterationDAggerDatasetAggregation) {
    // Verify iterative DAgger rollout aggregation: D <- D U D_k across multiple iterations
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    size_t dagger_iters = 3;
    auto res = VIPEROracle::extract_viper_apl(sim, 12, 40, dagger_iters, nullptr, 42);

    CHECK_EQ(res.dagger_iterations_run, dagger_iters);
    CHECK_EQ(res.dagger_history.size(), dagger_iters);

    // Validate that aggregated dataset grew monotonically across iterations: D_0 < D_1 < D_2
    for (size_t i = 0; i < res.dagger_history.size(); ++i) {
        const auto& log = res.dagger_history[i];
        CHECK_EQ(log.iteration, i + 1);
        CHECK(log.samples_added > 0);
        CHECK(log.candidate_dps > 0.0);
        CHECK(log.tree_weighted_fidelity_pct > 0.0);
        CHECK(log.tree_leaf_count > 0);

        if (i > 0) {
            CHECK(log.total_samples > res.dagger_history[i - 1].total_samples);
        }
    }

    CHECK(res.total_samples_collected == res.dagger_history.back().total_samples);
    CHECK(res.extracted_rules.size() > 0);
    CHECK(res.oracle_expected_dps >= res.baseline_expected_dps);
    CHECK(res.oracle_dps_stddev > 0.0);
    CHECK(res.viper_expected_dps > 0.0);
}

TEST_CASE(VIPERPipeline, SequentialDuplicateRuleCondensation) {
    // Verify that consecutive duplicate rules with identical conditions and shadowed rules are condensed
    PriorityRule r1;
    r1.action = PriorityAction::IMMOLATE;
    r1.enabled = true;
    r1.use_custom_thresholds = false;
    r1.trigger_condition = "(Always)";

    PriorityRule r2 = r1; // Duplicate #1
    PriorityRule r3 = r1; // Duplicate #2

    PriorityRule r4;
    r4.action = PriorityAction::LIFE_TAP;
    r4.enabled = true;
    r4.use_custom_thresholds = true;
    r4.max_mana_pct = 0.20f;
    r4.trigger_condition = "player_mana_pct <= 0.20";

    PriorityRule r5 = r4; // Duplicate Life Tap with same threshold

    PriorityRule r6;
    r6.action = PriorityAction::LIFE_TAP;
    r6.enabled = true;
    r6.use_custom_thresholds = true;
    r6.max_mana_pct = 0.50f;
    r6.trigger_condition = "player_mana_pct <= 0.50"; // Distinct threshold: should be preserved

    PriorityRule r7;
    r7.action = PriorityAction::SHADOW_BOLT_FILLER;
    r7.enabled = true;
    r7.use_custom_thresholds = false;
    r7.trigger_condition = "Always";

    std::vector<PriorityRule> input = { r1, r2, r3, r4, r5, r6, r7 };
    auto condensed = VIPEROracle::condense_and_deduplicate_rules(input);

    // Should condense 3 Immolates -> 1, 2 low-mana Life Taps -> 1, keep 50% Life Tap, and keep Shadow Bolt filler
    CHECK_EQ(condensed.size(), 4);
    CHECK(condensed[0].action == PriorityAction::IMMOLATE);
    CHECK(condensed[1].action == PriorityAction::LIFE_TAP);
    CHECK_NEAR(condensed[1].max_mana_pct, 0.20f, 0.001f);
    CHECK(condensed[2].action == PriorityAction::LIFE_TAP);
    CHECK_NEAR(condensed[2].max_mana_pct, 0.50f, 0.001f);
    CHECK(condensed[3].action == PriorityAction::SHADOW_BOLT_FILLER);
}

TEST_CASE(VIPERPipeline, APLOptimizerConstraintEnforcement) {
    // Verify structural constraints on Genetic APL Optimizer individuals
    APLOptimizer::Individual ind;
    ind.emergency_tap_mana = 0.05f; // below minimum (0.10)
    ind.maint_tap_mana = 0.85f;     // above maximum (0.50)
    ind.cod_time_cutoff = 90.0f;    // above maximum (75)
    ind.dot_refresh_window = 4.0f;  // above maximum (2.5)
    ind.exec_hp_threshold = 0.50f;  // above maximum (0.35)

    // Add duplicate spells and illegal spells
    PriorityRule immo; immo.action = PriorityAction::IMMOLATE; immo.enabled = true;
    PriorityRule immo2 = immo; // duplicate
    PriorityRule tap1; tap1.action = PriorityAction::LIFE_TAP; tap1.enabled = true;
    PriorityRule tap2 = tap1;
    PriorityRule tap3 = tap1; // 3rd tap should be discarded

    ind.tap_strategy = APLOptimizer::TapStrategy::DUAL_TAP;
    ind.rules = { immo, immo2, tap1, tap2, tap3 };

    Talents talents = Talents::create_forever_shadow_destro();
    APLOptimizer::enforce_constraints(ind, talents, Race::UNDEAD, true);

    // Bounded parameters verification
    CHECK_NEAR(ind.emergency_tap_mana, 0.10f, 0.001f);
    CHECK_NEAR(ind.maint_tap_mana, 0.50f, 0.001f);
    CHECK_NEAR(ind.cod_time_cutoff, 75.0f, 0.001f);
    CHECK_NEAR(ind.dot_refresh_window, 2.5f, 0.001f);
    CHECK_NEAR(ind.exec_hp_threshold, 0.35f, 0.001f);

    // Rule constraints verification: Emergency Life Tap, Immolate, Maintenance Life Tap, Fallback Filler
    CHECK_EQ(ind.rules.size(), 4);
    CHECK(ind.rules[0].action == PriorityAction::LIFE_TAP);
    CHECK_NEAR(ind.rules[0].max_mana_pct, 0.10f, 0.001f);
    CHECK(ind.rules[1].action == PriorityAction::IMMOLATE);
    CHECK(ind.rules[2].action == PriorityAction::LIFE_TAP);
    CHECK_NEAR(ind.rules[2].max_mana_pct, 0.50f, 0.001f);
    CHECK(ind.rules[3].action == PriorityAction::SHADOW_BOLT_FILLER);

    // Single Top Tap verification
    APLOptimizer::Individual ind_single;
    ind_single.tap_strategy = APLOptimizer::TapStrategy::SINGLE_TOP;
    ind_single.maint_tap_mana = 0.40f;
    ind_single.rules = { immo };
    APLOptimizer::enforce_constraints(ind_single, talents, Race::UNDEAD, true);
    CHECK_EQ(ind_single.rules.size(), 3);
    CHECK(ind_single.rules[0].action == PriorityAction::LIFE_TAP);
    CHECK_NEAR(ind_single.rules[0].max_mana_pct, 0.40f, 0.001f);
    CHECK(ind_single.rules[1].action == PriorityAction::IMMOLATE);
    CHECK(ind_single.rules[2].action == PriorityAction::SHADOW_BOLT_FILLER);
}

TEST_CASE(VIPERPipeline, GeneticAPLOptimizerRun) {
    // End-to-end optimization of an APL policy
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    APLOptimizerConfig cfg;
    cfg.population_size = 16;
    cfg.generations = 10;
    cfg.eval_simulations = 40;
    cfg.benchmark_simulations = 60;
    cfg.seed = 42;

    int progress_calls = 0;
    auto res = APLOptimizer::optimize_apl(sim, cfg, [&](float p, const std::string& status) {
        progress_calls++;
        CHECK(p >= 0.0f && p <= 1.0f);
        CHECK(!status.empty());
    });

    CHECK(progress_calls >= 4);
    CHECK(res.baseline_dps > 0.0);
    CHECK(res.optimized_dps >= res.baseline_dps - 0.5);
    CHECK(res.optimized_rules.size() >= 3);
    CHECK(res.evolution_history.size() == cfg.generations);
    CHECK(res.emergency_tap_mana <= 0.25f);
    CHECK(res.maint_tap_mana <= 0.50f);
}

TEST_CASE(VIPERPipeline, SeedFromCustomAPL) {
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    // Set custom APL on sim
    PriorityRule r1;
    r1.action = PriorityAction::IMMOLATE;
    r1.max_dot_rem_sec = 1.8f;
    PriorityRule r2;
    r2.action = PriorityAction::CONFLAGRATE;
    PriorityRule r3;
    r3.action = PriorityAction::LIFE_TAP;
    r3.max_mana_pct = 0.22f;
    PriorityRule r4;
    r4.action = PriorityAction::SHADOW_BOLT_FILLER;
    sim.policy.custom_rules = {r1, r2, r3, r4};
    sim.policy.use_custom_apl = true;

    APLOptimizerConfig cfg;
    cfg.population_size = 16;
    cfg.generations = 5;
    cfg.eval_simulations = 40;
    cfg.benchmark_simulations = 50;
    cfg.seed_from_current_apl = true;
    cfg.seed = 1234;

    auto res = APLOptimizer::optimize_apl(sim, cfg);
    CHECK_EQ(res.baseline_rules.size(), 4);
    CHECK(res.baseline_rules[0].action == PriorityAction::IMMOLATE);
    CHECK(res.optimized_dps >= res.baseline_dps - 1.0);
    CHECK(res.optimized_rules.size() >= 3);
}

TEST_CASE(VIPERPipeline, DiagnosticCompareSeedToBaseline) {
    std::vector<RotationChoice> rotations = {
        RotationChoice::SHADOW_DESTRO,
        RotationChoice::FIRE_DESTRO,
        RotationChoice::DP_AF_SHADOW,
        RotationChoice::DEEP_AFFLICTION,
        RotationChoice::SM_RUIN
    };

    for (auto rot : rotations) {
        WarlockSimulator sim;
        sim.policy.rotation = rot;
        sim.talents = Talents::create_forever_shadow_destro();
        if (rot == RotationChoice::FIRE_DESTRO) sim.talents = Talents::create_fire_destro();
        if (rot == RotationChoice::DP_AF_SHADOW) sim.talents = Talents::create_forever_demonic_pact();
        if (rot == RotationChoice::DEEP_AFFLICTION || rot == RotationChoice::SM_RUIN) sim.talents = Talents::create_sm_ruin();
        sim.fight_duration = 180.0;
        sim.randomize_duration = true;
        sim.duration_variance = 30.0;
        sim.use_raw_stats = true;

        APLOptimizerConfig cfg;
        cfg.population_size = 8;
        cfg.generations = 2;
        cfg.eval_simulations = 60;
        cfg.benchmark_simulations = 100;
        cfg.seed_from_current_apl = true;
        cfg.seed = 42;

        auto res = APLOptimizer::optimize_apl(sim, cfg);
        printf("[Diag %s] Baseline: %.1f DPS, Best Gen 1: %.1f DPS, Final Opt: %.1f DPS\n",
               rotation_choice_to_string(rot), res.baseline_dps, res.evolution_history[0].best_dps, res.optimized_dps);
        for (size_t c = 0; c < res.top_candidates.size(); ++c) {
            printf("  Cand #%zu: %-30s %.1f DPS\n", c + 1, res.top_candidates[c].name.c_str(), res.top_candidates[c].dps);
        }

        // Verification: When seeded with baseline, Champion must not drop below baseline
        CHECK(res.optimized_dps >= res.baseline_dps - 1.0);
    }
}

TEST_CASE(VIPERPipeline, DpAfFirePresetGeneticOptimization) {
    const SpecPreset* preset = find_spec_preset("dp_fire");
    CHECK(preset != nullptr);
    if (!preset) return;

    WarlockSimulator sim;
    apply_spec_preset(sim, *preset);
    sim.fight_duration = 180.0;
    sim.randomize_duration = true;
    sim.duration_variance = 30.0;
    sim.use_raw_stats = true;

    APLOptimizerConfig cfg;
    cfg.population_size = 16;
    cfg.generations = 4;
    cfg.eval_simulations = 60;
    cfg.benchmark_simulations = 150;
    cfg.seed_from_current_apl = true;
    cfg.allow_dual_life_tap = true;
    cfg.seed = 42;

    auto res = APLOptimizer::optimize_apl(sim, cfg);

    printf("\n=== [0/31/20 DP/AF Fire Genetic APL Optimization (App Defaults: 180s)] ===\n");
    printf("Baseline Expected DPS: %.1f ± %.1f\n", res.baseline_dps, res.baseline_stddev);
    printf("Evolved Champion DPS:  %.1f ± %.1f (%+.2f%%)\n", res.optimized_dps, res.optimized_stddev, res.dps_gain_pct);
    printf("Top Candidates Count:  %zu\n", res.top_candidates.size());

    for (const auto& cand : res.top_candidates) {
        printf("  Rank #%d: %-32s %.1f DPS (%+.2f%%)\n",
               cand.rank, cand.name.c_str(), cand.dps, cand.gain_pct);
    }

    CHECK(res.baseline_dps > 200.0);
    CHECK(res.optimized_dps >= res.baseline_dps - 0.5);
    CHECK(!res.top_candidates.empty());
    CHECK_EQ(res.top_candidates[0].rank, 1);
    CHECK(res.top_candidates[0].dps >= res.baseline_dps - 0.5);

    // Verify structural validity of the evolved rules
    CHECK(res.optimized_rules.size() >= 3);
    bool filler_found = false;
    for (size_t i = 0; i < res.optimized_rules.size(); ++i) {
        const auto& rule = res.optimized_rules[i];
        if (rule.action == PriorityAction::INCINERATE_FILLER ||
            rule.action == PriorityAction::SHADOW_BOLT_FILLER ||
            rule.action == PriorityAction::SEARING_PAIN_FILLER) {
            filler_found = true;
            CHECK_EQ(i, res.optimized_rules.size() - 1); // Filler must be at the very end
        }
    }
    CHECK(filler_found);
}

TEST_CASE(VIPERPipeline, DpAfShadowCorruption180sGeneticOptimization) {
    const SpecPreset* preset = find_spec_preset("dp_shadow_corr");
    CHECK(preset != nullptr);
    if (!preset) return;

    WarlockSimulator sim;
    apply_spec_preset(sim, *preset);
    sim.fight_duration = 180.0;
    sim.randomize_duration = true;
    sim.duration_variance = 30.0;
    sim.use_raw_stats = true;

    APLOptimizerConfig cfg;
    cfg.population_size = 16;
    cfg.generations = 4;
    cfg.eval_simulations = 60;
    cfg.benchmark_simulations = 150;
    cfg.seed_from_current_apl = true;
    cfg.allow_dual_life_tap = true;
    cfg.seed = 42;

    auto res = APLOptimizer::optimize_apl(sim, cfg);

    printf("\n=== [2/31/18 DP/AF Shadow Corruption Optimization (App Defaults: 180s)] ===\n");
    printf("Baseline Expected DPS: %.1f ± %.1f\n", res.baseline_dps, res.baseline_stddev);
    printf("Evolved Champion DPS:  %.1f ± %.1f (%+.2f%%)\n", res.optimized_dps, res.optimized_stddev, res.dps_gain_pct);
    printf("Top Candidates Count:  %zu\n", res.top_candidates.size());

    for (const auto& cand : res.top_candidates) {
        printf("  Rank #%d: %-32s %.1f DPS (%+.2f%%)\n",
               cand.rank, cand.name.c_str(), cand.dps, cand.gain_pct);
    }

    CHECK(res.baseline_dps > 300.0);
    CHECK(res.optimized_dps >= res.baseline_dps - 0.5);
    CHECK(!res.top_candidates.empty());
    CHECK_EQ(res.top_candidates[0].rank, 1);
}

TEST_CASE(VIPERPipeline, AllStandardSpecPresetsGeneticAPLCheck) {
    const auto& presets = standard_spec_presets();
    printf("\n=== Running Genetic APL Benchmark on All %zu Standard Spec Presets (App Defaults: 180s) ===\n", presets.size());

    for (const auto& preset : presets) {
        WarlockSimulator sim;
        apply_spec_preset(sim, preset);
        sim.fight_duration = 180.0;
        sim.randomize_duration = true;
        sim.duration_variance = 30.0;
        sim.use_raw_stats = true;

        APLOptimizerConfig cfg;
        cfg.population_size = 8;
        cfg.generations = 2;
        cfg.eval_simulations = 40;
        cfg.benchmark_simulations = 80;
        cfg.seed_from_current_apl = true;
        cfg.seed = 42;

        auto res = APLOptimizer::optimize_apl(sim, cfg);
        printf("  Preset: %-42s | Base: %5.1f DPS | Opt: %5.1f DPS (%+5.1f%%)\n",
               preset.display_name, res.baseline_dps, res.optimized_dps, res.dps_gain_pct);

        CHECK(res.optimized_dps >= res.baseline_dps - 1.0);
        CHECK(!res.top_candidates.empty());
        CHECK_EQ(res.top_candidates[0].rank, 1);
    }
}

TEST_CASE(VIPERPipeline, SimulatedAnnealingAPLOptimizerRun) {
    WarlockSimulator sim;
    sim.talents = Talents::create_sm_ruin();
    sim.fight_duration = 180.0;
    sim.randomize_duration = true;
    sim.duration_variance = 30.0;
    sim.use_raw_stats = true;

    APLOptimizerConfig cfg;
    cfg.population_size = 16;
    cfg.generations = 4;
    cfg.eval_simulations = 50;
    cfg.benchmark_simulations = 120;
    cfg.seed_from_current_apl = true;
    cfg.use_simulated_annealing = true;
    cfg.sa_initial_temp = 30.0f;
    cfg.sa_min_temp = 0.5f;
    cfg.seed = 42;

    auto res = APLOptimizer::optimize_apl(sim, cfg);

    printf("\n=== [Simulated Annealing APL Run (SM/Ruin 180s)] ===\n");
    printf("Baseline Expected DPS: %.1f ± %.1f\n", res.baseline_dps, res.baseline_stddev);
    printf("SA Champion DPS:       %.1f ± %.1f (%+.2f%%)\n", res.optimized_dps, res.optimized_stddev, res.dps_gain_pct);
    printf("Evolution History:     %zu epochs recorded\n", res.evolution_history.size());
    printf("Top Candidates Count:  %zu\n", res.top_candidates.size());

    for (const auto& cand : res.top_candidates) {
        printf("  Rank #%d: %-32s %.1f DPS (%+.2f%%)\n",
               cand.rank, cand.name.c_str(), cand.dps, cand.gain_pct);
    }

    CHECK(res.baseline_dps > 300.0);
    CHECK(res.optimized_dps >= res.baseline_dps - 0.5);
    CHECK(!res.top_candidates.empty());
    CHECK_EQ(res.top_candidates[0].rank, 1);
    CHECK_EQ(res.evolution_history.size(), 4);
}

TEST_CASE(VIPERPipeline, ShadowburnISBConditionalExecution) {
    // 1. With ISB required but 0% shadow crit / no ISB triggers, SHADOWBURN_ISB must never fire
    {
        FastRNG rng(42);
        WarlockSimulator sim;
        sim.talents = Talents::create_sm_ruin();
        sim.talents.destro.shadowburn = 1;
        sim.talents.destro.ruin = 1;
        sim.fight_duration = 30.0;
        sim.use_raw_stats = true;
        sim.raw_stats.spell_crit_percent = -100.0; // Ensure 0% crit so ISB never procs from SB

        PriorityRule sb_isb_rule;
        sb_isb_rule.action = PriorityAction::SHADOWBURN_ISB;
        sb_isb_rule.spell_id = SpellID::SHADOWBURN;
        sb_isb_rule.enabled = true;
        sb_isb_rule.use_custom_thresholds = true;
        sb_isb_rule.require_isb_active = true;

        PriorityRule filler_rule;
        filler_rule.action = PriorityAction::SHADOW_BOLT_FILLER;
        filler_rule.spell_id = SpellID::SHADOW_BOLT;
        filler_rule.enabled = true;

        sim.policy.use_custom_apl = true;
        sim.policy.custom_rules = { sb_isb_rule, filler_rule };

        SimResult res = sim.run_single_simulation(rng);
        // With 0% crit, ISB never activated, so Shadowburn casts should be exactly 0
        int sb_casts = res.spell_stats[static_cast<size_t>(SpellID::SHADOWBURN)].casts;
        CHECK_EQ(sb_casts, 0);
    }

    // 2. With unconditional SHADOWBURN rule, Shadowburn MUST fire on cooldown even without ISB
    {
        FastRNG rng(42);
        WarlockSimulator sim;
        sim.talents = Talents::create_sm_ruin();
        sim.talents.destro.shadowburn = 1;
        sim.fight_duration = 30.0;
        sim.use_raw_stats = true;
        sim.raw_stats.spell_crit_percent = -100.0;

        PriorityRule sb_rule;
        sb_rule.action = PriorityAction::SHADOWBURN;
        sb_rule.spell_id = SpellID::SHADOWBURN;
        sb_rule.enabled = true;
        sb_rule.use_custom_thresholds = false;
        sb_rule.require_isb_active = false;

        PriorityRule filler_rule;
        filler_rule.action = PriorityAction::SHADOW_BOLT_FILLER;
        filler_rule.spell_id = SpellID::SHADOW_BOLT;
        filler_rule.enabled = true;

        sim.policy.use_custom_apl = true;
        sim.policy.custom_rules = { sb_rule, filler_rule };

        SimResult res = sim.run_single_simulation(rng);
        int sb_casts = res.spell_stats[static_cast<size_t>(SpellID::SHADOWBURN)].casts;
        CHECK(sb_casts > 0);
    }
}




