import { getClassModule } from './classes/registry.js';
import { ACTIVE_CLASS } from './classes/active_class.js';
import { resolveClassRuntime } from './classes/runtime.js';
import { requireClassCapability } from './classes/capabilities.js';
import { createBuildOptimizer } from './search/genetic_optimizer.js';
import { getTalentFlagsFromRanks } from './talents.js';
import { getPresetPetAndSac } from './classes/warlock_preset_options.js';

// Existing helper exports remain bound to startup Warlock for compatibility.
const defaults = createBuildOptimizer({ classModule: ACTIVE_CLASS, dependencies: { resolveTalentFlags: getTalentFlagsFromRanks, resolvePetAndSac: getPresetPetAndSac } });
export const { createCandidateResult, formatBuildName, getMapElitesKey, getIndUniqueKey, enforceConstraints, createRandomIndividual, crossoverIndividuals, mutateIndividual, individualToConfig, getPolicyAPLAndActions, PET_CONSTRAINTS, PET_MODES, TalentGraph, TALENT_DEFINITIONS, TOTAL_TALENT_NODES, AFFLICTION_NODE_COUNT, DEMONOLOGY_NODE_COUNT, DESTRUCTION_NODE_COUNT, RACES, ROTATION_CHOICES, ROTATION_LABELS, generateUniqueRandomIndividual, generateUniqueOffspring } = defaults;

export async function runConstrainedGeneticSearch(baseStatsConfig, gaConfig, options = {}) {
  const classId = Object.hasOwn(options, 'classId') ? options.classId : ACTIVE_CLASS.id;
  if (typeof classId !== 'string') throw new Error('Search requires a supported classId.');
  const classModule = getClassModule(classId);
  requireClassCapability(classModule, 'buildSearch');
  const [runtime, dependencies] = await Promise.all([
    resolveClassRuntime(classId), classModule.loadSearchDependencies(),
  ]);
  const optimizer = createBuildOptimizer({ classModule, simulation: runtime.simulation, dependencies });
  return optimizer.runConstrainedGeneticSearch(baseStatsConfig, gaConfig, options);
}
