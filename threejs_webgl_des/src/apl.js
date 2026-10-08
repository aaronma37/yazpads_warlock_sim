import { isAPLRuleEnabled, isAPLRuleBlocking, getAPLUnreachableFlags } from './apl_rules.js';
import { fallbackRow } from './apl_fallback_view.js';
// Authentic Action Priority List (APL) Engine & Interactive Manager
import { APL_ACTION, APL_COND } from './model.js';

export const ACTION_INFO = {
  berserking: { id: 'berserking', spell: 'Berserking', icon: 'Racial_Troll_Berserk.png', action: APL_ACTION.BERSERKING, defaultCond: 'true' },
  bloodFury: { id: 'bloodFury', spell: 'Blood Fury', icon: 'Racial_Orc_BerserkerStrength.png', action: APL_ACTION.BLOOD_FURY, defaultCond: 'true' },
  eureka: { id: 'eureka', spell: 'Eureka!', icon: 'Spell_Nature_WispSplode.png', action: APL_ACTION.EUREKA, defaultCond: 'true' },
  tap: { id: 'tap', spell: 'Life Tap', icon: 'Spell_Shadow_BurningSpirit.png', action: APL_ACTION.LIFE_TAP, defaultCond: 'mana_pct <= 25' },
  nightfall: { id: 'nightfall', spell: 'Nightfall: Shadow Bolt', icon: 'Spell_Shadow_Twilight.png', action: APL_ACTION.NIGHTFALL_SHADOW_BOLT, defaultCond: 'buff.shadow_trance' },
  brand: { id: 'brand', spell: 'Demonic Brand', icon: 'ability_demonhunter_chaoticimprint_fire.png', action: APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, defaultCond: 'debuff.demonic_brand_missing' },
  decimateSearing: { id: 'decimateSearing', spell: 'Decimation: Searing Pain', icon: 'Spell_Fire_SoulBurn.png', action: APL_ACTION.DECIMATION_SEARING_PAIN, defaultCond: 'decimation.inactive' },
  decimateSoulFire: { id: 'decimateSoulFire', spell: 'Decimation: Soul Fire', icon: 'Spell_Fire_Fireball.png', action: APL_ACTION.DECIMATION_SOUL_FIRE, defaultCond: 'decimation.active' },
  curse: { id: 'curse', spell: 'Bane of Doom', icon: 'Spell_Shadow_AuraOfDarkness.png', action: APL_ACTION.CURSE_OF_DOOM, defaultCond: 'target_ttd >= 60' },
  agony: { id: 'agony', spell: 'Bane of Agony', icon: 'Spell_Shadow_CurseOfSargeras.png', action: APL_ACTION.CURSE_OF_AGONY, defaultCond: 'target.debuff_remains("Bane of Agony") <= 0' },
  corr: { id: 'corr', spell: 'Corruption', icon: 'Spell_Shadow_AbominationExplosion.png', action: APL_ACTION.CORRUPTION, defaultCond: 'target.debuff_remains("Corruption") <= 0' },
  immo: { id: 'immo', spell: 'Immolate', icon: 'Spell_Fire_Immolation.png', action: APL_ACTION.IMMOLATE, defaultCond: 'target.debuff_remains("Immolate") <= 0' },
  conflag: { id: 'conflag', spell: 'Conflagrate', icon: 'Spell_Fire_Fireball.png', action: APL_ACTION.CONFLAGRATE, defaultCond: 'true' },
  shadowburn: { id: 'shadowburn', spell: 'Shadowburn', icon: 'Spell_Shadow_ScourgeBuild.png', action: APL_ACTION.SHADOWBURN, defaultCond: 'true' },
  incinerate: { id: 'incinerate', spell: 'Incinerate', icon: 'Spell_Fire_Burnout.png', action: APL_ACTION.INCINERATE_FILLER, defaultCond: 'true' },
  searing: { id: 'searing', spell: 'Searing Pain', icon: 'Spell_Fire_SoulBurn.png', action: APL_ACTION.SEARING_PAIN_FILLER, defaultCond: 'true' },
  drain: { id: 'drain', spell: 'Drain Soul', icon: 'ability_deathknight_hemorrhagicfever.png', action: APL_ACTION.DRAIN_SOUL_FILLER, defaultCond: 'true' },
  siphon: { id: 'siphon', spell: 'Siphon Life', icon: 'Spell_Shadow_Requiem.png', action: APL_ACTION.SIPHON_LIFE, defaultCond: 'target.debuff_remains("Siphon Life") <= 0' },
  wrack: { id: 'wrack', spell: 'Wrack', icon: 'ability_deathknight_hemorrhagicfever.png', action: APL_ACTION.DRAIN_HOPE, defaultCond: 'true' },
  hellfire: { id: 'hellfire', spell: 'Hellfire', icon: 'Spell_Fire_Incinerate.png', action: APL_ACTION.HELLFIRE, defaultCond: 'true' },
  bolt: { id: 'bolt', spell: 'Shadow Bolt', icon: 'Spell_Shadow_ShadowBolt.png', action: APL_ACTION.SHADOW_BOLT_FILLER, defaultCond: 'true' },
};

export const CANONICAL_ACTION_NAMES = {
  berserking: 'berserking',
  bloodFury: 'blood_fury',
  eureka: 'eureka',
  curse: 'bane_of_doom',
  agony: 'bane_of_agony',
  bolt: 'shadow_bolt',
  searing: 'searing_pain',
  tap: 'life_tap',
  corr: 'corruption',
  immo: 'immolate',
  conflag: 'conflagrate',
  shadowburn: 'shadowburn',
  incinerate: 'incinerate',
  nightfall: 'nightfall',
  drain: 'drain_soul',
  siphon: 'siphon_life',
  wrack: 'wrack',
  hellfire: 'hellfire',
  brand: 'demonic_brand',
  decimateSearing: 'decimate_searing',
  decimateSoulFire: 'decimate_soul_fire',
};

