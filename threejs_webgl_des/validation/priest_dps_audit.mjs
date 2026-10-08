// Run with node validation/priest_dps_audit.mjs; emits completed GPU fixture measurements.
import {fileURLToPath} from 'node:url';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {readFileSync,writeFileSync,mkdtempSync,rmSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import {resolvePriestUIBuild} from '../src/priest/ui_config.js';
import {packMultiConfig,packConfig,decodeState,STATE_WORDS,COMPACT_STRIPES} from '../src/priest/model.js';
import {FRAGMENT,VERTEX} from '../src/priest/kernel.js';
const root=fileURLToPath(new URL('../',import.meta.url));
const data=JSON.parse(readFileSync(root+'/data/priest_preview.json','utf8'));
const stats={spellPower:500,shadowPower:0,holyPower:0,hit:12,crit:15,intellect:200,spirit:100,stamina:220,mp5:20};
const inputs=[];
for(const id of ['shadow_core','shadow_standard','undead_shadow','holy_core','power_infusion_dps']){
 const preset=data.presets.find(p=>p.id===id);
 const config=resolvePriestUIBuild({stats,sim:{duration:180,iterations:64,seed:42},preset,trees:data.trees}).config;
 inputs.push({name:id+' default',config});
 if(id==='shadow_core'||id==='undead_shadow'){
  inputs.push({name:id+' unlimited mana',config:{...config,maxMana:1000000}});
  inputs.push({name:id+' 1000 SP',config:{...config,spellPower:1000,holySpellPower:1000}});
  inputs.push({name:id+' +10% damage',config:{...config,damageMultiplier:config.damageMultiplier*1.1}});
 }
}
const configs=inputs.map(x=>x.config),iterations=64,count=configs.length*iterations,width=64,height=Math.ceil(count/width)*COMPACT_STRIPES;
const texture=packMultiConfig(configs);
const directory=mkdtempSync(join(tmpdir(),'priest-dps-audit-')),manifest=join(directory,'manifest.json');
writeFileSync(manifest,JSON.stringify({vertex:'#version 300 es\n'+VERTEX,detailed:'#version 300 es\n'+FRAGMENT,fast:'#version 300 es\n'+FRAGMENT,cases:[{mode:'detailed',width,height,texture:{...texture,words:Array.from(texture.words)},configWords:Array.from(packConfig(configs[0])),uniforms:{numConfigs:configs.length,fightsPerConfig:iterations,seed:42,offset:0,count,gridWidth:width,eventBudget:65536}}]}));
const r=spawnSync('python3',[root+'/validation/headless_gles.py',manifest],{encoding:'utf8',maxBuffer:16*1024*1024,timeout:60000});
rmSync(directory,{recursive:true,force:true});
if(r.status!==0)throw new Error(r.stderr||r.error?.message);
const {outputs}=JSON.parse(r.stdout),reports=inputs.map(x=>({...x.name?{name:x.name}:{},states:[]}));
for(let lane=0;lane<count;lane++){
 const words=new Uint32Array(STATE_WORDS),x=lane%width,y=Math.floor(lane/width);
 for(let word=0;word<STATE_WORDS;word++)words[word]=outputs[0][Math.floor(word%16/4)][((y*COMPACT_STRIPES+Math.floor(word/16))*width+x)*4+word%4];
 reports[Math.floor(lane/iterations)].states.push(decodeState(words,true));
}
for(const report of reports){const avg=key=>report.states.reduce((s,x)=>s+x[key],0)/iterations;console.log(JSON.stringify({name:report.name,dps:avg('total')/180,manaSpent:avg('manaSpent'),manaGained:avg('manaGained'),manaLeft:avg('mana'),casts:Array.from({length:9},(_,i)=>avg('casts'+i)),damage:Array.from({length:9},(_,i)=>avg('damage'+i)/180)}));}
