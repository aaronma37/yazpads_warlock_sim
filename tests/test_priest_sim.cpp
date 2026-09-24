#include "test_framework.hpp"
#include "src/sim/priest/priest_sim.hpp"
#include "src/sim/priest/talents.hpp"
#include "src/sim/priest/talent_graph.hpp"
#include "src/sim/priest/spells.hpp"
#include "src/sim/priest/spec_presets.hpp"
#include "src/sim/priest/optimizer.hpp"
#include "src/sim/priest/parallel_runner.hpp"
#include "src/sim/warlock_sim.hpp"
#include <random>

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

TEST_CASE(PriestSim, DevouringPlagueAndContagion) {
    sim::FastRNG rng(777);
    PriestSimulator sim;
    sim.race = sim::Race::UNDEAD;
    sim.fight_duration = 65.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 500.0;
    sim.raw_stats.spell_hit_percent = 16.0;
    sim.raw_stats.max_mana = 5000.0;
    sim.policy.cast_devouring_plague = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dmg_devouring_plague > 0.0);
    CHECK(res.healing_devouring_plague > 0.0);
    CHECK_NEAR(res.dmg_devouring_plague, res.healing_devouring_plague, 0.01);
}

TEST_CASE(PriestSim, ShadowWordDeathBacklashMaxHealth) {
    sim::FastRNG rng(101);
    PriestSimulator sim;
    sim.fight_duration = 30.0;
    sim.use_raw_stats = true;
    sim.buffs = sim::BuffConfig{}; // Zero extra buffs for exact max health control
    sim.raw_stats.stamina = 200.0; // 1404 base health + 2000 = 3404 max health
    sim.raw_stats.spell_power = 400.0;
    sim.raw_stats.max_mana = 5000.0;
    sim.mechanics.sw_death_backlash = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dmg_sw_death > 0.0);
    CHECK(res.self_damage_sw_death > 0.0);
    // Total stamina = base 119 + gear 200 = 319 stamina. Total HP = 1404 + 3190 = 4594 HP.
    // 10% of 4594 HP per cast = 459.4 self-damage per cast
    int swd_hits = res.spell_stats[static_cast<size_t>(SpellID::SHADOW_WORD_DEATH)].hits;
    if (swd_hits > 0) {
        CHECK_NEAR(res.self_damage_sw_death / swd_hits, 459.4, 1.0);
    }
}

TEST_CASE(PriestSim, HolyPrecisionHitCap) {
    PriestSimulator sim;
    sim.target_config.level = 63; // Boss level: 83% base hit
    sim.use_raw_stats = true;
    sim.raw_stats.spell_hit_percent = 0.0;

    sim.talents.disc.holy_precision = 0;
    CHECK_NEAR(sim.calculate_hit_chance(sim::School::HOLY), 0.83, 0.001);

    sim.talents.disc.holy_precision = 1; // +6%
    CHECK_NEAR(sim.calculate_hit_chance(sim::School::HOLY), 0.89, 0.001);

    sim.talents.disc.holy_precision = 2; // +12%
    CHECK_NEAR(sim.calculate_hit_chance(sim::School::HOLY), 0.95, 0.001);

    sim.talents.disc.holy_precision = 3; // +18% -> reaches 100% cap (overcaps by 1%)
    CHECK_NEAR(sim.calculate_hit_chance(sim::School::HOLY), 1.00, 0.001);
}

TEST_CASE(PriestSim, HolyFireWeavingAndPenance) {
    sim::FastRNG rng(1234);
    PriestSimulator sim;
    sim.talents = Talents::create_forever_smite();
    sim.talents.disc.penance = 1;
    sim.talents.disc.power_in_light = 5; // +10% Smite/Penance during Holy Fire
    sim.policy.rotation = RotationChoice::SMITE_PRIEST;
    sim.policy.cast_holy_fire = true;
    sim.policy.cast_penance = true;
    sim.fight_duration = 30.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 400.0;
    sim.raw_stats.holy_power = 100.0;
    sim.raw_stats.intellect = 200.0;
    sim.raw_stats.spirit = 200.0;
    sim.raw_stats.max_mana = 4500.0;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dmg_holy_fire > 0.0);
    CHECK(res.dmg_penance > 0.0);
    CHECK(res.dmg_smite > 0.0);
}

