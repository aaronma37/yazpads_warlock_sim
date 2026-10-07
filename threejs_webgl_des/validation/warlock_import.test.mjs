import test from 'node:test';
import assert from 'node:assert/strict';
import { migrateLegacyBuild } from '../src/contracts/legacy_build.js';
import { normalizeWarlockImport } from '../src/classes/warlock_import.js';
const normalize = payload => normalizeWarlockImport(migrateLegacyBuild(payload));
test('partial direct imports receive class defaults without mutation', () => {
 const input = { stats: { intellect: 315 }, race: 'gnome' };
 const result = normalize(input);
 assert.equal(result.candidate.stats.intellect,315);
 assert.equal(result.candidate.stats.spirit,100);
 assert.equal(result.candidate.classOptions.race,'GNOME');
 assert.equal(result.candidate.classOptions.pet,'imp');
 assert.equal(result.candidate.encounter.duration,180);
 assert.equal(result.evaluationDefaults.iterations,4096);
 assert.deepEqual(input,{stats:{intellect:315},race:'gnome'});
 assert.deepEqual(normalizeWarlockImport(result),result);
});
test('invalid explicit stats, talents, choices, and actions are rejected', () => {
 for(const payload of [{stats:{intellect:-1}},{stats:{haste:1}},{race:'priest'},
  {talents:{affliction:{nightfall:3}}},{talents:{affliction:{wrack:1}}},
  {ds:'imp'},{pet:'dragon'},{aplText:'Incinerate'},{aplText:'Frostbolt'},
  {sim:{iterations:0}},{target:{targetCount:2}},{gearMode:'equipped'}]) assert.throws(()=>normalize(payload));
 assert.doesNotThrow(()=>normalize({aplText:'Shadow Bolt'}));
});
test('resolved inputs validate directly and do not reapply racial or talent effects', () => {
 const result=normalize({resolvedConfig:{race:'GNOME',intellect:315,spellPower:777}});
 assert.equal(result.resolvedConfig.intellect,315);
 assert.equal(result.resolvedConfig.spellPower,777);
 assert.throws(()=>normalize({resolvedConfig:{intellect:-1}}));
});
