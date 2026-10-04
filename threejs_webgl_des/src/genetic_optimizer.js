// Genetic Algorithm Constrained Spec Search Engine for Classic WoW Warlock DES
// Seamlessly operates in tandem with WebGL2 multi-config GPU shader simulation
import { runMultiSimulation, summarize } from './engine.js';
import { getTalentFlagsFromRanks } from './talents.js';
import { buildFightConfig } from './config_builder.js';
import { APL_ACTION, APL_COND, MAX_APL_RULES } from './model.js';
import { getPresetPetAndSac } from './presets.js';

export const TOTAL_TALENT_NODES = 52;
export const AFFLICTION_NODE_COUNT = 17;
export const DEMONOLOGY_NODE_COUNT = 19;
export const DESTRUCTION_NODE_COUNT = 16;

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
}

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
      if (node.tree === 0) res.affliction[node.id] = v[i];
      else if (node.tree === 1) res.demonology[node.id] = v[i];
      else res.destruction[node.id] = v[i];
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

// Enforce Constraints matching C++ oracle
export function enforceConstraints(ind, config, rng) {
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

  // Locked race
  if (config.forcedRace && config.forcedRace !== 'ALL') {
    ind.race = config.forcedRace;
  }

  // Locked rotation
  if (config.forcedRotation && config.forcedRotation !== 'ALL') {
    ind.rotation = config.forcedRotation;
  }
}

