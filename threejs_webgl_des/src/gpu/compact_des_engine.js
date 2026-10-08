import { WebGLRenderer, WebGLRenderTarget, RawShaderMaterial, BufferGeometry, BufferAttribute,
  Mesh, Scene, Camera, GLSL3, RGBAIntegerFormat, UnsignedIntType, NearestFilter, NoBlending, DataTexture } from 'three';

// Class-independent packed DES transport. Bind class data once, outside GPU loops.
// Existing Warlock transport remains intact while this extension is validated.
export function createCompactDESEngine({ model, shaders, summarize, id, scope }) {
  let engine = null, running = false;
  const abort = signal => { if (signal?.aborted) throw new DOMException('Run cancelled.', 'AbortError'); };
  const yieldUI = () => new Promise(resolve => setTimeout(resolve, 0));
  function acquire() {
    if (engine) return engine;
    const canvas = document.createElement('canvas');
    const context = canvas.getContext('webgl2', { alpha: false, antialias: false, depth: false, stencil: false });
    if (!context) throw new Error('WebGL2 is unavailable.');
    const renderer = new WebGLRenderer({ canvas, context, antialias: false }); renderer.autoClear = false;
    const dummy = new DataTexture(new Uint32Array(4), 1, 1, RGBAIntegerFormat, UnsignedIntType); dummy.needsUpdate = true;
    const uniforms = Object.fromEntries(['numConfigs','fightsPerConfig','seed','offset','count','gridWidth','eventBudget'].map(key => [key, { value: 0 }]));
    uniforms.configWords = { value: new Uint32Array(model.CONFIG_WORDS) }; uniforms.configTex = { value: dummy };
    const material = fragmentShader => new RawShaderMaterial({ glslVersion: GLSL3, vertexShader: shaders.vertex,
      fragmentShader, uniforms, depthTest: false, depthWrite: false, blending: NoBlending });
    const detailed = material(shaders.detailed), fast = material(shaders.fast);
    const geometry = new BufferGeometry(); geometry.setAttribute('position', new BufferAttribute(new Float32Array([-1,-1,0,3,-1,0,-1,3,0]),3));
    const mesh = new Mesh(geometry, detailed); mesh.frustumCulled = false;
    const scene = new Scene(); scene.add(mesh);
    const owned = { renderer, gl: context, uniforms, detailed, fast, geometry, mesh, scene, camera: new Camera(), dummy, compiled: new Set(), error: null };
    canvas.addEventListener('webglcontextlost', event => { event.preventDefault(); owned.error = new Error('WebGL context lost.'); });
    renderer.debug.onShaderError = (gl, program, _vs, fs) => { owned.error = new Error(`DES shader failed: ${gl.getProgramInfoLog(program)} ${gl.getShaderInfoLog(fs)}`); };
    engine = owned; return owned;
  }
  async function compile(e, detailedResults) {
    e.mesh.material = detailedResults ? e.detailed : e.fast;
    if (!e.compiled.has(detailedResults)) { await e.renderer.compileAsync(e.scene,e.camera); if (e.error) throw e.error; e.compiled.add(detailedResults); }
  }
  async function preloadShader({ detailedResults = true } = {}) {
    if (running) throw new Error('Cannot preload during an active simulation.');
    running = true;
    try { await compile(acquire(), detailedResults); return true; } finally { running = false; }
  }
  function disposeEngine() {
    if (running) throw new Error('Cannot dispose an active simulation.');
    if (!engine) return;
    engine.dummy.dispose(); engine.fast.dispose(); engine.detailed.dispose(); engine.geometry.dispose(); engine.renderer.dispose(); engine.renderer.forceContextLoss(); engine = null;
  }
  async function runMultiSimulation(inputs, { signal, onProgress = () => {}, batchSize = 16384, iterations = null, eventBudget = 65536, detailedResults = true } = {}) {
    if (running) throw new Error('A simulation is already running for this class.');
    abort(signal);
    if (!Array.isArray(inputs) || !inputs.length) throw new Error('Simulation requires configs.');
    if (!Number.isInteger(batchSize) || batchSize < 1 || !Number.isInteger(eventBudget) || eventBudget < 1 || eventBudget > 1000000) throw new Error('Invalid DES execution limits.');
    const configs = inputs.map(input => model.validate(iterations == null ? input : { ...input, iterations }));
    const fights = configs[0].iterations;
    if (configs.some(c => c.iterations !== fights || c.seed !== configs[0].seed)) throw new Error('A native batch requires one iteration count and seed.');
    running = true;
    let target, configTexture;
    try {
      const started = performance.now(), e = acquire();
      const beforeCompile = performance.now(); await compile(e, detailedResults); const compileMs = performance.now() - beforeCompile;
      abort(signal);
      const packed = model.packMultiConfig(configs), maxSize = e.gl.getParameter(e.gl.MAX_TEXTURE_SIZE);
      if (packed.height > maxSize || packed.width > maxSize) throw new Error('Config batch exceeds texture limits.');
      configTexture = new DataTexture(packed.words, packed.width, packed.height, RGBAIntegerFormat, UnsignedIntType);
      configTexture.minFilter = NearestFilter; configTexture.magFilter = NearestFilter; configTexture.generateMipmaps = false; configTexture.needsUpdate = true;
      const stripes = detailedResults ? model.COMPACT_STRIPES : 1, total = fights * configs.length;
      const width = Math.min(512, maxSize, batchSize, total);
      const capacity = Math.min(batchSize, width * Math.floor(maxSize / stripes), total);
      const height = Math.ceil(capacity / width) * stripes;
      target = new WebGLRenderTarget(width,height,{ count: 4, format: RGBAIntegerFormat, type: UnsignedIntType,
        internalFormat: 'RGBA32UI', minFilter: NearestFilter, magFilter: NearestFilter, depthBuffer: false, stencilBuffer: false, samples: 0, generateMipmaps: false });
      Object.assign(e.uniforms.configWords, { value: model.packConfig(configs[0]) });
      e.uniforms.configTex.value = configTexture; e.uniforms.numConfigs.value = configs.length;
      e.uniforms.fightsPerConfig.value = fights; e.uniforms.seed.value = configs[0].seed;
      e.uniforms.gridWidth.value = width; e.uniforms.eventBudget.value = eventBudget;
      const states = Array.from({ length: configs.length }, () => []);
      let draws = 0;
      for (let first = 0; first < total; first += capacity) {
        abort(signal); if (e.error) throw e.error;
        const count = Math.min(capacity,total-first); e.uniforms.offset.value = first; e.uniforms.count.value = count;
        e.renderer.setRenderTarget(target); e.renderer.render(e.scene,e.camera);
        const outputs = [];
        // Three.js pixel-pack buffer state requires sequential attachment reads.
        for (let attachment = 0; attachment < 4; attachment++) outputs.push(await e.renderer.readRenderTargetPixelsAsync(target,0,0,width,height,new Uint32Array(width*height*4),undefined,attachment));
        abort(signal); if (e.error) throw e.error;
        if (e.gl.isContextLost()) throw new Error('WebGL context lost during simulation.');
        const error = e.gl.getError(); if (error !== e.gl.NO_ERROR) throw new Error(`WebGL execution failed (${error}).`);
        for (let lane = 0; lane < count; lane++) {
          const words = new Uint32Array(detailedResults ? model.STATE_WORDS : Object.keys(model.FAST_STATE).length);
          const x = lane % width, y = Math.floor(lane/width);
          for (let word = 0; word < words.length; word++) {
            const stripe = Math.floor(word/16), attachment = Math.floor((word%16)/4), component = word%4;
            words[word] = outputs[attachment][((y*stripes+stripe)*width+x)*4+component];
          }
          states[Math.floor((first+lane)/fights)].push(model.decodeState(words,detailedResults));
        }
        draws++; onProgress((first+count)/total); await yieldUI(); abort(signal);
      }
      const results = configs.map((config,index) => ({ config, states: states[index], summary: summarize(states[index],config.duration), detailedResults }));
      return { results, detailedResults, scope, engine: `${id} / WebGL2 DES`, timing: { elapsedMs: performance.now()-started, compileMs, draws, totalFights: total } };
    } finally {
      if (engine) { engine.renderer.setRenderTarget(null); engine.uniforms.configTex.value = engine.dummy; }
      target?.dispose(); configTexture?.dispose(); running = false;
    }
  }
  async function runSimulation(input, options) {
    const batch = await runMultiSimulation([input],options);
    return { ...batch.results[0], timing: batch.timing, scope, engine: batch.engine };
  }
  return Object.freeze({ runSimulation, runMultiSimulation, preloadShader, disposeEngine });
}
