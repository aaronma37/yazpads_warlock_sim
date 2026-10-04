import { WebGLRenderer, WebGLRenderTarget, RawShaderMaterial, BufferGeometry, BufferAttribute,
  Mesh, Scene, Camera, GLSL3, RGBAIntegerFormat, UnsignedIntType, NearestFilter, NoBlending, DataTexture } from 'three';
import { COMPACT_WORDS, COMPACT_STRIPES, validate, packConfig, decodeStates, STATE_WORDS, TRACE_CAPACITY, SPELLS, CONFIG, CONFIG_WORDS } from './model.js';
import { VERTEX, FRAGMENT } from './kernel.js';

const yieldUI=()=>new Promise(resolve=>setTimeout(resolve,0));
function checkAbort(signal){if(signal?.aborted)throw new DOMException('Run cancelled.','AbortError');}
let engine=null, running=false;

function acquire(){
 if(engine)return engine;
 const canvas=document.createElement('canvas');
 const context=canvas.getContext('webgl2',{alpha:false,antialias:false,depth:false,stencil:false});
 if(!context)throw new Error('WebGL2 is unavailable. Enable hardware acceleration or try another browser.');
 const renderer=new WebGLRenderer({canvas,context,antialias:false});
 renderer.autoClear=false;
 const texWidth=Math.ceil(CONFIG_WORDS/4);
 const dummyTex=new DataTexture(new Uint32Array(texWidth*4),texWidth,1,RGBAIntegerFormat,UnsignedIntType);
 dummyTex.needsUpdate=true;
 const uniforms={
   configWords:{value:new Uint32Array(CONFIG_WORDS)},
   configTex:{value:dummyTex},
   numConfigs:{value:1},
   fightsPerConfig:{value:0},
   seed:{value:0},offset:{value:0},count:{value:0},gridWidth:{value:0},mode:{value:0},eventBudget:{value:0}
 };
 const material=new RawShaderMaterial({glslVersion:GLSL3,vertexShader:VERTEX,fragmentShader:FRAGMENT,uniforms,depthTest:false,depthWrite:false,blending:NoBlending});
 const geometry=new BufferGeometry();
 geometry.setAttribute('position',new BufferAttribute(new Float32Array([-1,-1,0,3,-1,0,-1,3,0]),3));
 const mesh=new Mesh(geometry,material);mesh.frustumCulled=false;
 const scene=new Scene();scene.add(mesh);
 engine={renderer,gl:context,uniforms,material,geometry,scene,camera:new Camera(),error:null,compiled:false};
 const owned=engine;
 canvas.addEventListener('webglcontextlost',event=>{event.preventDefault();owned.error=new Error('WebGL context lost. Reload to run again.');});
 renderer.debug.onShaderError=(gl,program,vs,fs)=>{owned.error=new Error(`Simulation shader failed: ${gl.getProgramInfoLog(program)} ${gl.getShaderInfoLog(fs)} ${gl.getShaderInfoLog(vs)}`);};
 return engine;
}

export async function preloadShader() {
  try {
    const e = acquire();
    if (!e.compiled) {
      await e.renderer.compileAsync(e.scene, e.camera);
      if (e.error) throw e.error;
      e.compiled = true;
    }
    return true;
  } catch (err) {
    console.warn('Background shader compilation warning:', err);
    return false;
  }
}

export function disposeEngine(){
 if(running)throw new Error('Cannot dispose an active simulation.');
 if(engine){engine.material.dispose();engine.geometry.dispose();engine.renderer.dispose();engine.renderer.forceContextLoss();engine=null;}
}

function target(width,height){
  return new WebGLRenderTarget(width,height,{
    count: 4,
    format: RGBAIntegerFormat,
    type: UnsignedIntType,
    internalFormat: 'RGBA32UI',
    minFilter: NearestFilter,
    magFilter: NearestFilter,
    depthBuffer: false,
    stencilBuffer: false,
    samples: 0,
    generateMipmaps: false
  });
}

