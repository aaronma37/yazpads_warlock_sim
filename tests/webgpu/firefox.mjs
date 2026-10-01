// Firefox startup diagnostic using WebDriver BiDi; no npm dependencies.
// Launch an isolated Firefox with --remote-debugging-port 9224 first.
import {writeFile} from 'node:fs/promises';
const ws=new WebSocket(process.env.BIDI_URL || 'ws://127.0.0.1:9224/session');
const pending=new Map();let id=0;
const timeout=setTimeout(()=>{console.error('Firefox diagnostic timed out');process.exit(1);},90000);
await new Promise((resolve,reject)=>{ws.onopen=resolve;ws.onerror=reject;});
ws.onmessage=({data})=>{const reply=JSON.parse(data);if(reply.id){const entry=pending.get(reply.id);pending.delete(reply.id);if(reply.type==='error')entry.reject(Error(JSON.stringify(reply)));else entry.resolve(reply.result);}};
const call=(method,params={})=>new Promise((resolve,reject)=>{pending.set(++id,{resolve,reject});ws.send(JSON.stringify({id,method,params}));});
try {
 const session=await call('session.new',{capabilities:{}});
 const {context}=await call('browsingContext.create',{type:'tab'});
 await call('browsingContext.navigate',{context,url:process.env.PAGE_URL || 'http://localhost:8087/docs/webgpu/index.html',wait:'complete'});
 const result=await call('script.evaluate',{target:{context},awaitPromise:true,expression:`(async()=>{
   try { const report=await window.runExperiment(true); return JSON.stringify({passed:true,report}); }
   catch(error) {return JSON.stringify({passed:false,error:error.message,status:document.querySelector('#status').textContent,output:document.querySelector('#output').textContent,runEnabled:!document.querySelector('#run').disabled});}
 })()`});
 const report={capabilities:session.capabilities,result};
 console.log(JSON.stringify(report,null,2));
 await writeFile('/tmp/warlock-webgpu-firefox.json',JSON.stringify(report,null,2)+'\n');
 await call('browser.close');
} finally {clearTimeout(timeout);ws.close();}
