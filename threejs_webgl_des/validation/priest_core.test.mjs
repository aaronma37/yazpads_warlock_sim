import { packPriestAPL } from '../src/priest/policy.js';
import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, writeFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { CONFIG, CONFIG_WORDS, FAST_STATE, STATE, STATE_WORDS, COMPACT_STRIPES, validate, packConfig, packMultiConfig, decodeState } from '../src/priest/model.js';
import { FRAGMENT, FAST_FRAGMENT, VERTEX } from '../src/priest/kernel.js';
import { resolveShadowTalentBuild, describeTalentEffects } from '../src/priest/talents.js';
import { readFileSync } from 'node:fs';
import { summarize } from '../src/priest/results.js';

function unpack(outputs, lane, width, detailed) {
  const stripes = detailed ? COMPACT_STRIPES : 1;
  const words = new Uint32Array(detailed ? STATE_WORDS : 8);
  for (let word=0;word<words.length;word++) {
    words[word] = outputs[Math.floor(word%16/4)][((Math.floor(lane/width)*stripes+Math.floor(word/16))*width+lane%width)*4+word%4];
  }
  return decodeState(words,detailed);
}

test('Priest resolved core validates before packing and rejects Warlock/logical fields', () => {
  assert.throws(() => validate({ petChoice:'imp' }), /Unsupported/);
  assert.throws(() => validate({ talents:{} }), /Unsupported/);
  for (const input of [{ iterations:1.1 }, {seed:-1}, {hitChance:95}, {maxMana:0}, {swpTicks:9}, {spellPower:NaN}, {rotation:'toString'}, {clipMindFlayEnabled:2}, {manaPoolingEnabled:.5}]) assert.throws(() => validate(input));
  const words = packConfig({ duration:1.5, mindBlastCooldown:5.5, spellPower:123 });
  assert.equal(words.length,CONFIG_WORDS); assert.equal(words[0],1500);
  const index = Object.keys(CONFIG).indexOf('mindBlastCooldownMs'); assert.equal(words[index],5500);
  assert.equal(new Float32Array(words.buffer)[1],123);
  const batch = packMultiConfig([{spellPower:123},{spellPower:456}]);
  assert.equal(batch.height,2); assert.equal(batch.words[0],180000);
  assert.equal(new Float32Array(batch.words.buffer)[batch.width*4+1],456);
});

test('Priest incomplete states cannot contribute to result summaries', () => {
  const words = new Uint32Array(STATE_WORDS);
  for (const status of [0,2,3]) { words[1]=status; assert.throws(() => decodeState(words), /Incomplete/); }
  words[1]=1;
  const state = decodeState(words);
  assert.equal(state.total,0); assert.equal(Object.keys(STATE).length,STATE_WORDS);
  assert.throws(() => summarize([{...state,done:3}],1), /Incomplete/);
  assert.equal(summarize([state],1).spells.length,24);
});

test('Both Priest shader variants compile as GLSL ES 3', t => {
  const compiler = spawnSync('glslangValidator',['--version'],{encoding:'utf8'});
  if (compiler.error?.code === 'ENOENT') { t.skip('glslangValidator unavailable'); return; }
  const directory = mkdtempSync(join(tmpdir(),'priest-glsl-'));
  try {
    for (const [name,shader] of [['detailed.frag',FRAGMENT],['fast.frag',FAST_FRAGMENT],['shader.vert',VERTEX]]) {
      const path = join(directory,name); writeFileSync(path,'#version 300 es\n'+shader);
      const result = spawnSync('glslangValidator',[path],{encoding:'utf8'});
      assert.equal(result.status,0,result.stdout+result.stderr);
    }
  } finally { rmSync(directory,{recursive:true,force:true}); }
});

