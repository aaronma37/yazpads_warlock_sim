#include "test_framework.hpp"
#include "src/sim/isb_analysis.hpp"
#include <cmath>

using namespace warlock;
using namespace warlock::isb_analysis;

TEST_CASE(IsbAnalysis, DegenerateInputsGiveZero) {
    // No crits, no hits, or no casts: ISB can never proc.
    CHECK_EQ(expected_uptime(0.0, 0.99, 2.5), 0.0);
    CHECK_EQ(expected_uptime(0.20, 0.0, 2.5), 0.0);
    CHECK_EQ(expected_uptime(0.20, 0.99, 0.0), 0.0);
    CHECK_EQ(expected_uptime(0.20, 0.99, -1.0), 0.0);
    CHECK_EQ(expected_uptime(0.20, 0.99, 2.5, 0.0), 0.0);
    CHECK_EQ(proc_rate_per_second(0.20, 0.99, 0.0), 0.0);
}

TEST_CASE(IsbAnalysis, ClosedFormSpotValues) {
    // lambda = h*c/T; uptime = 1 - exp(-lambda * 12).
    CHECK_NEAR(proc_rate_per_second(0.20, 0.83, 2.5), 0.20 * 0.83 / 2.5, 1e-12);
    CHECK_NEAR(expected_uptime(0.20, 1.0, 2.5), 1.0 - std::exp(-0.96), 1e-12);
    CHECK_NEAR(expected_uptime(0.20, 0.83, 2.5),
               1.0 - std::exp(-0.20 * 0.83 / 2.5 * 12.0), 1e-12);
    // Inputs clamp to [0, 1]: crit above 100% behaves as 100%.
    CHECK_NEAR(expected_uptime(1.50, 1.0, 2.5), expected_uptime(1.0, 1.0, 2.5), 1e-12);
    CHECK_EQ(expected_uptime(-0.10, 1.0, 2.5), 0.0);
}

TEST_CASE(IsbAnalysis, MonotonicAndSaturating) {
    // More crit / more hit / faster casts can never hurt uptime.
    CHECK(expected_uptime(0.30, 0.83, 2.5) > expected_uptime(0.10, 0.83, 2.5));
    CHECK(expected_uptime(0.20, 0.99, 2.5) > expected_uptime(0.20, 0.83, 2.5));
    CHECK(expected_uptime(0.20, 0.99, 2.0) > expected_uptime(0.20, 0.99, 3.0));
    // Uptime saturates toward (but never exceeds) 100%.
    const double saturated = expected_uptime(1.0, 1.0, 2.5);
    CHECK(saturated > 0.99);
    CHECK(saturated <= 1.0);
    // Hit lines stay ordered across the whole crit axis.
    for (int i = 0; i <= 40; ++i) {
        const double c = i / 100.0;
        CHECK(expected_uptime(c, 0.99, 2.5) >= expected_uptime(c, 0.94, 2.5));
        CHECK(expected_uptime(c, 0.94, 2.5) >= expected_uptime(c, 0.89, 2.5));
        CHECK(expected_uptime(c, 0.89, 2.5) >= expected_uptime(c, 0.83, 2.5));
    }
}
