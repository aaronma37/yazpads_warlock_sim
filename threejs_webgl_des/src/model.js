// High-Fidelity Discrete Event Simulation (DES) Model & Configuration Contract
// Fully aligned with Authoritative C++ Oracle (src/sim/warlock/warlock_sim.cpp)

export const SPELLS = ['Shadow Bolt', 'Corruption', 'Bane of Agony', 'Immolate', 'Incinerate', 'Searing Pain', 'Bane of Doom', 'Soul Fire', 'Conflagrate', 'Shadowburn', 'Siphon Life', 'Wrack', 'Touch of the Grave', 'Hellfire'];
export const CPU_IDS = [1, 2, 5, 8, 12, 9, 6, 13, 11, 10, 17, 14, 30, null]; // Hellfire currently has no CPU oracle implementation.

export const APL_ACTION = Object.freeze({
  NONE: 0,
  LIFE_TAP: 1,
  NIGHTFALL_SHADOW_BOLT: 2,
  DECIMATION_SEARING_PAIN: 3,
  DECIMATION_SOUL_FIRE: 4,
  DEMONIC_BRAND_SEARING_PAIN: 5,
  CORRUPTION: 6,
  CURSE_OF_DOOM: 7,
  CURSE_OF_AGONY: 8,
  IMMOLATE: 9,
  CONFLAGRATE: 10,
  SHADOWBURN: 11,
  INCINERATE_FILLER: 12,
  SEARING_PAIN_FILLER: 13,
  DRAIN_SOUL_FILLER: 14,
  DRAIN_LIFE_FILLER: 15,
  SHADOW_BOLT_FILLER: 16,
  SIPHON_LIFE: 17,
  DRAIN_HOPE: 18,
  HELLFIRE: 19,
});

export const APL_COND = Object.freeze({
  ALWAYS: 0,
  MANA_LE: 1,               // Mana% <= param (e.g. 25.0)
  MANA_GE: 2,               // Mana% >= param
  TARGET_HP_LE: 3,          // Target HP% <= param (e.g. 35.0 execute)
  TARGET_HP_GE: 4,          // Target HP% >= param
  DOT_REM_LE: 5,            // DoT remaining <= param (seconds)
  FIGHT_TIME_GE: 6,         // Fight duration remaining >= param (seconds, e.g. 60.0 for Doom)
  FIGHT_TIME_LE: 7,         // Fight duration remaining <= param
  SHADOW_TRANCE: 8,         // Shadow Trance (Nightfall) proc active
  DECIMATION_ACTIVE: 9,     // Decimation buff active (<35% HP execute buff)
  DECIMATION_INACTIVE: 10,  // Decimation buff inactive
  DEMONIC_BRAND_MISSING: 11,// Demonic Brand debuff down or missing
  DOOM_MISSING: 12,         // Bane of Doom debuff not active
  ISB_ACTIVE: 13,           // ISB debuff active
  FIGHT_GE_DOT_MISSING: 14,// Fight time threshold and a named DoT/debuff missing
  NIGHTFALL_DOT_MISSING: 15,// Nightfall talent present and a named DoT missing
  FIGHT_GE_DOOM_AGONY_MISSING: 16, // Fight time threshold and both curses missing
  MANA_LT: 17,
  MANA_GT: 18,
  TARGET_HP_LT: 19,
  TARGET_HP_GT: 20,
  FIGHT_TIME_GT: 21,
  FIGHT_TIME_LT: 22,
  DOT_REM_LT: 23,
});

export const MAX_APL_RULES = 32;

