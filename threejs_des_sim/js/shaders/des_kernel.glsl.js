/**
 * GLSL Discrete Event Simulation (DES) Shader for Three.js GPGPU
 * 
 * Features:
 * - True Variable-Step Discrete Event Scheduler on GPU (no fixed-interval polling)
 * - Exact Pixel Coordinate Mapping: Row Y = Config Index, Column X = Iteration Index
 * - Full combat model: Spell Casts, Projectile Impacts, DoT Ticks, Buffs (ISB, Spells'n'Flames),
 *   Mana Regeneration (MP5 / Life Tap), Cooldowns (Conflagrate, Shadowburn, Trinkets),
 *   Dynamic APL rule-based priority engine.
 * - LCG high-performance PRNG per simulation thread.
 * - Batched configuration lookup from a dynamic 2D DataTexture.
 */

export const DES_PASS_NAMES = [
    'state0', // (now, mana, rng, done)
    'state1', // (ready, castEnd, castSpellId, regenNext)
    'state2', // (dotNext, dotTicks, agonyNext, agonyTicks)
    'state3', // (doomNext, siphonNext, siphonTicks, immolateNext)
    'state4', // (immolateTicks, isbCharges, isbExpire, wrackNext)
    'state5', // (wrackTicks, wrackEnd, conflagCd, sburnCd)
    'state6', // (snfShadowEnd, snfFireEnd, trinketEnd, trinketReady)
    'dmg0',   // (totalDmg, boltDmg, dotDmg, agonyDmg)
    'dmg1',   // (doomDmg, siphonDmg, immolateDmg, incinerateDmg)
    'dmg2',   // (conflagDmg, sburnDmg, searingDmg, wrackDmg)
    'counts0',// (casts, hits, crits, misses)
    'counts1',// (taps, isbProcs, isbConsumed, loops)
    'counts2' // (events, dps, activeTime, isbUptime)
];

export const DES_KERNEL_COMMON_GLSL = `
#define NEVER 999999.0
#define PI 3.141592653589793

// Read configuration parameters from configs texture
// 12 vec4s (48 floats) per configuration index
vec4 fetchConfigVec4(sampler2D configTex, float configIdx, float vec4Idx, float totalConfigs) {
    float x = (vec4Idx + 0.5) / 12.0;
    float y = (configIdx + 0.5) / totalConfigs;
    return texture2D(configTex, vec2(x, y));
}

// Fast LCG random number generator
float nextRandom(inout float rngState) {
    rngState = mod(rngState * 1664525.0 + 1013904223.0, 4294967296.0);
    return rngState / 4294967296.0;
}
`;

