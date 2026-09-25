#pragma once
#include <algorithm>
#include <cmath>

namespace warlock {
namespace imp_analysis {

// Analytic expected-DPS models for the Imp (Firebolt) and the Succubus
// (melee auto-attacks + Lash of Pain).
//
// Mirrors the discrete-event pet logic in warlock_sim.cpp with all
// randomness replaced by expected values. Imp section mirrors PET_CAST_FINISH
// for PetChoice::IMP:
//   - Firebolt base damage is uniform 85-98  -> expected 91.5
//   - Pet spell power = 15% of the master's Fire spell power, scaled by the
//     (1.5 / 3.5) cast-time coefficient, exactly as in the sim
//   - 83% hit chance vs a raid boss, 5% crit chance at 1.5x damage
//   - No Curse of Elements, no resistances, no JoW / mana buffs
//   - Mana model: 1150 base pool, 115 mana per Firebolt, +45 mana every 5s
//     (base regen only), first cast attempt at 0.3s, 1.5s interval on a
//     successful cast, 1.0s retry while OOM.
//
// Talent axes: Unholy Power (+2% pet damage per point) and Improved Imp
// (+10% Firebolt damage per point). Fel Vitality is assumed 0/3.

constexpr double kModernFireboltBaseDamage = 44.0;
constexpr double kFireboltMinDamage = 85.0;
constexpr double kFireboltMaxDamage = 98.0;
constexpr double kPetSpRatio = 0.10;          // 10 SP = 1 Pet SP (10% SP inheritance)
constexpr double kCastTimeCoefficient = 1.5 / 3.5;
constexpr double kHitChance = 0.83;
constexpr double kCritChance = 0.05;
constexpr double kCritMultiplier = 1.5;
constexpr double kBaseMana = 1150.0;
constexpr double kFireboltCost = 115.0;
constexpr double kBaseRegenPerTick = 45.0;    // every 5s, base regen only
constexpr double kFirstCastTime = 0.3;
constexpr double kCastInterval = 1.5;
constexpr double kOomRetryInterval = 1.0;
constexpr double kRegenPeriod = 5.0;

// Expected damage of one Firebolt cast that spends mana (hit chance and
// crits folded in; resists assumed 0, no CoE).
inline double expected_damage_per_cast(double master_sp, int unholy_power, int improved_imp, bool modern_scaling = true) {
    double dmg = 0.0;
    const double pet_sp = kPetSpRatio * master_sp;
    if (modern_scaling) {
        dmg = kModernFireboltBaseDamage + (2.0 / 3.5) * pet_sp;
    } else {
        const double avg_base = 0.5 * (kFireboltMinDamage + kFireboltMaxDamage);
        dmg = avg_base + kCastTimeCoefficient * pet_sp;
    }
    dmg *= (1.0 + unholy_power * 0.02);
    dmg *= (1.0 + improved_imp * 0.10);
    dmg *= kHitChance;
    dmg *= (1.0 + kCritChance * (kCritMultiplier - 1.0));
    return dmg;
}

// Deterministic mirror of the sim's cast scheduling: how many Firebolts
// actually spend mana over a fight of the given duration.
inline int expected_cast_count(double fight_duration, bool mana_limited, bool modern_scaling = true) {
    if (fight_duration <= kFirstCastTime) return 0;
    const double interval = modern_scaling ? 2.0 : kCastInterval;
    if (!mana_limited) {
        return static_cast<int>(std::floor((fight_duration - kFirstCastTime) / interval)) + 1;
    }
    double mana = kBaseMana;
    double next_tick = kRegenPeriod;
    double t = kFirstCastTime;
    int casts = 0;
    while (t < fight_duration) {
        while (next_tick <= t) {
            mana = std::min(kBaseMana, mana + kBaseRegenPerTick);
            next_tick += kRegenPeriod;
        }
        if (mana >= kFireboltCost) {
            mana -= kFireboltCost;
            ++casts;
            t += interval;
        } else {
            t += kOomRetryInterval;
        }
    }
    return casts;
}

inline double expected_dps(double master_sp, int unholy_power, int improved_imp,
                           bool mana_limited, double fight_duration, bool modern_scaling = true) {
    if (fight_duration <= 0.0) return 0.0;
    const int casts = expected_cast_count(fight_duration, mana_limited, modern_scaling);
    return casts * expected_damage_per_cast(master_sp, unholy_power, improved_imp, modern_scaling) / fight_duration;
}

// ---------------------------------------------------------------------------
// Succubus: melee auto-attacks (free) + Lash of Pain (mana).
// Mirrors PET_MELEE_SWING and PET_CAST_FINISH for PetChoice::SUCCUBUS:
//   - Melee: 145-195 base + (bonus AP / 14) * 2.0, where bonus AP is 57% of
//     the master's Shadow spell power; x0.86 armor; 95% hit; 5% crit at 2x;
//     swings every 2.0s from 1.0s and never costs mana.
//   - Lash of Pain (Rank 6): 99-115 + (1.5 / 3.5) * 15% of master Shadow SP;
//     83% hit; 5% crit at 1.5x; cooldown max(3s, 9s - 1s per Improved Sayaad).
//   - Mana model: 1450 base pool, 160 mana per Lash, +45 mana every 5s
//     (base regen only), first Lash attempt at 0.5s, 1.5s retry while OOM.
// Talent axes: Unholy Power (+2% pet damage per point) and Improved Sayaad
// (+10% Lash damage per point, -1s Lash cooldown per point). Fel Vitality 0/3.

constexpr double kSuccubusMeleeMinDamage = 101.0;
constexpr double kSuccubusMeleeMaxDamage = 101.0;
constexpr double kPetApRatio = 1.0 / 6.0;    // 6 SP = 1 Pet AP (~16.67% SP to pet Attack Power)
constexpr double kMeleeArmorMultiplier = 0.86;
constexpr double kMeleeHitChance = 0.95;
constexpr double kMeleeCritChance = 0.05;
constexpr double kMeleeCritMultiplier = 2.0;
constexpr double kFirstSwingTime = 1.0;
constexpr double kSwingInterval = 2.0;
constexpr double kLopMinDamage = 50.0;
constexpr double kLopMaxDamage = 50.0;
constexpr double kSuccubusBaseMana = 1450.0;
constexpr double kLopCost = 160.0;
constexpr double kFirstLopTime = 0.5;
constexpr double kLopBaseCooldown = 12.0;
constexpr double kLopCooldownPerSayaad = 1.0;
constexpr double kLopMinCooldown = 3.0;
constexpr double kLopOomRetry = 1.5;

inline double succubus_lop_cooldown(int improved_sayaad) {
    return std::max(kLopMinCooldown, kLopBaseCooldown - kLopCooldownPerSayaad * improved_sayaad);
}

inline double succubus_expected_melee_per_swing(double master_sp, int unholy_power) {
    const double avg_base = 0.5 * (kSuccubusMeleeMinDamage + kSuccubusMeleeMaxDamage);
    const double bonus_ap = kPetApRatio * master_sp;
    double dmg = avg_base + (bonus_ap / 14.0) * 2.0;
    dmg *= (1.0 + unholy_power * 0.02);
    dmg *= kMeleeArmorMultiplier;
    dmg *= kMeleeHitChance;
    dmg *= (1.0 + kMeleeCritChance * (kMeleeCritMultiplier - 1.0));
    return dmg;
}

inline int succubus_expected_melee_swings(double fight_duration) {
    if (fight_duration <= kFirstSwingTime) return 0;
    return static_cast<int>(std::floor((fight_duration - kFirstSwingTime) / kSwingInterval)) + 1;
}

inline double succubus_expected_lop_per_cast(double master_sp, int unholy_power, int improved_sayaad) {
    const double avg_base = 0.5 * (kLopMinDamage + kLopMaxDamage);
    const double pet_sp = kPetSpRatio * master_sp;
    double dmg = avg_base + kCastTimeCoefficient * pet_sp;
    dmg *= (1.0 + unholy_power * 0.02);
    dmg *= (1.0 + improved_sayaad * 0.10);
    dmg *= kHitChance;
    dmg *= (1.0 + kCritChance * (kCritMultiplier - 1.0));
    return dmg;
}

inline int succubus_expected_lop_casts(double fight_duration, int improved_sayaad, bool mana_limited) {
    const double cd = succubus_lop_cooldown(improved_sayaad);
    if (fight_duration <= kFirstLopTime) return 0;
    if (!mana_limited) {
        return static_cast<int>(std::floor((fight_duration - kFirstLopTime) / cd)) + 1;
    }
    double mana = kSuccubusBaseMana;
    double next_tick = kRegenPeriod;
    double t = kFirstLopTime;
    int casts = 0;
    while (t < fight_duration) {
        while (next_tick <= t) {
            mana = std::min(kSuccubusBaseMana, mana + kBaseRegenPerTick);
            next_tick += kRegenPeriod;
        }
        if (mana >= kLopCost) {
            mana -= kLopCost;
            ++casts;
            t += cd;
        } else {
            t += kLopOomRetry;
        }
    }
    return casts;
}

inline double succubus_expected_dps(double master_sp, int unholy_power, int improved_sayaad,
                                    bool mana_limited, double fight_duration) {
    if (fight_duration <= 0.0) return 0.0;
    const double melee = succubus_expected_melee_swings(fight_duration) *
                         succubus_expected_melee_per_swing(master_sp, unholy_power);
    const double lop = succubus_expected_lop_casts(fight_duration, improved_sayaad, mana_limited) *
                       succubus_expected_lop_per_cast(master_sp, unholy_power, improved_sayaad);
    return (melee + lop) / fight_duration;
}

} // namespace imp_analysis
} // namespace warlock
