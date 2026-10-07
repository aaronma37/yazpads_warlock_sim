import { WARLOCK_VERSIONS } from './warlock_versions.js';
import { WARLOCK_SEARCH } from './warlock_search.js';
import { WARLOCK_PRESENTATION } from './warlock_presentation.js';

// Simulation loading is deferred so presentation consumers stay lightweight.
export const DEFAULT_CLASS_ID = 'warlock';

const classes = Object.freeze({
  warlock: Object.freeze({
    id: 'warlock',
    capabilities: Object.freeze({
      schemaVersion: 1,
      features: Object.freeze({ simulation: true, batchSimulation: true,
        buildSearch: true, aplSearch: true, importBuild: true, regretScan: true }),
      modes: Object.freeze(['fast', 'detailed']),
      objectives: Object.freeze(['mean-dps']),
      seedSchedules: Object.freeze(['existing-warlock-v1']),
    }),
    presentation: WARLOCK_PRESENTATION,
    versions: WARLOCK_VERSIONS,
    search: WARLOCK_SEARCH,
    loadSearchDependencies: async () => (await import('./warlock_search_dependencies.js')).WARLOCK_SEARCH_DEPENDENCIES,
    loadImports: async () => (await import('./warlock_import.js')).normalizeWarlockImport,
    loadImportResolver: async () => (await import('./warlock_import.js')).resolveWarlockImport,
    loadSimulation: async () => (await import('./warlock_simulation.js')).WARLOCK_SIMULATION,
  }),
});

export function getClassModule(classId = DEFAULT_CLASS_ID) {
  if (typeof classId !== 'string' || !Object.hasOwn(classes, classId)) {
    throw new Error(`Unsupported character class: ${String(classId)}`);
  }
  return classes[classId];
}
