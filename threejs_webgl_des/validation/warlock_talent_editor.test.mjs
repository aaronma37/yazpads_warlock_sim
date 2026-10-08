import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { initTalents, applyTalentsObject, canChangeTalentPoint, getTalentsObject } from '../src/talents.js';

const data = JSON.parse(readFileSync(new URL('../data/talents.json', import.meta.url)));
globalThis.document = { getElementById: () => null, querySelectorAll: () => [] };
globalThis.fetch = async () => ({ ok: true, json: async () => data });
await initTalents();
const can = (tree, name, delta) => canChangeTalentPoint(tree, data.trees[tree].talents.findIndex(t => t.name === name), delta);

test('Refunds preserve row requirements while allowing spare and leaf points', () => {
  applyTalentsObject({ affliction: { suppression: 5, malediction: 1 } });
  assert.equal(can(0, 'Suppression', -1), false);
  assert.equal(can(0, 'Malediction', -1), true);
  applyTalentsObject({ affliction: { suppression: 5, improved_corruption: 1, malediction: 1 } });
  assert.equal(can(0, 'Suppression', -1), true);
  assert.equal(can(0, 'Improved Corruption', -1), true);
  applyTalentsObject({ affliction: { suppression: 5 } });
  assert.equal(can(0, 'Suppression', -1), true);
});

test('Higher-row points cannot pay for lower-row requirements', () => {
  applyTalentsObject({ affliction: { suppression: 5, malediction: 5, fel_concentration: 3 } });
  assert.equal(can(0, 'Suppression', -1), false);
  assert.equal(can(0, 'Malediction', -1), false);
  assert.equal(can(0, 'Fel Concentration', -1), true);
});

test('Prerequisites stay fully ranked even when row requirements have spare points', () => {
  applyTalentsObject({ destruction: { improved_shadow_bolt: 5, bane: 5, molten_skin: 5, ruin: 1 } });
  const before = getTalentsObject();
  assert.equal(can(2, 'Improved Shadow Bolt', -1), false);
  assert.equal(can(2, 'Bane', -1), true);
  assert.deepEqual(getTalentsObject(), before);
  applyTalentsObject({ destruction: { improved_shadow_bolt: 4, bane: 5, molten_skin: 5 } });
  assert.equal(can(2, 'Ruin', 1), false);
});

test('Learning requires earlier-row points and enforces the budget for partially ranked talents', () => {
  applyTalentsObject({ affliction: { suppression: 4, malediction: 5 } });
  assert.equal(can(0, 'Soul Harvesting', 1), false);
  applyTalentsObject({ affliction: { suppression: 5, improved_corruption: 5, malediction: 5, improved_drains: 3, fel_concentration: 3, malevolence: 5, nightfall: 2, siphon_life: 1, shadow_mastery: 5 }, demonology: { demonic_embrace: 5, unholy_power: 5, demonic_aegis: 2, fel_vitality: 3, demonic_energies: 1, improved_imp: 1 } });
  assert.equal(can(0, 'Soul Siphon', 1), false);
  assert.equal(can(1, 'Improved Imp', 1), false);
  applyTalentsObject({ affliction: { suppression: 5, malediction: 1 } });
  assert.equal(can(0, 'Malediction', 1), true);
});
