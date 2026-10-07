import { createWarlockSearchResults } from './warlock_search_results.js';
import { createWarlockPresetCandidate } from './warlock_search_presets.js';
import { createWarlockSearchIdentity } from './warlock_search_identity.js';
import { createWarlockCandidateConfig } from './warlock_candidate_config.js';
import { createWarlockBuildSearch } from './warlock_build_search.js';
import { WARLOCK_APL_SEARCH } from './warlock_apl_search.js';
import { buildPolicyAPL } from './warlock_policy_apl.js';

// Warlock-owned search rules. Methods and RNG consumption are unchanged.
// Mutation orchestration and synthesized APL encoding remain in the optimizers.
export const TOTAL_TALENT_NODES = 52;
export const AFFLICTION_NODE_COUNT = 17;
export const DEMONOLOGY_NODE_COUNT = 19;
export const DESTRUCTION_NODE_COUNT = 16;

export const TALENT_DEFINITIONS = [
  // Affliction (0..16)
  { id: 'improved_life_tap', name: 'Improved Life Tap', tree: 0, treeName: 'Affliction', row: 1, col: 1, max: 2, req: null },
  { id: 'suppression', name: 'Suppression', tree: 0, treeName: 'Affliction', row: 1, col: 2, max: 5, req: null },
  { id: 'improved_corruption', name: 'Improved Corruption', tree: 0, treeName: 'Affliction', row: 1, col: 3, max: 5, req: null },
  { id: 'malediction', name: 'Malediction', tree: 0, treeName: 'Affliction', row: 2, col: 1, max: 5, req: null },
  { id: 'soul_harvesting', name: 'Soul Harvesting', tree: 0, treeName: 'Affliction', row: 2, col: 2, max: 2, req: null },
  { id: 'improved_drains', name: 'Improved Drains', tree: 0, treeName: 'Affliction', row: 2, col: 3, max: 3, req: null },
  { id: 'improved_bane_of_agony', name: 'Improved Bane of Agony', tree: 0, treeName: 'Affliction', row: 3, col: 1, max: 2, req: null },
  { id: 'fel_concentration', name: 'Fel Concentration', tree: 0, treeName: 'Affliction', row: 3, col: 2, max: 3, req: null },
  { id: 'amplify_curse', name: 'Amplify Curse', tree: 0, treeName: 'Affliction', row: 3, col: 3, max: 1, req: null },
  { id: 'pandemic', name: 'Pandemic', tree: 0, treeName: 'Affliction', row: 3, col: 4, max: 3, req: null },
  { id: 'malevolence', name: 'Malevolence', tree: 0, treeName: 'Affliction', row: 4, col: 1, max: 5, req: null },
  { id: 'nightfall', name: 'Nightfall', tree: 0, treeName: 'Affliction', row: 4, col: 2, max: 2, req: null },
  { id: 'curse_of_exhaustion', name: 'Curse of Exhaustion', tree: 0, treeName: 'Affliction', row: 4, col: 3, max: 1, req: 8 }, // req Amplify Curse
  { id: 'siphon_life', name: 'Siphon Life', tree: 0, treeName: 'Affliction', row: 5, col: 2, max: 1, req: null },
  { id: 'soul_siphon', name: 'Soul Siphon', tree: 0, treeName: 'Affliction', row: 5, col: 3, max: 3, req: null },
  { id: 'shadow_mastery', name: 'Shadow Mastery', tree: 0, treeName: 'Affliction', row: 6, col: 3, max: 5, req: null },
  { id: 'wrack', name: 'Wrack', tree: 0, treeName: 'Affliction', row: 7, col: 2, max: 1, req: 13 }, // req Siphon Life

  // Demonology (17..35)
  { id: 'improved_health_funnel', name: 'Improved Health Funnel', tree: 1, treeName: 'Demonology', row: 1, col: 1, max: 2, req: null },
  { id: 'improved_imp', name: 'Improved Imp', tree: 1, treeName: 'Demonology', row: 1, col: 2, max: 3, req: null },
  { id: 'demonic_embrace', name: 'Demonic Embrace', tree: 1, treeName: 'Demonology', row: 1, col: 3, max: 5, req: null },
  { id: 'unholy_power', name: 'Unholy Power', tree: 1, treeName: 'Demonology', row: 1, col: 4, max: 5, req: null },
  { id: 'demonic_aegis', name: 'Demonic Aegis', tree: 1, treeName: 'Demonology', row: 2, col: 1, max: 2, req: null },
  { id: 'improved_voidwalker', name: 'Improved Voidwalker', tree: 1, treeName: 'Demonology', row: 2, col: 2, max: 3, req: null },
  { id: 'fel_vitality', name: 'Fel Vitality', tree: 1, treeName: 'Demonology', row: 2, col: 3, max: 3, req: null },
  { id: 'demonic_energies', name: 'Demonic Energies', tree: 1, treeName: 'Demonology', row: 2, col: 4, max: 2, req: null },
  { id: 'improved_sayaad', name: 'Improved Sayaad', tree: 1, treeName: 'Demonology', row: 3, col: 1, max: 3, req: null },
  { id: 'demonic_sacrifice', name: 'Demonic Sacrifice', tree: 1, treeName: 'Demonology', row: 3, col: 2, max: 1, req: null },
  { id: 'master_summoner', name: 'Master Summoner', tree: 1, treeName: 'Demonology', row: 3, col: 3, max: 2, req: null },
  { id: 'decimation', name: 'Decimation', tree: 1, treeName: 'Demonology', row: 4, col: 1, max: 2, req: null },
  { id: 'fel_domination', name: 'Fel Domination', tree: 1, treeName: 'Demonology', row: 4, col: 3, max: 1, req: 27 }, // req Master Summoner
  { id: 'demonic_brand', name: 'Demonic Brand', tree: 1, treeName: 'Demonology', row: 4, col: 4, max: 3, req: null },
  { id: 'improved_felhunter', name: 'Improved Felhunter', tree: 1, treeName: 'Demonology', row: 5, col: 1, max: 3, req: null },
  { id: 'soul_link', name: 'Soul Link', tree: 1, treeName: 'Demonology', row: 5, col: 2, max: 1, req: 26 }, // req Demonic Sacrifice
  { id: 'demonic_knowledge', name: 'Demonic Knowledge', tree: 1, treeName: 'Demonology', row: 5, col: 3, max: 3, req: null },
  { id: 'master_demonologist', name: 'Master Demonologist', tree: 1, treeName: 'Demonology', row: 6, col: 3, max: 5, req: null },
  { id: 'demonic_pact', name: 'Demonic Pact', tree: 1, treeName: 'Demonology', row: 7, col: 2, max: 1, req: 32 }, // req Soul Link

  // Destruction (36..51)
  { id: 'destructive_reach', name: 'Destructive Reach', tree: 2, treeName: 'Destruction', row: 1, col: 1, max: 2, req: null },
  { id: 'improved_shadow_bolt', name: 'Improved Shadow Bolt', tree: 2, treeName: 'Destruction', row: 1, col: 2, max: 5, req: null },
  { id: 'bane', name: 'Bane', tree: 2, treeName: 'Destruction', row: 1, col: 3, max: 5, req: null },
  { id: 'molten_skin', name: 'Molten Skin', tree: 2, treeName: 'Destruction', row: 2, col: 1, max: 5, req: null },
  { id: 'cataclysm', name: 'Cataclysm', tree: 2, treeName: 'Destruction', row: 2, col: 2, max: 3, req: null },
  { id: 'aftermath', name: 'Aftermath', tree: 2, treeName: 'Destruction', row: 2, col: 3, max: 5, req: null },
  { id: 'ruin', name: 'Ruin', tree: 2, treeName: 'Destruction', row: 3, col: 2, max: 5, req: 37 }, // req Improved Shadow Bolt
  { id: 'shadowburn', name: 'Shadowburn', tree: 2, treeName: 'Destruction', row: 3, col: 3, max: 1, req: null },
  { id: 'intensity', name: 'Intensity', tree: 2, treeName: 'Destruction', row: 4, col: 1, max: 3, req: null },
  { id: 'agonizing_flames', name: 'Agonizing Flames', tree: 2, treeName: 'Destruction', row: 4, col: 2, max: 3, req: null },
  { id: 'conflagrate', name: 'Conflagrate', tree: 2, treeName: 'Destruction', row: 4, col: 3, max: 1, req: null },
  { id: 'pyroclasm', name: 'Pyroclasm', tree: 2, treeName: 'Destruction', row: 5, col: 1, max: 2, req: 44 }, // req Intensity
  { id: 'bane_of_havoc', name: 'Bane of Havoc', tree: 2, treeName: 'Destruction', row: 5, col: 2, max: 1, req: null },
  { id: 'fire_and_brimstone', name: 'Fire and Brimstone', tree: 2, treeName: 'Destruction', row: 5, col: 3, max: 3, req: 46 }, // req Conflagrate
  { id: 'shadow_and_flame', name: 'Shadow and Flame', tree: 2, treeName: 'Destruction', row: 6, col: 3, max: 5, req: null },
  { id: 'incinerate', name: 'Incinerate', tree: 2, treeName: 'Destruction', row: 7, col: 2, max: 1, req: 48 } // req Bane of Havoc
];