function createConfigTexture(configs){
 const texWidth=Math.ceil(CONFIG_WORDS/4),texHeight=configs.length;
 const data=new Uint32Array(texWidth*texHeight*4);
 for(let i=0;i<configs.length;i++){
  const packed=packConfig(configs[i]);
  for(let j=0;j<packed.length;j++)data[i*texWidth*4+j]=packed[j];
 }
 const tex=new DataTexture(data,texWidth,texHeight,RGBAIntegerFormat,UnsignedIntType);
 tex.minFilter=NearestFilter;tex.magFilter=NearestFilter;tex.generateMipmaps=false;tex.needsUpdate=true;
 return tex;
}

async function drawRead(e,rt,count,gridWidth,mode,offset,signal){
 checkAbort(signal);if(e.error)throw e.error;
 e.uniforms.count.value=count;e.uniforms.gridWidth.value=gridWidth;e.uniforms.mode.value=mode;e.uniforms.offset.value=offset;
 e.renderer.setRenderTarget(rt);
 if(e.gl.checkFramebufferStatus(e.gl.FRAMEBUFFER)!==e.gl.FRAMEBUFFER_COMPLETE)throw new Error('Integer result framebuffer is unsupported.');
 e.renderer.render(e.scene,e.camera);
 if(e.error)throw e.error;
 const attachments=[];
 // Keep readback sequential: Three.js owns pixel-pack buffer binding state.
 for(let i=0;i<4;i++)attachments.push(await e.renderer.readRenderTargetPixelsAsync(rt,0,0,rt.width,rt.height,new Uint32Array(rt.width*rt.height*4),undefined,i));
 if(e.gl.isContextLost())throw new Error('WebGL context lost during simulation.');
 const error=e.gl.getError();if(error!==e.gl.NO_ERROR)throw new Error(`WebGL execution failed (0x${error.toString(16)}).`);
 checkAbort(signal);return attachments;
}

function unpack(attachments,width,lane,words,mode=0){
 const result=new Uint32Array(words);
 if(mode===1){
  for(let j=0;j<words;j++)result[j]=attachments[Math.floor(j%16/4)][Math.floor(j/16)*4+j%4];
  return result;
 }
 const simX=lane%width,simY=Math.floor(lane/width);
 for(let j=0;j<words;j++){
  const stripe=Math.floor(j/16),att=Math.floor(j%16/4),comp=j%4;
  result[j]=attachments[att][((simY*COMPACT_STRIPES+stripe)*width+simX)*4+comp];
 }
 return result;
}

