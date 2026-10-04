// Genetic Algorithm APL Synthesis Engine for Classic WoW Warlock GPU Simulator
// Synthesizes optimal Action Priority Lists (APL) using continuous parameter evolution,
// discrete condition mutation, and index swap operations with exactly 2 copies per action.

import { runMultiSimulation } from './engine.js';
import { buildFightConfig } from './config_builder.js';
import { APL_ACTION, APL_COND, MAX_APL_RULES } from './model.js';
import { getPresetPetAndSac } from './presets.js';
import { TalentGraph } from './genetic_optimizer.js';

// 16 Authentic Spell Actions in the simulator
export const APL_SYNTHESIS_ACTIONS = [
  { id: 'tap', spell: 'Life Tap', icon: 'Spell_Shadow_BurningSpirit.png', action: APL_ACTION.LIFE_TAP, category: 'Resource', defaultRaw: 'mana_pct <= 20' },
  { id: 'nightfall', spell: 'Nightfall: Shadow Bolt', icon: 'Spell_Shadow_Twilight.png', action: APL_ACTION.NIGHTFALL_SHADOW_BOLT, category: 'Proc', defaultRaw: 'buff.shadow_trance' },
  { id: 'brand', spell: 'Demonic Brand Refresher', icon: 'ability_demonhunter_chaoticimprint_fire.png', action: APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, category: 'Debuff', defaultRaw: 'debuff.demonic_brand_missing' },
  { id: 'decimateSearing', spell: 'Decimation: Searing Pain', icon: 'Spell_Fire_SoulBurn.png', action: APL_ACTION.DECIMATION_SEARING_PAIN, category: 'Execute', defaultRaw: 'decimation.inactive' },
  { id: 'decimateSoulFire', spell: 'Decimation: Soul Fire', icon: 'Spell_Fire_Fireball.png', action: APL_ACTION.DECIMATION_SOUL_FIRE, category: 'Execute', defaultRaw: 'decimation.active' },
  { id: 'curse', spell: 'Bane of Doom', icon: 'Spell_Shadow_AuraOfDarkness.png', action: APL_ACTION.CURSE_OF_DOOM, category: 'Curse', defaultRaw: 'target_ttd >= 60 && !target.has_debuff("Bane of Doom")' },
  { id: 'agony', spell: 'Bane of Agony', icon: 'Spell_Shadow_CurseOfSargeras.png', action: APL_ACTION.CURSE_OF_AGONY, category: 'Curse', defaultRaw: '!target.has_debuff("Bane of Doom") && !target.has_debuff("Bane of Agony")' },
  { id: 'corr', spell: 'Corruption', icon: 'Spell_Shadow_AbominationExplosion.png', action: APL_ACTION.CORRUPTION, category: 'DoT', defaultRaw: 'target_ttd >= 12 && !target.has_debuff("Corruption")' },
  { id: 'immo', spell: 'Immolate', icon: 'Spell_Fire_Immolation.png', action: APL_ACTION.IMMOLATE, category: 'DoT', defaultRaw: 'target_ttd >= 12 && !target.has_debuff("Immolate")' },
  { id: 'conflag', spell: 'Conflagrate', icon: 'Spell_Fire_Fireball.png', action: APL_ACTION.CONFLAGRATE, category: 'Direct', defaultRaw: 'target.debuff_remains("Immolate") < 6' },
  { id: 'shadowburn', spell: 'Shadowburn', icon: 'Spell_Shadow_ScourgeBuild.png', action: APL_ACTION.SHADOWBURN, category: 'Direct', defaultRaw: 'true' },
  { id: 'incinerate', spell: 'Incinerate', icon: 'Spell_Fire_Burnout.png', action: APL_ACTION.INCINERATE_FILLER, category: 'Direct', defaultRaw: 'true' },
  { id: 'searing', spell: 'Searing Pain', icon: 'Spell_Fire_SoulBurn.png', action: APL_ACTION.SEARING_PAIN_FILLER, category: 'Direct', defaultRaw: 'true' },
  { id: 'siphon', spell: 'Siphon Life', icon: 'Spell_Shadow_Requiem.png', action: APL_ACTION.SIPHON_LIFE, category: 'DoT', defaultRaw: 'target.debuff_remains("Siphon Life") <= 0 && target_ttd >= 12' },
  { id: 'wrack', spell: 'Wrack', icon: 'ability_deathknight_hemorrhagicfever.png', action: APL_ACTION.DRAIN_HOPE, category: 'Channel', defaultRaw: 'true' },
  { id: 'bolt', spell: 'Shadow Bolt', icon: 'Spell_Shadow_ShadowBolt.png', action: APL_ACTION.SHADOW_BOLT_FILLER, category: 'Direct', defaultRaw: 'true' }
];

export const TOTAL_APL_RULES = 16; // Exactly 1 copy of each of the 16 actions (with 2 conditions per action)

const ACTION_MAP_BY_ID = new Map(APL_SYNTHESIS_ACTIONS.map(a => [a.id, a]));
const ACTION_MAP_BY_ACTION_ENUM = new Map(APL_SYNTHESIS_ACTIONS.map(a => [a.action, a]));

export const SPELL_IDS = {
  corruption: 1,
  'curse of agony': 2,
  'bane of agony': 2,
  'curse of doom': 22,
  'bane of doom': 22,
  immolate: 3,
  'siphon life': 15
};

