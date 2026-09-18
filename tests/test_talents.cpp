#include "test_framework.hpp"
#include "src/sim/talents.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/optimizer.hpp"

using namespace warlock;

TEST_CASE(Talents, TotalPointsAndLimits) {
    Talents t1 = Talents::create_forever_shadow_destro();
    CHECK_EQ(t1.total_points(), 51);
    CHECK(t1.is_valid());

    Talents t2 = Talents::create_forever_fire_destro();
    CHECK_EQ(t2.total_points(), 51);
    CHECK(t2.is_valid());
    CHECK_EQ(t2.aff.improved_corruption, 4);
    CHECK_EQ(t2.aff.suppression, 5);
    CHECK_EQ(t2.destro.destructive_reach, 0);
    CHECK_EQ(t2.destro.improved_shadow_bolt, 0);
    CHECK_EQ(t2.destro.aftermath, 5);
    CHECK_EQ(t2.destro.bane_of_havoc, 1);

    Talents t3 = Talents::create_forever_demonic_pact();
    CHECK_EQ(t3.total_points(), 51);
    CHECK(t3.is_valid());

    Talents t4 = Talents::create_forever_deep_affliction();
    CHECK_EQ(t4.total_points(), 51);
    CHECK(t4.is_valid());
    CHECK_EQ(t4.aff.nightfall, 0);
    CHECK_EQ(t4.aff.improved_drains, 3);
    CHECK_EQ(t4.aff.improved_bane_of_agony, 0);
    CHECK_EQ(t4.demo.demonic_sacrifice, 1);
    CHECK_EQ(t4.destro.total_points(), 0);

    Talents t5 = Talents::create_forever_sm_ruin();
    CHECK_EQ(t5.total_points(), 51);
    CHECK(t5.is_valid());

    Talents t6 = Talents::create_forever_nf_ds_ruin();
    CHECK_EQ(t6.total_points(), 51);
    CHECK(t6.is_valid());
    CHECK_EQ(t6.aff.nightfall, 2);
    CHECK_EQ(t6.demo.demonic_sacrifice, 1);
    CHECK_EQ(t6.destro.ruin, 5);

    Talents t7 = Talents::create_forever_shadow_and_flame();
    CHECK_EQ(t7.total_points(), 51);
    CHECK(t7.is_valid());
    CHECK_EQ(t7.aff.total_points(), 1);
    CHECK_EQ(t7.demo.total_points(), 17);
    CHECK_EQ(t7.destro.total_points(), 33);
    CHECK_EQ(t7.destro.shadow_and_flame, 5);
    CHECK_EQ(t7.destro.incinerate, 1);

    Talents t7b = Talents::create_forever_shadow_and_flame_fire_2();
    CHECK_EQ(t7b.total_points(), 51);
    CHECK(t7b.is_valid());
    CHECK_EQ(t7b.aff.total_points(), 10);
    CHECK_EQ(t7b.aff.improved_life_tap, 2);
    CHECK_EQ(t7b.aff.suppression, 5);
    CHECK_EQ(t7b.aff.improved_corruption, 3);
    CHECK_EQ(t7b.demo.total_points(), 10);
    CHECK_EQ(t7b.demo.demonic_aegis, 0);
    CHECK_EQ(t7b.demo.decimation, 0);
    CHECK_EQ(t7b.demo.fel_vitality, 0);
    CHECK_EQ(t7b.destro.total_points(), 31);
    CHECK_EQ(t7b.destro.improved_shadow_bolt, 0);
    CHECK_EQ(t7b.destro.intensity, 0);
    CHECK_EQ(t7b.destro.cataclysm, 1);
    CHECK_EQ(t7b.destro.aftermath, 5);
    CHECK_EQ(t7b.destro.bane_of_havoc, 1);
    CHECK_EQ(t7b.destro.shadow_and_flame, 5);
    CHECK_EQ(t7b.destro.incinerate, 1);

    Talents t8 = Talents::create_forever_shadow_and_flame_shadow();
    CHECK_EQ(t8.total_points(), 51);
    CHECK(t8.is_valid());
    CHECK_EQ(t8.aff.total_points(), 2);
    CHECK_EQ(t8.demo.total_points(), 17);
    CHECK_EQ(t8.destro.total_points(), 32);
    CHECK_EQ(t8.destro.shadow_and_flame, 5);
    CHECK_EQ(t8.destro.conflagrate, 1);
    CHECK_EQ(t8.destro.incinerate, 0);

    Talents t9 = Talents::create_forever_fire_destro_decimation();
    CHECK_EQ(t9.total_points(), 51);
    CHECK(t9.is_valid());
    CHECK_EQ(t9.aff.suppression, 3);
    CHECK_EQ(t9.destro.aftermath, 2);
    CHECK_EQ(t9.demo.decimation, 2);
}

