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
    // 5/11/35 DS/AF weaves Immolate and Conflagrate for +10% Shadow & Flame buff
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
}

TEST_CASE(Rotations, FireDestroDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_ds_incinerate();
    sim.policy.rotation = RotationChoice::FIRE_DESTRO;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    // Primary filler must be Incinerate, with Immolate, Conflagrate, and Corruption
    CHECK(res.dmg_incinerate > 0.0);
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
    CHECK(res.dmg_corruption > 0.0);
}

TEST_CASE(Rotations, ShadowAndFlameRotationExecution) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_and_flame();
    sim.policy.rotation = RotationChoice::FIRE_DESTRO;
    sim.policy.maintain_immolate = true;
    sim.policy.pet = PetChoice::IMP;
    sim.buffs.sacrifice_succubus = false;
    sim.buffs.sacrifice_imp = false;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    
    // Rotation must enforce maintaining Immolate, casting Conflagrate, weaving Shadowburn, and filling with Incinerate
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
    CHECK(res.dmg_shadowburn > 0.0);
    CHECK(res.dmg_incinerate > 0.0);
    CHECK(res.dmg_pet_firebolt > 0.0);
}

TEST_CASE(Rotations, ShadowAndFlameFire2RotationExecution) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_and_flame_fire_2();
    sim.policy.rotation = RotationChoice::SHADOW_AND_FLAME_FIRE_2;
    sim.policy.maintain_immolate = true;
    sim.policy.pet = PetChoice::IMP;
    sim.buffs.sacrifice_succubus = false;
    sim.buffs.sacrifice_imp = false;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    
    // Rotation must maintain Immolate, Conflagrate on CD, maintain Corruption, weave Shadowburn, fill with Incinerate
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_shadowburn > 0.0);
    CHECK(res.dmg_incinerate > 0.0);
    CHECK(res.dmg_pet_firebolt > 0.0);
    
    // Must NOT cast Searing Pain or Soul Fire
    CHECK_EQ(res.dmg_searing_pain, 0.0);
    CHECK_EQ(res.dmg_soul_fire, 0.0);
}

TEST_CASE(Rotations, ShadowAndFlameFireBaneRotationExecution) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_and_flame_fire_2();
    sim.policy.rotation = RotationChoice::SHADOW_AND_FLAME_FIRE_BANE;
    sim.policy.maintain_immolate = true;
    sim.policy.pet = PetChoice::IMP;
    sim.buffs.sacrifice_succubus = false;
    sim.buffs.sacrifice_imp = false;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    
    // Rotation must maintain Immolate, Conflagrate on CD, Corruption, Bane (Curse of Doom / Agony), Shadowburn, fill with Incinerate
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_curse > 0.0);
    CHECK(res.dmg_shadowburn > 0.0);
    CHECK(res.dmg_incinerate > 0.0);
    CHECK(res.dmg_pet_firebolt > 0.0);
    
    // Must NOT cast Searing Pain or Soul Fire
    CHECK_EQ(res.dmg_searing_pain, 0.0);
    CHECK_EQ(res.dmg_soul_fire, 0.0);
}

TEST_CASE(Rotations, ShadowAndFlameShadowRotationExecution) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_and_flame_shadow();
    sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    sim.policy.maintain_immolate = true;
    sim.policy.pet = PetChoice::IMP;
    sim.buffs.sacrifice_succubus = false;
    sim.buffs.sacrifice_imp = false;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    
    // Rotation must enforce maintaining Immolate, casting Conflagrate to proc +10% Shadow from Shadow & Flame, and filling with Shadow Bolt
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
    CHECK(res.dmg_shadow_bolt > 0.0);
    CHECK(res.shadow_bolt_casts > 0);
    CHECK(res.dmg_pet_firebolt > 0.0);
}

