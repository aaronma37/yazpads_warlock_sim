import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, writeFileSync, existsSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { packConfig, STATE_WORDS, decodeStates, APL_ACTION } from '../src/model.js';
import { FRAGMENT, FAST_FRAGMENT, VERTEX } from '../src/kernel.js';
import { parseAPLText, compileAPLToBytecode, formatAPLToText, generateAPLForPreset, ACTION_INFO } from '../src/apl.js';
import { isAPLRuleBlocking, getRaceVisibleAPLRules } from '../src/apl_rules.js';
import { resolveSavedBuild, exportLogicalBuild, exportResolvedBuild } from '../src/contracts/build_io.js';

test('Blood Fury parses, round trips, and appears only on Orc spec APL displays', async () => {
 for(const name of ['Blood Fury','blood_fury','bloodFury']){
  const rules=parseAPLText(`${name} if target_hp_pct <= 35\nShadow Bolt`);
  assert.equal(rules[0].id,'bloodFury');
  assert.equal(compileAPLToBytecode(rules)[0].action,APL_ACTION.BLOOD_FURY);
  assert.equal(parseAPLText(formatAPLToText(rules))[0].id,'bloodFury');
  assert.equal(isAPLRuleBlocking(rules[0]),false);
 }
 assert.ok(existsSync(new URL('../assets/icons/'+ACTION_INFO.bloodFury.icon,import.meta.url)));
 const preset=generateAPLForPreset('shadow');
 for(const race of ['Human','Orc','Undead','Troll','Gnome']){
  const visible=getRaceVisibleAPLRules(preset,race);
  assert.equal(visible.some(r=>r.id==='bloodFury'),race==='Orc');
  assert.equal(visible.some(r=>r.id==='eureka'),race==='Gnome');
 }
 const input={race:'ORC',aplText:'Blood Fury\nShadow Bolt'};
 const original=await resolveSavedBuild(input);
 for(const exported of [await exportLogicalBuild(input),await exportResolvedBuild(input)]){
  const imported=await resolveSavedBuild(JSON.parse(JSON.stringify(exported)));
  assert.deepEqual(packConfig(imported.config),packConfig(original.config));
 }
});

const base={race:'ORC',duration:4,distance:0,intellect:5000,hit:100,nightfall:false,isb:false,partialResists:false};
const config=(text,extra={})=>({...base,aplRules:compileAPLToBytecode(parseAPLText(text)),...extra});
const uniforms={numConfigs:1,fightsPerConfig:1,seed:42,offset:0,count:1,gridWidth:1,mode:1,eventBudget:65536};

