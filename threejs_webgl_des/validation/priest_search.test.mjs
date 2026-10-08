import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, mkdtempSync, writeFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { PriestTalentGraph as Graph, TALENT_DEFINITIONS as nodes, PRIEST_SEARCH_CLASS, candidateToConfig, candidateToPreset, normalizeSearchConfig, runPriestConstrainedSearch } from '../src/priest/search.js';
import { packPriestAPL } from '../src/priest/policy.js';
import { createBuildOptimizer } from '../src/search/genetic_optimizer.js';
import { validate, packConfig, packMultiConfig, decodeState } from '../src/priest/model.js';
import { VERTEX, FAST_FRAGMENT } from '../src/priest/kernel.js';
import { summarize } from '../src/priest/results.js';
const data=JSON.parse(readFileSync(new URL('../data/priest_preview.json',import.meta.url)));
const base={stats:{spellPower:500,shadowPower:100,holyPower:0,hit:12,crit:15,intellect:200,stamina:220,spirit:100,mp5:20},sim:{duration:12,iterations:2,seed:42},target:{level:63,resistance:0},trees:data.trees,race:'HUMAN'};
function rng(seed=12){let state=seed>>>0;return {nextU64(){state=(Math.imul(state,1664525)+1013904223)>>>0;return state;},nextDouble(){return this.nextU64()/4294967296;}};}
const index=key=>nodes.findIndex(node=>node.key===key);
const optimizer=createBuildOptimizer({classModule:PRIEST_SEARCH_CLASS});
const alloc=preset=>Object.fromEntries(Object.entries(data.trees).map(([tree,nodes])=>[tree.toLowerCase(),Object.fromEntries(nodes.map(node=>[node.key,preset.ranks[node.key]??0]))]));

test('Priest constrained graph preserves 51 points, row gates, prerequisites and maxed requirements through evolution',()=>{
  const random=rng();
  for(const required of [[],['shadowform'],['power_infusion'],['spiritual_guidance'],['shadowform','inner_focus']]){
    const config=normalizeSearchConfig({requiredTalents:required,forcedRace:'UNDEAD',forcedRotation:'mixed'},base);
    for(let n=0;n<30;n++){
      const a=optimizer.createRandomIndividual(random,config),b=optimizer.createRandomIndividual(random,config);
      const child=optimizer.crossoverIndividuals(a,b,random,config);optimizer.mutateIndividual(child,random,config);
      for(const ind of [a,b,child]){
        assert.ok(Graph.isValid(ind.talents));assert.equal(Graph.countTotalPoints(ind.talents),51);
        for(const key of required)assert.equal(ind.talents[index(key)],nodes[index(key)].max);
        assert.equal(ind.race,'UNDEAD');assert.equal(ind.rotation,'mixed');
      }
    }
  }
  assert.throws(()=>normalizeSearchConfig({requiredTalents:['shadowform','power_infusion']},base),/51 points/);
  assert.throws(()=>normalizeSearchConfig({requiredTalents:['fake']},base),/Unknown/);
  for(const invalid of [{populationSize:1},{generations:NaN},{screeningSims:0},{finalSims:1.5},{mutationRate:2},{forcedRace:'ORC'},{forcedRotation:'warlock'},{seed:-1}])assert.throws(()=>normalizeSearchConfig(invalid,base));
});

test('Each candidate resolves neutral direct stats once and stays single-target with duration-based execute',()=>{
  for(const preset of data.presets){
    const talents=Graph.fromTalentsObject(alloc(preset));
    const ind={talents,race:preset.race??'HUMAN',rotation:preset.rotation??'shadow'};
    const before=structuredClone(base),config=candidateToConfig(ind,base);
    assert.deepEqual(base,before);assert.doesNotThrow(()=>validate(config));
    assert.equal(config.targetDeaths,0);assert.equal(config.targetDeathInterval,0);assert.equal(config.targetDistance,0);
    assert.equal(config.plagueEnabled,1);
    const applied=candidateToPreset({...ind,name:'test',talents:Graph.toTalentsObject(talents)});
    assert.deepEqual(Graph.fromTalentsObject(alloc(applied)),talents);
    assert.equal(applied.rotation,ind.rotation);assert.equal(applied.race,ind.race);
  }
  const nightElf=candidateToConfig({talents:Graph.fromTalentsObject(alloc(data.presets[0])),race:'NIGHT_ELF',rotation:'mixed'},base);
  assert.equal(nightElf.starshardsEnabled,1);assert.equal(nightElf.plagueEnabled,1);
  assert.equal(normalizeSearchConfig({forcedRace:'NIGHT_ELF'},base).forcedRace,'NIGHT_ELF');
  const config=normalizeSearchConfig({optimizeRace:false}, {...base,race:'UNDEAD'});assert.equal(config.forcedRace,'UNDEAD');
});

