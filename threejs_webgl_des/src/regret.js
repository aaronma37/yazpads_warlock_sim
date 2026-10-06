// Dedicated diagnostic variant. The production detailed and fast shaders are unchanged.
// Use the lean kernel as a template: rollouts need total damage, not spell counters.
import { FAST_FRAGMENT as FRAGMENT } from './kernel.js';
export const ACTION_NAMES = {1:'Life Tap',2:'Nightfall Shadow Bolt',3:'Searing Pain',4:'Soul Fire',6:'Corruption',7:'Curse of Doom',8:'Curse of Agony',9:'Immolate',10:'Conflagrate',11:'Shadowburn',12:'Incinerate',16:'Shadow Bolt',17:'Siphon Life',18:'Drain Hope',19:'Hellfire'};
export const REGRET_ACTIONS = [0,...Object.keys(ACTION_NAMES).map(Number)];
const start = FRAGMENT.indexOf('  // Execute Action');
const end = FRAGMENT.indexOf('\n }\n\n // Fallback', start);
if(start<0||end<0)throw new Error('Diagnostic action extraction no longer matches the simulation shader.');
let execution = FRAGMENT.slice(start, end).replaceAll('return;', 'return true;');
// An unavailable alternative must be rejected, not silently replaced by Life Tap.
execution = execution.replaceAll('else{tap();return true;}', 'else{if(strict)return false;chosenAction=1u;tap();return true;}');
// A configured filler remains executable even when its maintenance toggle is off.
execution = execution.replace('if(c.incinerate!=0u)', 'if(c.incinerate!=0u||(strict&&c.filler==4u))');
const helpers = `
uniform highp usampler2D regretJobTex;
uniform uint regretSamples;
uniform uint continuationSeed;
uint targetDecision=0u, forcedAction=0u, sampleForRegret=0u;
const uint regretActions[${REGRET_ACTIONS.length}]=uint[${REGRET_ACTIONS.length}](${REGRET_ACTIONS.map(action=>`${action}u`).join(',')});
uint decisionCount=0u, chosenAction=0u, decisionTime=0u;
float decisionMana=0.0;
bool reached=false;
bool executeDiagnostic(uint action, bool strict){
${execution}
 return false;
}
bool takeAction(uint action){
 chosenAction=(action==5u||action==13u)?3u:(action==14u||action==15u)?16u:action;
 if(executeDiagnostic(action,false))return true;
 return false;
}
`;
let shader = FRAGMENT.slice(0,start) + '  if(takeAction(action))return;' + FRAGMENT.slice(end);
shader = shader.replace('void decide(){', helpers + '\nvoid decide(){');
shader = shader.replace(' checkTrinket();checkRacial();', `
 if(decisionCount++==targetDecision){
  reached=true;decisionTime=s.now;decisionMana=s.mana;
  if(mode==4u){
   // Fixed prefix, independent continuation streams shared across actions.
   seedRandom(continuationSeed+sampleForRegret);
  }
 }
 checkTrinket();checkRacial();
 if(mode==4u&&reached&&decisionCount==targetDecision+1u&&forcedAction!=0u){if(!executeDiagnostic(forcedAction,true))s.done=9u;return;} `);
