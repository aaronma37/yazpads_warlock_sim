import test from 'node:test';
import assert from 'node:assert/strict';
import { APL_ACTION, APL_COND } from '../src/model.js';
import {
  parseAPLLine,
  parseAPLText,
  formatAPLEntry,
  formatAPLToText,
  compileAPLToBytecode,
  normalizeConditionForAction,
  initAPL,
  getActiveAPL,
  getActiveBytecodeRules,
  setAPLPreset,
  applySynthesizedAPL
} from '../src/apl.js';

test('parseAPLLine parses action names and condition operators', () => {
  // Canonical action names
  const rule1 = parseAPLLine('shadow_bolt', 1);
  assert.equal(rule1.id, 'bolt');
  assert.equal(rule1.action, APL_ACTION.SHADOW_BOLT_FILLER);
  assert.equal(rule1.rawCond, 'true');

  // Action with if mana_remaining > 40%
  const rule2 = parseAPLLine('shadow_bolt if mana_remaining > 40%', 2);
  assert.equal(rule2.id, 'bolt');
  assert.equal(rule2.rawCond, 'mana_pct > 40');
  const [bytecode2] = compileAPLToBytecode([rule2]);
  assert.equal(bytecode2.action, APL_ACTION.SHADOW_BOLT_FILLER);
  assert.equal(bytecode2.cond1, APL_COND.MANA_GT);
  assert.equal(bytecode2.param1, 40);

  // Searing Pain with condition
  const rule3 = parseAPLLine('searing_pain if mana_remaining > 30%', 3);
  assert.equal(rule3.id, 'searing');
  const [bytecode3] = compileAPLToBytecode([rule3]);
  assert.equal(bytecode3.action, APL_ACTION.SEARING_PAIN_FILLER);
  assert.equal(bytecode3.cond1, APL_COND.MANA_GT);
  assert.equal(bytecode3.param1, 30);

  // Bane of Doom with condition
  const rule4 = parseAPLLine('bane_of_doom if target_ttd >= 60s', 4);
  assert.equal(rule4.id, 'curse');
  const [bytecode4] = compileAPLToBytecode([rule4]);
  assert.equal(bytecode4.action, APL_ACTION.CURSE_OF_DOOM);
  assert.equal(bytecode4.cond1, APL_COND.FIGHT_TIME_GE);
  assert.equal(bytecode4.param1, 60);

  // Target HP execute condition
  const rule5 = parseAPLLine('decimate_searing if target_hp <= 35% && decimation.inactive', 5);
  assert.equal(rule5.id, 'decimateSearing');
  const [bytecode5] = compileAPLToBytecode([rule5]);
  assert.equal(bytecode5.action, APL_ACTION.DECIMATION_SEARING_PAIN);
  assert.equal(bytecode5.cond1, APL_COND.TARGET_HP_LE);
  assert.equal(bytecode5.param1, 35);
  assert.equal(bytecode5.cond2, APL_COND.DECIMATION_INACTIVE);

  // Shadow Trance / Nightfall condition
  const rule6 = parseAPLLine('nightfall if buff.shadow_trance', 6);
  assert.equal(rule6.id, 'nightfall');
  const [bytecode6] = compileAPLToBytecode([rule6]);
  assert.equal(bytecode6.action, APL_ACTION.NIGHTFALL_SHADOW_BOLT);
  assert.equal(bytecode6.cond1, APL_COND.SHADOW_TRANCE);

  // Comments and empty lines return null
  assert.equal(parseAPLLine('# This is a comment', 7), null);
  assert.equal(parseAPLLine('   // Another comment', 8), null);
  assert.equal(parseAPLLine('   ', 9), null);
});

test('parseAPLLine throws descriptive error on unknown action or invalid condition', () => {
  assert.throws(() => parseAPLLine('unknown_spell if mana > 50%', 1), /Line 1: Unknown action/);
  assert.throws(() => parseAPLLine('shadow_bolt if unknown_condition_expression', 2), /Line 2/);
});

