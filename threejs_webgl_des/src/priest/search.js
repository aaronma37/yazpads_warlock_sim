import { PRIEST_RACES } from './racials.js';
import { createPriestAPL } from './policy.js';
import { TALENT_TREES } from './talent_data.js';
import { validateTalentAllocation } from './talents.js';
import { resolvePriestUIBuild } from './ui_config.js';
import { createBuildOptimizer } from '../search/genetic_optimizer.js';

export const TREE_NAMES = ['Discipline','Holy','Shadow'];
export const TALENT_DEFINITIONS = Object.entries(TALENT_TREES).flatMap(([tree,nodes],treeIndex)=>
  nodes.map(node=>({...node,id:node.key,tree:treeIndex,treeName:TREE_NAMES[treeIndex],treeKey:tree})));
const indexByKey=new Map(TALENT_DEFINITIONS.map((node,index)=>[node.key,index]));
const prerequisites=TALENT_DEFINITIONS.map(node=>node.requires.map(req=>({index:indexByKey.get(req.key),rank:req.rank})));
export const RACES=PRIEST_RACES;
export const ROTATIONS=['shadow','holy','mixed'];
export const ROTATION_LABELS={shadow:'Shadow',holy:'Holy / Smite',mixed:'Mixed Holy & Shadow'};
const pick=(rng,values)=>values[rng.nextU64()%values.length];

export class PriestTalentGraph {
  static countTotalPoints(v){return Array.from(v).reduce((a,b)=>a+b,0);}
  static countTreePoints(v,tree){return TALENT_DEFINITIONS.reduce((sum,node,i)=>sum+(node.tree===tree?v[i]:0),0);}
  static toTalentsObject(v){
    if(!v||v.length!==TALENT_DEFINITIONS.length)throw new Error('Priest talent vector must contain 53 ranks.');
    return Object.fromEntries(Object.keys(TALENT_TREES).map(tree=>[tree,Object.fromEntries(TALENT_DEFINITIONS.flatMap((node,i)=>node.treeKey===tree?[[node.key,v[i]]]:[]))]));
  }
  static fromTalentsObject(input){const allocation=validateTalentAllocation(input);return Uint8Array.from(TALENT_DEFINITIONS.map(node=>allocation[node.treeKey][node.key]));}
  static isValid(v,total=51){
    if(!v||v.length!==53||this.countTotalPoints(v)!==total)return false;
    try{validateTalentAllocation(this.toTalentsObject(v));return true;}catch{return false;}
  }
  static getValidReceivers(v){
    const rows=Array.from({length:3},()=>Array(7).fill(0));
    TALENT_DEFINITIONS.forEach((node,i)=>{rows[node.tree][node.row]+=v[i];});
    return TALENT_DEFINITIONS.flatMap((node,i)=>v[i]<node.max&&
      rows[node.tree].slice(0,node.row).reduce((a,b)=>a+b,0)>=node.requiredPoints&&
      prerequisites[i].every(req=>v[req.index]>=req.rank)?[i]:[]);
  }
  static getValidDonors(v,protectedRanks=new Map()){
    return TALENT_DEFINITIONS.flatMap((_,i)=>{
      if(v[i]<=(protectedRanks.get(i)??0))return [];
      const test=Array.from(v);test[i]--;
      return this.isValid(test,this.countTotalPoints(v)-1)?[i]:[];
    });
  }
  // Construct forward from legal receivers: prerequisites/row gates cannot be
  // broken by crossover repair, and impossible constraints fail before GPU work.
  static build(rng,required=[],preferred=null){
    const v=new Uint8Array(53), protectedRanks=new Map();
    const requireRank=(index,rank)=>{
      for(const req of prerequisites[index])requireRank(req.index,req.rank);
      const node=TALENT_DEFINITIONS[index];
      while(TALENT_DEFINITIONS.reduce((sum,n,i)=>sum+(n.tree===node.tree&&n.row<node.row?v[i]:0),0)<node.requiredPoints){
        const choices=this.getValidReceivers(v).filter(i=>TALENT_DEFINITIONS[i].tree===node.tree&&TALENT_DEFINITIONS[i].row<node.row);
        if(!choices.length)throw new Error('Cannot satisfy Priest talent row requirements.');
        const wanted=preferred?choices.filter(i=>v[i]<preferred[i]):[];
        v[pick(rng,wanted.length?wanted:choices)]++;
      }
      v[index]=Math.max(v[index],rank);protectedRanks.set(index,Math.max(protectedRanks.get(index)??0,rank));
    };
    for(const index of [...required].sort((a,b)=>TALENT_DEFINITIONS[a].row-TALENT_DEFINITIONS[b].row))requireRank(index,TALENT_DEFINITIONS[index].max);
    if(this.countTotalPoints(v)>51)throw new Error('Required Priest talents cannot fit in 51 points.');
    const primary=rng.nextU64()%3;
    while(this.countTotalPoints(v)<51){
      const choices=this.getValidReceivers(v), wanted=preferred?choices.filter(i=>v[i]<preferred[i]):[];
      const focused=choices.filter(i=>TALENT_DEFINITIONS[i].tree===primary);
      const candidates=wanted.length?wanted:focused.length&&rng.nextDouble()<.8?focused:choices;
      if(!candidates.length)throw new Error('Cannot construct a legal Priest talent allocation.');
      v[pick(rng,candidates)]++;
    }
    return {v,protectedRanks};
  }
}

