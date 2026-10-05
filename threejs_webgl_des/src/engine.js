import { WebGLRenderer, WebGLRenderTarget, RawShaderMaterial, BufferGeometry, BufferAttribute,
  Mesh, Scene, Camera, GLSL3, RGBAIntegerFormat, UnsignedIntType, NearestFilter, NoBlending, DataTexture } from 'three';
import { COMPACT_STRIPES, validate, packConfig, decodeStates, STATE_WORDS, TRACE_CAPACITY, SPELLS, CONFIG, CONFIG_WORDS } from './model.js';
import { VERTEX, FRAGMENT, FAST_FRAGMENT } from './kernel.js';

const yieldUI=()=>new Promise(resolve=>setTimeout(resolve,0));
function checkAbort(signal){if(signal?.aborted)throw new DOMException('Run cancelled.','AbortError');}
let engine=null, running=false;

function acquire(){
 if(engine)return engine;
 const canvas=document.createElement('canvas');
 const context=canvas.getContext('webgl2',{alpha:false,antialias:false,depth:false,stencil:false,powerPreference:'high-performance'});
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
 const makeMaterial=fragmentShader=>new RawShaderMaterial({glslVersion:GLSL3,vertexShader:VERTEX,fragmentShader,uniforms,depthTest:false,depthWrite:false,blending:NoBlending});
 const material=makeMaterial(FRAGMENT),fastMaterial=makeMaterial(FAST_FRAGMENT);
 const geometry=new BufferGeometry();
 geometry.setAttribute('position',new BufferAttribute(new Float32Array([-1,-1,0,3,-1,0,-1,3,0]),3));
 const mesh=new Mesh(geometry,material);mesh.frustumCulled=false;
 const scene=new Scene();scene.add(mesh);
 engine={renderer,gl:context,uniforms,material,fastMaterial,mesh,geometry,scene,camera:new Camera(),error:null,compiledVariants:new Set()};
 const owned=engine;
 canvas.addEventListener('webglcontextlost',event=>{event.preventDefault();owned.error=new Error('WebGL context lost. Reload to run again.');});
 renderer.debug.onShaderError=(gl,program,vs,fs)=>{owned.error=new Error(`Simulation shader failed: ${gl.getProgramInfoLog(program)} ${gl.getShaderInfoLog(fs)} ${gl.getShaderInfoLog(vs)}`);};
 return engine;
}

export async function preloadShader() {
  try {
    const e = acquire();
    if (!e.compiledVariants.has(true)) {
      e.mesh.material=e.material;
      await e.renderer.compileAsync(e.scene, e.camera);
      if (e.error) throw e.error;
      e.compiledVariants.add(true);
    }
    return true;
  } catch (err) {
    console.warn('Background shader compilation warning:', err);
    return false;
  }
}

export function disposeEngine(){
 if(running)throw new Error('Cannot dispose an active simulation.');
 if(engine){engine.material.dispose();engine.fastMaterial.dispose();engine.geometry.dispose();engine.renderer.dispose();engine.renderer.forceContextLoss();engine=null;}
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

// Keep the literal state layout in sync with SPELLS and compactWord in kernel.js.
// A fixed object shape and shared views avoid per-fight copies and dictionary properties.
function decodeBatch2D(outputs,width,count,config,first,states){
 const att0=outputs[0],att1=outputs[1],att2=outputs[2],att3=outputs[3];
 const f0=new Float32Array(att0.buffer),f1=new Float32Array(att1.buffer),f2=new Float32Array(att2.buffer),f3=new Float32Array(att3.buffer);
 for(let lane=0;lane<count;lane++){
  const simX=lane%width,simY=Math.floor(lane/width);
  const p0=((simY*COMPACT_STRIPES)*width+simX)*4,p1=((simY*COMPACT_STRIPES+1)*width+simX)*4;
  const p2=p0+2*width*4,p3=p0+3*width*4;
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
   damage6:f0[p2],casts6:att0[p2+1]&65535,hits6:att0[p2+1]>>>16,crits6:att0[p2+2]&65535,misses6:att0[p2+2]>>>16,
   damage7:f0[p2+3],casts7:att1[p2]&65535,hits7:att1[p2]>>>16,crits7:att1[p2+1]&65535,misses7:att1[p2+1]>>>16,
   damage8:f1[p2+2],casts8:att1[p2+3]&65535,hits8:att1[p2+3]>>>16,crits8:att2[p2]&65535,misses8:att2[p2]>>>16,
   damage9:f2[p2+1],casts9:att2[p2+2]&65535,hits9:att2[p2+2]>>>16,crits9:att2[p2+3]&65535,misses9:att2[p2+3]>>>16,
   damage10:f3[p2],casts10:att3[p2+1]&65535,hits10:att3[p2+1]>>>16,crits10:att3[p2+2]&65535,misses10:att3[p2+2]>>>16,
   damage11:f3[p2+3],casts11:att0[p3]&65535,hits11:att0[p3]>>>16,crits11:att0[p3+1]&65535,misses11:att0[p3+1]>>>16,
   damage12:f0[p3+2],casts12:att0[p3+3]&65535,hits12:att0[p3+3]>>>16,crits12:att1[p3]&65535,misses12:att1[p3]>>>16,
   damage0,casts0,hits0,crits0,misses0,
   damage1,casts1,hits1,crits1,misses1,
   damage2,casts2,hits2,crits2,misses2,
   damage3,casts3,hits3,crits3,misses3,
   damage4,casts4,hits4,crits4,misses4,
   damage5,casts5,hits5,crits5,misses5,
   petMeleeDamage,petMeleeCasts,petMeleeHits,petMeleeCrits,petMeleeMisses,
   petSpellDamage,petSpellCasts,petSpellHits,petSpellCrits,petSpellMisses
  };
  states.push(s);
 }
}

