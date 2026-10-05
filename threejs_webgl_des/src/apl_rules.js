// Availability checks (cooldowns, procs, mana and talents) happen in the shader.
// An Always condition alone does not make later enabled rules unreachable.
export function isAPLRuleEnabled(rule) {
  return rule.enabled !== false && rule.enabled !== 0 &&
    rule.condKey !== 'NEVER' && rule.condKey1 !== 'NEVER' && rule.condKey2 !== 'NEVER' &&
    String(rule.rawCond ?? '').trim().toLowerCase() !== 'false';
}

export function getEnabledAPLRules(rules = []) {
  return rules.filter(isAPLRuleEnabled);
}
