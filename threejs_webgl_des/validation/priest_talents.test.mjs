import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { TALENT_TREES } from '../src/priest/talent_data.js';
import { validateTalentAllocation, describeTalentEffects, resolveShadowTalentBuild } from '../src/priest/talents.js';
const data=JSON.parse(readFileSync(new URL('../data/priest_preview.json',import.meta.url)));
const allocationFor=preset=>Object.fromEntries(Object.entries(data.trees).map(([tree,nodes])=>[tree.toLowerCase(),Object.fromEntries(nodes.map(node=>[node.key,preset.ranks[node.key]||0]))]));
const shadow=allocationFor(data.presets.find(p=>p.id==='shadow_standard'));
const defer=describeTalentEffects(shadow).filter(effect=>effect.status==='pending').map(effect=>effect.key);

test('All 53 Forever talents retain rank bounds, row gates and prerequisites; CPU preview presets are legal',()=> {
  const local=JSON.parse(readFileSync(new URL('../../data/forever/priest_talents.json',import.meta.url)));
  assert.equal(Object.values(TALENT_TREES).flat().length,53);
  for(const [tree,nodes] of Object.entries(local.trees))for(const node of nodes) {
    const rule=TALENT_TREES[tree.toLowerCase()].find(n=>n.name===node.name);
    assert.equal(rule.max,node.ranks.length);assert.equal(rule.row,node.row);assert.equal(rule.requiredPoints,node.requiredPoints);
    const keys=new Map(TALENT_TREES[tree.toLowerCase()].map(n=>[n.name,n.key]));
    assert.deepEqual(rule.requires,node.requires.map(req=>({key:keys.get(nodes.find(n=>n.id===req.id).name),rank:req.qty})));
  }
  for(const preset of data.presets)assert.doesNotThrow(()=>validateTalentAllocation(allocationFor(preset)));
  assert.throws(()=>validateTalentAllocation({shadow:{shadowform:1}}),/row requirement/);
  assert.throws(()=>validateTalentAllocation({shadow:{shadow_focus:6}}),/rank/);
  assert.throws(()=>validateTalentAllocation({shadow:{shadow_focus:1.5}}),/rank/);
  assert.throws(()=>validateTalentAllocation({shadow:{fake:1}}),/Unknown/);
  const invalid=structuredClone(shadow);invalid.shadow.vampiric_embrace=0;
  assert.throws(()=>validateTalentAllocation(invalid),/prerequisite/);
  const overflow=structuredClone(shadow);overflow.discipline.wand_specialization=1;
  assert.throws(()=>validateTalentAllocation(overflow),/51/);
});

test('Forever Shadow preset resolves the actual rank effects and cannot silently ignore unfinished mechanics',()=> {
  assert.doesNotThrow(()=>resolveShadowTalentBuild({talents:shadow}));
  const input={talents:shadow,core:{hitChance:0.83},defer};
  const before=structuredClone(input);const build=resolveShadowTalentBuild(input);
  assert.deepEqual(input,before);
  const c=build.config;
  assert.equal(c.hitChance,0.88);assert.equal(c.swpTicks,8);assert.equal(c.mindBlastCooldown,5.5);
  assert.equal(c.mindFlayEnabled,1);assert.equal(c.flayDamageMultiplier,1.2);
  assert.equal(c.instantDamageMultiplier,1.05);assert.equal(c.costMultiplier,0.5);assert.equal(c.critMultiplier,2);
  assert.equal(c.innerFocusEnabled,1);assert.equal(c.castingRegenRatio,0.5);
  assert.ok(Math.abs(c.damageMultiplier-1.21)<1e-12);
  assert.equal(build.effects.length,Object.values(shadow).flatMap(tree=>Object.values(tree)).filter(Boolean).length);
  assert.ok(build.effects.some(effect=>effect.key==='spirit_tap'&&effect.status==='implemented'));
  assert.throws(()=>resolveShadowTalentBuild({core:{instantDamageMultiplier:1.05}}),/derived/);
});

test('Mental Agility uses Forever 3/7/10% and only instant spells; untalented Mind Flay is gated',()=> {
  for(const [rank,multiplier] of [[0,1],[1,0.97],[2,0.93],[3,0.9]]) {
    const talents={discipline:{power_in_light:5,twin_disciplines:5,mental_agility:rank}};
    const c=resolveShadowTalentBuild({talents}).config;
    assert.ok(Math.abs(c.instantCostMultiplier-multiplier)<1e-12);assert.equal(c.costMultiplier,1);
    assert.equal(c.mindFlayEnabled,0);assert.equal(c.critMultiplier,1.5);
  }
});

test('Meditation uses Forever 17/33/50%, with learned Inner Focus gated and derived fields protected',()=> {
  for(const [rank,ratio] of [[0,0],[1,0.17],[2,0.33],[3,0.5]]) {
    const build=resolveShadowTalentBuild({talents:{discipline:{power_in_light:5,twin_disciplines:5,inner_focus:1,meditation:rank}}});
    assert.equal(build.config.castingRegenRatio,ratio);assert.equal(build.config.innerFocusEnabled,1);
    assert.ok(build.effects.filter(e=>e.key==='meditation'||e.key==='inner_focus').every(e=>e.status==='implemented'));
  }
  assert.equal(resolveShadowTalentBuild().config.innerFocusEnabled,0);
  assert.throws(()=>resolveShadowTalentBuild({core:{innerFocusEnabled:1}}),/derived/);
  assert.throws(()=>resolveShadowTalentBuild({core:{castingRegenRatio:0.5}}),/derived/);
});

