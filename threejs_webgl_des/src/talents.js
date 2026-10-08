import { ACTIVE_CLASS, CLASS_PRESENTATION } from './classes/active_class.js';
// Interactive 3-tree Warlock Talent System (Flicker-Free DOM Architecture with Authentic Tooltips)
import { showTooltip, hideTooltip } from './tooltips.js';

let talentData = null;
let currentAllocation = {}; // key: `${treeIdx}_${talentIdx}`, value: rank
let isDomBuilt = false;
let activeTalentChangeCallback = null;

export async function initTalents(onTalentChange) {
  try {
    if (onTalentChange) activeTalentChangeCallback = onTalentChange;
    const res = await fetch(CLASS_PRESENTATION.talentDataUrl);
    if (!res.ok) throw new Error('Could not load talent data.');
    talentData = await res.json();
    buildTalentTreesDOM(activeTalentChangeCallback);
    updateTalentsView(activeTalentChangeCallback);
    return talentData;
  } catch (err) {
    console.error('Failed to initialize talents:', err);
    return null;
  }
}

export function getTalentData() {
  return talentData;
}

export function getPointsPerTree() {
  const points = [0, 0, 0];
  if (!talentData) return points;
  for (const [key, rank] of Object.entries(currentAllocation)) {
    const [tIdx] = key.split('_').map(Number);
    points[tIdx] += rank;
  }
  return points;
}

export function getTotalPoints() {
  return Object.values(currentAllocation).reduce((sum, r) => sum + r, 0);
}

// Use the same row and prerequisite rules as build imports and spec search.
export function canChangeTalentPoint(treeIdx, talentIdx, delta) {
  if (!talentData || ![1, -1].includes(delta)) return false;
  const { graph, definitions, pointBudget } = ACTIVE_CLASS.search.talents;
  const talent = talentData.trees[treeIdx]?.talents[talentIdx];
  const nodeIdx = definitions.findIndex(node => node.tree === treeIdx && node.name === talent?.name);
  if (nodeIdx < 0) return false;
  const ranks = definitions.map(node => {
    const index = talentData.trees[node.tree]?.talents.findIndex(t => t.name === node.name);
    return currentAllocation[`${node.tree}_${index}`] || 0;
  });
  if (delta > 0) {
    return getTotalPoints() < pointBudget && graph.getValidReceivers(ranks).includes(nodeIdx);
  }
  return graph.getValidDonors(ranks).includes(nodeIdx);
}

export function resetTalents(onTalentChange) {
  currentAllocation = {};
  const cb = onTalentChange || activeTalentChangeCallback;
  updateTalentsView(cb);
  if (cb) cb(getSimTalentFlags());
}