function requiredIndices(config){
  if(!Array.isArray(config.requiredTalents??[]))throw new Error('Priest required talents must be an array.');
  return [...new Set(config.requiredTalents??[])].map(value=>{
    const index=typeof value==='string'?indexByKey.get(value):value;
    if(!Number.isInteger(index)||index<0||index>=53)throw new Error('Unknown required Priest talent.');
    return index;
  });
}
function choices(ind,config){
  if(config.forcedRace&&config.forcedRace!=='ALL')ind.race=config.forcedRace;
  if(config.forcedRotation&&config.forcedRotation!=='ALL')ind.rotation=config.forcedRotation;
  if(!RACES.includes(ind.race)||!ROTATIONS.includes(ind.rotation))throw new Error('Unsupported Priest search race or rotation.');
}
function syncAPL(ind,config,rng,randomize=false){
  if(config.aplMode!=='coevolve'){ind.apl=null;return;}
  const defaults=createPriestAPL(ind.rotation), previous=ind.apl??[];
  ind.apl=previous.filter(r=>defaults.some(d=>d.spell===r.spell)).map(r=>({...r}));
  for(const rule of defaults)if(!ind.apl.some(r=>r.spell===rule.spell))ind.apl.push({...rule});
  if(config.lockConditions)for(const rule of ind.apl)Object.assign(rule,{healthMax:100,manaMin:0,enabled:true});
  if(randomize)for(let i=ind.apl.length-1;i>0;i--){const j=rng.nextU64()%(i+1);[ind.apl[i],ind.apl[j]]=[ind.apl[j],ind.apl[i]];}
}
function enforceConstraints(ind,config,rng){
  choices(ind,config);
  const required=requiredIndices(config);
  if(!PriestTalentGraph.isValid(ind.talents)||required.some(i=>ind.talents[i]!==TALENT_DEFINITIONS[i].max))
    ind.talents=PriestTalentGraph.build(rng,required,ind.talents).v;
  syncAPL(ind,config,rng);
}
function createRandomIndividual(rng,config){
  const ind={talents:PriestTalentGraph.build(rng,requiredIndices(config)).v,race:pick(rng,RACES),rotation:pick(rng,ROTATIONS),apl:null,fitness:0,batch:null};
  choices(ind,config);syncAPL(ind,config,rng,true);return ind;
}
function crossoverIndividuals(a,b,rng,config){
  const preferred=Uint8Array.from(a.talents,(rank,i)=>rng.nextU64()%2?rank:b.talents[i]);
  const child={talents:PriestTalentGraph.build(rng,requiredIndices(config),preferred).v,race:rng.nextU64()%2?a.race:b.race,rotation:rng.nextU64()%2?a.rotation:b.rotation,apl:null,fitness:0,batch:null};
  child.apl=structuredClone(rng.nextU64()%2?a.apl:b.apl);
  choices(child,config);syncAPL(child,config,rng);return child;
}
function mutateIndividual(ind,rng,config){
  if(rng.nextDouble()<(config.mutationRate??.45)){
    const protectedRanks=new Map();
    const protect=(i,rank)=>{protectedRanks.set(i,Math.max(rank,protectedRanks.get(i)??0));prerequisites[i].forEach(req=>protect(req.index,req.rank));};
    requiredIndices(config).forEach(i=>protect(i,TALENT_DEFINITIONS[i].max));
    for(let n=0,count=1+rng.nextU64()%3;n<count;n++){
      const donors=PriestTalentGraph.getValidDonors(ind.talents,protectedRanks);
      if(!donors.length)break;
      const donor=pick(rng,donors);ind.talents[donor]--;
      const receivers=PriestTalentGraph.getValidReceivers(ind.talents);
      ind.talents[pick(rng,receivers)]++;
    }
  }
  if(config.optimizeRace&&config.forcedRace==='ALL'&&rng.nextDouble()<.2)ind.race=pick(rng,RACES);
  if(config.forcedRotation==='ALL'&&rng.nextDouble()<(config.mutationRate??.45))ind.rotation=pick(rng,ROTATIONS);
  choices(ind,config);syncAPL(ind,config,rng);
  if(ind.apl&&rng.nextDouble()<(config.mutationRate??.45)){
    const i=rng.nextU64()%ind.apl.length,j=rng.nextU64()%ind.apl.length;
    [ind.apl[i],ind.apl[j]]=[ind.apl[j],ind.apl[i]];
    if(!config.lockConditions){const r=ind.apl[i];r.healthMax=rng.nextU64()%101;r.manaMin=rng.nextU64()%101;if(rng.nextDouble()<.15)r.enabled=!r.enabled;}
  }
}
const identity={
  formatBuildName:ind=>[0,1,2].map(tree=>PriestTalentGraph.countTreePoints(ind.talents,tree)).join('/'),
  getIndUniqueKey:ind=>`${ind.race}:${ind.rotation}:${Array.from(ind.talents).join(',')}:${JSON.stringify(ind.apl??null)}`,
  getMapElitesKey:ind=>`${ind.race}:${ind.rotation}:${[0,1,2].map(tree=>Math.floor(PriestTalentGraph.countTreePoints(ind.talents,tree)/5)).join(':')}:${['shadowform','penance','power_infusion'].map(key=>ind.talents[indexByKey.get(key)]).join('')}`,
};
export function candidateToPreset(candidate){
  const allocation=candidate.talents??PriestTalentGraph.toTalentsObject(candidate.talentsVector);
  return {id:'priest_search_result',label:`Search: ${candidate.name} · ${ROTATION_LABELS[candidate.rotation]} · ${candidate.race}`,race:candidate.race,rotation:candidate.rotation,apl:structuredClone(candidate.apl??candidate.individual?.apl??null),
    ranks:Object.assign({},...Object.values(validateTalentAllocation(allocation)))};
}
export function candidateToConfig(ind,base){
  const preset=candidateToPreset({...ind,talents:PriestTalentGraph.toTalentsObject(ind.talents)});
  // A candidate always starts from neutral direct stats, never the resolved
  // current build. The UI adapter applies its own race and talent effects once.
  return resolvePriestUIBuild({...base,preset}).config;
}
function createCandidateResult(ind,rank=1){
  const summary=ind.batch?.summary??{}, states=ind.batch?.states??[];
  const samples=states.map(s=>s.total/ind.batch.config.duration).sort((a,b)=>a-b);
  const points=[0,1,2].map(tree=>PriestTalentGraph.countTreePoints(ind.talents,tree));
  return {id:identity.getIndUniqueKey(ind),rank,name:identity.formatBuildName(ind),category:ROTATION_LABELS[ind.rotation],race:ind.race,rotation:ind.rotation,
    mean_dps:summary.mean??ind.fitness,std_dev:summary.sd??0,ci95:samples.length>1?1.96*(summary.sd??0)/Math.sqrt(samples.length):0,
    min_dps:samples.length?samples[Math.floor((samples.length-1)*.05)]:0,max_dps:samples.length?samples[Math.floor((samples.length-1)*.95)]:0,
    points,apl:structuredClone(ind.apl),talents:PriestTalentGraph.toTalentsObject(ind.talents),talentsVector:Array.from(ind.talents),summary,states,individual:ind};
}
export const PRIEST_SEARCH_CLASS=Object.freeze({id:'priest',capabilities:{schemaVersion:1,features:{batchSimulation:true,buildSearch:true}},search:{
  identity,results:{createCandidateResult},
  talents:{graph:PriestTalentGraph,definitions:TALENT_DEFINITIONS,nodeCount:53,treeNodeCounts:[18,17,18],pointBudget:51},
  choices:{races:RACES,rotations:ROTATIONS,rotationLabels:ROTATION_LABELS,enforceLocked:choices},
  pets:{constraints:[],modes:[],enforce:enforceConstraints},
  apl:{cloneIndividual:apl=>structuredClone(apl),createDefault:()=>null,availableActions:()=>[],encodeIndividual:()=>[]},
  createBuildSearch:()=>({enforceConstraints,createRandomIndividual,crossoverIndividuals,mutateIndividual}),
  createCandidateConfig:()=>({buildToConfig:candidateToConfig,policyForBuild:ind=>({rotation:ind.rotation})}),
  createPresetCandidate:({enforceConstraints:enforce})=>(preset,config,rng)=>{
    const ind={talents:PriestTalentGraph.fromTalentsObject(Object.fromEntries(Object.entries(TALENT_TREES).map(([tree,nodes])=>[tree,Object.fromEntries(nodes.map(node=>[node.key,preset.ranks[node.key]??0]))]))),race:preset.race??'HUMAN',rotation:preset.rotation??'shadow',apl:null,fitness:0,batch:null};
    enforce(ind,config,rng);return ind;
  },
}});
export function normalizeSearchConfig(input,base){
  const c={populationSize:1000,generations:15,screeningSims:100,finalSims:2000,seed:42,mutationRate:.45,seedPresets:true,optimizeRace:true,forcedRace:'ALL',forcedRotation:'ALL',requiredTalents:[],aplMode:'coevolve',lockConditions:true,...input};
  if(!['static','coevolve'].includes(c.aplMode)||typeof c.lockConditions!=='boolean')throw new Error('Invalid Priest APL strategy.');
  for(const [key,min,max] of [['populationSize',2,10000],['generations',1,1000],['screeningSims',1,10000],['finalSims',1,50000],['seed',0,4294967295]])
    if(!Number.isInteger(c[key])||c[key]<min||c[key]>max)throw new Error(`Invalid Priest search ${key}.`);
  if(!Number.isFinite(c.mutationRate)||c.mutationRate<0||c.mutationRate>1)throw new Error('Invalid Priest mutation rate.');
  if(c.forcedRace!=='ALL'&&!RACES.includes(c.forcedRace))throw new Error('Unsupported Priest search race.');
  if(c.forcedRotation!=='ALL'&&!ROTATIONS.includes(c.forcedRotation))throw new Error('Unsupported Priest search rotation.');
  for(const key of ['optimizeRace','seedPresets'])if(typeof c[key]!=='boolean')throw new Error(`Invalid Priest search ${key}.`);
  if(!c.optimizeRace&&c.forcedRace==='ALL')c.forcedRace=base.race??'HUMAN';
  if(c.forcedRace!=='ALL'&&!RACES.includes(c.forcedRace))throw new Error('Unsupported Priest search race.');
  c.requiredTalents=requiredIndices(c);
  // Fixed deterministic feasibility pass; consumes none of the evolutionary RNG.
  PriestTalentGraph.build({nextU64:()=>0,nextDouble:()=>0},c.requiredTalents);
  if(!Array.isArray(c.presetsList??[]))throw new Error('Priest search presets must be an array.');
  return c;
}
export async function runPriestConstrainedSearch(base,input,{simulation,signal,onProgress=()=>{},onGeneration=()=>{}}={}){
  const config=normalizeSearchConfig(input,base);
  if(!simulation||simulation.id!=='priest'||typeof simulation.runMultiSimulation!=='function')throw new Error('Priest search requires the Priest batch simulation.');
  let latest=null;
  const stopped=()=>({stopped:true,candidates:latest?.elites??[],bestCandidate:latest?.elites?.[0]??null,evolutionHistory:latest?.evolutionHistory??[],totalEvaluations:latest?.totalEvaluations??0,totalSimulations:latest?.totalSimulations??0,uniqueConfigsCount:latest?.uniqueConfigsCount??0});
  if(signal?.aborted)return stopped();
  const optimizer=createBuildOptimizer({classModule:PRIEST_SEARCH_CLASS,simulation});
  try{const result=await optimizer.runConstrainedGeneticSearch(base,config,{signal,onProgress,onGeneration:state=>{latest=state;onGeneration(state);}});return signal?.aborted?stopped():result;}
  catch(error){if(error.name==='AbortError')return stopped();throw error;}
}
