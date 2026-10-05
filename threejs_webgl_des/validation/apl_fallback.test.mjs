import test from 'node:test';
import assert from 'node:assert/strict';
import { getEnabledAPLRules } from '../src/apl_rules.js';
import { APL_ACTION, APL_COND, CONFIG, packConfig } from '../src/model.js';
import { candidateFallbackRotation, fallbackRow, fallbackSummary } from '../src/apl_fallback_view.js';
import { initAPL, renderAPLTable, getActiveBytecodeRules, DEFAULT_APL, generateAPLForPreset, compileAPLToBytecode, applySynthesizedAPL, getActiveAPL, updateAPLCondition } from '../src/apl.js';

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

test('Hellfire is available in presets and compiles when enabled', () => {
  for (const rows of [DEFAULT_APL, generateAPLForPreset()]) {
    const row = rows.find(entry => entry.id === 'hellfire');
    assert.equal(row.spell, 'Hellfire');
    assert.equal(row.enabled, false);
    assert.ok(rows.indexOf(row) < rows.findIndex(entry => entry.id === 'bolt'));
    const rules = compileAPLToBytecode([{ ...row, enabled: true }]);
    assert.equal(rules.length, 1);
    assert.equal(rules[0].action, 19);
  }
});

test('synthesis display and application preserve enabled rules after cooldown actions', () => {
  globalThis.document = { getElementById() { return null; } };
  const original = getActiveAPL().map(rule => ({ ...rule }));
  const rules = [
    { id: 'shadowburn', action: APL_ACTION.SHADOWBURN, cond1: 0, cond2: 0, rawCond: 'true', enabled: true },
    { id: 'wrack', action: APL_ACTION.DRAIN_HOPE, cond1: 0, cond2: 0, rawCond: 'true', enabled: true },
    { id: 'bolt', action: APL_ACTION.SHADOW_BOLT_FILLER, cond1: APL_COND.MANA_GT, param1: 25, cond2: 0, rawCond: 'mana_pct > 25', enabled: true },
    { id: 'hellfire', action: APL_ACTION.HELLFIRE, cond1: 0, cond2: 0, enabled: false },
    { id: 'corr', action: APL_ACTION.CORRUPTION, condKey1: 'NEVER', cond1: 0, enabled: true },
  ];
  try {
    assert.deepEqual(getEnabledAPLRules(rules).map(r => r.id), ['shadowburn', 'wrack', 'bolt']);
    applySynthesizedAPL(rules);
    assert.deepEqual(getActiveAPL().map(r => r.enabled), [true, true, true, false, false]);
    const bytecode = getActiveBytecodeRules();
    assert.deepEqual(bytecode.map(r => r.enabled), [1, 1, 1, 0, 0]);
    assert.equal(bytecode[2].cond1, APL_COND.MANA_GT);
    const words = packConfig({ aplRules: bytecode });
    const keys = Object.keys(CONFIG);
    assert.equal(words[keys.indexOf('aplHeader0_2')] >>> 24, 1);
    assert.equal(words[keys.indexOf('aplHeader0_3')] >>> 24, 0);
  } finally {
    applySynthesizedAPL(original);
    delete globalThis.document;
  }
});

