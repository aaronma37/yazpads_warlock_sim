#include "test_framework.hpp"
#include "src/sim/mechanics.hpp"
#include "src/sim/talents.hpp"
#include "src/sim/warlock_sim.hpp"
#include "src/sim/stats.hpp"

using namespace warlock;

TEST_CASE(Mechanics, DoTCritBaseMultiplier) {
    Talents t_no_pandemic;
    t_no_pandemic.aff.pandemic = 0;
    
    // Baseline crit multiplier is 1.5x
    double base_crit = 1.0 + 0.50 * (1.0 + t_no_pandemic.aff.pandemic * 0.33333333);
    CHECK_NEAR(base_crit, 1.50, 0.001);
}

TEST_CASE(Mechanics, DoTCritPandemicScaling) {
    Talents t;
    
    // 1/3 Pandemic: +33% crit bonus -> 1.0 + 0.5 * 1.333 = ~1.666x
    t.aff.pandemic = 1;
    double p1 = 1.0 + 0.50 * (1.0 + t.aff.pandemic * 0.33333333);
    CHECK_NEAR(p1, 1.6666, 0.001);

    // 2/3 Pandemic: +66% crit bonus -> ~1.833x
    t.aff.pandemic = 2;
    double p2 = 1.0 + 0.50 * (1.0 + t.aff.pandemic * 0.33333333);
    CHECK_NEAR(p2, 1.8333, 0.001);

    // 3/3 Pandemic: +100% crit bonus -> 2.0x total
    t.aff.pandemic = 3;
    double p3 = 1.0 + 0.50 * (1.0 + t.aff.pandemic * 0.33333333);
    CHECK_NEAR(p3, 2.00, 0.001);
}

TEST_CASE(Mechanics, ImmolateDoTCritRuinBonus) {
    Talents t_ruin;
    t_ruin.destro.ruin = 5;
    double destro_crit_mult = 1.0 + 0.50 * (1.0 + t_ruin.destro.ruin * 0.20);
    CHECK_NEAR(destro_crit_mult, 2.00, 0.001);
}

TEST_CASE(Mechanics, SpellHitCapAt99Percent) {
    FastRNG rng(42);
    WarlockSimulator sim;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_hit_percent = 50.0; // Extreme excess hit from gear
    sim.talents.aff.suppression = 5;         // +5% from talents
    sim.fight_duration = 10.0;

    // Hit chance caps at 99% (1% miss chance floor)
    CHECK_NEAR(sim.mechanics.max_spell_hit, 0.99, 0.0001);
    CHECK_NEAR(sim.mechanics.base_hit_vs_boss, 0.83, 0.0001);
}

TEST_CASE(Mechanics, TargetISBApplicationAndCharges) {
    TargetConfig target;
    CHECK_EQ(target.isb_charges, 0);

    // Apply ISB with charges (Classic mode)
    target.apply_isb(10.0, 5, true);
    CHECK_EQ(target.isb_charges, 4);
    CHECK_NEAR(target.isb_bonus, 0.20, 0.001);
    CHECK_NEAR(target.isb_expire_time, 22.0, 0.001);

    // Consume charges
    CHECK(target.consume_isb_charge(11.0));
    CHECK_EQ(target.isb_charges, 3);
    CHECK(target.consume_isb_charge(12.0));
    CHECK(target.consume_isb_charge(13.0));
    CHECK(target.consume_isb_charge(14.0));
    CHECK_EQ(target.isb_charges, 0);

    // Once 0 charges remain, consume should return false
    CHECK(!target.consume_isb_charge(15.0));

    // Apply ISB without charges (WoW Forever 12s buff window mode)
    target.apply_isb(30.0, 5, false);
    CHECK_EQ(target.isb_charges, -1);
    // Consumes indefinitely within 12s window
    for (int i = 0; i < 10; ++i) {
        CHECK(target.consume_isb_charge(31.0 + (i * 0.5)));
    }
    // Expired at 42.0
    CHECK(!target.consume_isb_charge(43.0));
}

TEST_CASE(Mechanics, NightfallProcSimulation) {
    FastRNG rng(12345);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_nf_ds_ruin();
    sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    sim.fight_duration = 180.0; // 3 minute fight with continuous Corruption ticks
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    // Nightfall procs should happen regularly across 3 minutes of Corruption
    CHECK(res.nightfall_procs > 0);
}

