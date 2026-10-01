// Real UI cancellation regression: CPU work is isolated so Stop can respond.
import assert from 'node:assert/strict';
const base=process.env.CDP_URL || 'http://127.0.0.1:9223';
const page=await(await fetch(`${base}/json/new?http://localhost:8087/docs/webgpu/index.html`,{method:'PUT'})).json();
const ws=new WebSocket(page.webSocketDebuggerUrl);let id=0;
const pending=new Map();
const timeout=setTimeout(()=>{console.error('Cancellation test timed out');process.exit(1);},15000);
await new Promise((resolve,reject)=>{ws.onopen=resolve;ws.onerror=reject;});
ws.onmessage=({data})=>{const msg=JSON.parse(data);if(msg.id){const p=pending.get(msg.id);pending.delete(msg.id);msg.error?p.reject(msg.error):p.resolve(msg.result);}};
const call=(method,params={})=>new Promise((resolve,reject)=>{pending.set(++id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));});
try {
 const response=await call('Runtime.evaluate',{awaitPromise:true,returnByValue:true,expression:`(async()=>{
  for(let i=0;i<100&&!window.runExperiment;i++)await new Promise(resolve=>setTimeout(resolve,20));
  const outcome=window.runExperiment(false,{largeBatches:true}).then(()=>({unexpectedCompletion:true}),error=>({name:error.name}));
  setTimeout(()=>document.querySelector('#stop').click(),50);
  const result=await outcome;
  return {...result,status:document.querySelector('#status').textContent,enabled:!document.querySelector('#run').disabled};
 })()`});
 assert.equal(response.exceptionDetails,undefined);
 assert.deepEqual(response.result.value,{name:'AbortError',status:'Stopped',enabled:true});
 console.log('Browser Stop interrupts benchmark and restores Run.');
}finally{await call('Page.close');ws.close();clearTimeout(timeout);}
