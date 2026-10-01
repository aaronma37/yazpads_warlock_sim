#include "test_framework.hpp"
#include "src/sim/webgpu/parity_comparator.hpp"
#include <iostream>

TEST_CASE(WebGPUParitySuite, PureDirectDamageAndStatScaling) {
    auto sweep = warlock::ParityComparator::get_spell_power_sweep(12.0, 15.0);
    for (const auto& sc : sweep) {
        auto report = warlock::ParityComparator::compare(
            sc.name, sc.description, sc.sim_config, 5000, 42, 2.0, 2.0
        );
        CHECK(report.overall_passed);
        CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
    }
}

TEST_CASE(WebGPUParitySuite, ImprovedShadowBoltMechanics) {
    // 1. ISB Classic 4 charges
    {
        warlock::WarlockSimulator sim;
        sim.fight_duration = 180.0;
        sim.race = warlock::Race::HUMAN;
        sim.use_raw_stats = true;
        sim.raw_stats.spell_power = 600.0;
        sim.raw_stats.max_mana = 6500.0;
        sim.raw_stats.spell_hit_percent = 12.0;
        sim.raw_stats.spell_crit_percent = 25.0;
        sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
        sim.policy.corruption = warlock::DotPolicy::NEVER;
        sim.policy.pet = warlock::PetChoice::NONE;
        sim.buffs = warlock::BuffConfig{};
        sim.talents = warlock::Talents{};
        sim.talents.destro.bane = 5;
        sim.talents.destro.ruin = 5;
        sim.talents.destro.improved_shadow_bolt = 5;
        sim.mechanics.isb_has_charges = true;

        auto report = warlock::ParityComparator::compare("isb_charges", "ISB 4 Charges", sim, 5000, 42, 2.0, 2.0);
        CHECK(report.overall_passed);
        CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
        CHECK(report.isb_uptime_metric.cpu_val > 30.0);
    }

    // 2. ISB Forever 12s window
    {
        warlock::WarlockSimulator sim;
        sim.fight_duration = 180.0;
        sim.race = warlock::Race::HUMAN;
        sim.use_raw_stats = true;
        sim.raw_stats.spell_power = 600.0;
        sim.raw_stats.max_mana = 6500.0;
        sim.raw_stats.spell_hit_percent = 12.0;
        sim.raw_stats.spell_crit_percent = 25.0;
        sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
        sim.policy.corruption = warlock::DotPolicy::NEVER;
        sim.policy.pet = warlock::PetChoice::NONE;
        sim.buffs = warlock::BuffConfig{};
        sim.talents = warlock::Talents{};
        sim.talents.destro.bane = 5;
        sim.talents.destro.ruin = 5;
        sim.talents.destro.improved_shadow_bolt = 5;
        sim.mechanics.isb_has_charges = false;

        auto report = warlock::ParityComparator::compare("isb_window", "ISB 12s Window", sim, 5000, 42, 2.0, 2.0);
        CHECK(report.overall_passed);
        CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
    }
}

TEST_CASE(WebGPUParitySuite, MultiDotAndCurseBreakdown) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::HUMAN;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 650.0;
    sim.raw_stats.max_mana = 7000.0;
    sim.raw_stats.spell_hit_percent = 14.0;
    sim.raw_stats.spell_crit_percent = 20.0;
    sim.policy.rotation = warlock::RotationChoice::AFFLICTION_HYBRID_DOTS;
    sim.policy.curse = warlock::CurseChoice::BANE_OF_AGONY;
    sim.policy.corruption = warlock::DotPolicy::ALWAYS;
    sim.policy.maintain_immolate = true;
    sim.policy.pet = warlock::PetChoice::NONE;
    sim.buffs = warlock::BuffConfig{};
    sim.talents = warlock::Talents{};
    sim.talents.aff.siphon_life = 1;
    sim.talents.aff.improved_bane_of_agony = 2;
    sim.talents.aff.improved_corruption = 5;
    sim.talents.aff.shadow_mastery = 5;
    sim.talents.destro.bane = 5;
    sim.talents.destro.ruin = 5;

    auto report = warlock::ParityComparator::compare("multidot", "Multi-DoT SM/Ruin", sim, 5000, 777, 2.0, 2.0);
    CHECK(report.overall_passed);
    CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
    for (const auto& bm : report.breakdown_metrics) {
        CHECK(bm.passed);
    }
}

