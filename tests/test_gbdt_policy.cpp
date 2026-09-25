#include "test_framework.hpp"
#include "src/sim/common/gbdt.hpp"
#include <cmath>

TEST_CASE(GBDTTest, gbdt_synthetic_non_linear_regression) {
    // Generate non-linear synthetic regression dataset: y = 2.0 * x0^2 - 3.0 * x1 + 1.5 * (x2 > 0.5)
    std::vector<std::vector<float>> X;
    std::vector<float> y;
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (size_t i = 0; i < 500; ++i) {
        float x0 = dist(rng);
        float x1 = dist(rng);
        float x2 = dist(rng);
        float x3 = dist(rng);
        float target = 2.0f * x0 * x0 - 3.0f * x1 + (x2 > 0.5f ? 1.5f : 0.0f);
        X.push_back({x0, x1, x2, x3});
        y.push_back(target);
    }

    sim::GBDTConfig cfg;
    cfg.num_trees = 30;
    cfg.max_depth = 4;
    cfg.learning_rate = 0.15f;
    cfg.l2_reg = 1.0f;
    cfg.min_child_weight = 2.0f;

    sim::GBDTRegressor model(cfg);
    model.fit(X, y);

    CHECK_EQ(model.num_trees(), 30);

    // Evaluate Mean Squared Error on test set
    double total_mse = 0.0;
    for (size_t i = 0; i < 100; ++i) {
        float x0 = dist(rng);
        float x1 = dist(rng);
        float x2 = dist(rng);
        float x3 = dist(rng);
        float true_val = 2.0f * x0 * x0 - 3.0f * x1 + (x2 > 0.5f ? 1.5f : 0.0f);
        std::vector<float> sample = {x0, x1, x2, x3};
        float pred = model.predict(sample.data());
        float err = true_val - pred;
        total_mse += err * err;
    }
    double rmse = std::sqrt(total_mse / 100.0);
    // GBDT should fit this non-linear function with low RMSE
    CHECK(rmse < 0.25);
}

TEST_CASE(GBDTTest, gbdt_multi_action_q_policy_selection) {
    // Simulate Q-learning dataset where:
    // Action 0 (Life Tap) is best when mana <= 0.25
    // Action 1 (Soul Fire) is best when target_hp <= 0.35 and decimation is active
    // Action 2 (Shadow Bolt) is default filler
    sim::GBDTConfig cfg;
    cfg.num_trees = 25;
    cfg.max_depth = 3;
    cfg.learning_rate = 0.15f;

    std::vector<sim::GBDTMultiActionQPolicy::QSample> samples;
    std::mt19937 rng(123);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (size_t i = 0; i < 400; ++i) {
        sim::SimObservation obs;
        obs.player_mana_pct = dist(rng);
        obs.target_hp_pct = dist(rng);
        obs.decimation_rem_sec = (dist(rng) > 0.5f) ? 8.0f : 0.0f;
        obs.time_remaining_sec = 60.0f * dist(rng);

        // Action 0 (Life Tap) Q-value
        float q0 = (obs.player_mana_pct <= 0.25f) ? 2500.0f : 500.0f;
        // Action 1 (Soul Fire) Q-value
        float q1 = (obs.target_hp_pct <= 0.35f && obs.decimation_rem_sec > 0.0f) ? 3000.0f : 800.0f;
        // Action 2 (Shadow Bolt filler) Q-value
        float q2 = 1800.0f;

        samples.push_back({obs, 0, q0, 1.0f});
        samples.push_back({obs, 1, q1, 1.0f});
        samples.push_back({obs, 2, q2, 1.0f});
    }

    sim::GBDTMultiActionQPolicy policy(cfg);
    policy.fit(samples);

    // Test Case 1: Low mana (mana = 0.15) -> Should select Life Tap (Action 0)
    sim::SimObservation o1;
    o1.player_mana_pct = 0.15f;
    o1.target_hp_pct = 0.80f;
    o1.decimation_rem_sec = 0.0f;
    uint8_t best_act1 = policy.select_best_action(o1, {0, 1, 2});
    CHECK_EQ(best_act1, 0);

    // Test Case 2: High mana, Execute phase + Decimation active -> Should select Soul Fire (Action 1)
    sim::SimObservation o2;
    o2.player_mana_pct = 0.80f;
    o2.target_hp_pct = 0.20f;
    o2.decimation_rem_sec = 5.0f;
    uint8_t best_act2 = policy.select_best_action(o2, {0, 1, 2});
    CHECK_EQ(best_act2, 1);

    // Test Case 3: Action 1 is illegal (masked out) in execute phase -> Should select Filler (Action 2)
    uint8_t masked_act = policy.select_best_action(o2, {0, 2}); // Action 1 not legal
    CHECK_EQ(masked_act, 2);
}
