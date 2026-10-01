#include "test_framework.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/webgpu/webgpu_sim_runner.hpp"
#include <iostream>

TEST_CASE(WebGPURunner, InAppSimulationAndThroughput) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::HUMAN;
    sim.talents = warlock::Talents::create_forever_shadow_destro();
    sim.raw_stats.spell_power = 500.0;
    sim.raw_stats.shadow_power = 100.0;
    sim.raw_stats.max_mana = 6500.0;
    sim.raw_stats.spell_hit_percent = 6.0;
    sim.raw_stats.spell_crit_percent = 15.0;
    sim.use_raw_stats = true;

    // Run 10,000 iterations via next-event engine
    auto res = warlock::WebGPUSimRunner::run_batch(sim, 10000, 0);

    CHECK(res.total_iterations == 10000);
    CHECK(res.mean_dps > 200.0 && res.mean_dps < 2000.0);
    CHECK(res.std_dev_dps > 0.0);
    CHECK(res.min_dps > 0.0);
    CHECK(res.max_dps >= res.min_dps);
    CHECK(res.p50_dps >= res.p5_dps && res.p50_dps <= res.p95_dps);
    CHECK(res.histogram.size() == 20);
    CHECK(res.iterations_per_second > 10000.0);
}

TEST_CASE(WebGPURunner, ImprovedShadowBoltParityAndUptime) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::HUMAN;
    sim.raw_stats.spell_power = 500.0;
    sim.raw_stats.shadow_power = 0.0;
    sim.raw_stats.max_mana = 6500.0;
    sim.raw_stats.spell_hit_percent = 12.0; // 95% cap
    sim.raw_stats.spell_crit_percent = 25.0; // 25% crit
    sim.use_raw_stats = true;
    sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
    sim.policy.corruption = warlock::DotPolicy::NEVER;

    // 1. Without ISB (0/5)
    sim.talents.destro.improved_shadow_bolt = 0;
    sim.talents.destro.ruin = 1;
    sim.talents.destro.bane = 5;
    auto res_no_isb = warlock::WebGPUSimRunner::run_batch(sim, 10000, 0, 12345);
    CHECK_EQ(res_no_isb.mean_isb_uptime, 0.0);

    // 2. With 5/5 ISB (Classic charges: 4 charges)
    sim.talents.destro.improved_shadow_bolt = 5;
    sim.mechanics.isb_has_charges = true;
    auto res_isb_charges = warlock::WebGPUSimRunner::run_batch(sim, 10000, 0, 12345);
    CHECK(res_isb_charges.mean_isb_uptime > 30.0 && res_isb_charges.mean_isb_uptime < 90.0);
    CHECK(res_isb_charges.mean_dps > res_no_isb.mean_dps * 1.05); // Noticeable DPS increase from +20% bonus

    // 3. With 5/5 ISB (Forever chargeless 12s window)
    sim.mechanics.isb_has_charges = false;
    auto res_isb_window = warlock::WebGPUSimRunner::run_batch(sim, 10000, 0, 12345);
    CHECK(res_isb_window.mean_isb_uptime >= res_isb_charges.mean_isb_uptime);
    CHECK(res_isb_window.mean_dps >= res_isb_charges.mean_dps);
}

