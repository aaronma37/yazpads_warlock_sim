import {runStage} from './async.mjs';
const CONFIG_BYTES = 336, CONFIG_FLOATS = 84, STATE_BYTES = 260, STATE_WORDS = 65;
export const fields = [
  'duration', 'castTime', 'gcd', 'maxMana',
  'spellPower', 'hit', 'crit', 'boltCrit',
  'dotCrit', 'tapGain', 'mp5', 'tapThreshold',
  'corruption', 'nightfall', 'boltCost', 'corruptionCost',
  'boltMin', 'boltMax', 'dotBase', 'dotMultiplier',
  'isbBonus', 'isbCharges', 'isbAllShadow', 'pad0',
  'agony', 'agonyCost', 'agonyBase', 'agonyMultiplier',
  'doom', 'doomCost', 'doomBase', 'doomMultiplier',
  'siphon', 'siphonCost', 'siphonBase', 'siphonMultiplier',
  'immolate', 'immolateCost', 'immolateCastTime', 'immolateMultiplier', 'immolateDirectMultiplier',
  'immolateMin', 'immolateMax', 'immolateDotBase', 'pad1',
  'petType', 'petCastInterval', 'petBaseMin', 'petBaseMax',
  'petSpRatio', 'petMultiplier', 'petLopBase', 'petLopSpRatio',
  'petLopCd', 'petMeleeBase', 'petApRatio', 'pad2',
  'shadowPower', 'petSpellHit', 'petSpellCrit', 'petMeleeMissPct',
  'petMeleeDodgePct', 'petGlancePct', 'petGlanceMultiplier', 'petArmorMultiplier',
  'petMeleeMultiplier', 'petLopMultiplier', 'petLopCost', 'petManaManagement',
  'petManaMax', 'petMp5', 'petFlatSP', 'petMeleeCritPct', 'petSpellPiercing', 'petPiercingBonus',
  'firePower', 'fireCrit', 'corruptionCastTime', 'corruptionMultiplier', 'executeBonus', 'corruptionSpCoefficient',
  'shadowBoltMultiplier', 'pad3', 'pad4', 'pad5'
];
export const defaults = Object.freeze({
  duration: 180.123, castTime: 2.5, gcd: 1.5, maxMana: 6433,
  spellPower: 500, hit: 0.95, crit: 0.2, boltCrit: 2,
  dotCrit: 1.5, tapGain: 692.4, mp5: 20, tapThreshold: 0.3,
  corruption: 1, nightfall: 0.04, boltCost: 370, corruptionCost: 290,
  boltMin: 246, boltMax: 274, dotBase: 57, dotMultiplier: 1.1,
  isbBonus: 0, isbCharges: 0, isbAllShadow: 0, pad0: 0,
  agony: 0, agonyCost: 215, agonyBase: 46, agonyMultiplier: 1.1,
  doom: 0, doomCost: 300, doomBase: 1742, doomMultiplier: 1.1,
  siphon: 0, siphonCost: 365, siphonBase: 41, siphonMultiplier: 1.1,
  immolate: 0, immolateCost: 380, immolateCastTime: 1.5, immolateMultiplier: 1.0, immolateDirectMultiplier: 1.0,
  immolateMin: 158, immolateMax: 158, immolateDotBase: 55, pad1: 0,
  petType: 0, petCastInterval: 2.0, petBaseMin: 0, petBaseMax: 0,
  petSpRatio: 0, petMultiplier: 1.0, petLopBase: 0, petLopSpRatio: 0,
  petLopCd: 12.0, petMeleeBase: 0, petApRatio: 0, pad2: 0,
  shadowPower: 0, petSpellHit: 0.95, petSpellCrit: 0.2, petMeleeMissPct: 8,
  petMeleeDodgePct: 6.5, petGlancePct: 40, petGlanceMultiplier: 0.65, petArmorMultiplier: 1,
  petMeleeMultiplier: 1, petLopMultiplier: 1, petLopCost: 160, petManaManagement: 1,
  petManaMax: 1450, petMp5: 45, petFlatSP: 0, petMeleeCritPct: 2.72,
  petSpellPiercing: 1, petPiercingBonus: 0.00575, firePower: 500, fireCrit: 0.2,
  corruptionCastTime: 2.0, corruptionMultiplier: 1.1, executeBonus: 0, corruptionSpCoefficient: 0.2,
  shadowBoltMultiplier: 1.1, pad3: 0, pad4: 0, pad5: 0
});
export function validateConfig(input = {}) {
  if (!input || typeof input !== 'object' || Array.isArray(input)) throw new Error('Configuration must be an object');
  for (const key of Object.keys(input)) if (!fields.includes(key)) throw new Error(`Unsupported slice parameter: ${key}`);
  const c = {...defaults, ...input};
  for (const key of fields) if (!Number.isFinite(c[key]) || c[key] < 0 || c[key] > 1e7) throw new Error(`Invalid ${key}; expected finite [0, 10000000]`);
  if (c.duration < 0.001 || c.duration > 600 || c.castTime < 1 || c.castTime > 60 || c.gcd < 1 || c.gcd > 60 || c.maxMana <= 0) throw new Error('Duration must be [0.001, 600], cast/GCD [1, 60], mana (0, 1e7]');
  if (c.boltMin > c.boltMax) throw new Error('boltMin exceeds boltMax');
  if (c.petBaseMin > c.petBaseMax) throw new Error('petBaseMin exceeds petBaseMax');
  for (const key of ['hit', 'crit', 'nightfall', 'tapThreshold', 'corruption', 'agony', 'doom', 'siphon', 'immolate']) if (c[key] > 1) throw new Error(`${key} must be in [0, 1]`);
  return c;
}
export function packConfigs(candidates) {
  return new Float32Array(candidates.flatMap(c => fields.map(key => c[key])));
}
export function summarize(buffer, candidates, perCandidate) {
  const u = new Uint32Array(buffer), f = new Float32Array(buffer);
  const counterNames = ['bolts','dots','ticks','taps','procs','consumed','misses','crits','agonies','dooms','siphons','immolates','isbProcs','isbConsumed','loops','events','petCasts','petHits','petCrits','petMeleeCasts','petMeleeHits','petLopCasts','petLopHits','petMeleeCrits','petLopCrits','incinerates','incinerateHits','incinerateMisses','incinerateCrits'];
  const counterOffsets = [28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,46,47,52,53,54,55,56,57,58,60,61,62,63];
  return candidates.map((c, ci) => {
    let mean = 0, m2 = 0;
    let boltDamage = 0, dotDamage = 0, agonyDamage = 0, doomDamage = 0, siphonDamage = 0, immolateDamage = 0, incinerateDamage = 0, petDamage = 0, isbUptime = 0;
    const totals = new Float64Array(counterNames.length);
    for (let j = 0; j < perCandidate; j++) {
      const p = (ci * perCandidate + j) * STATE_WORDS;
      if (u[p + 3] !== 1) throw new Error(`Simulation ${ci}:${j} did not complete`);
      const damage = f[p + 21] + f[p + 22] + f[p + 23] + f[p + 24] + f[p + 25] + f[p + 26] + f[p + 48] + f[p + 64];
      const dps = damage / c.duration;
      if (!Number.isFinite(dps) || f[p + 20] < -0.01 || f[p + 20] > c.maxMana + 1) throw new Error(`Invalid simulation ${ci}:${j}`);
      const delta = dps - mean; mean += delta / (j + 1); m2 += delta * (dps - mean);
      boltDamage += f[p + 21]; dotDamage += f[p + 22]; agonyDamage += f[p + 23];
      doomDamage += f[p + 24]; siphonDamage += f[p + 25]; immolateDamage += f[p + 26];
      incinerateDamage += f[p + 64];
      petDamage += f[p + 48];
      isbUptime += f[p + 27] / 1e6;
      for (let k = 0; k < counterOffsets.length; k++) totals[k] += u[p + counterOffsets[k]];
    }
    return {
      meanDps: mean, standardError: perCandidate > 1 ? Math.sqrt(m2 / (perCandidate - 1) / perCandidate) : 0,
      boltDamage: boltDamage / perCandidate, dotDamage: dotDamage / perCandidate,
      agonyDamage: agonyDamage / perCandidate, doomDamage: doomDamage / perCandidate,
      siphonDamage: siphonDamage / perCandidate, immolateDamage: immolateDamage / perCandidate,
      incinerateDamage: incinerateDamage / perCandidate,
      petDamage: petDamage / perCandidate,
      isbUptime: isbUptime / perCandidate, ...Object.fromEntries(counterNames.map((key,k) => [key, totals[k] / perCandidate]))
    };
  });
}
export class GpuSimulator {
  static async create({signal, onProgress = () => {}, timeoutMs = 30000} = {}) {
    onProgress(`Checking WebGPU (secure context: ${globalThis.isSecureContext}, API: ${!!navigator.gpu})`);
    if (!navigator.gpu) throw new Error('WebGPU unavailable. Use a WebGPU browser on HTTPS or localhost with graphics acceleration enabled.');
    const options = {signal, onProgress, timeoutMs};
    const adapter = await runStage('Requesting GPU adapter',
      () => navigator.gpu.requestAdapter({powerPreference: 'high-performance'}), options);
    if (!adapter) throw new Error('The browser returned no WebGPU adapter. Check graphics acceleration and browser GPU support.');
    const source = await runStage('Loading combat shader', async () => {
      const response = await fetch(new URL('./combat.wgsl', import.meta.url), {signal});
      if (!response.ok) throw new Error(`Shader fetch: HTTP ${response.status}`);
      return response.text();
    }, options);
    const device = await runStage('Requesting GPU device', () => adapter.requestDevice(),
      {...options, onLateValue: device => device.destroy()});
    try {
      const instance = new GpuSimulator(device, source, adapter.info);
      const info = await runStage('Validating combat shader', () => instance.module.getCompilationInfo(), options);
      const errors = info.messages.filter(m => m.type === 'error');
      if (errors.length) throw new Error(errors.map(m => `${m.lineNum}: ${m.message}`).join('\n'));
      return instance;
    } catch (error) {
      device.destroy();
      throw error;
    }
  }
  constructor(device, source, info) {
    this.device = device; this.info = info; this.pipelines = new Map(); this.busy = false;
    this.module = device.createShaderModule({code: source});
    device.lost.then(info => { this.lost = `${info.reason}: ${info.message}`; });
    device.addEventListener('uncapturederror', e => { this.error = e.error.message; });
  }
  async pipeline(mode, stepUs, workgroupSize, signal) {
    const key = `${mode}:${stepUs}:${workgroupSize}`;
    if (!this.pipelines.has(key)) {
      const start = performance.now();
      const pipeline = await runStage('Compiling simulation pipeline', () => this.device.createComputePipelineAsync({layout:'auto', compute:{module:this.module, entryPoint:'simulate', constants:{FIXED: mode === 'fixed', STEP_US:stepUs, CHUNK:mode === 'fixed' ? 4096 : 256, GROUP_SIZE:workgroupSize}}}), {signal});
      this.pipelines.set(key, {pipeline, compileMs:performance.now() - start});
    }
    return this.pipelines.get(key);
  }
  // Candidate-major layout. Seed is keyed by replica, so candidates share random streams.
  // Packed fields can also be supplied from WASM memory using runPacked().
  async run(candidates = [defaults], {perCandidate = 10000, seed = 42, mode = 'event', stepUs = 1000, workgroupSize = 64, raw = false, signal} = {}) {
    if (this.busy) throw new Error('A batch is already running on this simulator');
    if (this.lost || this.error) throw new Error(this.lost || this.error);
    if (!Array.isArray(candidates) || !candidates.length || candidates.length > 1024) throw new Error('Expected 1–1024 candidates');
    candidates = candidates.map(validateConfig);
    if (!Number.isSafeInteger(perCandidate) || perCandidate < 1 || perCandidate > 1e6) throw new Error('Invalid replica count');
    if (!Number.isInteger(seed) || seed < 0 || seed > 0xffffffff) throw new Error('Seed must be uint32');
    if (!['event', 'fixed'].includes(mode) || ![1000, 10000, 50000].includes(stepUs) || ![32,64,128].includes(workgroupSize)) throw new Error('Unsupported scheduler options');
    const count = candidates.length * perCandidate, bytes = count * STATE_BYTES, d = this.device;
    if (bytes > Math.min(d.limits.maxStorageBufferBindingSize, d.limits.maxBufferSize) || Math.ceil(count/workgroupSize) > d.limits.maxComputeWorkgroupsPerDimension || candidates.length * CONFIG_BYTES > d.limits.maxStorageBufferBindingSize) throw new Error('Batch exceeds device limits; split candidates into smaller batches');
    signal?.throwIfAborted();
    this.busy = true;
    const buffers = [];
    const make = (size,usage) => {const b=d.createBuffer({size,usage});buffers.push(b);return b;};
    try {
      signal?.throwIfAborted();
      const cold = !this.pipelines.has(`${mode}:${stepUs}:${workgroupSize}`);
      const {pipeline, compileMs} = await this.pipeline(mode, stepUs, workgroupSize, signal);
      const start = performance.now();
      const cfg = make(candidates.length*CONFIG_BYTES,GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_DST);
      const params = make(16,GPUBufferUsage.UNIFORM|GPUBufferUsage.COPY_DST);
      const state = make(bytes,GPUBufferUsage.STORAGE|GPUBufferUsage.COPY_SRC);
      const read = make(bytes,GPUBufferUsage.MAP_READ|GPUBufferUsage.COPY_DST);
      d.queue.writeBuffer(cfg,0,packConfigs(candidates));
      d.queue.writeBuffer(params,0,new Uint32Array([count,perCandidate,seed,0]));
      const group=d.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[cfg,params,state].map((buffer,binding)=>({binding,resource:{buffer}}))});
      // Conservative bound: decisions + cast finishes + ticks + regen, plus t=0.
      const maxDuration=Math.max(...candidates.map(c=>c.duration));
      // One extra fixed step covers float32 duration rounding at a chunk boundary.
      const rounds=mode==='fixed' ? Math.ceil((Math.ceil(maxDuration*1e6/stepUs)+1)/4096) : Math.ceil((Math.ceil(maxDuration)*4+8)/256);
      for(let first=0;first<rounds;first+=16) {
        signal?.throwIfAborted();
        const encoder=d.createCommandEncoder();
        for(let i=first;i<Math.min(rounds,first+16);i++) {
          const pass=encoder.beginComputePass(); pass.setPipeline(pipeline); pass.setBindGroup(0,group); pass.dispatchWorkgroups(Math.ceil(count/workgroupSize)); pass.end();
        }
        d.queue.submit([encoder.finish()]);
        await runStage('Executing GPU batch', () => d.queue.onSubmittedWorkDone(), {signal});
        if(this.lost || this.error) throw new Error(this.lost || this.error);
      }
      const encoder=d.createCommandEncoder();encoder.copyBufferToBuffer(state,0,read,0,bytes);d.queue.submit([encoder.finish()]);
      await runStage('Reading GPU results', () => read.mapAsync(GPUMapMode.READ), {signal});
      const buffer=read.getMappedRange().slice(0);read.unmap();
      const dispatchAndReadbackMs=performance.now()-start;
      const summaries=summarize(buffer,candidates,perCandidate);
      const elapsedMs=performance.now()-start;
      return {mode,stepUs:mode==='fixed'?stepUs:0,count,perCandidate,compileMs:cold?compileMs:0,elapsedMs,dispatchAndReadbackMs,simsPerSecond:count/(elapsedMs/1000),summaries,...(raw?{buffer}:{})};
    } catch (error) {
      // Do not reuse a device whose pending work could not be completed/cancelled.
      if (error.name === 'TimeoutError' || signal?.aborted) this.destroy();
      throw error;
    } finally {for(const b of buffers)b.destroy();this.busy=false;}
  }
  runPacked(heapF32, floatOffset, candidateCount, options) {
    if (!(heapF32 instanceof Float32Array) || !Number.isInteger(floatOffset) || floatOffset < 0 || !Number.isInteger(candidateCount) || candidateCount < 1 || floatOffset+candidateCount*CONFIG_FLOATS > heapF32.length) throw new Error('Invalid WASM config range');
    const candidates=Array.from({length:candidateCount},(_,i)=>Object.fromEntries(fields.map((key,j)=>[key,heapF32[floatOffset+i*CONFIG_FLOATS+j]])));
    return this.run(candidates,options);
  }
  destroy() { this.lost = 'Simulator disposed'; this.device.destroy(); }
}