// Diagnostic lanes span a 2D grid; job indices remain global across batches.
shader = shader.replace('lane=x;stripe=0u;', 'lane=mode>=3u?y*gridWidth+x:x;stripe=0u;');
shader = shader.replace('if(lane>=count&&mode==0u)return;', 'if(lane>=count&&(mode==0u||mode>=3u))return;');
shader = shader.replace(' uint globalLane=offset+lane;', `
 uint globalLane=offset+lane;
 if(mode==3u||mode==4u){
  uint decisionIndex=globalLane;
  if(mode==4u){
   uint group=globalLane/regretSamples;
   decisionIndex=group/${REGRET_ACTIONS.length}u;
   forcedAction=regretActions[group%${REGRET_ACTIONS.length}u];
   sampleForRegret=globalLane%regretSamples;
  }
  uvec4 job=texelFetch(regretJobTex,ivec2(int(decisionIndex/4u),0),0);
  targetDecision=job[int(decisionIndex%4u)];
 }
`);
shader = shader.replace('seedRandom(mode==2u?0u:(numConfigs>1u?fightInCfg:globalLane));', 'seedRandom(0u);');
shader = shader.replace('if(c.race==3u&&s.eurekaCharges==1u&&playerManaPct<70.0){tap();return;}', 'if(c.race==3u&&s.eurekaCharges==1u&&playerManaPct<70.0){chosenAction=1u;tap();return;}');
shader = shader.replace('if(s.mana>=cost(c.filler)){beginCast(c.filler);return;}\n tap();', 'if(s.mana>=cost(c.filler)){chosenAction=c.filler==4u?12u:c.filler==5u?3u:16u;beginCast(c.filler);return;}\n chosenAction=1u;tap();');
shader = shader.replace('if(s.done!=0u)break;advance();', 'if(s.done!=0u||(mode==3u&&reached))break;advance();');
shader = shader.replace('if(mode!=2u&&s.done==0u)s.done=6u;', 'if(mode==3u&&reached)s.done=8u;else if(mode!=2u&&s.done==0u)s.done=6u;');
shader = shader.replace('if(mode==2u){\n  report0=', `if(mode==3u||mode==4u||mode==5u){
  report0=uvec4(floatBitsToUint(s.total),s.done,uint(reached),decisionTime);
  report1=uvec4(chosenAction,floatBitsToUint(decisionMana),decisionCount,s.rngCalls);
 }else if(mode==2u){\n  report0=`);
export const REGRET_FRAGMENT = shader;

export function summarizeDifferences(baseline, alternatives, duration){
 return alternatives.map(({action, totals})=>{
  const differences=Array.from(totals,(value,i)=>(value-baseline[i])/duration);
  const mean=differences.reduce((a,b)=>a+b,0)/differences.length;
  const variance=differences.reduce((a,b)=>a+(b-mean)**2,0)/(differences.length-1);
  return {action, deltaDps:mean, standardError:Math.sqrt(variance/differences.length), ci95:1.96*Math.sqrt(variance/differences.length)};
 });
}

export function shortlistDecisions(results,limit=20){
 return results.filter(result=>result.rows.some(row=>row.action!==result.policyAction&&row.deltaDps>0))
  .sort((a,b)=>b.regretDps-a.regretDps||a.decision-b.decision).slice(0,limit);
}

// Acklam inverse normal CDF. Confirmation uses at least 128 samples, so the
// Student-t expansion is accurate for the Bonferroni tails used here.
function normalQuantile(p){
 const a=[-39.6968302866538,220.946098424521,-275.928510446969,138.357751867269,-30.6647980661472,2.50662827745924];
 const b=[-54.4760987982241,161.585836858041,-155.698979859887,66.8013118877197,-13.2806815528857];
 const c=[-.00778489400243029,-.322396458041136,-2.40075827716184,-2.54973253934373,4.37466414146497,2.93816398269878];
 const d=[.00778469570904146,.32246712907004,2.445134137143,3.75440866190742];
 const polynomial=(coeffs,x)=>coeffs.reduce((value,coefficient)=>value*x+coefficient,0);
 if(p<.02425){const q=Math.sqrt(-2*Math.log(p));return polynomial(c,q)/(polynomial(d,q)*q+1);}
 if(p>1-.02425)return -normalQuantile(1-p);
 const q=p-.5,r=q*q;return polynomial(a,r)*q/(polynomial(b,r)*r+1);
}
export function confirmationCriticalValue(comparisons,samples){
 const z=normalQuantile(1-.05/(2*Math.max(1,comparisons))),df=samples-1;
 return z+(z**3+z)/(4*df)+(5*z**5+16*z**3+3*z)/(96*df**2)
  +(3*z**7+19*z**5+17*z**3-15*z)/(384*df**3);
}