TEST_CASE(PriestSim, DefaultStatsMatchWarlock) {
    PriestSimulator psim;
    warlock::WarlockSimulator wsim;

    CHECK(psim.use_raw_stats);
    CHECK(wsim.use_raw_stats);
    CHECK_EQ(psim.raw_stats.spell_power, wsim.raw_stats.spell_power);
    CHECK_EQ(psim.raw_stats.shadow_power, wsim.raw_stats.shadow_power);
    CHECK_EQ(psim.raw_stats.spell_hit_percent, wsim.raw_stats.spell_hit_percent);
    CHECK_EQ(psim.raw_stats.spell_crit_percent, wsim.raw_stats.spell_crit_percent);
    CHECK_EQ(psim.raw_stats.stamina, wsim.raw_stats.stamina);
    CHECK_EQ(psim.raw_stats.intellect, wsim.raw_stats.intellect);
    CHECK_EQ(psim.raw_stats.spirit, wsim.raw_stats.spirit);
}

TEST_CASE(PriestSim, TalentGraphRepairIllegalTalents) {
    const auto& graph = TalentGraph::get();
    std::mt19937_64 rng(42);

    // Create an illegal 18-point build with Power Infusion (index 17, Row 7)
    // Row 7 requires 30 points in rows 1..6 and Penance (index 14) maxed.
    std::vector<int> v(graph.nodes().size(), 0);
    v[0] = 5;  // Power in Light (Row 1)
    v[3] = 3;  // Silent Resolve (Row 2)
    v[7] = 3;  // Mental Agility (Row 3)
    v[10] = 3; // Improved Inner Fire (Row 4)
    v[11] = 3; // Mental Strength (Row 4)
    v[17] = 1; // Power Infusion (Row 7) -- ILLEGAL! Only 17 points in rows 1..6, Penance not taken.

    CHECK_EQ(graph.count_tree_points(v, 0), 18);
    CHECK(!graph.is_valid(v, 51));
    CHECK(!graph.is_valid(v, 18)); // Even at 18 points, tier requirements fail!

    // Repair into valid 51-point build
    graph.repair(v, rng);

    CHECK(graph.is_valid(v, 51));
    CHECK_EQ(graph.count_total_points(v), 51);

    // If PI is active, Disc MUST have >= 31 points and Penance must be 1
    if (v[17] > 0) {
        CHECK(graph.count_tree_points(v, 0) >= 31);
        CHECK_EQ(v[14], 1); // Penance prerequisite
    }
}

TEST_CASE(PriestSim, TalentGraphRequiredTalentPI) {
    const auto& graph = TalentGraph::get();
    std::mt19937_64 rng(100);

    std::vector<int> v(graph.nodes().size(), 0);
    std::vector<int> reqs = {17}; // Power Infusion required

    graph.repair(v, rng, reqs);

    CHECK(graph.is_valid(v, 51));
    CHECK_EQ(graph.count_total_points(v), 51);
    CHECK_EQ(v[17], 1);                         // Power Infusion maxed
    CHECK_EQ(v[14], 1);                         // Penance prerequisite maxed
    CHECK(graph.count_tree_points(v, 0) >= 31); // At least 31 points in Discipline
}

