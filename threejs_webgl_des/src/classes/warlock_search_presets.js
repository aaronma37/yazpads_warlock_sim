// Bind preset interpretation once; the optimizer retains ordering, deduplication, and limits.
export function createWarlockPresetCandidate({ talents, apl, resolveTalentFlags, resolvePetAndSac, enforceConstraints }) {
  const TalentGraph = talents.graph;
  const getTalentFlagsFromRanks = resolveTalentFlags;
  const getPresetPetAndSac = resolvePetAndSac;
  const getAvailableActionsForSpec = apl.availableActions;
  const createDefaultAPLIndividual = apl.createDefault;
  return function createPresetCandidate(p, config, rng) {
const { pet: presetPet, sac: presetSac } = getPresetPetAndSac(p);
      const nameLower = (p.name || '').toLowerCase();
      const rotLower = (p.rotation || '').toLowerCase();
      const isSearing = nameLower.includes('searing') || rotLower.includes('searing') || nameLower.includes('dp fire') || nameLower.includes('dp_fire');
      const isIncinerate = nameLower.includes('incin') || rotLower.includes('incin');
      const isBrand = nameLower.includes('brand') || rotLower.includes('brand');

      let initialRotation = 'SHADOW_DESTRO';
      if (isSearing) initialRotation = 'DP_AF_FIRE';
      else if (isIncinerate) initialRotation = 'FIRE_DESTRO';
      else if (isBrand) initialRotation = 'DP_AF_SHADOW';

      const talentsVector = TalentGraph.fromTalentsObject(p.talents);
      const tObj = TalentGraph.toTalentsObject(talentsVector);
      const tf = getTalentFlagsFromRanks(tObj);
      const availableActions = getAvailableActionsForSpec(tf);

      let presetApl = null;
      if (config.aplMode !== 'static') {
        presetApl = createDefaultAPLIndividual(availableActions, tf);
      }

      const ind = {
        talents: talentsVector,
        race: (config.forcedRace && config.forcedRace !== 'ALL') ? config.forcedRace : (p.race?.toUpperCase() || 'HUMAN'),
        rotation: initialRotation,
        pet: p.pet || presetPet || 'none',
        sacImp: p.sac === 'imp' || presetSac === 'imp',
        sacSuccubus: p.sac === 'succubus' || presetSac === 'succubus',
        apl: presetApl,
        fitness: 0,
        batch: null
      };
      enforceConstraints(ind, config, rng);
    return ind;
  };
}