function decodeBatch2D(outputs,width,count,config,first,states){
 const att0=outputs[0],att1=outputs[1],att2=outputs[2],att3=outputs[3];
 const f0=new Float32Array(att0.buffer),f1=new Float32Array(att1.buffer),f2=new Float32Array(att2.buffer),f3=new Float32Array(att3.buffer);
 for(let lane=0;lane<count;lane++){
  const simX=lane%width,simY=Math.floor(lane/width);
  const p0=((simY*COMPACT_STRIPES)*width+simX)*4,p1=((simY*COMPACT_STRIPES+1)*width+simX)*4;
  const total=f0[p0];
  const doneWord=att0[p0+1];
  const done=doneWord&65535,highWater=doneWord>>>16;
  const events=att0[p0+2];
  const tapWord=att0[p0+3];
  const taps=tapWord&65535,procs=tapWord>>>16;
  if(done!==1||!Number.isFinite(total))throw new Error(`Fight ${first+lane} failed (status ${done}). Incomplete results rejected.`);
  const isbWord=att1[p0];
  const isbProcs=isbWord&65535,isbConsumed=isbWord>>>16;
  const petBrandDamage=f1[p0+1];
  const petDamage=f1[p0+2];
  const petCastsWord=att1[p0+3];
  const petCasts=petCastsWord&65535,rngCalls=petCastsWord>>>16;

  const damage0=f2[p0];
  const c0=att2[p0+1],casts0=c0&65535,hits0=c0>>>16;
  const cr0=att2[p0+2],crits0=cr0&65535,misses0=cr0>>>16;
  const damage1=f2[p0+3];

  const c1=att3[p0],casts1=c1&65535,hits1=c1>>>16;
  const cr1=att3[p0+1],crits1=cr1&65535,misses1=cr1>>>16;
  const damage2=f3[p0+2];
  const c2=att3[p0+3],casts2=c2&65535,hits2=c2>>>16;

  const cr2=att0[p1],crits2=cr2&65535,misses2=cr2>>>16;
  const damage3=f0[p1+1];
  const c3=att0[p1+2],casts3=c3&65535,hits3=c3>>>16;
  const cr3=att0[p1+3],crits3=cr3&65535,misses3=cr3>>>16;

  const damage4=f1[p1];
  const c4=att1[p1+1],casts4=c4&65535,hits4=c4>>>16;
  const cr4=att1[p1+2],crits4=cr4&65535,misses4=cr4>>>16;
  const damage5=f1[p1+3];

  const c5=att2[p1],casts5=c5&65535,hits5=c5>>>16;
  const cr5=att2[p1+1],crits5=cr5&65535,misses5=cr5>>>16;
  const petMeleeDamage=f2[p1+2];
  const pmc=att2[p1+3],petMeleeCasts=pmc&65535,petMeleeHits=pmc>>>16;

  const pmcr=att3[p1],petMeleeCrits=pmcr&65535,petMeleeMisses=pmcr>>>16;
  const petSpellDamage=f3[p1+1];
  const psc=att3[p1+2],petSpellCasts=psc&65535,petSpellHits=psc>>>16;
  const pscr=att3[p1+3],petSpellCrits=pscr&65535,petSpellMisses=pscr>>>16;

  const s={
   total,done,highWater,events,taps,procs,isbProcs,isbConsumed,
   petBrandDamage,petDamage,petCasts,rngCalls,
   damage0,casts0,hits0,crits0,misses0,
   damage1,casts1,hits1,crits1,misses1,
   damage2,casts2,hits2,crits2,misses2,
   damage3,casts3,hits3,crits3,misses3,
   damage4,casts4,hits4,crits4,misses4,
   damage5,casts5,hits5,crits5,misses5,
   petMeleeDamage,petMeleeCasts,petMeleeHits,petMeleeCrits,petMeleeMisses,
   petSpellDamage,petSpellCasts,petSpellHits,petSpellCrits,petSpellMisses
  };
  const words=unpack(outputs,width,lane,COMPACT_WORDS);
  const floats=new Float32Array(words.buffer);
  for(let i=6;i<SPELLS.length;i++){
   const j=32+(i-6)*3;
   s[`damage${i}`]=floats[j];s[`casts${i}`]=words[j+1]&65535;s[`hits${i}`]=words[j+1]>>>16;
   s[`crits${i}`]=words[j+2]&65535;s[`misses${i}`]=words[j+2]>>>16;
  }
  states.push(s);
 }
}

