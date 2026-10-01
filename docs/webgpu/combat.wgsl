// Experimental single-target slice. All timestamps are integer microseconds.
// Both schedulers execute these same transitions. No host round-trip per event.
struct Config {
    duration: f32, castTime: f32, gcd: f32, maxMana: f32,
    spellPower: f32, hit: f32, crit: f32, boltCrit: f32,
    dotCrit: f32, tapGain: f32, mp5: f32, tapThreshold: f32,
    boltCost: f32, corruptionCost: f32, agonyCost: f32, doomCost: f32,
    siphonCost: f32, immolateCost: f32, conflagCost: f32, shadowburnCost: f32,
    boltMin: f32, boltMax: f32, dotBase: f32, dotMultiplier: f32,
    isbBonus: f32, isbCharges: f32, isbAllShadow: f32, executeBonus: f32,
    agonyBase: f32, agonyMultiplier: f32, doomBase: f32, doomMultiplier: f32,
    siphonBase: f32, siphonMultiplier: f32, immolateCastTime: f32, immolateMultiplier: f32,
    immolateDirectMultiplier: f32,
    immolateMin: f32, immolateMax: f32, immolateDotBase: f32, corruptionCastTime: f32,
    corruptionMultiplier: f32, corruptionSpCoeff: f32, shadowBoltMultiplier: f32, shadowPower: f32,
    firePower: f32, fireCrit: f32, conflagMultiplier: f32, shadowburnMultiplier: f32,
    snfShadowBonus: f32, snfFireBonus: f32, snfConflagSaveChance: f32, fnbCritBonus: f32,
    conflagMin: f32, conflagMax: f32, sburnMin: f32, sburnMax: f32,
    petType: f32, petCastInterval: f32, petBaseMin: f32, petBaseMax: f32,
    petSpRatio: f32, petMultiplier: f32, petLopBase: f32, petLopSpRatio: f32,
    petLopCd: f32, petMeleeBase: f32, petApRatio: f32, petArmorMultiplier: f32,
    petSpellHit: f32, petSpellCrit: f32, petMeleeMissPct: f32, petMeleeDodgePct: f32,
    petGlancePct: f32, petGlanceMultiplier: f32, petMeleeMultiplier: f32, petLopMultiplier: f32,
    petLopCost: f32, petManaManagement: f32, petManaMax: f32, petMp5: f32,
    petFlatSP: f32, petMeleeCritPct: f32, petSpellPiercing: f32, petPiercingBonus: f32,
    targetResist: f32, spellPen: f32, partialResistEnabled: f32, petTapGain: f32,
    ampCurse: f32, fillerType: f32, ruleCount: f32, nightfall: f32,
    rule0: f32, rule1: f32, rule2: f32, rule3: f32,
    rule4: f32, rule5: f32, rule6: f32, rule7: f32,
    rule8: f32, rule9: f32, rule10: f32, rule11: f32,
    rule12: f32, rule13: f32, rule14: f32, rule15: f32,
    trinketBonus: f32, trinketDuration: f32, trinketCooldown: f32, trinketEnabled: f32,
    searingCost: f32, searingCastTime: f32, searingMultiplier: f32, searingCritBonus: f32,
    agonyRefreshSec: f32, corruptionRefreshSec: f32, siphonRefreshSec: f32, immolateRefreshSec: f32,
    wrackCost: f32, wrackBase: f32, wrackSpCoeff: f32, wrackMultiplier: f32,
    wrackSoulSiphonPerEffect: f32, wrackChannelTime: f32, wrackTickInterval: f32,
    doomMinTimeRemaining: f32,
    demonicBrandRank: f32, demonicBrandMultiplier: f32,
    soulFireCost: f32, soulFireCastTime: f32, soulFireCooldown: f32, soulFireMultiplier: f32,
    touchOfTheGraveDamage: f32,
    racialType: f32, racialPolicy: f32, racialDuration: f32, racialCooldown: f32,
    racialBonus: f32, racialPad0: f32, racialPad1: f32, racialPad2: f32,
}
struct Params { count: u32, perCandidate: u32, seed: u32, pad: u32 }
struct State {
    initialized: u32, rng: u32, now: u32, done: u32,
    ready: u32, castEnd: u32, castSpellId: u32, regenNext: u32,
    dotNext: u32, dotTicks: u32, tranceEnd: u32, isbExpire: u32,
    agonyNext: u32, agonyTicks: u32, doomNext: u32, siphonNext: u32,
    siphonTicks: u32, immolateNext: u32, immolateTicks: u32, isbCharges: u32,
    conflagrateCdReady: u32, shadowburnCdReady: u32, snfShadowEnd: u32, snfFireEnd: u32,
    mana: f32, boltDamage: f32, dotDamage: f32, agonyDamage: f32,
    doomDamage: f32, siphonDamage: f32, immolateDamage: f32, isbUptimeUs: f32,
    incinerateDamage: f32, conflagrateDamage: f32, shadowburnDamage: f32, petDamage: f32,
    petMeleeDamage: f32, petLopDamage: f32, petMana: f32, doomHits: u32,
    bolts: u32, dots: u32, ticks: u32, taps: u32,
    procs: u32, consumed: u32, misses: u32, crits: u32,
    agonies: u32, dooms: u32, siphons: u32, immolates: u32,
    immolateDirectHits: u32, immolateDotHits: u32,
    incinerates: u32, incinerateHits: u32, incinerateMisses: u32, incinerateCrits: u32,
    conflagrates: u32, shadowburns: u32, isbProcs: u32, isbConsumed: u32,
    loops: u32, events: u32, petNext: u32, petLopNext: u32,
    petCasts: u32, petHits: u32, petCrits: u32, petMeleeCasts: u32,
    petMeleeHits: u32, petMeleeCrits: u32, petLopCasts: u32, petLopHits: u32,
    petLopCrits: u32, corruptionCrits: u32, corruptionTicks: u32, doomCrits: u32,
    trinketEnd: u32, trinketReady: u32,
    searingDamage: f32, searingCasts: u32, searingHits: u32, searingCrits: u32,
    wrackDamage: f32, wracks: u32, wrackHits: u32, wrackCrits: u32,
    wrackNext: u32, wrackTicks: u32, wrackEnd: u32,
    demonicBrandExpire: u32, demonicBrandCharges: u32, demonicBrandDamage: f32,
    decimationExpire: u32, soulFireCdReady: u32, soulFires: u32, soulFireHits: u32, soulFireCrits: u32,
    soulFireDamage: f32,
    agonyHits: u32, agonyCrits: u32, siphonHits: u32, siphonCrits: u32,
    immolateMisses: u32, soulFireMisses: u32, searingMisses: u32, totgCdReady: u32,
    totgProcs: u32, totgDamage: f32, racialEnd: u32, racialReady: u32,
}
@group(0) @binding(0) var<storage, read> configs: array<Config>;
@group(0) @binding(1) var<uniform> params: Params;
@group(0) @binding(2) var<storage, read_write> states: array<State>;
override FIXED: bool = false;
override STEP_US: u32 = 1000u;
override CHUNK: u32 = 256u;
override GROUP_SIZE: u32 = 64u;
const NEVER: u32 = 0xffffffffu;
var<private> s: State;
var<private> c: Config;
fn micros(seconds: f32) -> u32 { return u32(round(seconds * 1000000.0)); }
fn later(seconds: f32) -> u32 {
    let delay = micros(seconds);
    if delay >= (NEVER - s.now) { return NEVER; }
    return s.now + delay;
}
fn random() -> f32 {
    s.rng = s.rng * 1664525u + 1013904223u;
    return f32(s.rng) * (1.0 / 4294967296.0);
}
fn current_shadow_power() -> f32 {
    var sp: f32 = c.shadowPower + select(0.0, c.trinketBonus, s.now < s.trinketEnd);
    if c.racialType == 1.0 && s.now < s.racialEnd {
        sp += c.racialBonus;
    }
    return sp;
}
fn current_fire_power() -> f32 {
    var sp: f32 = c.firePower + select(0.0, c.trinketBonus, s.now < s.trinketEnd);
    if c.racialType == 1.0 && s.now < s.racialEnd {
        sp += c.racialBonus;
    }
    return sp;
}

