#include "test_framework.hpp"
#include "src/sim/warlock/apl_analyzer.hpp"
#include "src/sim/warlock/spec_presets.hpp"

using namespace warlock;

TEST_CASE(APLAnalyzerTests, SingleRunAnalysisBasicProperties) {
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    APLAnalysisRun run = APLAnalyzer::analyze_single_run(sim, 1, 42, 6);

    CHECK_EQ(run.run_index, 1);
    CHECK(run.apl_dps > 0.0);
    CHECK(run.mcts_dps > 0.0);
    CHECK(run.total_decisions > 10);
    CHECK(run.agreement_rate_pct >= 0.0 && run.agreement_rate_pct <= 100.0);

    for (const auto& ev : run.events) {
        CHECK(ev.timestamp >= 0.0 && ev.timestamp <= sim.fight_duration + 5.0);
        CHECK(!ev.apl_action_name.empty());
        CHECK(!ev.mcts_action_name.empty());
        CHECK(ev.delta_dps > 0.0);
        CHECK(ev.confidence_pct >= 50.0 && ev.confidence_pct <= 100.0);
        CHECK(!ev.candidate_evals.empty());
        CHECK(!ev.rationale.empty());
    }
}

TEST_CASE(APLAnalyzerTests, MultiRunBatchReportAggregation) {
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_dp_af_shadow();
    sim.fight_duration = 60.0;

    APLAnalysisReport report = APLAnalyzer::run_analysis(sim, 3, 6, nullptr, 1337);

    CHECK(report.is_valid);
    CHECK_EQ(report.total_runs, 3);
    CHECK_EQ(report.runs.size(), 3);
    CHECK(report.avg_apl_dps > 0.0);
    CHECK(report.avg_mcts_dps > 0.0);
    CHECK(report.total_decisions_evaluated > 30);
    CHECK(report.overall_agreement_pct >= 0.0 && report.overall_agreement_pct <= 100.0);

    for (const auto& mismatch : report.top_mismatches) {
        CHECK(mismatch.occurrences > 0);
        CHECK(!mismatch.apl_action_name.empty());
        CHECK(!mismatch.preferred_mcts_action_name.empty());
        CHECK(!mismatch.primary_cause.empty());
        CHECK(mismatch.avg_confidence_pct >= 50.0);
    }
}

TEST_CASE(APLAnalyzerTests, DetectsFlawedSuboptimalRotationAPL) {
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    // Intentionally create a flawed custom APL: Spam Life Tap unconditionally at top priority!
    sim.policy.use_custom_apl = true;
    sim.policy.custom_rules.clear();

    PriorityRule flawed_tap;
    flawed_tap.action = PriorityAction::LIFE_TAP;
    flawed_tap.spell_id = SpellID::LIFE_TAP;
    flawed_tap.name = "Life Tap Spam";
    flawed_tap.enabled = true;
    flawed_tap.use_custom_thresholds = true; // Always tap
    flawed_tap.max_mana_pct = 1.0f;
    flawed_tap.min_mana_pct = 0.0f;
    flawed_tap.min_hp_pct = 0.20f;
    flawed_tap.condition_summary = "Unconditional Tap";
    sim.policy.custom_rules.push_back(flawed_tap);

    PriorityRule filler;
    filler.action = PriorityAction::SHADOW_BOLT_FILLER;
    filler.spell_id = SpellID::SHADOW_BOLT;
    filler.name = "Shadow Bolt";
    filler.enabled = true;
    filler.use_custom_thresholds = false;
    filler.condition_summary = "Filler";
    sim.policy.custom_rules.push_back(filler);

    APLAnalysisRun run = APLAnalyzer::analyze_single_run(sim, 1, 999, 8);

    CHECK(run.divergence_count > 0);
    CHECK(run.mcts_dps >= run.apl_dps);

    // Verify that Life Tap divergence was detected with statistical confidence
    bool found_tap_divergence = false;
    for (const auto& ev : run.events) {
        if (ev.apl_action == PriorityAction::LIFE_TAP) {
            found_tap_divergence = true;
            CHECK(ev.delta_dps > 0.0);
            CHECK(ev.confidence_pct >= 50.0);
            CHECK(!ev.candidate_evals.empty());
            CHECK(!ev.rationale.empty());
        }
    }
    CHECK(found_tap_divergence);
}

TEST_CASE(APLAnalyzerTests, HardCappedAdaptiveRolloutsEarlyStopping) {
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 30.0;

    std::vector<PriorityAction> legal_cands = {
        PriorityAction::SHADOW_BOLT_FILLER,
        PriorityAction::LIFE_TAP,
        PriorityAction::CORRUPTION
    };

    size_t hard_cap = 64;
    size_t min_rollouts = 16;

    // Test Adaptive Mode: Should respect hard cap and record valid rollout counts
    auto adaptive_evals = APLAnalyzer::evaluate_candidate_branches_crn(
        sim, {}, legal_cands, hard_cap, 12345, true, min_rollouts
    );

    CHECK_EQ(adaptive_evals.size(), legal_cands.size());
    for (const auto& ev : adaptive_evals) {
        CHECK(ev.rollout_count >= min_rollouts);
        CHECK(ev.rollout_count <= hard_cap);
        CHECK(ev.mean_dps >= 0.0);
    }

    // Test Fixed Mode: All candidates must receive exactly hard_cap rollouts
    auto fixed_evals = APLAnalyzer::evaluate_candidate_branches_crn(
        sim, {}, legal_cands, hard_cap, 12345, false, min_rollouts
    );

    CHECK_EQ(fixed_evals.size(), legal_cands.size());
    for (const auto& ev : fixed_evals) {
        CHECK_EQ(ev.rollout_count, hard_cap);
    }
}

