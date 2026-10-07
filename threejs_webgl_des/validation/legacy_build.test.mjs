import test from 'node:test';
import assert from 'node:assert/strict';
import { classifyLegacyBuild, migrateLegacyBuild } from '../src/contracts/legacy_build.js';

test('legacy classification preserves resolved precedence and raw-input detection', () => {
  assert.equal(classifyLegacyBuild({ resolvedConfig: {}, config: {}, stats: {} }).format, 'resolvedConfig');
  assert.equal(classifyLegacyBuild({ config: {}, stats: {} }).format, 'config');
  assert.equal(classifyLegacyBuild({ duration: 180, iterations: 100, spellPower: 0 }).format, 'raw-resolved');
  assert.equal(classifyLegacyBuild({ duration: 180, iterations: 100, spellPower: 0, stats: {} }).format, 'ui-build');
});

test('UI migration maps fields losslessly without interpreting rotation or reapplying effects', () => {
  const payload = { version: 1, race: 'GNOME', pet: 'imp', ds: 'succubus', rotation: 'fire',
    gearMode: 'direct', stats: { intellect: 315 }, gear: { head: 123 }, talents: { affliction: {} },
    buffs: { kings: true }, target: { level: 63, racialPolicy: 'execute' },
    sim: { duration: 180, distance: 30, iterations: 4096, detailedResults: false }, aplText: 'Life Tap' };
  const original = structuredClone(payload);
  const result = migrateLegacyBuild(payload, { candidateId: 'import-1' });
  assert.equal(result.candidate.stats.intellect, 315);
  assert.deepEqual(result.candidate.encounter, { target: payload.target, duration: 180, distance: 30 });
  assert.deepEqual(result.evaluationDefaults, { iterations: 4096, mode: 'fast' });
  assert.deepEqual(result.candidate.equipment, { items: payload.gear, mode: 'direct' });
  assert.deepEqual(result.candidate.apl, { kind: 'text', text: 'Life Tap' });
  assert.equal(result.candidate.classOptions.rotation, 'fire');
  assert.equal(result.candidate.classOptions.sacrifice, 'succubus');
  assert.deepEqual(JSON.parse(JSON.stringify(result)), result);
  result.candidate.stats.intellect = 1;
  result.candidate.encounter.target.level = 1;
  assert.deepEqual(payload, original);
});

test('resolved migration copies configs without manufacturing version compatibility or logical stats', () => {
  for (const payload of [{ resolvedConfig: { intellect: 315 } }, { config: { intellect: 315 }, summary: {} },
    { duration: 180, iterations: 100, spellPower: 500 }]) {
    const result = migrateLegacyBuild(payload);
    assert.equal(result.kind, 'resolved-config');
    assert.equal(result.classId, 'warlock');
    assert.equal(result.schemaVersion, undefined);
    assert.equal(result.candidate, undefined);
    const original = structuredClone(payload);
    result.resolvedConfig.intellect = 0;
    assert.deepEqual(payload, original);
  }
});

test('partial migration defers defaults and rejects malformed/unknown legacy shapes', () => {
  const result = migrateLegacyBuild({ stats: {} });
  assert.equal(result.candidate.apl, null);
  assert.deepEqual(result.evaluationDefaults, {});
  assert.equal(result.candidate.equipment.mode, undefined);
  for (const payload of [{}, { summary: {} }, { stats: [] }, { talents: null }, { race: 1 },
    { sim: { detailedResults: 'false' } }, { stats: {}, version: 2 }, { config: 'bad' },
    { classId: 'priest', stats: {} }, { schemaVersion: 1, classId: 'warlock', kind: 'resolved-config' }]) {
    assert.throws(() => migrateLegacyBuild(payload));
  }
  assert.throws(() => migrateLegacyBuild({ stats: {} }, { candidateId: '' }));
});
