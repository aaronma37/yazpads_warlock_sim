import { buildFightConfig } from '../config_builder.js';
import { resolveWarlockBuffEffects } from './warlock_import_buffs.js';
import { resolveWarlockEquipment } from './warlock_equipment.js';
import { WARLOCK_SEARCH } from './warlock_search.js';
import { DEFAULTS, validate, MAX_APL_RULES, APL_ACTION, APL_COND } from '../model.js';
import { parseAPLText, compileAPLToBytecode, ACTION_INFO } from '../apl.js';
import { getTalentFlagsFromRanks } from '../talents.js';

const statKeys = ['swordEquipped', 'spellPower', 'shadowPower', 'firePower', 'intellect', 'stamina', 'spirit', 'hit', 'crit', 'mp5', 'penetration'];
export const WARLOCK_IMPORT_BUFF_KEYS = Object.freeze(['flaskSupremePower', 'greaterArcaneElixir', 'shadowPowerElixir', 'greaterFirepowerElixir', 'wizardOil', 'magebloodElixir', 'nightfinSoup', 'arcaneIntellect', 'markOfTheWild', 'blessingOfKings', 'blessingOfWisdom', 'manaSpringTotem', 'dragonslayer', 'songflower', 'warchiefsBlessing', 'spiritOfZandalar', 'saygesFortune', 'curseOfShadow', 'curseOfElements', 'shadowWeaving', 'improvedScorch', 'nightfallProcDebuff']);
const trees = ['affliction', 'demonology', 'destruction'];
function record(value, name) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error(`${name} must be an object.`);
}
function keys(value, allowed, name) {
  record(value, name);
  for (const key of Object.keys(value)) if (!allowed.includes(key)) throw new Error(`Unsupported ${name}: ${key}`);
}

