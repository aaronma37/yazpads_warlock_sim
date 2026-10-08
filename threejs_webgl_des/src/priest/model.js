import { RACIAL_DEFAULTS, RACIAL_CONFIG, RACIAL_BOUNDS, RACIAL_STATE } from './racials.js';
// Resolved Priest damage-core GPU contract; logical stats/talents use talents.js.
export const SCOPE = 'priest-damage-core-v7';
export const SPELLS = Object.freeze(['Shadow Word: Pain', 'Mind Flay', 'Mind Blast', 'Shadow Word: Death', 'Smite', 'Holy Fire', 'Penance', 'Holy Nova', 'Devouring Plague', 'Wand', 'Mana Burn', 'Starshards', 'Chastise', 'Shadowguard', 'Feedback', 'Touch of Weakness', 'Hex of Weakness', 'Dark Sacrifice', 'Desperate Prayer', 'Divine Grace', 'Contingency Plan', 'Confounding Flash', "Elune’s Grace", 'Touch of the Grave']);
// Null denotes a spell absent from the current CPU Priest implementation.
export const CPU_IDS = Object.freeze([1, 2, 3, 4, 10, 11, 13, 12, 5, null, null, null, ...Array(12).fill(null)]);
export const HEAP_CAPACITY = 32;
export const STATUS = Object.freeze({ complete: 1, heapOverflow: 2, eventBudget: 3 });
const APL_DEFAULTS=Object.fromEntries(Array.from({length:11},(_,i)=>[`apl${i}`,0]));
export const DEFAULTS = Object.freeze({ ...RACIAL_DEFAULTS, ...APL_DEFAULTS, aplEnabled:0, duration: 180, iterations: 4096, seed: 42,
  spellPower: 500, maxMana: 4200, manaPerTick: 8, hitChance: 0.95,
  critChance: 0.15, critMultiplier: 1.5, damageMultiplier: 1, costMultiplier: 1,
  swpTicks: 6, mindBlastCooldown: 8, rotation: 'shadow',
  instantDamageMultiplier: 1, flayDamageMultiplier: 1, instantCostMultiplier: 1, mindFlayEnabled: 1, weavingChance: 0, spiritManaPerTick: 0, castingRegenRatio: 0, innerFocusEnabled: 0, holyBaseCostMultiplier: 1, holyDamageMultiplier: 1, holySpellPower: 500, holyHitChance: 0.95, holyCritChance: 0.15, holyCastReduction: 0, holyCostMultiplier: 1, powerInLightBonus: 0, penanceEnabled: 0, penanceCostMultiplier: 1, holyNovaEnabled: 0, deathExecuteCritBonus: 0, plagueEnabled: 0, plagueCostMultiplier: 1, plagueSpreadRadius: 0, powerInfusionEnabled: 0, searingNovaChance: 0, targetDistance: 0, shadowRangeMultiplier: 1, holyRangeMultiplier: 1, flayRangeBonus: 0, wandHitChance: 0.95, wandDamage: 100, wandInterval: 1.5, wandDamageMultiplier: 1, shadowThreatMultiplier: 1, holyThreatMultiplier: 1, manaBurnCastReduction: 0, targetMana: 0, targetDeathInterval: 0, targetDeaths: 0, nextTargetDistance: 0, spiritTapChance: 0, spiritTapSpellPower: 0, starshardsEnabled: 0, arcaneSpellPower: 500, arcaneHitChance: 0.95, arcaneDamageMultiplier: 1, arcaneCostMultiplier: 1, arcaneThreatMultiplier: 1, clipMindFlayEnabled: 1, manaPoolingEnabled: 1 });