TEST_CASE(PriestSim, GeneticOptimizerStrictValidity) {
    PriestSimulator sim;
    sim.fight_duration = 30.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 400.0;
    sim.raw_stats.shadow_power = 100.0;
    sim.raw_stats.intellect = 200.0;
    sim.raw_stats.spirit = 200.0;
    sim.raw_stats.max_mana = 4500.0;

    const auto& graph = TalentGraph::get();

    // Run genetic AI with presets disabled to force pure exploration and repair
    auto results = Optimizer::optimize_genetic_ai(sim, 8, 3, 10, 20, false, false);
    CHECK(!results.empty());

    for (const auto& r : results) {
        auto genes = graph.to_vector(r.talents);
        CHECK_EQ(graph.count_total_points(genes), 51);
        CHECK(graph.is_valid(genes, 51));

        // Invariant: Power Infusion requires at least 31 points in Disc and Penance
        if (r.talents.disc.power_infusion > 0) {
            CHECK(r.talents.disc.total_points() >= 31);
            CHECK_EQ(r.talents.disc.penance, 1);
        }

        // Invariant: Shadowform requires at least 31 points in Shadow and Vampiric Embrace
        if (r.talents.shadow.shadowform > 0) {
            CHECK(r.talents.shadow.total_points() >= 31);
            CHECK_EQ(r.talents.shadow.vampiric_embrace, 1);
        }
    }
}

TEST_CASE(PriestSim, UndeadTouchOfTheGrave) {
    // 1. Undead priest triggers Touch of the Grave
    PriestSimulator undead_sim;
    undead_sim.race = sim::Race::UNDEAD;
    undead_sim.fight_duration = 180.0;
    undead_sim.use_raw_stats = true;
    undead_sim.raw_stats.spell_power = 500.0;
    undead_sim.raw_stats.shadow_power = 100.0;
    undead_sim.raw_stats.intellect = 250.0;
    undead_sim.raw_stats.spirit = 250.0;
    undead_sim.raw_stats.max_mana = 5000.0;
    undead_sim.raw_stats.max_health = 4000.0;

    sim::FastRNG rng1(1337);
    SimResult undead_res = undead_sim.run_single_simulation(rng1);
    CHECK(undead_res.touch_of_the_grave_procs > 0);
    CHECK(undead_res.dmg_touch_of_the_grave > 0.0);
    CHECK(undead_res.spell_stats[static_cast<size_t>(SpellID::TOUCH_OF_THE_GRAVE)].hits > 0);
    CHECK(undead_res.spell_stats[static_cast<size_t>(SpellID::TOUCH_OF_THE_GRAVE)].damage > 0.0);

    // 2. Non-undead (e.g. Human) priest never triggers Touch of the Grave
    PriestSimulator human_sim = undead_sim;
    human_sim.race = sim::Race::HUMAN;

    sim::FastRNG rng2(1337);
    SimResult human_res = human_sim.run_single_simulation(rng2);
    CHECK_EQ(human_res.touch_of_the_grave_procs, 0);
    CHECK_EQ(human_res.dmg_touch_of_the_grave, 0.0);
    CHECK_EQ(human_res.spell_stats[static_cast<size_t>(SpellID::TOUCH_OF_THE_GRAVE)].hits, 0);
    CHECK_EQ(human_res.spell_stats[static_cast<size_t>(SpellID::TOUCH_OF_THE_GRAVE)].damage, 0.0);

    // 3. Verify BatchSimResult records pct_touch_of_the_grave
    BatchSimResult batch = ParallelSimRunner::run_batch(undead_sim, 50, 2);
    CHECK(batch.pct_touch_of_the_grave > 0.0);
    CHECK(batch.spell_stats[static_cast<size_t>(SpellID::TOUCH_OF_THE_GRAVE)].mean_hits > 0.0);
}

