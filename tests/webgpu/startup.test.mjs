import test from 'node:test';
import assert from 'node:assert/strict';
import {runStage} from '../../src/sim/webgpu/async.mjs';
import {GpuSimulator} from '../../src/sim/webgpu/runner.mjs';

test('startup displays the pending stage and times out', async () => {
  const progress=[];
  await assert.rejects(runStage('Requesting GPU adapter', () => new Promise(()=>{}),
    {timeoutMs:10,onProgress:message=>progress.push(message)}),
    {name:'TimeoutError',message:/Requesting GPU adapter timed out/});
  assert.deepEqual(progress,['Requesting GPU adapter']);
});
test('stopping interrupts a pending stage', async () => {
  const controller=new AbortController();
  const pending=runStage('Pending',()=>new Promise(()=>{}),{signal:controller.signal});
  controller.abort();
  await assert.rejects(pending,{name:'AbortError'});
});
test('pre-aborted stages never start', async () => {
  const controller=new AbortController();controller.abort();let started=false;
  await assert.rejects(runStage('Pending',()=>{started=true;},{signal:controller.signal}),{name:'AbortError'});
  assert.equal(started,false);
});
test('late device creation is cleaned up after timeout', async () => {
  let finish,cleaned=false;
  const pending=runStage('Device',()=>new Promise(resolve=>{finish=resolve;}),
    {timeoutMs:10,onLateValue:device=>device.destroy()});
  await assert.rejects(pending,{name:'TimeoutError'});
  finish({destroy(){cleaned=true;}});
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.equal(cleaned,true);
});
test('synchronous failures and rejected operations propagate', async () => {
  await assert.rejects(runStage('Compile',()=>{throw Error('Shader invalid');}),/Shader invalid/);
  await assert.rejects(runStage('Fetch',()=>Promise.reject(Error('HTTP 404'))),/HTTP 404/);
  assert.equal(await runStage('Ready',()=>42),42);
});
test('actual simulator adapter startup cannot stay pending forever', async () => {
  const previous=Object.getOwnPropertyDescriptor(globalThis,'navigator');
  Object.defineProperty(globalThis,'navigator',{configurable:true,value:{gpu:{requestAdapter:()=>new Promise(()=>{})}}});
  try {
    await assert.rejects(GpuSimulator.create({timeoutMs:10}),/Requesting GPU adapter timed out/);
  } finally {
    if(previous)Object.defineProperty(globalThis,'navigator',previous);else delete globalThis.navigator;
  }
});
