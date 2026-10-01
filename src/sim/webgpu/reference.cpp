// Small CPU implementation of the experimental slice, for scheduler/WASM comparison.
// The independent, full legacy simulator is tested by tests/webgpu/oracle.cpp.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <limits>
#include "combat_types.hpp"

static float random(State& s) {
    s.rng=s.rng*747796405u+2891336453u;
    const auto word=((s.rng>>((s.rng>>28u)+4u))^s.rng)*277803737u;
    return float(((word>>22u)^word)>>8u)*(1.0f/16777216.0f);
}

static uint32_t micros(float x) { return uint32_t(std::round(x*1000000.0f)); }

static inline uint32_t get_rule(const Config& c, uint32_t idx) {
    switch (idx) {
        case 0u: return static_cast<uint32_t>(c.rule0);
        case 1u: return static_cast<uint32_t>(c.rule1);
        case 2u: return static_cast<uint32_t>(c.rule2);
        case 3u: return static_cast<uint32_t>(c.rule3);
        case 4u: return static_cast<uint32_t>(c.rule4);
        case 5u: return static_cast<uint32_t>(c.rule5);
        case 6u: return static_cast<uint32_t>(c.rule6);
        case 7u: return static_cast<uint32_t>(c.rule7);
        case 8u: return static_cast<uint32_t>(c.rule8);
        case 9u: return static_cast<uint32_t>(c.rule9);
        case 10u: return static_cast<uint32_t>(c.rule10);
        case 11u: return static_cast<uint32_t>(c.rule11);
        case 12u: return static_cast<uint32_t>(c.rule12);
        case 13u: return static_cast<uint32_t>(c.rule13);
        case 14u: return static_cast<uint32_t>(c.rule14);
        case 15u: return static_cast<uint32_t>(c.rule15);
        default: return 255u;
    }
}

static float dot_remaining(const State& s, uint32_t next_tick, uint32_t ticks, float interval) {
    if (ticks == 0u || next_tick == UINT32_MAX) return 0.0f;
    return static_cast<float>(next_tick - s.now) * 0.000001f + static_cast<float>(ticks - 1u) * interval;
}