export const ACTION_ALIASES = {
  berserking: 'berserking', beserking: 'berserking',
  blood_fury: 'bloodFury', bloodfury: 'bloodFury', 'blood fury': 'bloodFury',
  eureka: 'eureka', 'eureka!': 'eureka',
  // Shadow Bolt
  shadow_bolt: 'bolt', shadowbolt: 'bolt', 'shadow bolt': 'bolt', bolt: 'bolt',
  // Life Tap
  life_tap: 'tap', lifetap: 'tap', 'life tap': 'tap', tap: 'tap',
  // Bane of Doom
  bane_of_doom: 'curse', 'bane of doom': 'curse', curse_of_doom: 'curse', 'curse of doom': 'curse', doom: 'curse', curse: 'curse',
  // Bane of Agony
  bane_of_agony: 'agony', 'bane of agony': 'agony', curse_of_agony: 'agony', 'curse of agony': 'agony', agony: 'agony',
  // Searing Pain
  searing_pain: 'searing', 'searing pain': 'searing', searing: 'searing',
  // Corruption
  corruption: 'corr', corr: 'corr',
  // Immolate
  immolate: 'immo', immo: 'immo',
  // Conflagrate
  conflagrate: 'conflag', conflag: 'conflag',
  // Shadowburn
  shadowburn: 'shadowburn',
  // Incinerate
  incinerate: 'incinerate',
  // Nightfall
  nightfall: 'nightfall', 'nightfall: shadow bolt': 'nightfall', nightfall_bolt: 'nightfall', 'nightfall shadow bolt': 'nightfall', 'nightfall shadowbolt': 'nightfall',
  // Siphon Life
  siphon_life: 'siphon', 'siphon life': 'siphon', siphon: 'siphon',
  // Wrack
  wrack: 'wrack',
  // Drain Soul
  drain_soul: 'drain', 'drain soul': 'drain', drain: 'drain',
  // Demonic Brand
  demonic_brand: 'brand', 'demonic brand': 'brand', brand: 'brand', demonic_brand_refresher: 'brand', 'demonic brand refresher': 'brand',
  // Decimation Searing Pain
  decimate_searing: 'decimateSearing', 'decimate searing': 'decimateSearing', decimate_searing_pain: 'decimateSearing', 'decimation: searing pain': 'decimateSearing', 'decimation searing pain': 'decimateSearing', decimatesearing: 'decimateSearing',
  // Decimation Soul Fire
  decimate_soul_fire: 'decimateSoulFire', 'decimate soul fire': 'decimateSoulFire', decimate_soulfire: 'decimateSoulFire', 'decimation: soul fire': 'decimateSoulFire', 'decimation soul fire': 'decimateSoulFire', decimatesoulfire: 'decimateSoulFire',
  // Hellfire
  hellfire: 'hellfire',
};