export function summarize(states, duration) {
  const dps = states.map(s => s.total / duration).sort((a,b) => a-b);
  let mean = 0, m2 = 0;
  dps.forEach((value,i) => { const delta = value - mean; mean += delta / (i+1); m2 += delta * (value - mean); });
  const sd = dps.length > 1 ? Math.sqrt(m2 / (dps.length-1)) : 0;
  const sum = key => states.reduce((n,s) => n + (s[key] || 0), 0);
  
  const spells = SPELLS.map((name,i) => ({
    name,
    damage: sum(`damage${i}`) / states.length,
    casts: sum(`casts${i}`) / states.length,
    hits: sum(`hits${i}`) / states.length,
    crits: sum(`crits${i}`) / states.length,
    misses: sum(`misses${i}`) / states.length,
    school: ['Immolate', 'Incinerate', 'Searing Pain', 'Soul Fire', 'Conflagrate'].includes(name) ? 'fire' : 'shadow'
  }));

  const petMeleeDamage = sum('petMeleeDamage') / states.length;
  const petMeleeCasts = sum('petMeleeCasts') / states.length;
  const petMeleeHits = sum('petMeleeHits') / states.length;
  const petMeleeCrits = sum('petMeleeCrits') / states.length;
  const petMeleeMisses = sum('petMeleeMisses') / states.length;

  const petSpellDamage = sum('petSpellDamage') / states.length;
  const petSpellCasts = sum('petSpellCasts') / states.length;
  const petSpellHits = sum('petSpellHits') / states.length;
  const petSpellCrits = sum('petSpellCrits') / states.length;
  const petSpellMisses = sum('petSpellMisses') / states.length;

  const petBrandDamage = sum('petBrandDamage') / states.length;
  const rawPetDamage = sum('petDamage') / states.length;
  const totalPetDmg = rawPetDamage > 0 ? rawPetDamage : (petMeleeDamage + petSpellDamage + petBrandDamage);

  // Succubus Melee swing damage
  if (petMeleeDamage > 0 || petMeleeCasts > 0) {
    spells.push({
      name: 'Succubus Melee',
      damage: petMeleeDamage,
      casts: petMeleeCasts,
      hits: petMeleeHits,
      crits: petMeleeCrits,
      misses: petMeleeMisses,
      school: 'physical'
    });
  }

  // Pet spell damage (Imp Firebolt or Succubus Lash of Pain)
  if (petSpellDamage > 0 || petSpellCasts > 0) {
    const isSuccubus = (petMeleeDamage > 0 || petMeleeCasts > 0);
    const petSpellName = isSuccubus ? 'Succubus Lash of Pain' : 'Imp Firebolt';
    const petSchool = isSuccubus ? 'shadow' : 'fire';
    spells.push({
      name: petSpellName,
      damage: petSpellDamage,
      casts: petSpellCasts,
      hits: petSpellHits,
      crits: petSpellCrits,
      misses: petSpellMisses,
      school: petSchool
    });
  }

  // Demonic Brand proc damage
  if (petBrandDamage > 0) {
    spells.push({
      name: 'Demonic Brand',
      damage: petBrandDamage,
      casts: 0,
      hits: 0,
      crits: 0,
      misses: 0,
      school: 'fire'
    });
  }

  const shadowDmg = (spells[0]?.damage || 0) + (spells[1]?.damage || 0) + (spells[2]?.damage || 0) + (petMeleeDamage > 0 ? petSpellDamage : 0);
  const fireDmg = (spells[3]?.damage || 0) + (spells[4]?.damage || 0) + (spells[5]?.damage || 0) + (petMeleeDamage === 0 ? petSpellDamage : 0) + petBrandDamage;
  const physicalDmg = petMeleeDamage;
  const totalDamage = shadowDmg + fireDmg + physicalDmg;

  return {
    count: states.length,
    mean,
    sd,
    ci95: 1.96 * sd / Math.sqrt(dps.length),
    p05: dps[Math.floor((dps.length-1)*.05)],
    p50: dps[Math.floor((dps.length-1)*.5)],
    p95: dps[Math.floor((dps.length-1)*.95)],
    dps,
    events: sum('events'),
    taps: sum('taps') / states.length,
    damage: totalDamage,
    shadowDamage: shadowDmg,
    fireDamage: fireDmg,
    physicalDamage: physicalDmg,
    petDamage: totalPetDmg,
    petMeleeDamage,
    petSpellDamage,
    petBrandDamage,
    maxHeap: states.reduce((n,s)=>Math.max(n,s.highWater || 0),0),
    spells
  };
}