export const ROTATIONS = Object.freeze({ shadow: 0, 'pain-only': 1, 'flay-only': 2, 'blast-only': 3, 'death-only': 4, holy: 5, 'smite-only': 6, 'holy-fire-only': 7, 'penance-only': 8, 'nova-only': 9, 'plague-only': 10, 'wand-only': 11, 'mana-burn-only': 12, 'holy-nova': 13, mixed: 14, 'starshards-only': 15, 'chastise-only': 16, 'idle': 17 });
export const CONFIG = Object.freeze({ durationMs: 'u32', spellPower: 'f32', maxMana: 'f32',
  manaPerTick: 'f32', hitChance: 'f32', critChance: 'f32', critMultiplier: 'f32',
  damageMultiplier: 'f32', costMultiplier: 'f32', swpTicks: 'u32', mindBlastCooldownMs: 'u32', rotation: 'u32', instantDamageMultiplier: 'f32',
  flayDamageMultiplier: 'f32', instantCostMultiplier: 'f32', mindFlayEnabled: 'u32', weavingChance: 'f32', spiritManaPerTick: 'f32', castingRegenRatio: 'f32', innerFocusEnabled: 'u32', holyBaseCostMultiplier: 'f32', holyDamageMultiplier: 'f32', holySpellPower: 'f32', holyHitChance: 'f32', holyCritChance: 'f32', holyCastReduction: 'f32', holyCostMultiplier: 'f32', powerInLightBonus: 'f32', penanceEnabled: 'u32', penanceCostMultiplier: 'f32', holyNovaEnabled: 'u32', deathExecuteCritBonus: 'f32', plagueEnabled: 'u32', plagueCostMultiplier: 'f32', plagueSpreadRadius: 'f32', powerInfusionEnabled: 'u32', searingNovaChance: 'f32', targetDistance: 'f32', shadowRangeMultiplier: 'f32', holyRangeMultiplier: 'f32', flayRangeBonus: 'f32', wandHitChance: 'f32', wandDamage: 'f32', wandIntervalMs: 'u32', wandDamageMultiplier: 'f32', shadowThreatMultiplier: 'f32', holyThreatMultiplier: 'f32', manaBurnCastReduction: 'f32', targetMana: 'f32', targetDeathIntervalMs: 'u32', targetDeaths: 'u32', nextTargetDistance: 'f32', spiritTapChance: 'f32', spiritTapSpellPower: 'f32', starshardsEnabled: 'u32', arcaneSpellPower: 'f32', arcaneHitChance: 'f32', arcaneDamageMultiplier: 'f32', arcaneCostMultiplier: 'f32', arcaneThreatMultiplier: 'f32', clipMindFlayEnabled: 'u32', manaPoolingEnabled: 'u32', ...RACIAL_CONFIG, ...Object.fromEntries(Object.keys(APL_DEFAULTS).map(key=>[key,'u32'])), aplEnabled:'u32' });
// Detailed words have full-width counters rather than silently truncating to 16 bits.
export const FAST_STATE = Object.freeze({ total: 'f32', done: 'u32', events: 'u32', rngCalls: 'u32',
  mana: 'f32', manaSpent: 'f32', manaGained: 'f32', highWater: 'u32' });
export const STATE = Object.freeze({ ...FAST_STATE, ...RACIAL_STATE, ...Object.fromEntries(SPELLS.flatMap((_, i) =>
  [[`damage${i}`, 'f32'], [`casts${i}`, 'u32'], [`hits${i}`, 'u32'], [`crits${i}`, 'u32'], [`misses${i}`, 'u32']])), innerFocusUses: 'u32', powerInfusionUses: 'u32', freeNovaUses: 'u32', plagueSpreads: 'u32', spiritTapProcs: 'u32', threat: 'f32', targetManaBurned: 'f32', flayClips: 'u32', manaPoolWaits: 'u32', manaPoolTimeMs: 'f32' });
export const CONFIG_WORDS = Object.keys(CONFIG).length;
export const STATE_WORDS = Object.keys(STATE).length;
export const COMPACT_STRIPES = Math.ceil(STATE_WORDS / 16);