// Full Condition Types Definition & Continuous Ranges
export const CONDITION_TYPES = {
  NEVER: { id: 'NEVER', name: 'Never Use', type: 'boolean', min: 0, max: 0, step: 0, unit: '' },
  ALWAYS: { id: 'ALWAYS', name: 'Always', type: 'boolean', min: 0, max: 0, step: 0, unit: '' },
  MANA_LE: { id: 'MANA_LE', name: 'Mana <= X%', type: 'continuous', min: 5, max: 90, step: 1, unit: '%', condEnum: APL_COND.MANA_LE },
  MANA_LT: { id: 'MANA_LT', name: 'Mana < X%', type: 'continuous', min: 5, max: 90, step: 1, unit: '%', condEnum: APL_COND.MANA_LT },
  MANA_GE: { id: 'MANA_GE', name: 'Mana >= X%', type: 'continuous', min: 10, max: 95, step: 1, unit: '%', condEnum: APL_COND.MANA_GE },
  TARGET_HP_LE: { id: 'TARGET_HP_LE', name: 'Target HP <= X%', type: 'continuous', min: 5, max: 50, step: 1, unit: '%', condEnum: APL_COND.TARGET_HP_LE },
  TARGET_HP_LT: { id: 'TARGET_HP_LT', name: 'Target HP < X%', type: 'continuous', min: 5, max: 50, step: 1, unit: '%', condEnum: APL_COND.TARGET_HP_LT },
  TARGET_HP_GE: { id: 'TARGET_HP_GE', name: 'Target HP >= X%', type: 'continuous', min: 20, max: 90, step: 1, unit: '%', condEnum: APL_COND.TARGET_HP_GE },
  DOT_REM_LE: { id: 'DOT_REM_LE', name: 'DoT Remains <= Xs', type: 'continuous', min: 0.0, max: 6.0, step: 0.1, unit: 's', condEnum: APL_COND.DOT_REM_LE },
  DOT_REM_LT: { id: 'DOT_REM_LT', name: 'DoT Remains < Xs', type: 'continuous', min: 0.0, max: 6.0, step: 0.1, unit: 's', condEnum: APL_COND.DOT_REM_LT },
  FIGHT_TIME_GE: { id: 'FIGHT_TIME_GE', name: 'Target TTDie >= Xs', type: 'continuous', min: 10, max: 120, step: 1, unit: 's', condEnum: APL_COND.FIGHT_TIME_GE },
  FIGHT_TIME_LE: { id: 'FIGHT_TIME_LE', name: 'Target TTDie <= Xs', type: 'continuous', min: 5, max: 60, step: 1, unit: 's', condEnum: APL_COND.FIGHT_TIME_LE },
  SHADOW_TRANCE: { id: 'SHADOW_TRANCE', name: 'Shadow Trance active', type: 'boolean', min: 0, max: 0, step: 0, unit: '', condEnum: APL_COND.SHADOW_TRANCE },
  DECIMATION_ACTIVE: { id: 'DECIMATION_ACTIVE', name: 'Decimation active (HP <= 35%)', type: 'continuous', min: 10, max: 35, step: 1, unit: '%', condEnum: APL_COND.DECIMATION_ACTIVE },
  DECIMATION_INACTIVE: { id: 'DECIMATION_INACTIVE', name: 'Decimation inactive (HP <= 35%)', type: 'continuous', min: 10, max: 35, step: 1, unit: '%', condEnum: APL_COND.DECIMATION_INACTIVE },
  DEMONIC_BRAND_MISSING: { id: 'DEMONIC_BRAND_MISSING', name: 'Brand missing', type: 'boolean', min: 0, max: 0, step: 0, unit: '', condEnum: APL_COND.DEMONIC_BRAND_MISSING },
  DOOM_MISSING: { id: 'DOOM_MISSING', name: 'Doom missing', type: 'boolean', min: 0, max: 0, step: 0, unit: '', condEnum: APL_COND.DOOM_MISSING },
  FIGHT_GE_DOT_MISSING: { id: 'FIGHT_GE_DOT_MISSING', name: 'Target TTDie >= Xs & DoT missing', type: 'continuous', min: 10, max: 90, step: 1, unit: 's', condEnum: APL_COND.FIGHT_GE_DOT_MISSING },
  FIGHT_GE_DOOM_AGONY_MISSING: { id: 'FIGHT_GE_DOOM_AGONY_MISSING', name: 'Target TTDie >= Xs & Curses missing', type: 'continuous', min: 15, max: 90, step: 1, unit: 's', condEnum: APL_COND.FIGHT_GE_DOOM_AGONY_MISSING },
  ISB_ACTIVE: { id: 'ISB_ACTIVE', name: 'ISB active', type: 'boolean', min: 0, max: 0, step: 0, unit: '', condEnum: APL_COND.ISB_ACTIVE }
};

// Authentic action condition sets (which conditions make sense for each action)
export const ACTION_VALID_CONDITIONS = {
  tap: ['MANA_LE', 'MANA_LT', 'MANA_GE', 'ALWAYS', 'NEVER'],
  nightfall: ['SHADOW_TRANCE', 'ALWAYS', 'NEVER'],
  brand: ['DEMONIC_BRAND_MISSING', 'ALWAYS', 'NEVER'],
  decimateSearing: ['DECIMATION_INACTIVE', 'TARGET_HP_LE', 'TARGET_HP_LT', 'ALWAYS', 'NEVER'],
  decimateSoulFire: ['DECIMATION_ACTIVE', 'TARGET_HP_LE', 'TARGET_HP_LT', 'ALWAYS', 'NEVER'],
  curse: ['FIGHT_TIME_GE', 'DOOM_MISSING', 'FIGHT_GE_DOT_MISSING', 'FIGHT_GE_DOOM_AGONY_MISSING', 'ALWAYS', 'NEVER'],
  agony: ['DOT_REM_LE', 'DOT_REM_LT', 'DOOM_MISSING', 'FIGHT_TIME_GE', 'FIGHT_GE_DOT_MISSING', 'FIGHT_GE_DOOM_AGONY_MISSING', 'ALWAYS', 'NEVER'],
  corr: ['DOT_REM_LE', 'DOT_REM_LT', 'FIGHT_TIME_GE', 'FIGHT_GE_DOT_MISSING', 'ALWAYS', 'NEVER'],
  immo: ['DOT_REM_LE', 'DOT_REM_LT', 'FIGHT_TIME_GE', 'FIGHT_GE_DOT_MISSING', 'ALWAYS', 'NEVER'],
  conflag: ['DOT_REM_LE', 'DOT_REM_LT', 'FIGHT_TIME_GE', 'ALWAYS', 'NEVER'],
  shadowburn: ['TARGET_HP_LE', 'TARGET_HP_LT', 'FIGHT_TIME_LE', 'MANA_GE', 'ALWAYS', 'NEVER'],
  incinerate: ['TARGET_HP_LE', 'TARGET_HP_GE', 'MANA_GE', 'ALWAYS', 'NEVER'],
  searing: ['TARGET_HP_LE', 'TARGET_HP_GE', 'MANA_GE', 'ALWAYS', 'NEVER'],
  siphon: ['DOT_REM_LE', 'DOT_REM_LT', 'FIGHT_TIME_GE', 'FIGHT_GE_DOT_MISSING', 'ALWAYS', 'NEVER'],
  wrack: ['TARGET_HP_LE', 'TARGET_HP_LT', 'FIGHT_TIME_GE', 'ALWAYS', 'NEVER'],
  bolt: ['TARGET_HP_LE', 'TARGET_HP_GE', 'MANA_GE', 'ALWAYS', 'NEVER']
};

class FastRNG {
  constructor(seed = 0x1337BEEF) {
    this.s0 = (seed & 0xFFFFFFFF) >>> 0;
    this.s1 = ((seed ^ 0x9E3779B9) & 0xFFFFFFFF) >>> 0;
    this.s2 = ((seed * 1664525 + 1013904223) & 0xFFFFFFFF) >>> 0;
    this.s3 = ((seed * 1103515245 + 12345) & 0xFFFFFFFF) >>> 0;
    for (let i = 0; i < 16; i++) this.nextU32();
  }

  nextU32() {
    const t = (this.s1 << 9) >>> 0;
    this.s2 ^= this.s0;
    this.s3 ^= this.s1;
    this.s1 ^= this.s2;
    this.s0 ^= this.s3;
    this.s2 ^= t;
    this.s3 = ((this.s3 << 11) | (this.s3 >>> 21)) >>> 0;
    return (this.s0 + this.s3) >>> 0;
  }

  nextDouble() {
    return (this.nextU32() >>> 8) * (1.0 / 16777216.0);
  }

  nextInt(min, max) {
    return min + Math.floor(this.nextDouble() * (max - min + 1));
  }

  nextFloat(min, max) {
    return min + this.nextDouble() * (max - min);
  }

  nextGaussian(mean = 0, stdev = 1) {
    const u1 = Math.max(1e-9, this.nextDouble());
    const u2 = this.nextDouble();
    const z = Math.sqrt(-2.0 * Math.log(u1)) * Math.cos(2.0 * Math.PI * u2);
    return mean + z * stdev;
  }
}