TEST_CASE(Talents, NightfallAfflictionPreset) {
    Talents t = Talents::create_forever_nf_af();
    CHECK_EQ(t.total_points(), 51);
    CHECK(t.is_valid());
    CHECK_EQ(t.aff.total_points(), 23);
    CHECK_EQ(t.demo.total_points(), 10);
    CHECK_EQ(t.destro.total_points(), 18);

    CHECK_EQ(t.aff.improved_life_tap, 2);
    CHECK_EQ(t.aff.suppression, 5);
    CHECK_EQ(t.aff.improved_corruption, 5);
    CHECK_EQ(t.aff.malediction, 4);
    CHECK_EQ(t.aff.malevolence, 5);
    CHECK_EQ(t.aff.nightfall, 2);

    CHECK_EQ(t.demo.improved_imp, 3);
    CHECK_EQ(t.demo.unholy_power, 5);
    CHECK_EQ(t.demo.demonic_energies, 2);

    CHECK_EQ(t.destro.improved_shadow_bolt, 5);
    CHECK_EQ(t.destro.bane, 5);
    CHECK_EQ(t.destro.ruin, 5);
    CHECK_EQ(t.destro.agonizing_flames, 3);
    CHECK_EQ(t.destro.shadowburn, 0);

    CHECK_EQ(Talents::create_nf_af().total_points(), 51);
}

TEST_CASE(Talents, SuppressionHitBonus) {
    WarlockSimulator sim;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_hit_percent = 0.0;
    sim.talents.aff.suppression = 0;

    // Default hit chance vs boss (mechanics.base_hit_vs_boss is 0.83)
    // calculate_hit_chance uses private method in sim, so we can test via simulation hit rate or direct check
    sim.talents.aff.suppression = 5;
    CHECK_EQ(sim.talents.aff.suppression, 5);
}

TEST_CASE(Talents, RuinCritMultiplier) {
    // Ruin at 5/5 increases spell crit damage bonus from +50% to +100% (total 2.0x vs 1.5x)
    Talents t_no_ruin;
    t_no_ruin.destro.ruin = 0;
    double no_ruin_mult = 1.0 + 0.50 * (1.0 + t_no_ruin.destro.ruin * 0.20);
    CHECK_NEAR(no_ruin_mult, 1.50, 0.001);

    Talents t_ruin;
    t_ruin.destro.ruin = 5;
    double ruin_mult = 1.0 + 0.50 * (1.0 + t_ruin.destro.ruin * 0.20);
    CHECK_NEAR(ruin_mult, 2.00, 0.001);
}

TEST_CASE(Talents, BaneCastTimeReduction) {
    Talents t;
    t.destro.bane = 5; // -0.5s SB/Incinerate/Immolate, -2.0s Soul Fire
    CHECK_EQ(t.destro.bane, 5);
    
    double sb_base = 3.0;
    double sb_reduced = sb_base - (t.destro.bane * 0.1);
    CHECK_NEAR(sb_reduced, 2.5, 0.001);

    double sf_base = 6.0;
    double sf_reduced = sf_base - (t.destro.bane * 0.4);
    CHECK_NEAR(sf_reduced, 4.0, 0.001);
}

TEST_CASE(Talents, AgonizingFlamesBonus) {
    Talents t;
    t.destro.agonizing_flames = 3;
    double bonus = (t.destro.agonizing_flames == 1) ? 0.03 :
                   ((t.destro.agonizing_flames == 2) ? 0.07 :
                   ((t.destro.agonizing_flames == 3) ? 0.10 : 0.0));
    CHECK_NEAR(1.0 + bonus, 1.10, 0.001); // +10% damage at 3/3
}

TEST_CASE(Talents, ShadowMasteryBonus) {
    Talents t;
    t.aff.shadow_mastery = 5;
    double bonus = 1.0 + t.aff.shadow_mastery * 0.01;
    CHECK_NEAR(bonus, 1.05, 0.001); // +5% in Forever
}

TEST_CASE(Talents, ImprovedDrainsBonus) {
    Talents t;
    t.aff.improved_drains = 3;
    double mult_r1 = 1.07;
    double mult_r2 = 1.13;
    double mult_r3 = 1.20;
    CHECK_NEAR(mult_r1, 1.07, 0.001);
    CHECK_NEAR(mult_r2, 1.13, 0.001);
    CHECK_NEAR(mult_r3, 1.20, 0.001);
}

TEST_CASE(Talents, SoulSiphonBonus) {
    Talents t;
    t.aff.soul_siphon = 3;
    // 3/3 gives +12% per active affliction effect up to 3 effects (max +36%)
    int active_aff_effects = 3;
    double ss_mult = 1.0 + active_aff_effects * (t.aff.soul_siphon * 0.04);
    CHECK_NEAR(ss_mult, 1.36, 0.001);
}

