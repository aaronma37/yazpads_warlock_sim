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