// Normalize staged direct/equipped imports. No UI mutation or effect application.
// Equipment/buff resolution and evaluation remain separate integration steps.
export function normalizeWarlockImport(staged, { itemDatabase } = {}) {
  record(staged, 'Staged import');
  if (staged.classId !== 'warlock') throw new Error('Warlock import requires classId warlock.');
  if (staged.kind === 'resolved-config') {
    record(staged.resolvedConfig, 'Resolved config');
    const config = validate(structuredClone(staged.resolvedConfig));
    if (config.aplRules !== null) {
      if (!Array.isArray(config.aplRules) || config.aplRules.length > MAX_APL_RULES) throw new Error('Invalid resolved APL rules.');
      for (const rule of config.aplRules) {
        record(rule, 'Resolved APL rule');
        if (!Object.values(APL_ACTION).includes(rule.action)) throw new Error('Unsupported resolved APL action.');
        for (const field of ['cond', 'cond1', 'cond2']) if (Object.hasOwn(rule, field) && !Object.values(APL_COND).includes(rule[field])) throw new Error('Unsupported resolved APL condition.');
        for (const field of ['param', 'param1', 'param2']) if (Object.hasOwn(rule, field) && !Number.isFinite(rule[field])) throw new Error('Invalid resolved APL parameter.');
        for (const field of ['targetSpell', 'targetSpell1', 'targetSpell2']) if (Object.hasOwn(rule, field) && ![0,1,2,3,5,13,15,22].includes(rule[field])) throw new Error('Unsupported resolved APL target.');
        if (![0, 1, false, true].includes(rule.enabled)) throw new Error('Invalid resolved APL enabled flag.');
      }
    }
    return { ...structuredClone(staged), resolvedConfig: config };
  }
  if (staged.kind !== 'logical-build') throw new Error('Unsupported staged import kind.');
  const c = structuredClone(staged.candidate);
  record(c, 'Candidate');
  if (c.classId !== 'warlock' || c.schemaVersion !== 1 || typeof c.candidateId !== 'string' || !c.candidateId.trim()) throw new Error('Invalid Warlock candidate identity.');
  if (Object.hasOwn(c.stats, 'haste')) {
    if (c.stats.haste !== 0) throw new Error('Haste is unsupported.');
    delete c.stats.haste;
  }
  keys(c.stats, statKeys, 'stat');
  c.stats = { ...Object.fromEntries(statKeys.map(key => [key, DEFAULTS[key]])), ...c.stats };
  keys(c.classOptions, ['race', 'pet', 'sacrifice', 'rotation'], 'class option');
  c.classOptions = { race: 'HUMAN', pet: 'imp', sacrifice: 'none', rotation: 'shadow', ...c.classOptions };
  const options = c.classOptions;
  if (typeof options.race !== 'string') throw new Error('Race must be a string.');
  options.race = options.race.toUpperCase();
  if (!WARLOCK_SEARCH.choices.races.includes(options.race)) throw new Error('Unsupported Warlock race.');
  if (!['none', 'imp', 'succubus'].includes(options.pet) || !['none', 'imp', 'succubus'].includes(options.sacrifice)) throw new Error('Unsupported pet/sacrifice choice.');
  if (!['shadow', 'fire', 'searing', 'bolt'].includes(options.rotation)) throw new Error('Unsupported rotation.');
  keys(c.equipment, ['mode', 'items'], 'equipment field');
  if (!Object.hasOwn(c.equipment, 'mode')) c.equipment.mode = 'direct';
  record(c.equipment.items, 'Equipment items');
  if (!['direct', 'equipped'].includes(c.equipment.mode)) throw new Error('Unsupported equipment mode.');
  if (c.equipment.mode === 'equipped' || Object.keys(c.equipment.items).length) {
    const equipped = resolveWarlockEquipment(c.equipment.items, itemDatabase);
    if (c.equipment.mode === 'equipped') c.stats = { ...equipped, swordEquipped: c.stats.swordEquipped };
  }
  keys(c.talents, trees, 'talent tree');
  c.talents = { affliction: {}, demonology: {}, destruction: {}, ...c.talents };
  const { graph, definitions } = WARLOCK_SEARCH.talents;
  for (let tree = 0; tree < 3; tree++) {
    const ranks = c.talents[trees[tree]];
    keys(ranks, definitions.filter(d => d.tree === tree).map(d => d.id), 'talent');
    for (const [id, rank] of Object.entries(ranks)) {
      const def = definitions.find(d => d.id === id);
      if (!Number.isInteger(rank) || rank < 0 || rank > def.max) throw new Error(`Invalid talent rank: ${id}`);
    }
  }
  const vector = graph.fromTalentsObject(c.talents);
  const total = graph.countTotalPoints(vector);
  if (total > 51 || !graph.isValid(vector, total)) throw new Error('Invalid talent budget, row requirement, or prerequisite.');
  const flags = getTalentFlagsFromRanks(c.talents);
  if (options.sacrifice !== 'none' && !flags.sacRank) throw new Error('Sacrifice requires Demonic Sacrifice.');
  if (options.sacrifice !== 'none' && options.pet !== 'none' && !vector[35]) throw new Error('Active pet with sacrifice requires Demonic Pact.');
  keys(c.encounter, ['duration', 'distance', 'target'], 'encounter field');
  if (Object.hasOwn(c.encounter, 'target')) record(c.encounter.target, 'Target');
  const target = { level: 63, targetCount: 1, targetHP: 1000000, bossArmor: DEFAULTS.bossArmor,
    resistance: DEFAULTS.resistance, racialPolicy: DEFAULTS.racialPolicy, targetIsBeast: false, ...c.encounter.target };
  keys(target, ['level', 'targetCount', 'targetHP', 'bossArmor', 'resistance', 'racialPolicy', 'targetIsBeast'], 'target field');
  if (target.level !== 63 || target.targetCount !== 1) throw new Error('Only level-63 single-target encounters are supported by this import adapter.');
  if (!Number.isFinite(target.targetHP) || target.targetHP <= 0) throw new Error('Invalid target HP.');
  c.encounter = { duration: DEFAULTS.duration, distance: DEFAULTS.distance, ...c.encounter, target };
  const evaluationDefaults = { iterations: DEFAULTS.iterations, mode: 'detailed', ...staged.evaluationDefaults };
  keys(evaluationDefaults, ['iterations', 'mode'], 'evaluation default');
  if (!['fast', 'detailed'].includes(evaluationDefaults.mode)) throw new Error('Unsupported output mode.');
  validate({ ...c.stats, duration: c.encounter.duration, distance: c.encounter.distance,
    iterations: evaluationDefaults.iterations, race: options.race, rotation: options.rotation,
    petChoice: options.pet, bossArmor: target.bossArmor, resistance: target.resistance,
    racialPolicy: target.racialPolicy, targetIsBeast: target.targetIsBeast });
  record(c.buffs, 'Buffs');
  keys(c.buffs, WARLOCK_IMPORT_BUFF_KEYS, 'buff');
  // Buff effect resolution remains separate; this is not yet an evaluable build.
  for (const value of Object.values(c.buffs)) if (typeof value !== 'boolean') throw new Error('Buff selections must be boolean.');
  c.apl ??= { kind: 'policy', policyId: 'SHADOW_DESTRO' };
  if (c.apl.kind === 'text') {
    if (typeof c.apl.text !== 'string') throw new Error('APL text must be a string.');
    const rules = parseAPLText(c.apl.text);
    if (rules.length > MAX_APL_RULES) throw new Error('APL exceeds rule limit.');
    const allowed = new Set(WARLOCK_SEARCH.apl.availableActions(flags).map(a => a.id));
    allowed.add('drain');
    for (const rule of rules) if (rule.enabled && !allowed.has(rule.id)) throw new Error(`Action requires an unavailable talent: ${rule.id}`);
    compileAPLToBytecode(rules);
  } else if (c.apl.kind === 'policy') {
    if (!WARLOCK_SEARCH.choices.rotations.includes(c.apl.policyId)) throw new Error('Unsupported Warlock policy.');
  } else if (c.apl.kind === 'rules') {
    if (!Array.isArray(c.apl.rules) || !c.apl.rules.length || c.apl.rules.length > MAX_APL_RULES) throw new Error('Invalid APL rule count.');
    const allowed = new Set(WARLOCK_SEARCH.apl.availableActions(flags).map(a => a.id));
    allowed.add('drain');
    for (const rule of c.apl.rules) {
      record(rule, 'APL rule');
      if (!Object.hasOwn(ACTION_INFO, rule.id)) throw new Error(`Unsupported APL action: ${rule.id}`);
      if (typeof rule.enabled !== 'boolean' || typeof rule.rawCond !== 'string') throw new Error('APL rules require boolean enabled and string rawCond.');
      if (rule.enabled && !allowed.has(rule.id)) throw new Error(`Action requires an unavailable talent: ${rule.id}`);
      if (!rule.rawCond.trim() || /[\r\n]/.test(rule.rawCond)) throw new Error('APL rule conditions must be nonempty single lines.');
      // Compile every condition, including disabled rows, without trusting numeric IDs.
      parseAPLText(`${rule.id} if ${rule.rawCond}`);
    }
    c.apl.rules = c.apl.rules.map(rule => {
      const parsed = parseAPLText(`${rule.id} if ${rule.rawCond}`)[0];
      return { ...parsed, enabled: rule.enabled && parsed.enabled };
    });
    compileAPLToBytecode(c.apl.rules);
  } else throw new Error('Unsupported APL kind.');
  return { ...structuredClone(staged), candidate: c, evaluationDefaults };
}