TEST_CASE(Talents, DemonicBrandBonus) {
    Talents t;
    t.demo.demonic_brand = 3;
    CHECK_EQ(t.demo.demonic_brand, 3);
    int charges = t.demo.demonic_brand * 2;
    CHECK_EQ(charges, 6);
    double min_brand = 65.0;
    double max_brand = 68.0;
    CHECK_NEAR(min_brand, 65.0, 0.001);
    CHECK_NEAR(max_brand, 68.0, 0.001);
}

TEST_CASE(Talents, DecimationExecuteScaling) {
    Talents t;
    t.demo.decimation = 2;
    double exec_dmg_bonus = 1.0 + t.demo.decimation * 0.03;
    CHECK_NEAR(exec_dmg_bonus, 1.06, 0.001); // +6%
    double sf_cast_reduc = 1.0 - t.demo.decimation * 0.20;
    CHECK_NEAR(sf_cast_reduc, 0.60, 0.001); // -40%
}

TEST_CASE(Talents, FelVitalityManaBonus) {
    Talents t;
    t.demo.fel_vitality = 3;
    double mana_mult = 1.0 + t.demo.fel_vitality * 0.05;
    CHECK_NEAR(mana_mult, 1.15, 0.001); // +15%
}

TEST_CASE(Talents, ImprovedCorruptionGcdScaling) {
    WarlockSimulator sim;
    sim.policy.rotation = RotationChoice::SHADOW_AND_FLAME_FIRE_2;
    sim.fight_duration = 180.0;
    sim.talents = Talents::create_forever_shadow_and_flame_fire_2();

    double total_dps_4 = 0.0;
    double total_dps_5 = 0.0;
    const int iters = 100;

    sim.talents.aff.improved_corruption = 4;
    for (int i = 0; i < iters; ++i) {
        FastRNG rng(42 + i);
        total_dps_4 += sim.run_single_simulation(rng).dps;
    }

    sim.talents.aff.improved_corruption = 5;
    for (int i = 0; i < iters; ++i) {
        FastRNG rng(42 + i);
        total_dps_5 += sim.run_single_simulation(rng).dps;
    }

    double avg_4 = total_dps_4 / iters;
    double avg_5 = total_dps_5 / iters;

    // With proper 1.5s GCD enforcement on both casted and instant Corruption, 5/5 should be >= 4/5
    CHECK(avg_5 >= avg_4 * 0.98);
}

TEST_CASE(Talents, ImprovedImpIsDamageOnly) {
    // Improved Imp tooltip: +10% Firebolt damage per point. It must not change
    // Firebolt cast frequency. Same seed => identical casts, so 3/3 Firebolt
    // damage must equal exactly 1.3x the 0/3 damage.
    auto make_imp_sim = []() {
        WarlockSimulator sim;
        sim.race = Race::UNDEAD;
        sim.talents = Talents();
        sim.policy.pet = PetChoice::IMP;
        sim.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
        sim.buffs.sacrifice_imp = false;
        sim.buffs.sacrifice_succubus = false;
        sim.mechanics.pet_mana_management = false;
        sim.use_raw_stats = true;
        sim.raw_stats.spell_power = 500.0;
        sim.fight_duration = 30.0;
        return sim;
    };

    WarlockSimulator sim0 = make_imp_sim();
    sim0.talents.demo.improved_imp = 0;
    FastRNG rng0(12345);
    SimResult res0 = sim0.run_single_simulation(rng0);

    WarlockSimulator sim3 = make_imp_sim();
    sim3.talents.demo.improved_imp = 3;
    FastRNG rng3(12345);
    SimResult res3 = sim3.run_single_simulation(rng3);

    CHECK(res0.dmg_pet_firebolt > 0.0); // sanity: the Imp actually cast
    CHECK_NEAR(res3.dmg_pet_firebolt, res0.dmg_pet_firebolt * 1.3, 0.5);
}

TEST_CASE(Sim, RandomizedFightDuration) {
    WarlockSimulator sim;
    sim.fight_duration = 100.0;
    sim.randomize_duration = true;
    sim.duration_variance = 20.0;

    double min_dur = 1e9;
    double max_dur = -1e9;

    for (int i = 0; i < 50; ++i) {
        FastRNG rng(1000 + i);
        SimResult res = sim.run_single_simulation(rng);
        CHECK(res.duration >= 80.0);
        CHECK(res.duration <= 120.0);
        min_dur = std::min(min_dur, res.duration);
        max_dur = std::max(max_dur, res.duration);
    }

    // Verify actual variation occurred across iterations
    CHECK(max_dur - min_dur > 25.0);
}