// Generate condition representation and bytecode fields from condition template & param
export function createRuleCondition(actionId, condKey, param = 0.0) {
  const cDef = CONDITION_TYPES[condKey] || CONDITION_TYPES.ALWAYS;
  const actInfo = ACTION_MAP_BY_ID.get(actionId) || APL_SYNTHESIS_ACTIONS[0];
  let targetSpell = 0;
  if (actionId === 'corr') targetSpell = 1;
  else if (actionId === 'agony') targetSpell = 2;
  else if (actionId === 'immo') targetSpell = 3;
  else if (actionId === 'conflag') targetSpell = 3;
  else if (actionId === 'curse') targetSpell = 22;
  else if (actionId === 'siphon') targetSpell = 15;

  let enabled = true;
  let condEnum = APL_COND.ALWAYS;
  let conditionText = 'Always';
  let rawCond = 'true';
  let cleanParam = Number(param) || 0.0;

  if (condKey === 'NEVER') {
    enabled = false;
    condEnum = APL_COND.ALWAYS;
    conditionText = 'Never Use';
    rawCond = 'false';
    cleanParam = 0.0;
  } else if (condKey === 'ALWAYS') {
    enabled = true;
    condEnum = APL_COND.ALWAYS;
    conditionText = 'Always';
    rawCond = 'true';
    cleanParam = 0.0;
  } else if (condKey === 'MANA_LE' || condKey === 'MANA_LT') {
    cleanParam = Math.round(Math.max(5, Math.min(95, cleanParam)));
    const op = condKey === 'MANA_LT' ? '<' : '<=';
    condEnum = condKey === 'MANA_LT' ? APL_COND.MANA_LT : APL_COND.MANA_LE;
    conditionText = `Mana ${op} ${cleanParam}%`;
    rawCond = `mana_pct ${op} ${cleanParam}`;
  } else if (condKey === 'MANA_GE') {
    cleanParam = Math.round(Math.max(10, Math.min(95, cleanParam)));
    condEnum = APL_COND.MANA_GE;
    conditionText = `Mana >= ${cleanParam}%`;
    rawCond = `mana_pct >= ${cleanParam}`;
  } else if (condKey === 'TARGET_HP_LE' || condKey === 'TARGET_HP_LT') {
    cleanParam = Math.round(Math.max(5, Math.min(50, cleanParam)));
    const op = condKey === 'TARGET_HP_LT' ? '<' : '<=';
    condEnum = condKey === 'TARGET_HP_LT' ? APL_COND.TARGET_HP_LT : APL_COND.TARGET_HP_LE;
    conditionText = `Target HP ${op} ${cleanParam}%`;
    rawCond = `target_hp_pct ${op} ${cleanParam}`;
  } else if (condKey === 'TARGET_HP_GE') {
    cleanParam = Math.round(Math.max(20, Math.min(95, cleanParam)));
    condEnum = APL_COND.TARGET_HP_GE;
    conditionText = `Target HP >= ${cleanParam}%`;
    rawCond = `target_hp_pct >= ${cleanParam}`;
  } else if (condKey === 'DOT_REM_LE' || condKey === 'DOT_REM_LT') {
    cleanParam = Number(Math.max(0.0, Math.min(6.0, cleanParam)).toFixed(1));
    const op = condKey === 'DOT_REM_LT' ? '<' : '<=';
    condEnum = condKey === 'DOT_REM_LT' ? APL_COND.DOT_REM_LT : APL_COND.DOT_REM_LE;
    conditionText = cleanParam === 0 ? 'DoT Expired' : `DoT Remains ${op} ${cleanParam}s`;
    rawCond = `target.debuff_remains("${actInfo.spell}") ${op} ${cleanParam}`;
  } else if (condKey === 'FIGHT_TIME_GE') {
    cleanParam = Math.round(Math.max(10, Math.min(180, cleanParam)));
    condEnum = APL_COND.FIGHT_TIME_GE;
    conditionText = `Target TTDie >= ${cleanParam}s`;
    rawCond = `target_ttd >= ${cleanParam}`;
  } else if (condKey === 'FIGHT_TIME_LE') {
    cleanParam = Math.round(Math.max(5, Math.min(60, cleanParam)));
    condEnum = APL_COND.FIGHT_TIME_LE;
    conditionText = `Target TTDie <= ${cleanParam}s`;
    rawCond = `target_ttd <= ${cleanParam}`;
  } else if (condKey === 'SHADOW_TRANCE') {
    condEnum = APL_COND.SHADOW_TRANCE;
    conditionText = 'Shadow Trance active';
    rawCond = 'buff.shadow_trance';
    cleanParam = 0.0;
  } else if (condKey === 'DECIMATION_ACTIVE') {
    cleanParam = Math.round(Math.max(10, Math.min(35, cleanParam || 35)));
    condEnum = APL_COND.DECIMATION_ACTIVE;
    conditionText = `Target HP <= ${cleanParam}%, buff active`;
    rawCond = 'decimation.active';
    targetSpell = 13;
  } else if (condKey === 'DECIMATION_INACTIVE') {
    cleanParam = Math.round(Math.max(10, Math.min(35, cleanParam || 35)));
    condEnum = APL_COND.DECIMATION_INACTIVE;
    conditionText = `Target HP <= ${cleanParam}%, buff inactive`;
    rawCond = 'decimation.inactive';
    targetSpell = 5;
  } else if (condKey === 'DEMONIC_BRAND_MISSING') {
    condEnum = APL_COND.DEMONIC_BRAND_MISSING;
    conditionText = 'Brand missing';
    rawCond = 'debuff.demonic_brand_missing';
    cleanParam = 0.0;
  } else if (condKey === 'DOOM_MISSING') {
    condEnum = APL_COND.DOOM_MISSING;
    conditionText = 'Doom missing';
    rawCond = '!target.has_debuff("Bane of Doom")';
    cleanParam = 0.0;
    targetSpell = 2;
  } else if (condKey === 'FIGHT_GE_DOT_MISSING') {
    cleanParam = Math.round(Math.max(10, Math.min(90, cleanParam)));
    condEnum = APL_COND.FIGHT_GE_DOT_MISSING;
    conditionText = `TTDie >= ${cleanParam}s & Missing`;
    rawCond = `target_ttd >= ${cleanParam} && !target.has_debuff("${actInfo.spell}")`;
  } else if (condKey === 'FIGHT_GE_DOOM_AGONY_MISSING') {
    cleanParam = Math.round(Math.max(15, Math.min(90, cleanParam)));
    condEnum = APL_COND.FIGHT_GE_DOOM_AGONY_MISSING;
    conditionText = `TTDie >= ${cleanParam}s & No Curses`;
    rawCond = `target_ttd >= ${cleanParam} && !target.has_debuff("Bane of Doom") && !target.has_debuff("Bane of Agony")`;
    targetSpell = 2;
  } else if (condKey === 'ISB_ACTIVE') {
    condEnum = APL_COND.ISB_ACTIVE;
    conditionText = 'ISB active';
    rawCond = 'buff.isb';
    cleanParam = 0.0;
  }

  return {
    condKey,
    enabled,
    cond: condEnum,
    param: cleanParam,
    targetSpell,
    condition: conditionText,
    rawCond
  };
}