export const DEFAULTS = Object.freeze({
  race: 'HUMAN', racialPolicy: 'execute', targetIsBeast: false, maxHealth: 0, duration: 180, iterations: 4096, seed: 42, rotation: 'shadow',
  spellPower: 500, shadowPower: 0, firePower: 0, intellect: 200, stamina: 220, spirit: 100, hit: 12, crit: 15, mp5: 20,
  distance: 30, resistance: 0, penetration: 0, tapThreshold: 25, bossArmor: 3731,
  book: false, charges: false, partialResists: true, piercing: true,
  corruption: true, agony: true, immolate: false, instantCorruption: true,
  nightfall: true, isb: true, ruin: true, improvedTap: true, amplifyCurse: false,
  sacImp: false, sacSucc: false, masterDemo: 0, petChoice: 'none',
  trinketSP: 0, trinketDuration: 20, trinketCD: 120,
  shadowMultiplier: 1.0, fireMultiplier: 1.0,
  tapBonus: 0.0, nightfallChance: 0.0, isbBonus: 0.0, ruinRank: 0,
  shadowMasteryBonus: 0.0, improvedCorruptionBonus: 0.0,
  conflagrate: false, shadowburn: false, curseOfDoom: false, incinerate: false,
  decimation: false, decimationSearing: false, demonicBrand: false, demonicBrandRank: 0,
  baneRank: 0, decimationRank: 0,
  siphonLife: false, drainHope: false,
  malevolence: 0.0, afBonus: 0.0, aftermathBonus: 0.0, cataclysmCostMult: 1.0, improvedAgonyBonus: 0.0, felVitalityBonus: 0.0, maledictionBonus: 0.0, improvedDrainsBonus: 0.0, soulSiphonBonus: 0.0,
  demonicEnergies: 0.0, demonicKnowledge: 0,
  snfChance: 0.0, snfBonus: 0.0, dotCrit: 1.5, fnbCrit: 0.0, petMult: 1.0, petFireboltMult: 1.0, petMeleeMult: 1.0, petLashMult: 1.0, brandMult: 1.0, corrMultiplier: 0.0,
  aplRules: null,
});

export function validate(input) {
  const c = { ...DEFAULTS, ...input };
  const bounds = { maxHealth: [0, 100000], duration: [1, 1800], iterations: [1, 1048576], seed: [0, 4294967295],
    spellPower: [0, 10000], shadowPower: [0, 10000], firePower: [0, 10000],
    intellect: [0, 5000], stamina: [0, 5000], spirit: [0, 5000], hit: [0, 100],
    crit: [0, 100], mp5: [0, 10000], resistance: [0, 1000], penetration: [0, 1000], tapThreshold: [0, 100], bossArmor: [0, 100000],
    masterDemo: [0, 5], demonicBrandRank: [0, 3], trinketSP: [0, 10000], trinketDuration: [0, 600], trinketCD: [0, 1200],
    shadowMultiplier: [0, 10], fireMultiplier: [0, 10],
    malevolence: [0, 10], afBonus: [0, 1], aftermathBonus: [0, 1], cataclysmCostMult: [0, 2], improvedAgonyBonus: [0, 1], felVitalityBonus: [0, 1], maledictionBonus: [0, 1], improvedDrainsBonus: [0, 1], soulSiphonBonus: [0, 1],
    tapBonus: [0, 0.2], nightfallChance: [0, 0.04], isbBonus: [0, 0.2], ruinRank: [0, 5],
    shadowMasteryBonus: [0, 1], improvedCorruptionBonus: [0, 0.1],
    demonicEnergies: [0, 2], demonicKnowledge: [0, 3], baneRank: [0, 5], decimationRank: [0, 2],
    snfChance: [0, 1], snfBonus: [0, 1], dotCrit: [1, 3], fnbCrit: [0, 100], petMult: [0, 10], petFireboltMult: [0, 10], petMeleeMult: [0, 10], petLashMult: [0, 10], brandMult: [0, 10], corrMultiplier: [0, 10] };
  for (const [key, [lo, hi]] of Object.entries(bounds)) {
    if (typeof c[key] !== 'number' || !Number.isFinite(c[key]) || c[key] < lo || c[key] > hi)
      throw new Error(`${key} must be a number between ${lo} and ${hi}.`);
  }
  for (const key of ['iterations', 'seed', 'duration', 'masterDemo', 'demonicBrandRank', 'demonicKnowledge', 'ruinRank', 'baneRank', 'decimationRank']) if (!Number.isInteger(c[key])) throw new Error(`${key} must be an integer.`);
  if (![0, 30, 60, 120].includes(c.distance)) throw new Error('Supported distances: 0, 30, 60, 120 yards.');
  if (!['execute', 'cooldown', 'align-execute', 'align-doom'].includes(c.racialPolicy)) throw new Error('Unsupported racial policy.');
  if (!['HUMAN', 'GNOME', 'ORC', 'TROLL', 'UNDEAD'].includes(c.race)) throw new Error('Unsupported race.');
  if (!['shadow', 'fire', 'searing', 'bolt'].includes(c.rotation)) throw new Error('Unsupported rotation.');
  if (!['none', 'imp', 'succubus'].includes(c.petChoice)) throw new Error('Unsupported pet choice.');
  for (const [key, value] of Object.entries(DEFAULTS)) {
    if (key === 'aplRules') continue;
    if (typeof value === 'boolean' && typeof c[key] !== 'boolean') throw new Error(`${key} must be boolean.`);
  }
  for (const key of Object.keys(input)) if (!(key in DEFAULTS)) throw new Error(`Unsupported option: ${key}`);
  return c;
}