test('parseAPLText parses full multiline APL string with canonical names and aliases', () => {
  const aplText = `
    # Custom Warlock Priority List
    life_tap if mana_remaining < 25%
    bane_of_doom if target_ttd >= 60s
    corruption if dot_remains <= 0
    searing_pain if mana_remaining > 50%
    shadow_bolt if mana_remaining > 40%
    shadow_bolt
  `;

  const rules = parseAPLText(aplText);
  assert.equal(rules.length, 6);
  assert.deepEqual(rules.map(r => r.id), ['tap', 'curse', 'corr', 'searing', 'bolt', 'bolt']);

  const bytecode = compileAPLToBytecode(rules);
  assert.equal(bytecode.length, 6);
  assert.equal(bytecode[0].action, APL_ACTION.LIFE_TAP);
  assert.equal(bytecode[0].cond1, APL_COND.MANA_LT);
  assert.equal(bytecode[0].param1, 25);
  assert.equal(bytecode[1].action, APL_ACTION.CURSE_OF_DOOM);
  assert.equal(bytecode[1].cond1, APL_COND.FIGHT_TIME_GE);
  assert.equal(bytecode[1].param1, 60);
  assert.equal(bytecode[3].action, APL_ACTION.SEARING_PAIN_FILLER);
  assert.equal(bytecode[3].cond1, APL_COND.MANA_GT);
  assert.equal(bytecode[3].param1, 50);
  assert.equal(bytecode[4].action, APL_ACTION.SHADOW_BOLT_FILLER);
  assert.equal(bytecode[4].cond1, APL_COND.MANA_GT);
  assert.equal(bytecode[4].param1, 40);
  assert.equal(bytecode[5].action, APL_ACTION.SHADOW_BOLT_FILLER);
  assert.equal(bytecode[5].cond1, APL_COND.ALWAYS);
});

test('formatAPLToText formats APL rules into clean text DSL using canonical names', () => {
  const rules = [
    { id: 'tap', spell: 'Life Tap', rawCond: 'mana_pct <= 25', enabled: true },
    { id: 'curse', spell: 'Bane of Doom', rawCond: 'target_ttd >= 60', enabled: true },
    { id: 'searing', spell: 'Searing Pain', rawCond: 'mana_pct > 30', enabled: true },
    { id: 'bolt', spell: 'Shadow Bolt', rawCond: 'mana_pct > 40', enabled: true },
    { id: 'bolt', spell: 'Shadow Bolt', rawCond: 'true', enabled: true },
    { id: 'hellfire', spell: 'Hellfire', rawCond: 'true', enabled: false }, // disabled should be omitted
  ];

  const text = formatAPLToText(rules);
  assert.match(text, /life_tap if mana_remaining <= 25%/);
  assert.match(text, /bane_of_doom if target_ttd >= 60s/);
  assert.match(text, /searing_pain if mana_remaining > 30%/);
  assert.match(text, /shadow_bolt if mana_remaining > 40%/);
  assert.match(text, /\nshadow_bolt$/);
  assert.doesNotMatch(text, /hellfire/);
});

test('APL edit box DOM integration reacts to typing and invalid input feedback', () => {
  const domBox = {
    value: '',
    classList: {
      tokens: new Set(),
      add(cls) { this.tokens.add(cls); },
      remove(cls) { this.tokens.delete(cls); },
      contains(cls) { return this.tokens.has(cls); }
    },
    listeners: {},
    addEventListener(evt, fn) { this.listeners[evt] = fn; }
  };

  const domError = {
    textContent: '',
    style: { display: 'none' }
  };

  globalThis.document = {
    activeElement: null,
    getElementById(id) {
      if (id === 'apl-edit-box') return domBox;
      if (id === 'apl-text-error') return domError;
      return null;
    }
  };

  let notifyCount = 0;
  try {
    initAPL(() => { notifyCount++; });

    // Simulate user typing a valid APL
    domBox.value = 'tap if mana_remaining < 20%\nbolt if mana_remaining > 40%\nbolt';
    domBox.listeners.input();

    assert.equal(notifyCount, 1);
    assert.equal(domError.style.display, 'none');
    assert.equal(domBox.classList.contains('input-invalid'), false);
    assert.equal(getActiveAPL().length, 3);
    assert.equal(getActiveBytecodeRules()[1].param1, 40);

    // Simulate user typing an invalid line
    domBox.value = 'tap if mana_remaining < 20%\nblablabla\nbolt';
    domBox.listeners.input();

    assert.equal(notifyCount, 1); // should not notify on error
    assert.equal(domError.style.display, 'block');
    assert.match(domError.textContent, /Line 2: Unknown action 'blablabla'/);
    assert.equal(domBox.classList.contains('input-invalid'), true);

    // Fix the error
    domBox.value = 'tap if mana_remaining < 20%\nbolt';
    domBox.listeners.input();

    assert.equal(notifyCount, 2);
    assert.equal(domError.style.display, 'none');
    assert.equal(domBox.classList.contains('input-invalid'), false);
    assert.equal(getActiveAPL().length, 2);
  } finally {
    delete globalThis.document;
  }
});
