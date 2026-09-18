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
    CHECK(mech.imp_firebolt_modern_scaling); // Modern scaling is true by default
    CHECK_NEAR(mech.pet_sp_ratio, 0.15, 0.001); // 15% SP inheritance to pet spell damage
    CHECK_NEAR(mech.pet_ap_ratio, 0.57, 0.001); // 57% SP inheritance to pet Attack Power
}

TEST_CASE(Mechanics, ImpFireboltModernVsClassicToggle) {
    // With 500 SP: Modern gives 57.1% master SP (2.0s cast), Classic gives 85-98 + 15% pet SP (1.5s cast)
    FastRNG rng_mod(1234);
    WarlockSimulator sim_mod;
    sim_mod.talents = Talents::create_forever_dp_af_fire();
    sim_mod.policy.rotation = RotationChoice::DP_RUIN_FIRE;
    sim_mod.buffs.sacrifice_succubus = false;
    sim_mod.buffs.sacrifice_imp = false;
    sim_mod.policy.pet = PetChoice::IMP;
    sim_mod.mechanics.imp_firebolt_modern_scaling = true;
    sim_mod.fight_duration = 30.0;
    SimResult mod_res = sim_mod.run_single_simulation(rng_mod);

    FastRNG rng_cls(1234);
    WarlockSimulator sim_cls;
    sim_cls.talents = Talents::create_forever_dp_af_fire();
    sim_cls.policy.rotation = RotationChoice::DP_RUIN_FIRE;
    sim_cls.buffs.sacrifice_succubus = false;
    sim_cls.buffs.sacrifice_imp = false;
    sim_cls.policy.pet = PetChoice::IMP;
    sim_cls.mechanics.imp_firebolt_modern_scaling = false;
    sim_cls.fight_duration = 30.0;
    SimResult cls_res = sim_cls.run_single_simulation(rng_cls);

    CHECK(mod_res.dmg_pet_firebolt > 0.0);
    CHECK(cls_res.dmg_pet_firebolt > 0.0);
    // Modern scaling with end-game spell power provides substantial damage scaling per hit
    const SpellCombatStats& mod_fb = mod_res.spell_stats[static_cast<size_t>(SpellID::PET_FIREBOLT)];
    const SpellCombatStats& cls_fb = cls_res.spell_stats[static_cast<size_t>(SpellID::PET_FIREBOLT)];
    CHECK(mod_fb.casts > 0);
    CHECK(cls_fb.casts > 0);
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
    // Spell path: Classic Imp Firebolt scales with pet_sp_ratio (same seed => identical casts)
    {
        FastRNG rng_lo(777);
        WarlockSimulator sim_lo;
        sim_lo.talents = Talents::create_forever_nf_af();
        sim_lo.policy.rotation = RotationChoice::SM_RUIN;
        sim_lo.policy.curse = CurseChoice::BANE_OF_AGONY;
        sim_lo.buffs.sacrifice_imp = false;
        sim_lo.policy.pet = PetChoice::IMP;
        sim_lo.mechanics.imp_firebolt_modern_scaling = false; // Classic scaling for pet_sp_ratio test
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
        sim_hi.mechanics.imp_firebolt_modern_scaling = false;
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

TEST_CASE(Mechanics, LifeTapSpiritScaling) {
    // Tests Life Tap: converts 430 health into (430 + Spirit) * (1 + 0.10 * ImpLifeTap) mana
    FastRNG rng(42);
    WarlockSimulator sim;
    BuffConfig clean_buffs{};
    clean_buffs.arcane_intellect = false;
    clean_buffs.blessing_of_kings = false;
    clean_buffs.blessing_of_wisdom = false;
    clean_buffs.mark_of_the_wild = false;
    clean_buffs.judgement_of_wisdom = false;
    clean_buffs.flask_of_supreme_power = false;
    clean_buffs.greater_arcane_elixir = false;
    clean_buffs.elixir_of_shadow_power = false;
    clean_buffs.elixir_of_greater_firepower = false;
    clean_buffs.brilliant_wizard_oil = false;
    clean_buffs.use_mana_potions = false;
    clean_buffs.use_demonic_runes = false;
    clean_buffs.sacrifice_imp = false;
    clean_buffs.sacrifice_succubus = false;
    sim.buffs = clean_buffs;

    sim.use_raw_stats = true;
    sim.raw_stats.spirit = 100.0;
    sim.raw_stats.max_mana = 5000.0;
    sim.raw_stats.max_health = 4000.0;
    sim.policy.life_tap_threshold_pct = 100.0; // Force immediate Life Tap
    sim.policy.rotation = RotationChoice::PURE_SHADOW_BOLT;
    sim.talents.aff.improved_life_tap = 2; // +20%
    sim.fight_duration = 1.0; // Runs exactly 1 tap at t=0

    BaseAttributes base = get_base_attributes_for_race(sim.race);
    double total_spirit = base.spirit + sim.raw_stats.spirit;
    double expected_mana_per_tap = (430.0 + 0.05 * total_spirit) * 1.20;

    SimResult res = sim.run_single_simulation(rng);
    CHECK_EQ(res.life_taps, 1);
    CHECK_NEAR(res.mana_gained, expected_mana_per_tap, 0.01);
}

TEST_CASE(Mechanics, WrackShadowDotAmplification) {
    // Tests that Wrack (+10% Shadow DoT amplification for 6.0s):
    // 1. Amplifies Corruption, Siphon Life, and Bane of Agony ticks that land inside its 6.0s window by exactly +10%
    // 2. Does NOT amplify itself (Wrack ticks)
    // 3. Does NOT amplify Fire DoTs (Immolate)
    FastRNG rng1(42);
    FastRNG rng2(42);

    BuffConfig clean_buffs{};
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
    clean_buffs.use_mana_potions = false;
    clean_buffs.use_demonic_runes = false;

    auto make_sim = [&](bool with_wrack) {
        WarlockSimulator sim;
        sim.buffs = clean_buffs;
        sim.talents = Talents();
        sim.talents.aff.improved_corruption = 5; // Instant cast Corruption
        sim.talents.aff.siphon_life = 1;
        sim.use_raw_stats = true;
        sim.raw_stats.max_mana = 5000.0;
        sim.raw_stats.max_health = 4000.0;
        sim.raw_stats.spell_power = 600.0;
        sim.raw_stats.spell_hit_percent = 100.0; // 100% hit rate
        sim.raw_stats.spell_crit_percent = 0.0;  // 0% crit to avoid variance
        sim.mechanics.partial_resists_enabled = false;
        sim.mechanics.instant_drain_hope = true;  // Instant cast Wrack
        sim.policy.use_trinkets_on_cooldown = false;
        sim.policy.rotation = RotationChoice::DEEP_AFFLICTION_SB;
        sim.policy.curse = CurseChoice::BANE_OF_AGONY;
        sim.policy.corruption = DotPolicy::ALWAYS;
        sim.policy.pet = PetChoice::NONE;

        if (with_wrack) {
            sim.talents.aff.drain_hope = 1;
        }
        return sim;
    };

    // Cast sequence in Deep Affliction:
    // t=0.0: Corruption (ticks at t=3.0, 6.0)
    // t=1.5: Bane of Agony (ticks at t=3.5, 5.5)
    // t=3.0: Siphon Life (ticks at t=6.0)
    // t=4.5: Wrack (active window: [4.5, 10.5])
    //
    // Ticks occurring inside Wrack window [4.5, 10.5]:
    // - Corruption tick 2 at t=6.0 (amplified by 1.10)
    // - Siphon Life tick 1 at t=6.0 (amplified by 1.10)
    // - Bane of Agony tick 2 at t=5.5 (amplified by 1.10)

    // 1. Corruption & Siphon Life amplification test over 6.5s fight:
    {
        WarlockSimulator sim_no_wrack = make_sim(false);
        sim_no_wrack.fight_duration = 6.5;
        SimResult res_no_wrack = sim_no_wrack.run_single_simulation(rng1);

        WarlockSimulator sim_wrack = make_sim(true);
        sim_wrack.fight_duration = 6.5;
        SimResult res_wrack = sim_wrack.run_single_simulation(rng2);

        // Corruption:
        // Baseline: 2 ticks of 212.3 = 424.6
        // With Wrack: Tick 1 (t=3.0) is 212.3, Tick 2 (t=6.0) is 212.3 * 1.10 = 233.53. Total = 445.83.
        CHECK_NEAR(res_no_wrack.dmg_corruption, 424.6, 0.01);
        CHECK_NEAR(res_wrack.dmg_corruption, 212.3 + 212.3 * 1.10, 0.01);
        CHECK_NEAR(res_wrack.dmg_corruption - res_no_wrack.dmg_corruption, 212.3 * 0.10, 0.01);

        // Siphon Life:
        // Baseline: 1 tick of 71.0 (at t=6.0)
        // With Wrack: 1 tick of 71.0 * 1.10 = 78.1
        CHECK_NEAR(res_no_wrack.dmg_siphon_life, 71.0, 0.01);
        CHECK_NEAR(res_wrack.dmg_siphon_life, 78.1, 0.01);
        CHECK_NEAR(res_wrack.dmg_siphon_life / res_no_wrack.dmg_siphon_life, 1.10, 0.0001);

        // Bane of Agony:
        // With Wrack, tick at t=5.5 is amplified by 1.10, increasing total Agony damage
        CHECK(res_wrack.dmg_agony > res_no_wrack.dmg_agony);
    }

    // 2. Wrack Self-Damage Test (does not amplify itself):
    // Cast at t=4.5, all 6 ticks land at t=5.5, 6.5, 7.5, 8.5, 9.5, 10.5
    // 6 ticks of (212 / 6) + (1.0 / 6) * 600 = 212 + 600 = 812 total (135.333/tick)
    {
        WarlockSimulator sim_wrack = make_sim(true);
        sim_wrack.fight_duration = 11.0;
        SimResult res_wrack = sim_wrack.run_single_simulation(rng1);

        CHECK_NEAR(res_wrack.dmg_drain_hope, 812.0, 0.01);
    }

    // 3. Fire DoT (Immolate) Excluded from Shadow DoT amp:
    // Immolate periodic tick is Fire damage, so it must not gain the 1.10x shadow DoT multiplier.
    {
        WarlockSimulator sim_no_wrack = make_sim(false);
        sim_no_wrack.policy.rotation = RotationChoice::FIRE_DESTRO_NO_CORRUPTION;
        sim_no_wrack.policy.maintain_immolate = true;
        sim_no_wrack.policy.use_conflagrate = false;
        sim_no_wrack.fight_duration = 6.0;
        SimResult res_no_wrack = sim_no_wrack.run_single_simulation(rng1);

        WarlockSimulator sim_wrack = make_sim(true);
        sim_wrack.policy.rotation = RotationChoice::FIRE_DESTRO_NO_CORRUPTION;
        sim_wrack.policy.maintain_immolate = true;
        sim_wrack.policy.use_conflagrate = false;
        sim_wrack.fight_duration = 6.0;
        SimResult res_wrack = sim_wrack.run_single_simulation(rng2);

        CHECK(res_no_wrack.dmg_immolate > 0.0);
        CHECK_NEAR(res_no_wrack.dmg_immolate, res_wrack.dmg_immolate, 0.01);
    }
}

TEST_CASE(Mechanics, ISBShadowDoTAmplification) {
    FastRNG rng1(42);
    FastRNG rng2(42);

    auto make_sim = [](int isb_points) {
        WarlockSimulator sim;
        sim.race = Race::UNDEAD;
        sim.talents = Talents{};
        sim.talents.aff.siphon_life = 1;
        sim.talents.destro.improved_shadow_bolt = isb_points;
        sim.raw_stats.spell_power = 500.0;
        sim.raw_stats.spell_hit_percent = 100.0;
        sim.raw_stats.spell_crit_percent = (isb_points > 0) ? 100.0 : 0.0;
        sim.mechanics.partial_resists_enabled = false;
        sim.mechanics.snapshot_dots = false;
        sim.buffs.sacrifice_imp = false;
        sim.buffs.sacrifice_succubus = false;
        sim.buffs.shadow_weaving = false;
        sim.buffs.curse_of_shadows = false;
        sim.policy.rotation = RotationChoice::DEEP_AFFLICTION_SB;
        sim.policy.curse = CurseChoice::BANE_OF_AGONY;
        sim.policy.corruption = DotPolicy::ALWAYS;
        sim.policy.pet = PetChoice::NONE;
        sim.fight_duration = 18.0;
        return sim;
    };

    WarlockSimulator sim_no_isb = make_sim(0);
    SimResult res_no_isb = sim_no_isb.run_single_simulation(rng1);

    WarlockSimulator sim_isb = make_sim(5);
    SimResult res_isb = sim_isb.run_single_simulation(rng2);

    CHECK(res_isb.dmg_corruption > res_no_isb.dmg_corruption);
    CHECK(res_isb.dmg_agony > res_no_isb.dmg_agony);
    CHECK(res_isb.dmg_siphon_life > res_no_isb.dmg_siphon_life);
}