test('editing synthesized conditions replaces old bytecode and validates before saving', () => {
  globalThis.document = { getElementById() { return null; } };
  const original = getActiveAPL().map(rule => ({ ...rule }));
  const rules = [{ id: 'hellfire', action: APL_ACTION.HELLFIRE, enabled: true,
    cond1: APL_COND.FIGHT_TIME_GE, param1: 15, targetSpell1: 0,
    cond2: APL_COND.MANA_GE, param2: 30, targetSpell2: 0,
    rawCond: 'target_ttd >= 15 && mana_pct >= 30', condition: 'Original' }];
  try {
    applySynthesizedAPL(rules);
    const before = getActiveBytecodeRules();
    updateAPLCondition(0, 'New label', rules[0].rawCond);
    assert.deepEqual(getActiveBytecodeRules(), before, 'label edits preserve both compiled conditions');
    updateAPLCondition(0, 'New condition', 'target_ttd > 20 && mana_pct > 40');
    let [rule] = getActiveBytecodeRules();
    assert.equal(rule.cond1, APL_COND.FIGHT_TIME_GT);
    assert.equal(rule.param1, 20);
    assert.equal(rule.cond2, APL_COND.MANA_GT);
    assert.equal(rule.param2, 40);
    updateAPLCondition(0, 'Always', 'true');
    [rule] = getActiveBytecodeRules();
    assert.equal(rule.cond1, APL_COND.ALWAYS);
    assert.equal(rule.cond2, APL_COND.ALWAYS);
    const valid = JSON.stringify(getActiveAPL());
    assert.throws(() => updateAPLCondition(0, 'Bad condition', 'mana_pct === potato'), /not supported/);
    assert.equal(JSON.stringify(getActiveAPL()), valid, 'invalid edits leave the saved APL intact');
    updateAPLCondition(0, 'Never Use', 'false');
    assert.equal(getActiveBytecodeRules()[0].enabled, 0);
  } finally {
    applySynthesizedAPL(original);
    delete globalThis.document;
  }
});

test('current configuration edit and toggle buttons notify with the updated executable rules', () => {
  const original = getActiveAPL().map(rule => ({ ...rule }));
  const node = () => ({ value: '', style: {}, dataset: {}, handlers: {},
    addEventListener(type, handler) { this.handlers[type] = handler; } });
  const nodes = Object.fromEntries(['apl-condition-modal', 'btn-save-apl-cond', 'apl-modal-cond-label',
    'apl-modal-cond-raw', 'apl-modal-cond-error'].map(id => [id, node()]));
  const rows = [];
  const tbody = { set innerHTML(value) { rows.length = 0; }, appendChild(row) { rows.push(row); },
    querySelector() { return null; }, insertAdjacentHTML() {} };
  globalThis.document = {
    getElementById(id) { return id === 'apl-table-body' ? tbody : nodes[id] || null; },
    createElement() {
      const row = node();
      const buttons = Object.fromEntries(['.edit-cond-btn', '.toggle-btn'].map(key => [key, node()]));
      row.querySelector = key => buttons[key] || null;
      return row;
    }
  };
  let notifications = 0;
  try {
    initAPL(() => { notifications++; });
    applySynthesizedAPL([{ id: 'bolt', spell: 'Shadow Bolt', enabled: true,
      action: APL_ACTION.SHADOW_BOLT_FILLER, cond1: 0, cond2: 0, rawCond: 'true' }]);
    notifications = 0;
    rows[0].querySelector('.edit-cond-btn').handlers.click({ stopPropagation() {} });
    nodes['apl-modal-cond-label'].value = 'Mana > 35%';
    nodes['apl-modal-cond-raw'].value = 'mana_pct > 35';
    nodes['btn-save-apl-cond'].handlers.click();
    assert.equal(notifications, 1);
    assert.equal(nodes['apl-condition-modal'].style.display, 'none');
    assert.equal(getActiveBytecodeRules()[0].param1, 35);
    assert.equal(getActiveBytecodeRules()[0].cond1, APL_COND.MANA_GT);
    rows[0].querySelector('.toggle-btn').handlers.click({ stopPropagation() {} });
    assert.equal(getActiveBytecodeRules()[0].enabled, 0);
    rows[0].querySelector('.toggle-btn').handlers.click({ stopPropagation() {} });
    assert.equal(getActiveBytecodeRules()[0].enabled, 1);
    assert.equal(getActiveBytecodeRules()[0].param1, 35);
    assert.equal(notifications, 3);
    rows[0].querySelector('.edit-cond-btn').handlers.click({ stopPropagation() {} });
    nodes['apl-modal-cond-raw'].value = 'unsupported';
    nodes['btn-save-apl-cond'].handlers.click();
    assert.equal(nodes['apl-condition-modal'].style.display, 'flex');
    assert.match(nodes['apl-modal-cond-error'].textContent, /not supported/);
    assert.equal(notifications, 3);
    assert.equal(getActiveBytecodeRules()[0].param1, 35);
  } finally {
    initAPL(() => {});
    applySynthesizedAPL(original);
    delete globalThis.document;
  }
});
