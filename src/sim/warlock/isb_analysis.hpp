#pragma once
#include <algorithm>
#include <cmath>

namespace warlock {
namespace isb_analysis {

// Analytic steady-state model for Improved Shadow Bolt (ISB) uptime.
//
// Mirrors the discrete-event ISB logic with all randomness replaced by
// expected values:
//   - warlock_sim.cpp resolves each Shadow Bolt as miss first, then rolls
//     crit only on landed hits, so the proc probability per cast is
//     hit_chance * crit_chance (crit_chance is the chance a LANDED bolt
//     crits, i.e. paperdoll crit).
//   - stats.hpp default (mechanics.isb_has_charges == false): each crit
//     refreshes a chargeless 12s window (apply_isb sets
//     isb_expire_time = now + 12.0).
//
// With procs arriving as a Poisson stream at rate lambda = h * c / T
// (h = hit chance, c = crit chance, T = mean seconds per Shadow Bolt
// cast), the buff is down only when a gap between procs exceeds the
// window D, so steady-state uptime = 1 - exp(-lambda * D).
//
// This models the default Forever (chargeless) ISB rules. Classic
// 4-charge ISB additionally expires when the raid consumes all charges,
// so this is an upper bound when charges are enabled.

constexpr double kIsbWindowSeconds = 12.0; // apply_isb: expire = now + 12.0

inline double proc_rate_per_second(double crit_chance, double hit_chance,
                                   double cast_interval_seconds) {
    if (cast_interval_seconds <= 0.0) return 0.0;
    const double c = std::clamp(crit_chance, 0.0, 1.0);
    const double h = std::clamp(hit_chance, 0.0, 1.0);
    return h * c / cast_interval_seconds;
}

// Steady-state fraction of time [0, 1] the ISB debuff is active.
inline double expected_uptime(double crit_chance, double hit_chance,
                              double cast_interval_seconds,
                              double window_seconds = kIsbWindowSeconds) {
    if (window_seconds <= 0.0) return 0.0;
    const double lambda =
        proc_rate_per_second(crit_chance, hit_chance, cast_interval_seconds);
    if (lambda <= 0.0) return 0.0;
    return 1.0 - std::exp(-lambda * window_seconds);
}

} // namespace isb_analysis
} // namespace warlock