TEST_CASE(Rotations, DSSearingPainDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_ds_searing_pain();
    sim.policy.rotation = RotationChoice::FIRE_DESTRO;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    // Primary filler must be Searing Pain
    CHECK(res.dmg_searing_pain > 0.0);
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
    CHECK_EQ(res.dmg_incinerate, 0.0);
}

TEST_CASE(Rotations, DeepAfflictionDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_deep_affliction();
    sim.policy.rotation = RotationChoice::DEEP_AFFLICTION;
    sim.policy.curse = CurseChoice::BANE_OF_AGONY;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_curse > 0.0); // Bane of Agony
    CHECK(res.dmg_drain_hope > 0.0);
    CHECK(res.dmg_drain_soul > 0.0); // Drain Soul as filler
    CHECK_EQ(res.dmg_immolate, 0.0); // No Immolate in Deep Affliction
}

TEST_CASE(Rotations, DeepAfflictionSBFillerDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_deep_affliction();
    sim.policy.rotation = RotationChoice::DEEP_AFFLICTION_SB;
    sim.policy.curse = CurseChoice::BANE_OF_AGONY;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_curse > 0.0); // Bane of Agony
    CHECK(res.dmg_drain_hope > 0.0);
    CHECK_EQ(res.dmg_drain_soul, 0.0); // No Drain Soul
    CHECK(res.shadow_bolt_casts > 0);   // Shadow Bolt filler
    CHECK(res.dmg_shadow_bolt > 0.0);
    CHECK_EQ(res.dmg_immolate, 0.0);   // No Immolate
}

TEST_CASE(Rotations, DeepAfflictionSBFillerNoSLDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_deep_affliction();
    sim.policy.rotation = RotationChoice::DEEP_AFFLICTION_SB_NO_SL;
    sim.policy.curse = CurseChoice::BANE_OF_AGONY;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_curse > 0.0);        // Bane of Agony
    CHECK(res.dmg_drain_hope > 0.0);    // Wrack
    CHECK_EQ(res.dmg_siphon_life, 0.0); // No Siphon Life
    CHECK_EQ(res.dmg_drain_soul, 0.0);  // No Drain Soul
    CHECK(res.shadow_bolt_casts > 0);   // Shadow Bolt filler
    CHECK(res.dmg_shadow_bolt > 0.0);
    CHECK_EQ(res.dmg_immolate, 0.0);    // No Immolate
}

TEST_CASE(Rotations, SMRuinDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_sm_ruin();
    sim.policy.rotation = RotationChoice::SM_RUIN;
    sim.policy.curse = CurseChoice::BANE_OF_AGONY;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_curse > 0.0); // Bane of Agony
    CHECK(res.shadow_bolt_casts > 0);
    CHECK(res.dmg_shadowburn > 0.0);
    CHECK_EQ(res.dmg_immolate, 0.0); // No Immolate in SM/Ruin
}

TEST_CASE(Rotations, NFAfflictionUsesSMRuinRotation) {
    PolicyConfig policy;
    policy.rotation = RotationChoice::SM_RUIN;
    policy.curse = CurseChoice::BANE_OF_AGONY;
    Talents talents = Talents::create_forever_nf_af();
    auto rules = policy.get_priority_rules(talents);
    int pos_corr = -1, pos_agony = -1, pos_sb = -1, pos_sburn = -1;
    for (size_t i = 0; i < rules.size(); ++i) {
        if (rules[i].spell_id == SpellID::CORRUPTION) pos_corr = (int)i;
        if (rules[i].spell_id == SpellID::CURSE_OF_AGONY) pos_agony = (int)i;
        if (rules[i].spell_id == SpellID::SHADOW_BOLT) pos_sb = (int)i;
        if (rules[i].spell_id == SpellID::SHADOWBURN) pos_sburn = (int)i;
    }
    CHECK(pos_corr != -1);
    CHECK(pos_agony != -1);
    CHECK(pos_sb != -1);
    CHECK(pos_corr < pos_sb);
    CHECK(pos_agony < pos_sb);
    CHECK_EQ(pos_sburn, -1); // No Shadowburn point in 23/10/18, so no Shadowburn rule
    CHECK_EQ(static_cast<int>(rules.back().action), static_cast<int>(PriorityAction::SHADOW_BOLT_FILLER));

    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = talents;
    sim.policy.rotation = RotationChoice::SM_RUIN;
    sim.policy.curse = CurseChoice::BANE_OF_AGONY;
    sim.policy.pet = PetChoice::IMP;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_curse > 0.0); // Bane of Agony
    CHECK(res.shadow_bolt_casts > 0);
    CHECK_EQ(res.dmg_shadowburn, 0.0); // Untalented
    CHECK_EQ(res.dmg_immolate, 0.0); // No Immolate in SM/Ruin-style rotation
}

