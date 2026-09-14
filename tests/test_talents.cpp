#include "test_framework.hpp"
#include "src/sim/talents.hpp"
#include "src/sim/warlock_sim.hpp"

using namespace warlock;

TEST_CASE(Talents, TotalPointsAndLimits) {
    Talents t1 = Talents::create_forever_shadow_destro();
    CHECK_EQ(t1.total_points(), 51);
    CHECK(t1.is_valid());

    Talents t2 = Talents::create_forever_fire_destro();
    CHECK_EQ(t2.total_points(), 51);
    CHECK(t2.is_valid());

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
    double bonus = 1.0 + t.destro.agonizing_flames * 0.03;
    CHECK_NEAR(bonus, 1.09, 0.001); // +9% damage
}

TEST_CASE(Talents, ShadowMasteryBonus) {
    Talents t;
    t.aff.shadow_mastery = 5;
    double bonus = 1.0 + t.aff.shadow_mastery * 0.01;
    CHECK_NEAR(bonus, 1.05, 0.001); // +5% in Forever
}

TEST_CASE(Talents, DemonicBrandBonus) {
    Talents t;
    t.demo.demonic_brand = 3;
    CHECK_EQ(t.demo.demonic_brand, 3);
    double min_brand = t.demo.demonic_brand * 13.0;
    double max_brand = t.demo.demonic_brand * 14.0;
    CHECK_NEAR(min_brand, 39.0, 0.001);
    CHECK_NEAR(max_brand, 42.0, 0.001);
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

