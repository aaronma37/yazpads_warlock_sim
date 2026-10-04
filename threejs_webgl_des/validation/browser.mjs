// Start a static server first. Example: npm run serve
// BROWSER=firefox npm run test:browser
import assert from 'node:assert/strict';
import { mkdirSync, writeFileSync } from 'node:fs';
const {chromium,firefox}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const isFirefox=process.env.BROWSER==='firefox';
const browser=await (isFirefox?firefox:chromium).launch({
 headless:true,
 ...(process.env.BROWSER_EXECUTABLE?{executablePath:process.env.BROWSER_EXECUTABLE}:{}),
 ...(isFirefox?{firefoxUserPrefs:{'dom.webgpu.enabled':false,'webgl.force-enabled':true}}:
 {args:['--disable-webgpu','--use-angle=swiftshader','--enable-unsafe-swiftshader']}),
});
const artifacts=new URL('./artifacts/',import.meta.url);mkdirSync(artifacts,{recursive:true});
try{
 const page=await browser.newPage({viewport:{width:1440,height:1000}});
 const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.addInitScript(()=>Object.defineProperty(navigator,'gpu',{get(){throw new Error('WebGPU is forbidden in this app');}}));
 await page.goto(process.env.APP_URL || 'http://127.0.0.1:8080');
 const report=await page.evaluate(async()=>{
  const {runSimulation}=await import('./src/engine.js'),{compare}=await import('./validation/compare.js');
  const {cases}=await (await fetch('./validation/cpu-fixtures.json')).json();const checks=[];
  for(const fixture of cases){
   try{checks.push(compare(await runSimulation(fixture.config),fixture));}
   catch(e){checks.push({name:fixture.name,pass:false,failures:[e.message]});}
  }
  const input={iterations:65,duration:30};
  const a=await runSimulation(input,{batchSize:16,trace:false}),b=await runSimulation(input,{batchSize:65,trace:false});
  checks.push({name:'batch size / odd count invariance',pass:JSON.stringify(a.states)===JSON.stringify(b.states)});
  const repeat=await runSimulation(input,{batchSize:16,trace:false});
  checks.push({name:'repeatability',pass:JSON.stringify(a.states)===JSON.stringify(repeat.states)});
  const batch=await runSimulation({iterations:5},{trace:false,batchSize:3});
  const seeds=[42,43,44,45,46];
  checks.push({name:'every batch lane against CPU',pass:seeds.every((seed,i)=>{
   const expected=cases.find(c=>c.config.seed===seed&&c.name===(seed===42?'shadow baseline':`shadow seed ${seed}`)).expected;
   return Object.entries(expected).every(([key,value])=>{
    if(key==='nextRandom'||key==='damageTrace')return true;
    const tolerance=/^(total|mana|spent|gained|damage\d)$/.test(key)?Math.max(.02,Math.abs(value)*.00002):0;
    return Math.abs(batch.states[i][key]-value)<=tolerance;
   });
  })});
  try{await runSimulation({iterations:1},{eventBudget:1});checks.push({name:'event budget rejection',pass:false});}
  catch(e){checks.push({name:'event budget rejection',pass:e.message.includes('Incomplete results rejected')});}
  const controller=new AbortController();let cancelled=false;
  try{await runSimulation({iterations:64},{batchSize:16,signal:controller.signal,onProgress:p=>{if(p.completed>=16)controller.abort();}});}
  catch(e){cancelled=e.name==='AbortError';}
  checks.push({name:'cancellation after a submitted batch',pass:cancelled});
  const {checkAccounting}=await import('./validation/accounting.js');
  checks.push(...await checkAccounting());
  return {checks,timing:a.timing,userAgent:navigator.userAgent};
 });
 await page.selectOption('[name=iterations]','256');await page.fill('[name=duration]','30');await page.click('#run');
 await page.waitForFunction(()=>document.querySelector('#status').textContent.startsWith('Complete'),null,{timeout:120000});
 assert.notEqual(await page.textContent('#mean'),'—');
 assert.equal(await page.locator('#export').isEnabled(),true);
 await page.screenshot({path:new URL(`${isFirefox?'firefox':'chromium'}-desktop.png`,artifacts).pathname,fullPage:true});
 await page.setViewportSize({width:390,height:844});
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true,'Mobile page overflows');
 await page.screenshot({path:new URL(`${isFirefox?'firefox':'chromium'}-mobile.png`,artifacts).pathname,fullPage:true});
 writeFileSync(new URL(`${isFirefox?'firefox':'chromium'}.json`,artifacts),JSON.stringify({...report,errors},null,2));
 for(const check of report.checks)console.log(`${check.pass?'PASS':'FAIL'} ${check.name}${check.failures?.length?`: ${check.failures.join('; ')}`:''}`);
 assert.equal(errors.length,0,errors.join('\n'));
 assert.equal(report.checks.filter(c=>!c.pass).length,0,'Browser checks failed');
 console.log(`${report.checks.length} checks passed; desktop and mobile screenshots captured.`);
}finally{await browser.close();}