TEST_CASE(WebGPUParitySuite, PetsAndDemonicSacrifice) {
    // 1. Imp
    {
        warlock::WarlockSimulator sim;
        sim.fight_duration = 180.0;
        sim.race = warlock::Race::HUMAN;
        sim.use_raw_stats = true;
        sim.raw_stats.spell_power = 500.0;
        sim.raw_stats.max_mana = 6500.0;
        sim.raw_stats.spell_hit_percent = 10.0;
        sim.raw_stats.spell_crit_percent = 15.0;
        sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
        sim.policy.corruption = warlock::DotPolicy::NEVER;
        sim.policy.pet = warlock::PetChoice::IMP;
        sim.buffs = warlock::BuffConfig{};
        sim.talents = warlock::Talents{};
        sim.talents.destro.bane = 5;
        sim.talents.destro.ruin = 5;
        sim.talents.demo.improved_imp = 3;

        auto report = warlock::ParityComparator::compare("imp", "Imp Active", sim, 5000, 42, 2.0, 2.0);
        CHECK(report.overall_passed);
        CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
    }

    // 2. Succubus
    {
        warlock::WarlockSimulator sim;
        sim.fight_duration = 180.0;
        sim.race = warlock::Race::HUMAN;
        sim.use_raw_stats = true;
        sim.raw_stats.spell_power = 550.0;
        sim.raw_stats.max_mana = 6500.0;
        sim.raw_stats.spell_hit_percent = 10.0;
        sim.raw_stats.spell_crit_percent = 15.0;
        sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
        sim.policy.corruption = warlock::DotPolicy::NEVER;
        sim.policy.pet = warlock::PetChoice::SUCCUBUS;
        sim.buffs = warlock::BuffConfig{};
        sim.talents = warlock::Talents{};
        sim.talents.destro.bane = 5;
        sim.talents.destro.ruin = 5;
        sim.talents.demo.improved_sayaad = 2;

        auto report = warlock::ParityComparator::compare("succubus", "Succubus Active", sim, 5000, 42, 2.0, 2.0);
        CHECK(report.overall_passed);
        CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
    }
}

TEST_CASE(WebGPUParitySuite, UndeadTouchOfTheGrave) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::UNDEAD;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 600.0;
    sim.raw_stats.max_mana = 6500.0;
    sim.raw_stats.spell_hit_percent = 16.0;
    sim.raw_stats.spell_crit_percent = 20.0;
    sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
    sim.policy.corruption = warlock::DotPolicy::ALWAYS;
    sim.policy.pet = warlock::PetChoice::NONE;
    sim.buffs = warlock::BuffConfig{};
    sim.talents = warlock::Talents{};
    sim.talents.destro.bane = 5;
    sim.talents.destro.ruin = 5;

    auto report = warlock::ParityComparator::compare("undead_totg", "Undead Touch of the Grave", sim, 5000, 42, 2.0, 2.0);
    CHECK(report.overall_passed);
    CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
    
    // Ensure Touch of the Grave was tracked and procced on both CPU and WebGPU
    bool found_totg = false;
    for (const auto& m : report.breakdown_metrics) {
        if (m.name == "Touch of the Grave") {
            found_totg = true;
            CHECK(m.cpu_val > 0.0);
            CHECK(m.webgpu_val > 0.0);
            CHECK(m.passed);
        }
    }
    CHECK(found_totg);
}

TEST_CASE(WebGPUParitySuite, OrcBloodFury) {

    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::ORC;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 600.0;
    sim.raw_stats.max_mana = 6500.0;
    sim.raw_stats.spell_hit_percent = 12.0;
    sim.raw_stats.spell_crit_percent = 20.0;
    sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
    sim.policy.corruption = warlock::DotPolicy::ALWAYS;
    sim.policy.racial_policy = warlock::RacialPolicy::ON_COOLDOWN;
    sim.policy.pet = warlock::PetChoice::NONE;
    sim.buffs = warlock::BuffConfig{};
    sim.talents = warlock::Talents{};
    sim.talents.destro.bane = 5;
    sim.talents.destro.ruin = 5;

    auto report = warlock::ParityComparator::compare("orc_bf", "Orc Blood Fury", sim, 5000, 42, 2.0, 2.0);
    CHECK(report.overall_passed);
    CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
}

TEST_CASE(WebGPUParitySuite, TrollBerserking) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::TROLL;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 600.0;
    sim.raw_stats.max_mana = 6500.0;
    sim.raw_stats.spell_hit_percent = 12.0;
    sim.raw_stats.spell_crit_percent = 20.0;
    sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
    sim.policy.corruption = warlock::DotPolicy::ALWAYS;
    sim.policy.racial_policy = warlock::RacialPolicy::ON_COOLDOWN;
    sim.policy.pet = warlock::PetChoice::NONE;
    sim.buffs = warlock::BuffConfig{};
    sim.talents = warlock::Talents{};
    sim.talents.destro.bane = 5;
    sim.talents.destro.ruin = 5;

    auto report = warlock::ParityComparator::compare("troll_bs", "Troll Berserking", sim, 5000, 42, 2.0, 2.0);
    CHECK(report.overall_passed);
    CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
}

TEST_CASE(WebGPUParitySuite, GnomeExpansiveMind) {
    warlock::WarlockSimulator sim;
    sim.fight_duration = 180.0;
    sim.race = warlock::Race::GNOME;
    sim.use_raw_stats = true;
    sim.raw_stats.spell_power = 600.0;
    sim.raw_stats.max_mana = 6500.0;
    sim.raw_stats.spell_hit_percent = 12.0;
    sim.raw_stats.spell_crit_percent = 20.0;
    sim.policy.rotation = warlock::RotationChoice::PURE_SHADOW_BOLT;
    sim.policy.corruption = warlock::DotPolicy::ALWAYS;
    sim.policy.racial_policy = warlock::RacialPolicy::ON_COOLDOWN;
    sim.policy.pet = warlock::PetChoice::NONE;
    sim.buffs = warlock::BuffConfig{};
    sim.talents = warlock::Talents{};
    sim.talents.destro.bane = 5;
    sim.talents.destro.ruin = 5;

    auto report = warlock::ParityComparator::compare("gnome_em", "Gnome Expansive Mind & Eureka!", sim, 5000, 42, 2.0, 2.0);
    CHECK(report.overall_passed);
    CHECK(std::abs(report.mean_dps_metric.pct_diff) <= 2.0);
}