export function buildDefaultAPLRules(c) {
  const rot = c.rotation;
  const rules = [];

  const add = (action, cond = APL_COND.ALWAYS, param = 0.0, targetSpell = 0, enabled = 1) => {
    if (rules.length < MAX_APL_RULES) {
      rules.push({ action, cond, param, targetSpell, enabled });
    }
  };

  if (rot === 'searing') {
    // Dedicated DP_RUIN_FIRE APL
    add(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 57.0, 2);
    add(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 2.5, 2);
    add(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 2.5, 1);
    add(APL_ACTION.IMMOLATE, APL_COND.DOT_REM_LE, 2.5, 3);
    if (c.demonicBrand) add(APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, APL_COND.DEMONIC_BRAND_MISSING, 0.0, 5);
    if (c.decimation) add(APL_ACTION.DECIMATION_SEARING_PAIN, APL_COND.DECIMATION_INACTIVE, 28.0, 5);
    add(APL_ACTION.LIFE_TAP, APL_COND.MANA_LE, 17.0);
    if (c.decimation) add(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 28.0, 13);
    add(APL_ACTION.LIFE_TAP, APL_COND.MANA_LE, 37.0);
    add(APL_ACTION.SEARING_PAIN_FILLER, APL_COND.ALWAYS);
    return rules;
  }

  // Rule 0: Life Tap resource safeguard
  add(APL_ACTION.LIFE_TAP, APL_COND.MANA_LE, c.tapThreshold !== undefined ? c.tapThreshold : 25.0);

  if (rot === 'bolt') {
    add(APL_ACTION.SHADOW_BOLT_FILLER, APL_COND.ALWAYS);
    return rules;
  }

  // Decimation Soul Fire (<35% HP execute)
  if (c.decimation) {
    if (rot === 'searing' || rot === 'fire' || c.decimationSearing) {
      add(APL_ACTION.DECIMATION_SEARING_PAIN, APL_COND.DECIMATION_INACTIVE, 35.0, 5);
    }
    add(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 35.0, 13);
  }

  // Demonic Brand weave for pet damage empowerment
  if (c.demonicBrand) {
    add(APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, APL_COND.DEMONIC_BRAND_MISSING, 0.0, 5);
  }

  // Immolate DoT & Conflagrate Burst
  if (c.immolate) {
    add(APL_ACTION.IMMOLATE, APL_COND.DOT_REM_LE, 0.0, 3);
  }
  if (c.conflagrate) {
    add(APL_ACTION.CONFLAGRATE, APL_COND.ALWAYS);
  }

  // Nightfall Shadow Trance instant Shadow Bolt
  if (c.nightfall) {
    add(APL_ACTION.NIGHTFALL_SHADOW_BOLT, APL_COND.SHADOW_TRANCE);
  }

  // Corruption DoT
  if (c.corruption) {
    add(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 0.0, 1);
  }

  // Adaptive Curse: Bane of Doom (>60s left) / Bane of Agony
  if (c.curseOfDoom) {
    add(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 60.0, 2);
  }
  if (c.agony) {
    add(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 0.0, 2);
  }

  // Siphon Life DoT
  if (c.siphonLife) {
    add(APL_ACTION.SIPHON_LIFE, APL_COND.DOT_REM_LE, 0.0, 15);
  }

  // Wrack / Drain Hope Channel
  if (c.drainHope) {
    add(APL_ACTION.DRAIN_HOPE, APL_COND.ALWAYS);
  }

  // Shadowburn on cooldown
  if (c.shadowburn) {
    add(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
  }

  // Primary Rotational Fallback Filler
  if (rot === 'fire') {
    add(APL_ACTION.INCINERATE_FILLER, APL_COND.ALWAYS);
  } else if (rot === 'searing') {
    add(APL_ACTION.SEARING_PAIN_FILLER, APL_COND.ALWAYS);
  } else {
    add(APL_ACTION.SHADOW_BOLT_FILLER, APL_COND.ALWAYS);
  }

  return rules;
}

export function encodeAPLRule(rule) {
  const action = (rule.action || 0) & 0xFF;
  const cond1 = (rule.cond !== undefined ? rule.cond : (rule.cond1 || 0)) & 0xFF;
  const targetSpell1 = (rule.targetSpell !== undefined ? rule.targetSpell : (rule.targetSpell1 || 0)) & 0xFF;
  const param1 = Number(rule.param !== undefined ? rule.param : (rule.param1 || 0.0));

  const cond2 = (rule.cond2 || 0) & 0xFF;
  const targetSpell2 = (rule.targetSpell2 || 0) & 0xFF;
  const param2 = Number(rule.param2 || 0.0);

  const enabled = (rule.enabled !== false && rule.enabled !== 0) ? 1 : 0;
  const header0 = (enabled << 24) | (targetSpell1 << 16) | (cond1 << 8) | action;
  const header1 = (targetSpell2 << 8) | cond2;

  return { header0, param0: param1, header1, param1: param2 };
}

// Explicit word schemas keep JS and GLSL offsets in one place. All scalar
// members have four-byte alignment. Never encode counters/timestamps as floats.
export const CONFIG = {
  race: 'u32', racialPolicy: 'u32', targetIsBeast: 'u32', basePower: 'f32', maxHealth: 'f32', end: 'u32', travel: 'u32', corrCast: 'u32', filler: 'u32', corr: 'u32', agony: 'u32', immolate: 'u32',
  baneRank: 'u32', decimationRank: 'u32',
  charges: 'u32', partial: 'u32', piercing: 'u32',
  petChoice: 'u32', trinketDuration: 'u32', trinketCD: 'u32',
  conflagrate: 'u32', shadowburn: 'u32', curseOfDoom: 'u32', incinerate: 'u32',
  decimation: 'u32', demonicBrand: 'u32', demonicBrandRank: 'u32', siphonLife: 'u32', drainHope: 'u32',
  hellfirePowerOffset: 'f32', power: 'f32', maxMana: 'f32', tapGain: 'f32', tapThreshold: 'f32', mp5: 'f32',
  hit: 'f32', crit: 'f32', fireCrit: 'f32', shadowCrit: 'f32', directCrit: 'f32', nightfall: 'f32', isb: 'f32',
  resistance: 'f32', penetration: 'f32', boltCost: 'f32', boltMin: 'f32', boltMax: 'f32',
  corrCost: 'f32', corrBase: 'f32', corrMultiplier: 'f32', immCost: 'f32', immDirect: 'f32', immTick: 'f32',
  shadowMult: 'f32', fireMult: 'f32', shadowMasteryBonus: 'f32', afBonus: 'f32', aftermathBonus: 'f32', maledictionBonus: 'f32', improvedCorruptionBonus: 'f32', improvedDrainsBonus: 'f32', soulSiphonBonus: 'f32',
  demonicEnergies: 'f32',
  trinketSP: 'f32', petSP: 'f32', petAP: 'f32', snfChance: 'f32', snfBonus: 'f32', dotCrit: 'f32', fnbCrit: 'f32', petMult: 'f32', petFireboltMult: 'f32', petMeleeMult: 'f32', petLashMult: 'f32', brandMult: 'f32',
  cataclysmCostMult: 'f32', improvedAgonyBonus: 'f32', felVitalityBonus: 'f32', amplifyCurse: 'u32',
  ...Object.fromEntries(Array.from({length: MAX_APL_RULES}, (_, i) => [`aplHeader0_${i}`, 'u32'])),
  ...Object.fromEntries(Array.from({length: MAX_APL_RULES}, (_, i) => [`aplParam0_${i}`, 'f32'])),
  ...Object.fromEntries(Array.from({length: MAX_APL_RULES}, (_, i) => [`aplHeader1_${i}`, 'u32'])),
  ...Object.fromEntries(Array.from({length: MAX_APL_RULES}, (_, i) => [`aplParam1_${i}`, 'f32'])),
};

export const STATE = {
  racialReady:'u32', racialEnd:'u32', eurekaCharges:'u32', graveReady:'u32', doomEnd:'u32', castBonus:'f32', corrBonus:'f32', agonyBonus:'f32', immBonus:'f32', siphonBonus:'f32', initialized:'u32', done:'u32', now:'u32', size:'u32', events:'u32', highWater:'u32',
  casting:'u32', ready:'u32', trance:'u32', tranceEnd:'u32', isbEnd:'u32', isbCharges:'u32',
  corrTicks:'u32', corrGen:'u32', corrEnd:'u32', agonyTicks:'u32', agonyGen:'u32', agonyEnd:'u32',
  immTicks:'u32', immGen:'u32', immEnd:'u32',
  siphonTicks:'u32', siphonGen:'u32', siphonEnd:'u32', drainHopeEnd:'u32', drainHopeReady:'u32',
  taps:'u32', procs:'u32', isbProcs:'u32', isbConsumed:'u32', traceCount:'u32', rngCalls:'u32',
  petCasts:'u32',
  trinketEnd:'u32', trinketReady:'u32', petReady:'u32',
  conflagReady:'u32', shadowburnReady:'u32', soulFireReady:'u32', doomActive:'u32',
  amplifyReady:'u32', agonyAmplified:'u32',
  decimationEnd:'u32', brandCharges:'u32', brandEnd:'u32',
  snfShadowEnd:'u32', snfFireEnd:'u32',
  mana:'f32', spent:'f32', gained:'f32', total:'f32', petMana:'f32', petDamage:'f32',
  petBrandDamage:'f32',
  petMeleeDamage:'f32', petMeleeCasts:'u32', petMeleeHits:'u32', petMeleeCrits:'u32', petMeleeMisses:'u32',
  petSpellDamage:'f32', petSpellCasts:'u32', petSpellHits:'u32', petSpellCrits:'u32', petSpellMisses:'u32',
  ...Object.fromEntries(Array.from({length:8}, (_, i) => [`rng${i}`, 'u32'])),
  ...Object.fromEntries(SPELLS.flatMap((_, i) => [[`damage${i}`, 'f32'], [`casts${i}`, 'u32'], [`hits${i}`, 'u32'], [`crits${i}`, 'u32'], [`misses${i}`, 'u32']])),
};

export const STATE_WORDS = Object.keys(STATE).length;
export const FAST_STATE = Object.fromEntries(Object.entries(STATE).filter(([key]) => !/^(damage|casts|hits|crits|misses)\d+$/.test(key)));
export const FAST_STATE_WORDS = Object.keys(FAST_STATE).length;
export const HEAP_CAPACITY = 40;
export const TRACE_CAPACITY = 256;
export const TRACE_WORDS = 8;
export const CONFIG_WORDS = Object.keys(CONFIG).length;
export const APL_HEADER0_OFFSET = Object.keys(CONFIG).indexOf('aplHeader0_0');
export const APL_PARAM0_OFFSET = Object.keys(CONFIG).indexOf('aplParam0_0');
export const APL_HEADER1_OFFSET = Object.keys(CONFIG).indexOf('aplHeader1_0');
export const APL_PARAM1_OFFSET = Object.keys(CONFIG).indexOf('aplParam1_0');

export function packConfig(input) {
  const c = validate(input);
  const int = 136 + c.intellect;
  let sMult = c.shadowMultiplier;
  let fMult = c.fireMultiplier;

  // Baseline sacrifice multipliers: Sac Imp = +15% Shadow, Sac Succ = +15% Fire
  if (c.sacImp && c.shadowMultiplier === 1.0) sMult *= 1.15;
  if (c.sacSucc && c.fireMultiplier === 1.0) fMult *= 1.15;

  if (c.masterDemo > 0 && c.shadowMultiplier === 1.0 && c.fireMultiplier === 1.0) {
    if (c.petChoice === 'succubus' || (c.petChoice === 'none' && c.rotation !== 'fire' && c.rotation !== 'searing')) {
      sMult *= (1 + c.masterDemo * 0.02);
    }
    if (c.petChoice === 'imp' || (c.petChoice === 'none' && (c.rotation === 'fire' || c.rotation === 'searing'))) {
      fMult *= (1 + c.masterDemo * 0.02);
    }
  }

  const petChoiceMap = { none: 0, imp: 1, succubus: 2 };
  const rawRules = Array.isArray(c.aplRules) && c.aplRules.length > 0 ? c.aplRules : buildDefaultAPLRules(c);

  const baseCrit = (1.7 + int / 60.6 + c.crit + 2) / 100;
  const shadowCrit = (1.7 + int / 60.6 + c.crit + 2 + (c.malevolence || 0)) / 100;

  const impCorrRanks = c.improvedCorruptionBonus > 0 ? Math.round(c.improvedCorruptionBonus / 0.02) : (c.instantCorruption ? 5 : 0);
  const corrCastTime = Math.max(0, 2.0 - 0.4 * impCorrRanks);

  const felVitalityBonus = c.felVitalityBonus !== undefined ? c.felVitalityBonus : (c.felVitalityRank ? c.felVitalityRank * 0.05 : c.felVitality ? (typeof c.felVitality === 'number' ? c.felVitality * 0.05 : 0.15) : 0.0);

  const values = {
    race: ({HUMAN:0, ORC:1, TROLL:2, GNOME:3, UNDEAD:4})[c.race],
    racialPolicy: ({execute:0, cooldown:1, 'align-execute':2, 'align-doom':3})[c.racialPolicy], targetIsBeast: +c.targetIsBeast,
    basePower: c.spellPower, maxHealth: c.maxHealth || 1500 + c.stamina * 10,
    end: c.duration * 1e6, travel: c.distance / 24 * 1e6, corrCast: Math.round(corrCastTime * 1e6),
    baneRank: c.baneRank, decimationRank: c.decimationRank,
    filler: ({shadow:0, bolt:0, fire:4, searing:5})[c.rotation],
    corr: +(c.corruption && c.rotation !== 'bolt'), agony: +(c.agony && c.rotation !== 'bolt'),
    immolate: +(c.immolate && c.rotation !== 'bolt'), charges: +c.charges, partial: +c.partialResists, piercing: +c.piercing,
    petChoice: petChoiceMap[c.petChoice] || 0,
    trinketDuration: c.trinketDuration * 1e6,
    trinketCD: c.trinketCD * 1e6,
    conflagrate: +(c.conflagrate && c.rotation !== 'bolt'),
    incinerate: +(c.incinerate && c.rotation !== 'bolt'),
    shadowburn: +(c.shadowburn && c.rotation !== 'bolt'),
    curseOfDoom: +(c.curseOfDoom && c.rotation !== 'bolt'),
    decimation: +c.decimation,
    demonicBrand: +c.demonicBrand,
    demonicBrandRank: c.demonicBrandRank,
    siphonLife: +c.siphonLife,
    drainHope: +c.drainHope,
    hellfirePowerOffset: c.firePower - (c.rotation === 'fire' || c.rotation === 'searing' ? c.firePower : c.shadowPower),
    power: c.spellPower + (c.rotation === 'fire' || c.rotation === 'searing' ? c.firePower : c.shadowPower),
    maxMana: (1393 + int * 15) * (c.race === 'GNOME' ? 1.05 : 1) * (1.0 + felVitalityBonus),
    tapGain: (430 + 147 + c.spirit) * (1 + (c.tapBonus > 0 ? c.tapBonus : c.improvedTap ? 0.2 : 0)),
    tapThreshold: c.tapThreshold,
    mp5: c.mp5,
    hit: Math.min(1, .83 + c.hit / 100),
    crit: baseCrit,
    fireCrit: baseCrit,
    shadowCrit: shadowCrit,
    directCrit: c.ruinRank > 0 ? 1.5 + c.ruinRank * 0.1 : c.ruin ? 2.0 : 1.5,
    nightfall: c.nightfallChance > 0 ? c.nightfallChance : c.nightfall ? 0.04 : 0.0,
    isb: c.isbBonus > 0 ? c.isbBonus : c.isb ? 0.20 : 0.0,
    resistance: c.resistance,
    penetration: c.penetration,
    boltCost: (c.book ? 380 : 370) * (c.cataclysmCostMult !== undefined ? c.cataclysmCostMult : (c.cataclysmRank === 3 || c.cataclysm === 3 ? 0.90 : c.cataclysmRank === 2 || c.cataclysm === 2 ? 0.94 : c.cataclysmRank === 1 || c.cataclysm === 1 ? 0.97 : 1.0)),
    boltMin: c.book ? 253 : 246,
    boltMax: c.book ? 283 : 274,
    corrCost: c.book ? 340 : 290,
    corrBase: c.book ? 73 : 57,
    corrMultiplier: (c.corrMultiplier > 0 ? c.corrMultiplier :
      (1 + (c.shadowMasteryBonus || 0) + (c.maledictionBonus || 0) + (c.improvedCorruptionBonus > 0 ? c.improvedCorruptionBonus : c.instantCorruption ? 0.1 : 0.0)) * sMult),
    immCost: (c.book ? 380 : 370) * (c.cataclysmCostMult !== undefined ? c.cataclysmCostMult : (c.cataclysmRank === 3 || c.cataclysm === 3 ? 0.90 : c.cataclysmRank === 2 || c.cataclysm === 2 ? 0.94 : c.cataclysmRank === 1 || c.cataclysm === 1 ? 0.97 : 1.0)),
    immDirect: c.book ? 158 : 157.5,
    immTick: c.book ? 55 : 52,
    shadowMult: sMult,
    fireMult: fMult,
    shadowMasteryBonus: c.shadowMasteryBonus || 0.0,
    afBonus: c.afBonus || 0.0,
    aftermathBonus: c.aftermathBonus !== undefined ? c.aftermathBonus : (c.aftermathRank ? c.aftermathRank * 0.10 : c.aftermath ? c.aftermath * 0.10 : 0.0),
    maledictionBonus: c.maledictionBonus || 0.0,
    improvedCorruptionBonus: c.improvedCorruptionBonus || 0.0,
    improvedDrainsBonus: c.improvedDrainsBonus || 0.0,
    soulSiphonBonus: c.soulSiphonBonus || 0.0,
    demonicEnergies: c.demonicEnergies !== undefined ? c.demonicEnergies : 1.0,
    trinketSP: c.trinketSP,
    petSP: c.spellPower * 0.10 + (c.demonicKnowledge ? c.demonicKnowledge * 20.0 : 0.0),
    petAP: c.spellPower * (1.0 / 6.0),
    snfChance: c.snfChance,
    snfBonus: c.snfBonus !== undefined ? c.snfBonus : (c.snfChance > 0.0 ? c.snfChance * 0.10 : 0.0),
    dotCrit: c.dotCrit !== undefined ? c.dotCrit : 1.5,
    fnbCrit: c.fnbCrit / 100,
    petMult: c.petMult || 1.0,
    petFireboltMult: c.petFireboltMult || c.petMult || 1.0,
    petMeleeMult: (c.petMeleeMult || c.petMult || 1.0) * (1.0 - Math.max(0, c.bossArmor !== undefined ? c.bossArmor : 3731.0) / (Math.max(0, c.bossArmor !== undefined ? c.bossArmor : 3731.0) + 400.0 + 85.0 * 60.0)),
    petLashMult: c.petLashMult || c.petMult || 1.0,
    brandMult: c.brandMult || 1.0,
    cataclysmCostMult: c.cataclysmCostMult !== undefined ? c.cataclysmCostMult : (c.cataclysmRank === 3 || c.cataclysm === 3 ? 0.90 : c.cataclysmRank === 2 || c.cataclysm === 2 ? 0.94 : c.cataclysmRank === 1 || c.cataclysm === 1 ? 0.97 : 1.0),
    improvedAgonyBonus: c.improvedAgonyBonus !== undefined ? c.improvedAgonyBonus : (c.improvedAgonyRank ? c.improvedAgonyRank * 0.05 : c.improvedAgony ? (typeof c.improvedAgony === 'number' ? c.improvedAgony * 0.05 : 0.10) : 0.0),
    felVitalityBonus,
    amplifyCurse: +c.amplifyCurse,
  };

  // Populate bytecode entries (2 conditions per rule) for all rules
  for (let i = 0; i < MAX_APL_RULES; i++) {
    if (i < rawRules.length) {
      const { header0, param0, header1, param1 } = encodeAPLRule(rawRules[i]);
      values[`aplHeader0_${i}`] = header0;
      values[`aplParam0_${i}`] = param0;
      values[`aplHeader1_${i}`] = header1;
      values[`aplParam1_${i}`] = param1;
    } else {
      values[`aplHeader0_${i}`] = 0;
      values[`aplParam0_${i}`] = 0.0;
      values[`aplHeader1_${i}`] = 0;
      values[`aplParam1_${i}`] = 0.0;
    }
  }

  const words = new Uint32Array(CONFIG_WORDS);
  const floats = new Float32Array(words.buffer);
  Object.entries(CONFIG).forEach(([key, type], i) => { (type === 'f32' ? floats : words)[i] = values[key]; });
  return words;
}

export function packMultiConfig(configs) {
  const words = new Uint32Array(configs.length * CONFIG_WORDS);
  for (let i = 0; i < configs.length; i++) {
    const packed = packConfig(configs[i]);
    words.set(packed, i * CONFIG_WORDS);
  }
  return words;
}

export function decodeStates(buffer, count) {
  const u = new Uint32Array(buffer), f = new Float32Array(buffer);
  return Array.from({length:count}, (_, lane) => Object.fromEntries(Object.entries(STATE).map(([key, type], j) => [key, (type === 'f32' ? f : u)[lane * STATE_WORDS + j]])));
}

export const COMPACT_WORDS = 32 + (SPELLS.length - 6) * 3;
export const COMPACT_STRIPES = Math.ceil(COMPACT_WORDS / 16);
