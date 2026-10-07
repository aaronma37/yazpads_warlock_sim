import { createNativeEvaluator } from '../contracts/native_evaluation.js';

// Class/simulation references are bound once before a search; native loops are unchanged.
export function createAPLOptimizer({ classModule, simulation, dependencies = {} }) {
  const runMultiSimulation = simulation?.runMultiSimulation;
// Genetic Algorithm APL Synthesis Engine for Classic WoW Warlock GPU Simulator
// Synthesizes optimal Action Priority Lists (APL) using continuous parameter evolution,
// discrete condition mutation, and index swap operations with exactly 2 copies per action.

const activeClass = classModule;
const searchRules = activeClass.search;
const evaluateBatch = createNativeEvaluator({ classId: activeClass.id, runBatch: runMultiSimulation });
const { formatAPLName, createCandidateAPLResult } = searchRules.results;
const { getAPLUniqueKey, getAPLMapElitesKey } = searchRules.identity;
const { aplToConfig: individualToConfig } = searchRules.createCandidateConfig();
const { graph: TalentGraph } = searchRules.talents;

// Resolve class-owned rules once; retain direct references and public exports.
const { actions: APL_SYNTHESIS_ACTIONS, totalRules: TOTAL_APL_RULES, spellIds: SPELL_IDS,
  conditionTypes: CONDITION_TYPES, validConditions: ACTION_VALID_CONDITIONS,
  availableActions: getAvailableActionsForSpec, conditionsForAction: getValidConditionsForAction,
  createCondition: createRuleCondition, createTwoConditionRule, createRule,
  handcraftedRule: getHandcraftedRuleForAction, encodeIndividual: individualToBytecodeRules,
  randomCondition: getRandomConditionParamsForAction, randomRule: getRandomTwoConditionRule,
  createDefault: createDefaultIndividual, createRandom: createRandomAPLIndividual,
  repair: repairAPLIndividual, crossover: crossoverAPLIndividuals, mutate: mutateAPLIndividual } = searchRules.apl;


class FastRNG {
  constructor(seed = 0x1337BEEF) {
    this.s0 = (seed & 0xFFFFFFFF) >>> 0;
    this.s1 = ((seed ^ 0x9E3779B9) & 0xFFFFFFFF) >>> 0;
    this.s2 = ((seed * 1664525 + 1013904223) & 0xFFFFFFFF) >>> 0;
    this.s3 = ((seed * 1103515245 + 12345) & 0xFFFFFFFF) >>> 0;
    for (let i = 0; i < 16; i++) this.nextU32();
  }

  nextU32() {
    const t = (this.s1 << 9) >>> 0;
    this.s2 ^= this.s0;
    this.s3 ^= this.s1;
    this.s1 ^= this.s2;
    this.s0 ^= this.s3;
    this.s2 ^= t;
    this.s3 = ((this.s3 << 11) | (this.s3 >>> 21)) >>> 0;
    return (this.s0 + this.s3) >>> 0;
  }

  nextDouble() {
    return (this.nextU32() >>> 8) * (1.0 / 16777216.0);
  }

  nextInt(min, max) {
    return min + Math.floor(this.nextDouble() * (max - min + 1));
  }

  nextFloat(min, max) {
    return min + this.nextDouble() * (max - min);
  }

  nextGaussian(mean = 0, stdev = 1) {
    const u1 = Math.max(1e-9, this.nextDouble());
    const u2 = this.nextDouble();
    const z = Math.sqrt(-2.0 * Math.log(u1)) * Math.cos(2.0 * Math.PI * u2);
    return mean + z * stdev;
  }
}

// Extract top unique distinct APL candidates
function extractTopUniqueCandidates(population, mapElitesGrid, maxCount = 15) {
  const mapList = Array.from(mapElitesGrid.values());
  const combined = [...population, ...mapList];
  combined.sort((a, b) => b.fitness - a.fitness);

  const seen = new Set();
  const unique = [];
  for (const ind of combined) {
    // Unique signature by full action priority sequence
    const sig = ind.rules.map(r => `${r.id}:${r.enabled ? 1 : 0}`).join('-');
    if (!seen.has(sig)) {
      seen.add(sig);
      unique.push(ind);
      if (unique.length >= maxCount) break;
    }
  }

  return unique.map((ind, i) => createCandidateAPLResult(ind, i + 1));
}

// Main Genetic Algorithm APL Synthesis Loop with WebGL Multi-Config Simulation & Double Buffering
async function runAPLGeneticSynthesis(baseStatsConfig, gaConfig, { signal, onProgress = () => {}, onGeneration = () => {} } = {}) {
  const rng = new FastRNG(gaConfig.seed || 0x41504C53);
  const popSize = gaConfig.populationSize || 500;
  const generations = gaConfig.generations || 15;
  const screeningSims = gaConfig.screeningSims || 100;
  const finalSims = gaConfig.finalSims || 2000;

  const talentFlags = baseStatsConfig?.talentFlags || {};
  const selectedActions = Array.isArray(gaConfig.selectedActionIds) ? new Set(gaConfig.selectedActionIds) : null;
  const availableActions = getAvailableActionsForSpec(talentFlags)
    .filter(action => !selectedActions || selectedActions.has(action.id));
  if (!availableActions.length) throw new Error('Select at least one spell available to the current build.');

  const mapElitesGrid = new Map();
  const evolutionHistory = [];
  const uniqueAPLsSet = new Set();
  let population = [];

  const lockConditions = gaConfig.lockConditions ?? true;

  // 1. Seed with Default Canonical & Preset-derived APLs
  if (gaConfig.seedPresets) {
    const def = createDefaultIndividual(availableActions, talentFlags);
    uniqueAPLsSet.add(getAPLUniqueKey(def));
    population.push(def);
  }

  // 2. Fill remainder with randomized unique legal APL individuals
  // Small selected pools can have fewer unique APLs than the requested population.
  let seedAttempts = 0;
  while (population.length < popSize && seedAttempts++ < popSize * 20) {
    const ind = createRandomAPLIndividual(rng, availableActions, talentFlags, lockConditions);
    const key = getAPLUniqueKey(ind);
    if (!uniqueAPLsSet.has(key)) {
      uniqueAPLsSet.add(key);
      population.push(ind);
    }
  }

  onProgress({ phase: 'Evaluating Initial APL Generation (GPU Shader)', completed: 0, total: generations + 1 });

  let totalEvalsCount = population.length;
  let totalSimsCount = population.length * screeningSims;

  // Launch Generation 0 simulation on GPU
  const gen0Configs = population.map(ind => individualToConfig(ind, baseStatsConfig));
  let currentSimPromise = evaluateBatch(gen0Configs, { signal, iterations: screeningSims, detailedResults: false }, {
    requestId: 'apl-generation-0', candidateIds: population.map((_, i) => `apl-generation-0-${i}`),
  });
  let currentPool = population;

  // Prepare Generation 1 offspring on CPU concurrently
  let nextBatch = null;
  if (generations >= 1) {
    nextBatch = prepareAPLOffspringBatch(population, popSize, rng, gaConfig, baseStatsConfig, uniqueAPLsSet, availableActions, talentFlags, mapElitesGrid);
  }

  // Await Gen 0 GPU results
  const simRes0 = await currentSimPromise;
  if (signal?.aborted) {
    return { candidates: [], evolutionHistory: [], bestCandidate: null, totalEvaluations: 0, totalSimulations: 0, uniqueConfigsCount: 0 };
  }

  for (let i = 0; i < currentPool.length; i++) {
    const res = simRes0.results[i];
    currentPool[i].fitness = simRes0.fitnessResults[i].objective.value;
    currentPool[i].batch = res;
    const key = getAPLMapElitesKey(currentPool[i]);
    if (!mapElitesGrid.has(key) || currentPool[i].fitness > mapElitesGrid.get(key).fitness) {
      mapElitesGrid.set(key, { ...currentPool[i] });
    }
  }

  population.sort((a, b) => b.fitness - a.fitness);
  const gen0Best = population[0].fitness;
  const gen0Mean = population.reduce((s, ind) => s + ind.fitness, 0) / population.length;
  evolutionHistory.push({ gen: 0, bestDps: gen0Best, avgDps: gen0Mean });

  let elites = extractTopUniqueCandidates(population, mapElitesGrid, 15);
  onGeneration({
    gen: 0,
    maxGens: generations,
    bestDps: gen0Best,
    avgDps: gen0Mean,
    elites,
    progress: 0,
    status: `Gen 0/${generations} [Best: ${gen0Best.toFixed(1)} DPS]`,
    evolutionHistory,
    uniqueConfigsCount: uniqueAPLsSet.size,
    totalEvaluations: totalEvalsCount,
    totalSimulations: totalSimsCount
  });

  // Generational Evolution Loop with Double Buffering
  for (let gen = 1; gen <= generations; gen++) {
    if (signal?.aborted) break;

    const activeBatch = nextBatch;
    totalEvalsCount += activeBatch.pool.length;
    totalSimsCount += activeBatch.pool.length * screeningSims;

    // Launch GPU simulation for activeBatch
    currentSimPromise = evaluateBatch(activeBatch.configs, { signal, iterations: screeningSims, detailedResults: false }, {
      requestId: `apl-generation-${gen}`, candidateIds: activeBatch.pool.map((_, i) => `apl-generation-${gen}-${i}`),
    });

    // Concurrently prepare next generation batch on CPU
    if (gen < generations) {
      nextBatch = prepareAPLOffspringBatch(population, popSize, rng, gaConfig, baseStatsConfig, uniqueAPLsSet, availableActions, talentFlags, mapElitesGrid);
    } else {
      nextBatch = null;
    }

    const offSimRes = await currentSimPromise;
    if (signal?.aborted) break;

    for (let i = 0; i < activeBatch.pool.length; i++) {
      const res = offSimRes.results[i];
      activeBatch.pool[i].fitness = offSimRes.fitnessResults[i].objective.value;
      activeBatch.pool[i].batch = res;
      const key = getAPLMapElitesKey(activeBatch.pool[i]);
      if (!mapElitesGrid.has(key) || activeBatch.pool[i].fitness > mapElitesGrid.get(key).fitness) {
        mapElitesGrid.set(key, { ...activeBatch.pool[i] });
      }
    }

    // Population update & elitism
    const popMap = new Map();
    for (const ind of population.slice(0, Math.min(20, population.length))) {
      popMap.set(getAPLUniqueKey(ind), ind);
    }
    for (const ind of activeBatch.pool) {
      const k = getAPLUniqueKey(ind);
      if (!popMap.has(k) || ind.fitness > popMap.get(k).fitness) {
        popMap.set(k, ind);
      }
    }
    population = Array.from(popMap.values())
      .sort((a, b) => b.fitness - a.fitness)
      .slice(0, popSize);

    const bestDps = population[0]?.fitness || 0;
    const meanDps = population.reduce((s, ind) => s + ind.fitness, 0) / population.length;
    evolutionHistory.push({ gen, bestDps, avgDps: meanDps });

    elites = extractTopUniqueCandidates(population, mapElitesGrid, 15);
    const curProg = gen / generations;
    const status = `Gen ${gen}/${generations} [Best: ${bestDps.toFixed(1)} DPS]`;

    onProgress({ phase: `APL Evolution Gen ${gen}/${generations}`, completed: gen, total: generations + 1 });
    onGeneration({
      gen,
      maxGens: generations,
      bestDps,
      avgDps: meanDps,
      elites,
      progress: curProg,
      status,
      evolutionHistory,
      uniqueConfigsCount: uniqueAPLsSet.size,
      totalEvaluations: totalEvalsCount,
      totalSimulations: totalSimsCount
    });

    await new Promise(resolve => setTimeout(resolve, 0));
  }

  // Final High-Precision Benchmarking of Discovered Elite APL Champions
  onProgress({ phase: 'Finalizing Elite APL Champions (High Precision GPU Sim)', completed: generations, total: generations + 1 });

  const finalCandidates = extractTopUniqueCandidates(population, mapElitesGrid, 15);

  if (finalCandidates.length > 0) {
    const finalConfigs = finalCandidates.map(c => individualToConfig(c, baseStatsConfig));
    const finalSimRes = await evaluateBatch(finalConfigs, { signal, iterations: finalSims, detailedResults: false }, {
      requestId: 'apl-finalists', candidateIds: finalCandidates.map((_, i) => `apl-finalist-${i}`),
    });

    for (let i = 0; i < finalCandidates.length; i++) {
      finalCandidates[i].batch = finalSimRes.results[i];
      finalCandidates[i].meanDps = finalSimRes.fitnessResults[i].objective.value;
      finalCandidates[i].summary = finalSimRes.results[i].summary;
    }
  }

  finalCandidates.sort((a, b) => b.meanDps - a.meanDps);
  finalCandidates.forEach((c, i) => { c.rank = i + 1; });
  const totalSimulations = totalSimsCount + (finalCandidates.length * finalSims);

  return {
    candidates: finalCandidates,
    evolutionHistory,
    bestCandidate: finalCandidates[0] || null,
    totalEvaluations: totalEvalsCount,
    totalSimulations,
    uniqueConfigsCount: uniqueAPLsSet.size,
    availableActionsCount: availableActions.length
  };
}

function prepareAPLOffspringBatch(population, popSize, rng, gaConfig, baseStatsConfig, uniqueAPLsSet, availableActions = APL_SYNTHESIS_ACTIONS, talentFlags = {}, mapElitesGrid = null) {
  const numElites = Math.min(20, Math.max(5, Math.floor(popSize * 0.05)));
  const numImmigrants = Math.max(5, Math.floor(popSize * 0.10));
  const numOffspring = Math.max(2, popSize - numElites - numImmigrants);
  const lockConditions = gaConfig.lockConditions ?? true;
  const pool = [];

  // 1. Elitism: preserve top individuals directly
  for (let i = 0; i < numElites; i++) {
    if (i < population.length) {
      pool.push({
        rules: population[i].rules.map(r => ({ ...r })),
        fitness: population[i].fitness,
        batch: population[i].batch
      });
    }
  }

  // Create parent pool from top 40% of population + all map elites
  const topPop = population.slice(0, Math.max(10, Math.floor(population.length * 0.40)));
  const mapElites = mapElitesGrid ? Array.from(mapElitesGrid.values()) : [];
  const parentPool = [...topPop, ...mapElites];

  const selectParent = () => {
    let best = parentPool[rng.nextInt(0, parentPool.length - 1)];
    for (let t = 0; t < 3; t++) {
      const candidate = parentPool[rng.nextInt(0, parentPool.length - 1)];
      if (candidate && candidate.fitness > best.fitness) {
        best = candidate;
      }
    }
    return best;
  };

  // 2. Tournament Crossover & Mutation Offspring
  for (let i = 0; i < numOffspring; i++) {
    const p1 = selectParent();
    const p2 = selectParent();
    const child = crossoverAPLIndividuals(p1, p2, rng, availableActions, talentFlags, lockConditions);
    mutateAPLIndividual(child, rng, gaConfig, talentFlags);
    const key = getAPLUniqueKey(child);
    if (!uniqueAPLsSet.has(key)) {
      uniqueAPLsSet.add(key);
      pool.push(child);
    } else {
      // Re-mutate on collision
      mutateAPLIndividual(child, rng, gaConfig, talentFlags);
      pool.push(child);
    }
  }

  // 3. Immigrants for diversity
  for (let imm = 0; imm < numImmigrants; imm++) {
    const randomInd = createRandomAPLIndividual(rng, availableActions, talentFlags, lockConditions);
    const key = getAPLUniqueKey(randomInd);
    uniqueAPLsSet.add(key);
    pool.push(randomInd);
  }

  const configs = pool.map(ind => individualToConfig(ind, baseStatsConfig));
  return { pool, configs };
}

  return Object.freeze({ formatAPLName, createCandidateAPLResult, getAPLUniqueKey, getAPLMapElitesKey, individualToConfig, APL_SYNTHESIS_ACTIONS, TOTAL_APL_RULES, SPELL_IDS, CONDITION_TYPES, ACTION_VALID_CONDITIONS, getAvailableActionsForSpec, getValidConditionsForAction, createRuleCondition, createTwoConditionRule, createRule, getHandcraftedRuleForAction, individualToBytecodeRules, createDefaultIndividual, createRandomAPLIndividual, repairAPLIndividual, crossoverAPLIndividuals, mutateAPLIndividual, extractTopUniqueCandidates, runAPLGeneticSynthesis });
}