test('Actual headless GLES: scheduled spell fixtures, fast/detailed parity, batching and failed-budget rejection', t => {
  const directory = mkdtempSync(join(tmpdir(),'priest-execution-'));
  const cases = [];
  const fixture = (config, seed=42, extra={}) => {
    const common = { configWords:Array.from(packConfig(config)), width:2,
      uniforms:{numConfigs:1,fightsPerConfig:2,seed,offset:0,count:2,gridWidth:2,eventBudget:65536}, ...extra };
    for (const mode of ['detailed','fast']) cases.push({...common,mode,height:mode==='detailed'?COMPACT_STRIPES:1});
  };
  fixture({duration:18,rotation:'pain-only',hitChance:1,critChance:0});
  fixture({duration:3,rotation:'flay-only',hitChance:1,critChance:0});
  fixture({duration:1.5,rotation:'blast-only',hitChance:1,critChance:1});
  fixture({duration:30,rotation:'shadow',hitChance:0.88,critChance:0.2,maxMana:5000});
  fixture({duration:10,rotation:'shadow',maxMana:100,manaPerTick:0});
  fixture({duration:3,rotation:'flay-only',hitChance:0});
  fixture({duration:10,rotation:'death-only',hitChance:1,critChance:0},0);
  const batch = packMultiConfig([{duration:18,rotation:'pain-only',hitChance:1},{duration:3,rotation:'flay-only',hitChance:1}]);
  const texture = {...batch,words:Array.from(batch.words)};
  const commonUniforms = {numConfigs:2,fightsPerConfig:2,seed:42,offset:0,count:4,gridWidth:2,eventBudget:65536};
  for (const mode of ['detailed','fast']) cases.push({mode,configWords:Array.from(packConfig({})),texture,width:2,height:mode==='detailed'?COMPACT_STRIPES*2:2,uniforms:commonUniforms});
  cases.push({mode:'fast',configWords:Array.from(packConfig({duration:18,rotation:'pain-only',hitChance:1})),width:1,height:1,uniforms:{...commonUniforms,numConfigs:1,offset:0,count:1,gridWidth:1,eventBudget:1}});
  cases.push({...cases[0], uniforms:{...cases[0].uniforms,offset:1,count:1}});
  const preview=JSON.parse(readFileSync(new URL('../data/priest_preview.json',import.meta.url)));
  const preset=preview.presets.find(p=>p.id==='shadow_standard');
  const talents=Object.fromEntries(Object.entries(preview.trees).map(([tree,nodes])=>[tree.toLowerCase(),Object.fromEntries(nodes.map(node=>[node.key,preset.ranks[node.key]||0]))]));
  talents.discipline.inner_focus=0;talents.discipline.meditation=0;
  talents.shadow.shadow_weaving=0; // Isolate the previously implemented static talent effects.
  const defer=describeTalentEffects(talents).filter(e=>e.status==='pending').map(e=>e.key);
  fixture(resolveShadowTalentBuild({core:{duration:24,rotation:'pain-only',hitChance:1,critChance:0},talents,defer}).config);
  fixture(resolveShadowTalentBuild({core:{duration:3,rotation:'flay-only',hitChance:1,critChance:0},talents,defer}).config);
  fixture(resolveShadowTalentBuild({core:{duration:7,rotation:'blast-only',hitChance:1,critChance:1},talents,defer}).config);
  fixture(resolveShadowTalentBuild({core:{duration:3,rotation:'flay-only'}}).config);
  fixture(resolveShadowTalentBuild({core:{duration:18,rotation:'pain-only',hitChance:1},talents:{discipline:{power_in_light:5,twin_disciplines:5,mental_agility:3}}}).config);
  fixture({duration:7,rotation:'blast-only',hitChance:1,critChance:1});
  fixture({duration:18,rotation:'pain-only',hitChance:1,critChance:0,weavingChance:1});
  fixture({duration:3,rotation:'flay-only',hitChance:1,critChance:0,weavingChance:1});
  fixture({duration:8,rotation:'pain-only',hitChance:1,maxMana:500,manaPerTick:0,spiritManaPerTick:100});
  fixture({duration:8,rotation:'pain-only',hitChance:1,maxMana:500,manaPerTick:0,spiritManaPerTick:100,castingRegenRatio:0.5});
  fixture({duration:31,rotation:'death-only',hitChance:1,critChance:0,maxMana:10000,weavingChance:1});
  fixture({duration:31,rotation:'death-only',hitChance:1,critChance:0,maxMana:10000,weavingChance:1e-20});
  fixture({duration:1.5,rotation:'blast-only',hitChance:1,critChance:0.75,innerFocusEnabled:1});
  fixture({duration:1,rotation:'death-only',hitChance:1,critChance:0.75,innerFocusEnabled:1});
  fixture({duration:6,rotation:'flay-only',hitChance:1,maxMana:206,manaPerTick:0,innerFocusEnabled:1});
  fixture({duration:21,rotation:'pain-only',hitChance:1,maxMana:471,manaPerTick:0,innerFocusEnabled:1});
  fixture({duration:3,rotation:'flay-only',hitChance:0,maxMana:1,manaPerTick:0,innerFocusEnabled:1});
  fixture({duration:183,rotation:'flay-only',hitChance:1,maxMana:1,manaPerTick:0,innerFocusEnabled:1});
  fixture({duration:186,rotation:'blast-only',hitChance:1,maxMana:100000,manaPerTick:0,spiritManaPerTick:10,innerFocusEnabled:1});
  for(const rank of [1,2,3])fixture(resolveShadowTalentBuild({core:{duration:8,rotation:'pain-only',hitChance:1,maxMana:500,manaPerTick:0,spiritManaPerTick:100},talents:{discipline:{power_in_light:5,twin_disciplines:5,meditation:rank}}}).config);
  const extraStart=cases.length;
  fixture({duration:2,rotation:'smite-only',holyHitChance:1,holyCritChance:0,holyCastReduction:.5,manaPerTick:0});
  fixture({duration:2,rotation:'smite-only',holyHitChance:1,holyCritChance:0,manaPerTick:0});
  fixture({duration:13.5,rotation:'holy-fire-only',holyHitChance:1,holyCritChance:0,manaPerTick:0});
  fixture({duration:2,rotation:'penance-only',penanceEnabled:1,holyHitChance:1,holyCritChance:0,penanceCostMultiplier:.85,manaPerTick:0});
  fixture({duration:2,rotation:'penance-only',holyHitChance:1,manaPerTick:0});
  fixture({duration:1,rotation:'nova-only',holyNovaEnabled:1,holyHitChance:1,holyCritChance:0,manaPerTick:0});
  fixture({duration:1,rotation:'nova-only',holyHitChance:1,manaPerTick:0});
  fixture({duration:18,rotation:'holy',penanceEnabled:1,holyHitChance:1,holyCritChance:0,maxMana:100000,manaPerTick:0});
  fixture({duration:18,rotation:'holy',penanceEnabled:1,holyHitChance:1,holyCritChance:0,powerInLightBonus:.1,maxMana:100000,manaPerTick:0});
  fixture({duration:1,rotation:'nova-only',holyNovaEnabled:1,holyHitChance:1,holyCritChance:0,costMultiplier:.5,damageMultiplier:1.21,weavingChance:1,manaPerTick:0});
  fixture({duration:80,rotation:'death-only',hitChance:1,critChance:.7,deathExecuteCritBonus:0,maxMana:100000,manaPerTick:0});
  fixture({duration:80,rotation:'death-only',hitChance:1,critChance:.7,deathExecuteCritBonus:.3,maxMana:100000,manaPerTick:0});
  const remainingStart=cases.length;
  fixture({duration:24,rotation:'plague-only',plagueEnabled:1,hitChance:1,manaPerTick:0});
  fixture({duration:24,rotation:'plague-only',plagueEnabled:1,plagueCostMultiplier:.5,hitChance:1,manaPerTick:0});
  fixture({duration:24,rotation:'plague-only',plagueEnabled:1,plagueSpreadRadius:5,targetDeathInterval:6,targetDeaths:2,nextTargetDistance:5,hitChance:1,manaPerTick:0});
  fixture({duration:24,rotation:'plague-only',plagueEnabled:1,plagueSpreadRadius:5,targetDeathInterval:6,targetDeaths:2,nextTargetDistance:6,hitChance:1,manaPerTick:0});
  fixture({duration:24,rotation:'plague-only',plagueEnabled:1,plagueSpreadRadius:10,targetDeathInterval:6,targetDeaths:2,nextTargetDistance:6,hitChance:1,manaPerTick:0});
  fixture({duration:3,rotation:'flay-only',hitChance:1,powerInfusionEnabled:1,manaPerTick:0});
  fixture({duration:18,rotation:'pain-only',hitChance:1,powerInfusionEnabled:1,manaPerTick:0});
  fixture({duration:183,rotation:'pain-only',hitChance:1,powerInfusionEnabled:1,maxMana:100000,manaPerTick:0});
  fixture({duration:3,rotation:'wand-only',wandHitChance:1,wandDamageMultiplier:1.25,powerInfusionEnabled:1,innerFocusEnabled:1,manaPerTick:0});
  fixture({duration:3,rotation:'wand-only',wandHitChance:1,wandDamageMultiplier:1.13,manaPerTick:0});
  fixture({duration:3,rotation:'pain-only',targetDistance:33,hitChance:1,manaPerTick:0});
  fixture({duration:3,rotation:'pain-only',targetDistance:33,shadowRangeMultiplier:1.1,hitChance:1,manaPerTick:0});
  fixture({duration:3,rotation:'flay-only',targetDistance:35,shadowRangeMultiplier:1.2,flayRangeBonus:10,hitChance:1,manaPerTick:0});
  fixture({duration:1,rotation:'nova-only',holyNovaEnabled:1,targetDistance:11,holyHitChance:1,holyCritChance:0,manaPerTick:0});
  fixture({duration:1,rotation:'nova-only',holyNovaEnabled:1,targetDistance:11,holyRangeMultiplier:1.1,holyHitChance:1,holyCritChance:0,manaPerTick:0});
  fixture({duration:3,rotation:'pain-only',shadowThreatMultiplier:.7,hitChance:1,manaPerTick:0});
  fixture({duration:2.5,rotation:'smite-only',holyThreatMultiplier:.7,holyHitChance:1,holyCritChance:0,manaPerTick:0});
  fixture({duration:2,rotation:'mana-burn-only',targetMana:1000,manaBurnCastReduction:1,hitChance:1,manaPerTick:0});
  fixture({duration:2,rotation:'mana-burn-only',targetMana:1000,hitChance:1,manaPerTick:0});
  fixture({duration:6,rotation:'mana-burn-only',targetMana:100,manaBurnCastReduction:1,hitChance:1,manaPerTick:0});
  fixture({duration:18,rotation:'plague-only',plagueEnabled:1,targetDeathInterval:6,targetDeaths:1,plagueSpreadRadius:5,spiritTapChance:1,spiritTapSpellPower:40,hitChance:1,spiritManaPerTick:100,manaPerTick:0});
  fixture({duration:300,rotation:'holy',holyNovaEnabled:1,searingNovaChance:.1,holyHitChance:1,holyCritChance:0,maxMana:1000000,manaPerTick:0});
  fixture({duration:24,rotation:'plague-only',hitChance:1,manaPerTick:0});
  fixture({duration:24,rotation:'plague-only',plagueEnabled:1,hitChance:0,manaPerTick:0});
  const policyStart=cases.length;
  fixture({duration:3,rotation:'shadow',hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0,deathExecuteCritBonus:.3});
  fixture({duration:3,rotation:'shadow',hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0,deathExecuteCritBonus:0});
  fixture({duration:12.5,rotation:'shadow',hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:12.5,rotation:'shadow',hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0,clipMindFlayEnabled:0});
  fixture({duration:12.5,rotation:'shadow',hitChance:1,critChance:0,maxMana:1685,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:26,rotation:'shadow',hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:5.999,rotation:'shadow',hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:6,rotation:'shadow',hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:8.999,rotation:'shadow',plagueEnabled:1,hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:9,rotation:'shadow',plagueEnabled:1,hitChance:1,critChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:6.999,rotation:'holy',holyCastReduction:.5,holyHitChance:1,holyCritChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:7,rotation:'holy',holyCastReduction:.5,holyHitChance:1,holyCritChance:0,maxMana:100000,manaPerTick:0,manaPoolingEnabled:0});
  fixture({duration:1.499,rotation:'blast-only',hitChance:1,maxMana:100000,manaPerTick:0});
  fixture({duration:60,rotation:'shadow',hitChance:1,critChance:0,maxMana:500,manaPerTick:150,swpTicks:8});
  fixture({duration:60,rotation:'shadow',hitChance:1,critChance:0,maxMana:500,manaPerTick:150,swpTicks:8,manaPoolingEnabled:0});
  fixture({duration:30,rotation:'shadow',hitChance:1,critChance:0,maxMana:300,manaPerTick:0});
  fixture({duration:30,rotation:'shadow',hitChance:1,critChance:0,maxMana:600,manaPerTick:0});
  fixture({duration:6,rotation:'shadow',hitChance:1,critChance:0,maxMana:300,manaPerTick:0,innerFocusEnabled:1});
  fixture({duration:30,rotation:'mixed',hitChance:1,holyHitChance:1,critChance:0,holyCritChance:0,maxMana:100000,manaPerTick:0,penanceEnabled:1});
  const starStart=cases.length;
  const star={rotation:'starshards-only',starshardsEnabled:1,arcaneHitChance:1,arcaneSpellPower:600,manaPerTick:0,critChance:1,holyCritChance:1};
  fixture({...star,duration:6});
  fixture({...star,duration:5});
  fixture({...star,duration:36});
  fixture({...star,duration:6,starshardsEnabled:0});
  fixture({...star,duration:6,arcaneHitChance:0});
  fixture({...star,duration:6,targetDistance:30.01});
  fixture({...star,duration:6,maxMana:349});
  fixture({...star,duration:6,damageMultiplier:2,holyDamageMultiplier:3,spellPower:2000,holySpellPower:3000});
  fixture({...star,duration:6,targetDeaths:1,targetDeathInterval:3});
  fixture({...star,duration:6,aplEnabled:1,...packPriestAPL([{spell:11,healthMax:100,manaMin:0,enabled:true}])});
  fixture({...star,duration:6,innerFocusEnabled:1});
  try {
    const path = join(directory,'manifest.json');
    writeFileSync(path,JSON.stringify({vertex:'#version 300 es\n'+VERTEX,detailed:'#version 300 es\n'+FRAGMENT,fast:'#version 300 es\n'+FAST_FRAGMENT,cases}));
    const execution = spawnSync('python3',[fileURLToPath(new URL('./headless_gles.py',import.meta.url)),path],{encoding:'utf8',timeout:180000,maxBuffer:4*1024*1024});
    if (/EGL .*unavailable|NoneType|cannot open shared object/.test(execution.stderr)) { t.skip(execution.stderr.trim()); return; }
    assert.equal(execution.status,0,execution.stderr || String(execution.error));
    const {outputs,renderer} = JSON.parse(execution.stdout); t.diagnostic(`Executed on ${renderer}`);
    const starState=i=>unpack(outputs[starStart+i*2],0,2,true);
    for(let i=0;i<11;i++){
      const fast=unpack(outputs[starStart+i*2+1],0,2,false),detailed=starState(i);
      for(const key of Object.keys(FAST_STATE))assert.equal(fast[key],detailed[key],`Starshards ${i} ${key}`);
    }
    const tick=300+600*0.167;
    assert.ok(Math.abs(starState(0).damage11-tick*6)<.01);assert.equal(starState(0).hits11,6);assert.equal(starState(0).casts11,1);assert.equal(starState(0).crits11,0);assert.equal(starState(0).manaSpent,350);
    assert.ok(Math.abs(starState(1).damage11-tick*5)<.01);assert.equal(starState(1).hits11,5);
    assert.ok(Math.abs(starState(2).damage11-tick*12)<.01);assert.equal(starState(2).casts11,2);assert.equal(starState(2).manaSpent,700);
    for(const i of [3,4,5,6])assert.equal(starState(i).damage11,0);
    assert.equal(starState(4).misses11,1);assert.equal(starState(4).manaSpent,350);
    assert.equal(starState(7).damage11,starState(0).damage11);
    assert.equal(starState(8).hits11,2);
    assert.equal(starState(9).damage11,starState(0).damage11);
    assert.equal(starState(10).manaSpent,0);assert.equal(starState(10).innerFocusUses,1);

    for (let i=0;i<16;i+=2) {
      const count = cases[i].uniforms.count;
      for (let lane=0;lane<count;lane++) {
        const detailed = unpack(outputs[i],lane,2,true), fast = unpack(outputs[i+1],lane,2,false);
        for (const key of ['total','done','events','rngCalls','mana','manaSpent','manaGained','highWater']) assert.equal(fast[key],detailed[key],`case ${i} lane ${lane} ${key}`);
      }
    }
    for(let i=18;i<cases.length;i+=2)for(let lane=0;lane<2;lane++) {
      const detailed=unpack(outputs[i],lane,2,true),fast=unpack(outputs[i+1],lane,2,false);
      for(const key of ['total','done','events','rngCalls','mana','manaSpent','manaGained','highWater']) assert.equal(fast[key],detailed[key],`talent case ${i} ${key}`);
    }
    const extra=index=>unpack(outputs[extraStart+index*2],0,2,true);
    assert.equal(extra(0).casts4,1);assert.equal(extra(0).manaSpent,280);assert.equal(extra(1).casts4,0);
    assert.equal(extra(2).hits5,6);assert.equal(extra(2).casts5,1);assert.equal(extra(2).manaSpent,255);
    assert.equal(extra(3).hits6,3);assert.ok(Math.abs(extra(3).total-820.5)<.01);assert.ok(Math.abs(extra(3).manaSpent-301.75)<.01);
    assert.equal(extra(4).total,0);assert.ok(extra(5).total>=227.5&&extra(5).total<=253.5);assert.equal(extra(5).manaSpent,750);assert.equal(extra(6).total,0);
    assert.ok(extra(8).damage4>extra(7).damage4);assert.ok(extra(8).damage6>extra(7).damage6);assert.equal(extra(8).damage5,extra(7).damage5);
    assert.equal(extra(9).total,extra(5).total);assert.equal(extra(9).manaSpent,750);assert.equal(extra(9).rngCalls,extra(5).rngCalls);
    assert.ok(extra(11).crits3>0);assert.ok(extra(11).crits3>=extra(10).crits3);assert.ok(extra(11).crits3-extra(10).crits3<=1);
    const remaining=index=>unpack(outputs[remainingStart+index*2],0,2,true);
    assert.ok(Math.abs(remaining(0).total-1248)<.01);assert.equal(remaining(0).hits8,8);assert.equal(remaining(0).manaSpent,985);
    assert.equal(remaining(1).total,remaining(0).total);assert.equal(remaining(1).manaSpent,492.5);
    assert.equal(remaining(2).total,remaining(0).total);assert.equal(remaining(2).plagueSpreads,2);assert.equal(remaining(2).casts8,1);
    assert.equal(remaining(3).hits8,1);assert.equal(remaining(3).plagueSpreads,0);
    assert.equal(remaining(4).total,remaining(0).total);assert.equal(remaining(4).plagueSpreads,2);
    assert.ok(Math.abs(remaining(5).total-640.05*1.2)<.01);assert.equal(remaining(5).powerInfusionUses,1);
    assert.ok(Math.abs(remaining(6).total-227*(4*1.2+2))<.01);assert.equal(remaining(7).powerInfusionUses,2);
    assert.equal(remaining(8).total,250);assert.equal(remaining(8).manaSpent,0);assert.equal(remaining(8).innerFocusUses,0);assert.equal(remaining(9).total,226);
    assert.equal(remaining(10).total,0);assert.equal(remaining(11).total,227);assert.ok(Math.abs(remaining(12).total-640.05)<.01);
    assert.equal(remaining(13).total,0);assert.ok(remaining(14).total>0);assert.equal(remaining(14).threat,0);
    assert.ok(Math.abs(remaining(15).threat-remaining(15).total*.7)<.01);assert.ok(Math.abs(remaining(16).threat-remaining(16).total*.7)<.01);
    assert.equal(remaining(17).casts10,1);assert.equal(remaining(17).manaSpent,270);assert.ok(remaining(17).targetManaBurned>=738&&remaining(17).targetManaBurned<=780);
    assert.equal(remaining(18).casts10,0);assert.equal(remaining(19).casts10,1);assert.equal(remaining(19).targetManaBurned,100);assert.equal(remaining(19).total,50);
    assert.equal(remaining(20).spiritTapProcs,1);assert.ok(Math.abs(remaining(20).total-(156+5*160))<.01);
    assert.equal(remaining(20).manaGained,985);assert.ok(remaining(21).freeNovaUses>0);assert.equal(remaining(21).casts7,remaining(21).freeNovaUses);
    assert.ok(Math.abs(remaining(21).manaSpent-(remaining(21).casts4*280+remaining(21).casts5*255))<.01);
    assert.equal(remaining(22).total,0);assert.equal(remaining(23).hits8,0);assert.equal(remaining(23).misses8,1);
    const policy=index=>unpack(outputs[policyStart+index*2],0,2,true);
    assert.equal(policy(0).casts3,1,'SW:D casts before execute');
    assert.equal(policy(0).total,policy(1).total,'execute bonus does not apply early');
    assert.equal(policy(2).flayClips,1);
    assert.equal(policy(2).hits1,6,'canceled channel ticks cannot land');
    assert.equal(policy(2).casts1,3,'replacement channel starts without stale ready events');
    assert.equal(policy(3).flayClips,0);
    assert.equal(policy(4).flayClips,0,'do not clip for an unaffordable spell');
    assert.equal(policy(5).flayClips,2);
    assert.equal(policy(6).casts0,0); assert.equal(policy(7).casts0,1);
    assert.equal(policy(8).casts8,0); assert.equal(policy(9).casts8,1);
    assert.equal(policy(10).casts5,0); assert.equal(policy(11).casts5,1);
    assert.equal(policy(12).manaSpent,0,'do not start a cast that cannot land');
    assert.equal(policy(13).manaPoolWaits,1);
    assert.equal(policy(13).manaPoolTimeMs,6000);
    assert.equal(policy(13).casts0,2,'reserve mana for the DoT refresh');
    assert.equal(policy(14).manaPoolWaits,0); assert.equal(policy(14).casts0,1);
    assert.ok(policy(13).total>policy(14).total,'efficient pooling improves this mana-limited fixture');
    assert.equal(policy(15).manaPoolWaits,0); assert.equal(policy(15).casts1,1);
    assert.equal(policy(16).manaPoolWaits,0); assert.equal(policy(16).casts0,1);
    assert.equal(policy(17).casts0,1,'Inner Focus allows an otherwise unaffordable DoT');
    assert.equal(policy(17).manaPoolWaits,0);
    assert.ok(policy(18).flayClips>0); assert.ok(policy(18).casts5>0);

    const talentedPain=unpack(outputs[18],0,2,true);
    assert.equal(talentedPain.hits0,8);assert.equal(talentedPain.casts0,1);assert.equal(talentedPain.manaSpent,235);
    assert.ok(Math.abs(talentedPain.total-227*1.1*1.1*1.05*8)<0.005);
    const talentedFlay=unpack(outputs[20],0,2,true);
    assert.equal(talentedFlay.hits1,3);assert.equal(talentedFlay.manaSpent,102.5);
    assert.ok(Math.abs(talentedFlay.total-640.05*1.1*1.1*1.2)<0.005);
    const talentedBlast=unpack(outputs[22],0,2,true);
    assert.equal(talentedBlast.casts2,2);assert.equal(talentedBlast.crits2,2);assert.equal(talentedBlast.manaSpent,350);
    assert.equal(unpack(outputs[24],0,2,true).total,0);assert.equal(unpack(outputs[24],0,2,true).casts1,0);
    assert.ok(Math.abs(unpack(outputs[26],0,2,true).manaSpent-423)<0.001);
    assert.equal(unpack(outputs[28],0,2,true).casts2,1);
    const dynamicPain=unpack(outputs[30],0,2,true);
    assert.ok(Math.abs(dynamicPain.total-227*6.3)<0.005);assert.equal(dynamicPain.rngCalls,7);
    const dynamicFlay=unpack(outputs[32],0,2,true);
    assert.ok(Math.abs(dynamicFlay.total-213.35*3.06)<0.005);assert.equal(dynamicFlay.rngCalls,4);
    assert.equal(unpack(outputs[34],0,2,true).manaGained,200);
    assert.equal(unpack(outputs[36],0,2,true).manaGained,300);
    assert.equal(unpack(outputs[38],0,2,true).total,unpack(outputs[40],0,2,true).total,'Weaving expires before the next damage at the exact 15-second boundary');
    const focusedBlast=unpack(outputs[42],0,2,true);
    assert.equal(focusedBlast.casts2,1);assert.equal(focusedBlast.crits2,1);assert.equal(focusedBlast.manaSpent,0);assert.equal(focusedBlast.innerFocusUses,1);
    const focusedDeath=unpack(outputs[44],0,2,true);
    assert.equal(focusedDeath.crits3,1);assert.equal(focusedDeath.manaSpent,0);assert.equal(focusedDeath.innerFocusUses,1);
    const focusedFlay=unpack(outputs[46],0,2,true);
    assert.equal(focusedFlay.casts1,2);assert.equal(focusedFlay.hits1,6);assert.equal(focusedFlay.manaSpent,205);assert.equal(focusedFlay.innerFocusUses,1);assert.equal(focusedFlay.crits1,0);
    const focusedPain=unpack(outputs[48],0,2,true);
    assert.equal(focusedPain.casts0,2);assert.equal(focusedPain.hits0,7);assert.equal(focusedPain.manaSpent,470);assert.equal(focusedPain.innerFocusUses,1);assert.equal(focusedPain.crits0,0);
    const missedFocus=unpack(outputs[50],0,2,true);
    assert.equal(missedFocus.casts1,1);assert.equal(missedFocus.misses1,1);assert.equal(missedFocus.innerFocusUses,1);
    const reusedFocus=unpack(outputs[52],0,2,true);
    assert.equal(reusedFocus.casts1,2);assert.equal(reusedFocus.hits1,6);assert.equal(reusedFocus.innerFocusUses,2);assert.equal(reusedFocus.manaSpent,0);
    const freeFSR=unpack(outputs[54],0,2,true);
    assert.equal(freeFSR.innerFocusUses,2);assert.equal(freeFSR.casts2,24);assert.equal(freeFSR.manaSpent,7700);assert.equal(freeFSR.manaGained,230);
    for(const [index,expected] of [[56,234],[58,266],[60,300]])assert.ok(Math.abs(unpack(outputs[index],0,2,true).manaGained-expected)<0.005);
    const pain = unpack(outputs[0],0,2,true);
    assert.equal(pain.total,1362); assert.equal(pain.casts0,1); assert.equal(pain.hits0,6); assert.equal(pain.rngCalls,1);
    const flay = unpack(outputs[2],0,2,true);
    assert.ok(Math.abs(flay.total-640.05)<0.001); assert.equal(flay.casts1,1); assert.equal(flay.hits1,3);
    const blast = unpack(outputs[4],0,2,true);
    assert.equal(blast.casts2,1); assert.equal(blast.crits2,1); assert.equal(blast.manaSpent,350);
    assert.ok(blast.total>= (472+500*1.5/3.5)*1.5 && blast.total <= (498+500*1.5/3.5)*1.5);
    const starved = unpack(outputs[8],0,2,true); assert.equal(starved.total,0); assert.equal(starved.manaSpent,0);
    const missed = unpack(outputs[10],0,2,true); assert.equal(missed.total,0); assert.equal(missed.casts1,2); assert.equal(missed.misses1,2);
    assert.equal(unpack(outputs[14],0,2,true).total,pain.total);
    assert.equal(unpack(outputs[14],2,2,true).total,flay.total);
    assert.throws(() => unpack(outputs[16],0,1,false), /status 3/);
    assert.deepEqual(unpack(outputs[17],0,2,true),unpack(outputs[0],1,2,true));
  } finally { rmSync(directory,{recursive:true,force:true}); }
});