// Create a single APL Rule with up to 2 distinct conditions evaluated together
export function createTwoConditionRule(actionId, condKey1 = 'ALWAYS', param1 = 0.0, condKey2 = 'ALWAYS', param2 = 0.0) {
  const actInfo = ACTION_MAP_BY_ID.get(actionId) || APL_SYNTHESIS_ACTIONS[0];
  const c1 = createRuleCondition(actionId, condKey1, param1);
  const c2 = createRuleCondition(actionId, condKey2, param2);

  const isNever = !c1.enabled || !c2.enabled;
  const enabled = !isNever;

  let combinedCondition = 'Always';
  let combinedRaw = 'true';

  if (isNever) {
    combinedCondition = 'Never Use';
    combinedRaw = 'false';
  } else if (c1.condKey === 'ALWAYS' && c2.condKey === 'ALWAYS') {
    combinedCondition = 'Always';
    combinedRaw = 'true';
  } else if (c2.condKey === 'ALWAYS') {
    combinedCondition = c1.condition;
    combinedRaw = c1.rawCond;
  } else if (c1.condKey === 'ALWAYS') {
    combinedCondition = c2.condition;
    combinedRaw = c2.rawCond;
  } else {
    combinedCondition = `${c1.condition} & ${c2.condition}`;
    combinedRaw = `${c1.rawCond} && ${c2.rawCond}`;
  }

  return {
    id: actInfo.id,
    spell: actInfo.spell,
    icon: actInfo.icon,
    action: actInfo.action,
    category: actInfo.category,
    condKey1: c1.condKey,
    param1: c1.param,
    cond1: c1.cond,
    targetSpell1: c1.targetSpell,
    condKey2: c2.condKey,
    param2: c2.param,
    cond2: c2.cond,
    targetSpell2: c2.targetSpell,
    cond: c1.cond,
    param: c1.param,
    targetSpell: c1.targetSpell,
    condition: combinedCondition,
    condition1: c1.condition,
    condition2: c2.condition,
    rawCond: combinedRaw,
    enabled
  };
}

export function createRule(actionId, condKey = 'ALWAYS', param = 0.0, condKey2 = 'ALWAYS', param2 = 0.0) {
  return createTwoConditionRule(actionId, condKey, param, condKey2, param2);
}

// Get available actions for a specific talent tree configuration
export function getAvailableActionsForSpec(talentFlags = {}) {
  const tf = talentFlags || {};
  return APL_SYNTHESIS_ACTIONS.filter(act => {
    switch (act.id) {
      case 'nightfall': return Boolean(tf.nightfall);
      case 'incinerate': return Boolean(tf.incinerate);
      case 'conflag': return Boolean(tf.conflagrate);
      case 'shadowburn': return Boolean(tf.shadowburn);
      case 'siphon': return Boolean(tf.siphonLife);
      case 'wrack': return Boolean(tf.wrack);
      case 'brand': return Boolean(tf.demonicBrand);
      case 'decimateSearing': return Boolean(tf.decimation);
      case 'decimateSoulFire': return Boolean(tf.decimation);
      default: return true; // Baseline spells: tap, curse, agony, corr, immo, searing, bolt
    }
  });
}

// Get valid condition keys for an action filtered by talent requirements
export function getValidConditionsForAction(actionId, talentFlags = {}) {
  const tf = talentFlags || {};
  let validConds = [...(ACTION_VALID_CONDITIONS[actionId] || ['ALWAYS', 'NEVER'])];
  if (!tf.nightfall) {
    validConds = validConds.filter(c => c !== 'SHADOW_TRANCE');
  }
  if (!tf.decimation) {
    validConds = validConds.filter(c => c !== 'DECIMATION_ACTIVE' && c !== 'DECIMATION_INACTIVE');
  }
  if (!tf.demonicBrand) {
    validConds = validConds.filter(c => c !== 'DEMONIC_BRAND_MISSING');
  }
  if (!validConds.length) validConds = ['ALWAYS', 'NEVER'];
  return validConds;
}

// Generate a random valid condition tuple for a given action
function getRandomConditionParamsForAction(actionId, rng, talentFlags = {}) {
  const validConds = getValidConditionsForAction(actionId, talentFlags);
  const condKey = validConds[rng.nextInt(0, validConds.length - 1)];
  const cDef = CONDITION_TYPES[condKey] || CONDITION_TYPES.ALWAYS;

  let param = 0.0;
  if (cDef.type === 'continuous') {
    if (condKey === 'DOT_REM_LE' || condKey === 'DOT_REM_LT') {
      param = rng.nextFloat(0.0, 3.5);
    } else if (condKey === 'MANA_LE' || condKey === 'MANA_LT') {
      param = rng.nextFloat(10.0, 40.0);
    } else if (condKey === 'MANA_GE') {
      param = rng.nextFloat(30.0, 80.0);
    } else if (condKey === 'TARGET_HP_LE' || condKey === 'TARGET_HP_LT') {
      param = rng.nextFloat(15.0, 35.0);
    } else if (condKey === 'FIGHT_TIME_GE') {
      param = actionId === 'curse' ? rng.nextFloat(50.0, 70.0) : rng.nextFloat(15.0, 30.0);
    } else if (condKey === 'FIGHT_TIME_LE') {
      param = rng.nextFloat(5.0, 25.0);
    } else {
      param = rng.nextFloat(cDef.min, cDef.max);
    }
  } else if (condKey === 'DECIMATION_ACTIVE' || condKey === 'DECIMATION_INACTIVE') {
    param = 35.0;
  }

  return { condKey, param };
}

function getRandomTwoConditionRule(actionId, rng, talentFlags = {}) {
  const c1 = getRandomConditionParamsForAction(actionId, rng, talentFlags);
  // 50% chance of having a second active condition clause
  const hasCond2 = rng.nextDouble() < 0.50;
  const c2 = hasCond2 ? getRandomConditionParamsForAction(actionId, rng, talentFlags) : { condKey: 'ALWAYS', param: 0.0 };
  return createTwoConditionRule(actionId, c1.condKey, c1.param, c2.condKey, c2.param);
}

