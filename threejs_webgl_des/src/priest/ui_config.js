import { PRIEST_RACES } from './racials.js';
import { packPriestAPL } from './policy.js';
import { resolveShadowTalentBuild } from './talents.js';

// Human, level 60, one level-63 boss, direct stats; crit input is gear/buff bonus.
// Buff/equipment/action adapters are not implicitly applied here.
export function resolvePriestUIBuild({ stats, sim, preset, trees, target = {} }) {
  const bounds={spellPower:[0,10000],shadowPower:[0,10000],holyPower:[0,10000],hit:[0,100],crit:[0,100],intellect:[0,5000],stamina:[0,5000],spirit:[0,5000],mp5:[0,10000]};
  if (!stats || !sim || !preset || !trees) throw new Error('Priest build inputs are missing.');
  for(const [key,[min,max]] of Object.entries(bounds)) if(typeof stats[key]!=='number'||!Number.isFinite(stats[key])||stats[key]<min||stats[key]>max) throw new Error(`Invalid Priest ${key}.`);
  if(target.resistance!==undefined&&target.resistance!==0) throw new Error('Priest resistance handling is not available yet.');
  if(target.level!==undefined&&target.level!==63) throw new Error('Priest core requires a level-63 target.');
  const talents=Object.fromEntries(Object.entries(trees).map(([tree,nodes])=>[tree.toLowerCase(),Object.fromEntries(nodes.map(node=>[node.key,preset.ranks[node.key]||0]))]));
  const race=preset.race??'HUMAN';
  if (!PRIEST_RACES.includes(race)) throw new Error('Unsupported Priest race.');
  const spirit=stats.spirit*(race==='HUMAN'?1.05:1);
  const build=resolveShadowTalentBuild({talents,stats:{intellect:stats.intellect,spirit},core:{
    ...(preset.apl?{aplEnabled:1,...packPriestAPL(preset.apl)}:{}),
    race:PRIEST_RACES.indexOf(race),racialsEnabled:1,plagueEnabled:1,
    maxHealth:1404+stats.stamina*10,spirit,healingPower:stats.spellPower+stats.holyPower,manaCapacityMultiplier:race==='GNOME'?1.05:1,
    targetIsHumanoid:+(target.isHumanoid??false),targetIsBeast:+(target.isBeast??false),targetControlImmune:+(target.controlImmune??true),weaponIsMace:+(target.weaponIsMace??false),
    incomingAttackInterval:target.incomingAttackInterval??0,incomingAttackType:target.incomingAttackType??1,incomingDamage:target.incomingDamage??0,
    targetMana:target.mana??0,enemyHealingPerTick:target.enemyHealingPerTick??0,
    allyAttackInterval:target.allyAttackInterval??0,allyIncomingDamage:target.allyIncomingDamage??0,allyDistance:target.allyDistance??0,allyWeakenedSoul:target.allyWeakenedSoul??0,starshardsEnabled:race==='NIGHT_ELF'?1:0,arcaneSpellPower:stats.spellPower,arcaneHitChance:Math.min(1,0.83+stats.hit/100),targetDistance:target.distance??0,
    spellPower:stats.spellPower+stats.shadowPower,holySpellPower:stats.spellPower+stats.holyPower,maxMana:1456+stats.intellect*15,
    manaPerTick:stats.mp5*0.4,spiritManaPerTick:spirit>0?5*(0.001+Math.sqrt(stats.intellect)*spirit*0.009327):0,
    hitChance:Math.min(1,0.83+stats.hit/100),critChance:Math.min(1,(stats.crit+1.24+stats.intellect/59.2)/100),
    duration:sim.duration,iterations:sim.iterations,seed:sim.seed??42,rotation:preset.rotation??'shadow'
  }});
  return {...build,race,summary:{shadowSP:build.config.spellPower,holySP:build.config.holySpellPower,
    maxMana:build.config.maxMana,maxHealth:1404+stats.stamina*10,hit:build.config.hitChance*100,
    crit:Math.min(1,build.config.critChance+(race==='DWARF'&&target.weaponIsMace?0.01:0))*100,mp5:stats.mp5,intellect:stats.intellect*(1+build.allocation.discipline.mental_strength*0.03),stamina:stats.stamina,
    shadowMultiplier:build.config.damageMultiplier}};
}
