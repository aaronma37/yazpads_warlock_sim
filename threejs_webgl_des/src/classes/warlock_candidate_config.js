import { buildFightConfig } from '../config_builder.js';

// Bind conversion dependencies once, preserving existing candidate/config semantics.
export function createWarlockCandidateConfig({ talents, apl, policyAPL, resolveTalentFlags }) {
  const TalentGraph = talents.graph;
  const getTalentFlagsFromRanks = resolveTalentFlags;
  const individualToBytecodeRules = apl.encodeIndividual;
  const buildPolicyAPL = policyAPL.build;

// Preserve the existing public candidate API; class rules consume resolved talent flags.
function getPolicyAPLAndActions(ind) {
  const talentsObj = TalentGraph.toTalentsObject(ind.talents);
  const tf = getTalentFlagsFromRanks(talentsObj);
  return buildPolicyAPL(ind.rotation, tf);
}

// Convert Individual to Shader Simulation Config
function individualToConfig(ind, baseStatsConfig) {
  const talentsObj = TalentGraph.toTalentsObject(ind.talents);
  const tf = getTalentFlagsFromRanks(talentsObj);

  let shaderRotation, actionIds, aplRules;
  if (ind.apl && Array.isArray(ind.apl.rules) && ind.apl.rules.length > 0) {
    aplRules = individualToBytecodeRules(ind.apl);
    const activeSpells = ind.apl.rules.filter(r => r.enabled).map(r => r.id);
    actionIds = activeSpells;

    const firstNuke = activeSpells.find(id => id === 'incinerate' || id === 'searing' || id === 'bolt' || id === 'wrack' || id === 'hellfire');
    if (firstNuke === 'incinerate') shaderRotation = 'fire';
    else if (firstNuke === 'searing') shaderRotation = 'searing';
    else shaderRotation = 'shadow';
  } else {
    const pol = getPolicyAPLAndActions(ind);
    shaderRotation = pol.shaderRotation;
    actionIds = pol.actionIds;
    aplRules = pol.aplRules;
  }

  const base = {
    ...baseStatsConfig,
    rotation: shaderRotation,
    tapThreshold: baseStatsConfig.tapThreshold !== undefined ? baseStatsConfig.tapThreshold : 25
  };
  const baseRace = (base.race || 'HUMAN').toUpperCase();
  base.race = ind.race;

  // Un-apply baseline race stats if needed so all candidates start from neutral baseline
  let unscaledInt = base.intellect;
  let unscaledSpirit = base.spirit;
  if (baseRace === 'HUMAN') unscaledSpirit = Math.round(unscaledSpirit / 1.05);
  base.intellect = unscaledInt;
  base.spirit = ind.race === 'HUMAN' ? Math.round(unscaledSpirit * 1.05) : unscaledSpirit;

  return buildFightConfig({
    base,
    talent: tf,
    pet: ind.pet,
    sac: ind.sacImp ? 'imp' : ind.sacSuccubus ? 'succubus' : 'none',
    actionIds,
    aplRules,
    rotation: shaderRotation
  });
}

// Convert Individual APL to a FightConfig ready for simulation
function aplToConfig(ind, baseStatsConfig) {
  const aplRules = individualToBytecodeRules(ind);
  const base = { ...baseStatsConfig };

  // Set rotation to match the dominant/first damage nuke in this APL priority order
  const activeSpells = ind.rules.filter(r => r.enabled).map(r => r.id);
  const firstNuke = activeSpells.find(id => id === 'incinerate' || id === 'searing' || id === 'bolt' || id === 'wrack' || id === 'hellfire');
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

  return Object.freeze({ buildToConfig: individualToConfig, policyForBuild: getPolicyAPLAndActions, aplToConfig });
}
