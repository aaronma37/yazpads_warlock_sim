import { requireClassCapability } from '../classes/capabilities.js';
import { getClassModule } from '../classes/registry.js';
import { validateSavedBuildEnvelope } from './saved_build.js';
import { migrateLegacyBuild } from './legacy_build.js';

// Decoded JSON boundary, kept independent of browser transport and UI state.
export async function resolveSavedBuild(payload, options) {
  const identity = validateSavedBuildEnvelope(payload);
  const classModule = getClassModule(identity.classId);
  requireClassCapability(classModule, 'importBuild');
  if (!identity.legacy && identity.kind === 'resolved-config') {
    const versions = classModule.versions;
    if (!versions || payload.packingVersion !== versions.packingVersion || payload.simulationVersion !== versions.simulationVersion) {
      throw new Error('Unsupported resolved-config packing/simulation compatibility versions.');
    }
  }
  const staged = identity.legacy ? migrateLegacyBuild(payload) : structuredClone(payload);
  const resolve = await classModule.loadImportResolver();
  return resolve(staged, options);
}

export async function exportLogicalBuild(payload, options) {
  const resolved = await resolveSavedBuild(payload, options);
  if (resolved.kind !== 'logical-build') throw new Error('Resolved configs cannot be reconstructed as logical builds.');
  return { schemaVersion: 1, classId: resolved.classId, kind: 'logical-build',
    candidate: structuredClone(resolved.candidate), evaluationDefaults: structuredClone(resolved.evaluationDefaults) };
}

// Export the fully resolved config with explicit class-owned compatibility IDs.
export async function exportResolvedBuild(payload, options) {
  const resolved = await resolveSavedBuild(payload, options);
  const versions = getClassModule(resolved.classId).versions;
  if (!versions) throw new Error('Class has no registered resolved-config compatibility versions.');
  return { schemaVersion: 1, classId: resolved.classId, kind: 'resolved-config',
    resolvedConfig: structuredClone(resolved.config), ...versions };
}
