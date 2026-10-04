import test from 'node:test';
import assert from 'node:assert/strict';
import { CONFIG, SPELLS, CPU_IDS, STATE, COMPACT_WORDS, COMPACT_STRIPES, packConfig, validate } from '../src/model.js';

const packed = input => {
  const words = packConfig(input);
  const floats = new Float32Array(words.buffer);
  return Object.fromEntries(Object.entries(CONFIG).map(([key, type], i) => [key, type === 'f32' ? floats[i] : words[i]]));
};

test('race and racial timing survive validation and GPU packing', () => {
  for (const race of ['HUMAN', 'ORC', 'TROLL', 'GNOME', 'UNDEAD']) {
    assert.equal(validate({ race }).race, race);
    assert.ok(Number.isInteger(packed({ race }).race));
  }
  assert.throws(() => validate({ race: 'unknown' }));
  assert.throws(() => validate({ racialPolicy: 'unknown' }));
  assert.equal(packed({ racialPolicy: 'cooldown' }).racialPolicy, 1);
  assert.equal(packed({ targetIsBeast: true }).targetIsBeast, 1);
  assert.equal(packed({ race: 'UNDEAD', maxHealth: 6000 }).maxHealth, 6000);
});

test('Command applies once to every pet attack and Gnome gains maximum mana', () => {
  const base = { petFireboltMult: 1.2, petMeleeMult: 1.3, petLashMult: 1.4 };
  const human = packed(base), orc = packed({ ...base, race: 'ORC' });
  for (const key of ['petFireboltMult', 'petMeleeMult', 'petLashMult']) {
    assert.ok(Math.abs(orc[key] / human[key] - 1.05) < 1e-6);
  }
  assert.ok(Math.abs(packed({ race: 'GNOME' }).maxMana / human.maxMana - 1.05) < 1e-6);
});

test('damage sources have independent counters and fit the GPU result layout', () => {
  for (const name of ['Bane of Doom', 'Soul Fire', 'Conflagrate', 'Shadowburn', 'Siphon Life', 'Wrack', 'Touch of the Grave']) {
    const i = SPELLS.indexOf(name);
    assert.ok(i >= 6);
    for (const key of ['damage', 'casts', 'hits', 'crits', 'misses']) assert.ok(`${key}${i}` in STATE);
  }
  assert.equal(new Set(CPU_IDS).size, SPELLS.length);
  assert.ok(COMPACT_STRIPES * 16 >= COMPACT_WORDS);
});
