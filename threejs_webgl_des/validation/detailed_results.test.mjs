import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
import * as model from '../src/model.js';
import { FRAGMENT, FAST_FRAGMENT } from '../src/kernel.js';
import { summarizeDetails, renderDetailedResults } from '../src/detailed_results.js';

const source=readFileSync(new URL('../src/engine.js',import.meta.url),'utf8')
 .replace(/^import .*?;\n/gm,'').replace(/^import \{[\s\S]*?from 'three';\n/m,'').replaceAll('export ', '');
const globals={...model,summarizeDetails,Float64Array,Float32Array,Uint32Array};
vm.runInNewContext(source+'\nglobalThis.decode=decodeBatch2D;',globals);

test('Detailed transport uses existing four stripes and decodes all uptime pairs across rows',()=>{
 assert.equal(model.COMPACT_STRIPES,4);
 const width=2,count=3,outputs=Array.from({length:4},()=>new Uint32Array(width*8*4));
 const put=(lane,word,value,float=false)=>{
  const att=Math.floor(word%16/4),index=((Math.floor(lane/width)*4+Math.floor(word/16))*width+lane%width)*4+word%4;
  (float?new Float32Array(outputs[att].buffer):outputs[att])[index]=value;
 };
 for(let lane=0;lane<count;lane++){
  put(lane,0,100,true);put(lane,1,1);
  for(let i=0;i<6;i++)put(lane,model.DETAIL_WORD_OFFSET+i,16384|(49151<<16));
  put(lane,62,1234,true);put(lane,63,321,true);
 }
 const states=[];globals.decode(outputs,width,count,{duration:30},0,states);
 for(const state of states){
  assert.equal(state.spent,1234);assert.equal(state.manaWasted,321);
  model.UPTIME_EFFECTS.forEach(([,key],i)=>assert.ok(Math.abs(state[`uptime_${key}`]-(i%2?.75:.25))<1/65535));
 }
 const summary=globals.summarize(states,30);
 assert.equal(summary.details.manaSpent,1234);
 assert.equal(summary.details.histogram.reduce((n,b)=>n+b.count,0),count);
 assert.equal(summary.details.uptimes.length,12);
});

test('Fast schema and generated shader exclude all detailed metrics',()=>{
 for(const key of Object.keys(model.DETAIL_STATE))assert.ok(!(key in model.FAST_STATE));
 assert.doesNotMatch(FAST_FRAGMENT,/uptime_|manaWasted/);
 assert.match(FRAGMENT,/min\(e.at,s.immEnd\)-s.now/);
 assert.match(FRAGMENT,/s.immTicks>0u&&s.immEnd>s.now/);
 const summary=globals.summarize([{total:100,detailed:false}],10);
 assert.equal(summary.details,undefined);
});

test('Histogram includes every sample at the boundaries and handles a constant distribution',()=>{
 const a=summarizeDetails({},4,10,[10,20,30,40]);
 assert.equal(a.histogram.reduce((n,b)=>n+b.count,0),4);
 assert.equal(a.histogram.at(-1).max,40);
 assert.deepEqual(summarizeDetails({},2,10,[25,25]).histogram,[{min:25,max:25,count:2}]);
});

test('Switching to fast results clears the detailed panel',()=>{
 const container={hidden:false,replaceChildren(){this.cleared=true;}};
 renderDetailedResults(container,{detailed:false},10);
 assert.equal(container.hidden,true);assert.equal(container.cleared,true);
});
