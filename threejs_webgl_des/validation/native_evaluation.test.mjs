import test from 'node:test';
import assert from 'node:assert/strict';
import { createNativeEvaluator } from '../src/contracts/native_evaluation.js';
import { packConfig } from '../src/model.js';
test('native adapter forwards exact configs/options and preserves result refs, seeds, packing, and fitness order',async()=>{
 const configs=[{iterations:2,seed:0},{iterations:2,seed:99}];
 const original=configs.map(packConfig);
 const options={iterations:2,detailedResults:false,batchSize:7};
 const batch={results:configs.map((config,i)=>({config,summary:{mean:10+i,sd:2},states:[{done:1,total:100},{done:1,total:200}]})),timing:{executeMs:3}};
 const evaluate=createNativeEvaluator({classId:'warlock',runBatch:async(c,o)=>{assert.equal(c,configs);assert.equal(o,options);return batch;}});
 const result=await evaluate(configs,options,{requestId:'finalists',candidateIds:['b','a']});
 assert.equal(result.results,batch.results);assert.equal(result.timing,batch.timing);
 assert.deepEqual(result.fitnessResults.map(r=>r.candidateId),['b','a']);
 assert.deepEqual(result.fitnessResults.map(r=>r.objective.value),[10,11]);
 assert.deepEqual(configs.map(packConfig),original);
 assert.deepEqual(configs.map(c=>c.seed),[0,99]);
});
test('native adapter rejects invalid identity and incomplete results before fitness selection',async()=>{
 let calls=0;
 const evaluate=createNativeEvaluator({classId:'warlock',runBatch:async()=>{calls++;return{results:[{states:[{done:0,total:10}],summary:{mean:999}}]};}});
 await assert.rejects(evaluate([{iterations:1}],{}, {requestId:'test',candidateIds:['a','a']}));
 assert.equal(calls,0);
 await assert.rejects(evaluate([{iterations:1}],{}, {requestId:'test',candidateIds:['a']}),/Incomplete/);
 assert.equal(calls,1);
});
test('native adapter preserves detailed data and runner rejection',async()=>{
 const error=new Error('GPU failure');
 const evaluate=createNativeEvaluator({classId:'warlock',runBatch:async()=>{throw error;}});
 await assert.rejects(evaluate([{iterations:1}],{}, {requestId:'test',candidateIds:['a']}),e=>e===error);
});

const { readFileSync } = await import('node:fs');
const vm = await import('node:vm');
const { getClassModule } = await import('../src/classes/registry.js');
const { getTalentFlagsFromRanks } = await import('../src/talents.js');
test('actual APL search all-stage adapter preserves native batch inputs and final search output',async()=>{
 const source=readFileSync(new URL('../src/search/apl_genetic_optimizer.js',import.meta.url),'utf8');
 const baseline=source.replace(/await evaluateBatch\(finalConfigs, \{ signal, iterations: finalSims, detailedResults: false \}, \{\s*requestId: 'apl-finalists', candidateIds: finalCandidates\.map\(\(_, i\) => `apl-finalist-\$\{i\}`\),\s*\}\)/,
  'await runMultiSimulation(finalConfigs, { signal, iterations: finalSims, detailedResults: false })')
  .replace(/evaluateBatch\(gen0Configs, \{ signal, iterations: screeningSims, detailedResults: false \}, \{[\s\S]*?\n  \}\)/, 'runMultiSimulation(gen0Configs, { signal, iterations: screeningSims, detailedResults: false })')
  .replace(/evaluateBatch\(activeBatch.configs, \{ signal, iterations: screeningSims, detailedResults: false \}, \{[\s\S]*?\n    \}\)/, 'runMultiSimulation(activeBatch.configs, { signal, iterations: screeningSims, detailedResults: false })')
  .replace('finalSimRes.fitnessResults[i].objective.value','finalSimRes.results[i].summary.mean')
  .replace('simRes0.fitnessResults[i].objective.value','res.summary.mean')
  .replace('offSimRes.fitnessResults[i].objective.value','res.summary.mean');
 async function run(code,lockConditions) {
  const batches=[];let adapterCalls=0;
  const runBatch=async(configs,options)=>{
   batches.push({configs:structuredClone(configs),options:{iterations:options.iterations,detailedResults:options.detailedResults}});
   return {results:configs.map(config=>{
    const mean=config.spellPower+config.aplRules.reduce((sum,r)=>sum+r.action+r.param1,0);
    return {config,summary:{mean,sd:0},states:Array.from({length:options.iterations},()=>({done:1,total:mean*config.duration}))};
   })};
  };
  const context={getClassModule,runMultiSimulation:runBatch,setTimeout,
   createNativeEvaluator:deps=>{
    const evaluate=createNativeEvaluator(deps);
    return (...args)=>{adapterCalls++;return evaluate(...args);};
   }};
  vm.createContext(context);
  const runnable=code.replace(/^import .*;\n/gm,'').replaceAll('export ','');
  vm.runInContext(runnable+'; this.search = createAPLOptimizer({classModule:getClassModule(),simulation:{runMultiSimulation}}).runAPLGeneticSynthesis;',context);
  const flags=getTalentFlagsFromRanks({affliction:{},demonology:{},destruction:{}});
  const output=await context.search({talentFlags:flags,intellect:200,spirit:100},
   {populationSize:8,generations:2,screeningSims:2,finalSims:3,seed:42,seedPresets:true,lockConditions});
  return {output:JSON.stringify(output),batches:JSON.stringify(batches),adapterCalls};
 }
 for(const locked of [true,false]) {
  const before=await run(baseline,locked),after=await run(source,locked);
  assert.equal(after.batches,before.batches);
  assert.equal(after.output,before.output);
  assert.equal(before.adapterCalls,0);assert.equal(after.adapterCalls,4);
 }
});