TEST_CASE(APLAnalyzerTests, MultithreadedBlunderAnalysisDeterministicEquivalence) {
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 45.0;

    // Run single-threaded blunder analysis (max_threads = 1)
    APLAnalysisRun single_thread_run = APLAnalyzer::analyze_single_run(
        sim, 1, 4242, 32, AnalysisMode::BLUNDERS_ONLY, true, nullptr, 1
    );

    // Run multi-threaded blunder analysis across all available hardware threads
    APLAnalysisRun multi_thread_run = APLAnalyzer::analyze_single_run(
        sim, 1, 4242, 32, AnalysisMode::BLUNDERS_ONLY, true, nullptr, 0
    );

    // Verify bit-for-bit identical results and determinism
    CHECK_EQ(single_thread_run.total_decisions, multi_thread_run.total_decisions);
    CHECK_EQ(single_thread_run.divergence_count, multi_thread_run.divergence_count);
    CHECK_NEAR(single_thread_run.apl_dps, multi_thread_run.apl_dps, 1e-6);
    CHECK_NEAR(single_thread_run.agreement_rate_pct, multi_thread_run.agreement_rate_pct, 1e-6);
    CHECK_EQ(single_thread_run.events.size(), multi_thread_run.events.size());

    for (size_t i = 0; i < single_thread_run.events.size(); ++i) {
        const auto& ev_s = single_thread_run.events[i];
        const auto& ev_m = multi_thread_run.events[i];
        CHECK_EQ(ev_s.decision_step, ev_m.decision_step);
        CHECK_EQ(ev_s.blunder_rank, ev_m.blunder_rank);
        CHECK(ev_s.apl_action == ev_m.apl_action);
        CHECK(ev_s.mcts_action == ev_m.mcts_action);
        CHECK_NEAR(ev_s.delta_dps, ev_m.delta_dps, 1e-5);
        CHECK_NEAR(ev_s.confidence_pct, ev_m.confidence_pct, 1e-5);
    }
}

TEST_CASE(APLAnalyzerTests, ReportStatisticalConfidenceBoundsAndOptimality) {
    APLAnalysisReport report;
    CHECK_EQ(report.total_runs, 0);

    APLAnalysisRun r1;
    r1.run_index = 1;
    r1.apl_dps = 2000.0;
    r1.mcts_dps = 2100.0;
    r1.total_decisions = 50;
    r1.divergence_count = 2;
    report.add_run(r1);

    CHECK_EQ(report.total_runs, 1);
    CHECK_NEAR(report.avg_apl_dps, 2000.0, 1e-6);
    CHECK_NEAR(report.avg_mcts_dps, 2100.0, 1e-6);
    CHECK_NEAR(report.avg_dps_loss, 100.0, 1e-6);
    CHECK_NEAR(report.optimality_pct, (2000.0 / 2100.0) * 100.0, 1e-4);
    CHECK_EQ(report.apl_dps_stderr, 0.0);

    APLAnalysisRun r2;
    r2.run_index = 2;
    r2.apl_dps = 2050.0;
    r2.mcts_dps = 2150.0;
    r2.total_decisions = 50;
    r2.divergence_count = 1;
    report.add_run(r2);

    CHECK_EQ(report.total_runs, 2);
    CHECK_NEAR(report.avg_apl_dps, 2025.0, 1e-6);
    CHECK_NEAR(report.avg_mcts_dps, 2125.0, 1e-6);
    CHECK_NEAR(report.avg_dps_loss, 100.0, 1e-6);
    CHECK(report.apl_dps_stddev > 0.0);
    CHECK(report.apl_dps_stderr > 0.0);
    CHECK(report.apl_dps_ci_lower < report.avg_apl_dps);
    CHECK(report.apl_dps_ci_upper > report.avg_apl_dps);
    CHECK(report.optimality_pct > 90.0 && report.optimality_pct <= 100.0);
    CHECK(report.optimality_pct_ci_lower <= report.optimality_pct);
    CHECK(report.optimality_pct_ci_upper >= report.optimality_pct);
}

TEST_CASE(APLAnalyzerTests, ContrastiveTrajectoryDiffEvaluation) {
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.fight_duration = 60.0;

    std::vector<PriorityAction> prefix = { PriorityAction::CORRUPTION, PriorityAction::IMMOLATE };
    auto diff = APLAnalyzer::compute_contrastive_trajectory_diff(
        sim, 42, 2, prefix,
        PriorityAction::LIFE_TAP,           // Suboptimal APL action when mana is full
        PriorityAction::SHADOW_BOLT_FILLER, // Optimal MCTS action
        16                                  // 16 ensemble rollouts
    );

    CHECK(diff.computed);
    CHECK(diff.apl_branch.root_action == PriorityAction::LIFE_TAP);
    CHECK(diff.mcts_branch.root_action == PriorityAction::SHADOW_BOLT_FILLER);
    CHECK(diff.mcts_branch.metrics.total_damage >= diff.apl_branch.metrics.total_damage);
    CHECK(diff.mcts_branch.metrics.dps >= diff.apl_branch.metrics.dps);
    CHECK(!diff.takeaways.empty());
    CHECK(diff.apl_branch.metrics.immolate_uptime_pct >= 0.0);
    CHECK(diff.mcts_branch.metrics.immolate_uptime_pct >= 0.0);
    CHECK(diff.apl_branch.metrics.corruption_uptime_pct >= 0.0);
    CHECK(diff.mcts_branch.metrics.corruption_uptime_pct >= 0.0);
    CHECK(diff.apl_branch.metrics.final_mana >= 0.0);
    CHECK(diff.mcts_branch.metrics.final_mana >= 0.0);
}



