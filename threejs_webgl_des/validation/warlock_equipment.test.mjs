import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolveWarlockEquipment } from '../src/classes/warlock_equipment.js';
import { normalizeWarlockImport } from '../src/classes/warlock_import.js';
import { migrateLegacyBuild } from '../src/contracts/legacy_build.js';
const database = JSON.parse(readFileSync(new URL('../data/items.json', import.meta.url)));
test('equipped imports derive stats independently of saved display snapshots', () => {
 const gear = database.presets.p6.items;
 const staged = migrateLegacyBuild({ gearMode: 'equipped', gear, stats: { intellect: 999 } });
 const result = normalizeWarlockImport(staged, { itemDatabase: database });
 assert.deepEqual(result.candidate.stats, resolveWarlockEquipment(gear, database));
 assert.notEqual(result.candidate.stats.intellect, 999);
 assert.deepEqual(normalizeWarlockImport(result, { itemDatabase: database }), result);
 assert.equal(staged.candidate.stats.intellect, 999);
});
test('equipment rejects unknown IDs, slots, wrong-slot items, and missing data', () => {
 for (const items of [{ HEAD: 999999 }, { WRONG: 101 }, { FEET: 101 }, { HEAD: '101' }]) assert.throws(()=>resolveWarlockEquipment(items,database));
 assert.throws(()=>resolveWarlockEquipment({},null));
 const ring=database.items.find(x=>x.slot==='RING1');
 assert.doesNotThrow(()=>resolveWarlockEquipment({RING2:ring.id},database));
});
test('saved buff names use combat-form fields and explicit false values remain valid', () => {
 assert.doesNotThrow(()=>normalizeWarlockImport(migrateLegacyBuild({buffs:{blessingOfKings:false}})));
 assert.throws(()=>normalizeWarlockImport(migrateLegacyBuild({buffs:{blessing_of_kings:true}})));
 assert.throws(()=>normalizeWarlockImport(migrateLegacyBuild({buffs:{blessingOfKings:1}})));
});