TEST_CASE(Rotations, DemonologyExecuteSoulFire) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_md_ruin();
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

TEST_CASE(Rotations, DPAFShadowFullDurationCorruptionAndAgony) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_dp_af_shadow();
    sim.policy.rotation = RotationChoice::DP_AF_SHADOW;
    sim.buffs.sacrifice_imp = true;
    sim.policy.pet = PetChoice::SUCCUBUS;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    // Verify talent setup: 0 points in improved_corruption (2.0s hardcast, standard duration)
    CHECK_EQ(sim.talents.aff.improved_corruption, 0);

    SimResult res = sim.run_single_simulation(rng);

    // Verify Corruption, Bane of Agony, and Shadow Bolt deal substantial damage
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.dmg_curse > 0.0); // Bane of Agony
    CHECK(res.dmg_agony > 0.0);
    CHECK(res.shadow_bolt_casts > 0);
    CHECK(res.dmg_shadow_bolt > 0.0);

    // Verify in cast sequence that Corruption was hardcast (cast_time >= 2.0s)
    bool found_hardcast_corruption = false;
    bool found_agony = false;
    for (const auto& cast : res.cast_sequence) {
        if (cast.spell_id == SpellID::CORRUPTION) {
            CHECK_NEAR(cast.cast_time, 2.0, 0.05);
            found_hardcast_corruption = true;
        } else if (cast.spell_id == SpellID::CURSE_OF_AGONY) {
            found_agony = true;
        }
    }
    CHECK(found_hardcast_corruption);
    CHECK(found_agony);
}

TEST_CASE(Rotations, DPAFShadowHasNoSearingPainTrigger) {
    // Shadow Bolt casts trigger Decimation in execute, so the SB-filler
    // DP/AF Shadow rotation needs no dedicated Searing Pain trigger cast —
    // but the Soul Fire execute must still be scheduled.
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_dp_af_shadow();
    sim.policy.rotation = RotationChoice::DP_AF_SHADOW;
    auto rules = sim.policy.get_priority_rules(sim.talents);
    bool has_sp_trigger = false;
    bool has_soul_fire = false;
    bool has_corruption = false;
    for (const auto& r : rules) {
        if (r.action == PriorityAction::DECIMATION_SEARING_PAIN) has_sp_trigger = true;
        if (r.action == PriorityAction::DECIMATION_SOUL_FIRE) has_soul_fire = true;
        if (r.action == PriorityAction::CORRUPTION) has_corruption = true;
    }
    CHECK(has_sp_trigger == false);
    CHECK(has_soul_fire == true);
    CHECK(has_corruption == true);
    CHECK_EQ(static_cast<int>(rules.back().action),
             static_cast<int>(PriorityAction::SHADOW_BOLT_FILLER));

    // Default behavior is preserved: sibling rotations still schedule the
    // Searing Pain trigger.
    sim.policy.rotation = RotationChoice::DP_AF_SHADOW_NO_CORRUPTION;
    auto nocorr_rules = sim.policy.get_priority_rules(sim.talents);
    bool nocorr_has_sp_trigger = false;
    for (const auto& r : nocorr_rules) {
        if (r.action == PriorityAction::DECIMATION_SEARING_PAIN) nocorr_has_sp_trigger = true;
    }
    CHECK(nocorr_has_sp_trigger == true);
}