export class TalentGraph {
  static countTreePoints(v, treeIdx) {
    let sum = 0;
    for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
      if (TALENT_DEFINITIONS[i].tree === treeIdx) sum += v[i];
    }
    return sum;
  }

  static countTotalPoints(v) {
    let sum = 0;
    for (let i = 0; i < TOTAL_TALENT_NODES; i++) sum += v[i];
    return sum;
  }

  static isValid(v, targetTotal = 51) {
    if (this.countTotalPoints(v) !== targetTotal) return false;
    for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
      const node = TALENT_DEFINITIONS[i];
      if (v[i] < 0 || v[i] > node.max) return false;
      if (v[i] > 0 && node.req !== null) {
        if (v[node.req] < TALENT_DEFINITIONS[node.req].max) return false;
      }
    }
    for (let tree = 0; tree < 3; tree++) {
      const pointsBelowRow = [0, 0, 0, 0, 0, 0, 0, 0];
      let running = 0;
      for (let r = 1; r <= 7; r++) {
        pointsBelowRow[r] = running;
        for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
          if (TALENT_DEFINITIONS[i].tree === tree && TALENT_DEFINITIONS[i].row === r) {
            running += v[i];
          }
        }
      }
      for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
        if (TALENT_DEFINITIONS[i].tree === tree && v[i] > 0) {
          const req = 5 * (TALENT_DEFINITIONS[i].row - 1);
          if (pointsBelowRow[TALENT_DEFINITIONS[i].row] < req) return false;
        }
      }
    }
    return true;
  }

  static getTreeRowPoints(v) {
    // 3 trees x 8 rows (rows 1..7)
    const treeRow = new Int32Array(24);
    for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
      const pts = v[i];
      if (pts > 0) {
        const d = TALENT_DEFINITIONS[i];
        treeRow[d.tree * 8 + d.row] += pts;
      }
    }
    return treeRow;
  }

  static getValidDonors(v) {
    const donors = [];
    const treeRow = this.getTreeRowPoints(v);

    for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
      if (v[i] <= 0) continue;
      const def = TALENT_DEFINITIONS[i];

      // Check dependent prerequisites
      let hasDependent = false;
      for (let j = 0; j < TOTAL_TALENT_NODES; j++) {
        if (v[j] > 0 && TALENT_DEFINITIONS[j].req === i) {
          hasDependent = true;
          break;
        }
      }
      if (hasDependent) continue;

      const tree = def.tree;
      const tOff = tree * 8;
      let valid = true;
      let running = 0;

      for (let r = 1; r <= 7; r++) {
        const ptsInRow = treeRow[tOff + r] - (def.row === r ? 1 : 0);
        if (ptsInRow > 0) {
          const req = 5 * (r - 1);
          if (running < req) {
            valid = false;
            break;
          }
        }
        running += ptsInRow;
      }

      if (valid) donors.push(i);
    }
    return donors;
  }

  static getValidReceivers(v) {
    const receivers = [];
    const treeRow = this.getTreeRowPoints(v);

    // Compute prefix sums for each tree
    const prefix = new Int32Array(24);
    for (let t = 0; t < 3; t++) {
      const tOff = t * 8;
      let run = 0;
      for (let r = 1; r <= 7; r++) {
        prefix[tOff + r] = run;
        run += treeRow[tOff + r];
      }
    }

    for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
      const node = TALENT_DEFINITIONS[i];
      if (v[i] >= node.max) continue;
      if (node.req !== null && v[node.req] < TALENT_DEFINITIONS[node.req].max) continue;

      const reqPoints = 5 * (node.row - 1);
      const pointsBelow = prefix[node.tree * 8 + node.row];
      if (pointsBelow >= reqPoints) {
        receivers.push(i);
      }
    }
    return receivers;
  }

  static repair(v, rng, targetTotal = 51) {
    for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
      v[i] = Math.max(0, Math.min(v[i], TALENT_DEFINITIONS[i].max));
    }

    // Top-down prune illegal points
    for (let tree = 0; tree < 3; tree++) {
      let running = 0;
      for (let r = 1; r <= 7; r++) {
        const req = 5 * (r - 1);
        for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
          const def = TALENT_DEFINITIONS[i];
          if (def.tree === tree && def.row === r) {
            if (running < req || (def.req !== null && v[def.req] < TALENT_DEFINITIONS[def.req].max)) {
              v[i] = 0;
            }
          }
        }
        for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
          const def = TALENT_DEFINITIONS[i];
          if (def.tree === tree && def.row === r) {
            running += v[i];
          }
        }
      }
    }

    let current = this.countTotalPoints(v);
    while (current > targetTotal) {
      const donors = this.getValidDonors(v);
      if (donors.length === 0) {
        v.fill(0);
        current = 0;
        break;
      }
      const pick = donors[rng.nextU64() % donors.length];
      v[pick]--;
      current--;
    }

    while (current < targetTotal) {
      const receivers = this.getValidReceivers(v);
      if (receivers.length === 0) break;
      const pick = receivers[rng.nextU64() % receivers.length];
      v[pick]++;
      current++;
    }
  }

  static generateRandomValid(rng, targetTotal = 51) {
    const v = new Uint8Array(TOTAL_TALENT_NODES);
    for (let p = 0; p < targetTotal; p++) {
      const receivers = this.getValidReceivers(v);
      if (receivers.length === 0) break;
      const pick = receivers[rng.nextU64() % receivers.length];
      v[pick]++;
    }
    return v;
  }

  static toTalentsObject(v) {
    const res = { affliction: {}, demonology: {}, destruction: {} };
    for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
      const node = TALENT_DEFINITIONS[i];
      if (v[i] > 0) {
        if (node.tree === 0) res.affliction[node.id] = v[i];
        else if (node.tree === 1) res.demonology[node.id] = v[i];
        else res.destruction[node.id] = v[i];
      }
    }
    return res;
  }

  static fromTalentsObject(obj = {}) {
    const v = new Uint8Array(TOTAL_TALENT_NODES);
    if (!obj) return v;
    const aff = obj.affliction || {};
    const demo = obj.demonology || {};
    const destro = obj.destruction || {};

    const findVal = (map, id, name) => {
      if (map[id] !== undefined) return Number(map[id]) || 0;
      const clean = id.replace(/_/g, ' ').toLowerCase();
      for (const [k, val] of Object.entries(map)) {
        if (k.toLowerCase().replace(/_/g, ' ') === clean || k.toLowerCase() === name.toLowerCase()) {
          return Number(val) || 0;
        }
      }
      return 0;
    };

    for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
      const node = TALENT_DEFINITIONS[i];
      const map = node.tree === 0 ? aff : node.tree === 1 ? demo : destro;
      v[i] = Math.min(node.max, findVal(map, node.id, node.name));
    }
    return v;
  }
}