// Return normalized logical inputs plus resolved buff/racial stats. This does not
// apply talents/sacrifice or produce the final simulation config yet.
export function resolveWarlockImportEffects(staged, options) {
  const normalized = normalizeWarlockImport(staged, options);
  if (normalized.kind === 'resolved-config') return normalized;
  const c = normalized.candidate;
  return { ...normalized, effectiveStats: resolveWarlockBuffEffects(
    c.stats, c.buffs, c.classOptions.race, c.encounter.target) };
}

// Resolve once at the import boundary using the existing configuration builder.
// Preserve its current multiplier behavior; changing mechanics requires parity work.
export function resolveWarlockImport(staged, options) {
  const normalized = resolveWarlockImportEffects(staged, options);
  if (normalized.kind === 'resolved-config') return { ...normalized, config: normalized.resolvedConfig };
  const c = normalized.candidate;
  const flags = getTalentFlagsFromRanks(c.talents);
  let actionIds, aplRules, rotation = c.classOptions.rotation;
  if (c.apl.kind === 'policy') {
    const policy = WARLOCK_SEARCH.policyAPL.build(c.apl.policyId, flags);
    ({ actionIds, aplRules, shaderRotation: rotation } = policy);
  } else {
    const rules = c.apl.kind === 'text' ? parseAPLText(c.apl.text) : c.apl.rules;
    actionIds = rules.filter(r => r.enabled).map(r => r.id);
    aplRules = compileAPLToBytecode(rules);
  }
  const { haste, maxMana, ...effective } = normalized.effectiveStats;
  const config = buildFightConfig({
    base: { ...c.stats, ...effective, race: c.classOptions.race,
      racialPolicy: c.encounter.target.racialPolicy, targetIsBeast: c.encounter.target.targetIsBeast,
      duration: c.encounter.duration, distance: c.encounter.distance,
      iterations: normalized.evaluationDefaults.iterations },
    talent: flags, pet: c.classOptions.pet, sac: c.classOptions.sacrifice,
    actionIds, aplRules, rotation,
  });
  return { ...normalized, config };
}
