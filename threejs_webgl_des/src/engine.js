import { WebGLRenderer, WebGLRenderTarget, RawShaderMaterial, BufferGeometry, BufferAttribute,
  Mesh, Scene, Camera, GLSL3, RGBAIntegerFormat, UnsignedIntType, NearestFilter, NoBlending, DataTexture } from 'three';
import { validate, packConfig, decodeStates, STATE_WORDS, TRACE_CAPACITY, SPELLS, CONFIG, CONFIG_WORDS } from './model.js';
import { VERTEX, buildFragmentShader } from './kernel.js';

const yieldUI=()=>new Promise(resolve=>setTimeout(resolve,0));
function checkAbort(signal){if(signal?.aborted)throw new DOMException('Run cancelled.','AbortError');}
let engine=null, running=false;

function testMrtSupport(gl) {
  try {
    const maxDrawBuffers = gl.getParameter(gl.MAX_DRAW_BUFFERS);
    if (maxDrawBuffers < 4) return false;
    const isMobile = /Android|iPhone|iPad|iPod|Mobile|Silk/i.test(navigator?.userAgent || '') ||
                     (navigator?.maxTouchPoints > 1 && /Macintosh/i.test(navigator?.userAgent || ''));
    const fbo = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    const texes = [];
    for (let i = 0; i < 4; i++) {
      const tex = gl.createTexture();
      gl.bindTexture(gl.TEXTURE_2D, tex);
      gl.texStorage2D(gl.TEXTURE_2D, 1, gl.RGBA32UI, 4, 4);
      gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0 + i, gl.TEXTURE_2D, tex, 0);
      texes.push(tex);
    }
    gl.drawBuffers([gl.COLOR_ATTACHMENT0, gl.COLOR_ATTACHMENT1, gl.COLOR_ATTACHMENT2, gl.COLOR_ATTACHMENT3]);
    const status = gl.checkFramebufferStatus(gl.FRAMEBUFFER);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.deleteFramebuffer(fbo);
    for (const tex of texes) gl.deleteTexture(tex);
    if (status !== gl.FRAMEBUFFER_COMPLETE) return false;
    if (isMobile) return false;
    return true;
  } catch {
    return false;
  }
}

function acquire(){
 if(engine)return engine;
 const canvas=document.createElement('canvas');
 const context=canvas.getContext('webgl2',{alpha:false,antialias:false,depth:false,stencil:false});
 if(!context)throw new Error('WebGL2 is unavailable. Enable hardware acceleration or try another browser.');
 const renderer=new WebGLRenderer({canvas,context,antialias:false});
 renderer.autoClear=false;
 const use4Mrt = testMrtSupport(context);
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
 const fragmentShader = buildFragmentShader(!use4Mrt);
 const material=new RawShaderMaterial({glslVersion:GLSL3,vertexShader:VERTEX,fragmentShader,uniforms,depthTest:false,depthWrite:false,blending:NoBlending});
 const geometry=new BufferGeometry();
 geometry.setAttribute('position',new BufferAttribute(new Float32Array([-1,-1,0,3,-1,0,-1,3,0]),3));
 const mesh=new Mesh(geometry,material);mesh.frustumCulled=false;
 const scene=new Scene();scene.add(mesh);
 engine={renderer,gl:context,uniforms,material,geometry,scene,camera:new Camera(),error:null,compiled:false,use4Mrt};
 const owned=engine;
 canvas.addEventListener('webglcontextlost',event=>{event.preventDefault();owned.error=new Error('WebGL context lost. Reload to run again.');});
 renderer.debug.onShaderError=(gl,program,vs,fs)=>{owned.error=new Error(`Simulation shader failed: ${gl.getProgramInfoLog(program)} ${gl.getShaderInfoLog(fs)} ${gl.getShaderInfoLog(vs)}`);};
 return engine;
}

export function getEngineMode() {
  try {
    const e = acquire();
    return e.use4Mrt ? '4-MRT Mode' : 'Mobile Striped Mode';
  } catch {
    return 'Unavailable';
  }
}