export const PET_CONSTRAINTS = [
  { id: 'ALL', label: '[Search All]' },
  { id: 'ACTIVE_IMP', label: 'Imp Pet' },
  { id: 'ACTIVE_SUCCUBUS', label: 'Succubus Pet' },
  { id: 'SAC_IMP', label: 'Sac Imp (+15% Shadow)' },
  { id: 'SAC_SUCCUBUS', label: 'Sac Succubus (+15% Fire)' },
  { id: 'DEMONIC_PACT_SUCC_IMP', label: 'Imp Pet + Sac Succubus (+15% Fire)' },
  { id: 'DEMONIC_PACT_IMP_SUCC', label: 'Succubus Pet + Sac Imp (+15% Shadow)' },
  { id: 'NO_PET', label: 'No Pet' }
];

export const PET_MODES = [
  { pet: 'none', imp: false, succ: false },     // No Pet
  { pet: 'imp', imp: false, succ: false },      // Imp Pet
  { pet: 'succubus', imp: false, succ: false }, // Succubus Pet
  { pet: 'none', imp: true, succ: false },      // Sac Imp
  { pet: 'none', imp: false, succ: true },      // Sac Succubus
  { pet: 'imp', imp: false, succ: true },       // Imp Pet + Sac Succubus (DP Fire)
  { pet: 'succubus', imp: true, succ: false }   // Succubus Pet + Sac Imp (DP Shadow)
];