fn get_rule(idx: u32) -> u32 {
    switch idx {
        case 0u: { return u32(c.rule0); }
        case 1u: { return u32(c.rule1); }
        case 2u: { return u32(c.rule2); }
        case 3u: { return u32(c.rule3); }
        case 4u: { return u32(c.rule4); }
        case 5u: { return u32(c.rule5); }
        case 6u: { return u32(c.rule6); }
        case 7u: { return u32(c.rule7); }
        case 8u: { return u32(c.rule8); }
        case 9u: { return u32(c.rule9); }
        case 10u: { return u32(c.rule10); }
        case 11u: { return u32(c.rule11); }
        case 12u: { return u32(c.rule12); }
        case 13u: { return u32(c.rule13); }
        case 14u: { return u32(c.rule14); }
        case 15u: { return u32(c.rule15); }
        default: { return 255u; }
    }
}
fn dot_remaining(next_tick: u32, ticks: u32, interval: f32) -> f32 {
    if ticks == 0u || next_tick == NEVER { return 0.0; }
    return f32(next_tick - s.now) * 0.000001 + f32(ticks - 1u) * interval;
}
fn apply_spell_resist(rawDmg: f32) -> f32 {
    if c.partialResistEnabled == 0.0 { return rawDmg; }
    let effResist = max(0.0, c.targetResist - c.spellPen);
    let avgResist = effResist / (effResist + 400.0 + 85.0 * 60.0);
    var dmg = rawDmg * (1.0 - avgResist);
    if c.petSpellPiercing > 0.0 && effResist <= 0.0 {
        dmg *= (1.0 + c.spellPen * c.petPiercingBonus);
    }
    return dmg;
}
fn cast_dur(dur: f32) -> f32 {
    if c.racialType == 2.0 && s.now < s.racialEnd {
        return dur / (1.0 + c.racialBonus * 0.01);
    }
    return dur;
}
fn get_mana_cost(cost: f32) -> f32 {
    if c.racialType == 3.0 && s.racialEnd > 0u {
        return cost * 0.90;
    }
    return cost;
}
fn apply_racial_dmg_mods(raw_dmg: f32) -> f32 {
    var dmg = raw_dmg;
    if c.racialType == 2.0 && c.racialPad0 > 1.0 { dmg *= c.racialPad0; }
    if c.racialType == 3.0 && s.racialEnd > 0u {
        dmg *= (1.0 + c.racialBonus);
        s.racialEnd--;
    }
    return dmg;
}
fn apply_isb_to_damage(isShadow: bool) -> f32 {
    if !isShadow || s.isbExpire <= s.now { return 1.0; }
    if c.isbCharges > 0.0 {
        if s.isbCharges == 0u { return 1.0; }
        s.isbCharges--;
        s.isbConsumed++;
        if s.isbCharges == 0u { s.isbExpire = 0u; }
    } else {
        s.isbConsumed++;
    }
    return 1.0 + c.isbBonus;
}
fn apply_touch_of_the_grave() {
    if c.touchOfTheGraveDamage > 0.0 && s.now >= s.totgCdReady && random() < 0.10 {
        s.totgCdReady = later(1.0);
        s.totgProcs++;
        s.totgDamage += c.touchOfTheGraveDamage;
    }
}
fn bolt() {
    apply_touch_of_the_grave();
    s.bolts++;
    if random() >= c.hit { s.misses++; return; }
    var damage = (c.boltMin + random() * (c.boltMax - c.boltMin) + (3.0 / 3.5) * current_shadow_power()) * c.shadowBoltMultiplier;
    if s.snfShadowEnd > s.now { damage *= (1.0 + c.snfShadowBonus); }
    if s.now >= micros(c.duration * 0.65) { damage *= 1.0 + c.executeBonus; }
    damage *= apply_isb_to_damage(true);
    damage = apply_spell_resist(damage);
    if random() < c.crit {
        damage *= c.boltCrit;
        s.crits++;
        if c.isbBonus > 0.0 {
            s.isbExpire = later(12.0);
            s.isbCharges = u32(c.isbCharges);
            s.isbProcs++;
        }
    }
    damage = apply_racial_dmg_mods(damage);
    s.boltDamage += damage;
}
fn incinerate() {
    apply_touch_of_the_grave();
    s.incinerates++;
    if random() >= c.hit { s.misses++; s.incinerateMisses++; return; }
    s.incinerateHits++;
    var damage = (201.0 + random() * 32.0 + (2.5 / 3.5) * current_fire_power()) * c.conflagMultiplier;
    if s.immolateTicks > 0u { damage *= 1.25; }
    if s.snfFireEnd > s.now { damage *= (1.0 + c.snfFireBonus); }
    damage = apply_spell_resist(damage);
    if random() < c.fireCrit { damage *= c.boltCrit; s.crits++; s.incinerateCrits++; }
    damage = apply_racial_dmg_mods(damage);
    s.incinerateDamage += damage;
}
fn searing_pain() {
    apply_touch_of_the_grave();
    s.searingCasts++;
    if random() >= c.hit { s.misses++; s.searingMisses++; return; }
    s.searingHits++;
    var damage = (108.0 + random() * 19.0 + (1.5 / 3.5) * current_fire_power()) * c.searingMultiplier;
    if s.now >= micros(c.duration * 0.65) && c.executeBonus > 0.0 {
        damage *= 1.0 + c.executeBonus;
    }
    if s.snfFireEnd > s.now { damage *= (1.0 + c.snfFireBonus); }
    if random() < min(1.0, c.fireCrit + c.searingCritBonus) { damage *= c.boltCrit; s.crits++; s.searingCrits++; }
    damage = apply_spell_resist(damage);
    damage = apply_racial_dmg_mods(damage);
    s.searingDamage += damage;
    if c.demonicBrandRank > 0.0 {
        s.demonicBrandCharges = u32(c.demonicBrandRank) * 2u;
        s.demonicBrandExpire = later(10.0);
    }
}
fn process() {
    let end = micros(c.duration);
    if c.trinketEnabled > 0.0 && c.trinketCooldown > 0.0 && s.now >= s.trinketReady {
        s.trinketEnd = later(c.trinketDuration);
        s.trinketReady = later(c.trinketCooldown);
    }
    if (c.racialType == 1.0 || c.racialType == 2.0 || c.racialType == 3.0) && c.racialCooldown > 0.0 && s.now >= s.racialReady {
        let execute_phase: bool = s.now >= micros(c.duration * 0.65);
        var should_trigger: bool = false;
        let pol: u32 = u32(c.racialPolicy);
        if pol == 0u { // ON_COOLDOWN
            should_trigger = true;
        } else if pol == 1u { // EXECUTE_ONLY
            should_trigger = execute_phase;
        } else if pol == 2u { // ALIGN_DOOM
            var doom_time_left: f32 = 999.0;
            if s.doomNext != NEVER && s.doomNext > s.now {
                doom_time_left = f32(s.doomNext - s.now) * 0.000001;
            }
            let window: f32 = select(select(14.0, 10.0, c.racialType == 2.0), 6.0, c.racialType == 3.0);
            if doom_time_left <= window || execute_phase {
                should_trigger = true;
            }
        } else if pol == 3u { // ALIGN_EXECUTE
            let execute_start: f32 = c.duration * 0.65;
            if s.now < 1000000u && execute_start >= c.racialCooldown {
                should_trigger = true;
            } else {
                should_trigger = execute_phase;
            }
        }
        if should_trigger {
            s.racialEnd = select(later(c.racialDuration), u32(c.racialDuration), c.racialType == 3.0);
            s.racialReady = later(c.racialCooldown);
        }
    }


    // 1. Mana Regeneration
    if s.regenNext <= s.now {
        s.mana = min(c.maxMana, s.mana + c.mp5);
        if c.petManaManagement > 0.0 && c.petType != 0.0 {
            s.petMana = min(c.petManaMax, s.petMana + c.petMp5);
        }
        s.regenNext = later(5.0); s.events++;
    }
    // 2. Periodic DoT Ticks
    if s.dotNext <= s.now {
        var damage = (c.dotBase + current_shadow_power() * c.corruptionSpCoeff) * c.corruptionMultiplier;
        if s.snfShadowEnd > s.now { damage *= (1.0 + c.snfShadowBonus); }
        if s.wrackEnd > s.now { damage *= 1.10; }
        damage *= apply_isb_to_damage(true);
        if random() < c.crit { damage *= c.dotCrit; s.crits++; s.corruptionCrits++; }
        s.dotDamage += damage; s.ticks++; s.corruptionTicks++; s.dotTicks--; s.events++;
        if random() < c.nightfall { s.tranceEnd = later(10.0); s.procs++; }
        s.dotNext = NEVER;
        if s.dotTicks > 0u { s.dotNext = later(3.0); }
    }
    if s.agonyNext <= s.now {
        s.events++;
        let tick_idx = 12u - s.agonyTicks + 1u;
        var ramp: f32 = 1.0;
        if tick_idx <= 4u { ramp = 0.50; }
        else if tick_idx > 8u { ramp = 1.50; }
        var agonyBase = c.agonyBase;
        if c.ampCurse > 0.5 && s.agonies == 1u { agonyBase *= 1.5; }
        var damage = (agonyBase + current_shadow_power() * (1.596 / 12.0)) * ramp * c.agonyMultiplier;
        if s.snfShadowEnd > s.now { damage *= (1.0 + c.snfShadowBonus); }
        if s.wrackEnd > s.now { damage *= 1.10; }
        damage *= apply_isb_to_damage(true);
        if random() < c.crit { damage *= c.dotCrit; s.crits++; s.agonyCrits++; }
        s.agonyDamage += damage; s.ticks++; s.agonyHits++; s.agonyTicks--;
        s.agonyNext = NEVER;
        if s.agonyTicks > 0u { s.agonyNext = later(2.0); }
    }
    if s.siphonNext <= s.now {
        s.events++;
        var damage = (c.siphonBase + current_shadow_power() * 0.05) * c.siphonMultiplier;
        if s.snfShadowEnd > s.now { damage *= (1.0 + c.snfShadowBonus); }
        if s.wrackEnd > s.now { damage *= 1.10; }
        damage *= apply_isb_to_damage(true);
        if random() < c.crit { damage *= c.dotCrit; s.crits++; s.siphonCrits++; }
        s.siphonDamage += damage; s.ticks++; s.siphonHits++; s.siphonTicks--;
        s.siphonNext = NEVER;
        if s.siphonTicks > 0u { s.siphonNext = later(3.0); }
    }
    if s.immolateNext <= s.now {
        s.events++;
        var damage = (c.immolateDotBase + current_fire_power() * 0.13) * c.immolateMultiplier;
        if s.snfFireEnd > s.now { damage *= (1.0 + c.snfFireBonus); }
        if random() < c.fireCrit { damage *= c.boltCrit; s.crits++; }
        s.immolateDamage += damage; s.ticks++; s.immolateTicks--; s.immolateDotHits++;
        s.immolateNext = NEVER;
        if s.immolateTicks > 0u { s.immolateNext = later(3.0); }
    }
    if s.wrackNext <= s.now {
        s.events++;
        let aff_effects = select(0u, 1u, s.dotTicks > 0u) + select(0u, 1u, s.agonyTicks > 0u) + select(0u, 1u, s.siphonTicks > 0u);
        var damage = (c.wrackBase + current_shadow_power() * c.wrackSpCoeff) *
            c.wrackMultiplier * (1.0 + c.wrackSoulSiphonPerEffect * f32(aff_effects));
        if s.snfShadowEnd > s.now { damage *= (1.0 + c.snfShadowBonus); }
        damage *= apply_isb_to_damage(true);
        damage = apply_spell_resist(damage);
        if random() < c.crit { damage *= c.dotCrit; s.crits++; s.wrackCrits++; }
        s.wrackDamage += damage; s.wrackHits++; s.ticks++;
        if c.nightfall > 0.0 && random() < c.nightfall { s.tranceEnd = later(10.0); s.procs++; }
        s.wrackTicks--;
        s.wrackNext = NEVER;
        if s.wrackTicks > 0u { s.wrackNext = later(c.wrackTickInterval); }
    }
    if s.doomNext <= s.now {
        s.events++;
        var damage = (c.doomBase + current_shadow_power() * 4.0) * c.doomMultiplier;
        if s.snfShadowEnd > s.now { damage *= (1.0 + c.snfShadowBonus); }
        if s.wrackEnd > s.now { damage *= 1.10; }
        damage *= apply_isb_to_damage(true);
        damage = apply_spell_resist(damage);
        if random() < c.crit { damage *= c.dotCrit; s.crits++; s.doomCrits++; }
        s.doomDamage += damage; s.doomHits++; s.ticks++;
        s.doomNext = NEVER;
    }
    // 3. Pet Actions
    if s.petNext <= s.now && c.petType != 0.0 {
        s.events++;
        if c.petType == 1.0 {
            // Imp Firebolt
            if c.petManaManagement == 0.0 || s.petMana >= c.petLopCost {
                s.petCasts++;
                if c.petManaManagement > 0.0 { s.petMana -= c.petLopCost; }
                if random() >= c.hit {
                    // Miss
                } else {
                    s.petHits++;
                    var damage = (c.petBaseMin + random() * (c.petBaseMax - c.petBaseMin) + current_fire_power() * c.petSpRatio + c.petFlatSP) * c.petMultiplier;
                    if random() < c.fireCrit { damage *= 1.5; s.petCrits++; }
                    damage = apply_spell_resist(damage);
                    if c.demonicBrandRank > 0.0 && s.demonicBrandCharges > 0u && s.now < s.demonicBrandExpire {
                        s.demonicBrandCharges--;
                        var brand = (65.0 + random() * 3.0 + current_fire_power() * 0.078) * c.demonicBrandMultiplier;
                        brand = apply_spell_resist(brand);
                        s.demonicBrandDamage += brand;
                        damage += brand;
                    }
                    s.petDamage += damage;
                }
                s.petNext = later(c.petCastInterval);
            } else {
                s.petNext = later(1.0);
            }
        } else if c.petType == 2.0 {
            // Succubus Melee
            s.petCasts++;
            s.petMeleeCasts++;
            let roll = random() * 100.0;
            let miss = c.petMeleeMissPct;
            if roll < miss + c.petMeleeDodgePct {
                // Dodge / Miss
            } else {
                s.petHits++;
                s.petMeleeHits++;
                var damage = (c.petMeleeBase + c.shadowPower * c.petApRatio) * c.petMeleeMultiplier * c.petArmorMultiplier;
                let glance = roll < miss + c.petMeleeDodgePct + c.petGlancePct;
                let crit = !glance && roll < miss + c.petMeleeDodgePct + c.petGlancePct + max(0.0, c.petMeleeCritPct);
                if glance { damage *= c.petGlanceMultiplier; }
                if crit { damage *= 2.0; s.petCrits++; s.petMeleeCrits++; }
                s.petMeleeDamage += damage;
                if c.demonicBrandRank > 0.0 && s.demonicBrandCharges > 0u && s.now < s.demonicBrandExpire {
                    s.demonicBrandCharges--;
                    var brand = (65.0 + random() * 3.0 + c.shadowPower * 0.078) * c.demonicBrandMultiplier;
                    brand = apply_spell_resist(brand);
                    s.demonicBrandDamage += brand;
                    damage += brand;
                }
                s.petDamage += damage;
            }
            s.petNext = later(c.petCastInterval);
        }
    }
    if s.petLopNext <= s.now && c.petType == 2.0 {
        s.events++;
        if c.petManaManagement == 0.0 || s.petMana >= c.petLopCost {
            s.petCasts++;
            s.petLopCasts++;
            if c.petManaManagement > 0.0 { s.petMana -= c.petLopCost; }
            if random() >= c.petSpellHit {
                // Miss
            } else {
                s.petHits++;
                s.petLopHits++;
                var damage = (c.petLopBase + c.shadowPower * c.petLopSpRatio + c.petFlatSP) * c.petLopMultiplier;
                if random() < c.petSpellCrit { damage *= 1.5; s.petCrits++; s.petLopCrits++; }
                damage = apply_spell_resist(damage);
                s.petLopDamage += damage;
                if c.demonicBrandRank > 0.0 && s.demonicBrandCharges > 0u && s.now < s.demonicBrandExpire {
                    s.demonicBrandCharges--;
                    var brand = (65.0 + random() * 3.0 + c.shadowPower * 0.078) * c.demonicBrandMultiplier;
                    brand = apply_spell_resist(brand);
                    s.demonicBrandDamage += brand;
                    damage += brand;
                }
                s.petDamage += damage;
            }
            s.petLopNext = later(c.petLopCd);
        } else {
            s.petLopNext = later(1.5);
        }
    }
    // 4. Player Cast Completion
    if s.castEnd <= s.now {
        s.events++;
        if (s.castSpellId == 1u || s.castSpellId == 5u) && c.executeBonus > 0.0 && s.now >= micros(c.duration * 0.65) {
            s.decimationExpire = later(10.0);
        }
        if s.castSpellId == 2u {
            apply_touch_of_the_grave();
            s.immolates++;
            if random() >= c.hit { s.misses++; s.immolateMisses++; }
            else {
                var damage = (c.immolateMin + random() * (c.immolateMax - c.immolateMin) + current_fire_power() * 0.20) * c.immolateDirectMultiplier;
                if s.snfFireEnd > s.now { damage *= (1.0 + c.snfFireBonus); }
                damage = apply_spell_resist(damage);
                if random() < c.fireCrit { damage *= c.boltCrit; s.crits++; }
                s.immolateDamage += damage;
                s.immolateDirectHits++;
                s.immolateTicks = 5u;
                s.immolateNext = later(3.0);
            }
        } else if s.castSpellId == 3u {
            apply_touch_of_the_grave();
            s.mana = max(0.0, s.mana - c.corruptionCost);
            if random() >= c.hit { s.misses++; }
            else { s.dotTicks = 6u; s.dotNext = later(3.0); }
        } else if s.castSpellId == 4u {
            incinerate();
        } else if s.castSpellId == 5u {
            searing_pain();
        } else if s.castSpellId == 6u {
            apply_touch_of_the_grave();
            s.soulFires++;
            s.soulFireCdReady = later(c.soulFireCooldown);
            if random() >= c.hit { s.misses++; s.soulFireMisses++; }
            else {
                s.soulFireHits++;
                var damage = (383.0 + random() * 96.0 + current_fire_power()) * c.soulFireMultiplier;
                if s.snfFireEnd > s.now { damage *= (1.0 + c.snfFireBonus); }
                damage = apply_spell_resist(damage);
                if random() < c.fireCrit { damage *= c.boltCrit; s.crits++; s.soulFireCrits++; }
                s.soulFireDamage += damage;
            }
        } else {
            bolt();
        }
        s.castEnd = NEVER; s.castSpellId = 0u;
    }
    // 5. Player Decision via Action Priority List (APL)
    if s.ready <= s.now && s.castEnd == NEVER {
        s.events++;
        var acted: bool = false;
        let num_rules: u32 = u32(c.ruleCount);
        for (var r_idx: u32 = 0u; r_idx < num_rules; r_idx++) {
            let act: u32 = get_rule(r_idx);
            if act == 0u { // LIFE_TAP
                if s.mana <= c.maxMana * c.tapThreshold {
                    s.mana = min(c.maxMana, s.mana + c.tapGain);
                    if c.petTapGain > 0.0 && c.petType != 0.0 { s.petMana = min(c.petManaMax, s.petMana + c.petTapGain); }
                    s.taps++; s.ready = later(c.gcd); acted = true; break;
                }
            } else if act == 8u { // NIGHTFALL_SHADOW_BOLT
                if s.tranceEnd > s.now && s.mana >= get_mana_cost(c.boltCost) {
                    s.tranceEnd = 0u; s.consumed++; s.mana -= get_mana_cost(c.boltCost); bolt(); s.ready = later(c.gcd); acted = true; break;
                }
            } else if act == 9u { // DECIMATION_SEARING_PAIN
                if c.executeBonus > 0.0 && s.now >= micros(c.duration * 0.65) && s.decimationExpire <= s.now && s.mana >= get_mana_cost(c.searingCost) {
                    s.mana -= get_mana_cost(c.searingCost);
                    s.castEnd = later(cast_dur(c.searingCastTime)); s.castSpellId = 5u;
                    s.ready = max(later(c.gcd), s.castEnd); acted = true; break;
                }
            } else if act == 11u { // DEMONIC_BRAND_SEARING_PAIN
                if c.demonicBrandRank > 0.0 && (s.demonicBrandCharges == 0u || s.now >= s.demonicBrandExpire) && s.mana >= get_mana_cost(c.searingCost) {
                    s.mana -= get_mana_cost(c.searingCost);
                    s.castEnd = later(cast_dur(c.searingCastTime)); s.castSpellId = 5u;
                    s.ready = max(later(c.gcd), s.castEnd); acted = true; break;
                }
            } else if act == 10u { // DECIMATION_SOUL_FIRE
                if c.executeBonus > 0.0 && s.now >= micros(c.duration * 0.65) && s.decimationExpire > s.now &&
                    s.now >= s.soulFireCdReady && s.mana >= get_mana_cost(c.soulFireCost) {
                    s.mana -= get_mana_cost(c.soulFireCost);
                    s.castEnd = later(cast_dur(c.soulFireCastTime)); s.castSpellId = 6u;
                    s.ready = max(later(c.gcd), s.castEnd); acted = true; break;
                }
            } else if act == 12u { // CORRUPTION
                if c.corruptionCost > 0.0 && (s.dotTicks == 0u || (c.corruptionRefreshSec > 0.0 && dot_remaining(s.dotNext, s.dotTicks, 3.0) <= c.corruptionRefreshSec)) && s.mana >= get_mana_cost(c.corruptionCost) {
                    s.dots++;
                    if c.corruptionCastTime == 0.0 {
                        apply_touch_of_the_grave();
                        s.mana -= get_mana_cost(c.corruptionCost);
                        if random() < c.hit { s.dotTicks = 6u; s.dotNext = later(3.0); } else { s.misses++; }
                        s.ready = later(c.gcd);
                    } else {
                        s.castEnd = later(cast_dur(c.corruptionCastTime)); s.castSpellId = 3u;
                        s.ready = max(later(c.gcd), s.castEnd);
                    }
                    acted = true; break;
                }
            } else if act == 5u { // CURSE_OF_AGONY
                if c.agonyCost > 0.0 && (s.agonyTicks == 0u || (c.agonyRefreshSec > 0.0 && dot_remaining(s.agonyNext, s.agonyTicks, 2.0) <= c.agonyRefreshSec)) && s.doomNext == NEVER && s.mana >= get_mana_cost(c.agonyCost) {
                    apply_touch_of_the_grave();
                    s.mana -= get_mana_cost(c.agonyCost); s.agonies++;
                    if random() < c.hit { s.agonyTicks = 12u; s.agonyNext = later(2.0); } else { s.misses++; }
                    s.ready = later(c.gcd); acted = true; break;
                }
            } else if act == 6u { // CURSE_OF_DOOM
                let doom_min_remaining_us = u32(c.doomMinTimeRemaining * 1000000.0);
                if c.doomCost > 0.0 && s.doomNext == NEVER && s.agonyTicks == 0u && (c.agonyCost == 0.0 || s.now + doom_min_remaining_us <= end) && s.mana >= get_mana_cost(c.doomCost) {
                    apply_touch_of_the_grave();
                    s.mana -= get_mana_cost(c.doomCost); s.dooms++;
                    if random() < c.hit { s.doomNext = later(60.0); } else { s.misses++; }
                    s.ready = later(c.gcd); acted = true; break;
                }
            } else if act == 13u { // SIPHON_LIFE
                if c.siphonCost > 0.0 && (s.siphonTicks == 0u || (c.siphonRefreshSec > 0.0 && dot_remaining(s.siphonNext, s.siphonTicks, 3.0) <= c.siphonRefreshSec)) && s.mana >= get_mana_cost(c.siphonCost) {
                    apply_touch_of_the_grave();
                    s.mana -= get_mana_cost(c.siphonCost); s.siphons++;
                    if random() < c.hit { s.siphonTicks = 10u; s.siphonNext = later(3.0); } else { s.misses++; }
                    s.ready = later(c.gcd); acted = true; break;
                }
            } else if act == 14u { // WRACK (legacy enum name: DRAIN_HOPE)
                if c.wrackCost > 0.0 && s.wrackEnd <= s.now && s.mana >= get_mana_cost(c.wrackCost) {
                    apply_touch_of_the_grave();
                    s.mana -= get_mana_cost(c.wrackCost); s.wracks++;
                    s.wrackTicks = 6u; s.wrackNext = later(c.wrackTickInterval);
                    s.wrackEnd = later(cast_dur(c.wrackChannelTime));
                    s.ready = max(later(c.gcd), s.wrackEnd);
                    acted = true; break;
                }
            } else if act == 15u { // IMMOLATE
                if c.immolateCost > 0.0 && (s.immolateTicks == 0u || (c.immolateRefreshSec > 0.0 && dot_remaining(s.immolateNext, s.immolateTicks, 3.0) <= c.immolateRefreshSec)) && s.mana >= get_mana_cost(c.immolateCost) {
                    s.mana -= get_mana_cost(c.immolateCost);
                    s.castEnd = later(cast_dur(c.immolateCastTime)); s.castSpellId = 2u;
                    s.ready = max(later(c.gcd), s.castEnd); acted = true; break;
                }
            } else if act == 16u { // CONFLAGRATE
                if c.conflagCost > 0.0 && s.immolateTicks > 0u && s.now >= s.conflagrateCdReady && s.mana >= get_mana_cost(c.conflagCost) {
                    apply_touch_of_the_grave();
                    s.mana -= get_mana_cost(c.conflagCost); s.conflagrates++; s.conflagrateCdReady = later(10.0);
                    if random() >= c.hit { s.misses++; }
                    else {
                        var dmg = (c.conflagMin + random() * (c.conflagMax - c.conflagMin) + (1.5 / 3.5) * current_fire_power()) * c.conflagMultiplier;
                        if s.snfFireEnd > s.now { dmg *= (1.0 + c.snfFireBonus); }
                        if random() < c.fireCrit + c.fnbCritBonus { dmg *= c.boltCrit; s.crits++; }
                        dmg = apply_spell_resist(dmg);
                        dmg = apply_racial_dmg_mods(dmg);
                        s.conflagrateDamage += dmg;
                        if c.snfShadowBonus > 0.0 { s.snfShadowEnd = later(20.0); }
                    }
                    if c.snfConflagSaveChance == 0.0 || random() >= c.snfConflagSaveChance { s.immolateTicks = 0u; s.immolateNext = NEVER; }
                    s.ready = later(c.gcd); acted = true; break;
                }
            } else if act == 17u || act == 18u { // SHADOWBURN / SHADOWBURN_ISB
                if c.shadowburnCost > 0.0 && s.now >= s.shadowburnCdReady && s.mana >= get_mana_cost(c.shadowburnCost) {
                    apply_touch_of_the_grave();
                    s.mana -= get_mana_cost(c.shadowburnCost); s.shadowburns++; s.shadowburnCdReady = later(15.0);
                    if random() >= c.hit { s.misses++; }
                    else {
                        var dmg = (c.sburnMin + random() * (c.sburnMax - c.sburnMin) + (1.5 / 3.5) * current_shadow_power()) * c.shadowburnMultiplier;
                        if s.snfShadowEnd > s.now { dmg *= (1.0 + c.snfShadowBonus); }
                        dmg *= apply_isb_to_damage(true);
                        if random() < c.crit { dmg *= c.boltCrit; s.crits++; }
                        dmg = apply_spell_resist(dmg);
                        dmg = apply_racial_dmg_mods(dmg);
                        s.shadowburnDamage += dmg;
                        if c.snfFireBonus > 0.0 { s.snfFireEnd = later(20.0); }
                    }
                    s.ready = later(c.gcd); acted = true; break;
                }
            } else if act == 23u || act == 24u { // SHADOW_BOLT_FILLER / RANK2
                if s.mana >= get_mana_cost(c.boltCost) {
                    s.mana -= get_mana_cost(c.boltCost);
                    s.castEnd = later(cast_dur(c.castTime)); s.castSpellId = 1u;
                    s.ready = max(later(c.gcd), s.castEnd); acted = true; break;
                }
            } else if act == 19u { // INCINERATE_FILLER
                if s.mana >= get_mana_cost(c.boltCost) {
                    s.mana -= get_mana_cost(c.boltCost);
                    s.castEnd = later(cast_dur(c.castTime)); s.castSpellId = 4u;
                    s.ready = max(later(c.gcd), s.castEnd); acted = true; break;
                }
            } else if act == 20u { // SEARING_PAIN_FILLER
                if c.searingCost > 0.0 && s.mana >= get_mana_cost(c.searingCost) {
                    s.mana -= get_mana_cost(c.searingCost);
                    s.castEnd = later(cast_dur(c.searingCastTime)); s.castSpellId = 5u;
                    s.ready = max(later(c.gcd), s.castEnd); acted = true; break;
                }
            }
        }
            if !acted {
                let filler_cost = get_mana_cost(select(c.boltCost, c.searingCost, c.fillerType == 2.0));
                if s.mana >= filler_cost {
                s.mana -= filler_cost;
                s.castEnd = later(cast_dur(c.castTime));
                s.castSpellId = select(select(1u, 4u, c.fillerType == 1.0), 5u, c.fillerType == 2.0);
                s.ready = max(later(c.gcd), s.castEnd);
            } else {
                s.mana = min(c.maxMana, s.mana + c.tapGain);
                if c.petTapGain > 0.0 && c.petType != 0.0 {
                    s.petMana = min(c.petManaMax, s.petMana + c.petTapGain);
                }
                s.taps++;
                s.ready = later(c.gcd);
            }
        }

    }
}
@compute @workgroup_size(GROUP_SIZE)
fn simulate(@builtin(global_invocation_id) id: vec3<u32>) {
    if id.x >= params.count { return; }
    s = states[id.x];
    if s.done != 0u { return; }
    c = configs[id.x / params.perCandidate];
    if s.initialized == 0u {
        s.initialized = 1u;
        s.rng = (params.seed ^ (id.x * 1664525u + 1013904223u)) + 1u;
        s.now = 0u; s.ready = 0u; s.castEnd = NEVER; s.castSpellId = 0u;
        s.regenNext = micros(5.0); s.dotNext = NEVER; s.agonyNext = NEVER;
        s.doomNext = NEVER; s.siphonNext = NEVER; s.immolateNext = NEVER; s.wrackNext = NEVER;
        s.conflagrateCdReady = 0u; s.shadowburnCdReady = 0u;
        s.snfShadowEnd = 0u; s.snfFireEnd = 0u;
        s.mana = c.maxMana;
        s.petMana = c.petManaMax;
        if c.petType != 0.0 { s.petNext = 0u; } else { s.petNext = NEVER; }
        if c.petType == 2.0 { s.petLopNext = 0u; } else { s.petLopNext = NEVER; }
    }
    let end = micros(c.duration);
    for (var i = 0u; i < CHUNK && s.now < end; i++) {
        process();
        s.loops++;
        let next_dot = min(s.dotNext, min(s.agonyNext, min(s.doomNext, min(s.siphonNext, min(s.immolateNext, s.wrackNext)))));
        let next_pet = min(s.petNext, s.petLopNext);
        var next_now: u32;
        if FIXED { next_now = s.now + STEP_US; }
        else {
            let next_trinket = select(NEVER, s.trinketReady, c.trinketEnabled > 0.0);
            next_now = min(s.ready, min(s.castEnd, min(next_dot, min(next_pet, min(s.regenNext, next_trinket)))));
        }
        if s.isbExpire > s.now && (c.isbCharges == 0.0 || s.isbCharges > 0u) {
            let active_end = min(next_now, s.isbExpire);
            if active_end > s.now { s.isbUptimeUs += f32(active_end - s.now); }
        }
        s.now = next_now;
    }
    if s.now >= end { s.done = 1u; }
    states[id.x] = s;
}