TEST_CASE(PriestSim, RotationShadowNoMindBlast) {
    sim::FastRNG rng(42);
    PriestSimulator sim;
    sim.talents = Talents::create_forever_shadow();
    sim.policy.rotation = RotationChoice::SHADOW_NO_MB;
    sim.fight_duration = 60.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 500.0;
    sim.raw_stats.shadow_power = 100.0;
    sim.raw_stats.intellect = 250.0;
    sim.raw_stats.spirit = 250.0;
    sim.raw_stats.max_mana = 5000.0;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dps > 100.0);
    CHECK(res.dmg_sw_pain > 0.0);
    CHECK(res.dmg_mind_flay > 0.0);
    CHECK(res.dmg_devouring_plague > 0.0);
    CHECK_EQ(res.dmg_mind_blast, 0.0);

    auto rules = sim.policy.get_priority_rules(sim.talents, sim.race);
    bool has_mb = false;
    for (const auto& r : rules) {
        if (r.spell_id == SpellID::MIND_BLAST) has_mb = true;
    }
    CHECK(!has_mb);
}

TEST_CASE(PriestSim, RotationShadowSWPOnly) {
    sim::FastRNG rng(42);
    PriestSimulator sim;
    sim.talents = Talents::create_forever_shadow();
    sim.policy.rotation = RotationChoice::SHADOW_SWP_ONLY;
    sim.fight_duration = 60.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 500.0;
    sim.raw_stats.shadow_power = 100.0;
    sim.raw_stats.intellect = 250.0;
    sim.raw_stats.spirit = 250.0;
    sim.raw_stats.max_mana = 5000.0;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dps > 50.0);
    CHECK(res.dmg_sw_pain > 0.0);
    CHECK(res.dmg_mind_flay > 0.0);
    CHECK_EQ(res.dmg_mind_blast, 0.0);
    CHECK_EQ(res.dmg_devouring_plague, 0.0);

    auto rules = sim.policy.get_priority_rules(sim.talents, sim.race);
    for (const auto& r : rules) {
        CHECK(r.spell_id != SpellID::MIND_BLAST);
        CHECK(r.spell_id != SpellID::DEVOURING_PLAGUE);
    }
}

TEST_CASE(PriestSim, RotationPureSmite) {
    sim::FastRNG rng(42);
    PriestSimulator sim;
    sim.talents = Talents::create_forever_pi_smite();
    sim.policy.rotation = RotationChoice::PURE_SMITE;
    sim.fight_duration = 60.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 400.0;
    sim.raw_stats.holy_power = 100.0;
    sim.raw_stats.intellect = 200.0;
    sim.raw_stats.spirit = 200.0;
    sim.raw_stats.max_mana = 4500.0;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dps > 100.0);
    CHECK(res.dmg_smite > 0.0);
    CHECK(res.dmg_penance > 0.0);
    CHECK_EQ(res.dmg_holy_fire, 0.0);

    auto rules = sim.policy.get_priority_rules(sim.talents, sim.race);
    for (const auto& r : rules) {
        CHECK(r.spell_id != SpellID::HOLY_FIRE);
    }
}

TEST_CASE(PriestSim, RotationDiscInquisitor) {
    sim::FastRNG rng(42);
    PriestSimulator sim;
    sim.talents = Talents::create_forever_pi_smite();
    sim.policy.rotation = RotationChoice::DISC_INQUISITOR;
    sim.fight_duration = 60.0;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 400.0;
    sim.raw_stats.holy_power = 100.0;
    sim.raw_stats.shadow_power = 50.0;
    sim.raw_stats.intellect = 200.0;
    sim.raw_stats.spirit = 200.0;
    sim.raw_stats.max_mana = 4500.0;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dps > 100.0);
    CHECK(res.dmg_holy_fire > 0.0);
    CHECK(res.dmg_sw_pain > 0.0);
    CHECK(res.dmg_penance > 0.0);
    CHECK(res.dmg_sw_death > 0.0);
    CHECK(res.dmg_smite > 0.0);
}

TEST_CASE(PriestSim, RotationChoiceCoverageAndDescriptions) {
    for (int r = 0; r < static_cast<int>(RotationChoice::COUNT); ++r) {
        RotationChoice rc = static_cast<RotationChoice>(r);
        std::string name = rotation_choice_to_string(rc);
        std::string desc = rotation_choice_description(rc);
        CHECK(!name.empty());
        CHECK(!desc.empty());
        CHECK(name != "Unknown");
    }
}




