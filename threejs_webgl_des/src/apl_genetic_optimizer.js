import { getClassModule } from './classes/registry.js';
import { ACTIVE_CLASS } from './classes/active_class.js';
import { resolveClassRuntime } from './classes/runtime.js';
import { requireClassCapability } from './classes/capabilities.js';
import { createAPLOptimizer } from './search/apl_genetic_optimizer.js';

// Existing helper exports remain bound to startup Warlock for compatibility.
const defaults = createAPLOptimizer({ classModule: ACTIVE_CLASS });
export const { formatAPLName, createCandidateAPLResult, getAPLUniqueKey, getAPLMapElitesKey, individualToConfig, APL_SYNTHESIS_ACTIONS, TOTAL_APL_RULES, SPELL_IDS, CONDITION_TYPES, ACTION_VALID_CONDITIONS, getAvailableActionsForSpec, getValidConditionsForAction, createRuleCondition, createTwoConditionRule, createRule, getHandcraftedRuleForAction, individualToBytecodeRules, createDefaultIndividual, createRandomAPLIndividual, repairAPLIndividual, crossoverAPLIndividuals, mutateAPLIndividual, extractTopUniqueCandidates } = defaults;

export async function runAPLGeneticSynthesis(baseStatsConfig, gaConfig, options = {}) {
  const classId = Object.hasOwn(options, 'classId') ? options.classId : ACTIVE_CLASS.id;
  if (typeof classId !== 'string') throw new Error('Search requires a supported classId.');
  const classModule = getClassModule(classId);
  requireClassCapability(classModule, 'aplSearch');
  const [runtime, dependencies] = await Promise.all([
    resolveClassRuntime(classId), classModule.loadSearchDependencies(),
  ]);
  const optimizer = createAPLOptimizer({ classModule, simulation: runtime.simulation, dependencies });
  return optimizer.runAPLGeneticSynthesis(baseStatsConfig, gaConfig, options);
}
