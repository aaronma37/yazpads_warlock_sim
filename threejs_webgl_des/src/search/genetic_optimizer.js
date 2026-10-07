import { createNativeEvaluator } from '../contracts/native_evaluation.js';

// Class/simulation references are bound once before a search; native loops are unchanged.
export function createBuildOptimizer({ classModule, simulation, dependencies = {} }) {
  const runMultiSimulation = simulation?.runMultiSimulation;
  const getTalentFlagsFromRanks = dependencies.resolveTalentFlags;
  const getPresetPetAndSac = dependencies.resolvePetAndSac;
  const createDefaultAPLIndividual = classModule.search.apl.createDefault;
  const getAvailableActionsForSpec = classModule.search.apl.availableActions;
  const individualToBytecodeRules = classModule.search.apl.encodeIndividual;
// Genetic Algorithm Constrained Spec Search Engine for Classic WoW Warlock DES
// Seamlessly operates in tandem with WebGL2 multi-config GPU shader simulation

// Resolve once at module startup; candidate loops retain direct class-owned references.
const activeClass = classModule;
const searchRules = activeClass.search;
const evaluateBatch = createNativeEvaluator({ classId: activeClass.id, runBatch: runMultiSimulation });
const { createCandidateResult } = searchRules.results;
const { formatBuildName, getMapElitesKey, getIndUniqueKey } = searchRules.identity;
const { enforceConstraints, createRandomIndividual, crossoverIndividuals, mutateIndividual } =
  searchRules.createBuildSearch(getTalentFlagsFromRanks);
const createPresetCandidate = searchRules.createPresetCandidate({
  resolveTalentFlags: getTalentFlagsFromRanks, resolvePetAndSac: getPresetPetAndSac, enforceConstraints,
});
const { buildToConfig: individualToConfig, policyForBuild: getPolicyAPLAndActions } =
  searchRules.createCandidateConfig(getTalentFlagsFromRanks);
const { constraints: PET_CONSTRAINTS, modes: PET_MODES, enforce: enforceTalentAndPetConstraints } = searchRules.pets;
const { graph: TalentGraph, definitions: TALENT_DEFINITIONS, nodeCount: TOTAL_TALENT_NODES,
  treeNodeCounts: [AFFLICTION_NODE_COUNT, DEMONOLOGY_NODE_COUNT, DESTRUCTION_NODE_COUNT] } = searchRules.talents;

const { races: RACES, rotations: ROTATION_CHOICES, rotationLabels: ROTATION_LABELS,
  enforceLocked: enforceLockedChoices } = searchRules.choices;

class FastRNG {
  constructor(seed = 0xDEADBEEF) {
    this.state = BigInt(seed);
  }
  nextU64() {
    this.state = (this.state * 6364136223846793005n + 1442695040888963407n) & 0xFFFFFFFFFFFFFFFFn;
    return Number(this.state >> 32n) >>> 0;
  }
  nextDouble() {
    return this.nextU64() / 4294967296;
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

function generateUniqueRandomIndividual(rng, config, seenConfigsSet, maxAttempts = 20) {
  for (let attempt = 0; attempt < maxAttempts; attempt++) {
    const ind = createRandomIndividual(rng, config);
    const key = getIndUniqueKey(ind);
    if (!seenConfigsSet.has(key)) {
      seenConfigsSet.add(key);
      return ind;
    }
  }
  const ind = createRandomIndividual(rng, config);
  const key = getIndUniqueKey(ind);
  seenConfigsSet.add(key);
  return ind;
}

function generateUniqueOffspring(parents, rng, config, seenConfigsSet, maxAttempts = 20) {
  for (let attempt = 0; attempt < maxAttempts; attempt++) {
    const p1 = parents[rng.nextU64() % parents.length];
    const p2 = parents[rng.nextU64() % parents.length];
    const child = crossoverIndividuals(p1, p2, rng, config);
    mutateIndividual(child, rng, config);
    const key = getIndUniqueKey(child);
    if (!seenConfigsSet.has(key)) {
      seenConfigsSet.add(key);
      return child;
    }
  }
  // If still colliding after max attempts, force multi-point mutations
  const p1 = parents[rng.nextU64() % parents.length];
  const p2 = parents[rng.nextU64() % parents.length];
  const child = crossoverIndividuals(p1, p2, rng, config);
  for (let m = 0; m < 3; m++) {
    mutateIndividual(child, rng, config);
  }
  const key = getIndUniqueKey(child);
  seenConfigsSet.add(key);
  return child;
}

function prepareOffspringBatch(parents, popSize, rng, gaConfig, baseStatsConfig, uniqueConfigsSet) {
  const numImmigrants = Math.max(2, Math.floor(popSize * 0.15));
  const numOffspring = Math.max(2, popSize - numImmigrants);
  const pool = [];

  for (let i = 0; i < numOffspring; i++) {
    pool.push(generateUniqueOffspring(parents, rng, gaConfig, uniqueConfigsSet));
  }

  for (let imm = 0; imm < numImmigrants; imm++) {
    pool.push(generateUniqueRandomIndividual(rng, gaConfig, uniqueConfigsSet));
  }

  const configs = pool.map(ind => individualToConfig(ind, baseStatsConfig));
  return { pool, configs };
}

// Main Constrained Genetic Algorithm Execution Engine running with WebGL2 Kernel & Double Buffering
async function runConstrainedGeneticSearch(baseStatsConfig, gaConfig, { signal, onProgress = () => {}, onGeneration = () => {} } = {}) {
  const rng = new FastRNG(gaConfig.seed || 0x13374242);
  const popSize = gaConfig.populationSize || 1000;
  const generations = gaConfig.generations || 15;
  const screeningSims = gaConfig.screeningSims || 100; // 100 fights per config
  const finalSims = gaConfig.finalSims || 2000;

  const mapElitesGrid = new Map();
  const evolutionHistory = [];
  const uniqueConfigsSet = new Set();
  let population = [];

  // Seed with standard presets if requested (strictly deduplicated)
  if (gaConfig.seedPresets && Array.isArray(gaConfig.presetsList) && gaConfig.presetsList.length > 0) {
    for (const p of gaConfig.presetsList) {
      const ind = createPresetCandidate(p, gaConfig, rng);
      const key = getIndUniqueKey(ind);
      if (!uniqueConfigsSet.has(key)) {
        uniqueConfigsSet.add(key);
        population.push(ind);
      }
      if (population.length >= Math.floor(popSize / 2)) break;
    }
  }

  // Fill remainder of population with unique random legal builds
  while (population.length < popSize) {
    population.push(generateUniqueRandomIndividual(rng, gaConfig, uniqueConfigsSet));
  }

  onProgress({ phase: 'Evaluating Initial Generation (GPU Shader)', completed: 0, total: generations + 1 });

  let totalEvalsCount = population.length;
  let totalSimsCount = population.length * screeningSims;

  // Launch Generation 0 simulation on GPU (Buffer A)
  const gen0Configs = population.map(ind => individualToConfig(ind, baseStatsConfig));
  let currentSimPromise = evaluateBatch(gen0Configs, { signal, iterations: screeningSims, detailedResults: false }, {
    requestId: 'build-generation-0', candidateIds: population.map((_, i) => `build-generation-0-${i}`),
  });
  let currentPool = population;

  // Double Buffering: Overlap CPU generation of Gen 1 while GPU simulates Gen 0!
  let nextBatch = null;
  if (generations >= 1) {
    nextBatch = prepareOffspringBatch(population, popSize, rng, gaConfig, baseStatsConfig, uniqueConfigsSet);
  }

  // Await GPU simulation of Gen 0
  const simRes0 = await currentSimPromise;
  if (signal?.aborted) {
    return { candidates: [], evolutionHistory: [], bestCandidate: null, totalEvaluations: 0, totalSimulations: 0, uniqueConfigsCount: 0 };
  }

  for (let i = 0; i < currentPool.length; i++) {
    const res = simRes0.results[i];
    currentPool[i].fitness = simRes0.fitnessResults[i].objective.value;
    currentPool[i].batch = res;
    const key = getMapElitesKey(currentPool[i]);
    if (!mapElitesGrid.has(key) || currentPool[i].fitness > mapElitesGrid.get(key).fitness) {
      mapElitesGrid.set(key, {
        ...currentPool[i],
        talents: new Uint8Array(currentPool[i].talents),
        apl: currentPool[i].apl ? { rules: currentPool[i].apl.rules.map(r => ({ ...r })) } : null
      });
    }
  }

  population.sort((a, b) => b.fitness - a.fitness);
  const gen0Best = population[0].fitness;
  const gen0Mean = population.reduce((s, ind) => s + ind.fitness, 0) / population.length;
  evolutionHistory.push({ gen: 0, bestDps: gen0Best, avgDps: gen0Mean });

  let elites = Array.from(mapElitesGrid.values()).sort((a, b) => b.fitness - a.fitness).slice(0, 15).map((ind, i) => createCandidateResult(ind, i + 1));
  onGeneration({
    gen: 0,
    maxGens: generations,
    bestDps: gen0Best,
    avgDps: gen0Mean,
    elites,
    progress: 0,
    status: `Gen 0/${generations} [Best: ${gen0Best.toFixed(1)} DPS]`,
    evolutionHistory,
    uniqueConfigsCount: uniqueConfigsSet.size,
    totalEvaluations: totalEvalsCount,
    totalSimulations: totalSimsCount
  });

  // Double-Buffered Generational Evolution Loop
  for (let gen = 1; gen <= generations; gen++) {
    if (signal?.aborted) break;

    // Buffer swap: nextBatch was prepared on CPU during previous GPU simulation
    const activeBatch = nextBatch;
    totalEvalsCount += activeBatch.pool.length;
    totalSimsCount += activeBatch.pool.length * screeningSims;

    // 1. Immediately launch GPU simulation on activeBatch
    currentSimPromise = evaluateBatch(activeBatch.configs, { signal, iterations: screeningSims, detailedResults: false }, {
      requestId: `build-generation-${gen}`, candidateIds: activeBatch.pool.map((_, i) => `build-generation-${gen}-${i}`),
    });

    // 2. Concurrently on CPU: prepare next generation's batch (Gen + 1) while GPU runs activeBatch
    if (gen < generations) {
      const occupiedElites = Array.from(mapElitesGrid.values());
      const parents = occupiedElites.length > 0 ? occupiedElites : population;
      nextBatch = prepareOffspringBatch(parents, popSize, rng, gaConfig, baseStatsConfig, uniqueConfigsSet);
    } else {
      nextBatch = null;
    }

    // 3. Await GPU simulation results for activeBatch
    const offSimRes = await currentSimPromise;
    if (signal?.aborted) break;

    for (let i = 0; i < activeBatch.pool.length; i++) {
      const res = offSimRes.results[i];
      activeBatch.pool[i].fitness = offSimRes.fitnessResults[i].objective.value;
      activeBatch.pool[i].batch = res;
      const key = getMapElitesKey(activeBatch.pool[i]);
      if (!mapElitesGrid.has(key) || activeBatch.pool[i].fitness > mapElitesGrid.get(key).fitness) {
        mapElitesGrid.set(key, {
          ...activeBatch.pool[i],
          talents: new Uint8Array(activeBatch.pool[i].talents),
          apl: activeBatch.pool[i].apl ? { rules: activeBatch.pool[i].apl.rules.map(r => ({ ...r })) } : null
        });
      }
    }

    // 4. Update population without duplicates
    const popMap = new Map();
    for (const ind of population.slice(0, Math.min(10, population.length))) {
      popMap.set(getIndUniqueKey(ind), ind);
    }
    for (const ind of activeBatch.pool) {
      const k = getIndUniqueKey(ind);
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

    elites = Array.from(mapElitesGrid.values()).sort((a, b) => b.fitness - a.fitness).slice(0, 15).map((ind, i) => createCandidateResult(ind, i + 1));
    const curProg = gen / generations;
    const status = `Gen ${gen}/${generations} [Best: ${bestDps.toFixed(1)} DPS]`;

    onProgress({ phase: `Evolution Gen ${gen}/${generations}`, completed: gen, total: generations + 1 });
    onGeneration({
      gen,
      maxGens: generations,
      bestDps,
      avgDps: meanDps,
      elites,
      progress: curProg,
      status,
      evolutionHistory,
      uniqueConfigsCount: uniqueConfigsSet.size,
      totalEvaluations: totalEvalsCount,
      totalSimulations: totalSimsCount
    });

    // Yield to browser UI
    await new Promise(resolve => setTimeout(resolve, 0));
  }

  // Final High-Precision Benchmarking of Diverse Spec Champions
  onProgress({ phase: 'Finalizing Diverse Spec Champions (High Precision GPU Sim)', completed: generations, total: generations + 1 });
  
  const finalCandidates = Array.from(mapElitesGrid.values())
    .sort((a, b) => b.fitness - a.fitness)
    .slice(0, 15);

  if (finalCandidates.length > 0) {
    const finalConfigs = finalCandidates.map(ind => individualToConfig(ind, baseStatsConfig));
    const finalSimRes = await evaluateBatch(finalConfigs, { signal, iterations: finalSims, detailedResults: false }, {
      requestId: 'build-finalists', candidateIds: finalCandidates.map((_, i) => `build-finalist-${i}`),
    });

    for (let i = 0; i < finalCandidates.length; i++) {
      finalCandidates[i].batch = finalSimRes.results[i];
      finalCandidates[i].fitness = finalSimRes.fitnessResults[i].objective.value;
    }
  }

  finalCandidates.sort((a, b) => b.fitness - a.fitness);
  const finalResults = finalCandidates.map((ind, i) => createCandidateResult(ind, i + 1));
  const totalSimulations = totalSimsCount + (finalCandidates.length * finalSims);

  return {
    candidates: finalResults,
    evolutionHistory,
    bestCandidate: finalResults[0] || null,
    totalEvaluations: totalEvalsCount,
    totalSimulations,
    uniqueConfigsCount: uniqueConfigsSet.size
  };
}

  return Object.freeze({ createCandidateResult, formatBuildName, getMapElitesKey, getIndUniqueKey, enforceConstraints, createRandomIndividual, crossoverIndividuals, mutateIndividual, individualToConfig, getPolicyAPLAndActions, PET_CONSTRAINTS, PET_MODES, TalentGraph, TALENT_DEFINITIONS, TOTAL_TALENT_NODES, AFFLICTION_NODE_COUNT, DEMONOLOGY_NODE_COUNT, DESTRUCTION_NODE_COUNT, RACES, ROTATION_CHOICES, ROTATION_LABELS, generateUniqueRandomIndividual, generateUniqueOffspring, runConstrainedGeneticSearch });
}
