import { TALENT_TREES, TALENT_RULESET } from './talent_data.js';
import { validate } from './model.js';

const IMPLEMENTED = new Set(['shadow_focus','darkness','shadowform','twin_disciplines','mental_agility',
  'improved_shadow_word_pain','improved_mind_blast','mind_flay','improved_mind_flay','shadow_weaving','inner_focus','meditation', 'mental_strength','spiritual_guidance','early_demise','holy_precision','holy_specialization','divine_fury','power_in_light','penance','improved_healing','holy_nova','searing_light','power_infusion','devouring_contagion','shadow_reach','holy_reach','wand_specialization','shadow_affinity','silent_resolve','improved_mana_burn','spirit_tap','spiritual_healing']);
const PENDING = new Set();
const scopedReason = () => 'Healing, defense or control effect requires the corresponding encounter mechanic.';

export function validateTalentAllocation(input = {}) {
  if (!input || typeof input !== 'object' || Array.isArray(input)) throw new Error('Priest talents must be an object.');
  for (const tree of Object.keys(input)) if (!Object.hasOwn(TALENT_TREES,tree)) throw new Error(`Unknown Priest talent tree: ${tree}`);
  const allocation = {};
  let total = 0;
  for (const [tree,nodes] of Object.entries(TALENT_TREES)) {
    const ranks = Object.hasOwn(input,tree) ? input[tree] : {};
    if (!ranks || typeof ranks !== 'object' || Array.isArray(ranks)) throw new Error(`Invalid Priest ${tree} talents.`);
    for (const key of Object.keys(ranks)) if (!nodes.some(node=>node.key===key)) throw new Error(`Unknown Priest talent: ${key}`);
    allocation[tree] = {};
    for (const node of nodes) {
      const rank = Object.hasOwn(ranks,node.key) ? ranks[node.key] : 0;
      if (!Number.isInteger(rank) || rank < 0 || rank > node.max) throw new Error(`Invalid Priest talent rank: ${node.key}`);
      allocation[tree][node.key] = rank; total += rank;
    }
  }
  if (total > 51) throw new Error('Priest talents exceed 51 points.');
  for (const [tree,nodes] of Object.entries(TALENT_TREES)) for (const node of nodes) {
    if (!allocation[tree][node.key]) continue;
    const preceding = nodes.filter(other=>other.row<node.row).reduce((sum,other)=>sum+allocation[tree][other.key],0);
    if (preceding < node.requiredPoints) throw new Error(`Priest talent row requirement: ${node.key}`);
    for (const prerequisite of node.requires) if (allocation[tree][prerequisite.key] < prerequisite.rank) throw new Error(`Priest talent prerequisite: ${node.key}`);
  }
  return allocation;
}

export function describeTalentEffects(input) {
  const allocation = validateTalentAllocation(input);
  return Object.entries(TALENT_TREES).flatMap(([tree,nodes])=>nodes.filter(node=>allocation[tree][node.key]).map(node=>({
    tree, key:node.key, rank:allocation[tree][node.key],
    status:IMPLEMENTED.has(node.key)?'implemented':PENDING.has(node.key)?'pending':'outside-scope',
    reason:IMPLEMENTED.has(node.key)?(node.key==='silent_resolve'?'Holy threat reduction is applied; incoming Stun/Fear/Silence durations are not modeled.':'WoW Forever damage-core talent effect is applied when its spell or encounter trigger is present.'):PENDING.has(node.key)?'Requires an additional combat mechanic or spell.':scopedReason(node.key)
  })));
}