export async function runMultiSimulation(inputs, { signal, onProgress = () => {}, batchSize = 524288, iterations = null, eventBudget } = {}) {
  if (!Array.isArray(inputs) || inputs.length === 0) throw new Error('Inputs must be a non-empty array of configs.');
  const configs = inputs.map(c => validate(iterations == null ? c : { ...c, iterations }));
  const numConfigs = configs.length;
  const fightsPerConfig = configs[0].iterations;
  if (configs.some(c => c.iterations !== fightsPerConfig)) throw new Error('Multi-config runs require the same iteration count for every config.');
  const totalFights = numConfigs * fightsPerConfig;
  const maxDuration = Math.max(...configs.map(c => c.duration));
  const budget = eventBudget ?? (maxDuration * 32 + 1024);
  const effectiveBatch = batchSize ?? 524288;

  if (running) throw new Error('A simulation is already running.');
  running = true;
  const started = performance.now();
  let rt, cfgTex;
  try {
    checkAbort(signal);
    onProgress({ phase: 'Preparing WebGL2 Multi-Config GPU Kernel', completed: 0, total: totalFights });
    const e = acquire();
    if (e.error) throw e.error;

    cfgTex = createConfigTexture(configs);
    e.uniforms.numConfigs.value = numConfigs;
    e.uniforms.fightsPerConfig.value = fightsPerConfig;
    e.uniforms.configTex.value = cfgTex;
    e.uniforms.seed.value = configs[0].seed || 42;
    e.uniforms.eventBudget.value = budget;

    const maxTexSize = e.gl.getParameter(e.gl.MAX_TEXTURE_SIZE);
    const capacity = Math.min(effectiveBatch, totalFights);
    const gridWidth = Math.min(maxTexSize, Math.max(1, Math.min(1024, capacity)));
    const gridHeight = Math.ceil(capacity / gridWidth);
    if (gridHeight * COMPACT_STRIPES > maxTexSize) throw new Error(`Simulation batch exceeds maximum texture height (${maxTexSize}).`);

    rt = target(gridWidth, gridHeight * COMPACT_STRIPES);
    e.renderer.setRenderTarget(rt);

    let compileMs = 0;
    if (!e.compiled) {
      onProgress({ phase: 'Compiling GPU Shader', completed: 0, total: totalFights });
      const t = performance.now();
      await e.renderer.compileAsync(e.scene, e.camera);
      compileMs = performance.now() - t;
      if (e.error) throw e.error;
      e.compiled = true;
    }

    let executeMs = 0, draws = 0;
    const states = [];
    for (let first = 0; first < totalFights; first += capacity) {
      const n = Math.min(capacity, totalFights - first);
      const before = performance.now();
      const outputs = await drawRead(e, rt, n, gridWidth, 0, first, signal);
      executeMs += performance.now() - before;
      draws++;
      decodeBatch2D(outputs, gridWidth, n, configs[0], first, states);
      onProgress({ phase: 'Simulating All Specs', completed: first + n, total: totalFights });
      if (first + n < totalFights) await yieldUI();
    }

    const results = [];
    for (let i = 0; i < numConfigs; i++) {
      const cfgStates = states.slice(i * fightsPerConfig, (i + 1) * fightsPerConfig);
      const summary = summarize(cfgStates, configs[i].duration);
      results.push({ config: configs[i], summary, states: cfgStates });
    }

    const elapsedMs = performance.now() - started;
    return {
      results,
      timing: { elapsedMs, executeMs, compileMs, draws, totalFights },
      adapter: e.gl.getParameter(e.gl.RENDERER),
      engine: 'Three.js 0.180.0 / WebGL2 Multi-Config GLSL DES'
    };
  } finally {
    rt?.dispose();
    cfgTex?.dispose();
    if (engine?.uniforms) {
      engine.uniforms.numConfigs.value = 1;
      engine.uniforms.fightsPerConfig.value = 0;
    }
    engine?.renderer.setRenderTarget(null);
    running = false;
  }
}

