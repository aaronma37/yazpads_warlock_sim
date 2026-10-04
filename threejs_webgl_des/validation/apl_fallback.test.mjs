import test from 'node:test';
import assert from 'node:assert/strict';
import { candidateFallbackRotation, fallbackRow, fallbackSummary } from '../src/apl_fallback_view.js';
import { initAPL, renderAPLTable, getActiveBytecodeRules } from '../src/apl.js';

test('candidate display uses evaluated rotation rather than guessing from visible rules', () => {
  const candidate = { rules: [{ id: 'bolt', enabled: true }], batch: { config: { rotation: 'searing' } } };
  assert.match(fallbackSummary(candidateFallbackRotation(candidate)), /Searing Pain/);
  assert.match(fallbackRow('fire'), /Incinerate/);
  assert.match(fallbackRow('bolt'), /Shadow Bolt/);
  assert.match(fallbackRow('shadow'), /otherwise Life Tap/);
  assert.equal(candidateFallbackRotation({ rules: candidate.rules }), undefined);
});

test('rendering fallback follows active rotation without changing executable rules', () => {
  const before = JSON.stringify(getActiveBytecodeRules());
  let markup = '', rotation = 'searing';
  const tbody = {
    set innerHTML(value) { markup = value; },
    appendChild() {},
    querySelector() { return null; },
    insertAdjacentHTML(position, html) { markup += html; },
  };
  globalThis.document = {
    getElementById(id) { return id === 'apl-table-body' ? tbody : null; },
    createElement() { return { dataset: {}, querySelector() { return null; }, addEventListener() {} }; },
  };
  try {
    initAPL(() => {}, () => rotation);
    assert.match(markup, /Automatic fallback · Searing Pain/);
    rotation = 'fire';
    renderAPLTable();
    assert.match(markup, /Automatic fallback · Incinerate/);
    assert.equal(JSON.stringify(getActiveBytecodeRules()), before);
  } finally {
    delete globalThis.document;
  }
});