// Reuse views across the batch; allocate only the returned state per fight.
function decodeFastBatch(outputs,count,first,states){
 const [a,b,c,d]=outputs;
 const fa=new Float32Array(a.buffer),fb=new Float32Array(b.buffer),fc=new Float32Array(c.buffer);
 for(let lane=0;lane<count;lane++){
  const p=lane*4,done=a[p+1]&65535,total=fa[p];
  if(done!==1||!Number.isFinite(total))throw new Error(`Fight ${first+lane} failed (status ${done}). Incomplete results rejected.`);
  states.push({
   total,done,highWater:a[p+1]>>>16,events:a[p+2],taps:a[p+3]&65535,procs:a[p+3]>>>16,
   isbProcs:b[p]&65535,isbConsumed:b[p]>>>16,petBrandDamage:fb[p+1],petDamage:fb[p+2],petCasts:b[p+3]&65535,rngCalls:b[p+3]>>>16,
   petMeleeDamage:fc[p],petMeleeCasts:c[p+1]&65535,petMeleeHits:c[p+1]>>>16,petMeleeCrits:c[p+2]&65535,petMeleeMisses:c[p+2]>>>16,
   petSpellDamage:fc[p+3],petSpellCasts:d[p]&65535,petSpellHits:d[p]>>>16,petSpellCrits:d[p+1]&65535,petSpellMisses:d[p+1]>>>16,detailed:false
  });
 }
}

