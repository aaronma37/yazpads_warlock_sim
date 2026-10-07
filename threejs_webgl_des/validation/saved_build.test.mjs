import test from 'node:test';
import assert from 'node:assert/strict';
import { validateSavedBuildEnvelope as validate } from '../src/contracts/saved_build.js';

function logical() {
  return { schemaVersion: 1, classId: 'warlock', kind: 'logical-build',
    candidate: { schemaVersion: 1, classId: 'warlock', candidateId: 'candidate-0' } };
}

test('legacy decoded formats retain their original data and default only missing class identity', () => {
  for (const payload of [{ stats: { intellect: 300 }, talents: {} }, { resolvedConfig: {} },
    { config: {}, summary: {} }, { duration: 180, iterations: 100, spellPower: 500 },
    { classId: 'warlock', stats: {} }]) {
    const original = structuredClone(payload);
    const result = validate(payload);
    assert.equal(result.legacy, true);
    assert.equal(result.classId, 'warlock');
    assert.equal(result.payload, payload);
    assert.deepEqual(payload, original);
  }
  for (const classId of ['priest', '__proto__', 'constructor', '', null, undefined, 1]) {
    assert.throws(() => validate({ classId }));
  }
});

test('unknown versions and missing versioned class identity fail without legacy fallback', () => {
  for (const schemaVersion of [0, 2, '1', null, undefined]) {
    assert.throws(() => validate({ ...logical(), schemaVersion }), /Unsupported saved build schema/);
  }
  const payload = logical();
  delete payload.classId;
  assert.throws(() => validate(payload), /requires classId/);
  for (const value of [null, [], 'build', 123]) assert.throws(() => validate(value), /must be an object/);
});

test('logical envelopes check nested identity and candidate versions without mutating frozen input', () => {
  const payload = logical();
  Object.freeze(payload.candidate);
  Object.freeze(payload);
  assert.equal(validate(payload).payload, payload);
  for (const edit of [{ classId: 'priest' }, { classId: undefined }, { schemaVersion: 2 },
    { schemaVersion: undefined }, { candidateId: '' }, { candidateId: 1 }]) {
    assert.throws(() => validate({ ...payload, candidate: { ...payload.candidate, ...edit } }));
  }
  for (const candidate of [null, [], undefined]) assert.throws(() => validate({ ...payload, candidate }));
});

test('resolved envelopes require version identifiers and retain resolved data without applying effects', () => {
  const payload = { schemaVersion: 1, classId: 'warlock', kind: 'resolved-config',
    resolvedConfig: { intellect: 315 }, packingVersion: 'test-packing-v1', simulationVersion: 'test-simulation-v1' };
  const original = structuredClone(payload);
  assert.equal(validate(payload).kind, 'resolved-config');
  assert.deepEqual(payload, original);
  for (const field of ['packingVersion', 'simulationVersion', 'resolvedConfig']) {
    const edited = { ...payload };
    delete edited[field];
    assert.throws(() => validate(edited));
  }
});

test('envelopes reject unknown kinds and malformed evaluation defaults', () => {
  for (const kind of ['other', undefined, null]) assert.throws(() => validate({ ...logical(), kind }), /Unsupported saved build kind/);
  assert.equal(validate({ ...logical(), evaluationDefaults: {} }).legacy, false);
  for (const evaluationDefaults of [null, [], 'fast']) assert.throws(() => validate({ ...logical(), evaluationDefaults }));
});
