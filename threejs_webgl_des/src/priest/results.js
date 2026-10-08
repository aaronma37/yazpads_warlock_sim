import { RACIAL_STATE } from './racials.js';
import { SPELLS, STATUS } from './model.js';
export function summarize(states, duration) {
  if (!Array.isArray(states) || !states.length || !Number.isFinite(duration) || duration <= 0) throw new Error('Priest summary requires completed states.');
  let mean = 0, m2 = 0;
  states.forEach((state,index) => {
    if (state.done !== STATUS.complete || !Number.isFinite(state.total)) throw new Error('Incomplete Priest fight cannot contribute to fitness.');
    const delta = state.total/duration-mean; mean += delta/(index+1); m2 += delta*(state.total/duration-mean);
  });
  const sum = key => states.reduce((total, state) => total + state[key],0)/states.length;
  const detailed = states.every(state => state.detailed);
  return { mean, sd: states.length > 1 ? Math.sqrt(m2/(states.length-1)) : 0,
    samples: states.length, total: sum('total'), manaSpent: sum('manaSpent'), manaGained: sum('manaGained'),
    talents: detailed ? Object.fromEntries(['innerFocusUses','powerInfusionUses','freeNovaUses','plagueSpreads','spiritTapProcs','threat','targetManaBurned','flayClips','manaPoolWaits','manaPoolTimeMs'].map(key=>[key,sum(key)])) : {},
    racials: detailed ? Object.fromEntries(Object.keys(RACIAL_STATE).map(key=>[key,sum(key)])) : {},
    spells: detailed ? SPELLS.map((name,index) => ({ name, damage: sum(`damage${index}`), casts: sum(`casts${index}`), hits: sum(`hits${index}`), crits: sum(`crits${index}`), misses: sum(`misses${index}`), healing: [18,19,20].includes(index)?sum(`healing${index}`):0 })) : [] };
}
