import { runSimulation, runMultiSimulation, summarize } from '../src/engine.js';
import { SPELLS } from '../src/model.js';

function assert(condition, message) {
 if (!condition) throw new Error(message);
}

function sameFastResults(detailed, fast) {
 assert(fast.summary.detailed === false && fast.summary.spells.length === 0, 'Fast mode exposes unavailable spell accounting');
 for (let i = 0; i < fast.states.length; i++) {
  for (const [key, value] of Object.entries(fast.states[i])) {
   if (key !== 'detailed') assert(value === detailed.states[i][key], `Fast fight ${i}: ${key} differs`);
  }
 }
 for (const key of ['mean', 'sd', 'ci95', 'p05', 'p50', 'p95', 'events', 'taps', 'petDamage', 'maxHeap']) {
  assert(fast.summary[key] === detailed.summary[key], `Fast summary ${key} differs`);
 }
}

export async function checkAccounting() {
 const checks = [];
 // Independent reductions cover every spell, including fields in the final stripe.
 const states = Array.from({length: 31}, (_, lane) => {
  const state = {total: (31 - lane) * 123.25};
  for (let spell = 0; spell < SPELLS.length; spell++) {
   for (const key of ['damage', 'casts', 'hits', 'crits', 'misses']) state[`${key}${spell}`] = (lane + 1) * (spell + 1);
  }
  return state;
 });
 const summary = summarize(states, 180);
 for (let spell = 0; spell < SPELLS.length; spell++) {
  for (const key of ['damage', 'casts', 'hits', 'crits', 'misses']) {
   const expected = states.reduce((sum, state) => sum + state[`${key}${spell}`], 0) / states.length;
   assert(summary.spells[spell][key] === expected, `Summary ${SPELLS[spell]} ${key} differs`);
  }
 }
 assert(JSON.stringify(summary.dps) === JSON.stringify(states.map(s => s.total / 180).sort((a,b) => a-b)), 'Sorted DPS differs');
 checks.push({name: 'all spell summaries against independent reductions', pass: true});

 const inputs = [
  {race: 'UNDEAD', drainHope: true, siphonLife: true, curseOfDoom: true, agony: false},
  {race: 'GNOME', rotation: 'fire', immolate: true, conflagrate: true, shadowburn: true, petChoice: 'imp'},
  {race: 'TROLL', rotation: 'searing', decimation: true, decimationRank: 2, petChoice: 'succubus', demonicBrand: true, demonicBrandRank: 3}
 ].map(c => ({...c, duration: 180, iterations: 1031, seed: 42}));
 // 1027 crosses a texture row and leaves a partial final batch.
 for (const input of inputs) {
  const detailed = await runSimulation(input, {batchSize: 1027});
  const fast = await runSimulation(input, {batchSize: 1027, detailedResults: false});
  sameFastResults(detailed, fast);
  assert(JSON.stringify(detailed.trace) === JSON.stringify(fast.trace), 'Fast trace differs');
  const unbatched = await runSimulation(input, {trace: false});
  assert(JSON.stringify(detailed.states) === JSON.stringify(unbatched.states), 'Detailed batch boundary differs');
  const fastUnbatched = await runSimulation(input, {trace: false, detailedResults: false});
  assert(JSON.stringify(fast.states) === JSON.stringify(fastUnbatched.states), 'Fast batch boundary differs');
  checks.push({name: `${input.race}: fast/detailed results, trace and row/batch boundaries`, pass: true});
 }
 const detailed = await runMultiSimulation(inputs, {batchSize: 1027});
 const fast = await runMultiSimulation(inputs, {batchSize: 1027, detailedResults: false});
 for (let i = 0; i < inputs.length; i++) sameFastResults(detailed.results[i], fast.results[i]);
 checks.push({name: 'multi-config fast/detailed accounting', pass: true});
 try {
  await runSimulation({iterations: 1}, {eventBudget: 1, detailedResults: false});
  throw new Error('Fast mode accepted an incomplete fight');
 } catch (error) {
  assert(error.message.includes('Incomplete results rejected'), error.message);
 }
 checks.push({name: 'fast mode rejects incomplete fights', pass: true});
 return checks;
}