TEST_CASE(Rotations, AfflictionMultiDotHybrid) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_deep_affliction();
    sim.policy.rotation = RotationChoice::AFFLICTION_HYBRID_DOTS;
    sim.policy.curse = CurseChoice::BANE_OF_AGONY;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.dmg_curse > 0.0);      // Agony
    CHECK(res.dmg_corruption > 0.0); // Corruption
    CHECK(res.dmg_immolate > 0.0);   // Immolate
    CHECK(res.dmg_drain_hope > 0.0); // Drain Hope
}

TEST_CASE(Rotations, DemoExecuteCorruptionCast) {
    FastRNG rng(1337);
    
    // In Demo Execute with 0/5 Imp Corruption (2.0s cast), Corruption must still be cast and ticked
    WarlockSimulator sim_demo;
    sim_demo.talents = Talents::create_forever_demonic_pact();
    sim_demo.policy.rotation = RotationChoice::DEMONOLOGY_EXECUTE;
    sim_demo.fight_duration = 45.0;
    sim_demo.record_timeline = true;
    SimResult res_demo = sim_demo.run_single_simulation(rng);
    CHECK(res_demo.dmg_corruption > 0.0);
    CHECK(res_demo.total_damage > 0.0);
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

TEST_CASE(Rotations, PriorityRuleChainGeneration) {
    PolicyConfig policy;
    Talents talents_destro = Talents::create_forever_shadow_destro();
    policy.rotation = RotationChoice::SHADOW_DESTRO;
    
    auto rules = policy.get_priority_rules(talents_destro);
    CHECK(rules.size() >= 5);
    
    // First rule must be Life Tap
    CHECK_EQ(static_cast<int>(rules[0].action), static_cast<int>(PriorityAction::LIFE_TAP));
    CHECK_EQ(static_cast<int>(rules[0].spell_id), static_cast<int>(SpellID::LIFE_TAP));
    
    // 5/11/35 Shadow Destro with Shadow & Flame weaves Immolate and Conflagrate
    bool has_corr = false, has_immo = false, has_conflag = false;
    for (const auto& r : rules) {
        if (r.spell_id == SpellID::CORRUPTION) has_corr = true;
        if (r.spell_id == SpellID::IMMOLATE) has_immo = true;
        if (r.spell_id == SpellID::CONFLAGRATE) has_conflag = true;
    }
    CHECK(has_corr);
    CHECK(has_immo);
    CHECK(has_conflag);

    // Last rule must be Shadow Bolt Filler
    CHECK_EQ(static_cast<int>(rules.back().action), static_cast<int>(PriorityAction::SHADOW_BOLT_FILLER));
    CHECK_EQ(static_cast<int>(rules.back().spell_id), static_cast<int>(SpellID::SHADOW_BOLT));

    // For Fire Destro: filler must be Incinerate
    policy.rotation = RotationChoice::FIRE_DESTRO;
    Talents talents_fire = Talents::create_forever_fire_destro();
    auto fire_rules = policy.get_priority_rules(talents_fire);
    CHECK_EQ(static_cast<int>(fire_rules.back().action), static_cast<int>(PriorityAction::INCINERATE_FILLER));
    // For DP Ruin Fire: filler must be Searing Pain
    policy.rotation = RotationChoice::DP_RUIN_FIRE;
    Talents talents_dp_fire = Talents::create_forever_demonic_pact_fire();
    auto dp_fire_rules = policy.get_priority_rules(talents_dp_fire);
    CHECK_EQ(static_cast<int>(dp_fire_rules.back().action), static_cast<int>(PriorityAction::SEARING_PAIN_FILLER));
    CHECK_EQ(static_cast<int>(dp_fire_rules.back().spell_id), static_cast<int>(SpellID::SEARING_PAIN));

    // For Deep Affliction: Corruption > Bane of Agony > Drain Hope > Drain Soul Filler
    policy.rotation = RotationChoice::DEEP_AFFLICTION;
    Talents talents_aff = Talents::create_forever_deep_affliction();
    auto aff_rules = policy.get_priority_rules(talents_aff);
    // Find positions of Corruption, Agony, Drain Hope, Drain Soul
    int pos_corr = -1, pos_agony = -1, pos_dh = -1, pos_ds = -1;
    for (size_t i = 0; i < aff_rules.size(); ++i) {
        if (aff_rules[i].spell_id == SpellID::CORRUPTION) pos_corr = (int)i;
        if (aff_rules[i].spell_id == SpellID::CURSE_OF_AGONY) pos_agony = (int)i;
        if (aff_rules[i].spell_id == SpellID::DRAIN_HOPE) pos_dh = (int)i;
        if (aff_rules[i].spell_id == SpellID::DRAIN_SOUL) pos_ds = (int)i;
    }
    CHECK(pos_corr != -1);
    CHECK(pos_agony != -1);
    CHECK(pos_dh != -1);
    CHECK(pos_ds != -1);
    CHECK(pos_corr < pos_dh);
    CHECK(pos_agony < pos_dh);
    CHECK(pos_dh < pos_ds);
    CHECK_EQ(static_cast<int>(aff_rules.back().action), static_cast<int>(PriorityAction::DRAIN_SOUL_FILLER));

    // For Deep Affliction (SB filler): Corruption > Bane of Agony > Drain Hope > SB Filler
    policy.rotation = RotationChoice::DEEP_AFFLICTION_SB;
    auto aff_sb_rules = policy.get_priority_rules(talents_aff);
    int pos_sb_corr = -1, pos_sb_agony = -1, pos_sb_dh = -1, pos_sb_filler = -1;
    for (size_t i = 0; i < aff_sb_rules.size(); ++i) {
        if (aff_sb_rules[i].spell_id == SpellID::CORRUPTION) pos_sb_corr = (int)i;
        if (aff_sb_rules[i].spell_id == SpellID::CURSE_OF_AGONY) pos_sb_agony = (int)i;
        if (aff_sb_rules[i].spell_id == SpellID::DRAIN_HOPE) pos_sb_dh = (int)i;
        if (aff_sb_rules[i].spell_id == SpellID::SHADOW_BOLT && aff_sb_rules[i].action == PriorityAction::SHADOW_BOLT_FILLER) pos_sb_filler = (int)i;
    }
    CHECK(pos_sb_corr != -1);
    CHECK(pos_sb_agony != -1);
    CHECK(pos_sb_dh != -1);
    CHECK(pos_sb_filler != -1);
    CHECK(pos_sb_corr < pos_sb_dh);
    CHECK(pos_sb_agony < pos_sb_dh);
    CHECK(pos_sb_dh < pos_sb_filler);
    CHECK_EQ(static_cast<int>(aff_sb_rules.back().action), static_cast<int>(PriorityAction::SHADOW_BOLT_FILLER));

    // For Deep Affliction (SB filler, No Siphon Life): Corruption > Bane of Agony > Drain Hope > SB Filler, no Siphon Life
    policy.rotation = RotationChoice::DEEP_AFFLICTION_SB_NO_SL;
    auto aff_sb_no_sl_rules = policy.get_priority_rules(talents_aff);
    bool has_sl = false;
    for (const auto& r : aff_sb_no_sl_rules) {
        if (r.spell_id == SpellID::SIPHON_LIFE) has_sl = true;
    }
    CHECK(has_sl == false);
    CHECK_EQ(static_cast<int>(aff_sb_no_sl_rules.back().action), static_cast<int>(PriorityAction::SHADOW_BOLT_FILLER));

    // For SM Ruin: Corruption > Bane of Agony > Shadowburn > SB Filler
    policy.rotation = RotationChoice::SM_RUIN;
    Talents talents_sm = Talents::create_forever_sm_ruin();
    auto sm_rules = policy.get_priority_rules(talents_sm);
    int sm_corr = -1, sm_agony = -1, sm_sb = -1;
    for (size_t i = 0; i < sm_rules.size(); ++i) {
        if (sm_rules[i].spell_id == SpellID::CORRUPTION) sm_corr = (int)i;
        if (sm_rules[i].spell_id == SpellID::CURSE_OF_AGONY) sm_agony = (int)i;
        if (sm_rules[i].spell_id == SpellID::SHADOW_BOLT) sm_sb = (int)i;
    }
    CHECK(sm_corr != -1);
    CHECK(sm_agony != -1);
    CHECK(sm_sb != -1);
    CHECK(sm_corr < sm_sb);
    CHECK(sm_agony < sm_sb);
}

TEST_CASE(Rotations, DPRuinFireDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_demonic_pact_fire();
    sim.buffs.sacrifice_succubus = true;
    sim.buffs.sacrifice_imp = false;
    sim.policy.pet = PetChoice::IMP;
    sim.policy.rotation = RotationChoice::DP_RUIN_FIRE;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    CHECK(res.dmg_searing_pain > 0.0);
    CHECK_EQ(res.shadow_bolt_casts, 0);
    CHECK(res.dmg_immolate > 0.0);
    CHECK(res.dmg_conflagrate > 0.0);
    CHECK_EQ(res.dmg_shadowburn, 0.0);
    CHECK_EQ(res.dmg_soul_fire, 0.0);
}

TEST_CASE(Rotations, GnomeEurekaPriorityRuleAndExecution) {
    PolicyConfig policy;
    Talents talents = Talents::create_forever_shadow_destro();
    auto rules_gnome = policy.get_priority_rules(talents, Race::GNOME);
    bool has_eureka = false;
    for (const auto& r : rules_gnome) {
        if (r.action == PriorityAction::RACIAL_EUREKA) {
            has_eureka = true;
            CHECK_EQ(static_cast<int>(r.spell_id), static_cast<int>(SpellID::RACIAL_EUREKA));
        }
    }
    CHECK(has_eureka);

    auto rules_undead = policy.get_priority_rules(talents, Race::UNDEAD);
    bool undead_has_eureka = false;
    for (const auto& r : rules_undead) {
        if (r.action == PriorityAction::RACIAL_EUREKA) undead_has_eureka = true;
    }
    CHECK(!undead_has_eureka);

    // Verify simulation triggers Eureka
    FastRNG rng(42);
    WarlockSimulator sim;
    sim.race = Race::GNOME;
    sim.talents = talents;
    sim.record_timeline = true;
    sim.fight_duration = 30.0;
    SimResult res = sim.run_single_simulation(rng);
    bool eureka_logged = false;
    for (const auto& c : res.cast_sequence) {
        if (c.spell_id == SpellID::RACIAL_EUREKA) eureka_logged = true;
    }
    CHECK(eureka_logged);
}

TEST_CASE(Rotations, NFDSRuinDeterministicRun) {
    FastRNG rng(1337);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_nf_ds_ruin();
    sim.buffs.sacrifice_imp = true;
    sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    sim.fight_duration = 30.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);
    CHECK(res.dps > 0.0);
    CHECK(res.dmg_corruption > 0.0);
    CHECK(res.shadow_bolt_casts > 0);
    CHECK(res.dmg_shadow_bolt > 0.0);
    CHECK(res.dmg_shadowburn > 0.0);
}