test('actual GLSL Blood Fury is off-GCD and respects race, enablement, conditions, expiry, cooldown and legacy APLs',t=>{
 const inputs=[
  ['on',config('Blood Fury\nShadow Bolt')],
  ['off',config('Blood Fury if false\nShadow Bolt',{racialPolicy:'cooldown'})],
  ['never',config('Blood Fury if target_hp_pct < 1\nShadow Bolt')],
  ['delayed',config('Blood Fury if target_hp_pct <= 35\nShadow Bolt',{duration:7})],
  ['delayed-off',config('Blood Fury if false\nShadow Bolt',{duration:7})],
  ['human',config('Blood Fury\nShadow Bolt',{race:'HUMAN',swordEquipped:false})],
  ['cooldown',config('Blood Fury\nShadow Bolt',{duration:125})],
  ['legacy',config('Shadow Bolt',{racialPolicy:'cooldown'})],
  ['expiry-on',config('Blood Fury\nShadow Bolt',{duration:20})],
  ['expiry-off',config('Blood Fury if false\nShadow Bolt',{duration:20})]
 ];
 const cases=inputs.map(([,c])=>({mode:'detailed',width:1,height:Math.ceil(STATE_WORDS/16),configWords:Array.from(packConfig(c)),uniforms}));
 cases.push({mode:'fast',width:1,height:1,configWords:Array.from(packConfig(inputs[0][1])),uniforms:{...uniforms,mode:0}});
 const path=join(mkdtempSync(join(tmpdir(),'warlock-blood-fury-')),'cases.json');
 writeFileSync(path,JSON.stringify({vertex:'#version 300 es\n'+VERTEX,detailed:'#version 300 es\n'+FRAGMENT,fast:'#version 300 es\n'+FAST_FRAGMENT,cases}));
 const run=spawnSync('python3',[fileURLToPath(new URL('./headless_gles.py',import.meta.url)),path],{encoding:'utf8',timeout:120000,maxBuffer:8*1024*1024});
 assert.equal(run.status,0,run.stderr||String(run.error));
 const {outputs,renderer}=JSON.parse(run.stdout);t.diagnostic(renderer);
 const result=Object.fromEntries(inputs.map(([name],index)=>{
  const out=outputs[index],w=new Uint32Array(STATE_WORDS);
  for(let i=0;i<w.length;i++)w[i]=out[Math.floor(i%16/4)][Math.floor(i/16)*4+i%4];
  return [name,decodeStates(w.buffer,1)[0]];
 }));
 for(const state of Object.values(result))assert.equal(state.done,1);
 assert.equal(result.on.racialReady,120000000);
 assert.equal(result.on.racialEnd,15000000);
 for(const name of ['off','never','human'])assert.equal(result[name].racialReady,0);
 assert.equal(result.delayed.racialReady,126000000);
 // At six seconds the off-GCD activation can precede the same-time impact.
 assert.ok(Math.abs(result.delayed.total-result['delayed-off'].total-500*0.1*(3/3.5)*2)<0.01);
 assert.equal(result.cooldown.racialReady,240000000);
 assert.equal(result.legacy.total,result.on.total);
 assert.equal(result.on.events,result.off.events);
 assert.equal(result.on.spent,result.off.spent);
 const delta=500*0.1*(3/3.5)*2;
 assert.ok(Math.abs(result.on.total-result.off.total-delta)<0.01);
 assert.equal(result['expiry-on'].racialEnd,15000000);
 assert.ok(Math.abs(result['expiry-on'].total-result['expiry-off'].total-delta*4)<0.02);
 const fast=outputs.at(-1);
 assert.equal(fast[0][1]&65535,1);
 assert.equal(new Float32Array(new Uint32Array([fast[0][0]]).buffer)[0],result.on.total);
});

test('GPU regret replays Blood Fury before forcing the recorded spell', async t => {
 const { REGRET_FRAGMENT, REGRET_ACTIONS }=await import('../src/regret.js');
 const configWords=Array.from(packConfig(config('Blood Fury\nShadow Bolt')));
 // Evaluate decision zero, substituting only the diagnostic job lookup.
 const fragment=REGRET_FRAGMENT.replace(/uvec4 job=texelFetch\(regretJobTex,[^;]+;/,'uvec4 job=uvec4(0u);');
 const cases=[0,REGRET_ACTIONS.indexOf(APL_ACTION.SHADOW_BOLT_FILLER)].map(offset=>({mode:'regret',width:1,height:1,configWords,uniforms:{...uniforms,mode:4,fightsPerConfig:0,offset,regretSamples:1,continuationSeed:42}}));
 const path=join(mkdtempSync(join(tmpdir(),'warlock-blood-fury-regret-')),'cases.json');
 writeFileSync(path,JSON.stringify({vertex:'#version 300 es\n'+VERTEX,regret:'#version 300 es\n'+fragment,cases}));
 const run=spawnSync('python3',[fileURLToPath(new URL('./headless_gles.py',import.meta.url)),path],{encoding:'utf8',timeout:120000,maxBuffer:8*1024*1024});
 assert.equal(run.status,0,run.stderr||String(run.error));
 const {outputs,renderer}=JSON.parse(run.stdout);t.diagnostic(renderer);
 assert.equal(outputs[0][0][1],1);
 assert.equal(outputs[1][0][1],1);
 assert.equal(outputs[0][0][0],outputs[1][0][0]);
 assert.equal(outputs[0][1][3],outputs[1][1][3]);
});
