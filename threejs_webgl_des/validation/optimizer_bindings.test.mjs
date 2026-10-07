import { getTalentFlagsFromRanks } from '../src/talents.js';
import test from 'node:test';
import assert from 'node:assert/strict';
import { getClassModule } from '../src/classes/registry.js';
import { createAPLOptimizer } from '../src/search/apl_genetic_optimizer.js';
import { runAPLGeneticSynthesis, createDefaultIndividual } from '../src/apl_genetic_optimizer.js';

test('optimizer factories retain independent runner and class-rule bindings',async()=>{
 const original=getClassModule('warlock');
 function bind(value) {
  let runs=0,ruleCalls=0;
  const classModule={...original,search:{...original.search,apl:{...original.search.apl,
   availableActions:flags=>{ruleCalls++;return original.search.apl.availableActions(flags);}}}};
  const optimizer=createAPLOptimizer({classModule,simulation:{runMultiSimulation:async(configs,options)=>{
   runs++;return{results:configs.map(config=>({config,summary:{mean:value,sd:0},states:Array.from({length:options.iterations},()=>({done:1,total:value*config.duration}))}))};
  }}});
  return {optimizer,get runs(){return runs;},get ruleCalls(){return ruleCalls;}};
 }
 const first=bind(100),second=bind(200);
 const options={populationSize:2,generations:1,screeningSims:1,finalSims:1,selectedActionIds:['bolt','tap'],seedPresets:true};
 const [a,b]=await Promise.all([first.optimizer.runAPLGeneticSynthesis({talentFlags:getTalentFlagsFromRanks({})},options),second.optimizer.runAPLGeneticSynthesis({talentFlags:getTalentFlagsFromRanks({})},options)]);
 assert.equal(a.bestCandidate.meanDps,100);assert.equal(b.bestCandidate.meanDps,200);
 assert.equal(first.runs,3);assert.equal(second.runs,3);
 assert.equal(first.ruleCalls,1);assert.equal(second.ruleCalls,1);
});
test('public search wrapper rejects explicit unsupported identities before simulation loading',async()=>{
 for(const classId of ['priest',null,undefined]) await assert.rejects(runAPLGeneticSynthesis({}, {}, {classId}));
 assert.equal(typeof createDefaultIndividual,'function');
 assert.ok(createDefaultIndividual().rules.length);
});
