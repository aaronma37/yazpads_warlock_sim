import { APL_ACTION, APL_COND } from './model.js';

// Display-only filtering: preserve the executable APL and its saved rules.
export function getRaceVisibleAPLRules(rules = [], race) {
  if (String(race || '').toUpperCase() === 'GNOME') return rules;
  return rules.filter(rule => rule.action !== APL_ACTION.EUREKA &&
    !/^eureka!?$/i.test(String(rule.id || '').trim()) &&
    !/^eureka!?$/i.test(String(rule.spell || '').trim()));
}

// Availability checks (cooldowns, procs, mana and talents) happen in the shader.
// An Always condition alone does not make later enabled rules unreachable if the spell has a cooldown or dot duration.
export function isAPLRuleEnabled(rule) {
  return rule.enabled !== false && rule.enabled !== 0 &&
    rule.condKey !== 'NEVER' && rule.condKey1 !== 'NEVER' && rule.condKey2 !== 'NEVER' &&
    String(rule.rawCond ?? '').trim().toLowerCase() !== 'false';
}

export function getEnabledAPLRules(rules = []) {
  return rules.filter(isAPLRuleEnabled);
}

/**
 * Checks if a rule's condition evaluates unconditionally ("Always" / "true").
 */
export function isRuleConditionAlways(rule) {
  if (!rule) return false;
  const raw = String(rule.rawCond ?? '').trim().toLowerCase();
  if (raw && raw !== 'true' && raw !== 'always' && raw !== '1') {
    return false;
  }
  if (rule.condKey && rule.condKey !== 'ALWAYS') return false;
  if (rule.condKey1 && rule.condKey1 !== 'ALWAYS') return false;
  if (rule.condKey2 && rule.condKey2 !== 'ALWAYS') return false;
  if (rule.cond !== undefined && rule.cond !== 0 && rule.cond !== APL_COND.ALWAYS) return false;
  if (rule.cond1 !== undefined && rule.cond1 !== 0 && rule.cond1 !== APL_COND.ALWAYS) return false;
  if (rule.cond2 !== undefined && rule.cond2 !== 0 && rule.cond2 !== APL_COND.ALWAYS) return false;

  if (!raw && rule.condition) {
    const condLower = String(rule.condition).trim().toLowerCase();
    if (condLower !== 'always' && condLower !== 'always when available' && condLower !== 'true' && !condLower.startsWith('filler')) {
      return false;
    }
  }
  return true;
}

export const UNCONDITIONAL_BLOCKING_ACTION_IDS = new Set([
  'bolt', 'shadow_bolt', 'shadowbolt',
  'searing', 'searing_pain',
  'incinerate',
  'hellfire',
  'drain', 'drain_soul',
  'tap', 'life_tap', 'lifetap'
]);

/**
 * Returns true if an enabled rule is a non-cooldown filler or unconditional action that will ALWAYS
 * execute when evaluated, preventing any subsequent rules in the APL from ever being reached.
 */
export function isAPLRuleBlocking(rule) {
  if (!isAPLRuleEnabled(rule)) return false;
  if (!isRuleConditionAlways(rule)) return false;

  const id = String(rule.id || '').toLowerCase();
  const spell = String(rule.spell || '').toLowerCase();
  const action = rule.action;

  if (UNCONDITIONAL_BLOCKING_ACTION_IDS.has(id)) return true;
  if (spell.includes('shadow bolt') && !spell.includes('nightfall')) return true;
  if (spell === 'searing pain') return true;
  if (spell === 'incinerate') return true;
  if (spell === 'hellfire') return true;
  if (spell === 'drain soul') return true;
  if (spell === 'life tap') return true;

  if (action === APL_ACTION.SHADOW_BOLT_FILLER ||
      action === APL_ACTION.SEARING_PAIN_FILLER ||
      action === APL_ACTION.INCINERATE_FILLER ||
      action === APL_ACTION.HELLFIRE ||
      action === APL_ACTION.DRAIN_SOUL_FILLER ||
      action === APL_ACTION.LIFE_TAP) {
    return true;
  }

  return false;
}

/**
 * Computes a boolean array indicating whether each rule in the list is unreachable
 * (i.e. preceded by an enabled unconditional blocking action).
 */
export function getAPLUnreachableFlags(rules = []) {
  let isBlocked = false;
  return rules.map(rule => {
    const unreachable = isBlocked;
    if (isAPLRuleEnabled(rule) && isAPLRuleBlocking(rule)) {
      isBlocked = true;
    }
    return unreachable;
  });
}

