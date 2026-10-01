#pragma once
#include <cstdint>

// Experimental ABI v2: tightly packed 32-bit fields, shared by C++, WASM and WGSL.
// Config durations are seconds; State timestamps are integer microseconds.
// Config contains resolved combat values, APL priority rule sequence, and mechanics.
struct Config {
    float duration, castTime, gcd, maxMana;
    float spellPower, hit, crit, boltCrit;
    float dotCrit, tapGain, mp5, tapThreshold;
    float boltCost, corruptionCost, agonyCost, doomCost;
    float siphonCost, immolateCost, conflagCost, shadowburnCost;
    float boltMin, boltMax, dotBase, dotMultiplier;
    float isbBonus, isbCharges, isbAllShadow, executeBonus;
    float agonyBase, agonyMultiplier, doomBase, doomMultiplier;
    float siphonBase, siphonMultiplier, immolateCastTime, immolateMultiplier, immolateDirectMultiplier;
    float immolateMin, immolateMax, immolateDotBase, corruptionCastTime;
    float corruptionMultiplier, corruptionSpCoeff, shadowBoltMultiplier, shadowPower;
    float firePower, fireCrit, conflagMultiplier, shadowburnMultiplier;
    float snfShadowBonus, snfFireBonus, snfConflagSaveChance, fnbCritBonus;
    float conflagMin, conflagMax, sburnMin, sburnMax;
    float petType, petCastInterval, petBaseMin, petBaseMax;
    float petSpRatio, petMultiplier, petLopBase, petLopSpRatio;
    float petLopCd, petMeleeBase, petApRatio, petArmorMultiplier;
    float petSpellHit, petSpellCrit, petMeleeMissPct, petMeleeDodgePct;
    float petGlancePct, petGlanceMultiplier, petMeleeMultiplier, petLopMultiplier;
    float petLopCost, petManaManagement, petManaMax, petMp5;
    float petFlatSP, petMeleeCritPct, petSpellPiercing, petPiercingBonus;
    float targetResist, spellPen, partialResistEnabled, petTapGain;
    float ampCurse, fillerType, ruleCount, nightfall;
    float rule0, rule1, rule2, rule3;
    float rule4, rule5, rule6, rule7;
    float rule8, rule9, rule10, rule11;
    float rule12, rule13, rule14, rule15;
    float trinketBonus, trinketDuration, trinketCooldown, trinketEnabled;
    float searingCost, searingCastTime, searingMultiplier, searingCritBonus;
    float agonyRefreshSec, corruptionRefreshSec, siphonRefreshSec, immolateRefreshSec;
    float wrackCost, wrackBase, wrackSpCoeff, wrackMultiplier;
    float wrackSoulSiphonPerEffect, wrackChannelTime, wrackTickInterval;
    float doomMinTimeRemaining;
    float demonicBrandRank, demonicBrandMultiplier;
    float soulFireCost, soulFireCastTime, soulFireCooldown, soulFireMultiplier;
    float touchOfTheGraveDamage;
    float racialType, racialPolicy, racialDuration, racialCooldown;
    float racialBonus, racialPad0, racialPad1, racialPad2;
};

struct State {
    uint32_t initialized=1, rng=0, now=0, done=0;
    uint32_t ready=0, castEnd=UINT32_MAX, castSpellId=0, regenNext=5000000;
    uint32_t dotNext=UINT32_MAX, dotTicks=0, tranceEnd=0, isbExpire=0;
    uint32_t agonyNext=UINT32_MAX, agonyTicks=0, doomNext=UINT32_MAX, siphonNext=UINT32_MAX;
    uint32_t siphonTicks=0, immolateNext=UINT32_MAX, immolateTicks=0, isbCharges=0;
    uint32_t conflagrateCdReady=0, shadowburnCdReady=0, snfShadowEnd=0, snfFireEnd=0;
    float mana=0, boltDamage=0, dotDamage=0, agonyDamage=0;
    float doomDamage=0, siphonDamage=0, immolateDamage=0, isbUptimeUs=0;
    float incinerateDamage=0, conflagrateDamage=0, shadowburnDamage=0, petDamage=0;
    float petMeleeDamage=0, petLopDamage=0, petMana=0;
    uint32_t doomHits=0;
    uint32_t bolts=0, dots=0, ticks=0, taps=0;
    uint32_t procs=0, consumed=0, misses=0, crits=0;
    uint32_t agonies=0, dooms=0, siphons=0, immolates=0;
    uint32_t immolateDirectHits=0, immolateDotHits=0;
    uint32_t incinerates=0, incinerateHits=0, incinerateMisses=0, incinerateCrits=0;
    uint32_t conflagrates=0, shadowburns=0, isbProcs=0, isbConsumed=0;
    uint32_t loops=0, events=0, petNext=UINT32_MAX, petLopNext=UINT32_MAX;
    uint32_t petCasts=0, petHits=0, petCrits=0, petMeleeCasts=0;
    uint32_t petMeleeHits=0, petMeleeCrits=0, petLopCasts=0, petLopHits=0;
    uint32_t petLopCrits=0, corruptionCrits=0, corruptionTicks=0, doomCrits=0;
    uint32_t trinketEnd=0, trinketReady=0;
    float searingDamage=0;
    uint32_t searingCasts=0, searingHits=0, searingCrits=0;
    float wrackDamage=0;
    uint32_t wracks=0, wrackHits=0, wrackCrits=0, wrackNext=UINT32_MAX, wrackTicks=0, wrackEnd=0;
    uint32_t demonicBrandExpire=0, demonicBrandCharges=0;
    float demonicBrandDamage=0;
    uint32_t decimationExpire=0, soulFireCdReady=0, soulFires=0, soulFireHits=0, soulFireCrits=0;
    float soulFireDamage=0;
    uint32_t agonyHits=0, agonyCrits=0, siphonHits=0, siphonCrits=0;
    uint32_t immolateMisses=0, soulFireMisses=0, searingMisses=0, totgCdReady=0;
    uint32_t totgProcs=0;
    float totgDamage=0;
    uint32_t racialEnd=0, racialReady=0;
};

static_assert(sizeof(Config)==576 && sizeof(State)==448);


// Output must contain candidateCount * replicas State entries in candidate-major order.
// step=0 selects next-event scheduling; 1000/10000/50000 select fixed microseconds.
extern "C" int simulate_batch(const Config* configs, uint32_t candidateCount,
                              uint32_t replicas, uint32_t seed, uint32_t step,
                              State* output);