TEST_CASE(Rotations, DecimationSearingPainSoulFireExecution) {
    FastRNG rng(42);
    WarlockSimulator sim;
    sim.talents = Talents::create_forever_shadow_and_flame();
    sim.policy.rotation = RotationChoice::FIRE_DESTRO;
    sim.policy.use_decimation_soul_fire = true;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);
    CHECK(res.total_damage > 0.0);

    // Look for Searing Pain cast in execute phase (<35% HP -> time >= 60 * 0.65 = 39.0s)
    bool found_sp_execute = false;
    bool found_sf_execute = false;
    double first_sp_time = -1.0;
    double first_sf_time = -1.0;

    for (const auto& cast : res.cast_sequence) {
        if (cast.time >= 39.0) {
            if (cast.spell_id == SpellID::SEARING_PAIN && first_sp_time < 0.0) {
                first_sp_time = cast.time;
                found_sp_execute = true;
            }
            if (cast.spell_id == SpellID::SOUL_FIRE && first_sf_time < 0.0) {
                first_sf_time = cast.time;
                found_sf_execute = true;
            }
        }
    }

    CHECK(found_sp_execute);
    CHECK(found_sf_execute);
    // Searing Pain must trigger Decimation before Soul Fire can be cast
    CHECK(first_sp_time <= first_sf_time);
}

