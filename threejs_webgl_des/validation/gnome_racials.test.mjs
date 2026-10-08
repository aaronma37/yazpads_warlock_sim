import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { packConfig, CONFIG, STATE_WORDS, decodeStates, APL_ACTION } from '../src/model.js';
import { FRAGMENT, VERTEX } from '../src/kernel.js';
import { parseAPLText, compileAPLToBytecode, formatAPLToText, generateAPLForPreset } from '../src/apl.js';
import { isAPLRuleBlocking } from '../src/apl_rules.js';
import { resolveSavedBuild, exportLogicalBuild } from '../src/contracts/build_io.js';

test('Eureka appears in presets, parses, compiles, and round trips as an off-GCD action', async () => {
 const rules=parseAPLText('Eureka! if target_hp_pct <= 35\nShadow Bolt');
 assert.equal(rules[0].id,'eureka');
 assert.equal(compileAPLToBytecode(rules)[0].action,APL_ACTION.EUREKA);
 assert.equal(isAPLRuleBlocking(rules[0]),false);
 assert.equal(parseAPLText(formatAPLToText(rules))[0].id,'eureka');
 assert.equal(generateAPLForPreset('shadow').find(r=>r.id==='eureka').id,'eureka');
 const logical=await exportLogicalBuild({race:'GNOME',stats:{intellect:200},aplText:'Eureka!\nShadow Bolt'});
 const result=await resolveSavedBuild(logical);
 assert.equal(result.config.intellect,200);
 assert.equal(result.config.aplRules[0].action,APL_ACTION.EUREKA);
 const values=c=>{const w=packConfig(c),f=new Float32Array(w.buffer);return Object.fromEntries(Object.entries(CONFIG).map(([k,t],i)=>[k,t==='f32'?f[i]:w[i]]));};
 const human=values({race:'HUMAN',swordEquipped:false}),gnome=values({race:'GNOME'});
 assert.equal(human.crit,gnome.crit);
 assert.ok(Math.abs(gnome.maxMana/human.maxMana-1.05)<1e-6);
});

test('actual GLSL Eureka honors APL enablement, conditions, race, charges, cooldown and DoT boosts', t => {
 const cases=[],names=[];
 const add=(name,text,extra={})=>{
  names.push(name);
  cases.push({mode:'detailed',width:1,height:Math.ceil(STATE_WORDS/16),configWords:Array.from(packConfig({race:'GNOME',duration:4,distance:0,intellect:5000,instantCorruption:true,corrMultiplier:1,hit:100,nightfall:false,isb:false,partialResists:false,aplRules:compileAPLToBytecode(parseAPLText(text)),...extra})),uniforms:{numConfigs:1,fightsPerConfig:1,seed:42,offset:0,count:1,gridWidth:1,mode:1,eventBudget:65536}});
 };
 add('on','Eureka!\nCorruption if !target.has_debuff("Corruption")\nShadow Bolt');
 add('off','Eureka! if false\nCorruption if !target.has_debuff("Corruption")\nShadow Bolt',{racialPolicy:'cooldown'});
 add('condition','Eureka! if target_hp_pct < 1\nCorruption if !target.has_debuff("Corruption")\nShadow Bolt');
 add('human','Eureka!\nCorruption if !target.has_debuff("Corruption")\nShadow Bolt',{race:'HUMAN',swordEquipped:false});
 add('cooldown','Eureka!\nShadow Bolt',{duration:125});
 add('legacy','Shadow Bolt',{racialPolicy:'cooldown'});
 const dir=mkdtempSync(join(tmpdir(),'warlock-eureka-')),path=join(dir,'cases.json');
 writeFileSync(path,JSON.stringify({vertex:'#version 300 es\n'+VERTEX,detailed:'#version 300 es\n'+FRAGMENT,cases}));
 const run=spawnSync('python3',[fileURLToPath(new URL('./headless_gles.py',import.meta.url)),path],{encoding:'utf8',timeout:120000,maxBuffer:8*1024*1024});
 assert.equal(run.status,0,run.stderr||String(run.error));
 const {outputs,renderer}=JSON.parse(run.stdout);t.diagnostic(renderer);
 const result=Object.fromEntries(outputs.map((out,index)=>{
  const w=new Uint32Array(STATE_WORDS);
  for(let i=0;i<w.length;i++)w[i]=out[Math.floor(i%16/4)][Math.floor(i/16)*4+i%4];
  return [names[index],decodeStates(w.buffer,1)[0]];
 }));
 writeFileSync(join(dir,'results.json'),JSON.stringify(result,null,2));
 for(const state of Object.values(result))assert.equal(state.done,1);
 assert.equal(result.on.racialReady,120000000);
 assert.equal(result.off.racialReady,0);
 assert.equal(result.condition.racialReady,0);
 assert.equal(result.human.racialReady,0);
 assert.equal(result.legacy.racialReady,120000000);
 assert.equal(result.cooldown.racialReady,240000000);
 assert.equal(result.on.corrGen,1);
 assert.equal(result.on.corrGen,result.off.corrGen);
 assert.equal(result.on.events,result.off.events);
 assert.equal(result.on.eurekaCharges,2);
 assert.ok(result.off.total>0);
 assert.ok(Math.abs(result.on.total/result.off.total-1.1)<1e-5);
 assert.ok(result.on.spent<result.off.spent);
});

test('GPU regret replays Eureka before forcing the recorded spell', async t => {
 const { REGRET_FRAGMENT, REGRET_ACTIONS }=await import('../src/regret.js');
 const configWords=Array.from(packConfig({race:'GNOME',duration:4,distance:0,intellect:5000,corrMultiplier:1,hit:100,nightfall:false,isb:false,partialResists:false,aplRules:compileAPLToBytecode(parseAPLText('Eureka!\nCorruption if !target.has_debuff("Corruption")\nShadow Bolt'))}));
 // This test evaluates decision zero; substitute only the job texture lookup.
 const fragment=REGRET_FRAGMENT.replace(/uvec4 job=texelFetch\(regretJobTex,[^;]+;/,'uvec4 job=uvec4(0u);');
 const cases=[0,REGRET_ACTIONS.indexOf(APL_ACTION.CORRUPTION)].map(offset=>({mode:'regret',width:1,height:1,configWords,uniforms:{numConfigs:1,fightsPerConfig:0,seed:42,offset,count:1,gridWidth:1,mode:4,eventBudget:65536,regretSamples:1,continuationSeed:42}}));
 const path=join(mkdtempSync(join(tmpdir(),'warlock-eureka-regret-')),'cases.json');
 writeFileSync(path,JSON.stringify({vertex:'#version 300 es\n'+VERTEX,regret:'#version 300 es\n'+fragment,cases}));
 const run=spawnSync('python3',[fileURLToPath(new URL('./headless_gles.py',import.meta.url)),path],{encoding:'utf8',timeout:120000,maxBuffer:8*1024*1024});
 assert.equal(run.status,0,run.stderr||String(run.error));
 const {outputs,renderer}=JSON.parse(run.stdout);t.diagnostic(renderer);
 assert.equal(outputs[0][0][1],1);
 assert.equal(outputs[1][0][1],1);
 assert.equal(outputs[0][0][0],outputs[1][0][0]);
 assert.equal(outputs[0][1][3],outputs[1][1][3]);
});