test('Ten additional talents resolve real damage-core effects, stats and rank tables',()=> {
  const talents=allocationFor(data.presets.find(p=>p.id==='holy_core'));
  const core={spellPower:600,holySpellPower:500,maxMana:4456,critChance:0.1,hitChance:0.83,spiritManaPerTick:100,rotation:'holy'};
  const stats={intellect:200,spirit:105};
  const build=resolveShadowTalentBuild({talents,core,stats});
  const c=build.config;
  assert.equal(c.maxMana,4906);assert.equal(c.spellPower,608.4);assert.equal(c.holySpellPower,508.4);
  assert.ok(Math.abs(c.critChance-(0.1+30/5920))<1e-12);
  assert.ok(Math.abs(c.spiritManaPerTick-100*Math.sqrt(1.15))<1e-12);
  assert.equal(c.holyHitChance,1);assert.equal(c.holyCritChance,c.critChance+0.05);
  assert.equal(c.holyCastReduction,0.5);assert.equal(c.powerInLightBonus,0.1);
  assert.equal(c.penanceEnabled,1);assert.equal(c.penanceCostMultiplier,0.85);assert.equal(c.holyNovaEnabled,1);
  for (const key of ['mental_strength','spiritual_guidance','holy_precision','holy_specialization','divine_fury','power_in_light','penance','improved_healing','holy_nova'])assert.equal(build.effects.find(e=>e.key===key).status,'implemented');
  assert.throws(()=>resolveShadowTalentBuild({talents,core}),/requires baseline intellect/);
  for (const [rank,ratio] of [[0,0],[1,.01],[2,.03],[3,.05],[4,.06],[5,.08]]) {
    const t=structuredClone(talents);t.holy.spiritual_guidance=rank;
    assert.equal(resolveShadowTalentBuild({talents:t,core,stats}).config.spellPower,600+105*ratio);
  }
  for (let rank=0;rank<=5;rank++) {
    const t=structuredClone(talents);t.discipline.mental_strength=rank;
    assert.equal(resolveShadowTalentBuild({talents:t,core,stats}).config.maxMana,4456+200*rank*.03*15);
  }
  const t=structuredClone(shadow);t.shadow.devouring_contagion=0;
  for(let rank=0;rank<=2;rank++) {
    t.shadow.early_demise=rank;
    const b=resolveShadowTalentBuild({talents:t});assert.equal(b.config.deathExecuteCritBonus,rank*.15);
    if(rank)assert.equal(b.effects.find(e=>e.key==='early_demise').status,'implemented');
  }
  for (const key of ['holyHitChance','holyCritChance','holyDamageMultiplier','holyCastReduction','penanceEnabled','holyNovaEnabled','deathExecuteCritBonus'])assert.throws(()=>resolveShadowTalentBuild({core:{[key]:1}}),/derived/);
});

test('Remaining damage talents use Forever ranks and protect derived fields',()=> {
  const pi=allocationFor(data.presets.find(p=>p.id==='power_infusion_dps'));
  const build=resolveShadowTalentBuild({talents:pi,stats:{intellect:200,spirit:100}});
  assert.equal(build.config.powerInfusionEnabled,1);assert.equal(build.config.manaBurnCastReduction,1);
  assert.equal(build.config.holyDamageMultiplier,1.05);assert.equal(build.config.searingNovaChance,.1);
  assert.equal(build.config.holyRangeMultiplier,1.2);
  for(const rank of [0,1,2]) {
    const t=structuredClone(pi);t.holy.searing_light=rank;
    const c=resolveShadowTalentBuild({talents:t,stats:{intellect:200,spirit:100}}).config;
    assert.equal(c.holyDamageMultiplier,[1,1.02,1.05][rank]);assert.equal(c.searingNovaChance,rank*.05);
    const c2=resolveShadowTalentBuild({talents:{discipline:{wand_specialization:rank}}}).config;
    assert.equal(c2.wandDamageMultiplier,[1,1.13,1.25][rank]);
    const t2=structuredClone(shadow);t2.shadow.devouring_contagion=rank;
    const c3=resolveShadowTalentBuild({talents:t2}).config;
    assert.equal(c3.plagueCostMultiplier,1-rank*.25);assert.equal(c3.plagueSpreadRadius,rank*5);
  }
  assert.equal(resolveShadowTalentBuild({talents:shadow}).config.spiritTapChance,1);
  assert.equal(resolveShadowTalentBuild({talents:shadow}).config.shadowRangeMultiplier,1.2);
  for(const key of ['plagueCostMultiplier','plagueSpreadRadius','powerInfusionEnabled','searingNovaChance','shadowRangeMultiplier','holyRangeMultiplier','flayRangeBonus','wandDamageMultiplier','shadowThreatMultiplier','holyThreatMultiplier','manaBurnCastReduction','spiritTapChance','spiritTapSpellPower'])assert.throws(()=>resolveShadowTalentBuild({core:{[key]:1}}),/derived/);
});