// Preserve required-talent repair and pet enforcement order, including RNG calls.
export function enforceTalentAndPetConstraints(ind, config, rng) {
  const reqTalents = [...(config.requiredTalents || [])];
  
  // Implicit talent requirements from Pet/Sac constraints
  if (config.forcedPetMode === 'SAC_IMP' || config.forcedPetMode === 'SAC_SUCCUBUS') {
    if (!reqTalents.includes(26)) reqTalents.push(26); // Demonic Sacrifice
  } else if (config.forcedPetMode === 'DEMONIC_PACT_IMP_SUCC' || config.forcedPetMode === 'DEMONIC_PACT_SUCC_IMP') {
    if (!reqTalents.includes(35)) reqTalents.push(35); // Demonic Pact
  }

  if (reqTalents.length > 0) {
    const requireTalent = (reqIdx) => {
      if (reqIdx < 0 || reqIdx >= TOTAL_TALENT_NODES) return;
      const target = TALENT_DEFINITIONS[reqIdx];
      if (target.req !== null) requireTalent(target.req);
      const reqBelow = 5 * (target.row - 1);
      while (true) {
        let below = 0;
        for (let k = 0; k < TOTAL_TALENT_NODES; k++) {
          if (TALENT_DEFINITIONS[k].tree === target.tree && TALENT_DEFINITIONS[k].row < target.row) {
            below += ind.talents[k];
          }
        }
        if (below >= reqBelow) break;
        const receivers = TalentGraph.getValidReceivers(ind.talents).filter(k => 
          TALENT_DEFINITIONS[k].tree === target.tree && TALENT_DEFINITIONS[k].row < target.row
        );
        if (receivers.length === 0) break;
        ind.talents[receivers[rng.nextU64() % receivers.length]]++;
      }
      ind.talents[reqIdx] = target.max;
    };

    reqTalents.forEach(t => requireTalent(t));

    // Repair while protecting required talents
    let current = TalentGraph.countTotalPoints(ind.talents);
    while (current > 51) {
      const donors = TalentGraph.getValidDonors(ind.talents).filter(d => !reqTalents.includes(d));
      if (donors.length === 0) break;
      const pick = donors[rng.nextU64() % donors.length];
      ind.talents[pick]--;
      current--;
    }

    while (current < 51) {
      const receivers = TalentGraph.getValidReceivers(ind.talents);
      if (receivers.length === 0) break;
      const pick = receivers[rng.nextU64() % receivers.length];
      ind.talents[pick]++;
      current++;
    }
  }

  // Pet & Sacrifice constraint
  if (config.forcedPetMode && config.forcedPetMode !== 'ALL') {
    switch (config.forcedPetMode) {
      case 'ACTIVE_IMP':
        ind.pet = 'imp';
        ind.sacImp = false;
        ind.sacSuccubus = false;
        break;
      case 'ACTIVE_SUCCUBUS':
        ind.pet = 'succubus';
        ind.sacImp = false;
        ind.sacSuccubus = false;
        break;
      case 'SAC_IMP':
        ind.pet = 'none';
        ind.sacImp = true;
        ind.sacSuccubus = false;
        break;
      case 'SAC_SUCCUBUS':
        ind.pet = 'none';
        ind.sacImp = false;
        ind.sacSuccubus = true;
        break;
      case 'DEMONIC_PACT_IMP_SUCC':
        ind.pet = 'succubus';
        ind.sacImp = true;
        ind.sacSuccubus = false;
        break;
      case 'DEMONIC_PACT_SUCC_IMP':
        ind.pet = 'imp';
        ind.sacImp = false;
        ind.sacSuccubus = true;
        break;
      case 'NO_PET':
        ind.pet = 'none';
        ind.sacImp = false;
        ind.sacSuccubus = false;
        break;
    }
  } else {
    const hasDS = ind.talents[26] > 0; // Demonic Sacrifice
    const hasDP = ind.talents[35] > 0; // Demonic Pact
    if (!hasDS && !hasDP) {
      ind.sacImp = false;
      ind.sacSuccubus = false;
    } else if (!hasDP && (ind.sacImp || ind.sacSuccubus)) {
      ind.pet = 'none';
    }
  }

}