TEST_CASE(Rotations, MultiTargetCorruptionDistribution) {
    FastRNG rng1(42);
    FastRNG rng2(42);

    WarlockSimulator sim1; // 1 target
    sim1.target_config.target_count = 1;
    sim1.talents = Talents::create_forever_sm_ruin();
    sim1.policy.rotation = RotationChoice::SM_RUIN;
    sim1.policy.multi_dot_corruption = true;
    sim1.fight_duration = 60.0;
    SimResult res1 = sim1.run_single_simulation(rng1);

    WarlockSimulator sim2; // 2 targets
    sim2.target_config.target_count = 2;
    sim2.talents = Talents::create_forever_sm_ruin();
    sim2.policy.rotation = RotationChoice::SM_RUIN;
    sim2.policy.multi_dot_corruption = true;
    sim2.fight_duration = 60.0;
    SimResult res2 = sim2.run_single_simulation(rng2);

    // With 2 targets and multi-dotting enabled, corruption damage should roughly double
    CHECK(res2.dmg_corruption > res1.dmg_corruption * 1.5);
    CHECK(res2.total_damage > res1.total_damage);
}

TEST_CASE(Rotations, MultiTargetBaneOfHavocCleave) {
    FastRNG rng(12345);

    WarlockSimulator sim;
    sim.target_config.target_count = 2;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_hit_percent = 16.0; // Hit capped
    sim.talents = Talents::create_forever_shadow_destro();
    sim.talents.destro.bane_of_havoc = 1;
    sim.policy.rotation = RotationChoice::SHADOW_DESTRO;
    sim.policy.auto_bane_of_havoc = true;
    sim.fight_duration = 60.0;
    sim.record_timeline = true;

    SimResult res = sim.run_single_simulation(rng);

    CHECK(res.dmg_bane_of_havoc > 0.0);
    double primary_dmg = res.total_damage - res.dmg_bane_of_havoc;
    CHECK(primary_dmg > 0.0);
    CHECK(res.dmg_bane_of_havoc >= primary_dmg * 0.12);
    CHECK(res.dmg_bane_of_havoc <= primary_dmg * 0.16);
}