// Sum adjacent fields in one pass. Repeated dynamic-key reductions walk the
// entire (large) state array once per counter and dominate detailed runs.
function sumStates(states, detailed){
 const totals={
  total:0,events:0,taps:0,petDamage:0,petBrandDamage:0,
  petMeleeDamage:0,petMeleeCasts:0,petMeleeHits:0,petMeleeCrits:0,petMeleeMisses:0,
  petSpellDamage:0,petSpellCasts:0,petSpellHits:0,petSpellCrits:0,petSpellMisses:0,
  damage0:0,casts0:0,hits0:0,crits0:0,misses0:0,
  damage1:0,casts1:0,hits1:0,crits1:0,misses1:0,
  damage2:0,casts2:0,hits2:0,crits2:0,misses2:0,
  damage3:0,casts3:0,hits3:0,crits3:0,misses3:0,
  damage4:0,casts4:0,hits4:0,crits4:0,misses4:0,
  damage5:0,casts5:0,hits5:0,crits5:0,misses5:0,
  damage6:0,casts6:0,hits6:0,crits6:0,misses6:0,
  damage7:0,casts7:0,hits7:0,crits7:0,misses7:0,
  damage8:0,casts8:0,hits8:0,crits8:0,misses8:0,
  damage9:0,casts9:0,hits9:0,crits9:0,misses9:0,
  damage10:0,casts10:0,hits10:0,crits10:0,misses10:0,
  damage11:0,casts11:0,hits11:0,crits11:0,misses11:0,
  damage12:0,casts12:0,hits12:0,crits12:0,misses12:0,
 };
 for(const s of states){
  totals.total+=s.total||0;
  totals.events+=s.events||0;
  totals.taps+=s.taps||0;
  totals.petDamage+=s.petDamage||0;
  totals.petBrandDamage+=s.petBrandDamage||0;
  totals.petMeleeDamage+=s.petMeleeDamage||0;
  totals.petMeleeCasts+=s.petMeleeCasts||0;
  totals.petMeleeHits+=s.petMeleeHits||0;
  totals.petMeleeCrits+=s.petMeleeCrits||0;
  totals.petMeleeMisses+=s.petMeleeMisses||0;
  totals.petSpellDamage+=s.petSpellDamage||0;
  totals.petSpellCasts+=s.petSpellCasts||0;
  totals.petSpellHits+=s.petSpellHits||0;
  totals.petSpellCrits+=s.petSpellCrits||0;
  totals.petSpellMisses+=s.petSpellMisses||0;
  if(detailed){
   totals.damage0+=s.damage0||0;totals.casts0+=s.casts0||0;totals.hits0+=s.hits0||0;totals.crits0+=s.crits0||0;totals.misses0+=s.misses0||0;
   totals.damage1+=s.damage1||0;totals.casts1+=s.casts1||0;totals.hits1+=s.hits1||0;totals.crits1+=s.crits1||0;totals.misses1+=s.misses1||0;
   totals.damage2+=s.damage2||0;totals.casts2+=s.casts2||0;totals.hits2+=s.hits2||0;totals.crits2+=s.crits2||0;totals.misses2+=s.misses2||0;
   totals.damage3+=s.damage3||0;totals.casts3+=s.casts3||0;totals.hits3+=s.hits3||0;totals.crits3+=s.crits3||0;totals.misses3+=s.misses3||0;
   totals.damage4+=s.damage4||0;totals.casts4+=s.casts4||0;totals.hits4+=s.hits4||0;totals.crits4+=s.crits4||0;totals.misses4+=s.misses4||0;
   totals.damage5+=s.damage5||0;totals.casts5+=s.casts5||0;totals.hits5+=s.hits5||0;totals.crits5+=s.crits5||0;totals.misses5+=s.misses5||0;
   totals.damage6+=s.damage6||0;totals.casts6+=s.casts6||0;totals.hits6+=s.hits6||0;totals.crits6+=s.crits6||0;totals.misses6+=s.misses6||0;
   totals.damage7+=s.damage7||0;totals.casts7+=s.casts7||0;totals.hits7+=s.hits7||0;totals.crits7+=s.crits7||0;totals.misses7+=s.misses7||0;
   totals.damage8+=s.damage8||0;totals.casts8+=s.casts8||0;totals.hits8+=s.hits8||0;totals.crits8+=s.crits8||0;totals.misses8+=s.misses8||0;
   totals.damage9+=s.damage9||0;totals.casts9+=s.casts9||0;totals.hits9+=s.hits9||0;totals.crits9+=s.crits9||0;totals.misses9+=s.misses9||0;
   totals.damage10+=s.damage10||0;totals.casts10+=s.casts10||0;totals.hits10+=s.hits10||0;totals.crits10+=s.crits10||0;totals.misses10+=s.misses10||0;
   totals.damage11+=s.damage11||0;totals.casts11+=s.casts11||0;totals.hits11+=s.hits11||0;totals.crits11+=s.crits11||0;totals.misses11+=s.misses11||0;
   totals.damage12+=s.damage12||0;totals.casts12+=s.casts12||0;totals.hits12+=s.hits12||0;totals.crits12+=s.crits12||0;totals.misses12+=s.misses12||0;
  }
 }
 return totals;
}

