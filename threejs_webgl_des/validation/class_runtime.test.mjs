import test from 'node:test';
import assert from 'node:assert/strict';
import { createClassRuntimeResolver } from '../src/classes/runtime.js';
import { requireClassEvaluation, requireClassCapability } from '../src/classes/capabilities.js';
import { getClassModule } from '../src/classes/registry.js';
const caps=getClassModule('warlock').capabilities;
function simulation(id) {return {id,runSimulation(){},runMultiSimulation(){},validate(){},packConfig(){},shaders:{fast:'fast',detailed:'detailed'}};}
test('runtime caches per class and retains direct simulation references across concurrent requests',async()=>{
 const a=simulation('a'),b=simulation('b');let loads=0;
 const modules={a:{id:'a',capabilities:caps,loadSimulation:async()=>{loads++;return a;}},b:{id:'b',capabilities:caps,loadSimulation:async()=>{loads++;return b;}}};
 const resolve=createClassRuntimeResolver({getClass:id=>{if(!modules[id])throw new Error('unsupported');return modules[id];}});
 const [first,second]=await Promise.all([resolve('a'),resolve('a')]);
 assert.equal(loads,1);assert.equal(first,second);assert.equal(first.simulation.runMultiSimulation,a.runMultiSimulation);
 assert.equal((await resolve('b')).simulation,b);assert.equal(loads,2);
 await assert.rejects(resolve('unknown'));
});
test('unsupported capabilities reject before loading and mismatched contracts can retry',async()=>{
 let loads=0;
 const module={id:'a',capabilities:caps,loadSimulation:async()=>{loads++;return simulation(loads===1?'wrong':'a');}};
 const resolve=createClassRuntimeResolver({getClass:()=>module});
 await assert.rejects(resolve('a'),/identity mismatch/);
 assert.equal((await resolve('a')).simulation.id,'a');assert.equal(loads,2);
 const disabled=createClassRuntimeResolver({getClass:()=>({...module,capabilities:{...caps,features:{}}})});
 await assert.rejects(disabled('a'),/does not support/);assert.equal(loads,2);
});
test('capabilities explicitly gate modes, objectives, schedules, and features',()=>{
 const module=getClassModule('warlock');
 assert.doesNotThrow(()=>requireClassEvaluation(module,{mode:'fast',objective:'mean-dps',seedSchedule:{kind:'existing-warlock-v1'}}));
 for(const settings of [{mode:'other',objective:'mean-dps',seedSchedule:{kind:'existing-warlock-v1'}},{mode:'fast',objective:'healing',seedSchedule:{kind:'existing-warlock-v1'}},{mode:'fast',objective:'mean-dps',seedSchedule:{kind:'other'}}])assert.throws(()=>requireClassEvaluation(module,settings));
 assert.throws(()=>requireClassCapability(module,'healing'));
});