function mockSimulation(calls,{incomplete=false}={}){return {id:'priest',runMultiSimulation:async(configs,options)=>{
  calls.push({configs,options});if(options.signal?.aborted)throw new DOMException('Stopped','AbortError');
  return {results:configs.map(config=>{
    const mean=config.damageMultiplier*50+config.holyDamageMultiplier*25+config.spellPower/10;
    return {config,summary:{mean,sd:2},states:Array.from({length:options.iterations},()=>({done:incomplete?0:1,total:mean*config.duration}))};
  })};
}};}
const small={populationSize:6,generations:2,screeningSims:2,finalSims:4,seed:123,seedPresets:true,presetsList:data.presets,forcedRace:'UNDEAD',requiredTalents:['inner_focus']};
test('Shared GPU optimizer batches all generations and reranks finalists; replay and candidate Apply retain identity',async()=>{
  const calls=[],states=[];
  const result=await runPriestConstrainedSearch(base,small,{simulation:mockSimulation(calls),onGeneration:state=>states.push(state)});
  assert.equal(calls.length,4);assert.deepEqual(calls.map(c=>c.options.iterations),[2,2,2,4]);
  assert.equal(states.length,3);assert.ok(result.candidates.length>1);assert.equal(result.bestCandidate,result.candidates[0]);
  assert.equal(new Set(result.candidates.map(c=>c.id)).size,result.candidates.length);
  for(let i=0;i<result.candidates.length;i++){
    const c=result.candidates[i];assert.ok(Graph.isValid(c.talentsVector));assert.equal(c.talentsVector[index('inner_focus')],1);assert.equal(c.race,'UNDEAD');assert.equal(c.states.length,4);
    if(i)assert.ok(result.candidates[i-1].mean_dps>=c.mean_dps);
    assert.equal(candidateToConfig(c.individual,base).plagueEnabled,1);
  }
  assert.equal(result.totalSimulations,calls.reduce((sum,c)=>sum+c.configs.length*c.options.iterations,0));
  const replay=await runPriestConstrainedSearch(base,small,{simulation:mockSimulation([])});
  assert.deepEqual(replay.candidates.map(c=>[c.id,c.mean_dps]),result.candidates.map(c=>[c.id,c.mean_dps]));
});

test('Invalid/incomplete GPU scores are rejected; stopping preserves latest discovered specs',async()=>{
  const calls=[];
  await assert.rejects(runPriestConstrainedSearch(base,small,{simulation:{id:'warlock'}}),/Priest batch/);
  await assert.rejects(runPriestConstrainedSearch(base,small,{simulation:mockSimulation(calls,{incomplete:true})}),/Incomplete/);
  const controller=new AbortController();
  const result=await runPriestConstrainedSearch(base,small,{simulation:mockSimulation([]),signal:controller.signal,onGeneration:()=>controller.abort()});
  assert.equal(result.stopped,true);assert.ok(result.bestCandidate);assert.equal(result.evolutionHistory.length,1);
  assert.ok(Graph.isValid(result.bestCandidate.talentsVector));
  const aborted=new AbortController();aborted.abort();
  const empty=await runPriestConstrainedSearch(base,small,{simulation:mockSimulation([]),signal:aborted.signal});assert.equal(empty.stopped,true);assert.equal(empty.bestCandidate,null);
});