export function summarize(states, duration) {
  // Typed-array numeric sorting avoids a JS comparator call for every comparison.
  const sortedDps = Float64Array.from(states, s => s.total / duration);
  sortedDps.sort();
  const dps = Array.from(sortedDps);
  let mean = 0, m2 = 0;
  dps.forEach((value,i) => { const delta = value - mean; mean += delta / (i+1); m2 += delta * (value - mean); });
  const sd = dps.length > 1 ? Math.sqrt(m2 / (dps.length-1)) : 0;
  const detailed = states[0]?.detailed !== false;
  const totals = sumStates(states, detailed);
  const sum = key => totals[key] || 0;
  const spells = detailed ? SPELLS.map((name,i) => ({
    name,
    damage: sum(`damage${i}`) / states.length,
    casts: sum(`casts${i}`) / states.length,
    hits: sum(`hits${i}`) / states.length,
    crits: sum(`crits${i}`) / states.length,
    misses: sum(`misses${i}`) / states.length,
    ...(name === 'Hellfire' ? { selfDamage: sum(`hits${i}`) / states.length * 210 } : {}),
    school: ['Immolate', 'Incinerate', 'Searing Pain', 'Soul Fire', 'Conflagrate', 'Hellfire'].includes(name) ? 'fire' : 'shadow'
  })) : [];

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
  if (detailed && (petMeleeDamage > 0 || petMeleeCasts > 0)) {
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
  if (detailed && (petSpellDamage > 0 || petSpellCasts > 0)) {
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
  if (detailed && petBrandDamage > 0) {
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
  const fireDmg = (spells[3]?.damage || 0) + (spells[4]?.damage || 0) + (spells[5]?.damage || 0) + (petMeleeDamage === 0 ? petSpellDamage : 0) + petBrandDamage + (spells[13]?.damage || 0);
  const physicalDmg = petMeleeDamage;
  const totalDamage = detailed ? shadowDmg + fireDmg + physicalDmg : sum('total') / states.length;

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
    detailed,
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

export async function runMultiSimulation(inputs, { signal, onProgress = () => {}, batchSize = 524288, iterations = null, eventBudget, detailedResults = true } = {}) {
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
    e.mesh.material = detailedResults ? e.material : e.fastMaterial;

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
    const outputStripes = detailedResults ? COMPACT_STRIPES : 1;
    if (gridHeight * outputStripes > maxTexSize) throw new Error(`Simulation batch exceeds maximum texture height (${maxTexSize}).`);

    rt = target(gridWidth, gridHeight * outputStripes);
    e.renderer.setRenderTarget(rt);

    let compileMs = 0;
    if (!e.compiledVariants.has(!!detailedResults)) {
      onProgress({ phase: 'Compiling GPU Shader', completed: 0, total: totalFights });
      const t = performance.now();
      await e.renderer.compileAsync(e.scene, e.camera);
      compileMs = performance.now() - t;
      if (e.error) throw e.error;
      e.compiledVariants.add(!!detailedResults);
    }

    let executeMs = 0, decodeMs = 0, draws = 0;
    const states = [];
    for (let first = 0; first < totalFights; first += capacity) {
      const n = Math.min(capacity, totalFights - first);
      const before = performance.now();
      const outputs = await drawRead(e, rt, n, gridWidth, 0, first, signal);
      executeMs += performance.now() - before;
      draws++;
      const decodeStarted = performance.now();
      if (detailedResults) decodeBatch2D(outputs, gridWidth, n, configs[0], first, states);
      else decodeFastBatch(outputs, n, first, states);
      decodeMs += performance.now() - decodeStarted;
      onProgress({ phase: 'Simulating All Specs', completed: first + n, total: totalFights });
      if (first + n < totalFights) await yieldUI();
    }

    const summaryStarted = performance.now();
    const results = [];
    for (let i = 0; i < numConfigs; i++) {
      const cfgStates = states.slice(i * fightsPerConfig, (i + 1) * fightsPerConfig);
      const summary = summarize(cfgStates, configs[i].duration);
      results.push({ config: configs[i], summary, states: cfgStates });
    }

    const summaryMs = performance.now() - summaryStarted;
    const elapsedMs = performance.now() - started;
    return {
      results,
      timing: { elapsedMs, executeMs, decodeMs, summaryMs, compileMs, draws, totalFights },
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

export async function runSimulation(input,{signal,onProgress=()=>{},batchSize=524288,trace=true,eventBudget,detailedResults=true}={}){
 if (Array.isArray(input)) return runMultiSimulation(input, { signal, onProgress, batchSize, eventBudget, detailedResults });
 const config=validate(input);
 const effectiveBatch=batchSize??524288;
 if(!Number.isInteger(effectiveBatch)||effectiveBatch<1||effectiveBatch>1048576)throw new Error('Batch size must be 1–1048576.');
 const budget=eventBudget??(config.duration*32+1024);
 if(!Number.isInteger(budget)||budget<1||budget>100000)throw new Error('Invalid event budget.');
 if(running)throw new Error('A simulation is already running.');
 running=true;const started=performance.now();let rt,diagnostic,traceTarget;
 try{
  checkAbort(signal);onProgress({phase:'Preparing WebGL2',completed:0,total:config.iterations});
  const e=acquire();if(e.error)throw e.error;e.mesh.material=detailedResults?e.material:e.fastMaterial;
  e.uniforms.numConfigs.value=1;
  e.uniforms.fightsPerConfig.value=config.iterations;
  e.uniforms.configWords.value=packConfig(config);e.uniforms.seed.value=config.seed;e.uniforms.eventBudget.value=budget;
  const maxTexSize=e.gl.getParameter(e.gl.MAX_TEXTURE_SIZE);
  const capacity=Math.min(effectiveBatch,config.iterations);
  const gridWidth=Math.min(maxTexSize,Math.max(1,Math.min(1024,capacity)));
  const gridHeight=Math.ceil(capacity/gridWidth);
  const outputStripes=detailedResults?COMPACT_STRIPES:1;
  if(gridHeight*outputStripes>maxTexSize)throw new Error(`Simulation batch exceeds maximum texture height (${maxTexSize}).`);
  rt=target(gridWidth,gridHeight*outputStripes);e.renderer.setRenderTarget(rt);
  let compileMs=0;
  if(!e.compiledVariants.has(!!detailedResults)){
    onProgress({phase:'Compiling GPU Shader',completed:0,total:config.iterations});
    const t=performance.now();await e.renderer.compileAsync(e.scene,e.camera);compileMs=performance.now()-t;if(e.error)throw e.error;e.compiledVariants.add(!!detailedResults);
  }
  let executeMs=0,decodeMs=0,diagnosticMs=0,draws=0;
  const states=[];
  for(let first=0;first<config.iterations;first+=capacity){
   const n=Math.min(capacity,config.iterations-first),before=performance.now();
   const outputs=await drawRead(e,rt,n,gridWidth,0,first,signal);executeMs+=performance.now()-before;draws++;
   const decodeStarted=performance.now();
   if(detailedResults)decodeBatch2D(outputs,gridWidth,n,config,first,states);else decodeFastBatch(outputs,n,first,states);
   decodeMs+=performance.now()-decodeStarted;
   onProgress({phase:'Simulating',completed:first+n,total:config.iterations});
   if(first+n<config.iterations)await yieldUI();
  }
  const before=performance.now();
  if(!detailedResults){
   const traceLength=trace?Math.min(states[0].events,TRACE_CAPACITY):0,log=[];
   if(traceLength){traceTarget=target(traceLength,1);const outputs=await drawRead(e,traceTarget,traceLength,traceLength,2,0,signal);draws++;const floats=outputs.map(a=>new Float32Array(a.buffer));for(let i=0;i<traceLength;i++){const j=i*4;log.push({time:outputs[0][j]/1e6,kind:outputs[0][j+1],spell:outputs[0][j+2],damage:floats[0][j+3],mana:floats[1][j],flags:outputs[1][j+1],total:floats[1][j+2],rngCalls:outputs[1][j+3]});}}
   diagnosticMs=performance.now()-before;checkAbort(signal);const summaryStarted=performance.now(),summary=summarize(states,config.duration),summaryMs=performance.now()-summaryStarted,elapsedMs=performance.now()-started;
   return{config,summary,states,trace:log,traceTruncated:trace&&states[0].events>traceLength,timing:{elapsedMs,executeMs,decodeMs,summaryMs,compileMs,diagnosticMs,draws,capacity},adapter:e.gl.getParameter(e.gl.RENDERER),engine:'Three.js 0.180.0 / WebGL2 GLSL ES 3.00 DES',scope:'single-target-core-v1',detailedResults:false};
  }
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
  const summaryStarted=performance.now(),summary=summarize(states,config.duration),summaryMs=performance.now()-summaryStarted,elapsedMs=performance.now()-started;
  return{config,summary,states,trace:log,traceTruncated:trace&&first.events>traceLength,
   timing:{elapsedMs,executeMs,decodeMs,summaryMs,compileMs,diagnosticMs,draws,capacity},
   adapter:e.gl.getParameter(e.gl.RENDERER),engine:'Three.js 0.180.0 / WebGL2 GLSL ES 3.00 DES',scope:'single-target-core-v1'};
 }finally{rt?.dispose();diagnostic?.dispose();traceTarget?.dispose();engine?.renderer.setRenderTarget(null);running=false;}
}