TEST_CASE(Rotations, SingleTargetNoHavocOrMultiDotOverhead) {
    FastRNG rng1(999);
    FastRNG rng2(999);

    WarlockSimulator sim_default;
    sim_default.target_config.target_count = 1;
    sim_default.talents = Talents::create_forever_shadow_destro();
    sim_default.talents.destro.bane_of_havoc = 1;
    sim_default.fight_duration = 60.0;
    SimResult res1 = sim_default.run_single_simulation(rng1);

    WarlockSimulator sim_with_toggles;
    sim_with_toggles.target_config.target_count = 1;
    sim_with_toggles.talents = Talents::create_forever_shadow_destro();
    sim_with_toggles.talents.destro.bane_of_havoc = 1;
    sim_with_toggles.policy.auto_bane_of_havoc = true;
    sim_with_toggles.policy.multi_dot_corruption = true;
    sim_with_toggles.fight_duration = 60.0;
    SimResult res2 = sim_with_toggles.run_single_simulation(rng2);

    // Single target must produce identical byte-for-byte damage & zero Bane of Havoc
    CHECK_EQ(res1.dmg_bane_of_havoc, 0.0);
    CHECK_EQ(res2.dmg_bane_of_havoc, 0.0);
    CHECK_EQ(res1.total_damage, res2.total_damage);
    CHECK_EQ(res1.dps, res2.dps);
    CHECK_EQ(res1.direct_spell_casts, res2.direct_spell_casts);
}

