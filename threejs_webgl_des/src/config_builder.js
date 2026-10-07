import { DEFAULTS, validate } from './model.js';

// The only place that translates UI/preset choices and talent flags into the
// config consumed by packConfig/the GLSL kernel.
export function buildFightConfig({ base = {}, talent = {}, pet = 'none', sac = 'none',
  actionIds = null, aplRules = null, rotation = base.rotation || 'shadow', resolvedConfig = null } = {}) {
  // Build strings and exported simulation results contain an already-resolved
  // config. Validate it through the same boundary without applying talents twice.
  if (resolvedConfig) return validate(resolvedConfig);
  const config = { ...DEFAULTS };
  for (const [k, v] of Object.entries(base)) {
    if (k in DEFAULTS) config[k] = v;
  }
  config.rotation = rotation;
  config.petChoice = pet;
  config.sacSucc = sac === 'succubus' && talent.sacRank > 0;
  config.sacImp = sac === 'imp' && talent.sacRank > 0;
  config.hit += talent.suppressionHit || 0;
  Object.assign(config, {
    improvedTap: talent.improvedTap,
    tapBonus: talent.tapBonus,
    instantCorruption: talent.instantCorruption,
    shadowMasteryBonus: talent.shadowMasteryBonus || 0,
    improvedCorruptionBonus: talent.improvedCorruptionBonus,
    maledictionBonus: talent.maledictionBonus,
    improvedDrainsBonus: talent.improvedDrainsBonus,
    malevolence: talent.malevolence,
    nightfall: talent.nightfall,
    nightfallChance: talent.nightfallChance,
    isb: talent.isb,
    isbBonus: talent.isbBonus,
    ruin: talent.ruin,
    ruinRank: talent.ruinRank,
    siphonLife: talent.siphonLife,
    soulSiphonBonus: talent.soulSiphonBonus,
    decimation: talent.decimation,
    baneRank: talent.baneRank || 0,
    decimationRank: talent.decimationRank || 0,
    demonicBrand: talent.demonicBrand,
    demonicBrandRank: talent.demonicBrandRank,
    demonicEnergies: talent.demonicEnergies,
    masterDemo: pet === 'none' ? 0 : talent.masterDemo,
    afBonus: talent.afBonus,
    aftermathBonus: talent.aftermathBonus || 0,
    cataclysmCostMult: talent.cataclysmCostMult !== undefined ? talent.cataclysmCostMult : 1.0,
    improvedAgonyBonus: talent.improvedAgonyBonus || 0,
    amplifyCurse: talent.amplifyCurse || false,
    felVitalityBonus: talent.felVitalityBonus || 0,
    fnbCrit: talent.fnbCrit,
    snfChance: talent.snfChance,
    snfBonus: talent.snfBonus,
    dotCrit: talent.dotCrit,
    petFireboltMult: talent.petFireboltMult,
    petMeleeMult: talent.petMeleeMult,
    petLashMult: talent.petLashMult,
    brandMult: talent.brandMult,
  });
  if (pet !== 'none') config.spellPower += (talent.demonicKnowledge || 0) * 20;
  const hasPet = pet !== 'none';
  config.shadowMultiplier = (talent.shadowMultiplier || 1) * (config.sacImp ? 1.15 : 1) *
    (pet === 'succubus' ? 1 + talent.masterDemo * 0.02 : 1) * (hasPet && talent.soulLink ? 1.03 : 1);
  config.fireMultiplier = (talent.fireMultiplier || 1) * (config.sacSucc ? 1.15 : 1) *
    (pet === 'imp' ? 1 + talent.masterDemo * 0.02 : 1) * (hasPet && talent.soulLink ? 1.03 : 1);
  config.corrMultiplier = config.shadowMultiplier *
    (1 + (talent.shadowMasteryBonus || 0) + talent.maledictionBonus + talent.improvedCorruptionBonus);
  if (actionIds) {
    const actions = new Set(actionIds);
    config.corruption = actions.has('corr');
    config.agony = actions.has('agony');
    config.immolate = actions.has('immo');
    config.conflagrate = actions.has('conflag') && talent.conflagrate;
    config.shadowburn = actions.has('shadowburn') && talent.shadowburn;
    config.incinerate = actions.has('incinerate') && talent.incinerate;
    config.curseOfDoom = actions.has('curse');
    config.drainHope = actions.has('wrack') && talent.wrack;
    config.aplRules = aplRules;
  } else if (aplRules) {
    config.aplRules = aplRules;
  }
  return validate(config);
}
