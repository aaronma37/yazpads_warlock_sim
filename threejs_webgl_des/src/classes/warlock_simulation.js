import { buildFightConfig } from '../config_builder.js';
import { validate, packConfig, packMultiConfig, decodeStates, DEFAULTS, SPELLS,
  CONFIG_WORDS, STATE_WORDS, COMPACT_STRIPES } from '../model.js';
import { VERTEX, FRAGMENT, FAST_FRAGMENT } from '../kernel.js';
import { runSimulation, runMultiSimulation, summarize, preloadShader,
  disposeEngine, analyzeRegret, scanRegrets } from '../engine.js';

// Direct references only: no adapters, extra validation, or per-candidate dispatch.
// The existing engine still owns compact/fast readback and GPU resource lifetime.
export const WARLOCK_SIMULATION = Object.freeze({
  id: 'warlock',
  buildFightConfig,
  validate,
  packConfig,
  packMultiConfig,
  decodeStates,
  defaults: DEFAULTS,
  spells: SPELLS,
  shaders: Object.freeze({ vertex: VERTEX, detailed: FRAGMENT, fast: FAST_FRAGMENT }),
  layout: Object.freeze({ configWords: CONFIG_WORDS, stateWords: STATE_WORDS,
    compactStripes: COMPACT_STRIPES }),
  runSimulation,
  runMultiSimulation,
  summarize,
  preloadShader,
  disposeEngine,
  analyzeRegret,
  scanRegrets,
});