export const RACES = ['HUMAN', 'ORC', 'UNDEAD', 'TROLL', 'GNOME'];

export const ROTATION_CHOICES = [
  'SHADOW_DESTRO',
  'FIRE_DESTRO',
  'DP_AF_SHADOW',
  'DP_AF_FIRE',
  'DEEP_AFFLICTION',
  'SHADOW_AND_FLAME',
  'NIGHTFALL_AFFLICTION',
  'INCINERATE_DECIMATION'
];

export const ROTATION_LABELS = {
  SHADOW_DESTRO: 'Shadow Destro (SB Spam)',
  FIRE_DESTRO: 'Fire Destro (Incinerate)',
  DP_AF_SHADOW: 'Demonic Pact Shadow',
  DP_AF_FIRE: 'Demonic Pact Fire',
  DEEP_AFFLICTION: 'Deep Affliction (Wrack/SM)',
  SHADOW_AND_FLAME: 'Shadow & Flame',
  NIGHTFALL_AFFLICTION: 'Nightfall / Affliction',
  INCINERATE_DECIMATION: 'Incinerate / Decimation'
};

// Locked selections consume no randomness and retain the existing ALL/unset behavior.
export function enforceLockedChoices(ind, config) {
  // Locked race
  if (config.forcedRace && config.forcedRace !== 'ALL') {
    ind.race = config.forcedRace;
  }

  // Locked rotation
  if (config.forcedRotation && config.forcedRotation !== 'ALL') {
    ind.rotation = config.forcedRotation;
  }

}