TEST_CASE(Rotations, AmplifyCursePriorityRuleAndExecution) {
    PolicyConfig policy;
    policy.rotation = RotationChoice::DEEP_AFFLICTION;
    Talents talented = Talents::create_forever_deep_affliction();
    CHECK_EQ(talented.aff.amplify_curse, 1);

    auto rules = policy.get_priority_rules(talented);
    int pos_amp = -1, pos_agony = -1;
    for (size_t i = 0; i < rules.size(); ++i) {
        if (rules[i].spell_id == SpellID::AMPLIFY_CURSE) pos_amp = (int)i;
        if (rules[i].spell_id == SpellID::CURSE_OF_AGONY) pos_agony = (int)i;
    }
    CHECK(pos_amp != -1);
    CHECK(pos_agony != -1);
    CHECK(pos_amp < pos_agony);

    // When untalented, Amplify Curse must NOT be present
    Talents untalented = talented;
    untalented.aff.amplify_curse = 0;
    auto untalented_rules = policy.get_priority_rules(untalented);
    bool has_amp_rule = false;
    for (const auto& r : untalented_rules) {
        if (r.spell_id == SpellID::AMPLIFY_CURSE) has_amp_rule = true;
    }
    CHECK_EQ(has_amp_rule, false);

    // Simulation run with Amplify Curse talented
    FastRNG rng1(1337);
    FastRNG rng2(1337);

    WarlockSimulator sim_amp;
    sim_amp.talents = talented;
    sim_amp.policy.rotation = RotationChoice::DEEP_AFFLICTION;
    sim_amp.fight_duration = 24.0;
    sim_amp.record_timeline = true;
    SimResult res_amp = sim_amp.run_single_simulation(rng1);

    WarlockSimulator sim_no_amp;
    sim_no_amp.talents = untalented;
    sim_no_amp.policy.rotation = RotationChoice::DEEP_AFFLICTION;
    sim_no_amp.fight_duration = 24.0;
    sim_no_amp.record_timeline = true;
    SimResult res_no_amp = sim_no_amp.run_single_simulation(rng2);

    CHECK(res_amp.spell_stats[static_cast<size_t>(SpellID::AMPLIFY_CURSE)].casts > 0);
    CHECK_EQ(res_no_amp.spell_stats[static_cast<size_t>(SpellID::AMPLIFY_CURSE)].casts, 0);

    // Verify Amplify Curse is recorded in cast sequence
    bool amp_in_cast_seq = false;
    for (const auto& log : res_amp.cast_sequence) {
        if (log.spell_id == SpellID::AMPLIFY_CURSE) amp_in_cast_seq = true;
    }
    CHECK(amp_in_cast_seq);

    // Amplified Bane of Agony should deal ~50% more damage than unamplified
    CHECK(res_amp.dmg_agony > res_no_amp.dmg_agony);
    CHECK(res_amp.dmg_agony >= res_no_amp.dmg_agony * 1.45);
    CHECK(res_amp.dmg_agony <= res_no_amp.dmg_agony * 1.55);
}

