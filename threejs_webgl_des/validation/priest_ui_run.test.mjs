import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {resolvePriestUIBuild} from '../src/priest/ui_config.js';
import {createPriestRunController} from '../src/priest/run_controller.js';
const data=JSON.parse(readFileSync(new URL('../data/priest_preview.json',import.meta.url)));
const stats={spellPower:500,shadowPower:100,holyPower:0,hit:12,crit:15,intellect:200,stamina:220,spirit:100,mp5:20};
const input={stats,sim:{duration:180,iterations:4096},preset:data.presets.find(p=>p.id==='shadow_core'),trees:data.trees,target:{level:63,resistance:0}};

test('Runnable Shadow core resolves direct stats and all allocated combat talents without deferring effects',()=> {
  const before=structuredClone(input);const build=resolvePriestUIBuild(input);
  assert.deepEqual(input,before);assert.equal(build.config.spellPower,600);assert.equal(build.config.maxMana,4456);
  assert.equal(build.config.hitChance,1);assert.equal(build.config.weavingChance,1);assert.equal(build.config.mindBlastCooldown,6);
  assert.equal(build.config.manaPerTick,8);assert.ok(build.config.spiritManaPerTick>0);assert.equal(build.config.castingRegenRatio,0.5);assert.equal(build.config.innerFocusEnabled,1);
  assert.equal(build.effects.some(effect=>effect.status==='pending'),false);
  assert.throws(()=>resolvePriestUIBuild({...input,target:{resistance:10}}),/resistance/);
  assert.doesNotThrow(()=>resolvePriestUIBuild({...input,preset:data.presets.find(p=>p.id==='shadow_standard')}));
  assert.throws(()=>resolvePriestUIBuild({...input,stats:{...stats,intellect:NaN}}),/intellect/);
});

test('Priest browser controller passes validated config, mode and progress to the Priest runtime only',async()=> {
  const events=[],build=resolvePriestUIBuild(input),result={summary:{mean:1}};
  const controller=createPriestRunController({readBuild:()=>build,
    loadSimulation:async()=>({id:'priest',runSimulation:async(config,options)=>{
      assert.equal(config,build.config);assert.equal(options.detailedResults,false);assert.ok(options.signal instanceof AbortSignal);
      options.onProgress(1);return result;
    }}),onProgress:progress=>events.push(progress),onBusy:busy=>events.push(busy),onResult:value=>events.push(value)});
  assert.equal(await controller.run({detailedResults:false}),result);
  assert.deepEqual(events,[true,1,result,false]);assert.equal(controller.isRunning(),false);
  let called=false,status;
  const invalid=createPriestRunController({readBuild:()=>build,loadSimulation:async()=>({id:'warlock',runSimulation:()=>{called=true;}}),onStatus:value=>{status=value;}});
  await invalid.run();assert.equal(called,false);assert.match(status,/identity mismatch/);
});

test('Priest run cancellation during runtime loading prevents dispatch and duplicate clicks',async()=> {
  let resolve,loads=0,calls=0,status;
  const pending=new Promise(done=>{resolve=done;});
  const controller=createPriestRunController({readBuild:()=>({config:{}}),loadSimulation:()=>{loads++;return pending;},onStatus:value=>{status=value;}});
  const task=controller.run();assert.equal(controller.isRunning(),true);
  await controller.run();assert.equal(loads,1);
  controller.cancel();resolve({id:'priest',runSimulation:()=>{calls++;}});
  await task;assert.equal(calls,0);assert.equal(status,'Cancelled');assert.equal(controller.isRunning(),false);
});

test('Holy damage preset runs its school-specific spells and resolves intellect/spirit talents',()=> {
  const build=resolvePriestUIBuild({...input,preset:data.presets.find(p=>p.id==='holy_core')});
  assert.equal(build.config.rotation,'holy');assert.equal(build.config.penanceEnabled,1);assert.equal(build.config.holyNovaEnabled,1);
  assert.ok(Math.abs(build.summary.intellect-230)<1e-10);assert.equal(build.config.maxMana,4906);
  assert.equal(build.config.holySpellPower,508.4);assert.equal(build.config.spellPower,608.4);
  assert.equal(build.config.holyHitChance,1);assert.ok(Math.abs(build.config.hitChance-.95)<1e-10);
  assert.equal(build.config.damageMultiplier,1);assert.equal(build.config.costMultiplier,1);
  assert.equal(build.effects.some(effect=>effect.status==='pending'),false);
});

test('Standard and Smite presets are executable; all races enable shared Plague',()=> {
  for(const preset of data.presets)assert.doesNotThrow(()=>resolvePriestUIBuild({...input,preset}));
  assert.equal(resolvePriestUIBuild({...input,preset:data.presets.find(p=>p.id==='smite_dps')}).config.rotation,'holy');
  const human=resolvePriestUIBuild({...input,preset:data.presets.find(p=>p.id==='shadow_standard')});
  const undead=resolvePriestUIBuild({...input,preset:data.presets.find(p=>p.id==='undead_shadow')});
  assert.equal(human.config.plagueEnabled,1);assert.equal(undead.config.plagueEnabled,1);
  assert.equal(undead.config.plagueCostMultiplier,.5);assert.equal(undead.race,'UNDEAD');
  assert.ok(undead.config.spiritManaPerTick<human.config.spiritManaPerTick);
  const pi=resolvePriestUIBuild({...input,preset:data.presets.find(p=>p.id==='power_infusion_dps')});
  assert.equal(pi.config.powerInfusionEnabled,1);assert.equal(pi.config.searingNovaChance,.1);
  assert.equal(pi.config.holyDamageMultiplier,1.05);
  assert.throws(()=>resolvePriestUIBuild({...input,preset:{...input.preset,race:'FAKE'}}),/race/);
});


test('Night Elf Starshards uses general spell power and hit without Shadow or Holy bonuses',()=>{
  const build=resolvePriestUIBuild({...input,preset:{...input.preset,race:'NIGHT_ELF'}});
  assert.equal(build.config.starshardsEnabled,1);assert.equal(build.config.plagueEnabled,1);
  assert.equal(build.config.arcaneHitChance,Math.min(1,.83+stats.hit/100));
  assert.equal(build.config.arcaneDamageMultiplier,1);assert.equal(build.config.arcaneCostMultiplier,1);
  assert.ok(build.config.arcaneSpellPower>=stats.spellPower);
});