export function rankConfirmedRegrets(results,limit=5){
 const comparisons=results.reduce((count,result)=>count+result.rows.filter(row=>row.action!==result.policyAction).length,0);
 const items=[];
 for(const result of results){
  const critical=confirmationCriticalValue(comparisons,result.samples);
  const alternatives=result.rows.map(row=>({
   ...row,lower:row.deltaDps-critical*row.standardError,upper:row.deltaDps+critical*row.standardError,
  })).sort((a,b)=>b.deltaDps-a.deltaDps||a.action-b.action);
  const supported=alternatives.filter(row=>row.action!==result.policyAction&&Number.isFinite(row.lower)&&row.lower>0)
   .sort((a,b)=>b.lower-a.lower||b.deltaDps-a.deltaDps);
  if(supported.length){const best=supported[0];items.push({decision:result.decision,time:result.time,mana:result.mana,
   policyAction:result.policyAction,samples:result.samples,...best,alternatives,
   maxDpsGain:Math.max(0,...alternatives.map(row=>row.deltaDps))});}
 }
 items.sort((a,b)=>b.lower-a.lower||b.deltaDps-a.deltaDps||a.decision-b.decision);
 return {items:items.slice(0,limit),supportedDecisions:items.length,comparisons,confidence:.95};
}

// Keep scheduling separate from WebGL so complete coverage and independent
// confirmation can be checked without a GPU. Session evaluation owns cancellation.
export async function scanDecisionRegrets({countDecisions,evaluateMany},samples,onProgress=()=>{}){
 const scanSamples=32,totalDecisions=await countDecisions();
 const decisions=Array.from({length:totalDecisions},(_,decision)=>decision);
 const screened=await evaluateMany(decisions,scanSamples,0x9e3779b9,
  (completed,total)=>onProgress({phase:'Scanning decisions',completed,total}));
 const candidates=shortlistDecisions(screened,20);
 const confirmed=await evaluateMany(candidates.map(result=>result.decision),samples,0x243f6a88,
  (completed,total)=>onProgress({phase:'Confirming candidates',completed,total}));
 return {totalDecisions,scanSamples,confirmationSamples:samples,confirmedDecisions:confirmed.length,
  ...rankConfirmedRegrets(confirmed,5)};
}

// Arithmetic mapping is independent of batch offsets and texture row boundaries.
export function regretLaneJob(lane,samples){
 const group=Math.floor(lane/samples);
 return {decisionIndex:Math.floor(group/REGRET_ACTIONS.length),actionIndex:group%REGRET_ACTIONS.length,sample:lane%samples};
}

export function decodeRegretResults(decisions,samples,duration,metadata,totals,unavailable){
 return decisions.map((decision,index)=>{
  const {time,mana,policyAction}=metadata[index],base=index*REGRET_ACTIONS.length*samples;
  const baseline=totals.subarray(base,base+samples),valid=[];
  for(let actionIndex=1;actionIndex<REGRET_ACTIONS.length;actionIndex++){
   if(!unavailable[index*REGRET_ACTIONS.length+actionIndex]){
    const first=base+actionIndex*samples;
    valid.push({action:REGRET_ACTIONS[actionIndex],totals:totals.subarray(first,first+samples)});
   }
  }
  const policyReplay=valid.find(row=>row.action===policyAction);
  if(!policyReplay||policyReplay.totals.some((total,i)=>total!==baseline[i]))throw new Error('Forcing the policy action differs from its baseline; diagnostic results rejected.');
  const rows=summarizeDifferences(baseline,valid,duration).sort((a,b)=>b.deltaDps-a.deltaDps);
  return {decision,time,mana,policyAction,samples,rows,regretDps:Math.max(0,...rows.map(row=>row.deltaDps))};
 });
}
