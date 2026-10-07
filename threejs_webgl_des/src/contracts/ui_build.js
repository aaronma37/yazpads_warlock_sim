import { resolveSavedBuild } from './build_io.js';
import { parseAPLText } from '../apl.js';

// Stage all validation and APL parsing before the caller invokes any UI setter.
export async function prepareUIBuildImport(payload, options) {
  const resolved = await resolveSavedBuild(payload, options);
  if (resolved.kind === 'resolved-config' || resolved.candidate.apl.kind === 'policy') {
    return { kind: 'resolved-config', config: resolved.config };
  }
  const c = resolved.candidate;
  const rules = c.apl.kind === 'text' ? parseAPLText(c.apl.text) : c.apl.rules;
  return { kind: 'editable', config: resolved.config, rules, logicalBuild: {
    schemaVersion: 1, classId: resolved.classId, kind: 'logical-build',
    candidate: c, evaluationDefaults: resolved.evaluationDefaults,
  }, payload: {
    race: c.classOptions.race, pet: c.classOptions.pet, ds: c.classOptions.sacrifice,
    rotation: c.classOptions.rotation, gearMode: c.equipment.mode, gear: c.equipment.items,
    stats: c.stats, talents: c.talents, buffs: c.buffs, target: c.encounter.target,
    sim: { duration: c.encounter.duration, distance: c.encounter.distance,
      iterations: resolved.evaluationDefaults.iterations, detailedResults: resolved.evaluationDefaults.mode === 'detailed' },
  } };
}