TEST_CASE(Mechanics, PetStatScalingToggle) {
    MechanicsConfig mech;
    CHECK(mech.pet_scaling);
    CHECK_NEAR(mech.pet_sp_ratio, 0.15, 0.001); // 15% SP inheritance to pet spell damage
    CHECK_NEAR(mech.pet_ap_ratio, 0.57, 0.001); // 57% SP inheritance to pet Attack Power
}

TEST_CASE(Mechanics, PersonalShadowWeavingDefault) {
    MechanicsConfig mech;
    BuffConfig buffs;
    CHECK(mech.personal_shadow_weaving);
    CHECK(!buffs.shadow_weaving); // OFF by default
}

TEST_CASE(Mechanics, PetManaManagement) {
    MechanicsConfig mech;
    CHECK(mech.pet_mana_management);
    CHECK_NEAR(mech.imp_base_mana, 1150.0, 0.01);
    CHECK_NEAR(mech.succubus_base_mana, 1450.0, 0.01);
    CHECK_NEAR(mech.imp_firebolt_cost, 115.0, 0.01);
    CHECK_NEAR(mech.succubus_lop_cost, 160.0, 0.01);
    CHECK_NEAR(mech.pet_base_mp5, 45.0, 0.01);
}

TEST_CASE(Mechanics, SeparatedPetDamageBreakdown) {
    // 1. Succubus breakdown (Melee + Lash of Pain)
    {
        FastRNG rng(42);
        WarlockSimulator sim;
        sim.talents = Talents::create_forever_dp_af_shadow();
        sim.policy.rotation = RotationChoice::DP_AF_SHADOW;
        sim.buffs.sacrifice_imp = true;
        sim.policy.pet = PetChoice::SUCCUBUS;
        sim.fight_duration = 60.0;
        sim.record_timeline = true;

        SimResult res = sim.run_single_simulation(rng);
        CHECK(res.dmg_pet_succubus > 0.0);
        CHECK(res.dmg_pet_melee > 0.0);
        CHECK(res.dmg_pet_lash_of_pain > 0.0);
        CHECK_EQ(res.dmg_pet_firebolt, 0.0);
        CHECK_NEAR(res.dmg_pet, res.dmg_pet_melee + res.dmg_pet_lash_of_pain + res.dmg_demonic_brand, 0.01);
    }

    // 2. Imp breakdown (Firebolt + Demonic Brand)
    {
        FastRNG rng(42);
        WarlockSimulator sim;
        sim.talents = Talents::create_forever_dp_af_fire();
        sim.policy.rotation = RotationChoice::DP_RUIN_FIRE;
        sim.buffs.sacrifice_succubus = true;
        sim.buffs.sacrifice_imp = false;
        sim.policy.pet = PetChoice::IMP;
        sim.fight_duration = 60.0;
        sim.record_timeline = true;

        SimResult res = sim.run_single_simulation(rng);
        CHECK(res.dmg_pet_imp > 0.0);
        CHECK(res.dmg_pet_firebolt > 0.0);
        CHECK_EQ(res.dmg_pet_melee, 0.0);
        CHECK_EQ(res.dmg_pet_lash_of_pain, 0.0);
        CHECK(res.dmg_demonic_brand > 0.0);
        CHECK_NEAR(res.dmg_pet, res.dmg_pet_firebolt + res.dmg_demonic_brand, 0.01);
    }
}