export function disposeEngine(){
 if(running)throw new Error('Cannot dispose an active simulation.');
 if(engine){engine.material.dispose();engine.geometry.dispose();engine.renderer.dispose();engine.renderer.forceContextLoss();engine=null;}
}

function target(width,height,use4Mrt=true){
  return new WebGLRenderTarget(width,height,{
    count: use4Mrt ? 4 : 1,
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
 const numTargets = e.use4Mrt ? 4 : 1;
 // Keep readback sequential: Three.js owns pixel-pack buffer binding state.
 for(let i=0;i<numTargets;i++)attachments.push(await e.renderer.readRenderTargetPixelsAsync(rt,0,0,rt.width,rt.height,new Uint32Array(rt.width*rt.height*4),undefined,i));
 if(e.gl.isContextLost())throw new Error('WebGL context lost during simulation.');
 const error=e.gl.getError();if(error!==e.gl.NO_ERROR)throw new Error(`WebGL execution failed (0x${error.toString(16)}).`);
 checkAbort(signal);return attachments;
}

function unpack(attachments,width,lane,words,mode=0,use4Mrt=true){
 const result=new Uint32Array(words);
 if(!use4Mrt){
  const buf=attachments[0];
  if(mode===1){
   for(let j=0;j<words;j++){
    const stripe=Math.floor(j/4),comp=j%4;
    result[j]=buf[stripe*4+comp];
   }
   return result;
  }
  const simX=lane%width,simY=Math.floor(lane/width);
  for(let j=0;j<words;j++){
   const stripe=Math.floor(j/4),comp=j%4;
   result[j]=buf[((simY*8+stripe)*width+simX)*4+comp];
  }
  return result;
 }
 if(mode===1){
  for(let j=0;j<words;j++)result[j]=attachments[Math.floor(j%16/4)][Math.floor(j/16)*4+j%4];
  return result;
 }
 const simX=lane%width,simY=Math.floor(lane/width);
 for(let j=0;j<words;j++){
  const stripe=Math.floor(j/16),att=Math.floor(j%16/4),comp=j%4;
  result[j]=attachments[att][((simY*2+stripe)*width+simX)*4+comp];
 }
 return result;
}

function decodeBatch2D(outputs,width,count,config,first,states,use4Mrt=true){
 if(!use4Mrt){
  const raw=outputs[0];
  const f=new Float32Array(raw.buffer);
  for(let lane=0;lane<count;lane++){
   const simX=lane%width,simY=Math.floor(lane/width);
   const p0=((simY*8+0)*width+simX)*4;
   const p1=((simY*8+1)*width+simX)*4;
   const p2=((simY*8+2)*width+simX)*4;
   const p3=((simY*8+3)*width+simX)*4;
   const p4=((simY*8+4)*width+simX)*4;
   const p5=((simY*8+5)*width+simX)*4;
   const p6=((simY*8+6)*width+simX)*4;
   const p7=((simY*8+7)*width+simX)*4;

   const total=f[p0],mana=f[p0+1],spent=f[p0+2],gained=f[p0+3];
   const done=raw[p1],now=raw[p1+1],eventPetPacked=raw[p1+2],events=eventPetPacked&65535,highWater=raw[p1+3];
   if(done!==1||!Number.isFinite(total))throw new Error(`Fight ${first+lane} failed (status ${done}). Incomplete results rejected.`);
   const s={
    total,mana,spent,gained,done,now,events,highWater,petCasts:eventPetPacked>>>16,
    taps:raw[p2],procs:raw[p2+1],isbProcs:raw[p2+2],isbConsumed:raw[p2+3],
    damage0:f[p3],casts0:raw[p3+1]&65535,hits0:raw[p3+1]>>>16,crits0:raw[p3+2]&65535,misses0:raw[p3+2]>>>16,
    damage1:f[p3+3],casts1:raw[p4]&65535,hits1:raw[p4]>>>16,crits1:raw[p4+1]&65535,misses1:raw[p4+1]>>>16,
    damage2:f[p4+2],casts2:raw[p4+3]&65535,hits2:raw[p4+3]>>>16,crits2:raw[p5]&65535,misses2:raw[p5]>>>16,
    damage3:f[p5+1],casts3:raw[p5+2]&65535,hits3:raw[p5+2]>>>16,crits3:raw[p5+3]&65535,misses3:raw[p5+3]>>>16,
    damage4:f[p6],casts4:raw[p6+1]&65535,hits4:raw[p6+1]>>>16,crits4:raw[p6+2]&65535,misses4:raw[p6+2]>>>16,
    damage5:f[p6+3],casts5:raw[p7]&65535,hits5:raw[p7]>>>16,crits5:raw[p7+1]&65535,misses5:raw[p7+1]>>>16,
    petDamage:f[p7+2],petBrandDamage:f[p7+3]
   };
   states.push(s);
  }
  return;
 }
 const att0=outputs[0],att1=outputs[1],att2=outputs[2],att3=outputs[3];
 const f0=new Float32Array(att0.buffer),f3=new Float32Array(att3.buffer);
 const f1_1=new Float32Array(att1.buffer),f2_1=new Float32Array(att2.buffer),f3_1=new Float32Array(att3.buffer);
 for(let lane=0;lane<count;lane++){
  const simX=lane%width,simY=Math.floor(lane/width);
  const p0=((simY*2)*width+simX)*4,p1=((simY*2+1)*width+simX)*4;
  const done=att1[p0],now=att1[p0+1],eventPetPacked=att1[p0+2],events=eventPetPacked&65535,highWater=att1[p0+3];
  const total=f0[p0],mana=f0[p0+1],spent=f0[p0+2],gained=f0[p0+3];
  if(done!==1||!Number.isFinite(total))throw new Error(`Fight ${first+lane} failed (status ${done}). Incomplete results rejected.`);
  const s={
   total,mana,spent,gained,done,now,events,highWater,petCasts:eventPetPacked>>>16,
   taps:att2[p0],procs:att2[p0+1],isbProcs:att2[p0+2],isbConsumed:att2[p0+3],
   damage0:f3[p0],casts0:att3[p0+1]&65535,hits0:att3[p0+1]>>>16,crits0:att3[p0+2]&65535,misses0:att3[p0+2]>>>16,
   damage1:f3[p0+3],casts1:att0[p1]&65535,hits1:att0[p1]>>>16,crits1:att0[p1+1]&65535,misses1:att0[p1+1]>>>16,
   damage2:f0[p1+2],casts2:att0[p1+3]&65535,hits2:att0[p1+3]>>>16,crits2:att1[p1]&65535,misses2:att1[p1]>>>16,
   damage3:f1_1[p1+1],casts3:att1[p1+2]&65535,hits3:att1[p1+2]>>>16,crits3:att1[p1+3]&65535,misses3:att1[p1+3]>>>16,
   damage4:f2_1[p1],casts4:att2[p1+1]&65535,hits4:att2[p1+1]>>>16,crits4:att2[p1+2]&65535,misses4:att2[p1+2]>>>16,
   damage5:f2_1[p1+3],casts5:att3[p1]&65535,hits5:att3[p1]>>>16,crits5:att3[p1+1]&65535,misses5:att3[p1+1]>>>16,
   petDamage:f3_1[p1+2],petBrandDamage:f3_1[p1+3]
  };
  states.push(s);
 }
}

export function summarize(states, duration) {
  const dps = states.map(s => s.total / duration).sort((a,b) => a-b);
  let mean = 0, m2 = 0;
  dps.forEach((value,i) => { const delta = value - mean; mean += delta / (i+1); m2 += delta * (value - mean); });
  const sd = dps.length > 1 ? Math.sqrt(m2 / (dps.length-1)) : 0;
  const sum = key => states.reduce((n,s) => n+s[key], 0);
  return { count:states.length, mean, sd, ci95:1.96*sd/Math.sqrt(dps.length),
    p05:dps[Math.floor((dps.length-1)*.05)], p50:dps[Math.floor((dps.length-1)*.5)], p95:dps[Math.floor((dps.length-1)*.95)],
    dps, events:sum('events'), taps:sum('taps')/states.length, petDamage:sum('petDamage')/states.length, maxHeap:states.reduce((n,s)=>Math.max(n,s.highWater),0),
    spells:SPELLS.map((name,i) => ({name, damage:sum(`damage${i}`)/states.length,
      casts:sum(`casts${i}`)/states.length, hits:sum(`hits${i}`)/states.length,
      crits:sum(`crits${i}`)/states.length, misses:sum(`misses${i}`)/states.length})) };
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
    const stripeFactor = e.use4Mrt ? 2 : 8;
    if (gridHeight * stripeFactor > maxTexSize) throw new Error(`Simulation batch exceeds maximum texture height (${maxTexSize}).`);

    rt = target(gridWidth, gridHeight * stripeFactor, e.use4Mrt);
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
      decodeBatch2D(outputs, gridWidth, n, configs[0], first, states, e.use4Mrt);
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
      engine: `Three.js 0.180.0 / WebGL2 Multi-Config GLSL DES (${e.use4Mrt ? '4-MRT' : 'Mobile Striped'})`
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
  const stripeFactor=e.use4Mrt ? 2 : 8;
  if(gridHeight*stripeFactor>maxTexSize)throw new Error(`Simulation batch exceeds maximum texture height (${maxTexSize}).`);
  rt=target(gridWidth,gridHeight*stripeFactor,e.use4Mrt);e.renderer.setRenderTarget(rt);
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
   decodeBatch2D(outputs,gridWidth,n,config,first,states,e.use4Mrt);
   onProgress({phase:'Simulating',completed:first+n,total:config.iterations});
   if(first+n<config.iterations)await yieldUI();
  }
  const before=performance.now();
  const diagHeight = e.use4Mrt ? Math.ceil(STATE_WORDS/16) : Math.ceil(STATE_WORDS/4);
  diagnostic=target(1,diagHeight,e.use4Mrt);
  const full=await drawRead(e,diagnostic,1,1,1,0,signal);draws++;
  const first=decodeStates(unpack(full,1,0,STATE_WORDS,1,e.use4Mrt).buffer,1)[0];
  for(const [key,value]of Object.entries(states[0]))if(first[key]!==value)throw new Error(`Diagnostic replay differs at ${key}.`);
  states[0]=first;
  const traceLength=trace?Math.min(first.events,TRACE_CAPACITY):0,log=[];
  if(traceLength){
   const traceHeight = e.use4Mrt ? 1 : 2;
   traceTarget=target(traceLength,traceHeight,e.use4Mrt);
   const outputs=await drawRead(e,traceTarget,traceLength,traceLength,2,0,signal);draws++;
   if(e.use4Mrt){
    const floats=outputs.map(a=>new Float32Array(a.buffer));
    for(let i=0;i<traceLength;i++){const j=i*4;log.push({time:outputs[0][j]/1e6,kind:outputs[0][j+1],spell:outputs[0][j+2],damage:floats[0][j+3],mana:floats[1][j],flags:outputs[1][j+1],total:floats[1][j+2],rngCalls:outputs[1][j+3]});}
   } else {
    const raw=outputs[0];
    const floats=new Float32Array(raw.buffer);
    for(let i=0;i<traceLength;i++){
      const row0=i*4;
      const row1=(traceLength+i)*4;
      log.push({time:raw[row0]/1e6,kind:raw[row0+1],spell:raw[row0+2],damage:floats[row0+3],mana:floats[row1],flags:raw[row1+1],total:floats[row1+2],rngCalls:raw[row1+3]});
    }
   }
  }
  diagnosticMs=performance.now()-before;checkAbort(signal);
  const summary=summarize(states,config.duration),elapsedMs=performance.now()-started;
  return{config,summary,states,trace:log,traceTruncated:trace&&first.events>traceLength,
   timing:{elapsedMs,executeMs,compileMs,diagnosticMs,draws,capacity},
   adapter:e.gl.getParameter(e.gl.RENDERER),engine:`Three.js 0.180.0 / WebGL2 GLSL ES 3.00 DES (${e.use4Mrt ? '4-MRT' : 'Mobile Striped'})`,scope:'single-target-core-v1'};
 }finally{rt?.dispose();diagnostic?.dispose();traceTarget?.dispose();engine?.renderer.setRenderTarget(null);running=false;}
}