static State simulate(const Config& c, uint32_t replica, uint32_t seed, uint32_t step) {
    State s;
    s.rng = seed ^ ((replica + 1u) * 2246822519u);
    s.mana = c.maxMana;
    s.petMana = c.petManaMax;
    const auto later = [&](float delay) {
        auto n = micros(delay);
        if (n >= (UINT32_MAX - s.now)) return UINT32_MAX;
        return s.now + (step ? ((n + step - 1) / step) * step : n);
    };
    const auto shadow_power = [&]() {
        float sp = c.shadowPower + ((s.now < s.trinketEnd) ? c.trinketBonus : 0.0f);
        if (c.racialType == 1.0f && s.now < s.racialEnd) {
            sp += c.racialBonus;
        }
        return sp;
    };
    const auto fire_power = [&]() {
        float sp = c.firePower + ((s.now < s.trinketEnd) ? c.trinketBonus : 0.0f);
        if (c.racialType == 1.0f && s.now < s.racialEnd) {
            sp += c.racialBonus;
        }
        return sp;
    };


    const auto cast_dur = [&](float dur) {
        if (c.racialType == 2.0f && s.now < s.racialEnd) {
            return dur / (1.0f + c.racialBonus * 0.01f);
        }
        return dur;
    };
    const auto get_mana_cost = [&](float cost) {
        if (c.racialType == 3.0f && s.racialEnd > 0u) {
            return cost * 0.90f;
        }
        return cost;
    };
    const auto apply_racial_dmg_mods = [&](float raw_dmg) {
        float dmg = raw_dmg;
        if (c.racialType == 2.0f && c.racialPad0 > 1.0f) dmg *= c.racialPad0;
        if (c.racialType == 3.0f && s.racialEnd > 0u) {
            dmg *= (1.0f + c.racialBonus);
            s.racialEnd--;
        }
        return dmg;
    };

    if (c.petType != 0.0f) {
        s.petNext = later(c.petType == 2.0f ? 1.0f : 0.5f);
        s.petLopNext = (c.petType == 2.0f) ? later(0.5f) : UINT32_MAX;
    }

    const auto apply_isb = [&](bool is_all_shadow) -> float {
        const bool isb_active = (s.isbExpire > s.now) && (c.isbCharges == 0.0f || s.isbCharges > 0);
        if (isb_active && is_all_shadow) {
            s.isbConsumed++;
            if (c.isbCharges > 0.0f) {
                s.isbCharges--;
                if (s.isbCharges == 0) s.isbExpire = 0;
            }
            return 1.0f + c.isbBonus;
        }
        return 1.0f;
    };

    const auto apply_spell_resist = [&](float damage) -> float {
        if (c.partialResistEnabled == 0.0f) return damage;
        const float effective = std::max(0.0f, c.targetResist - c.spellPen);
        const float avgResist = effective / (effective + 400.0f + 85.0f * 60.0f);
        float dmg = damage * (1.0f - avgResist);
        if (c.petSpellPiercing > 0.0f && effective <= 0.0f) {
            dmg *= (1.0f + c.spellPen * c.petPiercingBonus);
        }
        return dmg;
    };

    const auto apply_touch_of_the_grave = [&]() {
        if (c.touchOfTheGraveDamage > 0.0f && s.now >= s.totgCdReady && random(s) < 0.10f) {
            s.totgCdReady = later(1.0f);
            s.totgProcs++;
            s.totgDamage += c.touchOfTheGraveDamage;
        }
    };

    const auto bolt = [&]() {
        apply_touch_of_the_grave();
        s.bolts++;
        if (random(s) >= c.hit) { s.misses++; return; }
        float damage = (c.boltMin + random(s) * (c.boltMax - c.boltMin) + (3.0f / 3.5f) * shadow_power()) * c.shadowBoltMultiplier;
        if (s.snfShadowEnd > s.now) damage *= (1.0f + c.snfShadowBonus);
        if (s.now >= micros(c.duration * 0.65f)) damage *= 1.0f + c.executeBonus;
        damage *= apply_isb(true);
        damage = apply_spell_resist(damage);
        if (random(s) < c.crit) {
            damage *= c.boltCrit;
            s.crits++;
            if (c.isbBonus > 0.0f) {
                s.isbExpire = later(12.0f);
                s.isbCharges = uint32_t(c.isbCharges);
                s.isbProcs++;
            }
        }
        damage = apply_racial_dmg_mods(damage);
        s.boltDamage += damage;
    };

    const auto incinerate = [&]() {
        apply_touch_of_the_grave();
        s.incinerates++;
        if (random(s) >= c.hit) { s.misses++; s.incinerateMisses++; return; }
        s.incinerateHits++;
        float damage = (201.0f + random(s) * 32.0f + (2.5f / 3.5f) * fire_power()) * c.conflagMultiplier;
        if (s.immolateTicks > 0) damage *= 1.25f;
        if (s.snfFireEnd > s.now) damage *= (1.0f + c.snfFireBonus);
        damage = apply_spell_resist(damage);
        if (random(s) < c.fireCrit) {
            damage *= c.boltCrit;
            s.crits++;
            s.incinerateCrits++;
        }
        damage = apply_racial_dmg_mods(damage);
        s.incinerateDamage += damage;
    };

    const uint32_t end = micros(c.duration);
    while (s.now < end) {
        if (c.trinketEnabled > 0.0f && c.trinketCooldown > 0.0f && s.now >= s.trinketReady) {
            s.trinketEnd = later(c.trinketDuration);
            s.trinketReady = later(c.trinketCooldown);
        }
        if ((c.racialType == 1.0f || c.racialType == 2.0f || c.racialType == 3.0f) && c.racialCooldown > 0.0f && s.now >= s.racialReady) {
            const bool execute_phase = (s.now >= micros(c.duration * 0.65f));
            bool should_trigger = false;
            const uint32_t pol = static_cast<uint32_t>(c.racialPolicy);
            if (pol == 0u) { // ON_COOLDOWN
                should_trigger = true;
            } else if (pol == 1u) { // EXECUTE_ONLY
                should_trigger = execute_phase;
            } else if (pol == 2u) { // ALIGN_DOOM
                float doom_time_left = 999.0f;
                if (s.doomNext != UINT32_MAX && s.doomNext > s.now) {
                    doom_time_left = static_cast<float>(s.doomNext - s.now) * 0.000001f;
                }
                const float window = (c.racialType == 3.0f) ? 6.0f : ((c.racialType == 2.0f) ? 10.0f : 14.0f);
                if (doom_time_left <= window || execute_phase) {
                    should_trigger = true;
                }
            } else if (pol == 3u) { // ALIGN_EXECUTE
                const float execute_start = c.duration * 0.65f;
                if (s.now < 1000000u && execute_start >= c.racialCooldown) {
                    should_trigger = true;
                } else {
                    should_trigger = execute_phase;
                }
            }
            if (should_trigger) {
                s.racialEnd = (c.racialType == 3.0f) ? static_cast<uint32_t>(c.racialDuration) : later(c.racialDuration);
                s.racialReady = later(c.racialCooldown);
            }
        }


        // 1. Mana Regeneration
        if (s.regenNext <= s.now) {
            s.mana = std::min(c.maxMana, s.mana + c.mp5);
            if (c.petManaManagement > 0 && c.petType != 0) {
                s.petMana = std::min(c.petManaMax, s.petMana + c.petMp5);
            }
            s.regenNext = later(5.0f);
            s.events++;
        }
        // 2. Periodic DoT Ticks
        if (s.dotNext <= s.now) {
            float damage = (c.dotBase + shadow_power() * c.corruptionSpCoeff) * c.corruptionMultiplier;
            if (s.snfShadowEnd > s.now) damage *= (1.0f + c.snfShadowBonus);
            if (s.wrackEnd > s.now) damage *= 1.10f;
            damage *= apply_isb(true);
            if (random(s) < c.crit) { damage *= c.dotCrit; s.crits++; s.corruptionCrits++; }
            s.dotDamage += damage; s.ticks++; s.corruptionTicks++; s.dotTicks--; s.events++;
            if (random(s) < c.nightfall) { s.tranceEnd = later(10.0f); s.procs++; }
            s.dotNext = s.dotTicks ? later(3.0f) : UINT32_MAX;
        }
        if (s.agonyNext <= s.now) {
            s.events++;
            const uint32_t tick_idx = 12u - s.agonyTicks + 1u;
            float ramp = 1.0f;
            if (tick_idx <= 4u) ramp = 0.50f;
            else if (tick_idx > 8u) ramp = 1.50f;
            float agony_base = c.agonyBase;
            if (c.ampCurse > 0.5f && s.agonies == 1) agony_base *= 1.5f;
            float damage = (agony_base + shadow_power() * (1.596f / 12.0f)) * ramp * c.agonyMultiplier;
            if (s.snfShadowEnd > s.now) damage *= (1.0f + c.snfShadowBonus);
            if (s.wrackEnd > s.now) damage *= 1.10f;
            damage *= apply_isb(true);
            if (random(s) < c.crit) { damage *= c.dotCrit; s.crits++; }
            s.agonyDamage += damage; s.ticks++; s.agonyTicks--;
            s.agonyNext = s.agonyTicks ? later(2.0f) : UINT32_MAX;
        }
        if (s.siphonNext <= s.now) {
            s.events++;
            float damage = (c.siphonBase + shadow_power() * 0.05f) * c.siphonMultiplier;
            if (s.snfShadowEnd > s.now) damage *= (1.0f + c.snfShadowBonus);
            if (s.wrackEnd > s.now) damage *= 1.10f;
            damage *= apply_isb(true);
            if (random(s) < c.crit) { damage *= c.dotCrit; s.crits++; }
            s.siphonDamage += damage; s.ticks++; s.siphonTicks--;
            s.siphonNext = s.siphonTicks ? later(3.0f) : UINT32_MAX;
        }
        if (s.immolateNext <= s.now) {
            s.events++;
            float damage = (c.immolateDotBase + fire_power() * 0.13f) * c.immolateMultiplier;
            if (s.snfFireEnd > s.now) damage *= (1.0f + c.snfFireBonus);
            if (random(s) < c.fireCrit) { damage *= c.boltCrit; s.crits++; }
            s.immolateDamage += damage; s.ticks++; s.immolateTicks--; s.immolateDotHits++;
            s.immolateNext = s.immolateTicks ? later(3.0f) : UINT32_MAX;
        }
        if (s.wrackNext <= s.now) {
            s.events++;
            const uint32_t aff_effects = static_cast<uint32_t>(s.dotTicks > 0) +
                static_cast<uint32_t>(s.agonyTicks > 0) + static_cast<uint32_t>(s.siphonTicks > 0);
            float damage = (c.wrackBase + shadow_power() * c.wrackSpCoeff) * c.wrackMultiplier *
                (1.0f + c.wrackSoulSiphonPerEffect * aff_effects);
            if (s.snfShadowEnd > s.now) damage *= (1.0f + c.snfShadowBonus);
            damage *= apply_isb(true);
            damage = apply_spell_resist(damage);
            if (random(s) < c.crit) { damage *= c.dotCrit; s.crits++; s.wrackCrits++; }
            s.wrackDamage += damage; s.wrackHits++; s.ticks++;
            if (c.nightfall > 0.0f && random(s) < c.nightfall) { s.tranceEnd = later(10.0f); s.procs++; }
            s.wrackTicks--;
            s.wrackNext = s.wrackTicks ? later(c.wrackTickInterval) : UINT32_MAX;
        }
        if (s.doomNext <= s.now) {
            s.events++;
            float damage = (c.doomBase + shadow_power() * 4.0f) * c.doomMultiplier;
            if (s.snfShadowEnd > s.now) damage *= (1.0f + c.snfShadowBonus);
            if (s.wrackEnd > s.now) damage *= 1.10f;
            damage *= apply_isb(true);
            damage = apply_spell_resist(damage);
            if (random(s) < c.crit) { damage *= c.dotCrit; s.crits++; s.doomCrits++; }
            s.doomDamage += damage; s.doomHits++; s.ticks++;
            s.doomNext = UINT32_MAX;
        }
        // 3. Pet Actions
        if (s.petNext <= s.now && c.petType != 0.0f) {
            s.events++;
            if (c.petType == 1.0f) {
                // Imp Firebolt
                if (c.petManaManagement == 0.0f || s.petMana >= c.petLopCost) {
                    s.petCasts++;
                    if (c.petManaManagement > 0.0f) s.petMana -= c.petLopCost;
                    if (random(s) >= c.hit) {
                        // Miss
                    } else {
                        s.petHits++;
                        float damage = (c.petBaseMin + random(s) * (c.petBaseMax - c.petBaseMin) + c.firePower * c.petSpRatio + c.petFlatSP) * c.petMultiplier;
                        if (random(s) < c.fireCrit) { damage *= 1.5f; s.petCrits++; }
                        damage = apply_spell_resist(damage);
                        if (c.demonicBrandRank > 0.0f && s.demonicBrandCharges > 0u && s.now < s.demonicBrandExpire) {
                            s.demonicBrandCharges--;
                            float brand = (65.0f + random(s) * 3.0f + fire_power() * 0.078f) * c.demonicBrandMultiplier;
                            brand = apply_spell_resist(brand);
                            s.demonicBrandDamage += brand;
                            damage += brand;
                        }
                        s.petDamage += damage;
                    }
                    s.petNext = later(c.petCastInterval);
                } else {
                    s.petNext = later(1.0f);
                }
            } else if (c.petType == 2.0f) {
                // Succubus Melee
                s.petCasts++;
                s.petMeleeCasts++;
                const float roll = random(s) * 100.0f;
                const float miss = c.petMeleeMissPct;
                if (roll < miss + c.petMeleeDodgePct) {
                    // Miss / Dodge
                } else {
                    s.petHits++;
                    s.petMeleeHits++;
                    float damage = (c.petMeleeBase + c.shadowPower * c.petApRatio) * c.petMeleeMultiplier * c.petArmorMultiplier;
                    const bool glance = roll < miss + c.petMeleeDodgePct + c.petGlancePct;
                    const bool crit = !glance && roll < miss + c.petMeleeDodgePct + c.petGlancePct + std::max(0.0f, c.petMeleeCritPct);
                    if (glance) damage *= c.petGlanceMultiplier;
                    if (crit) { damage *= 2.0f; s.petCrits++; s.petMeleeCrits++; }
                    s.petMeleeDamage += damage;
                    if (c.demonicBrandRank > 0.0f && s.demonicBrandCharges > 0u && s.now < s.demonicBrandExpire) {
                        s.demonicBrandCharges--;
                        float brand = (65.0f + random(s) * 3.0f + shadow_power() * 0.078f) * c.demonicBrandMultiplier;
                        brand = apply_spell_resist(brand);
                        s.demonicBrandDamage += brand;
                        damage += brand;
                    }
                    s.petDamage += damage;
                }
                s.petNext = later(c.petCastInterval);
            }
        }
        if (s.petLopNext <= s.now && c.petType == 2.0f) {
            s.events++;
            if (c.petManaManagement == 0.0f || s.petMana >= c.petLopCost) {
                s.petCasts++; s.petLopCasts++;
                if (c.petManaManagement > 0.0f) s.petMana -= c.petLopCost;
                if (random(s) >= c.petSpellHit) {
                    // Miss
                } else {
                    s.petHits++; s.petLopHits++;
                    float damage = (c.petLopBase + c.shadowPower * c.petLopSpRatio + c.petFlatSP) * c.petLopMultiplier;
                    if (random(s) < c.petSpellCrit) { damage *= 1.5f; s.petCrits++; s.petLopCrits++; }
                    damage = apply_spell_resist(damage);
                    s.petLopDamage += damage;
                    if (c.demonicBrandRank > 0.0f && s.demonicBrandCharges > 0u && s.now < s.demonicBrandExpire) {
                        s.demonicBrandCharges--;
                        float brand = (65.0f + random(s) * 3.0f + shadow_power() * 0.078f) * c.demonicBrandMultiplier;
                        brand = apply_spell_resist(brand);
                        s.demonicBrandDamage += brand;
                        damage += brand;
                    }
                    s.petDamage += damage;
                }
                s.petLopNext = later(c.petLopCd);
            } else {
                s.petLopNext = later(1.5f);
            }
        }
        // 4. Player Cast Completion
        if (s.castEnd <= s.now) {
            s.events++;
            if ((s.castSpellId == 1u || s.castSpellId == 5u) && c.executeBonus > 0.0f && s.now >= micros(c.duration * 0.65f)) {
                s.decimationExpire = later(10.0f);
            }
            if (s.castSpellId == 2u) {
                apply_touch_of_the_grave();
                if (random(s) >= c.hit) { s.misses++; }
                else {
                    float damage = (c.immolateMin + random(s) * (c.immolateMax - c.immolateMin) + fire_power() * 0.20f) * c.immolateDirectMultiplier;
                    if (s.snfFireEnd > s.now) damage *= (1.0f + c.snfFireBonus);
                    damage = apply_spell_resist(damage);
                    if (random(s) < c.fireCrit) { damage *= c.boltCrit; s.crits++; }
                    s.immolateDamage += damage;
                    s.immolateDirectHits++;
                    s.immolateTicks = 5u;
                    s.immolateNext = later(3.0f);
                }
            } else if (s.castSpellId == 3u) {
                apply_touch_of_the_grave();
                s.mana = std::max(0.0f, s.mana - c.corruptionCost);
                if (random(s) >= c.hit) s.misses++;
                else { s.dotTicks = 6u; s.dotNext = later(3.0f); }
            } else if (s.castSpellId == 4u) {
                incinerate();
            } else if (s.castSpellId == 5u) {
                apply_touch_of_the_grave();
                if (random(s) >= c.hit) { s.misses++; }
                else {
                    s.searingHits++;
                    float damage = (108.0f + random(s) * 19.0f + (1.5f / 3.5f) * fire_power()) * c.searingMultiplier;
                    if (s.now >= micros(c.duration * 0.65f)) damage *= (1.0f + c.executeBonus);
                    if (random(s) < std::min(1.0f, c.fireCrit + c.searingCritBonus)) { damage *= c.boltCrit; s.crits++; s.searingCrits++; }
                    damage = apply_spell_resist(damage);
                    s.searingDamage += damage;
                    if (c.demonicBrandRank > 0.0f) {
                        s.demonicBrandCharges = uint32_t(c.demonicBrandRank) * 2u;
                        s.demonicBrandExpire = later(10.0f);
                    }
                }
            } else if (s.castSpellId == 6u) {
                apply_touch_of_the_grave();
                s.soulFireCdReady = later(c.soulFireCooldown);
                if (random(s) >= c.hit) { s.misses++; }
                else {
                    s.soulFireHits++;
                    float damage = (383.0f + random(s) * 96.0f + fire_power()) * c.soulFireMultiplier;
                    damage = apply_spell_resist(damage);
                    if (random(s) < c.fireCrit) { damage *= c.boltCrit; s.crits++; s.soulFireCrits++; }
                    s.soulFireDamage += damage;
                }
            } else {
                bolt();
            }
            s.castEnd = UINT32_MAX; s.castSpellId = 0u;
        }
        // 5. Player Decision via Action Priority List (APL)
        if (s.ready <= s.now && s.castEnd == UINT32_MAX) {
            s.events++;
            bool acted = false;
            const uint32_t num_rules = static_cast<uint32_t>(c.ruleCount);
            for (uint32_t r_idx = 0u; r_idx < num_rules; ++r_idx) {
                const uint32_t act = get_rule(c, r_idx);
                if (act == 0u) { // LIFE_TAP
                    if (s.mana <= c.maxMana * c.tapThreshold) {
                        s.mana = std::min(c.maxMana, s.mana + c.tapGain);
                        if (c.petTapGain > 0.0f && c.petType != 0.0f) s.petMana = std::min(c.petManaMax, s.petMana + c.petTapGain);
                        s.taps++; s.ready = later(c.gcd); acted = true; break;
                    }
                } else if (act == 8u) { // NIGHTFALL_SHADOW_BOLT
                    if (s.tranceEnd > s.now && s.mana >= get_mana_cost(c.boltCost)) {
                        s.tranceEnd = 0u; s.consumed++; s.mana -= get_mana_cost(c.boltCost); bolt(); s.ready = later(c.gcd); acted = true; break;
                    }
                } else if (act == 9u) { // DECIMATION_SEARING_PAIN
                    if (c.executeBonus > 0.0f && s.now >= micros(c.duration * 0.65f) && s.decimationExpire <= s.now && s.mana >= get_mana_cost(c.searingCost)) {
                        s.mana -= get_mana_cost(c.searingCost); s.searingCasts++;
                        s.castEnd = later(cast_dur(c.searingCastTime)); s.castSpellId = 5u;
                        s.ready = std::max(later(c.gcd), s.castEnd); acted = true; break;
                    }
                } else if (act == 11u) { // DEMONIC_BRAND_SEARING_PAIN
                    if (c.demonicBrandRank > 0.0f && (s.demonicBrandCharges == 0u || s.now >= s.demonicBrandExpire) && s.mana >= get_mana_cost(c.searingCost)) {
                        s.mana -= get_mana_cost(c.searingCost); s.searingCasts++;
                        s.castEnd = later(cast_dur(c.searingCastTime)); s.castSpellId = 5u;
                        s.ready = std::max(later(c.gcd), s.castEnd); acted = true; break;
                    }
                } else if (act == 10u) { // DECIMATION_SOUL_FIRE
                    if (c.executeBonus > 0.0f && s.now >= micros(c.duration * 0.65f) && s.decimationExpire > s.now &&
                        s.now >= s.soulFireCdReady && s.mana >= get_mana_cost(c.soulFireCost)) {
                        s.mana -= get_mana_cost(c.soulFireCost); s.soulFires++;
                        s.castEnd = later(cast_dur(c.soulFireCastTime)); s.castSpellId = 6u;
                        s.ready = std::max(later(c.gcd), s.castEnd); acted = true; break;
                    }
                } else if (act == 12u) { // CORRUPTION
                if (c.corruptionCost > 0.0f && (s.dotTicks == 0u || (c.corruptionRefreshSec > 0.0f && dot_remaining(s, s.dotNext, s.dotTicks, 3.0f) <= c.corruptionRefreshSec)) && s.mana >= get_mana_cost(c.corruptionCost)) {
                        s.dots++;
                        if (c.corruptionCastTime == 0.0f) {
                            apply_touch_of_the_grave();
                            s.mana -= get_mana_cost(c.corruptionCost);
                            if (random(s) < c.hit) { s.dotTicks = 6u; s.dotNext = later(3.0f); } else { s.misses++; }
                            s.ready = later(c.gcd);
                        } else {
                            s.castEnd = later(cast_dur(c.corruptionCastTime)); s.castSpellId = 3u;
                            s.ready = std::max(later(c.gcd), s.castEnd);
                        }
                        acted = true; break;
                    }
                } else if (act == 5u) { // CURSE_OF_AGONY
                if (c.agonyCost > 0.0f && (s.agonyTicks == 0u || (c.agonyRefreshSec > 0.0f && dot_remaining(s, s.agonyNext, s.agonyTicks, 2.0f) <= c.agonyRefreshSec)) && s.doomNext == UINT32_MAX && s.mana >= get_mana_cost(c.agonyCost)) {
                        apply_touch_of_the_grave();
                        s.mana -= get_mana_cost(c.agonyCost); s.agonies++;
                        if (random(s) < c.hit) { s.agonyTicks = 12u; s.agonyNext = later(2.0f); } else { s.misses++; }
                        s.ready = later(c.gcd); acted = true; break;
                    }
                } else if (act == 6u) { // CURSE_OF_DOOM
                    const uint32_t doom_min_remaining_us = static_cast<uint32_t>(c.doomMinTimeRemaining * 1000000.0f);
                    if (c.doomCost > 0.0f && s.doomNext == UINT32_MAX && s.agonyTicks == 0u && (c.agonyCost == 0.0f || s.now + doom_min_remaining_us <= end) && s.mana >= get_mana_cost(c.doomCost)) {
                        apply_touch_of_the_grave();
                        s.mana -= get_mana_cost(c.doomCost); s.dooms++;
                        if (random(s) < c.hit) { s.doomNext = later(60.0f); } else { s.misses++; }
                        s.ready = later(c.gcd); acted = true; break;
                    }
                } else if (act == 13u) { // SIPHON_LIFE
                if (c.siphonCost > 0.0f && (s.siphonTicks == 0u || (c.siphonRefreshSec > 0.0f && dot_remaining(s, s.siphonNext, s.siphonTicks, 3.0f) <= c.siphonRefreshSec)) && s.mana >= get_mana_cost(c.siphonCost)) {
                        apply_touch_of_the_grave();
                        s.mana -= get_mana_cost(c.siphonCost); s.siphons++;
                        if (random(s) < c.hit) { s.siphonTicks = 10u; s.siphonNext = later(3.0f); } else { s.misses++; }
                    s.ready = later(c.gcd); acted = true; break;
                }
                } else if (act == 14u) { // WRACK (legacy enum name: DRAIN_HOPE)
                    if (c.wrackCost > 0.0f && s.wrackEnd <= s.now && s.mana >= get_mana_cost(c.wrackCost)) {
                        apply_touch_of_the_grave();
                        s.mana -= get_mana_cost(c.wrackCost); s.wracks++;
                        s.wrackTicks = 6u; s.wrackNext = later(c.wrackTickInterval);
                        s.wrackEnd = later(cast_dur(c.wrackChannelTime));
                        s.ready = std::max(later(c.gcd), s.wrackEnd);
                        acted = true; break;
                    }
                } else if (act == 15u) { // IMMOLATE
                if (c.immolateCost > 0.0f && (s.immolateTicks == 0u || (c.immolateRefreshSec > 0.0f && dot_remaining(s, s.immolateNext, s.immolateTicks, 3.0f) <= c.immolateRefreshSec)) && s.mana >= get_mana_cost(c.immolateCost)) {
                        s.mana -= get_mana_cost(c.immolateCost); s.immolates++;
                        s.castEnd = later(cast_dur(c.immolateCastTime)); s.castSpellId = 2u;
                        s.ready = std::max(later(c.gcd), s.castEnd); acted = true; break;
                    }
                } else if (act == 16u) { // CONFLAGRATE
                    if (c.conflagCost > 0.0f && s.immolateTicks > 0u && s.now >= s.conflagrateCdReady && s.mana >= get_mana_cost(c.conflagCost)) {
                        apply_touch_of_the_grave();
                        s.mana -= get_mana_cost(c.conflagCost); s.conflagrates++; s.conflagrateCdReady = later(10.0f);
                        if (random(s) >= c.hit) { s.misses++; }
                        else {
                            float dmg = (c.conflagMin + random(s) * (c.conflagMax - c.conflagMin) + (1.5f / 3.5f) * fire_power()) * c.conflagMultiplier;
                            if (s.snfFireEnd > s.now) dmg *= (1.0f + c.snfFireBonus);
                            if (random(s) < c.fireCrit + c.fnbCritBonus) { dmg *= c.boltCrit; s.crits++; }
                            dmg = apply_spell_resist(dmg);
                            dmg = apply_racial_dmg_mods(dmg);
                            s.conflagrateDamage += dmg;
                            if (c.snfShadowBonus > 0.0f) { s.snfShadowEnd = later(20.0f); }
                        }
                        if (c.snfConflagSaveChance == 0.0f || random(s) >= c.snfConflagSaveChance) { s.immolateTicks = 0u; s.immolateNext = UINT32_MAX; }
                        s.ready = later(c.gcd); acted = true; break;
                    }
                } else if (act == 17u || act == 18u) { // SHADOWBURN / SHADOWBURN_ISB
                    if (c.shadowburnCost > 0.0f && s.now >= s.shadowburnCdReady && s.mana >= get_mana_cost(c.shadowburnCost)) {
                        apply_touch_of_the_grave();
                        s.mana -= get_mana_cost(c.shadowburnCost); s.shadowburns++; s.shadowburnCdReady = later(15.0f);
                        if (random(s) >= c.hit) { s.misses++; }
                        else {
                            float dmg = (c.sburnMin + random(s) * (c.sburnMax - c.sburnMin) + (1.5f / 3.5f) * shadow_power()) * c.shadowburnMultiplier;
                            if (s.snfShadowEnd > s.now) dmg *= (1.0f + c.snfShadowBonus);
                            dmg *= apply_isb(true);
                            if (random(s) < c.crit) { dmg *= c.boltCrit; s.crits++; }
                            dmg = apply_spell_resist(dmg);
                            dmg = apply_racial_dmg_mods(dmg);
                            s.shadowburnDamage += dmg;
                            if (c.snfFireBonus > 0.0f) { s.snfFireEnd = later(20.0f); }
                        }
                        s.ready = later(c.gcd); acted = true; break;
                    }
                } else if (act == 23u || act == 24u) { // SHADOW_BOLT_FILLER / RANK2
                    if (s.mana >= get_mana_cost(c.boltCost)) {
                        s.mana -= get_mana_cost(c.boltCost);
                        s.castEnd = later(cast_dur(c.castTime)); s.castSpellId = 1u;
                        s.ready = std::max(later(c.gcd), s.castEnd); acted = true; break;
                    }
                } else if (act == 19u) { // INCINERATE_FILLER
                    if (s.mana >= get_mana_cost(c.boltCost)) {
                        s.mana -= get_mana_cost(c.boltCost);
                        s.castEnd = later(cast_dur(c.castTime)); s.castSpellId = 4u;
                        s.ready = std::max(later(c.gcd), s.castEnd); acted = true; break;
                    }
                } else if (act == 20u) { // SEARING_PAIN_FILLER
                    if (c.searingCost > 0.0f && s.mana >= get_mana_cost(c.searingCost)) {
                        s.mana -= get_mana_cost(c.searingCost);
                        s.castEnd = later(cast_dur(c.searingCastTime)); s.castSpellId = 5u;
                        s.ready = std::max(later(c.gcd), s.castEnd); acted = true; break;
                    }
                }
            }
            if (!acted) {
                const float filler_cost = get_mana_cost(c.fillerType == 2.0f ? c.searingCost : c.boltCost);
                if (s.mana >= filler_cost) {
                    s.mana -= filler_cost;
                    s.castEnd = later(cast_dur(c.castTime)); s.castSpellId = (c.fillerType == 1.0f ? 4u : (c.fillerType == 2.0f ? 5u : 1u));
                    s.ready = std::max(later(c.gcd), s.castEnd);
                } else {
                    s.mana = std::min(c.maxMana, s.mana + c.tapGain);
                    if (c.petTapGain > 0.0f && c.petType != 0.0f) {
                        s.petMana = std::min(c.petManaMax, s.petMana + c.petTapGain);
                    }
                    s.taps++;
                    s.ready = later(c.gcd);
                }
            }

        }
        s.loops++;
        const uint32_t next_dot = std::min({s.dotNext, s.agonyNext, s.doomNext, s.siphonNext, s.immolateNext, s.wrackNext});
        const uint32_t next_pet = std::min(s.petNext, s.petLopNext);
        const uint32_t next_trinket = c.trinketEnabled > 0.0f ? s.trinketReady : UINT32_MAX;
        const uint32_t next_now = step ? s.now + step : std::min({s.ready, s.castEnd, next_dot, next_pet, s.regenNext, next_trinket});
        if (s.isbExpire > s.now && (c.isbCharges == 0.0f || s.isbCharges > 0)) {
            const uint32_t active_end = std::min(next_now, s.isbExpire);
            if (active_end > s.now) s.isbUptimeUs += float(active_end - s.now);
        }
        s.now = next_now;
    }
    s.done = 1;
    return s;
}

extern "C" {
int simulate_batch(const Config* configs, uint32_t candidateCount,
                   uint32_t replicas, uint32_t seed, uint32_t step,
                   State* output) {
    if (!configs || !output || candidateCount == 0 || replicas == 0) return -1;
    for (uint32_t cand = 0; cand < candidateCount; ++cand) {
        const Config& c = configs[cand];
        for (uint32_t rep = 0; rep < replicas; ++rep) {
            output[cand * replicas + rep] = simulate(c, rep, seed + rep, step);
        }
    }
    return 0;
}
}
