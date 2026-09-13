#include "test_framework.hpp"
#include "src/sim/warlock_sim.hpp"

using namespace warlock;

TEST_CASE(Rotations, ShadowDestroDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    CHECK(res.cast_sequence.size() > 0);
    CHECK(res.shadow_bolt_casts > 0 || res.dmg_shadow_bolt > 0);
    // Immolate and Conflagrate should be used to trigger Shadow & Flame
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
}

TEST_CASE(Rotations, FireDestroDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_fire_destro();
    sim.policy.rotation = RotationChoice::FIRE_DESTRO;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    // Primary filler must be Incinerate
    CHECK(res.dmg_incinerate > 0.0);
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
}

TEST_CASE(Rotations, DeepAfflictionDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_deep_affliction();
    sim.policy.rotation = RotationChoice::DEEP_AFFLICTION;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_drain_hope > 0.0);
}

TEST_CASE(Rotations, SMRuinDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_sm_ruin();
    sim.policy.rotation = RotationChoice::SM_RUIN;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.shadow_bolt_casts > 0);
    CHECK(res.dmg_shadowburn > 0.0);
}

TEST_CASE(Rotations, DemonologyExecuteSoulFire) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_demonic_pact();
    sim.policy.rotation = RotationChoice::DEMONOLOGY_EXECUTE;
    sim.fight_duration = 60.0; // Long enough to enter execute phase (<35% HP in last 35% of fight)
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    // Should cast Soul Fire during execute phase
    CHECK(res.dmg_soul_fire > 0.0);
}

TEST_CASE(Rotations, PureShadowBoltNoDots) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_destro();
    sim.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    // Pure SB spam: no DoTs (Immolate, Corruption, Agony) should be cast
    CHECK_EQ(res.dmg_corruption, 0.0);
    CHECK_EQ(res.dmg_immolate, 0.0);
    CHECK_EQ(res.dmg_drain_hope, 0.0);
    CHECK(res.shadow_bolt_casts > 0);
}

TEST_CASE(Rotations, AfflictionMultiDotHybrid) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_deep_affliction();
    sim.policy.rotation = RotationChoice::AFFLICTION_HYBRID_DOTS;
    sim.policy.curse = CurseChoice::CURSE_OF_AGONY;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dmg_curse > 0.0);      // Agony
    CHECK(res.dmg_corruption > 0.0); // Corruption
    CHECK(res.dmg_immolate > 0.0);   // Immolate
    CHECK(res.dmg_drain_hope > 0.0); // Drain Hope
}

TEST_CASE(Rotations, AutoRotationSelection) {
    FastRNG rng(1337);
    
    // Auto on Fire Destro build should cast Incinerate
    WarlockSimulator sim_fire;
    sim_fire.talents = Talents::create_forever_fire_destro();
    sim_fire.policy.rotation = RotationChoice::AUTO;
    sim_fire.fight_duration = 30.0;
    SimResult res_fire = sim_fire.run_single_simulation(rng);
    CHECK(res_fire.dmg_incinerate > 0.0);

    // Auto on Deep Affliction build should channel Drain Hope & Corruption
    WarlockSimulator sim_aff;
    sim_aff.talents = Talents::create_forever_deep_affliction();
    sim_aff.policy.rotation = RotationChoice::AUTO;
    sim_aff.fight_duration = 30.0;
    SimResult res_aff = sim_aff.run_single_simulation(rng);
    CHECK(res_aff.dmg_corruption > 0.0);
    CHECK(res_aff.dmg_drain_hope > 0.0);
}

TEST_CASE(Rotations, LifeTapTriggerUnderThreshold) {
    FastRNG rng(42);
    WarlockSimulator sim;
    sim.fight_duration = 180.0; // Long fight to force mana depletion
    sim.policy.life_tap_threshold_pct = 30.0;
    sim.buffs.use_mana_potions = false;
    sim.buffs.use_demonic_runes = false;

    SimResult res = sim.run_single_simulation(rng);
    // Over a 3 minute fight without potions/runes, Life Tap must be triggered
    CHECK(res.life_taps > 0);
}