test('Actual headless GLES executes the constrained optimizer on the Priest fast shader and applies the winning spec',async t=>{
  const directory=mkdtempSync(join(tmpdir(),'priest-search-gles-'));let batches=0;
  try{
    const simulation={id:'priest',runMultiSimulation:async(configs,options)=>{
      const iterations=options.iterations,width=8,count=configs.length*iterations,height=Math.ceil(count/width),batch=packMultiConfig(configs);
      const manifest={vertex:'#version 300 es\n'+VERTEX,fast:'#version 300 es\n'+FAST_FRAGMENT,detailed:'#version 300 es\n'+FAST_FRAGMENT,cases:[{
        mode:'fast',configWords:Array.from(packConfig(configs[0])),texture:{...batch,words:Array.from(batch.words)},width,height,
        uniforms:{numConfigs:configs.length,fightsPerConfig:iterations,seed:configs[0].seed,offset:0,count,gridWidth:width,eventBudget:65536}}]};
      const path=join(directory,'manifest.json');writeFileSync(path,JSON.stringify(manifest));
      const execution=spawnSync('python3',[fileURLToPath(new URL('./headless_gles.py',import.meta.url)),path],{encoding:'utf8',timeout:60000,maxBuffer:4*1024*1024});
      assert.equal(execution.status,0,execution.stderr||String(execution.error));
      const {outputs,renderer}=JSON.parse(execution.stdout);batches++;if(batches===1)t.diagnostic(renderer);
      const states=Array.from({length:configs.length},()=>[]);
      for(let lane=0;lane<count;lane++){
        const words=new Uint32Array(8);
        for(let word=0;word<8;word++)words[word]=outputs[0][Math.floor(word/4)][lane*4+word%4];
        states[Math.floor(lane/iterations)].push(decodeState(words,false));
      }
      return {results:configs.map((config,i)=>({config,states:states[i],summary:summarize(states[i],config.duration)}))};
    }};
    const result=await runPriestConstrainedSearch(base,{...small,populationSize:4,generations:1,screeningSims:2,finalSims:3},{simulation});
    assert.equal(batches,3);assert.ok(result.bestCandidate.mean_dps>0);assert.equal(result.bestCandidate.states.length,3);
    const winningConfig=candidateToConfig(result.bestCandidate.individual,base);
    const appliedPreset=candidateToPreset(result.bestCandidate);
    const appliedConfig=candidateToConfig({talents:Graph.fromTalentsObject(alloc(appliedPreset)),race:appliedPreset.race,rotation:appliedPreset.rotation,apl:appliedPreset.apl},base);
    assert.deepEqual(packConfig(winningConfig),packConfig(appliedConfig));
    assert.ok(result.candidates.every(c=>c.states.every(s=>s.done===1)));
    const rule={spell:0,healthMax:100,manaMin:0,enabled:true};
    const policyConfig={...winningConfig,rotation:'shadow',aplEnabled:1,manaPoolingEnabled:0,...packPriestAPL([rule])};
    const probe=await simulation.runMultiSimulation([policyConfig,{...policyConfig,...packPriestAPL([{...rule,enabled:false}])},{...policyConfig,...packPriestAPL([{...rule,healthMax:0}])}],{iterations:2});
    assert.ok(probe.results[0].summary.mean>0);
    assert.equal(probe.results[1].summary.mean,0);assert.equal(probe.results[2].summary.mean,0);
  }finally{rmSync(directory,{recursive:true,force:true});}
});


test('Priest joint evolution retains policy identity, evolves conditions and honors static mode', async()=>{
  const random=rng(77),config=normalizeSearchConfig({...small,forcedRotation:'mixed',mutationRate:1,lockConditions:false},base);
  assert.equal(config.aplMode,'coevolve');
  const a=optimizer.createRandomIndividual(random,config),b=optimizer.createRandomIndividual(random,config);
  const child=optimizer.crossoverIndividuals(a,b,random,config);
  for(let i=0;i<10;i++)optimizer.mutateIndividual(child,random,config);
  assert.equal(new Set(child.apl.map(r=>r.spell)).size,11);
  assert.ok(child.apl.some(r=>r.healthMax!==100||r.manaMin!==0||!r.enabled));
  assert.notEqual(PRIEST_SEARCH_CLASS.search.identity.getIndUniqueKey(child),PRIEST_SEARCH_CLASS.search.identity.getIndUniqueKey({...child,apl:null}));
  const configGPU=candidateToConfig(child,base);assert.equal(configGPU.aplEnabled,1);assert.doesNotThrow(()=>validate(configGPU));
  const calls=[],result=await runPriestConstrainedSearch(base,config,{simulation:mockSimulation(calls)});
  const winner=result.bestCandidate,preset=candidateToPreset(winner);
  assert.deepEqual(preset.apl,winner.apl);
  assert.deepEqual(packConfig(candidateToConfig({...winner.individual,apl:preset.apl},base)),packConfig(candidateToConfig(winner.individual,base)));
  assert.ok(calls.every(call=>call.configs.every(c=>c.aplEnabled===1)));
  const locked=optimizer.createRandomIndividual(random,{...config,lockConditions:true});optimizer.mutateIndividual(locked,random,{...config,lockConditions:true});
  assert.ok(locked.apl.every(r=>r.healthMax===100&&r.manaMin===0&&r.enabled));
  const fixed=optimizer.createRandomIndividual(random,{...config,aplMode:'static'});assert.equal(fixed.apl,null);assert.equal(candidateToConfig(fixed,base).aplEnabled,0);
});
