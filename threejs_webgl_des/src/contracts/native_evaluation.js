import { requireClassCapability } from '../classes/capabilities.js';
import { getClassModule } from '../classes/registry.js';
import { mapEvaluationResults } from './evaluation.js';

// Transitional adapter for already-resolved configs. Bind once per search/module;
// do not migrate optimizer candidates or copy/reapply config effects.
export function createNativeEvaluator({ classId, classModule = getClassModule(classId), runBatch }) {
  if (classModule.id !== classId) throw new Error('Native evaluator class identity mismatch.');
  requireClassCapability(classModule, 'batchSimulation');
  return async function evaluateNative(configs, options = {}, { requestId, candidateIds } = {}) {
    if (typeof requestId !== 'string' || !requestId.trim()) throw new Error('Native evaluation requires requestId.');
    if (!Array.isArray(configs) || !configs.length || !Array.isArray(candidateIds) || candidateIds.length !== configs.length) throw new Error('Native candidate/config count mismatch.');
    const ids = candidateIds.slice();
    if (ids.some(id => typeof id !== 'string' || !id.trim()) || new Set(ids).size !== ids.length) throw new Error('Native candidate IDs must be unique nonempty strings.');
    const iterations = options.iterations ?? configs[0].iterations;
    if (!Number.isInteger(iterations) || iterations < 1 || iterations > 1048576) throw new Error('Invalid native iterations.');
    if (options.iterations == null && configs.some(c => c.iterations !== iterations)) throw new Error('Native configs require equal iteration counts.');
    const request = { requestId, candidates: ids.map(candidateId => ({ classId, candidateId })),
      settings: { iterations, mode: options.detailedResults === false ? 'fast' : 'detailed', objective: 'mean-dps' } };
    // Exact native input/options references and seeds pass through to the runner.
    const batch = await runBatch(configs, options);
    const fitnessResults = mapEvaluationResults(request, batch);
    if (fitnessResults.some(r => r.status !== 'complete')) throw new Error('Incomplete native evaluation results rejected before fitness selection.');
    return { ...batch, fitnessResults };
  };
}