// Handcrafted starting conditions for each action
export function getHandcraftedRuleForAction(actionId) {
  switch (actionId) {
    case 'curse': // Doom: 60s left in fight + no bane on target
      return createTwoConditionRule('curse', 'FIGHT_TIME_GE', 60.0, 'DOOM_MISSING', 0);
    case 'nightfall': // Nightfall: shadow trance active
      return createTwoConditionRule('nightfall', 'SHADOW_TRANCE', 0, 'ALWAYS', 0);
    case 'brand': // Demonic Brand Refresher: demonic brand buff missing on target
      return createTwoConditionRule('brand', 'DEMONIC_BRAND_MISSING', 0, 'ALWAYS', 0);
    case 'decimateSearing': // Decimation Searing Pain: target hp 35% + decimation buff missing
      return createTwoConditionRule('decimateSearing', 'TARGET_HP_LE', 35.0, 'DECIMATION_INACTIVE', 35.0);
    case 'decimateSoulFire': // Decimation Soul Fire: decimation buff active (don't worry about target hp)
      return createTwoConditionRule('decimateSoulFire', 'DECIMATION_ACTIVE', 35.0, 'ALWAYS', 0);
    case 'agony': // Bane of Agony: no bane on target
      return createTwoConditionRule('agony', 'DOOM_MISSING', 0, 'DOT_REM_LE', 0.0);
    case 'corr': // Corruption: time to die > 12s + corruption missing
      return createTwoConditionRule('corr', 'FIGHT_TIME_GE', 12.0, 'DOT_REM_LE', 0.0);
    case 'immo': // Immolate: ttd > 12s + immolate missing
      return createTwoConditionRule('immo', 'FIGHT_TIME_GE', 12.0, 'DOT_REM_LE', 0.0);
    case 'conflag': // Conflagrate: immolate remaining < 6s
      return createTwoConditionRule('conflag', 'DOT_REM_LT', 6.0, 'ALWAYS', 0);
    case 'shadowburn': // Shadowburn: on cooldown (always)
      return createTwoConditionRule('shadowburn', 'ALWAYS', 0, 'ALWAYS', 0);
    case 'incinerate': // Incinerate: always
      return createTwoConditionRule('incinerate', 'ALWAYS', 0, 'ALWAYS', 0);
    case 'searing': // Searing Pain: always
      return createTwoConditionRule('searing', 'ALWAYS', 0, 'ALWAYS', 0);
    case 'wrack': // Wrack: always
      return createTwoConditionRule('wrack', 'ALWAYS', 0, 'ALWAYS', 0);
    case 'siphon': // Siphon Life: missing siphon life + ttd > 12s
      return createTwoConditionRule('siphon', 'DOT_REM_LE', 0.0, 'FIGHT_TIME_GE', 12.0);
    case 'bolt': // Shadow Bolt: always
      return createTwoConditionRule('bolt', 'ALWAYS', 0, 'ALWAYS', 0);
    case 'tap': // Life Tap: mana <= 20%
      return createTwoConditionRule('tap', 'MANA_LE', 20.0, 'ALWAYS', 0);
    default:
      return createTwoConditionRule(actionId, 'ALWAYS', 0, 'ALWAYS', 0);
  }
}

// Generate a canonical default individual containing 1 of each available action
export function createDefaultIndividual(availableActions = APL_SYNTHESIS_ACTIONS, talentFlags = {}) {
  const availableIds = new Set(availableActions.map(a => a.id));
  const tf = talentFlags || {};

  const order = [
    'tap', 'nightfall', 'brand', 'decimateSearing', 'decimateSoulFire',
    'curse', 'agony', 'corr', 'immo', 'conflag', 'siphon', 'shadowburn', 'wrack'
  ];

  if (tf.incinerate) {
    order.push('incinerate', 'searing', 'bolt');
  } else {
    order.push('bolt', 'searing', 'incinerate');
  }

  const filtered = order.filter(id => availableIds.has(id));
  const rules = filtered.map(id => getHandcraftedRuleForAction(id));
  return {
    rules,
    fitness: 0,
    batch: null
  };
}

// Generate random legal APL individual with exactly 1 of each available action
export function createRandomAPLIndividual(rng, availableActions = APL_SYNTHESIS_ACTIONS, talentFlags = {}, lockConditions = true) {
  const actionIds = availableActions.map(a => a.id);

  for (let i = actionIds.length - 1; i > 0; i--) {
    const j = rng.nextInt(0, i);
    const temp = actionIds[i];
    actionIds[i] = actionIds[j];
    actionIds[j] = temp;
  }

  const rules = actionIds.map(id => lockConditions ? getHandcraftedRuleForAction(id) : getRandomTwoConditionRule(id, rng, talentFlags));
  return {
    rules,
    fitness: 0,
    batch: null
  };
}

// Validate and enforce that an individual has exactly 1 of each available action
export function repairAPLIndividual(ind, rng, availableActions = APL_SYNTHESIS_ACTIONS, talentFlags = {}, lockConditions = true) {
  const seen = new Set();
  const validRules = [];
  const allowedSet = new Set(availableActions.map(a => a.id));

  ind.rules.forEach(r => {
    if (allowedSet.has(r.id) && !seen.has(r.id)) {
      seen.add(r.id);
      validRules.push(lockConditions ? getHandcraftedRuleForAction(r.id) : r);
    }
  });

  availableActions.forEach(a => {
    if (!seen.has(a.id)) {
      validRules.push(lockConditions ? getHandcraftedRuleForAction(a.id) : getRandomTwoConditionRule(a.id, rng, talentFlags));
      seen.add(a.id);
    }
  });

  ind.rules = validRules.slice(0, availableActions.length);
}

// Crossover two parent APLs while preserving permutation of all available actions
export function crossoverAPLIndividuals(p1, p2, rng, availableActions = APL_SYNTHESIS_ACTIONS, talentFlags = {}, lockConditions = true) {
  const totalRules = p1.rules.length;
  if (totalRules <= 2) {
    const child = { rules: p1.rules.map(r => ({ ...r })), fitness: 0, batch: null };
    repairAPLIndividual(child, rng, availableActions, talentFlags, lockConditions);
    return child;
  }

  const childRules = new Array(totalRules);
  const inChild = new Set();

  const pt1 = rng.nextInt(0, totalRules - 2);
  const pt2 = rng.nextInt(pt1 + 1, totalRules - 1);

  for (let i = pt1; i <= pt2; i++) {
    const r = lockConditions ? getHandcraftedRuleForAction(p1.rules[i].id) : { ...p1.rules[i] };
    childRules[i] = r;
    inChild.add(r.id);
  }

  let p2Idx = 0;
  for (let i = 0; i < totalRules; i++) {
    if (i >= pt1 && i <= pt2) continue;

    while (p2Idx < p2.rules.length) {
      const candidate = p2.rules[p2Idx++];
      if (!inChild.has(candidate.id)) {
        childRules[i] = lockConditions ? getHandcraftedRuleForAction(candidate.id) : { ...candidate };
        inChild.add(candidate.id);
        break;
      }
    }
  }

  for (let i = 0; i < totalRules; i++) {
    if (!childRules[i]) {
      for (const a of availableActions) {
        if (!inChild.has(a.id)) {
          childRules[i] = lockConditions ? getHandcraftedRuleForAction(a.id) : getRandomTwoConditionRule(a.id, rng, talentFlags);
          inChild.add(a.id);
          break;
        }
      }
    }
  }

  const child = { rules: childRules, fitness: 0, batch: null };
  repairAPLIndividual(child, rng, availableActions, talentFlags, lockConditions);
  return child;
}

