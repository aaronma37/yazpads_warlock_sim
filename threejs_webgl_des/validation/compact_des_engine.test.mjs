import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
import * as model from '../src/priest/model.js';
import { summarize } from '../src/priest/results.js';

function harness() {
  const log = { attachments: [], compiled: [], disposed: 0, contexts:0 };
  const context = { NO_ERROR:0, MAX_TEXTURE_SIZE:1, getParameter: () => 2048, getError: () => 0, isContextLost: () => false };
  class Resource { constructor(...args) { this.args=args; } dispose() { log.disposed++; } }
  class Geometry extends Resource { setAttribute() {} }
  class Material extends Resource { constructor(options) { super(); Object.assign(this,options); } }
  class Mesh { constructor(_geometry,material) { this.material=material; } }
  class Scene { add(mesh) { this.mesh=mesh; } }
  class RenderTarget extends Resource { constructor(width,height) { super(); this.width=width;this.height=height; } }
  class Renderer extends Resource {
    constructor() { super(); this.debug={}; log.contexts++; }
    async compileAsync(scene) { log.compiled.push(scene.mesh.material.fragmentShader); }
    setRenderTarget(target) { this.target=target; }
    render(scene) {
      const uniforms=scene.mesh.material.uniforms, detailed=scene.mesh.material.fragmentShader==='detailed';
      const width=this.target.width,height=this.target.height,stripes=detailed?model.COMPACT_STRIPES:1;
      const schema=detailed?model.STATE:model.FAST_STATE;
      this.outputs=Array.from({length:4},()=>new Uint32Array(width*height*4));
      for(let lane=0;lane<uniforms.count.value;lane++) {
        const words=new Uint32Array(Object.keys(schema).length),floats=new Float32Array(words.buffer);
        Object.entries(schema).forEach(([key,type],index)=> {
          const value=key==='total'?100+uniforms.offset.value+lane:key==='done'?1:0;
          (type==='f32'?floats:words)[index]=value;
        });
        words.forEach((word,index)=> {
          const stripe=Math.floor(index/16),attachment=Math.floor(index%16/4),component=index%4;
          this.outputs[attachment][((Math.floor(lane/width)*stripes+stripe)*width+lane%width)*4+component]=word;
        });
      }
    }
    async readRenderTargetPixelsAsync(_target,_x,_y,_w,_h,array,_cube,attachment) { log.attachments.push(attachment); array.set(this.outputs[attachment]);return array; }
    forceContextLoss() {}
  }
  const source=readFileSync(new URL('../src/gpu/compact_des_engine.js',import.meta.url),'utf8').replace(/import[\s\S]*?from 'three';/,'').replace('export function','function');
  const globals={ WebGLRenderer:Renderer,WebGLRenderTarget:RenderTarget,RawShaderMaterial:Material,BufferGeometry:Geometry,
    BufferAttribute:Resource,Mesh,Scene,Camera:Resource,DataTexture:Resource,Uint32Array,Float32Array,performance,setTimeout,DOMException,
    document:{createElement:()=>({getContext:()=>context,addEventListener(){}})} };
  for(const name of ['GLSL3','RGBAIntegerFormat','UnsignedIntType','NearestFilter','NoBlending'])globals[name]=0;
  vm.runInNewContext(source+'\nglobalThis.factory=createCompactDESEngine;',globals);
  return { log, engine:globals.factory({model,shaders:{vertex:'vertex',detailed:'detailed',fast:'fast'},summarize,id:'priest',scope:model.SCOPE}) };
}

test('Packed DES transport preserves candidate grouping across batch boundaries in both variants',async()=> {
  const {engine,log}=harness();
  const configs=[{iterations:4,duration:10},{iterations:4,duration:20}];
  for(const detailedResults of [true,false]) {
    const batch=await engine.runMultiSimulation(configs,{batchSize:3,detailedResults});
    assert.deepEqual(Array.from(batch.results[0].states,s=>s.total),[100,101,102,103]);
    assert.deepEqual(Array.from(batch.results[1].states,s=>s.total),[104,105,106,107]);
    assert.equal(batch.results[0].summary.mean,10.15);assert.equal(batch.results[1].summary.mean,5.275);
    assert.equal(batch.timing.draws,3);
  }
  assert.deepEqual(log.compiled,['detailed','fast']);
  assert.deepEqual(log.attachments,Array.from({length:6},()=>[0,1,2,3]).flat());
  engine.disposeEngine();assert.ok(log.disposed>0);
});

test('Packed DES transport rejects mixed seeds, cancels cleanly and can retry',async()=> {
  const {engine}=harness();
  await assert.rejects(engine.runMultiSimulation([{seed:1},{seed:2}]),/one iteration count and seed/);
  const controller=new AbortController();
  await assert.rejects(engine.runSimulation({iterations:4},{batchSize:2,signal:controller.signal,onProgress:()=>controller.abort()}),error=>error.name==='AbortError');
  const finalAbort=new AbortController();
  await assert.rejects(engine.runSimulation({iterations:1},{signal:finalAbort.signal,onProgress:()=>finalAbort.abort()}),error=>error.name==='AbortError');
  const result=await engine.runSimulation({iterations:1});assert.equal(result.states.length,1);
  engine.disposeEngine();
});
