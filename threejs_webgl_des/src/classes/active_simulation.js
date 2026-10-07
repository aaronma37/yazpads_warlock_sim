import { resolveClassRuntime } from './runtime.js';
import { ACTIVE_CLASS } from './active_class.js';

// Resolve once before application runs; callers retain direct function references.
// Kept separate from presentation so talent-only consumers do not load Three.js.
export const CLASS_SIMULATION = (await resolveClassRuntime(ACTIVE_CLASS.id)).simulation;
