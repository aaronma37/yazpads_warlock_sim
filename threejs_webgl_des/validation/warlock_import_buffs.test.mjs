import test from 'node:test';
import assert from 'node:assert/strict';
import { migrateLegacyBuild } from '../src/contracts/legacy_build.js';
import { resolveWarlockImportEffects } from '../src/classes/warlock_import.js';
const resolve = p=>resolveWarlockImportEffects(migrateLegacyBuild(p));
test('buffs and racial effects preserve order, rounding, school bonuses, and resistance reduction',()=>{
 const input={race:'GNOME',stats:{intellect:200,stamina:220,spirit:100,spellPower:500},buffs:{
  arcaneIntellect:true,markOfTheWild:true,blessingOfKings:true,spiritOfZandalar:true,
  flaskSupremePower:true,shadowPowerElixir:true,curseOfShadow:true,warchiefsBlessing:true},target:{resistance:50}};
 const before=structuredClone(input),result=resolve(input);
 assert.equal(result.effectiveStats.intellect,Math.round((200+31+12)*1.1*1.15));
 assert.equal(result.effectiveStats.spellPower,650);
 assert.equal(result.effectiveStats.shadowPower,40);
 assert.equal(result.effectiveStats.resistance,0);
 assert.equal(result.effectiveStats.shadowMultiplier,1.1);
 assert.equal(result.effectiveStats.maxHealth,1500+Math.round((220+12)*1.1*1.15)*10+300);
 assert.deepEqual(input,before);
 assert.equal(result.candidate.stats.intellect,200);
 assert.deepEqual(resolveWarlockImportEffects(result),result);
});
test('resolved imports bypass buff/racial reapplication',()=>{
 const result=resolve({resolvedConfig:{intellect:315,spellPower:777,race:'GNOME'}});
 assert.equal(result.resolvedConfig.intellect,315);
 assert.equal(result.resolvedConfig.spellPower,777);
 assert.equal(result.effectiveStats,undefined);
});
const { resolveWarlockImport } = await import('../src/classes/warlock_import.js');
test('final resolution produces validated text/rules configs and skips resolved effects',()=>{
 const text=resolveWarlockImport(migrateLegacyBuild({race:'GNOME',stats:{intellect:200},aplText:'Shadow Bolt'}));
 assert.equal(text.config.intellect,200);
 assert.equal(text.config.aplRules.length,1);
 const rule={id:'bolt',rawCond:'true',enabled:true};
 const staged=migrateLegacyBuild({stats:{intellect:200}});
 staged.candidate.apl={kind:'rules',rules:[rule]};
 const result=resolveWarlockImport(staged);
 assert.equal(result.config.aplRules.length,1);
 assert.equal(staged.candidate.apl.rules[0],rule);
 staged.candidate.apl.rules[0]={id:'frostbolt',rawCond:'true',enabled:false};
 assert.throws(()=>resolveWarlockImport(staged));
 const resolved=resolveWarlockImport(migrateLegacyBuild({resolvedConfig:{intellect:315}}));
 assert.equal(resolved.config.intellect,315);
});
