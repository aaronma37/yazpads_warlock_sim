#include "test_framework.hpp"
#include "src/sim/priest/priest_sim.hpp"
#include "src/sim/priest/talents.hpp"
#include "src/sim/priest/talent_graph.hpp"
#include "src/sim/priest/spells.hpp"
#include "src/sim/priest/spec_presets.hpp"
#include "src/sim/priest/optimizer.hpp"

using namespace priest;

TEST_CASE(PriestSim, TalentGraph53Nodes) {
    const auto& graph = TalentGraph::get();
    CHECK_EQ(graph.nodes().size(), (size_t)53);

    Talents shadow = Talents::create_forever_shadow();
    CHECK_EQ(shadow.total_points(), 51);
    CHECK_EQ(shadow.shadow.shadowform, 1);
    CHECK_EQ(shadow.shadow.mind_flay, 1);
    CHECK_EQ(shadow.disc.meditation, 3);

    auto vec = graph.to_vector(shadow);
    CHECK_EQ(graph.count_total_points(vec), 51);
    CHECK(graph.is_valid(vec, 51));

    Talents reconstructed = graph.to_talents(vec);
    CHECK_EQ(reconstructed.total_points(), 51);
    CHECK_EQ(reconstructed.shadow.shadowform, 1);
}

TEST_CASE(PriestSim, BaselineShadowSimulation) {
    sim::FastRNG rng(1337);
    PriestSimulator sim;
    sim.fight_duration = 60.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 500.0;
    sim.raw_stats.shadow_power = 100.0;
    sim.raw_stats.intellect = 250.0;
    sim.raw_stats.spirit = 250.0;
    sim.raw_stats.max_mana = 5000.0;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.duration >= 59.9);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 200.0);
    CHECK(res.dmg_sw_pain > 0.0);
    CHECK(res.dmg_mind_flay > 0.0);
    CHECK(res.dmg_mind_blast > 0.0);
    CHECK(res.shadow_weaving_procs > 0);
    CHECK(res.mana_spent > 0.0);
    CHECK(res.mana_gained > 0.0);
}

TEST_CASE(PriestSim, SmiteSimulation) {
    sim::FastRNG rng(42);
    PriestSimulator sim;
    sim.talents = Talents::create_forever_smite();
    sim.policy.rotation = RotationChoice::SMITE_PRIEST;
    sim.fight_duration = 30.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 400.0;
    sim.raw_stats.holy_power = 100.0;
    sim.raw_stats.intellect = 200.0;
    sim.raw_stats.spirit = 200.0;
    sim.raw_stats.max_mana = 4500.0;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dps > 100.0);
    CHECK(res.dmg_smite > 0.0);
}

TEST_CASE(PriestSim, OptimizerBenchmark) {
    PriestSimulator sim;
    sim.fight_duration = 30.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 400.0;
    sim.raw_stats.shadow_power = 100.0;
    sim.raw_stats.intellect = 200.0;
    sim.raw_stats.spirit = 200.0;
    sim.raw_stats.max_mana = 4500.0;

    auto results = Optimizer::optimize_talents(sim, 20, nullptr, false);
    CHECK_EQ(results.size(), (size_t)4);
    CHECK_EQ(results[0].rank, 1);
    CHECK(results[0].mean_dps >= results[1].mean_dps);
    CHECK(results[0].mean_dps > 50.0);
}

TEST_CASE(PriestSim, OptimizerGeneticAI) {
    PriestSimulator sim;
    sim.fight_duration = 30.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 400.0;
    sim.raw_stats.shadow_power = 100.0;
    sim.raw_stats.intellect = 200.0;
    sim.raw_stats.spirit = 200.0;
    sim.raw_stats.max_mana = 4500.0;

    auto results = Optimizer::optimize_genetic_ai(sim, 6, 2, 15, 25, true, false);
    CHECK(!results.empty());
    CHECK_EQ(results[0].rank, 1);
    CHECK(results[0].mean_dps > 50.0);
    CHECK_EQ(results[0].talents.total_points(), 51);
}