// Generate Priority APL rules and actionIds based on policy and talents
export function getPolicyAPLAndActions(ind) {
  const talentsObj = TalentGraph.toTalentsObject(ind.talents);
  const tf = getTalentFlagsFromRanks(talentsObj);
  const rot = ind.rotation;
  
  let shaderRotation = 'shadow';
  const actions = new Set();
  const rules = [];

  const addRule = (action, cond = APL_COND.ALWAYS, param = 0.0, targetSpell = 0) => {
    if (rules.length < MAX_APL_RULES) {
      rules.push({ action, cond, param, targetSpell, enabled: 1 });
    }
  };

  // Rule 0: Life Tap resource safeguard
  addRule(APL_ACTION.LIFE_TAP, APL_COND.MANA_LE, 25.0);

  if (rot === 'FIRE_DESTRO' || rot === 'INCINERATE_DECIMATION') {
    shaderRotation = 'fire';
    actions.add('corr');
    actions.add('immo');
    if (tf.conflagrate) actions.add('conflag');
    if (tf.incinerate) actions.add('incinerate');
    if (tf.shadowburn) actions.add('shadowburn');
    
    if (tf.decimation) {
      addRule(APL_ACTION.DECIMATION_SEARING_PAIN, APL_COND.DECIMATION_INACTIVE, 35.0, 5);
      addRule(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 35.0, 13);
    }
    if (tf.immolate || tf.conflagrate) addRule(APL_ACTION.IMMOLATE, APL_COND.DOT_REM_LE, 0.0, 3);
    if (tf.conflagrate) addRule(APL_ACTION.CONFLAGRATE, APL_COND.ALWAYS);
    if (tf.shadowburn) addRule(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
    addRule(tf.incinerate ? APL_ACTION.INCINERATE_FILLER : APL_ACTION.SEARING_PAIN_FILLER, APL_COND.ALWAYS);
  } else if (rot === 'DP_AF_FIRE') {
    shaderRotation = 'searing';
    actions.add('corr');
    actions.add('agony');
    actions.add('curse');
    actions.add('immo');
    if (tf.demonicBrand) actions.add('brand');
    if (tf.conflagrate) actions.add('conflag');
    
    addRule(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 57.0, 2);
    addRule(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 2.5, 2);
    addRule(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 2.5, 1);
    addRule(APL_ACTION.IMMOLATE, APL_COND.DOT_REM_LE, 2.5, 3);
    if (tf.demonicBrand) addRule(APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, APL_COND.DEMONIC_BRAND_MISSING, 0.0, 5);
    if (tf.decimation) {
      addRule(APL_ACTION.DECIMATION_SEARING_PAIN, APL_COND.DECIMATION_INACTIVE, 28.0, 5);
      addRule(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 28.0, 13);
    }
    if (tf.conflagrate) addRule(APL_ACTION.CONFLAGRATE, APL_COND.ALWAYS);
    addRule(APL_ACTION.SEARING_PAIN_FILLER, APL_COND.ALWAYS);
  } else if (rot === 'DEEP_AFFLICTION' || rot === 'NIGHTFALL_AFFLICTION') {
    shaderRotation = 'shadow';
    actions.add('corr');
    actions.add('agony');
    actions.add('curse');
    if (tf.siphonLife) actions.add('siphon');
    if (tf.wrack) actions.add('wrack');
    if (tf.shadowburn) actions.add('shadowburn');

    if (tf.nightfall) addRule(APL_ACTION.NIGHTFALL_SHADOW_BOLT, APL_COND.SHADOW_TRANCE);
    if (tf.wrack) addRule(APL_ACTION.DRAIN_HOPE, APL_COND.ALWAYS);
    addRule(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 0.0, 1);
    addRule(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 60.0, 2);
    addRule(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 0.0, 2);
    if (tf.siphonLife) addRule(APL_ACTION.SIPHON_LIFE, APL_COND.DOT_REM_LE, 0.0, 15);
    if (tf.shadowburn) addRule(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
    addRule(APL_ACTION.SHADOW_BOLT_FILLER, APL_COND.ALWAYS);
  } else if (rot === 'SHADOW_AND_FLAME') {
    shaderRotation = 'shadow';
    actions.add('corr');
    actions.add('agony');
    actions.add('curse');
    actions.add('immo');
    if (tf.conflagrate) actions.add('conflag');
    if (tf.shadowburn) actions.add('shadowburn');
    if (tf.demonicBrand) actions.add('brand');

    if (tf.nightfall) addRule(APL_ACTION.NIGHTFALL_SHADOW_BOLT, APL_COND.SHADOW_TRANCE);
    if (tf.demonicBrand) addRule(APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, APL_COND.DEMONIC_BRAND_MISSING, 0.0, 5);
    if (tf.decimation) {
      addRule(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 35.0, 13);
    }
    addRule(APL_ACTION.IMMOLATE, APL_COND.DOT_REM_LE, 0.0, 3);
    if (tf.conflagrate) addRule(APL_ACTION.CONFLAGRATE, APL_COND.ALWAYS);
    addRule(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 0.0, 1);
    addRule(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 60.0, 2);
    addRule(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 0.0, 2);
    if (tf.shadowburn) addRule(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
    addRule(APL_ACTION.SHADOW_BOLT_FILLER, APL_COND.ALWAYS);
  } else {
    // SHADOW_DESTRO / DP_AF_SHADOW
    shaderRotation = 'shadow';
    actions.add('corr');
    actions.add('agony');
    actions.add('curse');
    if (tf.shadowburn) actions.add('shadowburn');
    if (tf.demonicBrand) actions.add('brand');

    if (tf.nightfall) addRule(APL_ACTION.NIGHTFALL_SHADOW_BOLT, APL_COND.SHADOW_TRANCE);
    if (tf.demonicBrand) addRule(APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, APL_COND.DEMONIC_BRAND_MISSING, 0.0, 5);
    if (tf.decimation) {
      addRule(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 35.0, 13);
    }
    addRule(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 0.0, 1);
    addRule(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 60.0, 2);
    addRule(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 0.0, 2);
    if (tf.shadowburn) addRule(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
    addRule(APL_ACTION.SHADOW_BOLT_FILLER, APL_COND.ALWAYS);
  }

  return { shaderRotation, actionIds: Array.from(actions), aplRules: rules };
}

// Convert Individual to Shader Simulation Config
export function individualToConfig(ind, baseStatsConfig) {
  const talentsObj = TalentGraph.toTalentsObject(ind.talents);
  const tf = getTalentFlagsFromRanks(talentsObj);
  const { shaderRotation, actionIds, aplRules } = getPolicyAPLAndActions(ind);

  const base = {
    ...baseStatsConfig,
    rotation: shaderRotation,
    tapThreshold: baseStatsConfig.tapThreshold !== undefined ? baseStatsConfig.tapThreshold : 25
  };
  const baseRace = (base.race || 'HUMAN').toUpperCase();
  delete base.race;

  // Un-apply baseline race stats if needed so all candidates start from neutral baseline
  let unscaledInt = base.intellect;
  let unscaledSpirit = base.spirit;
  if (baseRace === 'GNOME') {
    unscaledInt = Math.round(unscaledInt / 1.05);
  } else if (baseRace === 'HUMAN') {
    unscaledSpirit = Math.round(unscaledSpirit / 1.05);
  }

  // Apply candidate's race modifiers
  if (ind.race === 'GNOME') {
    base.intellect = Math.round(unscaledInt * 1.05);
    base.spirit = unscaledSpirit;
  } else if (ind.race === 'HUMAN') {
    base.intellect = unscaledInt;
    base.spirit = Math.round(unscaledSpirit * 1.05);
  } else {
    base.intellect = unscaledInt;
    base.spirit = unscaledSpirit;
  }

  // Orc racial: Command (+5% Pet Damage)
  if (ind.race === 'ORC') {
    tf.petFireboltMult = (tf.petFireboltMult || 1.0) * 1.05;
    tf.petMeleeMult = (tf.petMeleeMult || 1.0) * 1.05;
    tf.petLashMult = (tf.petLashMult || 1.0) * 1.05;
  }

  return buildFightConfig({
    base,
    talent: tf,
    pet: ind.pet,
    sac: ind.sacImp ? 'imp' : ind.sacSuccubus ? 'succubus' : 'none',
    actionIds,
    aplRules,
    rotation: shaderRotation
  });
}

// Format Human-Readable Spec Name Matching Desktop App
export function formatBuildName(ind) {
  const a = TalentGraph.countTreePoints(ind.talents, 0);
  const d = TalentGraph.countTreePoints(ind.talents, 1);
  const x = TalentGraph.countTreePoints(ind.talents, 2);

  const tObj = TalentGraph.toTalentsObject(ind.talents);
  const aff = tObj.affliction || {};
  const demo = tObj.demonology || {};
  const destro = tObj.destruction || {};

  const tags = [];
  if (aff.wrack > 0) tags.push('Wrack');
  else if (aff.shadow_mastery > 0) tags.push('SM');
  else if (aff.siphon_life > 0) tags.push('SL');
  else if (aff.nightfall > 0) tags.push('NF');
  else if (a >= 20) tags.push('Aff');

  if (demo.demonic_pact > 0) tags.push('DP');
  else if (demo.master_demonologist > 0) tags.push('MD');
  else if (demo.soul_link > 0) tags.push('Soul Link');
  else if (demo.demonic_sacrifice > 0 && demo.decimation > 0) tags.push('DS/Deci');
  else if (demo.demonic_sacrifice > 0) tags.push('DS');
  else if (demo.decimation > 0) tags.push('Deci');
  else if (d >= 20) tags.push('Demo');

  if (destro.incinerate > 0) tags.push('Incin');
  else if (destro.shadow_and_flame > 0) tags.push('S&F');
  else if (destro.conflagrate > 0) tags.push('Conflag');
  else if (destro.ruin > 0) tags.push('Ruin');
  else if (destro.shadowburn > 0) tags.push('Sburn');
  else if (x >= 20) tags.push('Destro');

  let tagStr = tags.join('/');
  if (!tagStr) {
    if (x >= a && x >= d) tagStr = 'Destro';
    else if (a >= d && a >= x) tagStr = 'Aff';
    else tagStr = 'Demo';
  }

  const raceFormatted = ind.race ? (ind.race.charAt(0) + ind.race.slice(1).toLowerCase()) : '';
  return `${a}/${d}/${x} ${tagStr} ${raceFormatted}`.trim();
}

// MAP-Elites Quality-Diversity Key
export function getMapElitesKey(ind) {
  const a = TalentGraph.countTreePoints(ind.talents, 0);
  const d = TalentGraph.countTreePoints(ind.talents, 1);
  const x = TalentGraph.countTreePoints(ind.talents, 2);

  const u = Math.min(7, Math.max(0, Math.floor(((x - a + 51) / 102.0) * 8)));
  const v = Math.min(7, Math.max(0, Math.floor((d / 51.0) * 8)));
  const petIdx = ind.pet === 'none' ? 0 : ind.pet === 'imp' ? 1 : 2;
  const sacIdx = ind.sacImp ? 1 : ind.sacSuccubus ? 2 : 0;
  const p = petIdx * 3 + sacIdx;
  const rotIdx = Math.max(0, ROTATION_CHOICES.indexOf(ind.rotation));

  return `${u}_${v}_${p}_${rotIdx}`;
}

export function createRandomIndividual(rng, config) {
  const pm = PET_MODES[rng.nextU64() % PET_MODES.length];
  const ind = {
    talents: TalentGraph.generateRandomValid(rng),
    race: config.forcedRace && config.forcedRace !== 'ALL' ? config.forcedRace : RACES[rng.nextU64() % RACES.length],
    rotation: config.forcedRotation && config.forcedRotation !== 'ALL' ? config.forcedRotation : ROTATION_CHOICES[rng.nextU64() % ROTATION_CHOICES.length],
    pet: pm.pet,
    sacImp: pm.imp,
    sacSuccubus: pm.succ,
    fitness: 0,
    batch: null
  };

  enforceConstraints(ind, config, rng);
  return ind;
}

export function crossoverIndividuals(p1, p2, rng, config) {
  const useP1Pet = rng.nextU64() % 2 === 0;
  const child = {
    talents: new Uint8Array(TOTAL_TALENT_NODES),
    race: rng.nextU64() % 2 === 0 ? p1.race : p2.race,
    rotation: rng.nextU64() % 2 === 0 ? p1.rotation : p2.rotation,
    pet: useP1Pet ? p1.pet : p2.pet,
    sacImp: useP1Pet ? p1.sacImp : p2.sacImp,
    sacSuccubus: useP1Pet ? p1.sacSuccubus : p2.sacSuccubus,
    fitness: 0,
    batch: null
  };

  for (let i = 0; i < TOTAL_TALENT_NODES; i++) {
    child.talents[i] = rng.nextU64() % 2 === 0 ? p1.talents[i] : p2.talents[i];
  }

  TalentGraph.repair(child.talents, rng);
  enforceConstraints(child, config, rng);
  return child;
}

export function mutateIndividual(ind, rng, config) {
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
}

// Convert simulated result object to standard CandidateResult representation
export function createCandidateResult(ind, rank = 1) {
  const a = TalentGraph.countTreePoints(ind.talents, 0);
  const d = TalentGraph.countTreePoints(ind.talents, 1);
  const x = TalentGraph.countTreePoints(ind.talents, 2);

  let cat = 'Hybrid Peak';
  if (x >= 25 && x >= a && x >= d) cat = 'Destruction Peak';
  else if (a >= 25 && a >= x && a >= d) cat = 'Affliction Peak';
  else if (d >= 25 && d >= a && d >= x) cat = 'Demonology Peak';

  const sum = ind.batch?.summary || {};
  const shadowDmg = sum.shadowDamage || 0;
  const fireDmg = sum.fireDamage || 0;
  const petDmg = sum.petDamage || 0;
  const totalDmg = shadowDmg + fireDmg + petDmg || 1;

  const shadowPct = (shadowDmg / totalDmg) * 100;
  const firePct = (fireDmg / totalDmg) * 100;
  const petPct = (petDmg / totalDmg) * 100;

  return {
    rank,
    name: formatBuildName(ind),
    category: cat,
    race: ind.race,
    pet: ind.pet,
    sacImp: ind.sacImp,
    sacSuccubus: ind.sacSuccubus,
    rotation: ind.rotation,
    mean_dps: sum.mean || ind.fitness,
    std_dev: sum.sd || 0,
    min_dps: sum.p05 || 0,
    max_dps: sum.p95 || 0,
    ci95: sum.ci95 || 0,
    shadow_pct: shadowPct,
    fire_pct: firePct,
    pet_pct: petPct,
    talents: TalentGraph.toTalentsObject(ind.talents),
    talentsVector: Array.from(ind.talents),
    summary: sum,
    states: ind.batch?.states || [],
    individual: ind
  };
}

// Generate unique individual key for duplicate prevention
export function getIndUniqueKey(ind) {
  return `${ind.race}_${ind.rotation}_${ind.pet}_${ind.sacImp ? 1 : 0}_${ind.sacSuccubus ? 1 : 0}_${Array.from(ind.talents).join(',')}`;
}

export function generateUniqueRandomIndividual(rng, config, seenConfigsSet, maxAttempts = 20) {
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

export function generateUniqueOffspring(parents, rng, config, seenConfigsSet, maxAttempts = 20) {
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
export async function runConstrainedGeneticSearch(baseStatsConfig, gaConfig, { signal, onProgress = () => {}, onGeneration = () => {} } = {}) {
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

      const ind = {
        talents: TalentGraph.fromTalentsObject(p.talents),
        race: (gaConfig.forcedRace && gaConfig.forcedRace !== 'ALL') ? gaConfig.forcedRace : (p.race?.toUpperCase() || 'HUMAN'),
        rotation: initialRotation,
        pet: p.pet || presetPet || 'none',
        sacImp: p.sac === 'imp' || presetSac === 'imp',
        sacSuccubus: p.sac === 'succubus' || presetSac === 'succubus',
        fitness: 0,
        batch: null
      };
      enforceConstraints(ind, gaConfig, rng);
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
  let currentSimPromise = runMultiSimulation(gen0Configs, { signal, iterations: screeningSims });
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
    currentPool[i].fitness = res.summary.mean;
    currentPool[i].batch = res;
    const key = getMapElitesKey(currentPool[i]);
    if (!mapElitesGrid.has(key) || currentPool[i].fitness > mapElitesGrid.get(key).fitness) {
      mapElitesGrid.set(key, { ...currentPool[i] });
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
    currentSimPromise = runMultiSimulation(activeBatch.configs, { signal, iterations: screeningSims });

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
      activeBatch.pool[i].fitness = res.summary.mean;
      activeBatch.pool[i].batch = res;
      const key = getMapElitesKey(activeBatch.pool[i]);
      if (!mapElitesGrid.has(key) || activeBatch.pool[i].fitness > mapElitesGrid.get(key).fitness) {
        mapElitesGrid.set(key, { ...activeBatch.pool[i] });
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
    const finalSimRes = await runMultiSimulation(finalConfigs, { signal, iterations: finalSims });

    for (let i = 0; i < finalCandidates.length; i++) {
      finalCandidates[i].batch = finalSimRes.results[i];
      finalCandidates[i].fitness = finalSimRes.results[i].summary.mean;
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
