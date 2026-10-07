import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { exportLogicalBuild, resolveSavedBuild } from '../src/contracts/build_io.js';
import { packConfig } from '../src/model.js';
const itemDatabase=JSON.parse(readFileSync(new URL('../data/items.json',import.meta.url)));
const transport = value => JSON.parse(Buffer.from(Buffer.from(JSON.stringify(value),'utf8').toString('base64'),'base64').toString('utf8'));
test('legacy and v1 logical builds round trip with exact configs and packed words',async()=>{
 const inputs=[{stats:{intellect:200,spellPower:500},race:'GNOME',buffs:{arcaneIntellect:true},aplText:'Shadow Bolt'},
  {gearMode:'equipped',gear:itemDatabase.presets.p6.items,stats:{intellect:999},race:'ORC',aplText:'Hellfire'},
  {stats:{intellect:0,hit:0,crit:0},buffs:{blessingOfKings:false}}];
 for(const input of inputs){
  const saved=structuredClone(input),original=await resolveSavedBuild(input,{itemDatabase});
  const envelope=await exportLogicalBuild(input,{itemDatabase});
  const imported=await resolveSavedBuild(transport(envelope),{itemDatabase});
  assert.deepEqual(imported.config,original.config);
  assert.deepEqual(packConfig(imported.config),packConfig(original.config));
  assert.deepEqual(await exportLogicalBuild(transport(envelope),{itemDatabase}),envelope);
  assert.deepEqual(input,saved);
 }
});
test('rules-form APLs round trip without trusting numeric action fields',async()=>{
 const saved=await exportLogicalBuild({stats:{}});
 saved.candidate.apl={kind:'rules',rules:[{id:'bolt',rawCond:'true',enabled:true,action:999}]};
 const resolved=await resolveSavedBuild(transport(saved));
 assert.notEqual(resolved.config.aplRules[0].action,999);
 const again=await exportLogicalBuild(saved);
 assert.deepEqual((await resolveSavedBuild(transport(again))).config,resolved.config);
 for(const rawCond of ['', 'true\nFrostbolt']) {
  const invalid=structuredClone(saved);invalid.candidate.apl.rules[0].rawCond=rawCond;
  await assert.rejects(resolveSavedBuild(invalid));
 }
});
test('legacy resolved formats round trip without logical reconstruction or effect duplication',async()=>{
 const resolved=(await resolveSavedBuild({stats:{intellect:200},race:'GNOME',aplText:'Shadow Bolt'})).config;
 for(const payload of [{resolvedConfig:resolved},{config:resolved,summary:{}},resolved]) {
  const result=await resolveSavedBuild(transport(payload));
  assert.deepEqual(result.config,resolved);
  await assert.rejects(exportLogicalBuild(payload),/cannot be reconstructed/);
 }
});
test('unsupported versioned resolved inputs and malformed resolved bytecode fail explicitly',async()=>{
 await assert.rejects(resolveSavedBuild({schemaVersion:1,classId:'warlock',kind:'resolved-config',resolvedConfig:{},packingVersion:'unknown',simulationVersion:'unknown'}),/compatibility/);
 for(const rule of [{action:999,enabled:1},{action:16,enabled:1,cond:999},{action:16,enabled:1,param:NaN},{action:16,enabled:1,targetSpell:999}]) {
  await assert.rejects(resolveSavedBuild({resolvedConfig:{aplRules:[rule]}}));
 }
});