test('Holy Nova has a learned base Holy Fire proc on direct damage and periodic damage, with Searing Light additive',t=>{
  const directory=mkdtempSync(join(tmpdir(),'priest-nova-audit-'));
  try{
    const iterations=256,width=32;
    const config={duration:5,rotation:'holy-fire-only',aplEnabled:1,...packPriestAPL([{spell:7,healthMax:100,manaMin:0,enabled:true},{spell:5,healthMax:100,manaMin:0,enabled:true}]),holyNovaEnabled:1,holyHitChance:1,holyCritChance:0,maxMana:1000000,manaPerTick:0,manaPoolingEnabled:0};
    const configs=[config,{...config,duration:60},{...config,duration:60,searingNovaChance:.1},{...config,duration:60,holyNovaEnabled:0,searingNovaChance:.1}];
    const texture=packMultiConfig(configs),count=configs.length*iterations;
    const cases=[true,false].map(detailed=>({mode:detailed?'detailed':'fast',width,height:Math.ceil(count/width)*(detailed?COMPACT_STRIPES:1),texture:{...texture,words:Array.from(texture.words)},configWords:Array.from(packConfig(configs[0])),uniforms:{numConfigs:configs.length,fightsPerConfig:iterations,seed:42,offset:0,count,gridWidth:width,eventBudget:65536}}));
    const path=join(directory,'manifest.json');writeFileSync(path,JSON.stringify({vertex:'#version 300 es\n'+VERTEX,detailed:'#version 300 es\n'+FRAGMENT,fast:'#version 300 es\n'+FAST_FRAGMENT,cases}));
    const execution=spawnSync('python3',[fileURLToPath(new URL('./headless_gles.py',import.meta.url)),path],{encoding:'utf8',timeout:60000,maxBuffer:8*1024*1024});
    assert.equal(execution.status,0,execution.stderr||String(execution.error));
    const {outputs}=JSON.parse(execution.stdout),uses=Array(configs.length).fill(0);
    for(let lane=0;lane<count;lane++){
      const detailed=unpack(outputs[0],lane,width,true),fast=unpack(outputs[1],lane,width,false);
      for(const key of Object.keys(FAST_STATE))assert.equal(detailed[key],fast[key]);
      uses[Math.floor(lane/iterations)]+=detailed.freeNovaUses;
      assert.equal(detailed.casts7,detailed.freeNovaUses);
    }
    // At 5 seconds Holy Fire has landed, but its first periodic tick has not.
    assert.ok(uses[0]>0&&uses[0]<iterations*.15,`direct-only base proc: ${uses[0]}`);
    assert.ok(uses[1]>uses[0],`periodic base procs: ${uses}`);
    assert.ok(uses[2]>uses[1],`Searing Light adds to the base proc: ${uses}`);
    assert.equal(uses[3],0,'unlearned Holy Nova cannot proc');
    t.diagnostic(`Free Nova uses across ${iterations} fights: ${uses.join(', ')}`);
  }finally{rmSync(directory,{recursive:true,force:true});}
});
