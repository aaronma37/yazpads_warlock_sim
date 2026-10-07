import { validateSavedBuildEnvelope } from './saved_build.js';

function record(value, field) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error(`${field} must be an object.`);
}

// Match the existing importer's resolved-input precedence; unknown objects fail
// instead of silently becoming an empty build. UI defaults are applied later.
export function classifyLegacyBuild(payload) {
  const identity = validateSavedBuildEnvelope(payload);
  if (!identity.legacy) throw new Error('Expected an unversioned legacy build.');
  if (payload.resolvedConfig || payload.config) {
    const field = payload.resolvedConfig ? 'resolvedConfig' : 'config';
    record(payload[field], field);
    return { ...identity, format: field, resolvedConfig: payload[field] };
  }
  if (payload.duration && payload.iterations && payload.spellPower !== undefined && !payload.talents && !payload.stats) {
    return { ...identity, format: 'raw-resolved', resolvedConfig: payload };
  }
  const uiFields = ['race', 'pet', 'ds', 'rotation', 'gearMode', 'stats', 'gear', 'talents', 'buffs', 'target', 'sim', 'aplText'];
  if (!uiFields.some(key => Object.hasOwn(payload, key))) throw new Error('Unrecognized legacy build format.');
  for (const field of ['stats', 'gear', 'talents', 'buffs', 'target', 'sim']) {
    if (Object.hasOwn(payload, field)) record(payload[field], field);
  }
  for (const field of ['race', 'pet', 'ds', 'rotation', 'gearMode', 'aplText']) {
    if (Object.hasOwn(payload, field) && typeof payload[field] !== 'string') throw new Error(`${field} must be a string.`);
  }
  if (Object.hasOwn(payload, 'version') && payload.version !== 1) throw new Error(`Unsupported legacy UI build version: ${String(payload.version)}`);
  return { ...identity, format: 'ui-build' };
}

// Produce an immutable staged mapping, not a validated/normalized candidate or
// versioned resolved envelope. Class defaults, APL compilation, combat validation,
// and resolved simulation-version compatibility are later integration checks.
export function migrateLegacyBuild(payload, { candidateId = 'imported-build' } = {}) {
  const classified = classifyLegacyBuild(payload);
  if (typeof candidateId !== 'string' || !candidateId.trim()) throw new Error('candidateId must be a nonempty string.');
  const source = structuredClone(payload);
  if (classified.format !== 'ui-build') {
    return { classId: classified.classId, kind: 'resolved-config', sourceFormat: classified.format,
      resolvedConfig: structuredClone(classified.resolvedConfig) };
  }
  const target = source.target || {};
  const sim = source.sim || {};
  const encounter = { target };
  if (Object.hasOwn(sim, 'duration')) encounter.duration = sim.duration;
  if (Object.hasOwn(sim, 'distance')) encounter.distance = sim.distance;
  const evaluationDefaults = {};
  if (Object.hasOwn(sim, 'iterations')) evaluationDefaults.iterations = sim.iterations;
  if (Object.hasOwn(sim, 'detailedResults')) {
    if (typeof sim.detailedResults !== 'boolean') throw new Error('sim.detailedResults must be a boolean.');
    evaluationDefaults.mode = sim.detailedResults ? 'detailed' : 'fast';
  }
  const equipment = { items: source.gear || {} };
  if (Object.hasOwn(source, 'gearMode')) equipment.mode = source.gearMode;
  const classOptions = {};
  for (const [oldKey, newKey] of [['race', 'race'], ['pet', 'pet'], ['ds', 'sacrifice'], ['rotation', 'rotation']]) {
    if (Object.hasOwn(source, oldKey)) classOptions[newKey] = source[oldKey];
  }
  // Legacy UI rotation values are simulator modes, not named GA policies.
  // Keep them in classOptions pending class resolution; never invent policy IDs.
  return { classId: classified.classId, kind: 'logical-build', sourceFormat: 'ui-build',
    candidate: { schemaVersion: 1, classId: classified.classId, candidateId,
      encounter, stats: source.stats || {}, equipment, buffs: source.buffs || {},
      talents: source.talents || {}, apl: Object.hasOwn(source, 'aplText') ? { kind: 'text', text: source.aplText } : null,
      classOptions }, evaluationDefaults };
}
