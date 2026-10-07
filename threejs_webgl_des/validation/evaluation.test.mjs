import test from 'node:test';
import assert from 'node:assert/strict';
import { exportLogicalBuild } from '../src/contracts/build_io.js';
import { resolveWarlockImport } from '../src/classes/warlock_import.js';
import { createEvaluator, validateEvaluationRequest, mapEvaluationResults } from '../src/contracts/evaluation.js';
async function request() {
 const {candidate}=await exportLogicalBuild({stats:{},aplText:'Shadow Bolt'});
 return {schemaVersion:1,requestId:'test',candidates:[{...candidate,candidateId:'a'},{...candidate,candidateId:'b'}],settings:{iterations:2,seedSchedule:{kind:'existing-warlock-v1',seed:0},mode:'fast',objective:'mean-dps',batchSize:16}};
}
test('common evaluator preserves settings, result order, native fitness, and input data',async()=>{
 const input=await request(),saved=structuredClone(input);
 const evaluate=createEvaluator({resolveCandidate:resolveWarlockImport,runBatch:async(configs,options)=>{
  assert.equal(configs[0].seed,0);assert.equal(configs[1].iterations,2);
  assert.equal(options.detailedResults,false);assert.equal(options.batchSize,16);
  return {results:configs.map((config,i)=>({config,states:[{done:1,total:100},{done:1,total:200}],summary:{mean:10+i,sd:2}}))};
 }});
 const result=await evaluate(input);
 assert.deepEqual(result.results.map(r=>r.candidateId),['a','b']);
 assert.deepEqual(result.results.map(r=>r.objective.value),[10,11]);
 assert.equal(result.results[0].uncertainty.value,2/Math.sqrt(2));
 assert.equal(result.results[0].detail,null);
 assert.deepEqual(input,saved);
});
test('incomplete/nonfinite results never receive successful fitness',async()=>{
 const input=await request();
 const result=mapEvaluationResults(input,{results:[{states:[{done:1,total:100}],summary:{mean:123}},
  {states:[{done:1,total:100},{done:0,total:200}],summary:{mean:Infinity}}]});
 for(const r of result){assert.equal(r.status,'incomplete');assert.equal(r.objective.value,null);assert.equal(r.detail,null);}
 assert.throws(()=>mapEvaluationResults(input,{results:[]}));
});
test('request rejects mixed classes, duplicate IDs, invalid settings, and unknown schedules',async()=>{
 const input=await request();
 for(const edit of [{iterations:0},{seedSchedule:{kind:'other',seed:42}},{seedSchedule:{kind:'existing-warlock-v1',seed:-1}},{mode:'other'},{objective:'healing'},{batchSize:0},{typo:true}])assert.throws(()=>validateEvaluationRequest({...input,settings:{...input.settings,...edit}}));
 assert.throws(()=>validateEvaluationRequest({...input,candidates:[input.candidates[0],input.candidates[0]]}));
 assert.throws(()=>validateEvaluationRequest({...input,candidates:[input.candidates[0],{...input.candidates[1],classId:'priest'}]}));
});
test('runner errors and cancellation reject without invented candidate scores',async()=>{
 const input=await request();const error=Object.assign(new Error('cancelled'),{name:'AbortError'});
 const evaluate=createEvaluator({resolveCandidate:resolveWarlockImport,runBatch:async()=>{throw error;}});
 await assert.rejects(evaluate(input),e=>e===error);
});