export const DES_FRAGMENT_SHADER = `
precision highp float;
precision highp int;

varying vec2 vUv;

// Textures for all state channels
uniform sampler2D tState0;
uniform sampler2D tState1;
uniform sampler2D tState2;
uniform sampler2D tState3;
uniform sampler2D tState4;
uniform sampler2D tState5;
uniform sampler2D tState6;
uniform sampler2D tDmg0;
uniform sampler2D tDmg1;
uniform sampler2D tDmg2;
uniform sampler2D tCounts0;
uniform sampler2D tCounts1;
uniform sampler2D tCounts2;

uniform sampler2D tConfigs;
uniform float uTotalConfigs;
uniform float uIterationsPerConfig;
uniform vec2 uResolution; // (width = iterations, height = configs)
uniform float uChunkSize;
uniform float uSeed;
uniform int uOutputTarget; // 0..12 to select which buffer to write in current pass

${DES_KERNEL_COMMON_GLSL}

void main() {
    vec2 uv = vUv;
    
    // Read current state
    vec4 s0 = texture2D(tState0, uv);
    vec4 s1 = texture2D(tState1, uv);
    vec4 s2 = texture2D(tState2, uv);
    vec4 s3 = texture2D(tState3, uv);
    vec4 s4 = texture2D(tState4, uv);
    vec4 s5 = texture2D(tState5, uv);
    vec4 s6 = texture2D(tState6, uv);
    vec4 d0 = texture2D(tDmg0, uv);
    vec4 d1 = texture2D(tDmg1, uv);
    vec4 d2 = texture2D(tDmg2, uv);
    vec4 c0 = texture2D(tCounts0, uv);
    vec4 c1 = texture2D(tCounts1, uv);
    vec4 c2 = texture2D(tCounts2, uv);

    // If simulation is already complete, preserve state
    if (s0.w > 0.5) {
        if (uOutputTarget == 0) gl_FragColor = s0;
        else if (uOutputTarget == 1) gl_FragColor = s1;
        else if (uOutputTarget == 2) gl_FragColor = s2;
        else if (uOutputTarget == 3) gl_FragColor = s3;
        else if (uOutputTarget == 4) gl_FragColor = s4;
        else if (uOutputTarget == 5) gl_FragColor = s5;
        else if (uOutputTarget == 6) gl_FragColor = s6;
        else if (uOutputTarget == 7) gl_FragColor = d0;
        else if (uOutputTarget == 8) gl_FragColor = d1;
        else if (uOutputTarget == 9) gl_FragColor = d2;
        else if (uOutputTarget == 10) gl_FragColor = c0;
        else if (uOutputTarget == 11) gl_FragColor = c1;
        else gl_FragColor = c2;
        return;
    }

    // Exact pixel coordinate mapping:
    // Y pixel = config index (0 .. totalConfigs - 1)
    // X pixel = iteration index (0 .. iterationsPerConfig - 1)
    float pixelX = floor(gl_FragCoord.x);
    float pixelY = floor(gl_FragCoord.y);
    float configIdx = clamp(pixelY, 0.0, uTotalConfigs - 1.0);
    float iterIdx = pixelX;
    float globalThreadId = pixelY * uResolution.x + pixelX;

    // Unpack Config from Texture
    vec4 cfg0 = fetchConfigVec4(tConfigs, configIdx, 0.0, uTotalConfigs);
    float duration = cfg0.x;
    float baseCastTime = cfg0.y;
    float gcd = cfg0.z;
    float maxMana = cfg0.w;

    vec4 cfg1 = fetchConfigVec4(tConfigs, configIdx, 1.0, uTotalConfigs);
    float spellPower = cfg1.x;
    float hit = cfg1.y;
    float crit = cfg1.z;
    float boltCritMult = cfg1.w;

    vec4 cfg2 = fetchConfigVec4(tConfigs, configIdx, 2.0, uTotalConfigs);
    float dotCritMult = cfg2.x;
    float tapGain = cfg2.y;
    float mp5 = cfg2.z;
    float tapThreshold = cfg2.w;

    vec4 cfg3 = fetchConfigVec4(tConfigs, configIdx, 3.0, uTotalConfigs);
    float boltCost = cfg3.x;
    float corruptionCost = cfg3.y;
    float agonyCost = cfg3.z;
    float doomCost = cfg3.w;

    vec4 cfg4 = fetchConfigVec4(tConfigs, configIdx, 4.0, uTotalConfigs);
    float siphonCost = cfg4.x;
    float immolateCost = cfg4.y;
    float conflagCost = cfg4.z;
    float sburnCost = cfg4.w;

    vec4 cfg5 = fetchConfigVec4(tConfigs, configIdx, 5.0, uTotalConfigs);
    float boltMin = cfg5.x;
    float boltMax = cfg5.y;
    float shadowPower = cfg5.z;
    float shadowBoltMultiplier = cfg5.w;

    vec4 cfg6 = fetchConfigVec4(tConfigs, configIdx, 6.0, uTotalConfigs);
    float isbBonus = cfg6.x;
    float isbMaxCharges = cfg6.y;
    float executeBonus = cfg6.z;
    float corruptionCastTime = cfg6.w;

    vec4 cfg7 = fetchConfigVec4(tConfigs, configIdx, 7.0, uTotalConfigs);
    float firePower = cfg7.x;
    float fireCrit = cfg7.y;
    float conflagMult = cfg7.z;
    float immolateMult = cfg7.w;

    vec4 cfg8 = fetchConfigVec4(tConfigs, configIdx, 8.0, uTotalConfigs);
    float immolateCastTime = cfg8.x;
    float immolateDotBase = cfg8.y;
    float conflagMin = cfg8.z;
    float conflagMax = cfg8.w;

    vec4 cfg9 = fetchConfigVec4(tConfigs, configIdx, 9.0, uTotalConfigs);
    float trinketBonus = cfg9.x;
    float trinketDuration = cfg9.y;
    float trinketCooldown = cfg9.z;
    float trinketEnabled = cfg9.w;

    vec4 cfg10 = fetchConfigVec4(tConfigs, configIdx, 10.0, uTotalConfigs);
    vec4 cfg11 = fetchConfigVec4(tConfigs, configIdx, 11.0, uTotalConfigs);
    float fillerType = cfg11.w;

    // Unpack mutable states
    float now = s0.x;
    float mana = s0.y;
    float rng = s0.z;
    float done = s0.w;

    float ready = s1.x;
    float castEnd = s1.y;
    float castSpellId = s1.z;
    float regenNext = s1.w;

    float dotNext = s2.x;
    float dotTicks = s2.y;
    float agonyNext = s2.z;
    float agonyTicks = s2.w;

    float doomNext = s3.x;
    float siphonNext = s3.y;
    float siphonTicks = s3.w;
    float immolateNext = s3.z;

    float immolateTicks = s4.x;
    float isbCharges = s4.y;
    float isbExpire = s4.z;
    float wrackNext = s4.w;

    float wrackTicks = s5.x;
    float wrackEnd = s5.y;
    float conflagCdReady = s5.z;
    float shadowburnCdReady = s5.w;

    float snfShadowEnd = s6.x;
    float snfFireEnd = s6.y;
    float trinketEnd = s6.z;
    float trinketReady = s6.w;

    float totalDamage = d0.x;
    float boltDamage = d0.y;
    float dotDamage = d0.z;
    float agonyDamage = d0.w;

    float doomDamage = d1.x;
    float siphonDamage = d1.y;
    float immolateDamage = d1.z;
    float incinerateDamage = d1.w;

    float conflagDamage = d2.x;
    float shadowburnDamage = d2.y;
    float searingDamage = d2.z;
    float wrackDamage = d2.w;

    float casts = c0.x;
    float hits = c0.y;
    float crits = c0.z;
    float misses = c0.w;

    float taps = c1.x;
    float isbProcs = c1.y;
    float isbConsumed = c1.z;
    float loops = c1.w;

    float eventCount = c2.x;
    float isbUptime = c2.w;

    // Thread State Initializer
    if (rng == 0.0) {
        rng = mod(uSeed + globalThreadId * 1664525.0 + 1013904223.0, 4294967296.0);
        now = 0.0;
        mana = maxMana;
        ready = 0.0;
        castEnd = NEVER;
        castSpellId = 0.0;
        regenNext = 5.0;
        dotNext = NEVER;
        agonyNext = NEVER;
        doomNext = NEVER;
        siphonNext = NEVER;
        immolateNext = NEVER;
        wrackNext = NEVER;
        wrackEnd = 0.0;
        conflagCdReady = 0.0;
        shadowburnCdReady = 0.0;
        snfShadowEnd = 0.0;
        snfFireEnd = 0.0;
        trinketEnd = 0.0;
        trinketReady = 0.0;
        totalDamage = 0.0;
        boltDamage = 0.0;
        dotDamage = 0.0;
        agonyDamage = 0.0;
        doomDamage = 0.0;
        siphonDamage = 0.0;
        immolateDamage = 0.0;
        incinerateDamage = 0.0;
        conflagDamage = 0.0;
        shadowburnDamage = 0.0;
        searingDamage = 0.0;
        wrackDamage = 0.0;
        casts = 0.0;
        hits = 0.0;
        crits = 0.0;
        misses = 0.0;
        taps = 0.0;
        isbProcs = 0.0;
        isbConsumed = 0.0;
        loops = 0.0;
        eventCount = 0.0;
        isbUptime = 0.0;
        done = 0.0;
    }

    // RUN DES DISCRETE EVENT CHUNK LOOP
    int chunk = int(uChunkSize);
    for (int step = 0; step < 128; step++) {
        if (step >= chunk || done > 0.5) break;

        float curShadowPower = shadowPower + (now < trinketEnd ? trinketBonus : 0.0);
        float curFirePower = firePower + (now < trinketEnd ? trinketBonus : 0.0);

        // 1. Trinket Off-Cooldown
        if (trinketEnabled > 0.5 && now >= trinketReady && trinketCooldown > 0.0) {
            trinketEnd = now + trinketDuration;
            trinketReady = now + trinketCooldown;
            eventCount += 1.0;
        }

        // 2. Periodic MP5 Mana Regen
        if (now >= regenNext) {
            mana = min(maxMana, mana + mp5);
            regenNext = now + 5.0;
            eventCount += 1.0;
        }

        // 3. Periodic DoT Ticks
        if (dotTicks > 0.5 && now >= dotNext) {
            dotTicks -= 1.0;
            dotNext = (dotTicks > 0.5) ? now + 3.0 : NEVER;
            float tickDmg = (137.0 + (curShadowPower * 0.1666)) * (now < snfShadowEnd ? 1.15 : 1.0);
            if (isbExpire > now) {
                tickDmg *= (1.0 + isbBonus);
                if (isbCharges > 0.5) {
                    isbCharges -= 1.0;
                    isbConsumed += 1.0;
                    if (isbCharges <= 0.0) isbExpire = 0.0;
                }
            }
            dotDamage += tickDmg;
            totalDamage += tickDmg;
            eventCount += 1.0;
        }

        if (agonyTicks > 0.5 && now >= agonyNext) {
            agonyTicks -= 1.0;
            agonyNext = (agonyTicks > 0.5) ? now + 2.0 : NEVER;
            float agonyDmg = 113.0 + (curShadowPower * 0.0833);
            if (isbExpire > now) agonyDmg *= (1.0 + isbBonus);
            agonyDamage += agonyDmg;
            totalDamage += agonyDmg;
            eventCount += 1.0;
        }

        if (now >= doomNext && doomNext < NEVER) {
            doomNext = NEVER;
            float dDmg = 3200.0 + curShadowPower * 2.0;
            if (isbExpire > now) dDmg *= (1.0 + isbBonus);
            if (nextRandom(rng) < crit) {
                dDmg *= boltCritMult;
                crits += 1.0;
            }
            doomDamage += dDmg;
            totalDamage += dDmg;
            eventCount += 1.0;
        }

        if (siphonTicks > 0.5 && now >= siphonNext) {
            siphonTicks -= 1.0;
            siphonNext = (siphonTicks > 0.5) ? now + 3.0 : NEVER;
            float sDmg = 63.0 + (curShadowPower * 0.10);
            if (isbExpire > now) sDmg *= (1.0 + isbBonus);
            siphonDamage += sDmg;
            totalDamage += sDmg;
            eventCount += 1.0;
        }

        if (immolateTicks > 0.5 && now >= immolateNext) {
            immolateTicks -= 1.0;
            immolateNext = (immolateTicks > 0.5) ? now + 3.0 : NEVER;
            float iDmg = (immolateDotBase + (curFirePower * 0.13)) * immolateMult;
            if (now < snfFireEnd) iDmg *= 1.15;
            immolateDamage += iDmg;
            totalDamage += iDmg;
            eventCount += 1.0;
        }

        if (wrackTicks > 0.5 && now >= wrackNext) {
            wrackTicks -= 1.0;
            wrackNext = (wrackTicks > 0.5) ? now + 1.0 : NEVER;
            float wDmg = 110.0 + curShadowPower * 0.20;
            if (isbExpire > now) wDmg *= (1.0 + isbBonus);
            wrackDamage += wDmg;
            totalDamage += wDmg;
            eventCount += 1.0;
        }

        // 4. Spell Cast Finish & Projectile Impact
        if (castSpellId > 0.5 && now >= castEnd) {
            float spell = castSpellId;
            castSpellId = 0.0;
            castEnd = NEVER;
            eventCount += 1.0;

            if (spell == 1.0) { // Shadow Bolt
                casts += 1.0;
                if (nextRandom(rng) < hit) {
                    hits += 1.0;
                    float dmg = (boltMin + nextRandom(rng) * (boltMax - boltMin) + (3.0 / 3.5) * curShadowPower) * shadowBoltMultiplier;
                    if (now < snfShadowEnd) dmg *= 1.15;
                    if (now >= duration * 0.65) dmg *= (1.0 + executeBonus);
                    if (isbExpire > now) {
                        dmg *= (1.0 + isbBonus);
                        if (isbCharges > 0.5) {
                            isbCharges -= 1.0;
                            isbConsumed += 1.0;
                            if (isbCharges <= 0.0) isbExpire = 0.0;
                        }
                    }
                    if (nextRandom(rng) < crit) {
                        dmg *= boltCritMult;
                        crits += 1.0;
                        if (isbBonus > 0.0) {
                            isbExpire = now + 12.0;
                            isbCharges = isbMaxCharges;
                            isbProcs += 1.0;
                        }
                    }
                    boltDamage += dmg;
                    totalDamage += dmg;
                } else {
                    misses += 1.0;
                }
            } else if (spell == 2.0) { // Immolate
                casts += 1.0;
                if (nextRandom(rng) < hit) {
                    hits += 1.0;
                    float dmg = (279.0 + nextRandom(rng) * 70.0 + (1.5 / 3.5) * curFirePower) * immolateMult;
                    if (now < snfFireEnd) dmg *= 1.15;
                    if (nextRandom(rng) < fireCrit) {
                        dmg *= boltCritMult;
                        crits += 1.0;
                    }
                    immolateDamage += dmg;
                    totalDamage += dmg;
                    immolateTicks = 5.0;
                    immolateNext = now + 3.0;
                } else {
                    misses += 1.0;
                }
            } else if (spell == 4.0) { // Incinerate
                casts += 1.0;
                if (nextRandom(rng) < hit) {
                    hits += 1.0;
                    float dmg = (201.0 + nextRandom(rng) * 32.0 + (2.5 / 3.5) * curFirePower) * conflagMult;
                    if (immolateTicks > 0.5) dmg *= 1.25;
                    if (now < snfFireEnd) dmg *= 1.15;
                    if (nextRandom(rng) < fireCrit) {
                        dmg *= boltCritMult;
                        crits += 1.0;
                    }
                    incinerateDamage += dmg;
                    totalDamage += dmg;
                } else {
                    misses += 1.0;
                }
            } else if (spell == 5.0) { // Searing Pain
                casts += 1.0;
                if (nextRandom(rng) < hit) {
                    hits += 1.0;
                    float dmg = (108.0 + nextRandom(rng) * 19.0 + (1.5 / 3.5) * curFirePower);
                    if (now >= duration * 0.65) dmg *= (1.0 + executeBonus);
                    if (now < snfFireEnd) dmg *= 1.15;
                    if (nextRandom(rng) < (fireCrit + 0.10)) {
                        dmg *= boltCritMult;
                        crits += 1.0;
                    }
                    searingDamage += dmg;
                    totalDamage += dmg;
                } else {
                    misses += 1.0;
                }
            }
        }

        // 5. Action Priority List (APL) Engine
        if (now >= ready && castSpellId == 0.0) {
            bool acted = false;

            for (int r = 0; r < 8; r++) {
                if (acted) break;
                float rule = (r < 4) ? 
                    ((r == 0) ? cfg10.x : (r == 1) ? cfg10.y : (r == 2) ? cfg10.z : cfg10.w) :
                    ((r == 4) ? cfg11.x : (r == 5) ? cfg11.y : (r == 6) ? cfg11.z : 255.0);

                if (rule == 0.0) { // LIFE_TAP
                    if (mana <= maxMana * tapThreshold) {
                        mana = min(maxMana, mana + tapGain);
                        taps += 1.0;
                        ready = now + gcd;
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 6.0) { // CURSE_OF_DOOM
                    if (doomCost > 0.0 && doomNext >= NEVER && agonyTicks == 0.0 && (now + 60.0 <= duration) && mana >= doomCost) {
                        mana -= doomCost;
                        if (nextRandom(rng) < hit) {
                            doomNext = now + 60.0;
                        } else {
                            misses += 1.0;
                        }
                        ready = now + gcd;
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 5.0) { // CURSE_OF_AGONY
                    if (agonyCost > 0.0 && agonyTicks == 0.0 && doomNext >= NEVER && mana >= agonyCost) {
                        mana -= agonyCost;
                        if (nextRandom(rng) < hit) {
                            agonyTicks = 12.0;
                            agonyNext = now + 2.0;
                        } else {
                            misses += 1.0;
                        }
                        ready = now + gcd;
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 12.0) { // CORRUPTION
                    if (corruptionCost > 0.0 && dotTicks == 0.0 && mana >= corruptionCost) {
                        mana -= corruptionCost;
                        if (corruptionCastTime <= 0.0) {
                            if (nextRandom(rng) < hit) {
                                dotTicks = 6.0;
                                dotNext = now + 3.0;
                            } else {
                                misses += 1.0;
                            }
                            ready = now + gcd;
                        } else {
                            castEnd = now + corruptionCastTime;
                            castSpellId = 3.0;
                            ready = max(now + gcd, castEnd);
                        }
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 13.0) { // SIPHON_LIFE
                    if (siphonCost > 0.0 && siphonTicks == 0.0 && mana >= siphonCost) {
                        mana -= siphonCost;
                        if (nextRandom(rng) < hit) {
                            siphonTicks = 10.0;
                            siphonNext = now + 3.0;
                        } else {
                            misses += 1.0;
                        }
                        ready = now + gcd;
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 15.0) { // IMMOLATE
                    if (immolateCost > 0.0 && immolateTicks == 0.0 && mana >= immolateCost) {
                        mana -= immolateCost;
                        castEnd = now + immolateCastTime;
                        castSpellId = 2.0;
                        ready = max(now + gcd, castEnd);
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 16.0) { // CONFLAGRATE
                    if (conflagCost > 0.0 && immolateTicks > 0.0 && now >= conflagCdReady && mana >= conflagCost) {
                        mana -= conflagCost;
                        conflagCdReady = now + 10.0;
                        if (nextRandom(rng) < hit) {
                            float cDmg = (conflagMin + nextRandom(rng) * (conflagMax - conflagMin) + (1.5 / 3.5) * curFirePower) * conflagMult;
                            if (now < snfFireEnd) cDmg *= 1.15;
                            if (nextRandom(rng) < fireCrit) {
                                cDmg *= boltCritMult;
                                crits += 1.0;
                            }
                            conflagDamage += cDmg;
                            totalDamage += cDmg;
                            snfShadowEnd = now + 20.0;
                        } else {
                            misses += 1.0;
                        }
                        immolateTicks = 0.0;
                        immolateNext = NEVER;
                        ready = now + gcd;
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 17.0) { // SHADOWBURN
                    if (sburnCost > 0.0 && now >= shadowburnCdReady && mana >= sburnCost) {
                        mana -= sburnCost;
                        shadowburnCdReady = now + 15.0;
                        if (nextRandom(rng) < hit) {
                            float sDmg = (450.0 + nextRandom(rng) * 52.0 + (1.5 / 3.5) * curShadowPower);
                            if (now < snfShadowEnd) sDmg *= 1.15;
                            if (isbExpire > now) {
                                sDmg *= (1.0 + isbBonus);
                                if (isbCharges > 0.5) {
                                    isbCharges -= 1.0;
                                    isbConsumed += 1.0;
                                    if (isbCharges <= 0.0) isbExpire = 0.0;
                                }
                            }
                            if (nextRandom(rng) < crit) {
                                sDmg *= boltCritMult;
                                crits += 1.0;
                            }
                            shadowburnDamage += sDmg;
                            totalDamage += sDmg;
                            snfFireEnd = now + 20.0;
                        } else {
                            misses += 1.0;
                        }
                        ready = now + gcd;
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 23.0) { // SHADOW_BOLT FILLER
                    if (mana >= boltCost) {
                        mana -= boltCost;
                        castEnd = now + baseCastTime;
                        castSpellId = 1.0;
                        ready = max(now + gcd, castEnd);
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 19.0) { // INCINERATE FILLER
                    if (mana >= boltCost) {
                        mana -= boltCost;
                        castEnd = now + baseCastTime;
                        castSpellId = 4.0;
                        ready = max(now + gcd, castEnd);
                        acted = true;
                        eventCount += 1.0;
                    }
                } else if (rule == 20.0) { // SEARING PAIN FILLER
                    if (mana >= 190.0) {
                        mana -= 190.0;
                        castEnd = now + 1.5;
                        castSpellId = 5.0;
                        ready = max(now + gcd, castEnd);
                        acted = true;
                        eventCount += 1.0;
                    }
                }
            }

            if (!acted) {
                if (mana >= boltCost) {
                    mana -= boltCost;
                    castEnd = now + baseCastTime;
                    castSpellId = (fillerType == 1.0) ? 4.0 : (fillerType == 2.0) ? 5.0 : 1.0;
                    ready = max(now + gcd, castEnd);
                } else {
                    mana = min(maxMana, mana + tapGain);
                    taps += 1.0;
                    ready = now + gcd;
                }
                eventCount += 1.0;
            }
        }

        // 6. VARIABLE-STEP DISCRETE EVENT ADVANCE
        float nextDot = min(dotNext, min(agonyNext, min(doomNext, min(siphonNext, min(immolateNext, wrackNext)))));
        float nextTrinket = (trinketEnabled > 0.5) ? trinketReady : NEVER;
        float nextNow = min(ready, min(castEnd, min(nextDot, min(regenNext, nextTrinket))));

        if (nextNow <= now) {
            nextNow = now + 0.001;
        }

        if (isbExpire > now && isbCharges > 0.0) {
            float activeEnd = min(nextNow, isbExpire);
            if (activeEnd > now) {
                isbUptime += (activeEnd - now);
            }
        }

        now = nextNow;
        loops += 1.0;

        if (now >= duration) {
            done = 1.0;
            now = duration;
            break;
        }
    }

    // Output Render Target Selector
    if (uOutputTarget == 0) {
        gl_FragColor = vec4(now, mana, rng, done);
    } else if (uOutputTarget == 1) {
        gl_FragColor = vec4(ready, castEnd, castSpellId, regenNext);
    } else if (uOutputTarget == 2) {
        gl_FragColor = vec4(dotNext, dotTicks, agonyNext, agonyTicks);
    } else if (uOutputTarget == 3) {
        gl_FragColor = vec4(doomNext, siphonNext, siphonTicks, immolateNext);
    } else if (uOutputTarget == 4) {
        gl_FragColor = vec4(immolateTicks, isbCharges, isbExpire, wrackNext);
    } else if (uOutputTarget == 5) {
        gl_FragColor = vec4(wrackTicks, wrackEnd, conflagCdReady, shadowburnCdReady);
    } else if (uOutputTarget == 6) {
        gl_FragColor = vec4(snfShadowEnd, snfFireEnd, trinketEnd, trinketReady);
    } else if (uOutputTarget == 7) {
        gl_FragColor = vec4(totalDamage, boltDamage, dotDamage, agonyDamage);
    } else if (uOutputTarget == 8) {
        gl_FragColor = vec4(doomDamage, siphonDamage, immolateDamage, incinerateDamage);
    } else if (uOutputTarget == 9) {
        gl_FragColor = vec4(conflagDamage, shadowburnDamage, searingDamage, wrackDamage);
    } else if (uOutputTarget == 10) {
        gl_FragColor = vec4(casts, hits, crits, misses);
    } else if (uOutputTarget == 11) {
        gl_FragColor = vec4(taps, isbProcs, isbConsumed, loops);
    } else {
        float dps = (now > 0.0) ? (totalDamage / now) : 0.0;
        gl_FragColor = vec4(eventCount, dps, now, isbUptime);
    }
}
`;
