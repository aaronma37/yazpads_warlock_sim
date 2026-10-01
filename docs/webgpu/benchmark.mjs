import {GpuSimulator, defaults, validateConfig, packConfigs, summarize} from './runner.mjs';
import createReference from './reference.mjs';
import {runStage} from './async.mjs';
const allModes=[{mode:'event'},...[1000,10000,50000].map(stepUs=>({mode:'fixed',stepUs}))];
function assert(ok,message){if(!ok)throw new Error(message);}
export async function runExperiment({quick=false,compareSchedulers=false,largeBatches=false,signal,onProgress=()=>{}}={}) {
 const modes=compareSchedulers?allModes:[allModes[0]];
 const report={date:new Date().toISOString(),userAgent:navigator.userAgent,scope:'Experimental single-target combat slice, not full simulator',passed:false,validation:[],benchmarks:[]};
 const gpu=await GpuSimulator.create({signal,onProgress});
 report.adapter={vendor:gpu.info?.vendor,architecture:gpu.info?.architecture,device:gpu.info?.device,description:gpu.info?.description,isFallbackAdapter:gpu.info?.isFallbackAdapter};
 onProgress('Adapter: '+JSON.stringify(report.adapter));
 try {
  const wasm=await runStage('Loading WASM reference', () => createReference(), {signal,onProgress});
  function cpu(candidates,n,options={}) {
   candidates=candidates.map(validateConfig);
   const cfg=packConfigs(candidates),input=wasm._malloc(cfg.byteLength),output=wasm._malloc(n*candidates.length*260);
   try {
    assert(input&&output,'WASM allocation failed');wasm.HEAPF32.set(cfg,input/4);
    const start=performance.now();
    const status=wasm._simulate_batch(input,candidates.length,n,options.seed??42,options.mode==='fixed'?options.stepUs:0,output);
    assert(status===0,`CPU reference error ${status}`);
    const kernelMs=performance.now()-start;
    const buffer=wasm.HEAPU8.slice(output,output+n*candidates.length*260).buffer;
    const summaries=summarize(buffer,candidates,n);
    return {buffer,summaries,kernelMs,elapsedMs:performance.now()-start};
   }finally{wasm._free(input);wasm._free(output);}
  }
  // Analytic no-randomness fixture, and awkward timestamps / exhausted mana / proc stress / ISB / multi-DoT / Pets.
  const analytic={...defaults,duration:30,castTime:2.5,corruption:0,nightfall:0,hit:1,crit:0,boltMin:250,boltMax:250,maxMana:100000,mp5:0,tapThreshold:0,dotMultiplier:1.0};
  const cases=[
    analytic,
    {...defaults,duration:1},
    {...defaults,duration:60.123,castTime:2.173913},
    {...defaults,duration:60.123,maxMana:400,tapThreshold:0,corruption:0,mp5:0},
    {...defaults,duration:90.123,nightfall:1,hit:1},
    {...defaults,duration:45.123,hit:0},
    {...defaults,duration:600,corruption:0},
    {...defaults,duration:32.768},
    {...defaults,duration:60.123,isbBonus:0.20,crit:0.3,isbCharges:4,isbAllShadow:1},
    {...defaults,duration:180.123,agony:1,corruption:1,siphon:1,immolate:1,hit:0.95,crit:0.25},
    {...defaults,duration:180.123,petType:1,petCastInterval:2.0,petBaseMin:44,petBaseMax:44,petSpRatio:2.0/3.5,petMultiplier:1.15}, // Imp
    {...defaults,duration:180.123,petType:2,petCastInterval:2.0,petBaseMin:101,petBaseMax:101,petSpRatio:0,petMultiplier:1.15,petLopBase:50,petLopSpRatio:1.5/3.5,petLopCd:12.0} // Succubus
  ];
  for(const options of modes) {
   signal?.throwIfAborted();
   onProgress(`Validating ${options.mode} ${options.stepUs??0} µs against WASM`);
   const n=quick?17:129;
   const actual=await gpu.run(cases,{...options,perCandidate:n,raw:true,signal});
   const expected=cpu(cases,n,options);
   const au=new Uint32Array(actual.buffer),eu=new Uint32Array(expected.buffer),af=new Float32Array(actual.buffer),ef=new Float32Array(expected.buffer);
   for(let i=0;i<au.length;i++) {
    const field=i%59;
    if((field>=20&&field<=27)||(field>=48&&field<=51))assert(Math.abs(af[i]-ef[i])<=Math.max(0.1,Math.abs(ef[i])*2e-6),`GPU/WASM numeric mismatch at ${i}: ${af[i]} vs ${ef[i]}`);
    else if(field!==42)assert(au[i]===eu[i],`GPU/WASM state mismatch (${options.mode}, ${options.stepUs}) at ${i}: ${au[i]} vs ${eu[i]}`);
   }
   assert(actual.summaries[0].bolts===11,'Fight-end boundary must exclude cast at 30s');
   assert(Math.abs(actual.summaries[0].meanDps-11*(250+500*3/3.5)/30)<0.001,'Analytic damage mismatch');
   assert(actual.summaries[3].bolts===1,'OOM stall behavior');
   report.validation.push({mode:options.mode,stepUs:options.stepUs??0,fights:cases.length*n,status:'GPU matches WASM state and analytic fixture',compileMs:actual.compileMs});
   onProgress(`Validated ${options.mode} ${options.stepUs??0} µs`);
  }
  // Candidate packing, padded workgroups, batch-independent replica streams, and WASM memory ABI.
  const candidates=[defaults,{...defaults,spellPower:600,castTime:2.173913}];
  const packed=packConfigs(candidates);
  const multi=await gpu.runPacked(packed,0,2,{perCandidate:131,raw:true,signal});
  const separate=await gpu.run([candidates[1]],{perCandidate:131,raw:true,signal});
  assert(new Uint8Array(multi.buffer,131*236).every((v,i)=>v===new Uint8Array(separate.buffer)[i]),'Candidate batch changed replica stream');
  report.validation.push({status:'Candidate packing, tail workgroup and replica independence passed'});
  const bad=[()=>gpu.run([{unknown:1}]),()=>gpu.run([null]),()=>gpu.run([{spellPower:1e100}]),()=>gpu.run([{boltMin:300,boltMax:200}]),()=>gpu.run([defaults],{perCandidate:0}),()=>gpu.run([defaults],{mode:'bad'}),()=>gpu.run([defaults],{perCandidate:1000001})];
  for(const action of bad){let rejected=false;try{await action();}catch{rejected=true;}assert(rejected,'Invalid input accepted');}
  const aborted=new AbortController();aborted.abort();let cancelled=false;try{await gpu.run([defaults],{signal:aborted.signal});}catch(e){cancelled=e.name==='AbortError';}assert(cancelled,'Cancellation failed');
  // Independent legacy CPU samples are generated by webgpu_oracle; optional in manual builds.
  const oracleResponse=await runStage('Loading legacy CPU fixtures', () => fetch(new URL('./oracle.json', import.meta.url), {signal}), {signal,onProgress});
  if(oracleResponse.ok){
   const oracle=await runStage('Reading legacy CPU fixtures', () => oracleResponse.json(), {signal,onProgress});report.oracle=[];
   for(const fixture of oracle.fixtures){
    onProgress(`Comparing legacy CPU fixture: ${fixture.name}`);
    const result=await gpu.run([fixture.config],{perCandidate:10000,seed:98765,signal});const actual=result.summaries[0],expected=fixture.summary;
    const tolerance=6*Math.hypot(actual.standardError,expected.standardError)+expected.meanDps*0.002;
    assert(Math.abs(actual.meanDps-expected.meanDps)<=tolerance,`Legacy CPU DPS mismatch for ${fixture.name}: ${actual.meanDps} vs ${expected.meanDps}`);
    for(const key of ['bolts','dots','ticks','taps','procs'])assert(Math.abs(actual[key]-expected[key])<=Math.max(0.25,expected[key]*0.015),`Legacy CPU ${key} mismatch for ${fixture.name}: ${actual[key]} vs ${expected[key]}`);
    report.oracle.push({name:fixture.name,cpu:expected,gpu:actual,dpsTolerance:tolerance});
   }
   onProgress('Independent legacy CPU fixtures passed');
  }else{report.oracle='Not run: build webgpu_oracle and generate docs/webgpu/oracle.json';}
  for(const c of (quick?[defaults]:[defaults,{...defaults,castTime:2.173913,duration:300.123}])) {
   const timings=modes.map(()=>[]),n=10000;
   onProgress(`Warming ${n.toLocaleString()}-fight batches (${c.duration}s)`);
   for(const options of modes)await gpu.run([c],{...options,perCandidate:n,signal}); // Full-size warmup.
   const repeats=quick?2:5;
   for(let repetition=0;repetition<repeats;repetition++) {
    // Rotate order to reduce systematic thermal/order bias.
    for(let j=0;j<modes.length;j++) {
     const index=(j+repetition)%modes.length,options=modes[index];
     const result=await gpu.run([c],{...options,perCandidate:n,signal});
     timings[index].push(result);
    }
    onProgress(`Measured ${c.duration}s fights, repetition ${repetition+1}/${repeats}`);
   }
   const med=x=>[...x].sort((a,b)=>a-b)[Math.floor(x.length/2)];
   onProgress(`Measuring WASM reference (${n.toLocaleString()} fights)`);
   const wasmEventMs=med(Array.from({length:repeats},()=>cpu([c],n).elapsedMs));
   const eventDps=timings[0][0].summaries[0].meanDps;
   for(let i=0;i<modes.length;i++) {
    const samples=timings[i],medianMs=med(samples.map(s=>s.elapsedMs));
    report.benchmarks.push({duration:c.duration,castTime:c.castTime,...modes[i],count:n,medianMs,simsPerSecond:n*1000/medianMs,wasmEventMs,speedupVsWasmEvent:wasmEventMs/medianMs,dpsDeltaPercent:(samples[0].summaries[0].meanDps/eventDps-1)*100,samplesMs:samples.map(s=>s.elapsedMs),summary:samples[0].summaries[0]});
   }
  }
  if(!quick) {
   report.scaling=[];
   let expectedSummary;
   for(const workgroupSize of [32,64,128]) {
    const result=await gpu.run([defaults],{perCandidate:100000,workgroupSize,signal});
    const summary=JSON.stringify(result.summaries);
    if(expectedSummary)assert(summary===expectedSummary,'Workgroup size changed results');
    expectedSummary=summary;
    report.scaling.push({count:result.count,workgroupSize,elapsedMs:result.elapsedMs,compileMs:result.compileMs,simsPerSecond:result.simsPerSecond});
   }
   const batch=await gpu.run([defaults,{...defaults,spellPower:600},{...defaults,tapThreshold:0.4}],{perCandidate:10000,signal});
   assert(batch.summaries[1].meanDps>batch.summaries[0].meanDps,'More spell power failed to improve expected damage');
   report.candidateBatch={count:batch.count,elapsedMs:batch.elapsedMs,summaries:batch.summaries};
   onProgress('100,000-fight batches and multi-candidate batch passed');
  }
  if(largeBatches) {
   report.batchScaling=[];
   const config={...defaults,castTime:2.173913,duration:300.123};
   const median=values=>[...values].sort((a,b)=>a-b)[Math.floor(values.length/2)];
   for(const count of [10000,100000,1000000]) {
    signal?.throwIfAborted();
    onProgress(`Warming event-driven GPU and WASM: ${count.toLocaleString()} fights`);
    await gpu.run([config],{perCandidate:count,signal});
    cpu([config],count);
    const gpuSamples=[],cpuSamples=[];
    for(let i=0;i<3;i++) {
     onProgress(`Measuring GPU: ${count.toLocaleString()} fights, sample ${i+1}/3`);
     const result=await gpu.run([config],{perCandidate:count,signal});
     gpuSamples.push(result.elapsedMs);
     onProgress(`Measuring WASM: ${count.toLocaleString()} fights, sample ${i+1}/3`);
     const reference=cpu([config],count);
     cpuSamples.push(reference.elapsedMs);
     assert(Math.abs(result.summaries[0].meanDps-reference.summaries[0].meanDps)<0.001,'Large batch changed expected damage');
    }
    const gpuMs=median(gpuSamples),wasmMs=median(cpuSamples);
    report.batchScaling.push({count,duration:config.duration,castTime:config.castTime,gpuMs,wasmMs,speedup:wasmMs/gpuMs,gpuSamples,wasmSamples:cpuSamples});
    onProgress(`${count.toLocaleString()} fights: GPU ${gpuMs.toFixed(1)} ms, WASM ${wasmMs.toFixed(1)} ms, ${(wasmMs/gpuMs).toFixed(1)}x`);
   }
  }
  report.passed=true;return report;
 }finally{gpu.destroy();}
}