const searchIdentity = createWarlockSearchIdentity({
  talents: { graph: TalentGraph }, choices: { rotations: ROTATION_CHOICES },
});

export const WARLOCK_SEARCH = Object.freeze({
  id: 'warlock',
  createPresetCandidate: (dependencies) => createWarlockPresetCandidate({
    talents: WARLOCK_SEARCH.talents, apl: WARLOCK_SEARCH.apl, ...dependencies,
  }),
  identity: searchIdentity,
  results: createWarlockSearchResults({
    talents: { graph: TalentGraph }, apl: WARLOCK_APL_SEARCH, identity: searchIdentity,
  }),
  createCandidateConfig: (resolveTalentFlags) => createWarlockCandidateConfig({
    talents: WARLOCK_SEARCH.talents, apl: WARLOCK_SEARCH.apl,
    policyAPL: WARLOCK_SEARCH.policyAPL, resolveTalentFlags,
  }),
  createBuildSearch: (resolveTalentFlags) => createWarlockBuildSearch({
    talents: WARLOCK_SEARCH.talents, pets: WARLOCK_SEARCH.pets,
    choices: WARLOCK_SEARCH.choices, apl: WARLOCK_SEARCH.apl, resolveTalentFlags,
  }),
  apl: WARLOCK_APL_SEARCH,
  policyAPL: Object.freeze({ build: buildPolicyAPL }),
  choices: Object.freeze({
    races: RACES,
    rotations: ROTATION_CHOICES,
    rotationLabels: ROTATION_LABELS,
    enforceLocked: enforceLockedChoices,
  }),
  pets: Object.freeze({
    constraints: PET_CONSTRAINTS,
    modes: PET_MODES,
    enforce: enforceTalentAndPetConstraints,
  }),
  talents: Object.freeze({
    graph: TalentGraph,
    definitions: TALENT_DEFINITIONS,
    nodeCount: TOTAL_TALENT_NODES,
    treeNodeCounts: Object.freeze([AFFLICTION_NODE_COUNT, DEMONOLOGY_NODE_COUNT, DESTRUCTION_NODE_COUNT]),
    pointBudget: 51,
  }),
});
