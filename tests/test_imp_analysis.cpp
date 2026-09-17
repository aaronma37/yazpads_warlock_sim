#include "test_framework.hpp"
#include "src/sim/pet_analysis.hpp"

using namespace warlock;
using namespace warlock::imp_analysis;

TEST_CASE(ImpAnalysis, InfiniteManaCastCount) {
    // Modern: First cast at 0.3s, then every 2.0s: floor((T - 0.3) / 2.0) + 1
    CHECK_EQ(expected_cast_count(120.0, false, true), 60);
    CHECK_EQ(expected_cast_count(60.0, false, true), 30);
    CHECK_EQ(expected_cast_count(300.0, false, true), 150);
    CHECK_EQ(expected_cast_count(0.3, false, true), 0);
    CHECK_EQ(expected_cast_count(0.0, false, true), 0);

    // Classic: First cast at 0.3s, then every 1.5s: floor((T - 0.3) / 1.5) + 1
    CHECK_EQ(expected_cast_count(120.0, false, false), 80);
    CHECK_EQ(expected_cast_count(60.0, false, false), 40);
    CHECK_EQ(expected_cast_count(300.0, false, false), 200);
}

TEST_CASE(ImpAnalysis, ManaStarvedCastsFewer) {
    // 1150 pool / 115 per cast, +45 mana per 5s: imp starves fast.
    CHECK_EQ(expected_cast_count(120.0, true, true), 19);
    CHECK(expected_cast_count(120.0, true, true) < expected_cast_count(120.0, false, true));
    CHECK(expected_cast_count(60.0, true, true) < expected_cast_count(60.0, false, true));
    // Longer fights still squeeze out more regen-limited casts.
    CHECK(expected_cast_count(300.0, true, true) > expected_cast_count(120.0, true, true));

    // Classic 1.5s interval
    CHECK_EQ(expected_cast_count(120.0, true, false), 19);
}

TEST_CASE(ImpAnalysis, ExpectedDamagePerCast) {
    // Modern: 44 base + 15% pet SP (2.0/3.5 coeff)
    CHECK_NEAR(expected_damage_per_cast(0.0, 0, 0, true), 44.0 * 0.83 * 1.025, 1e-6);
    CHECK_NEAR(expected_damage_per_cast(500.0, 0, 0, true),
               (44.0 + (2.0 / 3.5) * 0.15 * 500.0) * 0.83 * 1.025, 1e-6);
    // 5/5 Unholy Power (+10%) and 3/3 Improved Imp (+30%) stack multiplicatively.
    CHECK_NEAR(expected_damage_per_cast(500.0, 5, 3, true),
               (44.0 + (2.0 / 3.5) * 0.15 * 500.0) * 0.83 * 1.025 * 1.10 * 1.30, 1e-6);

    // Classic: (85 + 98) / 2 * 0.83 hit * 1.025 crit = 77.843625
    CHECK_NEAR(expected_damage_per_cast(0.0, 0, 0, false), 77.843625, 1e-6);
    CHECK_NEAR(expected_damage_per_cast(0.0, 5, 3, false),
               77.843625 * 1.10 * 1.30, 1e-6);
    CHECK_NEAR(expected_damage_per_cast(500.0, 0, 0, false),
               (91.5 + (1.5 / 3.5) * 0.15 * 500.0) * 0.83 * 1.025, 1e-6);
}

TEST_CASE(ImpAnalysis, DpsOrderingAndLinearity) {
    const double T = 120.0;
    const double base = expected_dps(400.0, 0, 0, false, T);
    const double up = expected_dps(400.0, 5, 0, false, T);
    const double imp = expected_dps(400.0, 0, 3, false, T);
    const double both = expected_dps(400.0, 5, 3, false, T);
    CHECK(both > imp);
    CHECK(imp > up);
    CHECK(up > base);
    // Infinite mana always beats mana-starved for identical talents.
    CHECK(expected_dps(400.0, 5, 3, false, T) > expected_dps(400.0, 5, 3, true, T));
    // DPS is linear in master spell power.
    const double d0 = expected_dps(0.0, 0, 0, false, T);
    const double d200 = expected_dps(200.0, 0, 0, false, T);
    const double d400 = expected_dps(400.0, 0, 0, false, T);
    CHECK_NEAR(d400 - d0, 2.0 * (d200 - d0), 1e-9);
}

