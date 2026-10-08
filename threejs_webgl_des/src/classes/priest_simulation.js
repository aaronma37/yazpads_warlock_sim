import { buildFightConfig, resolveShadowTalentBuild } from '../priest/talents.js';
import * as model from '../priest/model.js';
import { VERTEX, FRAGMENT, FAST_FRAGMENT } from '../priest/kernel.js';
import { summarize } from '../priest/results.js';
import { createCompactDESEngine } from '../gpu/compact_des_engine.js';
const shaders = Object.freeze({ vertex: VERTEX, detailed: FRAGMENT, fast: FAST_FRAGMENT });
const engine = createCompactDESEngine({ model, shaders, summarize, id: 'priest', scope: model.SCOPE });
// Class-owned GPU runtime for Priest runs and constrained spec search.
// Global build imports/APL synthesis remain gated until their contracts exist.
export const PRIEST_SIMULATION = Object.freeze({ id: 'priest', ...engine, buildFightConfig, resolveTalentBuild: resolveShadowTalentBuild, validate: model.validate,
  packConfig: model.packConfig, packMultiConfig: model.packMultiConfig, decodeState: model.decodeState,
  defaults: model.DEFAULTS, spells: model.SPELLS, shaders, summarize,
  layout: Object.freeze({ configWords: model.CONFIG_WORDS, stateWords: model.STATE_WORDS, compactStripes: model.COMPACT_STRIPES }) });