// Mutate an APL Individual with index swapping, dual condition mutation, and parameter jitter
export function mutateAPLIndividual(ind, rng, config = {}, talentFlags = {}) {
  const totalRules = ind.rules.length;
  if (totalRules < 2) return;

  const lockConditions = config.lockConditions ?? true;
  const swapRate = config.swapRate ?? 0.40;

  // 1. Swap Mutation (Action Index Reordering)
  if (rng.nextDouble() < swapRate) {
    const numSwaps = rng.nextInt(1, Math.min(3, totalRules - 1));
    for (let s = 0; s < numSwaps; s++) {
      const i = rng.nextInt(0, totalRules - 1);
      const j = rng.nextInt(0, totalRules - 1);
      if (i !== j) {
        const temp = ind.rules[i];
        ind.rules[i] = ind.rules[j];
        ind.rules[j] = temp;
      }
    }
  }

  // 2. Shift / Insertion Mutation
  if (rng.nextDouble() < 0.25) {
    const fromIdx = rng.nextInt(0, totalRules - 1);
    const toIdx = rng.nextInt(0, totalRules - 1);
    if (fromIdx !== toIdx) {
      const [removed] = ind.rules.splice(fromIdx, 1);
      ind.rules.splice(toIdx, 0, removed);
    }
  }

  // If conditions are locked to handcrafted definitions, do not mutate condition logic or parameters
  if (lockConditions) return;

  const condMutRate = config.condMutRate ?? 0.35;
  const jitterRate = config.jitterRate ?? 0.45;
  const toggleNeverRate = config.toggleNeverRate ?? 0.20;

  // 3. Condition Type Mutation (Condition 1 or Condition 2)
  if (rng.nextDouble() < condMutRate) {
    const mutateIdx = rng.nextInt(0, totalRules - 1);
    const rule = ind.rules[mutateIdx];
    const mutateCond2 = rng.nextDouble() < 0.50;

    if (mutateCond2) {
      const newC2 = getRandomConditionParamsForAction(rule.id, rng, talentFlags);
      ind.rules[mutateIdx] = createTwoConditionRule(rule.id, rule.condKey1 || 'ALWAYS', rule.param1 || 0, newC2.condKey, newC2.param);
    } else {
      const newC1 = getRandomConditionParamsForAction(rule.id, rng, talentFlags);
      ind.rules[mutateIdx] = createTwoConditionRule(rule.id, newC1.condKey, newC1.param, rule.condKey2 || 'ALWAYS', rule.param2 || 0);
    }
  }

  // 4. Continuous Parameter Gaussian Jitter (Param 1 or Param 2)
  if (rng.nextDouble() < jitterRate) {
    const jitterIdx = rng.nextInt(0, totalRules - 1);
    const rule = ind.rules[jitterIdx];
    const jitterSecond = rng.nextDouble() < 0.50;

    let p1 = rule.param1 ?? rule.param ?? 0.0;
    let p2 = rule.param2 ?? 0.0;

    if (!jitterSecond) {
      const cDef = CONDITION_TYPES[rule.condKey1 || 'ALWAYS'];
      if (cDef && cDef.type === 'continuous') {
        const span = cDef.max - cDef.min;
        p1 = Math.max(cDef.min, Math.min(cDef.max, p1 + rng.nextGaussian(0, span * 0.10)));
      }
    } else {
      const cDef = CONDITION_TYPES[rule.condKey2 || 'ALWAYS'];
      if (cDef && cDef.type === 'continuous') {
        const span = cDef.max - cDef.min;
        p2 = Math.max(cDef.min, Math.min(cDef.max, p2 + rng.nextGaussian(0, span * 0.10)));
      }
    }

    ind.rules[jitterIdx] = createTwoConditionRule(rule.id, rule.condKey1 || 'ALWAYS', p1, rule.condKey2 || 'ALWAYS', p2);
  }

  // 5. Toggle "Never Use"
  if (rng.nextDouble() < toggleNeverRate) {
    const toggleIdx = rng.nextInt(0, totalRules - 1);
    const rule = ind.rules[toggleIdx];
    if (rule.condKey1 === 'NEVER' || rule.condKey2 === 'NEVER' || !rule.enabled) {
      const validConds = getValidConditionsForAction(rule.id, talentFlags).filter(c => c !== 'NEVER');
      const newCondKey = validConds[rng.nextInt(0, validConds.length - 1)] || 'ALWAYS';
      ind.rules[toggleIdx] = createTwoConditionRule(rule.id, newCondKey, rule.param1 || 0, 'ALWAYS', 0);
    } else {
      ind.rules[toggleIdx] = createTwoConditionRule(rule.id, 'NEVER', 0, 'ALWAYS', 0);
    }
  }
}

// Convert an Individual APL to full Bytecode Rules array (with 2 conditions) for the GPU shader
export function individualToBytecodeRules(ind) {
  return ind.rules.map(r => ({
    action: r.action,
    cond1: r.cond1 !== undefined ? r.cond1 : r.cond,
    param1: Number(r.param1 !== undefined ? r.param1 : r.param || 0.0),
    targetSpell1: Number(r.targetSpell1 !== undefined ? r.targetSpell1 : r.targetSpell || 0),
    cond2: r.cond2 !== undefined ? r.cond2 : APL_COND.ALWAYS,
    param2: Number(r.param2 !== undefined ? r.param2 : 0.0),
    targetSpell2: Number(r.targetSpell2 !== undefined ? r.targetSpell2 : 0),
    cond: r.cond1 !== undefined ? r.cond1 : r.cond,
    param: Number(r.param1 !== undefined ? r.param1 : r.param || 0.0),
    targetSpell: Number(r.targetSpell1 !== undefined ? r.targetSpell1 : r.targetSpell || 0),
    enabled: r.enabled ? 1 : 0
  }));
}

// Convert Individual APL to a FightConfig ready for simulation
export function individualToConfig(ind, baseStatsConfig) {
  const aplRules = individualToBytecodeRules(ind);
  const base = { ...baseStatsConfig };

  // Set rotation to match the dominant/first damage nuke in this APL priority order
  const activeSpells = ind.rules.filter(r => r.enabled).map(r => r.id);
  const firstNuke = activeSpells.find(id => id === 'incinerate' || id === 'searing' || id === 'bolt' || id === 'wrack');
  let rot = baseStatsConfig.rotation || 'shadow';
  if (firstNuke === 'incinerate') rot = 'fire';
  else if (firstNuke === 'searing') rot = 'searing';
  else if (firstNuke === 'bolt') rot = 'shadow';

  return buildFightConfig({
    base,
    talent: baseStatsConfig.talentFlags || {},
    pet: baseStatsConfig.pet || 'imp',
    sac: baseStatsConfig.sac || 'none',
    actionIds: activeSpells,
    aplRules,
    rotation: rot
  });
}

// Generate unique hash key for deduplication and MAP-Elites
export function getAPLUniqueKey(ind) {
  return ind.rules.map(r => `${r.id}:${r.condKey1 || r.condKey || ''}:${r.condKey2 || ''}:${r.enabled ? Number(r.param1 !== undefined ? r.param1 : r.param || 0).toFixed(1) : 'OFF'}`).join('|');
}

// MAP-Elites Quality-Diversity Key for APLs
export function getAPLMapElitesKey(ind) {
  const active = ind.rules.filter(r => r.enabled);
  const activeIds = active.map(r => r.id);

  // Feature 1: Top priority action
  const topAction = activeIds[0] || 'none';

  // Feature 2: Top DoT / Curse applied first
  const firstDot = activeIds.find(id => id === 'curse' || id === 'agony' || id === 'corr' || id === 'immo' || id === 'siphon') || 'none';

  // Feature 3: Primary filler
  const firstNuke = activeIds.find(id => id === 'incinerate' || id === 'searing' || id === 'bolt' || id === 'wrack') || 'bolt';

  // Feature 4: Tap position relative to filler
  const tapIdx = activeIds.indexOf('tap');
  const nukeIdx = activeIds.indexOf(firstNuke);
  const tapPos = (tapIdx >= 0 && (nukeIdx < 0 || tapIdx < nukeIdx)) ? 'early' : 'late';

  return `${topAction}_${firstDot}_${firstNuke}_${tapPos}`;
}

