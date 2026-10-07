import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { WARLOCK_VERSIONS, WARLOCK_VERSION_SOURCE_HASHES } from '../src/classes/warlock_versions.js';
import { resolveSavedBuild, exportResolvedBuild } from '../src/contracts/build_io.js';
import { packConfig } from '../src/model.js';

test('registered compatibility IDs match current packing/mechanics sources', () => {
 for(const [path,hash] of Object.entries(WARLOCK_VERSION_SOURCE_HASHES)) {
  assert.equal(createHash('sha256').update(readFileSync(new URL(`../${path}`,import.meta.url))).digest('hex'),hash,
   `${path} changed; review compatibility and regenerate version IDs.`);
 }
 assert.equal(WARLOCK_VERSIONS.packingVersion,`warlock-packing-sha256-${WARLOCK_VERSION_SOURCE_HASHES['src/model.js']}`);
 const combined=Object.values(WARLOCK_VERSION_SOURCE_HASHES).join('');
 assert.equal(WARLOCK_VERSIONS.simulationVersion,`warlock-simulation-sha256-${createHash('sha256').update(combined).digest('hex')}`);
});
test('versioned resolved exports reimport without reapplying buffs or racial stats',async()=>{
 const input={race:'GNOME',stats:{intellect:200},buffs:{arcaneIntellect:true},aplText:'Shadow Bolt'};
 const original=await resolveSavedBuild(input);
 const envelope=await exportResolvedBuild(input);
 const imported=await resolveSavedBuild(JSON.parse(JSON.stringify(envelope)));
 assert.deepEqual(imported.config,original.config);
 assert.deepEqual(packConfig(imported.config),packConfig(original.config));
 assert.deepEqual(await exportResolvedBuild(envelope),envelope);
 assert.equal(imported.config.intellect,Math.round(231*1.05));
});
test('each incompatible version and class identity is rejected before resolution',async()=>{
 const envelope=await exportResolvedBuild({stats:{}});
 for(const edit of [{packingVersion:'other'},{simulationVersion:'other'},{classId:'priest'},
  {schemaVersion:2}]) await assert.rejects(resolveSavedBuild({...envelope,...edit}));
});