TEST_CASE(ImpAnalysis, SlopePer10Sp) {
    auto slope = [](double sp, int up, int ii, bool mana_limited, double T, bool modern = true) {
        return (expected_dps(sp, up, ii, mana_limited, T, modern) -
                expected_dps(0.0, up, ii, mana_limited, T, modern)) / sp * 10.0;
    };
    const double T = 120.0;
    // Modern scaling linearity & talent multiplier scaling
    CHECK_NEAR(slope(800.0, 5, 3, false, T, true), slope(400.0, 5, 3, false, T, true), 1e-9);
    CHECK_NEAR(slope(800.0, 5, 3, false, T, true) / slope(800.0, 0, 0, false, T, true), 1.43, 1e-9);
    CHECK(slope(800.0, 0, 0, true, T, true) < slope(800.0, 0, 0, false, T, true));

    // Classic spot value
    CHECK_NEAR(slope(800.0, 0, 0, false, T, false), 0.36460714285714285, 1e-9);
    CHECK_NEAR(slope(800.0, 0, 0, true, T, false), 0.08659419642857142, 1e-9);
}

TEST_CASE(ImpAnalysis, SuccubusMeleeSwings) {
    // First swing at 1.0s, then every 2.0s; melee never costs mana.
    CHECK_EQ(succubus_expected_melee_swings(120.0), 60);
    CHECK_EQ(succubus_expected_melee_swings(1.0), 0);
    CHECK_EQ(succubus_expected_melee_swings(0.0), 0);
    // 170 avg * 0.86 armor * 0.95 hit * 1.05 crit.
    CHECK_NEAR(succubus_expected_melee_per_swing(0.0, 0), 170.0 * 0.86 * 0.95 * 1.05, 1e-9);
    // 57% SP to AP: (0.57 / 14) * 2.0 per swing before multipliers.
    CHECK_NEAR(succubus_expected_melee_per_swing(500.0, 0),
               (170.0 + (0.57 * 500.0 / 14.0) * 2.0) * 0.86 * 0.95 * 1.05, 1e-9);
}

TEST_CASE(ImpAnalysis, SuccubusLopCasts) {
    // Cooldown 12s base, 9s at 3/3 Sayaad.
    CHECK_NEAR(succubus_lop_cooldown(0), 12.0, 1e-9);
    CHECK_NEAR(succubus_lop_cooldown(3), 9.0, 1e-9);
    // Infinite mana: first Lash at 0.5s.
    CHECK_EQ(succubus_expected_lop_casts(120.0, 0, false), 10);
    CHECK_EQ(succubus_expected_lop_casts(120.0, 3, false), 14);
    // Mana-starved (1450 pool, 160 per Lash): fits in pool over 120s
    CHECK_EQ(succubus_expected_lop_casts(120.0, 0, true), 10);
    CHECK_EQ(succubus_expected_lop_casts(120.0, 3, true), 14);
    CHECK(succubus_expected_lop_casts(300.0, 3, true) <
          succubus_expected_lop_casts(300.0, 3, false));
    // 50 avg * 0.83 hit * 1.025 crit; Sayaad +30% stacks with UP +10%.
    CHECK_NEAR(succubus_expected_lop_per_cast(0.0, 0, 0), 50.0 * 0.83 * 1.025, 1e-9);
    CHECK_NEAR(succubus_expected_lop_per_cast(0.0, 5, 3),
               50.0 * 1.10 * 1.30 * 0.83 * 1.025, 1e-9);
}

TEST_CASE(ImpAnalysis, SuccubusDpsOrderingAndSlope) {
    const double T = 120.0;
    const double base = succubus_expected_dps(400.0, 0, 0, false, T);
    const double up = succubus_expected_dps(400.0, 5, 0, false, T);
    const double sayaad = succubus_expected_dps(400.0, 0, 3, false, T);
    const double both = succubus_expected_dps(400.0, 5, 3, false, T);
    CHECK(both > up);
    CHECK(up > sayaad);
    CHECK(sayaad > base);
    CHECK(succubus_expected_dps(400.0, 0, 3, false, 300.0) >=
          succubus_expected_dps(400.0, 0, 3, true, 300.0));
    // Spot value: (60 melee swings + 10 lashes) over 120s.
    CHECK_NEAR(succubus_expected_dps(0.0, 0, 0, false, T),
               (60.0 * 170.0 * 0.86 * 0.95 * 1.05 + 10.0 * 50.0 * 0.83 * 1.025) / 120.0,
               1e-9);
    // Slope is linear in SP for both components.
    auto slope = [](double sp, int up, int sayaad, bool ml, double T) {
        return (succubus_expected_dps(sp, up, sayaad, ml, T) -
                succubus_expected_dps(0.0, up, sayaad, ml, T)) / sp * 10.0;
    };
    CHECK_NEAR(slope(800.0, 5, 3, false, T), slope(400.0, 5, 3, false, T), 1e-9);
    CHECK(slope(800.0, 0, 0, true, T) <= slope(800.0, 0, 0, false, T));
}
