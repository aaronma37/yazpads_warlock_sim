#include "test_framework.hpp"
#include "src/sim/pet_analysis.hpp"

using namespace warlock;
using namespace warlock::imp_analysis;

TEST_CASE(ImpAnalysis, InfiniteManaCastCount) {
    // First cast at 0.3s, then every 1.5s: floor((T - 0.3) / 1.5) + 1
    CHECK_EQ(expected_cast_count(120.0, false), 80);
    CHECK_EQ(expected_cast_count(60.0, false), 40);
    CHECK_EQ(expected_cast_count(300.0, false), 200);
    CHECK_EQ(expected_cast_count(0.3, false), 0);
    CHECK_EQ(expected_cast_count(0.0, false), 0);
}

TEST_CASE(ImpAnalysis, ManaStarvedCastsFewer) {
    // 1150 pool / 115 per cast, +45 mana per 5s: imp starves fast.
    CHECK_EQ(expected_cast_count(120.0, true), 19);
    CHECK(expected_cast_count(120.0, true) < expected_cast_count(120.0, false));
    CHECK(expected_cast_count(60.0, true) < expected_cast_count(60.0, false));
    // Longer fights still squeeze out more regen-limited casts.
    CHECK(expected_cast_count(300.0, true) > expected_cast_count(120.0, true));
}

TEST_CASE(ImpAnalysis, ExpectedDamagePerCast) {
    // (85 + 98) / 2 * 0.83 hit * 1.025 crit = 77.843625
    CHECK_NEAR(expected_damage_per_cast(0.0, 0, 0), 77.843625, 1e-6);
    // 5/5 Unholy Power (+10%) and 3/3 Improved Imp (+30%) stack multiplicatively.
    CHECK_NEAR(expected_damage_per_cast(0.0, 5, 3),
               77.843625 * 1.10 * 1.30, 1e-6);
    // 15% SP inheritance with the (1.5 / 3.5) coefficient.
    CHECK_NEAR(expected_damage_per_cast(500.0, 0, 0),
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
    // DPS is linear in master spell power (mana only affects cast count).
    const double d0 = expected_dps(0.0, 0, 0, false, T);
    const double d200 = expected_dps(200.0, 0, 0, false, T);
    const double d400 = expected_dps(400.0, 0, 0, false, T);
    CHECK_NEAR(d400 - d0, 2.0 * (d200 - d0), 1e-9);
    // Spot values: 80 casts over 120s.
    CHECK_NEAR(expected_dps(0.0, 0, 0, false, T), 80.0 * 77.843625 / 120.0, 1e-9);
    CHECK_NEAR(expected_dps(0.0, 0, 0, true, T), 19.0 * 77.843625 / 120.0, 1e-9);
}

TEST_CASE(ImpAnalysis, SlopePer10Sp) {
    // Slope shown in the UI table: DPS gained per 10 master spell power.
    auto slope = [](double sp, int up, int ii, bool mana_limited, double T) {
        return (expected_dps(sp, up, ii, mana_limited, T) -
                expected_dps(0.0, up, ii, mana_limited, T)) / sp * 10.0;
    };
    const double T = 120.0;
    // Independent of the SP span used (linearity).
    CHECK_NEAR(slope(800.0, 5, 3, false, T), slope(400.0, 5, 3, false, T), 1e-9);
    CHECK_NEAR(slope(800.0, 0, 0, true, T), slope(200.0, 0, 0, true, T), 1e-9);
    // Scales with the talent multipliers: 1.10 (UP 5/5) * 1.30 (Imp Imp 3/3).
    CHECK_NEAR(slope(800.0, 5, 3, false, T) / slope(800.0, 0, 0, false, T), 1.43, 1e-9);
    // Mana starvation flattens the slope (fewer casts per fight).
    CHECK(slope(800.0, 0, 0, true, T) < slope(800.0, 0, 0, false, T));
    // Spot values: casts * (1.5/3.5) * 0.15 * 0.83 * 1.025 / T * 10.
    CHECK_NEAR(slope(800.0, 0, 0, false, T), 0.36460714285714285, 1e-9);
    CHECK_NEAR(slope(800.0, 0, 0, true, T), 0.08659419642857142, 1e-9);
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
    // Cooldown 9s base, 6s at 3/3 Sayaad.
    CHECK_NEAR(succubus_lop_cooldown(0), 9.0, 1e-9);
    CHECK_NEAR(succubus_lop_cooldown(3), 6.0, 1e-9);
    // Infinite mana: first Lash at 0.5s.
    CHECK_EQ(succubus_expected_lop_casts(120.0, 0, false), 14);
    CHECK_EQ(succubus_expected_lop_casts(120.0, 3, false), 20);
    // Mana-starved (1450 pool, 160 per Lash): base rotation fits in the pool
    // at 120s, but 3/3 Sayaad outpaces regen.
    CHECK_EQ(succubus_expected_lop_casts(120.0, 0, true), 14);
    CHECK_EQ(succubus_expected_lop_casts(120.0, 3, true), 15);
    CHECK(succubus_expected_lop_casts(300.0, 0, true) <
          succubus_expected_lop_casts(300.0, 0, false));
    // 107 avg * 0.83 hit * 1.025 crit; Sayaad +30% stacks with UP +10%.
    CHECK_NEAR(succubus_expected_lop_per_cast(0.0, 0, 0), 107.0 * 0.83 * 1.025, 1e-9);
    CHECK_NEAR(succubus_expected_lop_per_cast(0.0, 5, 3),
               107.0 * 1.10 * 1.30 * 0.83 * 1.025, 1e-9);
}

TEST_CASE(ImpAnalysis, SuccubusDpsOrderingAndSlope) {
    const double T = 120.0;
    const double base = succubus_expected_dps(400.0, 0, 0, false, T);
    const double up = succubus_expected_dps(400.0, 5, 0, false, T);
    const double sayaad = succubus_expected_dps(400.0, 0, 3, false, T);
    const double both = succubus_expected_dps(400.0, 5, 3, false, T);
    CHECK(both > sayaad);
    CHECK(sayaad > up);
    CHECK(up > base);
    CHECK(succubus_expected_dps(400.0, 0, 3, false, T) >=
          succubus_expected_dps(400.0, 0, 3, true, T));
    // Spot value: (60 melee swings + 14 lashes) over 120s.
    CHECK_NEAR(succubus_expected_dps(0.0, 0, 0, false, T),
               (60.0 * 170.0 * 0.86 * 0.95 * 1.05 + 14.0 * 107.0 * 0.83 * 1.025) / 120.0,
               1e-9);
    // Slope is linear in SP for both components.
    auto slope = [](double sp, int up, int sayaad, bool ml, double T) {
        return (succubus_expected_dps(sp, up, sayaad, ml, T) -
                succubus_expected_dps(0.0, up, sayaad, ml, T)) / sp * 10.0;
    };
    CHECK_NEAR(slope(800.0, 5, 3, false, T), slope(400.0, 5, 3, false, T), 1e-9);
    CHECK(slope(800.0, 0, 0, true, T) <= slope(800.0, 0, 0, false, T));
}