// Extract top unique distinct APL candidates
export function extractTopUniqueCandidates(population, mapElitesGrid, maxCount = 15) {
  const mapList = Array.from(mapElitesGrid.values());
  const combined = [...population, ...mapList];
  combined.sort((a, b) => b.fitness - a.fitness);

  const seen = new Set();
  const unique = [];
  for (const ind of combined) {
    // Unique signature by full action priority sequence
    const sig = ind.rules.map(r => `${r.id}:${r.enabled ? 1 : 0}`).join('-');
    if (!seen.has(sig)) {
      seen.add(sig);
      unique.push(ind);
      if (unique.length >= maxCount) break;
    }
  }

  return unique.map((ind, i) => createCandidateAPLResult(ind, i + 1));
}

// Generate human-readable summary name for a synthesized APL
export function formatAPLName(ind) {
  const active = ind.rules.filter(r => r.enabled);
  const activeIds = active.map(r => r.id);

  const tags = [];
  if (activeIds.includes('curse')) tags.push('Doom');
  if (activeIds.includes('agony')) tags.push('Agony');
  if (activeIds.includes('corr')) tags.push('Corr');
  if (activeIds.includes('immo')) tags.push('Immo');
  if (activeIds.includes('conflag')) tags.push('Conflag');
  if (activeIds.includes('brand')) tags.push('Brand');
  if (activeIds.includes('decimateSoulFire') || activeIds.includes('decimateSearing')) tags.push('Deci');
  if (activeIds.includes('nightfall')) tags.push('Nightfall');
  if (activeIds.includes('siphon')) tags.push('Siphon');
  if (activeIds.includes('wrack')) tags.push('Wrack');
  if (activeIds.includes('shadowburn')) tags.push('Sburn');

  let primary = 'Shadow Bolt';
  const firstNuke = activeIds.find(id => id === 'incinerate' || id === 'searing' || id === 'bolt' || id === 'wrack');
  if (firstNuke === 'incinerate') primary = 'Incinerate';
  else if (firstNuke === 'searing') primary = 'Searing Pain';
  else if (firstNuke === 'wrack') primary = 'Wrack';

  const tagStr = tags.length > 0 ? tags.slice(0, 4).join('/') : 'Direct';
  return `${primary} (${tagStr} · ${active.length} Active Rules)`;
}

// Create elite candidate result payload
export function createCandidateAPLResult(ind, rank = 1) {
  const activeRules = ind.rules.filter(r => r.enabled);
  const name = formatAPLName(ind);
  const summary = ind.batch?.summary || { mean: ind.fitness || 0, stdDev: 0, min: 0, max: 0, confidence: 0 };

  return {
    rank,
    name,
    rules: ind.rules,
    activeRulesCount: activeRules.length,
    activeRules: activeRules.map(r => ({
      id: r.id,
      spell: r.spell,
      icon: r.icon,
      condition: r.condition,
      rawCond: r.rawCond,
      enabled: r.enabled
    })),
    meanDps: summary.mean || ind.fitness,
    stdDev: summary.stdDev || 0,
    confidence: summary.confidence || 0,
    summary,
    batch: ind.batch,
    aplRules: individualToBytecodeRules(ind)
  };
}