TEST_CASE(WebGPURunner, MultiDotAndCurseManagement) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::HUMAN;
    sim.raw_stats.spell_power = 600.0;
    sim.raw_stats.max_mana = 7000.0;
    sim.raw_stats.spell_hit_percent = 16.0;
    sim.raw_stats.spell_crit_percent = 20.0;
    sim.use_raw_stats = true;

    // 1. Full Affliction multi-DoT build (Agony + Corruption + Siphon Life + Immolate)
    sim.policy.pet = warlock::PetChoice::NONE;
    sim.policy.rotation = warlock::RotationChoice::AFFLICTION_HYBRID_DOTS;
    sim.policy.curse = warlock::CurseChoice::BANE_OF_AGONY;
    sim.policy.corruption = warlock::DotPolicy::ALWAYS;
    sim.policy.maintain_immolate = true;
    sim.talents.aff.siphon_life = 1;
    sim.talents.aff.improved_bane_of_agony = 3;
    sim.talents.aff.shadow_mastery = 5;
    sim.talents.destro.ruin = 1;
    sim.talents.destro.bane = 5;

    auto res_multidot = warlock::WebGPUSimRunner::run_batch(sim, 10000, 0, 777);
    CHECK(res_multidot.pct_agony > 3.0 || res_multidot.pct_doom > 3.0);
    CHECK(res_multidot.pct_corruption > 5.0);
    CHECK(res_multidot.pct_siphon_life > 3.0);
    CHECK(res_multidot.pct_immolate > 3.0);
    CHECK(res_multidot.pct_shadow_bolt > 30.0);
    CHECK_NEAR(res_multidot.pct_agony + res_multidot.pct_doom + res_multidot.pct_corruption + res_multidot.pct_siphon_life + res_multidot.pct_immolate + res_multidot.pct_shadow_bolt, 100.0, 1.0);

    // 2. Curse of Doom build
    sim.policy.curse = warlock::CurseChoice::BANE_OF_DOOM;
    sim.talents.aff.siphon_life = 0;
    sim.policy.maintain_immolate = false;
    auto res_doom = warlock::WebGPUSimRunner::run_batch(sim, 10000, 0, 777);
    CHECK(res_doom.pct_doom > 5.0);
    CHECK_EQ(res_doom.pct_agony, 0.0);
    CHECK_EQ(res_doom.pct_siphon_life, 0.0);
    CHECK_EQ(res_doom.pct_immolate, 0.0);
}

TEST_CASE(WebGPURunner, PetAndSacrificeParity) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::HUMAN;
    sim.raw_stats.spell_power = 500.0;
    sim.raw_stats.max_mana = 6500.0;
    sim.raw_stats.spell_hit_percent = 10.0;
    sim.raw_stats.spell_crit_percent = 15.0;
    sim.use_raw_stats = true;
    sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
    sim.policy.corruption = warlock::DotPolicy::NEVER;
    sim.buffs = warlock::BuffConfig{};
    sim.talents = warlock::Talents{};
    sim.talents.destro.bane = 5;
    sim.talents.destro.ruin = 1;

    // 1. Base no pet
    sim.policy.pet = warlock::PetChoice::NONE;
    auto res_none = warlock::WebGPUSimRunner::run_batch(sim, 5000, 0, 42);
    CHECK_EQ(res_none.pct_pet, 0.0);

    // 2. Active Imp (Firebolt)
    sim.policy.pet = warlock::PetChoice::IMP;
    sim.talents.demo.improved_imp = 3; // +30% Firebolt damage
    sim.talents.demo.unholy_power = 5;
    auto res_imp = warlock::WebGPUSimRunner::run_batch(sim, 5000, 0, 42);
    CHECK(res_imp.pct_pet > 4.0);
    CHECK(res_imp.mean_dps > res_none.mean_dps + 10.0);

    // 3. Active Succubus (Melee + Lash of Pain)
    sim.talents.demo.improved_imp = 0;
    sim.policy.pet = warlock::PetChoice::SUCCUBUS;
    sim.talents.demo.improved_sayaad = 2; // +20% damage
    auto res_succ = warlock::WebGPUSimRunner::run_batch(sim, 5000, 0, 42);
    CHECK(res_succ.pct_pet > 5.0);
    CHECK(res_succ.mean_dps > res_none.mean_dps + 20.0);

    // 4. Demonic Sacrifice: Succubus (+15% Shadow Damage)
    sim.policy.pet = warlock::PetChoice::NONE;
    sim.talents.demo.improved_sayaad = 0;
    sim.talents.demo.demonic_sacrifice = 1;
    sim.buffs.sacrifice_succubus = true;
    auto res_ds_succ = warlock::WebGPUSimRunner::run_batch(sim, 5000, 0, 42);
    CHECK_EQ(res_ds_succ.pct_pet, 0.0);
    // 15% bonus on all shadow bolts
    CHECK_NEAR(res_ds_succ.mean_dps, res_none.mean_dps * 1.15, res_none.mean_dps * 0.03);
}


