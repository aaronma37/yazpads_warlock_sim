// Bind class dependencies once; candidate operations retain the original RNG order.
export function createWarlockBuildSearch({ talents, pets, choices, apl, resolveTalentFlags }) {
  const { graph: TalentGraph, nodeCount: TOTAL_TALENT_NODES } = talents;
  const { modes: PET_MODES, enforce: enforceTalentAndPetConstraints } = pets;
  const { races: RACES, rotations: ROTATION_CHOICES, enforceLocked: enforceLockedChoices } = choices;
  const { availableActions: getAvailableActionsForSpec, createRandom: createRandomAPLIndividual,
    repair: repairAPLIndividual, crossover: crossoverAPLIndividuals, mutate: mutateAPLIndividual } = apl;
  const getTalentFlagsFromRanks = resolveTalentFlags;

// Enforce Constraints matching C++ oracle
function enforceConstraints(ind, config, rng) {
  enforceTalentAndPetConstraints(ind, config, rng);

  enforceLockedChoices(ind, config);

  // Enforce APL validity against active talents
  if (ind.apl && Array.isArray(ind.apl.rules)) {
    const tObj = TalentGraph.toTalentsObject(ind.talents);
    const tf = getTalentFlagsFromRanks(tObj);
    const availableActions = getAvailableActionsForSpec(tf);
    repairAPLIndividual(ind.apl, rng, availableActions, tf, config.lockConditions ?? true);
  }
}

function createRandomIndividual(rng, config) {
  const pm = PET_MODES[rng.nextU64() % PET_MODES.length];
  const talents = TalentGraph.generateRandomValid(rng);
  const tObj = TalentGraph.toTalentsObject(talents);
  const tf = getTalentFlagsFromRanks(tObj);
  const availableActions = getAvailableActionsForSpec(tf);

  const ind = {
    talents,
    race: config.forcedRace && config.forcedRace !== 'ALL' ? config.forcedRace : RACES[rng.nextU64() % RACES.length],
    rotation: config.forcedRotation && config.forcedRotation !== 'ALL' ? config.forcedRotation : ROTATION_CHOICES[rng.nextU64() % ROTATION_CHOICES.length],
    pet: pm.pet,
    sacImp: pm.imp,
    sacSuccubus: pm.succ,
    apl: config.aplMode !== 'static' ? createRandomAPLIndividual(rng, availableActions, tf, config.lockConditions ?? true) : null,
    fitness: 0,
    batch: null
  };

  enforceConstraints(ind, config, rng);
  return ind;
}

function crossoverIndividuals(p1, p2, rng, config) {
  const useP1Pet = rng.nextU64() % 2 === 0;
  const childTalents = new Uint8Array(TOTAL_TALENT_NODES);
  for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
    childTalents[i] = rng.nextU64() % 2 === 0 ? p1.talents[i] : p2.talents[i];
  }
  TalentGraph.repair(childTalents, rng);

  const tObj = TalentGraph.toTalentsObject(childTalents);
  const tf = getTalentFlagsFromRanks(tObj);
  const availableActions = getAvailableActionsForSpec(tf);

  let childApl = null;
  if (config.aplMode !== 'static') {
    if (p1.apl && p2.apl) {
      childApl = crossoverAPLIndividuals(p1.apl, p2.apl, rng, availableActions, tf, config.lockConditions ?? true);
    } else if (p1.apl || p2.apl) {
      const src = p1.apl || p2.apl;
      childApl = { rules: src.rules.map(r => ({ ...r })), fitness: 0, batch: null };
      repairAPLIndividual(childApl, rng, availableActions, tf, config.lockConditions ?? true);
    } else {
      childApl = createRandomAPLIndividual(rng, availableActions, tf, config.lockConditions ?? true);
    }
  }

  const child = {
    talents: childTalents,
    race: rng.nextU64() % 2 === 0 ? p1.race : p2.race,
    rotation: rng.nextU64() % 2 === 0 ? p1.rotation : p2.rotation,
    pet: useP1Pet ? p1.pet : p2.pet,
    sacImp: useP1Pet ? p1.sacImp : p2.sacImp,
    sacSuccubus: useP1Pet ? p1.sacSuccubus : p2.sacSuccubus,
    apl: childApl,
    fitness: 0,
    batch: null
  };

  enforceConstraints(child, config, rng);
  return child;
}

function mutateIndividual(ind, rng, config) {
  const mutRate = config.mutationRate ?? 0.45;
  const reqs = config.requiredTalents || [];

  if (rng.nextDouble() < mutRate) {
    const swaps = 1 + (rng.nextU64() % 3);
    for (let s = 0; s < swaps; s++) {
      const donors = TalentGraph.getValidDonors(ind.talents).filter(d => !reqs.includes(d));
      const receivers = TalentGraph.getValidReceivers(ind.talents);
      if (donors.length > 0 && receivers.length > 0) {
        const d = donors[rng.nextU64() % donors.length];
        ind.talents[d]--;
        const r = receivers[rng.nextU64() % receivers.length];
        ind.talents[r]++;
      }
    }
  }

  if (config.optimizeRace && (!config.forcedRace || config.forcedRace === 'ALL') && rng.nextDouble() < 0.20) {
    ind.race = RACES[rng.nextU64() % RACES.length];
  }

  if ((!config.forcedPetMode || config.forcedPetMode === 'ALL') && rng.nextDouble() < mutRate) {
    const pm = PET_MODES[rng.nextU64() % PET_MODES.length];
    ind.pet = pm.pet;
    ind.sacImp = pm.imp;
    ind.sacSuccubus = pm.succ;
  }

  if ((!config.forcedRotation || config.forcedRotation === 'ALL') && rng.nextDouble() < mutRate) {
    ind.rotation = ROTATION_CHOICES[rng.nextU64() % ROTATION_CHOICES.length];
  }

  enforceConstraints(ind, config, rng);

  // APL Mutation & Repair
  if (config.aplMode !== 'static') {
    const tObj = TalentGraph.toTalentsObject(ind.talents);
    const tf = getTalentFlagsFromRanks(tObj);
    const availableActions = getAvailableActionsForSpec(tf);

    if (!ind.apl) {
      ind.apl = createRandomAPLIndividual(rng, availableActions, tf, config.lockConditions ?? true);
    } else {
      repairAPLIndividual(ind.apl, rng, availableActions, tf, config.lockConditions ?? true);
      mutateAPLIndividual(ind.apl, rng, config, tf);
      repairAPLIndividual(ind.apl, rng, availableActions, tf, config.lockConditions ?? true);
    }
  }
}

  return Object.freeze({ enforceConstraints, createRandomIndividual, crossoverIndividuals, mutateIndividual });
}
