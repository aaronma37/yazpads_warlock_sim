import { WARLOCK_PRESENTATION } from './warlock_presentation.js';

// Presentation registration only; simulation implementations remain independent.
export const DEFAULT_CLASS_ID = 'warlock';

const classes = Object.freeze({
  warlock: Object.freeze({
    id: 'warlock',
    presentation: WARLOCK_PRESENTATION,
  }),
});

export function getClassModule(classId = DEFAULT_CLASS_ID) {
  if (typeof classId !== 'string' || !Object.hasOwn(classes, classId)) {
    throw new Error(`Unsupported character class: ${String(classId)}`);
  }
  return classes[classId];
}
