import { DEFAULT_CLASS_ID, getClassModule } from '../classes/registry.js';

export const SAVED_BUILD_SCHEMA_VERSION = 1;

function requireRecord(value, field) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) {
    throw new Error(`${field} must be an object.`);
  }
}

function requireText(value, field) {
  if (typeof value !== 'string' || !value.trim()) {
    throw new Error(`${field} must be a nonempty string.`);
  }
}

// Pure identity/version boundary for decoded JSON. No migration, class payload
// validation, simulation loading, or UI writes. The original payload is retained.
export function validateSavedBuildEnvelope(payload) {
  requireRecord(payload, 'Saved build');
  const versioned = Object.hasOwn(payload, 'schemaVersion');
  if (versioned && payload.schemaVersion !== SAVED_BUILD_SCHEMA_VERSION) {
    throw new Error(`Unsupported saved build schema version: ${String(payload.schemaVersion)}`);
  }
  const explicitClass = Object.hasOwn(payload, 'classId');
  if (versioned && !explicitClass) throw new Error('Versioned saved build requires classId.');
  const classId = explicitClass ? payload.classId : DEFAULT_CLASS_ID;
  // Explicit undefined must not use the registry's default parameter.
  requireText(classId, 'Saved build classId');
  getClassModule(classId);
  if (!versioned) return { legacy: true, schemaVersion: null, classId, kind: 'legacy', payload };

  if (payload.kind === 'logical-build') {
    requireRecord(payload.candidate, 'Saved build candidate');
    const candidate = payload.candidate;
    if (candidate.schemaVersion !== SAVED_BUILD_SCHEMA_VERSION) {
      throw new Error(`Unsupported candidate schema version: ${String(candidate.schemaVersion)}`);
    }
    requireText(candidate.classId, 'Candidate classId');
    if (candidate.classId !== classId) throw new Error('Saved build and candidate classId must match.');
    requireText(candidate.candidateId, 'Candidate candidateId');
  } else if (payload.kind === 'resolved-config') {
    requireRecord(payload.resolvedConfig, 'Saved build resolvedConfig');
    requireText(payload.packingVersion, 'Saved build packingVersion');
    requireText(payload.simulationVersion, 'Saved build simulationVersion');
  } else {
    throw new Error(`Unsupported saved build kind: ${String(payload.kind)}`);
  }
  if (Object.hasOwn(payload, 'evaluationDefaults')) {
    requireRecord(payload.evaluationDefaults, 'Saved build evaluationDefaults');
  }
  return { legacy: false, schemaVersion: payload.schemaVersion, classId, kind: payload.kind, payload };
}
