// Shared priority order and condition descriptions for execution and presentation.
// DoT thresholds are policy heuristics, not changes to spell coefficients.
export const DOT_MIN_TICKS = Object.freeze({ pain: 2, plague: 3, holyFire: 2 });
const rule=(spell,text,extra='true')=>Object.freeze({spell,text,extra});
const starshards=rule(11,'starshards, if Night Elf & cooldown_ready & remaining >= 6s');
const chastise=rule(12,'chastise, if Dwarf & target_is_humanoid & cooldown_ready & range <= 20yd');
const plague=rule(8,'devouring_plague, if learned & cooldown_ready & remaining >= 9s');
const nova=rule(7,'holy_nova, if learned & free_proc');
const fire=rule(5,'holy_fire, if dot_expired & remaining >= cast_time + 4s');
const penance=rule(6,'penance, if learned & cooldown_ready');
const pain=rule(0,'shadow_word_pain, if dot_expired & remaining >= 6s');
const blast=rule(2,'mind_blast, if cooldown_ready & cast_finishes');
const death=rule(3,'shadow_word_death, if cooldown_ready (execute crit bonus at health <= 20%)');
const flay=rule(1,'mind_flay, if learned; clip after tick 2 for a ready higher-priority spell');
const smite=rule(4,'smite, if cast_finishes');
export const POLICY_RULES=Object.freeze({
  shadow:[starshards,chastise,plague,pain,blast,death,flay],
  holy:[starshards,chastise,nova,fire,penance,smite],
  mixed:[starshards,chastise,plague,nova,fire,penance,pain,blast,death,flay,smite],
  idle:[],
  'chastise-only':[chastise],
  'starshards-only':[rule(11,'starshards')],
  'pain-only':[rule(0,'shadow_word_pain')],
  'flay-only':[rule(1,'mind_flay')],
  'blast-only':[rule(2,'mind_blast')],
  'death-only':[rule(3,'shadow_word_death')],
  'smite-only':[rule(4,'smite')],
  'holy-fire-only':[rule(5,'holy_fire')],
  'penance-only':[rule(6,'penance')],
  'nova-only':[rule(7,'holy_nova','true')],
  'plague-only':[rule(8,'devouring_plague')],
  'wand-only':[rule(9,'wand')],
  'mana-burn-only':[rule(10,'mana_burn')],
  'holy-nova':[rule(7,'holy_nova, if free_proc','freeNova'),fire,rule(7,'holy_nova')],
});
const RACIAL_POLICY_TEXT={
  HUMAN:['divine_grace, if ally_health < 50% & ally_in_range & not_shadowform & cooldown_ready','feedback, if incoming_spells & enemy_has_mana & cooldown_ready'],
  UNDEAD:['dark_sacrifice, if mana_deficit >= 1600 + Spirit & health > 1600 & cooldown_ready','touch_of_weakness, if incoming_melee & buff_consumed','touch_of_the_grave, passive; 10% chance per damaging cast, never on DoT ticks'],
  NIGHT_ELF:['elunes_light, if cooldown_ready (off GCD; +10% crit for 15s)','elunes_grace, if incoming_melee_or_ranged & cooldown_ready'],
  DWARF:['desperate_prayer, if health < 50% & not_shadowform & cooldown_ready'],
  GNOME:['eureka, if cooldown_ready (off GCD; 3 charges within 15s; direct spells/channels only)','contingency_plan, if ally_takes_damage & ally_in_range & not_shadowform & cooldown_ready','confounding_flash, if health < 50% & incoming_attacks & range <= 8yd & target_not_immune'],
  TROLL:['berserking, if cooldown_ready (off GCD; +10% haste for 10s)','shadowguard, if incoming_attacks & charges_expired','hex_of_weakness, if incoming_melee_or_enemy_healing & debuff_expired']
};
export function formatPriestPolicy({rotation='shadow',race='HUMAN',apl=null}={}){
  if(apl)return [...(RACIAL_POLICY_TEXT[race]??[]),...validatePriestAPL(apl).map(r=>`${POLICY_RULES.mixed.find(rule=>rule.spell===r.spell).text}; ${r.enabled?`health <= ${r.healthMax}%, mana >= ${r.manaMin}%`:'never_use'}`)].join('\n');
  const rules=POLICY_RULES[rotation];
  if(!rules)throw new Error('Unsupported Priest policy.');
  const lines=[...(RACIAL_POLICY_TEXT[race]??[])];
  if(['shadow','holy','mixed'].includes(rotation))lines.push('power_infusion, if learned & cooldown_ready (off GCD)');
  for(const r of rules){if(r.spell===11&&race!=='NIGHT_ELF'||r.spell===12&&race!=='DWARF')continue;lines.push(r.text);}
  lines.push('inner_focus, if learned & cooldown_ready, before the selected spell');
  if(['shadow','holy','mixed'].includes(rotation))lines.push('pool_mana, if a more mana-efficient higher-priority spell becomes affordable within 6s');
  return lines.join('\n');
}

// Each rule carries priority, target-health ceiling, mana floor and Never Use.
export function createPriestAPL(rotation='shadow') {
  return POLICY_RULES[rotation].map(({spell})=>({spell,healthMax:100,manaMin:0,enabled:true}));
}
export function validatePriestAPL(apl) {
  if(!Array.isArray(apl)||apl.length<1||apl.length>11)throw new Error('Invalid Priest APL.');
  const seen=new Set();
  for(const r of apl){
    if(!r||!Number.isInteger(r.spell)||r.spell<0||(r.spell>8&&r.spell!==11&&r.spell!==12)||seen.has(r.spell)||typeof r.enabled!=='boolean'||
      !Number.isInteger(r.healthMax)||r.healthMax<0||r.healthMax>100||!Number.isInteger(r.manaMin)||r.manaMin<0||r.manaMin>100)throw new Error('Invalid Priest APL rule.');
    seen.add(r.spell);
  }
  return apl;
}
export function packPriestAPL(apl) {
  validatePriestAPL(apl);
  return Object.fromEntries(Array.from({length:11},(_,i)=>{const r=apl[i];return [`apl${i}`,r?(r.spell+1)|(r.healthMax<<4)|(r.manaMin<<11)|(Number(r.enabled)<<18):0];}));
}