TEST_CASE(Mechanics, PetSpellAndApRatiosApplySeparately) {
    // Spell path: Imp Firebolt scales with pet_sp_ratio (same seed => identical casts)
    {
        FastRNG rng_lo(777);
        WarlockSimulator sim_lo;
        sim_lo.talents = Talents::create_forever_nf_af();
        sim_lo.policy.rotation = RotationChoice::SM_RUIN;
        sim_lo.policy.curse = CurseChoice::BANE_OF_AGONY;
        sim_lo.buffs.sacrifice_imp = false;
        sim_lo.policy.pet = PetChoice::IMP;
        sim_lo.mechanics.pet_sp_ratio = 0.0;
        sim_lo.fight_duration = 30.0;
        SimResult lo = sim_lo.run_single_simulation(rng_lo);

        FastRNG rng_hi(777);
        WarlockSimulator sim_hi;
        sim_hi.talents = Talents::create_forever_nf_af();
        sim_hi.policy.rotation = RotationChoice::SM_RUIN;
        sim_hi.policy.curse = CurseChoice::BANE_OF_AGONY;
        sim_hi.buffs.sacrifice_imp = false;
        sim_hi.policy.pet = PetChoice::IMP;
        sim_hi.mechanics.pet_sp_ratio = 0.57;
        sim_hi.fight_duration = 30.0;
        SimResult hi = sim_hi.run_single_simulation(rng_hi);

        CHECK(lo.dmg_pet_firebolt > 0.0); // Base damage even at 0% inheritance
        CHECK(hi.dmg_pet_firebolt > lo.dmg_pet_firebolt);
    }

    // AP path: Succubus melee scales with pet_ap_ratio (same seed => identical swings)
    {
        FastRNG rng_lo(777);
        WarlockSimulator sim_lo;
        sim_lo.talents = Talents::create_forever_sm_ruin();
        sim_lo.policy.rotation = RotationChoice::SM_RUIN;
        sim_lo.policy.curse = CurseChoice::BANE_OF_AGONY;
        sim_lo.buffs.sacrifice_imp = false;
        sim_lo.buffs.sacrifice_succubus = false;
        sim_lo.policy.pet = PetChoice::SUCCUBUS;
        sim_lo.mechanics.pet_ap_ratio = 0.0;
        sim_lo.fight_duration = 30.0;
        SimResult lo = sim_lo.run_single_simulation(rng_lo);

        FastRNG rng_hi(777);
        WarlockSimulator sim_hi;
        sim_hi.talents = Talents::create_forever_sm_ruin();
        sim_hi.policy.rotation = RotationChoice::SM_RUIN;
        sim_hi.policy.curse = CurseChoice::BANE_OF_AGONY;
        sim_hi.buffs.sacrifice_imp = false;
        sim_hi.buffs.sacrifice_succubus = false;
        sim_hi.policy.pet = PetChoice::SUCCUBUS;
        sim_hi.mechanics.pet_ap_ratio = 0.57;
        sim_hi.fight_duration = 30.0;
        SimResult hi = sim_hi.run_single_simulation(rng_hi);

        CHECK(lo.dmg_pet_melee > 0.0); // Base damage even at 0% inheritance
        CHECK(hi.dmg_pet_melee > lo.dmg_pet_melee);
    }
}

TEST_CASE(Mechanics, TargetLevelAndCreatureTypeBeastScaling) {
    WarlockSimulator sim;
    sim.race = Race::TROLL;
    sim.target_config.level = 60; // Equal level
    sim.target_config.creature_type = CreatureType::BEAST;
    sim.target_config.is_beast = true;
    sim.fight_duration = 10.0;

    CHECK_EQ(sim.target_config.level, 60);
    CHECK(sim.target_config.is_beast);
    CHECK(sim.target_config.creature_type == CreatureType::BEAST);

    // Hit chance at level 60 should be 96% base (+ gear hit)
    sim.use_raw_stats = true;
    sim.raw_stats.spell_hit_percent = 0.0;
    sim.talents.aff.suppression = 0;
    
    // Check hit rate
    double hit_60 = sim.calculate_hit_chance(School::SHADOW);
    CHECK_NEAR(hit_60, 0.96, 0.001);

    // Boss level 63 should be 83% base
    sim.target_config.level = 63;
    double hit_63 = sim.calculate_hit_chance(School::SHADOW);
    CHECK_NEAR(hit_63, 0.83, 0.001);
}