export async function runSimulation(input,{signal,onProgress=()=>{},batchSize=524288,trace=true,eventBudget}={}){
 if (Array.isArray(input)) return runMultiSimulation(input, { signal, onProgress, batchSize, eventBudget });
 const config=validate(input);
 const effectiveBatch=batchSize??524288;
 if(!Number.isInteger(effectiveBatch)||effectiveBatch<1||effectiveBatch>1048576)throw new Error('Batch size must be 1–1048576.');
 const budget=eventBudget??(config.duration*32+1024);
 if(!Number.isInteger(budget)||budget<1||budget>100000)throw new Error('Invalid event budget.');
 if(running)throw new Error('A simulation is already running.');
 running=true;const started=performance.now();let rt,diagnostic,traceTarget;
 try{
  checkAbort(signal);onProgress({phase:'Preparing WebGL2',completed:0,total:config.iterations});
  const e=acquire();if(e.error)throw e.error;
  e.uniforms.numConfigs.value=1;
  e.uniforms.fightsPerConfig.value=config.iterations;
  e.uniforms.configWords.value=packConfig(config);e.uniforms.seed.value=config.seed;e.uniforms.eventBudget.value=budget;
  const maxTexSize=e.gl.getParameter(e.gl.MAX_TEXTURE_SIZE);
  const capacity=Math.min(effectiveBatch,config.iterations);
  const gridWidth=Math.min(maxTexSize,Math.max(1,Math.min(1024,capacity)));
  const gridHeight=Math.ceil(capacity/gridWidth);
  if(gridHeight*COMPACT_STRIPES>maxTexSize)throw new Error(`Simulation batch exceeds maximum texture height (${maxTexSize}).`);
  rt=target(gridWidth,gridHeight*COMPACT_STRIPES);e.renderer.setRenderTarget(rt);
  let compileMs=0;
  if(!e.compiled){
    onProgress({phase:'Compiling GPU Shader',completed:0,total:config.iterations});
    const t=performance.now();await e.renderer.compileAsync(e.scene,e.camera);compileMs=performance.now()-t;if(e.error)throw e.error;e.compiled=true;
  }
  let executeMs=0,diagnosticMs=0,draws=0;
  const states=[];
  for(let first=0;first<config.iterations;first+=capacity){
   const n=Math.min(capacity,config.iterations-first),before=performance.now();
   const outputs=await drawRead(e,rt,n,gridWidth,0,first,signal);executeMs+=performance.now()-before;draws++;
   decodeBatch2D(outputs,gridWidth,n,config,first,states);
   onProgress({phase:'Simulating',completed:first+n,total:config.iterations});
   if(first+n<config.iterations)await yieldUI();
  }
  const before=performance.now();
  diagnostic=target(1,Math.ceil(STATE_WORDS/16));
  const full=await drawRead(e,diagnostic,1,1,1,0,signal);draws++;
  const first=decodeStates(unpack(full,1,0,STATE_WORDS,1).buffer,1)[0];
  for(const [key,value]of Object.entries(states[0]))if(first[key]!==value)throw new Error(`Diagnostic replay differs at ${key}.`);
  states[0]=first;
  const traceLength=trace?Math.min(first.events,TRACE_CAPACITY):0,log=[];
  if(traceLength){
   traceTarget=target(traceLength,1);
   const outputs=await drawRead(e,traceTarget,traceLength,traceLength,2,0,signal);draws++;
   const floats=outputs.map(a=>new Float32Array(a.buffer));
   for(let i=0;i<traceLength;i++){const j=i*4;log.push({time:outputs[0][j]/1e6,kind:outputs[0][j+1],spell:outputs[0][j+2],damage:floats[0][j+3],mana:floats[1][j],flags:outputs[1][j+1],total:floats[1][j+2],rngCalls:outputs[1][j+3]});}
  }
  diagnosticMs=performance.now()-before;checkAbort(signal);
  const summary=summarize(states,config.duration),elapsedMs=performance.now()-started;
  return{config,summary,states,trace:log,traceTruncated:trace&&first.events>traceLength,
   timing:{elapsedMs,executeMs,compileMs,diagnosticMs,draws,capacity},
   adapter:e.gl.getParameter(e.gl.RENDERER),engine:'Three.js 0.180.0 / WebGL2 GLSL ES 3.00 DES',scope:'single-target-core-v1'};
 }finally{rt?.dispose();diagnostic?.dispose();traceTarget?.dispose();engine?.renderer.setRenderTarget(null);running=false;}
}