export function getTalentsObject() {
  const result = { affliction: {}, demonology: {}, destruction: {} };
  if (!talentData) return result;
  for (const [key, rank] of Object.entries(currentAllocation)) {
    const [tIdx, talIdx] = key.split('_').map(Number);
    const tree = talentData.trees[tIdx];
    const talent = tree?.talents[talIdx];
    if (tree && talent && rank > 0) {
      const treeKey = tree.name.toLowerCase();
      const talKey = talent.name.toLowerCase().replace(/[\s\'-]/g, '_');
      result[treeKey][talKey] = rank;
    }
  }
  return result;
}

export function resetTreeTalents(treeIdx, onTalentChange) {
  for (const key of Object.keys(currentAllocation)) {
    const [tIdx] = key.split('_').map(Number);
    if (tIdx === treeIdx) {
      delete currentAllocation[key];
    }
  }
  const cb = onTalentChange || activeTalentChangeCallback;
  updateTalentsView(cb);
  if (cb) cb(getSimTalentFlags());
}

export function applyTalentsObject(talentsObj, onTalentChange) {
  if (!talentData || !talentsObj) return;
  currentAllocation = {};

  for (const [treeKey, treeTalents] of Object.entries(talentsObj)) {
    const tIdx = talentData.trees.findIndex(t => t.name.toLowerCase() === treeKey.toLowerCase());
    if (tIdx === -1) continue;
    const tree = talentData.trees[tIdx];

    for (const [talKey, rank] of Object.entries(treeTalents)) {
      const normKey = talKey.toLowerCase().replace(/[\s\'-]/g, '_');
      const talIdx = tree.talents.findIndex(t => {
        const tNorm = t.name.toLowerCase().replace(/[\s\'-]/g, '_');
        return tNorm === normKey || t.name.toLowerCase() === talKey.toLowerCase().replace(/_/g, ' ');
      });
      if (talIdx !== -1) {
        const numRank = Number(rank) || 0;
        if (numRank > 0) {
          currentAllocation[`${tIdx}_${talIdx}`] = Math.min(numRank, tree.talents[talIdx].max);
        }
      }
    }
  }

  const cb = onTalentChange || activeTalentChangeCallback;
  updateTalentsView(cb);
  if (cb) cb(getSimTalentFlags());
}

export function applyTalentPreset(presetObjOrName, onTalentChange) {
  if (typeof presetObjOrName === 'object' && presetObjOrName?.talents) {
    applyTalentsObject(presetObjOrName.talents, onTalentChange);
    return;
  }
  // If string name passed, attempt string match
  console.log('Applying talent preset by name:', presetObjOrName);
}

export function getSimTalentFlags() {
  const ranks = {};
  if (talentData) {
    for (const [key, rank] of Object.entries(currentAllocation)) {
      const [treeIndex, talentIndex] = key.split('_').map(Number);
      const tree = talentData.trees[treeIndex];
      const talent = tree?.talents[talentIndex];
      if (tree && talent) {
        ranks[`${tree.name.toLowerCase()}.${talent.name.toLowerCase()}`] = rank;
      }
    }
  }
  return getTalentFlagsFromRanks(ranks);
}

// Convert the preset JSON rank map through the same talent math as the editor.
export function getTalentFlagsFromRanks(talentTreeRanks = {}) {
  const ranks = {};
  for (const [tree, talents] of Object.entries(talentTreeRanks || {})) {
    if (talents && typeof talents === 'object') {
      for (const [talent, value] of Object.entries(talents)) {
        const cleanTree = tree.toLowerCase();
        const cleanTalent = talent.toLowerCase().replace(/_/g, ' ');
        ranks[`${cleanTree}.${cleanTalent}`] = Number(value) || 0;
      }
    } else {
      const cleanKey = tree.toLowerCase().replace(/_/g, ' ');
      ranks[cleanKey] = Number(talents) || 0;
    }
  }
  const rank = (tree, talent) => ranks[`${tree}.${talent}`] || 0;
  const aff = name => rank('affliction', name);
  const demo = name => rank('demonology', name);
  const destro = name => rank('destruction', name);
  const improvedCorruption = aff('improved corruption');
  const improvedDrains = aff('improved drains');
  const agonizingFlames = destro('agonizing flames');
  const shadowMastery = aff('shadow mastery');
  const improvedImp = demo('improved imp');
  const unholyPower = demo('unholy power');
  const improvedSayaad = demo('improved sayaad');
  const masterDemo = demo('master demonologist');
  const soulLink = demo('soul link') > 0;
  const improvedShadowBolt = destro('improved shadow bolt');
  const sacRank = demo('demonic sacrifice');
  const aftermath = destro('aftermath');
  const cataclysm = destro('cataclysm');
  const cataclysmCostMult = cataclysm === 3 ? 0.90 : cataclysm === 2 ? 0.94 : cataclysm === 1 ? 0.97 : 1.0;
  const affBonus = agonizingFlames >= 3 ? 0.10 : agonizingFlames === 2 ? 0.07 : agonizingFlames === 1 ? 0.03 : 0;

  const improvedAgony = aff('improved bane of agony') || aff('improved curse of agony');
  const amplifyCurse = aff('amplify curse') > 0;
  const felVitality = demo('fel vitality');
  const demonicPact = demo('demonic pact') > 0;

  return {
    // Direct flags and rank-scaled values consumed by the current DES shader.
    improvedTap: aff('improved life tap') > 0,
    tapBonus: aff('improved life tap') * 0.10,
    suppressionHit: aff('suppression'),
    instantCorruption: improvedCorruption >= 5,
    improvedCorruptionBonus: improvedCorruption >= 5 ? 0.10 : improvedCorruption * 0.02,
    improvedAgony: improvedAgony > 0,
    improvedAgonyRank: improvedAgony,
    improvedAgonyBonus: improvedAgony * 0.05,
    amplifyCurse,
    felVitality: felVitality > 0,
    felVitalityRank: felVitality,
    felVitalityBonus: felVitality * 0.05,
    maledictionBonus: aff('malediction') * 0.01,
    improvedDrainsBonus: improvedDrains >= 3 ? 0.20 : improvedDrains === 2 ? 0.13 : improvedDrains === 1 ? 0.07 : 0,
    malevolence: aff('malevolence'),
    nightfall: aff('nightfall') > 0,
    nightfallChance: aff('nightfall') * 0.02,
    isb: improvedShadowBolt > 0,
    isbBonus: improvedShadowBolt * 0.04,
    ruin: destro('ruin') > 0,
    ruinRank: destro('ruin'),
    shadowburn: destro('shadowburn') > 0,
    siphonLife: aff('siphon life') > 0,
    soulSiphonBonus: aff('soul siphon') * 0.04,
    decimation: demo('decimation') > 0,
    decimationRank: demo('decimation'),
    baneRank: destro('bane'),
    demonicBrand: demo('demonic brand') > 0,
    demonicBrandRank: demo('demonic brand'),
    demonicEnergies: demo('demonic energies'),
    demonicKnowledge: demo('demonic knowledge'),
    demonicPact,
    masterDemo,
    soulLink,
    sacRank,
    conflagrate: destro('conflagrate') > 0,
    incinerate: destro('incinerate') > 0,
    wrack: aff('wrack') > 0,
    aftermath: aftermath > 0,
    aftermathRank: aftermath,
    aftermathBonus: aftermath * 0.10,
    cataclysm: cataclysm > 0,
    cataclysmRank: cataclysm,
    cataclysmCostMult,
    afBonus: affBonus,
    fnbCrit: [0, 8, 17, 25][destro('fire and brimstone')] || 0,
    snfChance: destro('shadow and flame') * 0.20,
    snfBonus: destro('shadow and flame') * 0.02,
    dotCrit: 1.0 + 0.50 * (1.0 + aff('pandemic') / 3),
    shadowMasteryBonus: shadowMastery * 0.01,
    shadowMultiplier: 1,
    fireMultiplier: 1,
    corrMultiplier: 0,
    petFireboltMult: (1 + unholyPower * 0.02 + improvedImp * 0.10) * (1 + masterDemo * 0.02) * (soulLink ? 1.03 : 1),
    petMeleeMult: (1 + unholyPower * 0.02) * (soulLink ? 1.03 : 1),
    petLashMult: (1 + unholyPower * 0.02 + improvedSayaad * 0.10) * (1 + masterDemo * 0.02) * (soulLink ? 1.03 : 1),
    brandMult: (1 + unholyPower * 0.02) * (1 + masterDemo * 0.02) * (soulLink ? 1.03 : 1),
  };
}

// Build the DOM structure once to completely prevent browser image/node flicker
function buildTalentTreesDOM(onTalentChange) {
  const targets = [
    document.getElementById('talent-trees-container'),
    document.getElementById('talent-trees-container-full')
  ].filter(Boolean);

  if (targets.length === 0 || !talentData) return;

  targets.forEach(container => {
    container.innerHTML = '';
    talentData.trees.forEach((tree, tIdx) => {
      const treeCard = document.createElement('div');
      const treeClass = tree.name.toLowerCase();
      treeCard.className = `talent-tree-panel ${treeClass}`;
      const background = CLASS_PRESENTATION.talentBackgrounds[treeClass];
      if (background) treeCard.style.setProperty('--talent-background', `url("${background}")`);

      const header = document.createElement('div');
      header.className = 'tree-header';
      header.innerHTML = `
        <span class="tree-title">${tree.name}</span>
        <div class="tree-header-right">
          <span class="tree-points-display" data-tree="${tIdx}">0 pts</span>
          <button type="button" class="tree-reset-btn" title="Reset ${tree.name} Talents" data-tree="${tIdx}">✕</button>
        </div>
      `;
      header.querySelector('.tree-reset-btn')?.addEventListener('click', (e) => {
        e.stopPropagation();
        resetTreeTalents(tIdx, onTalentChange);
      });
      treeCard.appendChild(header);

      const grid = document.createElement('div');
      grid.className = 'talent-grid';

      for (let r = 1; r <= 7; r++) {
        for (let c = 1; c <= 4; c++) {
          const talIdx = tree.talents.findIndex(t => t.row === r && t.col === c);
          if (talIdx !== -1) {
            const talent = tree.talents[talIdx];
            const rankKey = `${tIdx}_${talIdx}`;

            const node = document.createElement('div');
            node.className = 'talent-node disabled';
            node.dataset.rankKey = rankKey;
            node.dataset.tIdx = tIdx;
            node.dataset.row = r;
            node.dataset.max = talent.max;

            const iconFile = talent.icon ? `${talent.icon}.png` : CLASS_PRESENTATION.talentDefaultIcon;
            node.innerHTML = `
              <div class="talent-icon-frame">
                <img src="./assets/icons/${iconFile}" alt="${talent.name}" onerror="this.src='./assets/icons/${CLASS_PRESENTATION.talentFallbackIcon}'">
              </div>
              <span class="talent-rank-badge">0/${talent.max}</span>
            `;

            // Tooltip events
            node.addEventListener('mouseenter', (e) => {
              renderTalentTooltip(e, tIdx, talIdx, r, talent, rankKey, tree.name);
            });
            node.addEventListener('mousemove', (e) => {
              renderTalentTooltip(e, tIdx, talIdx, r, talent, rankKey, tree.name);
            });
            node.addEventListener('mouseleave', () => {
              hideTooltip();
            });

            // Left click = allocate point
            node.addEventListener('click', (e) => {
              e.preventDefault();
              const currentRank = currentAllocation[rankKey] || 0;
              if (!canChangeTalentPoint(tIdx, talIdx, 1)) return;
              if (currentRank < talent.max) {
                currentAllocation[rankKey] = currentRank + 1;
                updateTalentsView(onTalentChange);
                if (onTalentChange) onTalentChange(getSimTalentFlags());
                renderTalentTooltip(e, tIdx, talIdx, r, talent, rankKey, tree.name);
              }
            });

            // Right click = refund point
            node.addEventListener('contextmenu', (e) => {
              e.preventDefault();
              const currentRank = currentAllocation[rankKey] || 0;
              if (currentRank > 0 && canChangeTalentPoint(tIdx, talIdx, -1)) {
                currentAllocation[rankKey] = currentRank - 1;
                if (currentAllocation[rankKey] === 0) delete currentAllocation[rankKey];
                updateTalentsView(onTalentChange);
                if (onTalentChange) onTalentChange(getSimTalentFlags());
                renderTalentTooltip(e, tIdx, talIdx, r, talent, rankKey, tree.name);
              }
            });

            grid.appendChild(node);
          } else {
            const emptyCell = document.createElement('div');
            grid.appendChild(emptyCell);
          }
        }
      }

      treeCard.appendChild(grid);
      container.appendChild(treeCard);
    });
  });

  isDomBuilt = true;
}

function renderTalentTooltip(e, tIdx, talIdx, row, talent, rankKey, treeName) {
  const currentRank = currentAllocation[rankKey] || 0;
  const reqPoints = (row - 1) * 5;
  const isUnlocked = canChangeTalentPoint(tIdx, talIdx, 1);

  const title = talent.name;
  const subtitle = `Rank ${currentRank} / ${talent.max}`;
  
  let desc = '';
  let nextRankDesc = '';

  if (currentRank > 0) {
    desc = talent.desc[currentRank - 1] || talent.desc[0];
    if (currentRank < talent.max) {
      nextRankDesc = talent.desc[currentRank] || '';
    }
  } else {
    desc = talent.desc[0] || '';
  }

  const instructions = [];
  if (currentRank < talent.max) {
    if (isUnlocked) instructions.push('Left-click to learn.');
    else instructions.push(`<span style="color:#ef4444;">Requires ${reqPoints} points in earlier rows, all prerequisite talents, and an available talent point.</span>`);
  }
  if (currentRank > 0) {
    instructions.push(canChangeTalentPoint(tIdx, talIdx, -1)
      ? 'Right-click to unlearn.'
      : '<span style="color:#ef4444;">Unlearn dependent talents first; this point is required by other talents.</span>');
  }
  const footer = instructions.join('<br>');

  showTooltip(e, { title, subtitle, desc, nextRankDesc, footer });
}

// In-place node updates: modifies only text content and classes without rebuilding DOM
export function updateTalentsView(onTalentChange) {
  if (!isDomBuilt) {
    buildTalentTreesDOM(onTalentChange);
  }

  const pointsPerTree = getPointsPerTree();
  const totalPoints = getTotalPoints();

  // Update summary badge counters
  const badges = [
    document.getElementById('talents-summary-badge'),
    document.getElementById('talents-summary-badge-2')
  ].filter(Boolean);

  badges.forEach(badge => {
    badge.textContent = `${pointsPerTree.join(' / ')} (${51 - totalPoints} remaining)`;
  });

  // Update tree header point counters
  document.querySelectorAll('.tree-points-display').forEach(el => {
    const tIdx = Number(el.dataset.tree);
    if (!isNaN(tIdx) && pointsPerTree[tIdx] !== undefined) {
      el.textContent = `${pointsPerTree[tIdx]} pts`;
    }
  });

  // Update all talent nodes in-place
  document.querySelectorAll('.talent-node').forEach(node => {
    const rankKey = node.dataset.rankKey;
    if (!rankKey) return;

    const [tIdx, talIdx] = rankKey.split('_').map(Number);
    const talent = talentData?.trees[tIdx]?.talents[talIdx];
    if (!talent) return;

    const currentRank = currentAllocation[rankKey] || 0;
    const isUnlocked = canChangeTalentPoint(tIdx, talIdx, 1);
    const isMax = currentRank === talent.max;
    const isActive = currentRank > 0;

    node.className = `talent-node ${isActive ? 'active' : ''} ${isMax ? 'max' : ''} ${!isUnlocked && !isActive ? 'disabled' : ''}`;

    const badge = node.querySelector('.talent-rank-badge');
    if (badge) {
      badge.textContent = `${currentRank}/${talent.max}`;
    }
  });
}