export const DEFAULT_APL = [
  { ...ACTION_INFO.berserking, condition: 'On cooldown (Troll only)', rawCond: 'true', enabled: true },
  { ...ACTION_INFO.bloodFury, condition: 'On cooldown (Orc only)', rawCond: 'true', enabled: true },
  { ...ACTION_INFO.eureka, condition: 'On cooldown (Gnome only)', rawCond: 'true', enabled: true },
  { id: 'tap', spell: 'Life Tap', icon: 'Spell_Shadow_BurningSpirit.png', condition: 'Mana < 20%', rawCond: 'mana_pct < 20', enabled: true },
  { id: 'nightfall', spell: 'Nightfall: Shadow Bolt', icon: 'Spell_Shadow_Twilight.png', condition: 'Shadow Trance active', rawCond: 'buff.shadow_trance', enabled: true },
  { id: 'brand', spell: 'Demonic Brand Refresher', icon: 'ability_demonhunter_chaoticimprint_fire.png', condition: 'Brand missing', rawCond: 'debuff.demonic_brand_missing', enabled: false },
  { id: 'decimateSearing', spell: 'Decimation: Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Target HP < 35%, buff inactive', rawCond: 'decimation.inactive', enabled: false },
  { id: 'decimateSoulFire', spell: 'Decimation: Soul Fire', icon: 'Spell_Fire_Fireball.png', condition: 'Target HP < 35%, buff active', rawCond: 'decimation.active', enabled: false },
  { id: 'curse', spell: 'Bane of Doom', icon: 'Spell_Shadow_AuraOfDarkness.png', condition: 'Target TTDie >= 60s', rawCond: 'target_ttd >= 60 && !target.has_debuff("Bane of Doom")', enabled: true },
  { id: 'agony', spell: 'Bane of Agony', icon: 'Spell_Shadow_CurseOfSargeras.png', condition: 'Target TTDie >= 20s & No Doom', rawCond: 'target_ttd >= 20 && !target.has_debuff("Bane of Doom") && !target.has_debuff("Bane of Agony")', enabled: true },
  { id: 'corr', spell: 'Corruption', icon: 'Spell_Shadow_AbominationExplosion.png', condition: 'Target TTDie >= 12s & !Active', rawCond: 'target_ttd >= 12 && !target.has_debuff("Corruption")', enabled: true },
  { id: 'immo', spell: 'Immolate', icon: 'Spell_Fire_Immolation.png', condition: 'Target TTDie >= 15s & !Active', rawCond: 'target_ttd >= 15 && !target.has_debuff("Immolate")', enabled: false },
  { id: 'conflag', spell: 'Conflagrate', icon: 'Spell_Fire_Fireball.png', condition: 'Immolate Remaining < 6s', rawCond: 'target.debuff_remains("Immolate") < 6', enabled: false },
  { id: 'shadowburn', spell: 'Shadowburn', icon: 'Spell_Shadow_ScourgeBuild.png', condition: 'Always when available', rawCond: 'true', enabled: false },
  { id: 'incinerate', spell: 'Incinerate', icon: 'Spell_Fire_Burnout.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'searing', spell: 'Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'drain', spell: 'Wrack', icon: 'ability_deathknight_hemorrhagicfever.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'siphon', spell: 'Siphon Life', icon: 'Spell_Shadow_Requiem.png', condition: 'Missing Siphon Life', rawCond: 'target.debuff_remains("Siphon Life") <= 0', enabled: false },
  { id: 'wrack', spell: 'Wrack', icon: 'ability_deathknight_hemorrhagicfever.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'hellfire', spell: 'Hellfire', icon: 'Spell_Fire_Incinerate.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'bolt', spell: 'Shadow Bolt', icon: 'Spell_Shadow_ShadowBolt.png', condition: 'Always', rawCond: 'true', enabled: true }
];

let currentAPL = JSON.parse(JSON.stringify(DEFAULT_APL));
let editingIndex = -1;
let dragSourceIndex = -1;
let onChangeCallback = null;
let fallbackRotationGetter = () => 'shadow';

export function refreshAPLFallback() {
  const tbody = document.getElementById('apl-table-body');
  if (tbody) {
    tbody.querySelector('.apl-fallback-row')?.remove();
    const hasUnconditionalFiller = currentAPL.some(r => isAPLRuleEnabled(r) && isAPLRuleBlocking(r));
    tbody.insertAdjacentHTML('beforeend', fallbackRow(fallbackRotationGetter(), 5, hasUnconditionalFiller));
  }
}

export function applySynthesizedAPL(rules) {
  if (!Array.isArray(rules)) return;
  currentAPL = rules.map(r => {
    const isEnabled = isAPLRuleEnabled(r);

    return {
      id: r.id,
      spell: r.spell,
      icon: ['eureka', 'bloodFury', 'berserking'].includes(r.id) ? ACTION_INFO[r.id].icon : r.icon,
      condition: r.condition,
      condition1: r.condition1,
      condKey: r.condKey,
      condKey1: r.condKey1,
      condKey2: r.condKey2,
      condition2: r.condition2,
      rawCond: r.rawCond || (isEnabled ? 'true' : 'false'),
      enabled: isEnabled,
      action: r.action,
      cond: r.cond1 !== undefined ? r.cond1 : r.cond,
      param: r.param1 !== undefined ? r.param1 : r.param,
      targetSpell: r.targetSpell1 !== undefined ? r.targetSpell1 : r.targetSpell,
      cond1: r.cond1,
      param1: r.param1,
      targetSpell1: r.targetSpell1,
      cond2: r.cond2,
      param2: r.param2,
      targetSpell2: r.targetSpell2
    };
  });
  renderAPLUI();
  if (onChangeCallback) onChangeCallback(currentAPL);
}

export function normalizeConditionForAction(rawInput, actionId) {
  let raw = String(rawInput ?? '').trim();
  if (!raw || raw === 'true' || raw === 'always' || raw === '1') return 'true';
  if (raw === 'false' || raw === 'never' || raw === '0') return 'false';

  // Handle boolean and (case insensitive)
  if (/\s+(?:and|&&)\s+/i.test(raw)) {
    const parts = raw.split(/\s+(?:and|&&)\s+/i);
    return parts.map(p => normalizeConditionForAction(p, actionId)).join(' && ');
  }

  // Mana conditions: mana_remaining > 40%, mana_pct <= 25, mana < 20%
  let m = raw.match(/^(?:mana_remaining|mana_pct|mana|mana_percent)\s*(<=|<|>=|>|==)\s*(\d+(?:\.\d+)?)\s*%?$/i);
  if (m) {
    const op = m[1] === '==' ? '<=' : m[1];
    return `mana_pct ${op} ${m[2]}`;
  }

  // Target HP conditions: target_hp < 35%, target_hp_pct <= 35, hp < 35%
  m = raw.match(/^(?:target_hp|target_hp_pct|target_health|target_health_pct|hp)\s*(<=|<|>=|>|==)\s*(\d+(?:\.\d+)?)\s*%?$/i);
  if (m) {
    const op = m[1] === '==' ? '<=' : m[1];
    return `target_hp_pct ${op} ${m[2]}`;
  }

  // Fight time / TTD conditions: target_ttd >= 60s, ttd >= 60, target_ttdie >= 60
  m = raw.match(/^(?:target_ttd|target_ttdie|ttd|ttdie)\s*(<=|<|>=|>|==)\s*(\d+(?:\.\d+)?)\s*s?$/i);
  if (m) {
    const op = m[1] === '==' ? '>=' : m[1];
    return `target_ttd ${op} ${m[2]}`;
  }

  // Buff / Proc conditions
  if (/^(?:buff\.)?shadow_trance(?:\s*active)?$/i.test(raw) || /^shadow\s+trance\s+active$/i.test(raw)) {
    return 'buff.shadow_trance';
  }
  if (/^(?:buff\.)?isb(?:\s*active)?$/i.test(raw)) {
    return 'buff.isb';
  }

  // Decimation conditions
  if (/^decimation\.inactive$/i.test(raw) || /^decimation\s+inactive$/i.test(raw) || /^!buff\.decimation$/i.test(raw)) {
    return 'decimation.inactive';
  }
  if (/^decimation\.active$/i.test(raw) || /^decimation\s+active$/i.test(raw) || /^buff\.decimation$/i.test(raw)) {
    return 'decimation.active';
  }

  // Demonic Brand missing
  if (/^(?:debuff\.)?demonic_brand_missing$/i.test(raw) || /^brand\s+missing$/i.test(raw) || /^demonic\s+brand\s+missing$/i.test(raw)) {
    return 'debuff.demonic_brand_missing';
  }

  // DoT remains / expired
  m = raw.match(/^(?:target\.)?debuff_remains\(\s*["']([^"']+)["']\s*\)\s*(<=|<|>=|>|==)\s*(\d+(?:\.\d+)?)\s*s?$/i);
  if (m) {
    const op = m[2] === '==' ? '<=' : m[2];
    return `target.debuff_remains("${m[1]}") ${op} ${m[3]}`;
  }

  m = raw.match(/^(?:dot_remains|dot_remaining|debuff_remains)\(\s*["']([^"']+)["']\s*\)\s*(<=|<|>=|>|==)\s*(\d+(?:\.\d+)?)\s*s?$/i);
  if (m) {
    const op = m[2] === '==' ? '<=' : m[2];
    return `target.debuff_remains("${m[1]}") ${op} ${m[3]}`;
  }

  m = raw.match(/^(?:dot_remains|dot_remaining|dot_rem|remains|remaining)\s*(<=|<|>=|>|==)\s*(\d+(?:\.\d+)?)\s*s?$/i);
  if (m) {
    const op = m[1] === '==' ? '<=' : m[1];
    const spellName = actionId === 'immo' ? 'Immolate' : actionId === 'corr' ? 'Corruption' : actionId === 'agony' ? 'Bane of Agony' : actionId === 'siphon' ? 'Siphon Life' : 'Corruption';
    return `target.debuff_remains("${spellName}") ${op} ${m[2]}`;
  }

  if (/^dot\s+expired$/i.test(raw) || /^missing\s+dot$/i.test(raw) || /^dot\s+down$/i.test(raw)) {
    const spellName = actionId === 'immo' ? 'Immolate' : actionId === 'corr' ? 'Corruption' : actionId === 'agony' ? 'Bane of Agony' : actionId === 'siphon' ? 'Siphon Life' : 'Corruption';
    return `target.debuff_remains("${spellName}") <= 0`;
  }

  if (/^immolate\s*remaining\s*(<=|<)\s*(\d+(?:\.\d+)?)\s*s?$/i.test(raw) || /^immolate_remains\s*(<=|<)\s*(\d+(?:\.\d+)?)\s*s?$/i.test(raw)) {
    const matchRem = raw.match(/\d+(?:\.\d+)?/);
    return `target.debuff_remains("Immolate") < ${matchRem ? matchRem[0] : '6'}`;
  }

  return raw;
}

export function parseAPLLine(line, lineNum = 1) {
  const trimmed = line.trim();
  if (!trimmed || trimmed.startsWith('#') || trimmed.startsWith('//')) {
    return null;
  }

  let actionStr = '';
  let condStr = 'true';

  const ifMatch = trimmed.match(/^([^#]+?)\s+(?:if|when)\s+(.+)$/i);
  if (ifMatch) {
    actionStr = ifMatch[1].trim();
    condStr = ifMatch[2].trim();
  } else {
    actionStr = trimmed.replace(/\s+#.*$/, '').trim();
  }

  const normActionKey = actionStr.toLowerCase().replace(/[\s\-_:]+/g, ' ');
  const cleanKey = actionStr.toLowerCase().replace(/[\s\-_:]+/g, '');
  const mappedId = ACTION_ALIASES[normActionKey] || ACTION_ALIASES[cleanKey] || ACTION_ALIASES[actionStr.toLowerCase()];

  if (!mappedId || !ACTION_INFO[mappedId]) {
    throw new Error(`Line ${lineNum}: Unknown action '${actionStr}'. Supported actions: berserking, blood_fury, eureka, bolt, tap, nightfall, corr, immo, conflag, shadowburn, incinerate, searing, drain, siphon, wrack, hellfire, curse, agony, brand, decimateSearing, decimateSoulFire`);
  }

  const info = ACTION_INFO[mappedId];
  const normalizedCond = normalizeConditionForAction(condStr, mappedId);

  const entry = {
    id: mappedId,
    spell: info.spell,
    icon: info.icon,
    condition: condStr === 'true' ? 'Always' : condStr,
    rawCond: normalizedCond,
    enabled: normalizedCond !== 'false',
    action: info.action,
  };

  // Validate compilation immediately to catch any malformed condition
  try {
    compileAPLToBytecode([entry]);
  } catch (err) {
    throw new Error(`Line ${lineNum} (${info.spell}): ${err.message}`);
  }

  return entry;
}

export function parseAPLText(text) {
  if (typeof text !== 'string') return [];
  const lines = text.split('\n');
  const rules = [];

  for (let i = 0; i < lines.length; i++) {
    const entry = parseAPLLine(lines[i], i + 1);
    if (entry) {
      rules.push(entry);
    }
  }

  if (rules.length === 0) {
    throw new Error('APL must contain at least one valid action rule.');
  }

  return rules;
}

export function formatAPLEntry(entry) {
  const actionName = CANONICAL_ACTION_NAMES[entry.id] || entry.id;
  const raw = String(entry.rawCond || 'true').trim();
  if (!raw || raw === 'true' || raw === 'always' || entry.condition === 'Always') {
    return actionName;
  }

  let formatted = raw
    .replace(/^mana_pct\s*(<=|<|>=|>)\s*(\d+(?:\.\d+)?)$/, (_, op, val) => `mana_remaining ${op} ${val}%`)
    .replace(/^target_hp_pct\s*(<=|<|>=|>)\s*(\d+(?:\.\d+)?)$/, (_, op, val) => `target_hp ${op} ${val}%`)
    .replace(/^target_ttd\s*(>=|>)\s*(\d+(?:\.\d+)?)$/, (_, op, val) => `target_ttd ${op} ${val}s`)
    .replace(/^target\.debuff_remains\(\s*["']([^"']+)["']\s*\)\s*(<=|<)\s*0(?:\.0+)?$/, 'dot_remains <= 0')
    .replace(/^target\.debuff_remains\(\s*["']([^"']+)["']\s*\)\s*(<=|<)\s*(\d+(?:\.\d+)?)$/, (_, spell, op, val) => `dot_remains("${spell}") ${op} ${val}s`);

  return `${actionName} if ${formatted}`;
}

export function formatAPLToText(aplList = currentAPL) {
  return (aplList || [])
    .filter(rule => isAPLRuleEnabled(rule))
    .map(formatAPLEntry)
    .join('\n');
}

export function compileAPLToBytecode(aplList = currentAPL) {
  const mapAction = {
    berserking: APL_ACTION.BERSERKING,
    bloodFury: APL_ACTION.BLOOD_FURY,
    eureka: APL_ACTION.EUREKA,
    tap: APL_ACTION.LIFE_TAP,
    nightfall: APL_ACTION.NIGHTFALL_SHADOW_BOLT,
    brand: APL_ACTION.DEMONIC_BRAND_SEARING_PAIN,
    decimateSearing: APL_ACTION.DECIMATION_SEARING_PAIN,
    decimateSoulFire: APL_ACTION.DECIMATION_SOUL_FIRE,
    curse: APL_ACTION.CURSE_OF_DOOM,
    agony: APL_ACTION.CURSE_OF_AGONY,
    corr: APL_ACTION.CORRUPTION,
    immo: APL_ACTION.IMMOLATE,
    conflag: APL_ACTION.CONFLAGRATE,
    shadowburn: APL_ACTION.SHADOWBURN,
    incinerate: APL_ACTION.INCINERATE_FILLER,
    searing: APL_ACTION.SEARING_PAIN_FILLER,
    drain: APL_ACTION.DRAIN_SOUL_FILLER,
    siphon: APL_ACTION.SIPHON_LIFE,
    wrack: APL_ACTION.DRAIN_HOPE,
    hellfire: APL_ACTION.HELLFIRE,
    bolt: APL_ACTION.SHADOW_BOLT_FILLER,
  };

  const spellIds = {
    'corruption': 1, 'curse of agony': 2, 'bane of agony': 2, 'curse of doom': 22, 'bane of doom': 22,
    'immolate': 3, 'siphon life': 15,
  };
  const spellFromText = text => {
    const name = String(text).toLowerCase();
    const found = Object.keys(spellIds).find(spell => name.includes(spell));
    return found ? spellIds[found] : 0;
  };
  const parseCondition = (entry) => {
    const raw = String(entry.rawCond || 'true').trim().toLowerCase();
    const match = (pattern) => raw.match(pattern);
    if (entry.id === 'nightfall' && raw === 'buff.shadow_trance')
      return { cond: APL_COND.SHADOW_TRANCE, param: 0, targetSpell: 0 };
    if (entry.id === 'brand' && raw === 'debuff.demonic_brand_missing')
      return { cond: APL_COND.DEMONIC_BRAND_MISSING, param: 0, targetSpell: 0 };
    if (entry.id === 'decimateSearing' && raw === 'decimation.inactive')
      return { cond: APL_COND.DECIMATION_INACTIVE, param: 35, targetSpell: 5 };
    if (entry.id === 'decimateSoulFire' && raw === 'decimation.active')
      return { cond: APL_COND.DECIMATION_ACTIVE, param: 35, targetSpell: 13 };
    if (raw === 'buff.isb') return { cond: APL_COND.ISB_ACTIVE, param: 0, targetSpell: 0 };
    if (!raw || raw === 'true') return { cond: APL_COND.ALWAYS, param: 0, targetSpell: 0 };

    let m = match(/^mana_pct\s*(<=|<|>=|>)\s*(\d+(?:\.\d+)?)$/);
    if (m) return { cond: ({'<': APL_COND.MANA_LT, '<=': APL_COND.MANA_LE,
      '>': APL_COND.MANA_GT, '>=': APL_COND.MANA_GE})[m[1]],
      param: Number(m[2]), targetSpell: 0 };
    if (raw.match(/^mana_pct\s*<=\s*(\d+)$/))
      return { cond: APL_COND.MANA_LE, param: Number(raw.match(/^mana_pct\s*<=\s*(\d+)$/)[1]), targetSpell: 0 };
    m = match(/^target_hp_pct\s*(<=|<|>=|>)\s*(\d+(?:\.\d+)?)$/);
    if (m) return { cond: ({'<': APL_COND.TARGET_HP_LT, '<=': APL_COND.TARGET_HP_LE,
      '>': APL_COND.TARGET_HP_GT, '>=': APL_COND.TARGET_HP_GE})[m[1]],
      param: Number(m[2]), targetSpell: 0 };
    m = match(/^target_ttd\s*(>=|>)\s*(\d+(?:\.\d+)?)$/);
    if (m) return { cond: m[1] === '>' ? APL_COND.FIGHT_TIME_GT : APL_COND.FIGHT_TIME_GE, param: Number(m[2]), targetSpell: 0 };
    m = match(/^target_ttd\s*(<=|<)\s*(\d+(?:\.\d+)?)$/);
    if (m) return { cond: m[1] === '<' ? APL_COND.FIGHT_TIME_LT : APL_COND.FIGHT_TIME_LE, param: Number(m[2]), targetSpell: 0 };

    m = match(/^target\.debuff_remains\(\s*["']([^"']+)["']\s*\)\s*(<=|<)\s*(\d+(?:\.\d+)?)$/);
    if (m) {
      const targetSpell = spellFromText(m[1]);
      if (!targetSpell) throw new Error(`APL condition refers to unsupported debuff: ${m[1]}`);
      return { cond: m[2] === '<' ? APL_COND.DOT_REM_LT : APL_COND.DOT_REM_LE, param: Number(m[3]), targetSpell };
    }

    const hasNightfall = /talent\.nightfall/.test(raw);
    const ttd = raw.match(/target_ttd\s*>=?\s*(\d+(?:\.\d+)?)/);
    const missing = [...raw.matchAll(/!target\.has_debuff\(\s*["']([^"']+)["']\s*\)/g)]
      .map(x => spellFromText(x[1]));
    const onlySupportedAtoms = raw.split(/\s*&&\s*/).every(atom =>
      /^target_ttd\s*>=?\s*\d+(?:\.\d+)?$/.test(atom.trim()) ||
      /^!target\.has_debuff\(\s*["'][^"']+["']\s*\)$/.test(atom.trim()) ||
      /^talent\.nightfall$/.test(atom.trim()));
    if (onlySupportedAtoms && missing.length && missing.every(Boolean)) {
      if (hasNightfall && !ttd && missing.length === 1) {
        return { cond: APL_COND.NIGHTFALL_DOT_MISSING, param: 0, targetSpell: missing[0] };
      }
      if (ttd && missing.length === 1) {
        return { cond: APL_COND.FIGHT_GE_DOT_MISSING, param: Number(ttd[1]), targetSpell: missing[0] };
      }
      if (ttd && missing.length === 2 && (missing.includes(2) || missing.includes(22))) {
        return { cond: APL_COND.FIGHT_GE_DOOM_AGONY_MISSING, param: Number(ttd[1]), targetSpell: 2 };
      }
    }
    if (/^!target\.has_debuff\(\s*["'](?:curse of doom|bane of doom)["']\s*\)$/i.test(raw))
      return { cond: APL_COND.DOOM_MISSING, param: 0, targetSpell: 2 };
    m = match(/^!target\.has_debuff\(\s*["']([^"']+)["']\s*\)$/);
    if (m && spellFromText(m[1]))
      return { cond: APL_COND.FIGHT_GE_DOT_MISSING, param: 0, targetSpell: spellFromText(m[1]) };
    throw new Error(`APL condition is not supported by the shader: ${entry.rawCond}`);
  };

  return (aplList || currentAPL).map(entry => {
    if (entry.enabled && entry.id === 'drain')
      throw new Error('Drain Soul is not implemented by the WebGL shader yet.');
    const action = (entry.action !== undefined && entry.action !== null) ? entry.action : mapAction[entry.id];
    if (action === undefined) throw new Error(`Unsupported APL action: ${entry.id}`);
    
    const enabled = isAPLRuleEnabled(entry);
    let first = { cond: APL_COND.ALWAYS, param: 0, targetSpell: 0 };
    let second = { cond: APL_COND.ALWAYS, param: 0, targetSpell: 0 };
    if (entry.cond1 != null || entry.cond != null) {
      first = { cond: entry.cond1 ?? entry.cond, param: Number(entry.param1 ?? entry.param ?? 0),
        targetSpell: Number(entry.targetSpell1 ?? entry.targetSpell ?? 0) };
      second = { cond: entry.cond2 ?? APL_COND.ALWAYS, param: Number(entry.param2 ?? 0),
        targetSpell: Number(entry.targetSpell2 ?? 0) };
    } else if (enabled) {
      try {
        first = parseCondition(entry);
      } catch (error) {
        // Keep existing compound conditions, then try two shader conditions joined by &&.
        const parts = String(entry.rawCond || '').split(/\s*&&\s*/);
        if (parts.length !== 2) throw error;
        first = parseCondition({ ...entry, rawCond: parts[0] });
        second = parseCondition({ ...entry, rawCond: parts[1] });
      }
    }

    return {
      action, ...first,
      cond1: first.cond, param1: first.param, targetSpell1: first.targetSpell,
      cond2: second.cond, param2: second.param, targetSpell2: second.targetSpell,
      enabled: enabled ? 1 : 0
    };
  });
}

export function getActiveBytecodeRules() {
  return compileAPLToBytecode(currentAPL);
}

export function initAPL(onAPLChange, getFallbackRotation = () => 'shadow') {
  fallbackRotationGetter = getFallbackRotation;
  onChangeCallback = onAPLChange;
  renderAPLUI();
  setupEditBoxEvents();
  setupConditionModal();
  setupMultiDotToggle();
}

export function getActiveAPL() {
  return currentAPL;
}

function generateSpellAPLForPreset(presetName = '', talentsObj = null, rotationChoice = null, isSacSuccubus = false) {
  const name = (presetName || '').toLowerCase();
  const rot = (rotationChoice || '').toLowerCase();
  
  const hasTalent = (tree, tal) => {
    if (!talentsObj) return false;
    const treeObj = talentsObj[tree];
    if (!treeObj) return false;
    for (const [k, v] of Object.entries(treeObj)) {
      if (k.toLowerCase().replace(/[\s\'-]/g, '_') === tal.toLowerCase().replace(/[\s\'-]/g, '_') && v > 0) return true;
    }
    return false;
  };

  const isBrand = name.includes('brand') || hasTalent('demonology', 'demonic_brand');
  const isDecimate = name.includes('decimate') || hasTalent('demonology', 'decimation');
  const isIncinerate = rot === 'fire_destro' || rot === 'incinerate_decimation' || name.includes('incinerate') || (hasTalent('destruction', 'incinerate') && !name.includes('searing') && rot !== 'dp_af_fire' && !rot.includes('shadow') && !rot.includes('affliction'));
  const isSearing = rot === 'dp_af_fire' || rot === 'searing' || name.includes('searing') || name.includes('dp fire') || name.includes('dp_fire');
  const isFire = isIncinerate || isSearing || rot.includes('fire') || name.includes('fire');
  const hasConflag = hasTalent('destruction', 'conflagrate');
  const hasShadowburn = hasTalent('destruction', 'shadowburn');
  const hasSiphon = hasTalent('affliction', 'siphon_life');
  const hasWrack = hasTalent('affliction', 'wrack');
  const hasNightfall = hasTalent('affliction', 'nightfall') || (!isFire && !name.includes('pure shadow bolt'));
  const noCorruption = name.includes('no corruption') || name.includes('pure shadow bolt');
  const noBane = name.includes('no bane');

  if (isSearing && isDecimate) {
    return [
      { id: 'curse', spell: 'Bane of Doom', icon: 'Spell_Shadow_AuraOfDarkness.png', condition: 'Target TTDie >= 57s', rawCond: 'target_ttd >= 57', enabled: true },
      { id: 'agony', spell: 'Bane of Agony', icon: 'Spell_Shadow_CurseOfSargeras.png', condition: 'DoT Remains <= 2.5s', rawCond: 'target.debuff_remains("Bane of Agony") <= 2.5', enabled: true },
      { id: 'corr', spell: 'Corruption', icon: 'Spell_Shadow_AbominationExplosion.png', condition: 'DoT Remains <= 2.5s', rawCond: 'target.debuff_remains("Corruption") <= 2.5', enabled: true },
      { id: 'immo', spell: 'Immolate', icon: 'Spell_Fire_Immolation.png', condition: 'DoT Remains <= 2.5s', rawCond: 'target.debuff_remains("Immolate") <= 2.5', enabled: true },
      { id: 'brand', spell: 'Demonic Brand', icon: 'ability_demonhunter_chaoticimprint_fire.png', condition: 'Brand missing', rawCond: 'debuff.demonic_brand_missing', enabled: isBrand },
      { id: 'decimateSearing', spell: 'Decimation: Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Target HP <= 28%, buff inactive', rawCond: 'decimation.inactive', enabled: true },
      { id: 'tap', spell: 'Life Tap', icon: 'Spell_Shadow_BurningSpirit.png', condition: 'Mana <= 17%', rawCond: 'mana_pct <= 17', enabled: true },
      { id: 'decimateSoulFire', spell: 'Decimation: Soul Fire', icon: 'Spell_Fire_Fireball.png', condition: 'Target HP <= 28%, buff active', rawCond: 'decimation.active', enabled: true },
      { id: 'searing', spell: 'Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Filler (Fire)', rawCond: 'true', enabled: true }
    ];
  }

  const list = [];
  list.push({ id: 'tap', spell: 'Life Tap', icon: 'Spell_Shadow_BurningSpirit.png', condition: 'Mana <= 25%', rawCond: 'mana_pct <= 25', enabled: true });

  if (isDecimate) {
    if (isFire || name.includes('shadow and flame shadow - decimate')) {
      list.push({ id: 'decimateSearing', spell: 'Decimation: Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Target HP <= 35%, buff inactive', rawCond: 'decimation.inactive', enabled: true });
    }
    list.push({ id: 'decimateSoulFire', spell: 'Decimation: Soul Fire', icon: 'Spell_Fire_Fireball.png', condition: 'Target HP <= 35%, buff active', rawCond: 'decimation.active', enabled: true });
  }

  if (isBrand) {
    list.push({ id: 'brand', spell: 'Demonic Brand', icon: 'ability_demonhunter_chaoticimprint_fire.png', condition: 'Brand missing', rawCond: 'debuff.demonic_brand_missing', enabled: true });
  }

  const hasImmo = isFire || (hasConflag && hasTalent('destruction', 'shadow_and_flame'));
  if (hasImmo) {
    list.push({ id: 'immo', spell: 'Immolate', icon: 'Spell_Fire_Immolation.png', condition: 'DoT Expired', rawCond: 'target.debuff_remains("Immolate") <= 0', enabled: true });
  }
  if (hasConflag && hasImmo && !name.includes('dp/ruin fire')) {
    list.push({ id: 'conflag', spell: 'Conflagrate', icon: 'Spell_Fire_Fireball.png', condition: 'Always when available', rawCond: 'true', enabled: true });
  }

  if (hasNightfall) {
    list.push({ id: 'nightfall', spell: 'Nightfall: Shadow Bolt', icon: 'Spell_Shadow_Twilight.png', condition: 'Shadow Trance active', rawCond: 'buff.shadow_trance', enabled: true });
  }

  if (!noCorruption) {
    list.push({ id: 'corr', spell: 'Corruption', icon: 'Spell_Shadow_AbominationExplosion.png', condition: 'DoT Expired', rawCond: 'target.debuff_remains("Corruption") <= 0', enabled: true });
  }

  if (!noBane) {
    list.push({ id: 'curse', spell: 'Bane of Doom', icon: 'Spell_Shadow_AuraOfDarkness.png', condition: 'Target TTDie >= 60s', rawCond: 'target_ttd >= 60', enabled: true });
    list.push({ id: 'agony', spell: 'Bane of Agony', icon: 'Spell_Shadow_CurseOfSargeras.png', condition: 'DoT Expired', rawCond: 'target.debuff_remains("Bane of Agony") <= 0', enabled: true });
  }

  if (hasSiphon) {
    list.push({ id: 'siphon', spell: 'Siphon Life', icon: 'Spell_Shadow_Requiem.png', condition: 'DoT Remains <= 0s', rawCond: 'target.debuff_remains("Siphon Life") <= 0', enabled: true });
  }
  if (hasWrack) {
    list.push({ id: 'wrack', spell: 'Wrack', icon: 'ability_deathknight_hemorrhagicfever.png', condition: 'Always when available', rawCond: 'true', enabled: true });
  }

  if (hasShadowburn) {
    list.push({ id: 'shadowburn', spell: 'Shadowburn', icon: 'Spell_Shadow_ScourgeBuild.png', condition: 'Always when available', rawCond: 'true', enabled: true });
  }

  list.push({ id: 'hellfire', spell: 'Hellfire', icon: 'Spell_Fire_Incinerate.png', condition: 'Always', rawCond: 'true', enabled: false });

  if (isIncinerate) {
    list.push({ id: 'incinerate', spell: 'Incinerate', icon: 'Spell_Fire_Burnout.png', condition: 'Always', rawCond: 'true', enabled: true });
  } else if (isSearing) {
    list.push({ id: 'searing', spell: 'Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Always', rawCond: 'true', enabled: true });
  } else {
    list.push({ id: 'bolt', spell: 'Shadow Bolt', icon: 'Spell_Shadow_ShadowBolt.png', condition: 'Always', rawCond: 'true', enabled: true });
  }

  return list;
}

export function generateAPLForPreset(...args) {
  return [{ ...ACTION_INFO.berserking, condition: 'On cooldown (Troll only)', rawCond: 'true', enabled: true }, { ...ACTION_INFO.bloodFury, condition: 'On cooldown (Orc only)', rawCond: 'true', enabled: true }, { ...ACTION_INFO.eureka, condition: 'On cooldown (Gnome only)', rawCond: 'true', enabled: true }, ...generateSpellAPLForPreset(...args)];
}

export function setAPLPreset(presetName, talentsObj = null, rotationChoice = null, isSacSuccubus = false) {
  currentAPL = generateAPLForPreset(presetName, talentsObj, rotationChoice, isSacSuccubus);
  renderAPLUI();
  if (onChangeCallback) onChangeCallback(currentAPL);
}

function updateLineNumbers(box) {
  const gutter = box?.parentElement?.querySelector('.apl-line-numbers');
  if (!gutter) return;
  const count = box.value.split('\n').length;
  gutter.textContent = Array.from({ length: count }, (_, index) => index + 1).join('\n');
  gutter.style.height = `${box.offsetHeight}px`;
  gutter.scrollTop = box.scrollTop;
}

function setupEditBoxEvents() {
  const box = document.getElementById('apl-edit-box');
  const errorEl = document.getElementById('apl-text-error');
  if (!box) return;

  const handleInput = () => {
    updateLineNumbers(box);
    try {
      const parsed = parseAPLText(box.value);
      currentAPL = parsed;
      if (errorEl) {
        errorEl.textContent = '';
        errorEl.style.display = 'none';
      }
      box.classList.remove('input-invalid');
      renderAPLTable();
      if (onChangeCallback) onChangeCallback(currentAPL);
    } catch (err) {
      if (errorEl) {
        errorEl.textContent = err.message;
        errorEl.style.display = 'block';
      }
      box.classList.add('input-invalid');
    }
  };

  box.addEventListener('input', handleInput);
  box.addEventListener('change', handleInput);
  box.addEventListener('scroll', () => updateLineNumbers(box));
  if (typeof ResizeObserver !== 'undefined') {
    new ResizeObserver(() => updateLineNumbers(box)).observe(box);
  }
  updateLineNumbers(box);
}

export function setAPLFromText(text) {
  const parsed = parseAPLText(text);
  currentAPL = parsed;
  const box = document.getElementById('apl-edit-box');
  const errorEl = document.getElementById('apl-text-error');
  if (box) {
    box.value = formatAPLToText(currentAPL);
    updateLineNumbers(box);
    box.classList.remove('input-invalid');
  }
  if (errorEl) {
    errorEl.textContent = '';
    errorEl.style.display = 'none';
  }
  renderAPLTable();
  if (onChangeCallback) onChangeCallback(currentAPL);
  return currentAPL;
}

export function renderAPLUI() {
  const box = document.getElementById('apl-edit-box');
  const errorEl = document.getElementById('apl-text-error');
  if (box && document.activeElement !== box) {
    box.value = formatAPLToText(currentAPL);
    updateLineNumbers(box);
    box.classList.remove('input-invalid');
    if (errorEl) {
      errorEl.textContent = '';
      errorEl.style.display = 'none';
    }
  }
  renderAPLTable();
}

export function renderAPLTable() {
  const tbody = document.getElementById('apl-table-body');
  if (!tbody) return;

  tbody.innerHTML = '';
  const unreachableFlags = getAPLUnreachableFlags(currentAPL);
  currentAPL.forEach((entry, idx) => {
    const isEnabled = isAPLRuleEnabled(entry);
    const isUnreachable = unreachableFlags[idx];
    const tr = document.createElement('tr');
    tr.className = `apl-row ${isEnabled ? '' : 'disabled-row'} ${isUnreachable ? 'unreachable-row' : ''}`;
    tr.draggable = true;
    tr.dataset.index = idx;
    if (isUnreachable) {
      tr.setAttribute('title', 'Unreachable: This action will never execute because an earlier unconditional action always fires.');
    }

    const unreachableTag = isUnreachable ? '<span class="apl-unreachable-tag">Unreachable</span>' : '';

    tr.innerHTML = `
      <td class="apl-col-reorder">
        <div class="apl-reorder-btns">
          <button type="button" class="apl-btn-move up-btn" title="Move Up" ${idx === 0 ? 'disabled' : ''}>▲</button>
          <button type="button" class="apl-btn-move down-btn" title="Move Down" ${idx === currentAPL.length - 1 ? 'disabled' : ''}>▼</button>
        </div>
      </td>
      <td class="apl-col-priority">
        <span class="apl-prio-badge">#${idx + 1}</span>
      </td>
      <td class="apl-col-spell">
        <div class="apl-spell-cell">
          <img src="./assets/icons/${entry.icon}" alt="${entry.spell}" class="apl-spell-icon" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
          <span class="apl-spell-name">${entry.spell}</span>
          ${unreachableTag}
        </div>
      </td>
      <td class="apl-col-condition">
        <div class="apl-condition-badge" title="${isUnreachable ? 'Unreachable action' : 'Right-click or click ↗ to edit condition'}">
          <code>${escapeHtml(entry.condition || entry.rawCond || 'Always')}</code>
        </div>
      </td>
      <td class="apl-col-actions">
        <button type="button" class="apl-btn-edit edit-cond-btn" title="Edit Condition" data-index="${idx}">↗</button>
        <button type="button" class="apl-btn-toggle toggle-btn" title="${entry.enabled ? 'Disable' : 'Enable'}" data-index="${idx}">
          ${entry.enabled ? '●' : '○'}
        </button>
      </td>
    `;

    tr.querySelector('.up-btn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      moveAPLEntry(idx, idx - 1);
    });

    tr.querySelector('.down-btn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      moveAPLEntry(idx, idx + 1);
    });

    tr.querySelector('.edit-cond-btn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      openConditionEditor(idx);
    });

    tr.querySelector('.toggle-btn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      currentAPL[idx].enabled = !currentAPL[idx].enabled;
      renderAPLUI();
      if (onChangeCallback) onChangeCallback(currentAPL);
    });

    tr.addEventListener('contextmenu', (e) => {
      e.preventDefault();
      openConditionEditor(idx);
    });

    tr.addEventListener('dragstart', (e) => {
      dragSourceIndex = idx;
      tr.classList.add('dragging');
      e.dataTransfer.effectAllowed = 'move';
    });

    tr.addEventListener('dragover', (e) => {
      e.preventDefault();
      e.dataTransfer.dropEffect = 'move';
      tr.classList.add('drag-over');
    });

    tr.addEventListener('dragleave', () => {
      tr.classList.remove('drag-over');
    });

    tr.addEventListener('drop', (e) => {
      e.preventDefault();
      tr.classList.remove('drag-over');
      const targetIdx = Number(tr.dataset.index);
      if (dragSourceIndex !== -1 && dragSourceIndex !== targetIdx) {
        moveAPLEntry(dragSourceIndex, targetIdx);
      }
    });

    tr.addEventListener('dragend', () => {
      tr.classList.remove('dragging');
      document.querySelectorAll('.apl-row').forEach(r => r.classList.remove('drag-over'));
      dragSourceIndex = -1;
    });

    if (entry.id === 'hellfire') {
      const cell = tr.querySelector('.apl-spell-cell');
      cell?.setAttribute('data-wow-tooltip-title', 'Hellfire');
      cell?.setAttribute('data-wow-tooltip', '1300 Mana\nChanneled (15 sec)\nRequires Warlock, level 54\n210 Fire damage to the caster and nearby enemies every 1 sec for 15 sec.\nSpell coefficient: 0.022 per tick.');
    }
    tbody.appendChild(tr);
  });
  refreshAPLFallback();
}

function moveAPLEntry(fromIdx, toIdx) {
  if (fromIdx < 0 || fromIdx >= currentAPL.length || toIdx < 0 || toIdx >= currentAPL.length) return;
  const [removed] = currentAPL.splice(fromIdx, 1);
  currentAPL.splice(toIdx, 0, removed);
  renderAPLUI();
  if (onChangeCallback) onChangeCallback(currentAPL);
}

function setupConditionModal() {
  const modal = document.getElementById('apl-condition-modal');
  const closeBtn = document.getElementById('btn-close-apl-modal');
  const saveBtn = document.getElementById('btn-save-apl-cond');
  const cancelBtn = document.getElementById('btn-cancel-apl-cond');

  closeBtn?.addEventListener('click', closeConditionEditor);
  cancelBtn?.addEventListener('click', closeConditionEditor);

  saveBtn?.addEventListener('click', () => {
    if (editingIndex >= 0 && editingIndex < currentAPL.length) {
      const labelInput = document.getElementById('apl-modal-cond-label');
      const rawInput = document.getElementById('apl-modal-cond-raw');
      try {
        updateAPLCondition(editingIndex, labelInput?.value, rawInput?.value);
      } catch (error) {
        const message = document.getElementById('apl-modal-cond-error');
        if (message) message.textContent = error.message;
        return;
      }
    }
    closeConditionEditor();
  });
}

export function updateAPLCondition(index, label, rawCond) {
  const entry = currentAPL[index];
  if (!entry) throw new Error('APL action does not exist.');
  const updated = { ...entry, condition: label ?? entry.condition, rawCond: rawCond ?? entry.rawCond };
  if (String(updated.rawCond ?? '').trim() !== String(entry.rawCond ?? '').trim()) {
    for (const key of ['cond', 'param', 'targetSpell', 'cond1', 'param1', 'targetSpell1',
      'cond2', 'param2', 'targetSpell2', 'condKey', 'condKey1', 'condKey2', 'condition1', 'condition2']) {
      delete updated[key];
    }
  }
  compileAPLToBytecode([{ ...updated, enabled: true }]);
  currentAPL[index] = updated;
  renderAPLUI();
  if (onChangeCallback) onChangeCallback(currentAPL);
}

function openConditionEditor(index) {
  const message = document.getElementById('apl-modal-cond-error');
  if (message) message.textContent = '';
  editingIndex = index;
  const entry = currentAPL[index];
  if (!entry) return;

  const modal = document.getElementById('apl-condition-modal');
  const title = document.getElementById('apl-modal-spell-title');
  const icon = document.getElementById('apl-modal-spell-icon');
  const labelInput = document.getElementById('apl-modal-cond-label');
  const rawInput = document.getElementById('apl-modal-cond-raw');

  if (title) title.textContent = `Edit Condition: ${entry.spell}`;
  if (icon) icon.src = `./assets/icons/${entry.icon}`;
  if (labelInput) labelInput.value = entry.condition;
  if (rawInput) rawInput.value = entry.rawCond || entry.condition;

  if (modal) {
    modal.style.display = 'flex';
  }
}

function closeConditionEditor() {
  const modal = document.getElementById('apl-condition-modal');
  if (modal) {
    modal.style.display = 'none';
  }
  editingIndex = -1;
}

function setupMultiDotToggle() {
  const toggle = document.getElementById('check-multidot-corruption');
  toggle?.addEventListener('change', () => {
    if (onChangeCallback) onChangeCallback(currentAPL);
  });
}

function escapeHtml(str) {
  return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

