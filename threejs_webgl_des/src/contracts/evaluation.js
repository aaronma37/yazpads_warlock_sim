import { resolveClassRuntime } from '../classes/runtime.js';
import { requireClassEvaluation } from '../classes/capabilities.js';
import { getClassModule } from '../classes/registry.js';

function text(value, field) {
  if (typeof value !== 'string' || !value.trim()) throw new Error(`${field} must be a nonempty string.`);
}
function record(value, field) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error(`${field} must be an object.`);
}
export function validateEvaluationRequest(request) {
  record(request, 'Evaluation request');
  if (request.schemaVersion !== 1) throw new Error('Unsupported evaluation schema version.');
  text(request.requestId, 'requestId');
  if (!Array.isArray(request.candidates) || !request.candidates.length) throw new Error('Candidates must be a nonempty array.');
  const ids = new Set();
  const classId = request.candidates[0]?.classId;
  getClassModule(classId === undefined ? null : classId);
  for (const candidate of request.candidates) {
    record(candidate, 'Candidate');
    if (candidate.schemaVersion !== 1 || candidate.classId !== classId) throw new Error('Evaluation requires one supported class and candidate schema version 1.');
    text(candidate.candidateId, 'candidateId');
    if (ids.has(candidate.candidateId)) throw new Error('Duplicate candidateId.');
    ids.add(candidate.candidateId);
    for (const field of ['encounter', 'stats', 'equipment', 'buffs', 'talents', 'apl', 'classOptions']) record(candidate[field], field);
  }
  const settings = request.settings;
  record(settings, 'Settings');
  for (const key of Object.keys(settings)) if (!['iterations','seedSchedule','mode','objective','batchSize'].includes(key)) throw new Error(`Unsupported evaluation setting: ${key}`);
  if (!Number.isInteger(settings.iterations) || settings.iterations < 1 || settings.iterations > 1048576) throw new Error('Invalid evaluation iterations.');
  record(settings.seedSchedule, 'Seed schedule');
  if (Object.keys(settings.seedSchedule).some(k=>!['kind','seed'].includes(k)) || settings.seedSchedule.kind !== 'existing-warlock-v1') throw new Error('Unsupported seed schedule.');
  const seed = settings.seedSchedule.seed;
  if (!Number.isInteger(seed) || seed < 0 || seed > 4294967295) throw new Error('Invalid seed.');
  if (!['fast','detailed'].includes(settings.mode) || settings.objective !== 'mean-dps') throw new Error('Unsupported evaluation mode/objective.');
  if (settings.batchSize !== undefined && (!Number.isInteger(settings.batchSize) || settings.batchSize < 1)) throw new Error('Invalid batchSize.');
  requireClassEvaluation(getClassModule(classId), settings);
  return request;
}

export function mapEvaluationResults(request, batch) {
  if (!batch || !Array.isArray(batch.results) || batch.results.length !== request.candidates.length) throw new Error('Evaluation result count mismatch.');
  return batch.results.map((result, index) => {
    const candidate = request.candidates[index];
    const states = result?.states;
    const completed = Array.isArray(states) ? states.filter(s=>s?.done === 1 && Number.isFinite(s.total)).length : 0;
    const mean = result?.summary?.mean;
    const complete = completed === request.settings.iterations && states.length === completed && Number.isFinite(mean);
    const sd = result?.summary?.sd;
    return { schemaVersion: 1, requestId: request.requestId, candidateId: candidate.candidateId,
      classId: candidate.classId, status: complete ? 'complete' : 'incomplete', sampleCount: completed,
      objective: { id: request.settings.objective, value: complete ? mean : null },
      uncertainty: complete && completed > 1 && Number.isFinite(sd) && sd >= 0 ?
        { kind: 'standard-error', value: sd / Math.sqrt(completed) } : null,
      detail: complete && request.settings.mode === 'detailed' ? result : null,
      error: complete ? null : { code: 'INCOMPLETE_RESULT', message: 'Expected completed finite samples and a finite fitness value.' } };
  });
}

// Dependency binding is outside candidate evaluation; native GPU batching is unchanged.
export function createEvaluator({ resolveCandidate, runBatch }) {
  return async function evaluate(request, { signal, onProgress, itemDatabase } = {}) {
    validateEvaluationRequest(request);
    const snapshot = structuredClone(request);
    const configs = [];
    for (const candidate of snapshot.candidates) {
      const resolved = await resolveCandidate({ classId: candidate.classId, kind: 'logical-build', candidate }, { itemDatabase });
      configs.push({ ...resolved.config, iterations: snapshot.settings.iterations, seed: snapshot.settings.seedSchedule.seed });
    }
    // Preserve existing runner cancellation/error rejection; no fabricated partial scores.
    const batch = await runBatch(configs, { signal, onProgress, iterations: snapshot.settings.iterations,
      batchSize: snapshot.settings.batchSize, detailedResults: snapshot.settings.mode === 'detailed' });
    return { schemaVersion: 1, requestId: snapshot.requestId, results: mapEvaluationResults(snapshot, batch),
      timing: batch.timing, adapter: batch.adapter, engine: batch.engine };
  };
}

export async function evaluateCandidates(request, options) {
  validateEvaluationRequest(request);
  const snapshot = structuredClone(request);
  const classModule = getClassModule(snapshot.candidates[0].classId);
  const [resolveCandidate, runtime] = await Promise.all([classModule.loadImportResolver(), resolveClassRuntime(classModule.id)]);
  return createEvaluator({ resolveCandidate, runBatch: runtime.simulation.runMultiSimulation })(snapshot, options);
}