// Baseline core inputs are untalented resolved stats. No hidden preset/default
// repairs; allocating an unfinished mechanic requires an explicit fixture scope.
export function resolveShadowTalentBuild({ core = {}, talents = {}, stats = {}, defer = [] } = {}) {
  if (!Array.isArray(defer) || defer.some(key=>!PENDING.has(key)) || new Set(defer).size!==defer.length) throw new Error('Invalid deferred Priest talent scope.');
  for (const key of ['instantDamageMultiplier','flayDamageMultiplier','instantCostMultiplier','mindFlayEnabled','swpTicks','mindBlastCooldown','weavingChance','castingRegenRatio','innerFocusEnabled','holyBaseCostMultiplier','holyDamageMultiplier','holyHitChance','holyCritChance','holyCastReduction','holyCostMultiplier','powerInLightBonus','penanceEnabled','penanceCostMultiplier','holyNovaEnabled','deathExecuteCritBonus','plagueCostMultiplier','plagueSpreadRadius','powerInfusionEnabled','searingNovaChance','shadowRangeMultiplier','holyRangeMultiplier','flayRangeBonus','wandDamageMultiplier','wandHitChance','shadowThreatMultiplier','holyThreatMultiplier','manaBurnCastReduction','spiritTapChance','spiritTapSpellPower']) {
    if (Object.hasOwn(core,key)) throw new Error(`Priest baseline cannot supply derived talent field: ${key}`);
  }
  const baseline = validate(core), allocation = validateTalentAllocation(talents), report = describeTalentEffects(allocation);
  const pending = report.filter(effect=>effect.status==='pending'&&!defer.includes(effect.key));
  if (pending.length) throw new Error(`Unimplemented Priest talent effects: ${pending.map(effect=>effect.key).join(', ')}`);
  const disc=allocation.discipline, shadow=allocation.shadow, holy=allocation.holy;
  if (!stats || typeof stats !== 'object' || Array.isArray(stats)) throw new Error('Priest stats must be an object.');
  for (const key of Object.keys(stats)) if (!['intellect','spirit'].includes(key)) throw new Error(`Unsupported Priest stat: ${key}`);
  for (const key of ['intellect','spirit']) if (stats[key] !== undefined && (!Number.isFinite(stats[key]) || stats[key]<0 || stats[key]>10000)) throw new Error(`Invalid Priest ${key}.`);
  if (disc.mental_strength && stats.intellect === undefined) throw new Error('Mental Strength requires baseline intellect.');
  if (holy.spiritual_guidance && stats.spirit === undefined) throw new Error('Spiritual Guidance requires baseline spirit.');
  const intellectBonus=(stats.intellect??0)*disc.mental_strength*0.03;
  const guidance=(stats.spirit??0)*[0,0.01,0.03,0.05,0.06,0.08][holy.spiritual_guidance];
  const form=shadow.shadowform===1;
  const config=validate({ ...baseline,
    spellPower:baseline.spellPower+guidance,
    holySpellPower:baseline.holySpellPower+guidance,
    arcaneSpellPower:baseline.arcaneSpellPower+guidance,
    arcaneHitChance:baseline.arcaneHitChance,
    arcaneDamageMultiplier:baseline.arcaneDamageMultiplier,
    arcaneCostMultiplier:baseline.arcaneCostMultiplier,
    arcaneThreatMultiplier:1,
    healingMultiplier:[1,1.05,1.1,1.15][holy.spiritual_healing],
    healingPower:baseline.healingPower+guidance,
    shadowformEnabled:shadow.shadowform,
    holyDamageMultiplier:baseline.damageMultiplier*[1,1.02,1.05][holy.searing_light],
    holyBaseCostMultiplier:baseline.costMultiplier,
    maxMana:(baseline.maxMana+intellectBonus*15)*baseline.manaCapacityMultiplier,
    critChance:Math.min(1,baseline.critChance+intellectBonus/5920),
    spiritManaPerTick:stats.intellect>0?baseline.spiritManaPerTick*Math.sqrt(1+disc.mental_strength*0.03):baseline.spiritManaPerTick,
    holyHitChance:Math.min(1,baseline.hitChance+disc.holy_precision*0.06),
    holyCritChance:Math.min(1,baseline.critChance+intellectBonus/5920+holy.holy_specialization*0.01),
    holyCastReduction:holy.divine_fury*0.1,
    holyCostMultiplier:1-[0,0.03,0.07,0.10][disc.mental_agility],
    powerInLightBonus:disc.power_in_light*0.02,
    penanceEnabled:disc.penance,
    penanceCostMultiplier:1-holy.improved_healing*0.05,
    holyNovaEnabled:holy.holy_nova,
    deathExecuteCritBonus:shadow.early_demise*0.15,
    plagueCostMultiplier:1-shadow.devouring_contagion*0.25,
    plagueSpreadRadius:shadow.devouring_contagion*5,
    powerInfusionEnabled:disc.power_infusion,
    searingNovaChance:holy.searing_light*0.05,
    shadowRangeMultiplier:1+shadow.shadow_reach*0.1,
    holyRangeMultiplier:1+holy.holy_reach*0.1,
    flayRangeBonus:shadow.improved_mind_flay*5,
    wandDamageMultiplier:[1,1.13,1.25][disc.wand_specialization],
    wandHitChance:baseline.hitChance,
    shadowThreatMultiplier:1-shadow.shadow_affinity*0.1,
    holyThreatMultiplier:1-disc.silent_resolve*0.1,
    manaBurnCastReduction:disc.improved_mana_burn*0.5,
    spiritTapChance:shadow.spirit_tap*0.2,
    spiritTapSpellPower:guidance,
    hitChance:Math.min(1,baseline.hitChance+shadow.shadow_focus*0.01),
    damageMultiplier:baseline.damageMultiplier*(1+shadow.darkness*0.02)*(form?1.10:1),
    critMultiplier:form?2:baseline.critMultiplier,
    costMultiplier:baseline.costMultiplier*(form?0.5:1),
    instantDamageMultiplier:1+disc.twin_disciplines*0.01,
    instantCostMultiplier:1-[0,0.03,0.07,0.10][disc.mental_agility],
    flayDamageMultiplier:1+shadow.improved_mind_flay*0.10,
    mindFlayEnabled:shadow.mind_flay,
    weavingChance:[0,0.33,0.67,1][shadow.shadow_weaving],
    castingRegenRatio:[0,0.17,0.33,0.50][disc.meditation],
    innerFocusEnabled:disc.inner_focus,
    swpTicks:6+shadow.improved_shadow_word_pain,
    mindBlastCooldown:8-shadow.improved_mind_blast*0.5 });
  return { ruleset:TALENT_RULESET, config, allocation, effects:report };
}
export function buildFightConfig(input) { return resolveShadowTalentBuild(input).config; }
