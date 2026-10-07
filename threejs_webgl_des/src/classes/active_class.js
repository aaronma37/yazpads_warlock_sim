import { getClassModule } from './registry.js';

// ES modules evaluate once: all presentation consumers share this selection.
// Runtime class switching will be introduced when a second class is supported.
export const ACTIVE_CLASS = getClassModule();
export const CLASS_PRESENTATION = ACTIVE_CLASS.presentation;
