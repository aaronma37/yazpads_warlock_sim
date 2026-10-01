// Dependency-free Chrome DevTools runner; Node 22+ provides WebSocket.
// Start Chrome with --remote-debugging-port=9223, serve repo on port 8087.
import {writeFile} from 'node:fs/promises';
const options={compareSchedulers:process.env.COMPARE_SCHEDULERS==='1',largeBatches:process.env.LARGE_BATCHES==='1'};
const base=process.env.CDP_URL || 'http://127.0.0.1:9223';
const target=await (await fetch(`${base}/json/new?${encodeURIComponent(process.env.PAGE_URL || 'http://127.0.0.1:8087/docs/webgpu/index.html')}`,{method:'PUT'})).json();
const ws=new WebSocket(target.webSocketDebuggerUrl), pending=new Map();let next=1;
const timeout=setTimeout(()=>{console.error('Browser experiment timed out after 120 seconds');ws.close();process.exit(1);},120000);
await new Promise((resolve,reject)=>{ws.onopen=resolve;ws.onerror=reject;});
ws.onmessage=e=>{const msg=JSON.parse(e.data);if(msg.id){const p=pending.get(msg.id);pending.delete(msg.id);if(msg.error)p.reject(msg.error);else p.resolve(msg.result);}};
const call=(method,params={})=>new Promise((resolve,reject)=>{const id=next++;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));});
try {
  await call('Runtime.enable');
  await call('Page.enable');
  // Module loading need not have finished at navigation response.
  const result=await call('Runtime.evaluate',{expression:`(async()=>{for(let i=0;i<200&&!window.runExperiment;i++)await new Promise(r=>setTimeout(r,50));if(!window.runExperiment)throw Error('Page did not load');return await window.runExperiment(${JSON.stringify(process.env.QUICK === '1')},${JSON.stringify(options)});})()`,awaitPromise:true,returnByValue:true});
  if(result.exceptionDetails)throw Error(JSON.stringify(result.exceptionDetails));
  const report=result.result.value;
  console.log(JSON.stringify(report,null,2));
  await writeFile(process.env.REPORT_PATH || '/tmp/warlock-webgpu-report.json',JSON.stringify(report,null,2)+'\n');
  if(!report.passed)process.exitCode=1;
} finally {await call('Page.close');ws.close();clearTimeout(timeout);}