// Main Genetic Algorithm APL Synthesis Loop with WebGL Multi-Config Simulation & Double Buffering
export async function runAPLGeneticSynthesis(baseStatsConfig, gaConfig, { signal, onProgress = () => {}, onGeneration = () => {} } = {}) {
  const rng = new FastRNG(gaConfig.seed || 0x41504C53);
  const popSize = gaConfig.populationSize || 500;
  const generations = gaConfig.generations || 15;
  const screeningSims = gaConfig.screeningSims || 100;
  const finalSims = gaConfig.finalSims || 2000;

  const talentFlags = baseStatsConfig?.talentFlags || {};
  const availableActions = getAvailableActionsForSpec(talentFlags);

  const mapElitesGrid = new Map();
  const evolutionHistory = [];
  const uniqueAPLsSet = new Set();
  let population = [];

  const lockConditions = gaConfig.lockConditions ?? true;

  // 1. Seed with Default Canonical & Preset-derived APLs
  if (gaConfig.seedPresets) {
    const def = createDefaultIndividual(availableActions, talentFlags);
    uniqueAPLsSet.add(getAPLUniqueKey(def));
    population.push(def);
  }

  // 2. Fill remainder with randomized unique legal APL individuals
  while (population.length < popSize) {
    const ind = createRandomAPLIndividual(rng, availableActions, talentFlags, lockConditions);
    const key = getAPLUniqueKey(ind);
    if (!uniqueAPLsSet.has(key)) {
      uniqueAPLsSet.add(key);
      population.push(ind);
    }
  }

  onProgress({ phase: 'Evaluating Initial APL Generation (GPU Shader)', completed: 0, total: generations + 1 });

  let totalEvalsCount = population.length;
  let totalSimsCount = population.length * screeningSims;

  // Launch Generation 0 simulation on GPU
  const gen0Configs = population.map(ind => individualToConfig(ind, baseStatsConfig));
  let currentSimPromise = runMultiSimulation(gen0Configs, { signal, iterations: screeningSims });
  let currentPool = population;

  // Prepare Generation 1 offspring on CPU concurrently
  let nextBatch = null;
  if (generations >= 1) {
    nextBatch = prepareAPLOffspringBatch(population, popSize, rng, gaConfig, baseStatsConfig, uniqueAPLsSet, availableActions, talentFlags, mapElitesGrid);
  }

  // Await Gen 0 GPU results
  const simRes0 = await currentSimPromise;
  if (signal?.aborted) {
    return { candidates: [], evolutionHistory: [], bestCandidate: null, totalEvaluations: 0, totalSimulations: 0, uniqueConfigsCount: 0 };
  }

  for (let i = 0; i < currentPool.length; i++) {
    const res = simRes0.results[i];
    currentPool[i].fitness = res.summary.mean;
    currentPool[i].batch = res;
    const key = getAPLMapElitesKey(currentPool[i]);
    if (!mapElitesGrid.has(key) || currentPool[i].fitness > mapElitesGrid.get(key).fitness) {
      mapElitesGrid.set(key, { ...currentPool[i] });
    }
  }

  population.sort((a, b) => b.fitness - a.fitness);
  const gen0Best = population[0].fitness;
  const gen0Mean = population.reduce((s, ind) => s + ind.fitness, 0) / population.length;
  evolutionHistory.push({ gen: 0, bestDps: gen0Best, avgDps: gen0Mean });

  let elites = extractTopUniqueCandidates(population, mapElitesGrid, 15);
  onGeneration({
    gen: 0,
    maxGens: generations,
    bestDps: gen0Best,
    avgDps: gen0Mean,
    elites,
    progress: 0,
    status: `Gen 0/${generations} [Best: ${gen0Best.toFixed(1)} DPS]`,
    evolutionHistory,
    uniqueConfigsCount: uniqueAPLsSet.size,
    totalEvaluations: totalEvalsCount,
    totalSimulations: totalSimsCount
  });

  // Generational Evolution Loop with Double Buffering
  for (let gen = 1; gen <= generations; gen++) {
    if (signal?.aborted) break;

    const activeBatch = nextBatch;
    totalEvalsCount += activeBatch.pool.length;
    totalSimsCount += activeBatch.pool.length * screeningSims;

    // Launch GPU simulation for activeBatch
    currentSimPromise = runMultiSimulation(activeBatch.configs, { signal, iterations: screeningSims });

    // Concurrently prepare next generation batch on CPU
    if (gen < generations) {
      nextBatch = prepareAPLOffspringBatch(population, popSize, rng, gaConfig, baseStatsConfig, uniqueAPLsSet, availableActions, talentFlags, mapElitesGrid);
    } else {
      nextBatch = null;
    }

    const offSimRes = await currentSimPromise;
    if (signal?.aborted) break;

    for (let i = 0; i < activeBatch.pool.length; i++) {
      const res = offSimRes.results[i];
      activeBatch.pool[i].fitness = res.summary.mean;
      activeBatch.pool[i].batch = res;
      const key = getAPLMapElitesKey(activeBatch.pool[i]);
      if (!mapElitesGrid.has(key) || activeBatch.pool[i].fitness > mapElitesGrid.get(key).fitness) {
        mapElitesGrid.set(key, { ...activeBatch.pool[i] });
      }
    }

    // Population update & elitism
    const popMap = new Map();
    for (const ind of population.slice(0, Math.min(20, population.length))) {
      popMap.set(getAPLUniqueKey(ind), ind);
    }
    for (const ind of activeBatch.pool) {
      const k = getAPLUniqueKey(ind);
      if (!popMap.has(k) || ind.fitness > popMap.get(k).fitness) {
        popMap.set(k, ind);
      }
    }
    population = Array.from(popMap.values())
      .sort((a, b) => b.fitness - a.fitness)
      .slice(0, popSize);

    const bestDps = population[0]?.fitness || 0;
    const meanDps = population.reduce((s, ind) => s + ind.fitness, 0) / population.length;
    evolutionHistory.push({ gen, bestDps, avgDps: meanDps });

    elites = extractTopUniqueCandidates(population, mapElitesGrid, 15);
    const curProg = gen / generations;
    const status = `Gen ${gen}/${generations} [Best: ${bestDps.toFixed(1)} DPS]`;

    onProgress({ phase: `APL Evolution Gen ${gen}/${generations}`, completed: gen, total: generations + 1 });
    onGeneration({
      gen,
      maxGens: generations,
      bestDps,
      avgDps: meanDps,
      elites,
      progress: curProg,
      status,
      evolutionHistory,
      uniqueConfigsCount: uniqueAPLsSet.size,
      totalEvaluations: totalEvalsCount,
      totalSimulations: totalSimsCount
    });

    await new Promise(resolve => setTimeout(resolve, 0));
  }

  // Final High-Precision Benchmarking of Discovered Elite APL Champions
  onProgress({ phase: 'Finalizing Elite APL Champions (High Precision GPU Sim)', completed: generations, total: generations + 1 });

  const finalCandidates = extractTopUniqueCandidates(population, mapElitesGrid, 15);

  if (finalCandidates.length > 0) {
    const finalConfigs = finalCandidates.map(c => individualToConfig(c, baseStatsConfig));
    const finalSimRes = await runMultiSimulation(finalConfigs, { signal, iterations: finalSims });

    for (let i = 0; i < finalCandidates.length; i++) {
      finalCandidates[i].batch = finalSimRes.results[i];
      finalCandidates[i].meanDps = finalSimRes.results[i].summary.mean;
      finalCandidates[i].summary = finalSimRes.results[i].summary;
    }
  }

  finalCandidates.sort((a, b) => b.meanDps - a.meanDps);
  finalCandidates.forEach((c, i) => { c.rank = i + 1; });
  const totalSimulations = totalSimsCount + (finalCandidates.length * finalSims);

  return {
    candidates: finalCandidates,
    evolutionHistory,
    bestCandidate: finalCandidates[0] || null,
    totalEvaluations: totalEvalsCount,
    totalSimulations,
    uniqueConfigsCount: uniqueAPLsSet.size,
    availableActionsCount: availableActions.length
  };
}

function prepareAPLOffspringBatch(population, popSize, rng, gaConfig, baseStatsConfig, uniqueAPLsSet, availableActions = APL_SYNTHESIS_ACTIONS, talentFlags = {}, mapElitesGrid = null) {
  const numElites = Math.min(20, Math.max(5, Math.floor(popSize * 0.05)));
  const numImmigrants = Math.max(5, Math.floor(popSize * 0.10));
  const numOffspring = Math.max(2, popSize - numElites - numImmigrants);
  const lockConditions = gaConfig.lockConditions ?? true;
  const pool = [];

  // 1. Elitism: preserve top individuals directly
  for (let i = 0; i < numElites; i++) {
    if (i < population.length) {
      pool.push({
        rules: population[i].rules.map(r => ({ ...r })),
        fitness: population[i].fitness,
        batch: population[i].batch
      });
    }
  }

  // Create parent pool from top 40% of population + all map elites
  const topPop = population.slice(0, Math.max(10, Math.floor(population.length * 0.40)));
  const mapElites = mapElitesGrid ? Array.from(mapElitesGrid.values()) : [];
  const parentPool = [...topPop, ...mapElites];

  const selectParent = () => {
    let best = parentPool[rng.nextInt(0, parentPool.length - 1)];
    for (let t = 0; t < 3; t++) {
      const candidate = parentPool[rng.nextInt(0, parentPool.length - 1)];
      if (candidate && candidate.fitness > best.fitness) {
        best = candidate;
      }
    }
    return best;
  };

  // 2. Tournament Crossover & Mutation Offspring
  for (let i = 0; i < numOffspring; i++) {
    const p1 = selectParent();
    const p2 = selectParent();
    const child = crossoverAPLIndividuals(p1, p2, rng, availableActions, talentFlags, lockConditions);
    mutateAPLIndividual(child, rng, gaConfig, talentFlags);
    const key = getAPLUniqueKey(child);
    if (!uniqueAPLsSet.has(key)) {
      uniqueAPLsSet.add(key);
      pool.push(child);
    } else {
      // Re-mutate on collision
      mutateAPLIndividual(child, rng, gaConfig, talentFlags);
      pool.push(child);
    }
  }

  // 3. Immigrants for diversity
  for (let imm = 0; imm < numImmigrants; imm++) {
    const randomInd = createRandomAPLIndividual(rng, availableActions, talentFlags, lockConditions);
    const key = getAPLUniqueKey(randomInd);
    uniqueAPLsSet.add(key);
    pool.push(randomInd);
  }

  const configs = pool.map(ind => individualToConfig(ind, baseStatsConfig));
  return { pool, configs };
}