export function validate(input = {}) {
  if (!input || typeof input !== 'object' || Array.isArray(input)) throw new Error('Priest config must be an object.');
  for (const key of Object.keys(input)) if (!Object.hasOwn(DEFAULTS, key)) throw new Error(`Unsupported Priest core field: ${key}`);
  const config = { ...DEFAULTS, ...input };
  if(![0,1].includes(config.aplEnabled))throw new Error('Invalid Priest aplEnabled.');
  const seen=new Set();
  for(const key of Object.keys(APL_DEFAULTS)){const word=config[key],spell=word&15;
    if(!Number.isInteger(word)||word<0||word>524287||(spell>9&&spell!=12&&spell!=13)||((word>>>4)&127)>100||((word>>>11)&127)>100||(!spell&&word)|| (spell&&seen.has(spell)))throw new Error('Invalid Priest APL word.');
    if(spell)seen.add(spell);
  }
  const bounds = { ...RACIAL_BOUNDS, duration: [1, 1800], iterations: [1, 1048576], seed: [0, 4294967295],
    spellPower: [0, 20000], maxMana: [1, 1000000], manaPerTick: [0, 1000000], hitChance: [0, 1],
    critChance: [0, 1], critMultiplier: [1, 3], damageMultiplier: [0, 10], costMultiplier: [0, 2],
    swpTicks: [6, 8], mindBlastCooldown: [5.5, 8], instantDamageMultiplier: [1, 1.05],
    flayDamageMultiplier: [1, 1.2], instantCostMultiplier: [0.9, 1], mindFlayEnabled: [0, 1], weavingChance: [0, 1], spiritManaPerTick: [0, 1000000], castingRegenRatio: [0, 1], innerFocusEnabled: [0, 1], holyBaseCostMultiplier: [0, 2], holyDamageMultiplier: [0, 10], holySpellPower: [0, 20000], holyHitChance: [0, 1], holyCritChance: [0, 1], holyCastReduction: [0, 0.5], holyCostMultiplier: [0.9, 1], powerInLightBonus: [0, 0.1], penanceEnabled: [0, 1], penanceCostMultiplier: [0.85, 1], holyNovaEnabled: [0, 1], deathExecuteCritBonus: [0, 0.3], plagueEnabled: [0, 1], plagueCostMultiplier: [0.5, 1], plagueSpreadRadius: [0, 10], powerInfusionEnabled: [0, 1], searingNovaChance: [0, 0.1], targetDistance: [0, 100], shadowRangeMultiplier: [1, 1.2], holyRangeMultiplier: [1, 1.2], flayRangeBonus: [0, 10], wandHitChance: [0, 1], wandDamage: [0, 10000], wandInterval: [0.5, 10], wandDamageMultiplier: [1, 1.25], shadowThreatMultiplier: [0.7, 1], holyThreatMultiplier: [0.7, 1], manaBurnCastReduction: [0, 1], targetMana: [0, 1000000], targetDeathInterval: [0, 1800], targetDeaths: [0, 1000], nextTargetDistance: [0, 100], spiritTapChance: [0, 1], spiritTapSpellPower: [0, 1000], starshardsEnabled: [0, 1], arcaneSpellPower: [0, 20000], arcaneHitChance: [0, 1], arcaneDamageMultiplier: [0, 10], arcaneCostMultiplier: [0, 2], arcaneThreatMultiplier: [0.7, 1], clipMindFlayEnabled: [0, 1], manaPoolingEnabled: [0, 1] };
  for (const [key, [min, max]] of Object.entries(bounds)) {
    if (typeof config[key] !== 'number' || !Number.isFinite(config[key]) || config[key] < min || config[key] > max) throw new Error(`Invalid Priest ${key}.`);
  }
  for (const key of ['iterations', 'seed', 'swpTicks', 'mindFlayEnabled', 'innerFocusEnabled', 'penanceEnabled', 'holyNovaEnabled', 'plagueEnabled', 'starshardsEnabled', 'powerInfusionEnabled', ...Object.keys(RACIAL_CONFIG).filter(key=>RACIAL_CONFIG[key]==='u32'), 'targetDeaths', 'clipMindFlayEnabled', 'manaPoolingEnabled']) if (!Number.isInteger(config[key])) throw new Error(`Priest ${key} must be an integer.`);
  for(const key of ['incomingAttackInterval','allyAttackInterval'])if(config[key]>0&&config[key]<0.001)throw new Error(`Priest ${key} must be at least one millisecond.`);
  if (config.targetDeathInterval>0 && config.targetDeathInterval<0.001) throw new Error('Priest target death interval must be at least one millisecond.');
  if (config.targetDeaths>0 && config.targetDeathInterval<=0) throw new Error('Priest target deaths require a positive interval.');
  if (typeof config.rotation !== 'string' || !Object.hasOwn(ROTATIONS, config.rotation)) throw new Error('Unsupported Priest core rotation.');
  return config;
}
export function packConfig(input) {
  const c = validate(input), buffer = new ArrayBuffer(CONFIG_WORDS * 4);
  const uints = new Uint32Array(buffer), floats = new Float32Array(buffer);
  const values = { ...c, wandIntervalMs: Math.round(c.wandInterval*1000), targetDeathIntervalMs: Math.round(c.targetDeathInterval*1000), durationMs: Math.round(c.duration * 1000), mindBlastCooldownMs: Math.round(c.mindBlastCooldown * 1000), rotation: ROTATIONS[c.rotation] };
  Object.entries(CONFIG).forEach(([key, type], index) => { (type === 'u32' ? uints : floats)[index] = values[key]; });
  return uints;
}
export function packMultiConfig(configs) {
  if (!Array.isArray(configs) || !configs.length) throw new Error('Priest batch requires configs.');
  const stride = Math.ceil(CONFIG_WORDS / 4) * 4, words = new Uint32Array(stride * configs.length);
  configs.forEach((config, index) => words.set(packConfig(config), index * stride));
  return { words, width: stride / 4, height: configs.length };
}
export function decodeState(words, detailed = true) {
  const schema = detailed ? STATE : FAST_STATE;
  if (!(words instanceof Uint32Array) || words.length < Object.keys(schema).length) throw new Error('Incomplete Priest state words.');
  const floats = new Float32Array(words.buffer, words.byteOffset, words.length);
  const state = { detailed };
  Object.entries(schema).forEach(([key, type], index) => { state[key] = (type === 'f32' ? floats : words)[index]; });
  if (state.done !== STATUS.complete || Object.entries(schema).some(([key,type]) => type === 'f32' && !Number.isFinite(state[key]))) throw new Error(`Incomplete Priest fight (status ${state.done}).`);
  return state;
}
