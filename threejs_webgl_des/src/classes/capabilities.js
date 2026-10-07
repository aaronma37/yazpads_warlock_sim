// Capabilities describe supported interfaces, not CPU/mechanics parity.
export function requireClassCapability(classModule, feature) {
  if (classModule.capabilities?.schemaVersion !== 1) throw new Error('Unsupported class capability schema.');
  if (classModule.capabilities.features?.[feature] !== true) {
    throw new Error(`Class ${classModule.id} does not support ${feature}.`);
  }
}

export function requireClassEvaluation(classModule, settings) {
  requireClassCapability(classModule, 'batchSimulation');
  const caps = classModule.capabilities;
  if (!caps.modes.includes(settings.mode)) throw new Error(`Class ${classModule.id} does not support mode ${settings.mode}.`);
  if (!caps.objectives.includes(settings.objective)) throw new Error(`Class ${classModule.id} does not support objective ${settings.objective}.`);
  if (!caps.seedSchedules.includes(settings.seedSchedule.kind)) throw new Error(`Class ${classModule.id} does not support seed schedule ${settings.seedSchedule.kind}.`);
}
