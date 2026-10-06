import test from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { FRAGMENT, FAST_FRAGMENT } from '../src/kernel.js';
import { REGRET_FRAGMENT, summarizeDifferences } from '../src/regret.js';

test('regret diagnostics do not change simulation shaders', () => {
 assert.ok(!FRAGMENT.includes('targetDecision'));
 assert.ok(!FAST_FRAGMENT.includes('targetDecision'));
 assert.ok(REGRET_FRAGMENT.includes('uniform highp usampler2D regretJobTex'));
 // Frozen pre-feature shader output: catches accidental changes to the fast path.
 assert.equal(createHash('sha256').update(FAST_FRAGMENT).digest('hex'), '6d939ed98a10f9e9668bbc748f9867355c3d3a72103b9d74593ba472cb115174');
});

test('paired differences use full fight duration and uncertainty of differences', () => {
 const rows=summarizeDifferences([100,200,300],[
  {action:1,totals:[120,220,320]},
  {action:16,totals:[100,200,300]},
  {action:6,totals:[80,180,280]},
  {action:9,totals:[110,220,330]},
 ],10);
 assert.equal(rows[0].deltaDps,2);assert.equal(rows[0].ci95,0);
 assert.equal(rows[1].deltaDps,0);assert.equal(rows[1].ci95,0);
 assert.equal(rows[2].deltaDps,-2);
 assert.ok(Math.abs(rows[3].ci95-1.96/Math.sqrt(3))<1e-12);
});

import { confirmationCriticalValue, rankConfirmedRegrets, scanDecisionRegrets } from '../src/regret.js';
const result=(decision,rows,samples=256)=>({decision,time:decision*2,mana:1000,policyAction:16,samples,
 rows:[{action:16,deltaDps:0,standardError:0},...rows],regretDps:Math.max(0,...rows.map(row=>row.deltaDps))});

test('confirmation adjusts for multiple comparisons and finite sample size',()=>{
 assert.ok(Math.abs(confirmationCriticalValue(1,128)-1.9788195)<.00001);
 assert.ok(confirmationCriticalValue(20,256)>confirmationCriticalValue(1,256));
 assert.ok(confirmationCriticalValue(20,128)>confirmationCriticalValue(20,4096));
});

test('ranking rejects uncertain wins, chooses one alternative per decision, and caps at five',()=>{
 const results=Array.from({length:7},(_,decision)=>result(decision,[
  {action:1,deltaDps:decision+2,standardError:.1},
  {action:6,deltaDps:100,standardError:100}, // Largest mean is not a supported win.
 ]));
 results.push(result(7,[{action:1,deltaDps:1,standardError:1}]));
 results.push(result(8,[{action:1,deltaDps:-1,standardError:0}]));
 results.push(result(9,[{action:1,deltaDps:0,standardError:0}]));
 const ranking=rankConfirmedRegrets(results);
 assert.equal(ranking.items.length,5);assert.equal(ranking.supportedDecisions,7);
 assert.equal(ranking.comparisons,17);assert.deepEqual(ranking.items.map(item=>item.decision),[6,5,4,3,2]);
 assert.ok(ranking.items.every(item=>item.action===1&&item.lower>0));
 assert.ok(ranking.items.every(item=>item.alternatives.length===3&&item.maxDpsGain===100));
 assert.deepEqual(ranking.items[0].alternatives.map(row=>row.action),[6,1,16]);
 assert.equal(ranking.items[0].alternatives.at(-1).deltaDps,0);
 assert.equal(rankConfirmedRegrets([]).items.length,0);
});

test('scan covers every decision and confirms at most twenty with fresh seed ranges',async()=>{
 const calls=[],progress=[];
 const scan=await scanDecisionRegrets({
  countDecisions:async()=>25,
  evaluateMany:async(decisions,samples,seed,notify)=>{
   calls.push({decisions,samples,seed});notify(decisions.length*17*samples,decisions.length*17*samples);
   return decisions.map(decision=>result(decision,[{action:1,deltaDps:decision+1,standardError:.01}],samples));
  },
 },256,p=>progress.push(p));
 assert.equal(calls.length,2); // One dispatch plan per stage, not one per decision.
 assert.deepEqual(calls[0].decisions,Array.from({length:25},(_,i)=>i));
 assert.equal(scan.totalDecisions,25);assert.equal(scan.confirmedDecisions,20);
 assert.equal(calls[0].samples,32);assert.equal(calls[1].samples,256);
 assert.notEqual(calls[0].seed,calls[1].seed);
 assert.deepEqual(scan.items.map(item=>item.decision),[24,23,22,21,20]);
 assert.equal(progress[0].completed,25*17*32);assert.equal(progress.at(-1).completed,20*17*256);
});

test('confirmation can overturn all screened wins and errors stop a scan',async()=>{
 const scan=await scanDecisionRegrets({countDecisions:async()=>3,evaluateMany:async(decisions,samples)=>
  decisions.map(decision=>result(decision,[{action:1,deltaDps:samples===32?10:-1,standardError:.1}],samples))},128);
 assert.equal(scan.totalDecisions,3);assert.equal(scan.confirmedDecisions,3);assert.deepEqual(scan.items,[]);
 let calls=0;
 await assert.rejects(scanDecisionRegrets({countDecisions:async()=>3,evaluateMany:async()=>{
  calls++;throw new DOMException('Cancelled','AbortError');
 }},128),{name:'AbortError'});
 assert.equal(calls,1);
});

import { REGRET_ACTIONS, regretLaneJob, decodeRegretResults } from '../src/regret.js';
test('parallel lane mapping preserves paired samples across batches and texture rows',()=>{
 const decisions=[17,2,9],samples=128,total=decisions.length*REGRET_ACTIONS.length*samples;
 const metadata=decisions.map(decision=>({time:decision,mana:1000,policyAction:16}));
 function run(batchSize){
  const totals=new Float32Array(total),unavailable=new Uint8Array(decisions.length*REGRET_ACTIONS.length);
  for(let first=0;first<total;first+=batchSize){
   for(let lane=0;lane<Math.min(batchSize,total-first);lane++){
    const globalLane=first+lane,{decisionIndex,actionIndex,sample}=regretLaneJob(globalLane,samples);
    const action=REGRET_ACTIONS[actionIndex];
    if(action===10){unavailable[decisionIndex*REGRET_ACTIONS.length+actionIndex]=1;continue;}
    const baseline=1000+decisionIndex*500+sample*50;
    totals[globalLane]=baseline+(action===0||action===16?0:action*20);
   }
  }
  return {results:decodeRegretResults(decisions,samples,10,metadata,totals,unavailable),totals,unavailable};
 }
 const large=run(65536),split=run(1009);
 assert.deepEqual(large.results,split.results);
 assert.deepEqual(large.results.map(row=>row.decision),decisions);
 for(const decision of large.results){
  assert.ok(!decision.rows.some(row=>row.action===10));
  assert.equal(decision.rows.find(row=>row.action===1).deltaDps,2);
  assert.equal(decision.rows.find(row=>row.action===1).standardError,0);
  assert.equal(decision.rows.find(row=>row.action===16).deltaDps,0);
 }
 const policyIndex=REGRET_ACTIONS.indexOf(16);large.totals[policyIndex*samples]++;
 assert.throws(()=>decodeRegretResults(decisions,samples,10,metadata,large.totals,large.unavailable),/policy action differs/);
});