test('actual build search all-stage adapter preserves batches, rankings, and final output',async()=>{
 const source=readFileSync(new URL('../src/search/genetic_optimizer.js',import.meta.url),'utf8');
 const baseline=source.replace(/await evaluateBatch\(finalConfigs, \{ signal, iterations: finalSims, detailedResults: false \}, \{\s*requestId: 'build-finalists', candidateIds: finalCandidates\.map\(\(_, i\) => `build-finalist-\$\{i\}`\),\s*\}\)/,
  'await runMultiSimulation(finalConfigs, { signal, iterations: finalSims, detailedResults: false })')
  .replace(/evaluateBatch\(gen0Configs, \{ signal, iterations: screeningSims, detailedResults: false \}, \{[\s\S]*?\n  \}\)/, 'runMultiSimulation(gen0Configs, { signal, iterations: screeningSims, detailedResults: false })')
  .replace(/evaluateBatch\(activeBatch.configs, \{ signal, iterations: screeningSims, detailedResults: false \}, \{[\s\S]*?\n    \}\)/, 'runMultiSimulation(activeBatch.configs, { signal, iterations: screeningSims, detailedResults: false })')
  .replace('finalSimRes.fitnessResults[i].objective.value','finalSimRes.results[i].summary.mean')
  .replace('simRes0.fitnessResults[i].objective.value','res.summary.mean')
  .replace('offSimRes.fitnessResults[i].objective.value','res.summary.mean');
 async function run(code,aplMode,lockConditions) {
  const batches=[];let adapterCalls=0;
  const active=getClassModule(),apl=active.search.apl;
  const runBatch=async(configs,options)=>{
   batches.push({configs:structuredClone(configs),options:{iterations:options.iterations,detailedResults:options.detailedResults}});
   return{results:configs.map(config=>{
    const mean=config.spellPower+config.aplRules.reduce((sum,r)=>sum+r.action+(r.param1??r.param??0),0);
    return{config,summary:{mean,sd:0},states:Array.from({length:options.iterations},()=>({done:1,total:mean*config.duration}))};
   })};
  };
  const context={getClassModule,getTalentFlagsFromRanks,runMultiSimulation:runBatch,setTimeout,
   getPresetPetAndSac:()=>({pet:'none',sac:'none'}),
   createDefaultAPLIndividual:apl.createDefault,createRandomAPLIndividual:apl.createRandom,
   repairAPLIndividual:apl.repair,crossoverAPLIndividuals:apl.crossover,mutateAPLIndividual:apl.mutate,
   individualToBytecodeRules:apl.encodeIndividual,getAvailableActionsForSpec:apl.availableActions,
   getAPLUniqueKey:active.search.identity.getAPLUniqueKey,formatAPLName:active.search.results.formatAPLName,
   APL_SYNTHESIS_ACTIONS:apl.actions,
   createNativeEvaluator:deps=>{const evaluate=createNativeEvaluator(deps);return(...args)=>{adapterCalls++;return evaluate(...args);};}
  };
  vm.createContext(context);
  const runnable=code.replace(/^import[\s\S]*?;\n/gm,'').replaceAll('export ','');
  vm.runInContext(runnable+'; this.search = createBuildOptimizer({classModule:getClassModule(),simulation:{runMultiSimulation},dependencies:{resolveTalentFlags:getTalentFlagsFromRanks,resolvePetAndSac:getPresetPetAndSac}}).runConstrainedGeneticSearch;',context);
  const output=await context.search({intellect:200,spirit:100},
   {populationSize:8,generations:2,screeningSims:2,finalSims:3,seed:42,aplMode,lockConditions});
  return{output:JSON.stringify(output),batches:JSON.stringify(batches),adapterCalls};
 }
 for(const [mode,locked] of [['static',true],['evolve',true],['evolve',false]]) {
  const before=await run(baseline,mode,locked),after=await run(source,mode,locked);
  assert.equal(after.batches,before.batches);assert.equal(after.output,before.output);
  assert.equal(before.adapterCalls,0);assert.equal(after.adapterCalls,4);
 }
});
