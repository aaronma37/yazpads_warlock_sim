import { getClassModule } from './registry.js';
import { requireClassCapability } from './capabilities.js';

// Resolve by class identity outside hot loops; loading does not switch UI state.
export function createClassRuntimeResolver({ getClass = getClassModule } = {}) {
  const simulations = new Map();
  return async function resolveRuntime(classId) {
    const classModule = getClass(classId);
    requireClassCapability(classModule, 'simulation');
    let pending = simulations.get(classModule.id);
    if (!pending) {
      pending = Promise.resolve().then(() => classModule.loadSimulation()).then(simulation => {
        if (!simulation || simulation.id !== classModule.id) throw new Error('Simulation class identity mismatch.');
        for (const name of ['runSimulation', 'runMultiSimulation', 'validate', 'packConfig']) {
          if (typeof simulation[name] !== 'function') throw new Error(`Class simulation requires ${name}.`);
        }
        for (const mode of classModule.capabilities.modes) {
          if (typeof simulation.shaders?.[mode] !== 'string' || !simulation.shaders[mode]) throw new Error(`Class simulation requires ${mode} shader.`);
        }
        return Object.freeze({ classModule, simulation });
      });
      simulations.set(classModule.id, pending);
      // Failed loads may retry; successful contracts keep stable direct references.
      pending.catch(() => { if (simulations.get(classModule.id) === pending) simulations.delete(classModule.id); });
    }
    return pending;
  };
}
export const resolveClassRuntime = createClassRuntimeResolver();