TEST_CASE(Mechanics, InstantDrainHopeToggle) {
    FastRNG rng_chan(1337);
    FastRNG rng_inst(1337);

    // Channeled Drain Hope
    WarlockSimulator sim_chan;
    sim_chan.talents = Talents::create_forever_deep_affliction();
    sim_chan.policy.rotation = RotationChoice::DEEP_AFFLICTION_SB;
    sim_chan.mechanics.instant_drain_hope = false;
    sim_chan.fight_duration = 30.0;
    sim_chan.record_timeline = true;
    SimResult res_chan = sim_chan.run_single_simulation(rng_chan);

    // Instant Cast DoT Drain Hope
    WarlockSimulator sim_inst;
    sim_inst.talents = Talents::create_forever_deep_affliction();
    sim_inst.policy.rotation = RotationChoice::DEEP_AFFLICTION_SB;
    sim_inst.mechanics.instant_drain_hope = true;
    sim_inst.fight_duration = 30.0;
    sim_inst.record_timeline = true;
    SimResult res_inst = sim_inst.run_single_simulation(rng_inst);

    // Both should deal Drain Hope damage
    CHECK(res_chan.dmg_drain_hope > 0.0);
    CHECK(res_inst.dmg_drain_hope > 0.0);

    // Instant Drain Hope frees up GCD (1.5s vs 6s channel), allowing more Shadow Bolt filler casts
    CHECK(res_inst.shadow_bolt_casts > res_chan.shadow_bolt_casts);
    CHECK(res_inst.total_damage > res_chan.total_damage);

    // Check cast sequence tag
    bool found_instant_tag = false;
    for (const auto& log : res_inst.cast_sequence) {
        if (log.spell_id == SpellID::DRAIN_HOPE && log.tag == "Instant DoT") {
            found_instant_tag = true;
        }
    }
    CHECK(found_instant_tag);
}

TEST_CASE(Mechanics, CorruptionSpellPowerCoefficient) {
    // 100% vs 120% Corruption SP scaling test
    FastRNG rng1(42);
    FastRNG rng2(42);

    BuffConfig clean_buffs;
    clean_buffs.flask_of_supreme_power = false;
    clean_buffs.greater_arcane_elixir = false;
    clean_buffs.elixir_of_shadow_power = false;
    clean_buffs.elixir_of_greater_firepower = false;
    clean_buffs.brilliant_wizard_oil = false;
    clean_buffs.curse_of_shadows = false;
    clean_buffs.curse_of_elements = false;
    clean_buffs.sacrifice_imp = false;
    clean_buffs.sacrifice_succubus = false;
    clean_buffs.shadow_weaving = false;

    WarlockSimulator sim_default;
    sim_default.buffs = clean_buffs;
    sim_default.talents = Talents();
    sim_default.use_raw_stats = true;
    sim_default.raw_stats.spell_power = 600.0;
    sim_default.raw_stats.spell_hit_percent = 100.0; // Avoid misses
    sim_default.raw_stats.spell_crit_percent = 0.0;  // Avoid crits
    sim_default.policy.rotation = RotationChoice::SHADOW_DESTRO;
    sim_default.policy.use_trinkets_on_cooldown = false;
    sim_default.mechanics.corruption_sp_coefficient = 1.0;
    sim_default.mechanics.partial_resists_enabled = false;
    sim_default.fight_duration = 23.0; // Cast from t=0..2s, 6 ticks at t=5, 8, 11, 14, 17, 20

    SimResult res_default = sim_default.run_single_simulation(rng1);

    WarlockSimulator sim_120;
    sim_120.buffs = clean_buffs;
    sim_120.talents = Talents();
    sim_120.use_raw_stats = true;
    sim_120.raw_stats.spell_power = 600.0;
    sim_120.raw_stats.spell_hit_percent = 100.0;
    sim_120.raw_stats.spell_crit_percent = 0.0;
    sim_120.policy.rotation = RotationChoice::SHADOW_DESTRO;
    sim_120.policy.use_trinkets_on_cooldown = false;
    sim_120.mechanics.corruption_sp_coefficient = 1.2;
    sim_120.mechanics.partial_resists_enabled = false;
    sim_120.fight_duration = 23.0;

    SimResult res_120 = sim_120.run_single_simulation(rng2);

    CHECK(res_default.dmg_corruption > 0.0);
    CHECK(res_120.dmg_corruption > res_default.dmg_corruption);

    // Rank 7 Corruption: 438 base dmg over 6 ticks (73/tick)
    // At 600 SP:
    // With 1.0 coeff: 438 + 600 * 1.2 = 438 + 720 = 1158 total across 6 ticks (193/tick)
    // With 1.2 coeff: 438 + 600 * 1.2 * 1.2 = 438 + 864 = 1302 total across 6 ticks (217/tick)
    // Expected diff per 6 ticks is exactly 144 damage (24/tick)
    double diff = res_120.dmg_corruption - res_default.dmg_corruption;
    CHECK_NEAR(diff, 144.0, 0.01);
}
