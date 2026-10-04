import { fallbackRow } from './apl_fallback_view.js';
// Constrained Spec Search Dashboard & View Controller for Three.js WebGL DES
import {
  TALENT_DEFINITIONS,
  ROTATION_CHOICES,
  ROTATION_LABELS,
  PET_CONSTRAINTS,
  RACES,
  runConstrainedGeneticSearch,
  getPolicyAPLAndActions,
  TalentGraph
} from './genetic_optimizer.js';
import { showTooltip, hideTooltip } from './tooltips.js';
import { SPELLS } from './model.js';

const SPELL_ICONS = {
  LIFE_TAP: 'Spell_Shadow_BurningSpirit.png',
  CORRUPTION: 'Spell_Shadow_AbominationExplosion.png',
  IMMOLATE: 'Spell_Fire_Immolation.png',
  SEARING_PAIN: 'Spell_Fire_SoulBurn.png',
  SHADOW_BOLT: 'Spell_Shadow_ShadowBolt.png',
  CONFLAGRATE: 'Spell_Fire_Fireball.png',
  INCINERATE: 'Spell_Fire_Burnout.png',
  DRAIN_SOUL: 'Spell_Shadow_Haunting.png',
  SIPHON_LIFE: 'Spell_Shadow_Requiem.png',
  AMPLIFY_CURSE: 'Spell_Shadow_Contagion.png',
  CURSE_OF_AGONY: 'Spell_Shadow_CurseOfSargeras.png',
  CURSE_OF_DOOM: 'Spell_Shadow_AuraOfDarkness.png',
  BANE_OF_AGONY: 'Spell_Shadow_CurseOfSargeras.png',
  BANE_OF_DOOM: 'Spell_Shadow_AuraOfDarkness.png',
  DEMONIC_BRAND: 'ability_demonhunter_chaoticimprint_fire.png',
  NIGHTFALL: 'Spell_Shadow_Twilight.png',
  DECIMATION_SOUL_FIRE: 'Spell_Fire_Fireball02.png',
  DRAIN_HOPE: 'ability_deathknight_hemorrhagicfever.png',
  SHADOWBURN: 'Spell_Shadow_ScourgeBuild.png'
};

const RACE_ICONS = {
  HUMAN: 'Achievement_Character_Human_Male.png',
  ORC: 'Achievement_Character_Orc_Male.png',
  UNDEAD: 'Achievement_Character_Undead_Male.png',
  TROLL: 'Achievement_Character_Troll_Male.png',
  GNOME: 'Achievement_Character_Gnome_Male.png',
  Human: 'Achievement_Character_Human_Male.png',
  Orc: 'Achievement_Character_Orc_Male.png',
  Undead: 'Achievement_Character_Undead_Male.png',
  Troll: 'Achievement_Character_Troll_Male.png',
  Gnome: 'Achievement_Character_Gnome_Male.png'
};

const PET_ICONS = {
  imp: 'Spell_Shadow_SummonImp.png',
  succubus: 'Spell_Shadow_SummonSuccubus.png',
  none: null
};

let currentCandidates = [];
let selectedCandidate = null;
let currentEvolutionHistory = [];
let onApplyCandidateCallback = null;

function applyCandidate(cand) {
  if (!cand) return;
  selectedCandidate = cand;
  onApplyCandidateCallback?.(cand);
  document.getElementById('btn-current-build')?.click();
}

export function initConstrainedSearchView(onApplyCandidate) {
  onApplyCandidateCallback = onApplyCandidate;

  populateTalentDropdowns();
  populateRotationDropdown();
  populatePetDSDropdown();
  populateRaceDropdown();

  // Advanced tuning toggle
  const tuningChk = document.getElementById('ga-advanced-tuning-chk');
  const tuningBox = document.getElementById('ga-advanced-tuning-box');
  if (tuningChk && tuningBox) {
    tuningChk.addEventListener('change', () => {
      tuningBox.style.display = tuningChk.checked ? 'block' : 'none';
    });
  }

  // Checkboxes
  ['ga-chk-pct-leader', 'ga-chk-show-sd'].forEach(id => {
    document.getElementById(id)?.addEventListener('change', () => {
      renderGALeaderboard();
      if (selectedCandidate) renderSelectedCandidateDetails(selectedCandidate);
    });
  });

  // Resize listener for graph canvas
  window.addEventListener('resize', () => {
    if (currentEvolutionHistory.length > 0) {
      renderGAGraph(currentEvolutionHistory);
    }
  });

  // Tab switch listener
  document.getElementById('btn-constrained-search')?.addEventListener('click', () => {
    setTimeout(() => {
      if (currentEvolutionHistory.length > 0) {
        renderGAGraph(currentEvolutionHistory);
      }
    }, 50);
  });
}

function populateTalentDropdowns() {
  ['ga-req-talent-1', 'ga-req-talent-2', 'ga-req-talent-3'].forEach(id => {
    const el = document.getElementById(id);
    if (!el) return;
    el.innerHTML = '<option value="-1">[None]</option>';

    const affGroup = document.createElement('optgroup');
    affGroup.label = 'Affliction';
    const demoGroup = document.createElement('optgroup');
    demoGroup.label = 'Demonology';
    const destroGroup = document.createElement('optgroup');
    destroGroup.label = 'Destruction';

    TALENT_DEFINITIONS.forEach((t, idx) => {
      const opt = document.createElement('option');
      opt.value = String(idx);
      opt.textContent = `${t.name} (${t.treeName.slice(0, 4)} R${t.row}C${t.col})`;
      if (t.tree === 0) affGroup.appendChild(opt);
      else if (t.tree === 1) demoGroup.appendChild(opt);
      else destroGroup.appendChild(opt);
    });

    el.appendChild(affGroup);
    el.appendChild(demoGroup);
    el.appendChild(destroGroup);
  });
}

function populateRotationDropdown() {
  const el = document.getElementById('ga-locked-rotation');
  if (!el) return;
  el.innerHTML = '<option value="ALL">[Search All]</option>';
  ROTATION_CHOICES.forEach(r => {
    const opt = document.createElement('option');
    opt.value = r;
    opt.textContent = ROTATION_LABELS[r] || r;
    el.appendChild(opt);
  });
}

function populatePetDSDropdown() {
  const el = document.getElementById('ga-locked-pet-ds');
  if (!el) return;
  el.innerHTML = '';
  PET_CONSTRAINTS.forEach(p => {
    const opt = document.createElement('option');
    opt.value = p.id;
    opt.textContent = p.label;
    el.appendChild(opt);
  });
}

function populateRaceDropdown() {
  const el = document.getElementById('ga-locked-race');
  if (!el) return;
  el.innerHTML = `
    <option value="ALL">[Any / Evolve]</option>
    <option value="HUMAN">Human (+5% Spirit)</option>
    <option value="ORC">Orc (Blood Fury, +5% Pet)</option>
    <option value="UNDEAD">Undead (WotF)</option>
    <option value="TROLL">Troll (Berserking)</option>
    <option value="GNOME">Gnome (+5% Int)</option>
  `;
}

export function readGAConfig() {
  const gens = Number(document.getElementById('ga-generations')?.value || 15);
  const pop = Number(document.getElementById('ga-pop-size')?.value || 1000);
  const screening = Number(document.getElementById('ga-screening-sims')?.value || 100);
  const finalSims = Number(document.getElementById('ga-final-sims')?.value || 2000);
  const seedPresets = document.getElementById('ga-seed-presets')?.checked ?? true;
  const optRace = document.getElementById('ga-optimize-race')?.checked ?? true;

  const t1 = Number(document.getElementById('ga-req-talent-1')?.value || -1);
  const t2 = Number(document.getElementById('ga-req-talent-2')?.value || -1);
  const t3 = Number(document.getElementById('ga-req-talent-3')?.value || -1);

  const reqTalents = [];
  if (t1 >= 0) reqTalents.push(t1);
  if (t2 >= 0 && !reqTalents.includes(t2)) reqTalents.push(t2);
  if (t3 >= 0 && !reqTalents.includes(t3)) reqTalents.push(t3);

  const forcedRace = document.getElementById('ga-locked-race')?.value || 'ALL';
  const forcedRotation = document.getElementById('ga-locked-rotation')?.value || 'ALL';
  const forcedPetMode = document.getElementById('ga-locked-pet-ds')?.value || 'ALL';

  const mutRate = Number(document.getElementById('ga-mutation-rate')?.value || 0.45);
  const initExplore = Number(document.getElementById('ga-init-explore')?.value || 0.50);
  const minExplore = Number(document.getElementById('ga-min-explore')?.value || 0.15);

  return {
    generations: Math.max(1, gens),
    populationSize: Math.max(2, pop),
    screeningSims: Math.max(10, screening),
    finalSims: Math.max(50, finalSims),
    seedPresets,
    optimizeRace: forcedRace !== 'ALL' ? false : optRace,
    requiredTalents: reqTalents,
    forcedRace,
    forcedRotation,
    forcedPetMode,
    mutationRate: mutRate,
    initialExplorationRate: initExplore,
    minExplorationRate: minExplore
  };
}

export function updateGALiveView({ gen, maxGens, bestDps, avgDps, elites, progress, status, evolutionHistory, uniqueConfigsCount, totalEvaluations, totalSimulations }) {
  currentCandidates = elites || [];
  if (evolutionHistory) {
    currentEvolutionHistory = evolutionHistory;
  }
  if (currentCandidates.length > 0 && !selectedCandidate) {
    selectedCandidate = currentCandidates[0];
  }
  renderGALeaderboard();
  if (selectedCandidate) {
    const updated = currentCandidates.find(c => c.name === selectedCandidate.name) || currentCandidates[0];
    selectedCandidate = updated;
    renderSelectedCandidateDetails(updated);
  }

  updateGAStatsCards({
    uniqueConfigsCount,
    totalEvaluations,
    totalSimulations,
    bestDps,
    gen,
    maxGens,
    history: currentEvolutionHistory
  });

  renderGAGraph(currentEvolutionHistory);
}

export function setGAExecutionResults({ candidates, evolutionHistory, bestCandidate, uniqueConfigsCount, totalEvaluations, totalSimulations }) {
  currentCandidates = candidates || [];
  currentEvolutionHistory = evolutionHistory || [];
  if (currentCandidates.length > 0) {
    selectedCandidate = currentCandidates[0];
  }
  renderGALeaderboard();
  if (selectedCandidate) {
    renderSelectedCandidateDetails(selectedCandidate);
  }

  const bestDps = currentCandidates.length > 0 ? currentCandidates[0].mean_dps : (currentEvolutionHistory.length > 0 ? currentEvolutionHistory[currentEvolutionHistory.length - 1].bestDps : 0);
  const maxGen = currentEvolutionHistory.length > 0 ? currentEvolutionHistory[currentEvolutionHistory.length - 1].gen : 15;

  updateGAStatsCards({
    uniqueConfigsCount: uniqueConfigsCount || currentCandidates.length * 10,
    totalEvaluations: totalEvaluations || currentEvolutionHistory.length * 1000,
    totalSimulations: totalSimulations || (currentEvolutionHistory.length * 1000 * 100),
    bestDps,
    gen: maxGen,
    maxGens: maxGen,
    history: currentEvolutionHistory
  });

  renderGAGraph(currentEvolutionHistory);
}

export function updateGAStatsCards({ uniqueConfigsCount, totalEvaluations, totalSimulations, bestDps, gen, maxGens, history }) {
  const elUnique = document.getElementById('ga-stat-unique-configs');
  const elSims = document.getElementById('ga-stat-total-fights');
  const elBest = document.getElementById('ga-stat-best-dps');
  const elGain = document.getElementById('ga-stat-gain');
  const elGens = document.getElementById('ga-stat-generations');

  if (elUnique && uniqueConfigsCount != null) {
    elUnique.textContent = uniqueConfigsCount.toLocaleString();
  }
  if (elSims && totalSimulations != null) {
    if (totalSimulations >= 1000000) {
      elSims.textContent = `${(totalSimulations / 1000000).toFixed(2)}M`;
    } else {
      elSims.textContent = totalSimulations.toLocaleString();
    }
  }
  if (elBest && bestDps != null) {
    elBest.textContent = `${bestDps.toFixed(1)} DPS`;
  }
  if (elGain && history && history.length > 0) {
    const baseDps = history[0]?.avgDps || 1;
    const curBest = bestDps || history[history.length - 1]?.bestDps || baseDps;
    const delta = curBest - baseDps;
    const deltaPct = (delta / baseDps) * 100;
    elGain.textContent = delta >= 0 ? `+${delta.toFixed(1)} (${deltaPct >= 0 ? '+' : ''}${deltaPct.toFixed(1)}%) vs Gen 0` : `${delta.toFixed(1)} vs Gen 0`;
    elGain.style.color = delta >= 0 ? '#4ade80' : '#f87171';
  }
  if (elGens && gen != null) {
    elGens.textContent = `Gen ${gen} / ${maxGens || gen}`;
  }
}

export function renderGAGraph(history = []) {
  const canvas = document.getElementById('ga-evolution-chart');
  const placeholder = document.getElementById('ga-chart-empty-placeholder');
  if (!canvas) return;

  if (!history || history.length === 0) {
    if (placeholder) placeholder.style.display = 'flex';
    const ctx = canvas.getContext('2d');
    if (ctx) ctx.clearRect(0, 0, canvas.width, canvas.height);
    return;
  }

  if (placeholder) placeholder.style.display = 'none';

  const container = canvas.parentElement;
  const dpr = window.devicePixelRatio || 1;
  const w = container ? container.clientWidth : 400;
  const h = container ? Math.max(220, container.clientHeight) : 220;

  canvas.width = Math.round(w * dpr);
  canvas.height = Math.round(h * dpr);
  canvas.style.width = `${w}px`;
  canvas.style.height = `${h}px`;

  const ctx = canvas.getContext('2d');
  if (!ctx) return;
  ctx.resetTransform?.();
  ctx.scale(dpr, dpr);
  ctx.clearRect(0, 0, w, h);

  const padding = { top: 25, right: 35, bottom: 30, left: 52 };
  const chartW = Math.max(10, w - padding.left - padding.right);
  const chartH = Math.max(10, h - padding.top - padding.bottom);

  // Compute min and max DPS
  let minVal = Infinity, maxVal = -Infinity;
  for (const item of history) {
    if (item.avgDps < minVal) minVal = item.avgDps;
    if (item.bestDps < minVal) minVal = item.bestDps;
    if (item.avgDps > maxVal) maxVal = item.avgDps;
    if (item.bestDps > maxVal) maxVal = item.bestDps;
  }
  if (!Number.isFinite(minVal)) minVal = 0;
  if (!Number.isFinite(maxVal) || maxVal <= minVal) maxVal = minVal + 100;

  const yMargin = Math.max(20, (maxVal - minVal) * 0.15);
  const yMin = Math.max(0, Math.floor((minVal - yMargin) / 25) * 25);
  const yMax = Math.ceil((maxVal + yMargin) / 25) * 25;
  const ySpan = Math.max(1, yMax - yMin);

  // Draw Horizontal Gridlines & Y-Axis Labels
  const gridCount = 4;
  ctx.lineWidth = 1;
  ctx.font = '10px ArialNarrow, ui-monospace, monospace';
  for (let i = 0; i <= gridCount; i++) {
    const val = yMin + (ySpan / gridCount) * i;
    const yPos = padding.top + chartH - (i / gridCount) * chartH;

    ctx.strokeStyle = 'rgba(255, 255, 255, 0.07)';
    ctx.beginPath();
    ctx.moveTo(padding.left, yPos);
    ctx.lineTo(w - padding.right, yPos);
    ctx.stroke();

    ctx.fillStyle = 'rgba(255, 255, 255, 0.5)';
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    ctx.fillText(`${Math.round(val)}`, padding.left - 8, yPos);
  }

  // Draw X-Axis Ticks & Labels
  const numPoints = history.length;
  const getX = (idx) => numPoints === 1 ? padding.left + chartW / 2 : padding.left + (idx / (numPoints - 1)) * chartW;
  const getY = (val) => padding.top + chartH - ((val - yMin) / ySpan) * chartH;

  ctx.fillStyle = 'rgba(255, 255, 255, 0.5)';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'top';

  const stepX = Math.max(1, Math.ceil(numPoints / 8));
  for (let i = 0; i < numPoints; i += stepX) {
    const x = getX(i);
    const genNum = history[i].gen;
    ctx.fillText(`G${genNum}`, x, padding.top + chartH + 6);
  }
  if ((numPoints - 1) % stepX !== 0) {
    const lastX = getX(numPoints - 1);
    ctx.fillText(`G${history[numPoints - 1].gen}`, lastX, padding.top + chartH + 6);
  }

  // Draw Mean DPS Area & Line (#38bdf8)
  if (numPoints > 1) {
    // Area Fill under Mean
    const meanGrad = ctx.createLinearGradient(0, padding.top, 0, padding.top + chartH);
    meanGrad.addColorStop(0, 'rgba(56, 189, 248, 0.15)');
    meanGrad.addColorStop(1, 'rgba(56, 189, 248, 0.01)');
    ctx.fillStyle = meanGrad;
    ctx.beginPath();
    ctx.moveTo(getX(0), padding.top + chartH);
    for (let i = 0; i < numPoints; i++) {
      ctx.lineTo(getX(i), getY(history[i].avgDps));
    }
    ctx.lineTo(getX(numPoints - 1), padding.top + chartH);
    ctx.closePath();
    ctx.fill();

    // Mean Line
    ctx.strokeStyle = '#38bdf8';
    ctx.lineWidth = 2;
    ctx.beginPath();
    for (let i = 0; i < numPoints; i++) {
      const x = getX(i);
      const y = getY(history[i].avgDps);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  // Draw Best DPS Area & Line (#4ade80)
  if (numPoints > 1) {
    // Area Fill under Best
    const bestGrad = ctx.createLinearGradient(0, padding.top, 0, padding.top + chartH);
    bestGrad.addColorStop(0, 'rgba(74, 222, 128, 0.20)');
    bestGrad.addColorStop(1, 'rgba(74, 222, 128, 0.02)');
    ctx.fillStyle = bestGrad;
    ctx.beginPath();
    ctx.moveTo(getX(0), padding.top + chartH);
    for (let i = 0; i < numPoints; i++) {
      ctx.lineTo(getX(i), getY(history[i].bestDps));
    }
    ctx.lineTo(getX(numPoints - 1), padding.top + chartH);
    ctx.closePath();
    ctx.fill();

    // Best Line
    ctx.strokeStyle = '#4ade80';
    ctx.lineWidth = 2.5;
    ctx.beginPath();
    for (let i = 0; i < numPoints; i++) {
      const x = getX(i);
      const y = getY(history[i].bestDps);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  // Draw Point Markers
  for (let i = 0; i < numPoints; i++) {
    const x = getX(i);
    const yMean = getY(history[i].avgDps);
    const yBest = getY(history[i].bestDps);

    // Mean point
    ctx.fillStyle = '#38bdf8';
    ctx.beginPath();
    ctx.arc(x, yMean, 3, 0, Math.PI * 2);
    ctx.fill();

    // Best point
    ctx.fillStyle = '#4ade80';
    ctx.beginPath();
    ctx.arc(x, yBest, 3.5, 0, Math.PI * 2);
    ctx.fill();
  }

  // Callout Badges on latest points
  if (numPoints > 0) {
    const last = history[numPoints - 1];
    const lastX = getX(numPoints - 1);
    const lastYBest = getY(last.bestDps);
    const lastYMean = getY(last.avgDps);

    ctx.font = 'bold 9px ArialNarrow, sans-serif';
    ctx.fillStyle = '#4ade80';
    ctx.textAlign = 'right';
    ctx.fillText(`${last.bestDps.toFixed(1)}`, Math.min(w - 5, lastX + 2), lastYBest - 6);

    ctx.fillStyle = '#38bdf8';
    ctx.fillText(`${last.avgDps.toFixed(1)}`, Math.min(w - 5, lastX + 2), Math.max(lastYMean + 10, lastYBest + 12));
  }
}

export function renderGALeaderboard() {
  const tbody = document.getElementById('ga-leaderboard-body');
  if (!tbody) return;

  if (!currentCandidates || currentCandidates.length === 0) {
    tbody.innerHTML = `<tr><td colspan="7" style="text-align: center; color: var(--text-dim); padding: 2rem;">Ready to search. Set constraints and click "Run AI Genetic Optimization".</td></tr>`;
    return;
  }

  const showPct = document.getElementById('ga-chk-pct-leader')?.checked;
  const showSd = document.getElementById('ga-chk-show-sd')?.checked;
  const leaderDps = currentCandidates[0]?.mean_dps || 1;

  tbody.innerHTML = '';

  currentCandidates.forEach((cand, idx) => {
    const tr = document.createElement('tr');
    tr.className = (selectedCandidate && selectedCandidate.name === cand.name) ? 'selected-row active' : '';

    // 1. Rank
    const tdRank = document.createElement('td');
    tdRank.style.textAlign = 'center';
    tdRank.style.fontWeight = 'bold';
    if (cand.rank === 1) tdRank.innerHTML = '<span style="color: #ffd100;">#1</span>';
    else if (cand.rank === 2) tdRank.innerHTML = '<span style="color: #e2e8f0;">#2</span>';
    else if (cand.rank === 3) tdRank.innerHTML = '<span style="color: #f97316;">#3</span>';
    else tdRank.innerHTML = `<span style="color: #94a3b8;">#${cand.rank}</span>`;
    tr.appendChild(tdRank);

    // 2. Spec Name & Archetype
    const tdName = document.createElement('td');
    tdName.innerHTML = `
      <div style="display: flex; flex-direction: column;">
        <span class="spec-name-link" style="color: var(--text-gold); font-weight: 700; cursor: pointer; font-size: 0.85rem;">${cand.name}</span>
        <span style="font-size: 0.72rem; color: var(--text-dim);">${cand.category || 'Hybrid Peak'}</span>
      </div>
    `;
    tdName.querySelector('.spec-name-link')?.addEventListener('click', (e) => {
      e.stopPropagation();
      applyCandidate(cand);
    });
    tr.appendChild(tdName);

    // 3. Race
    const tdRace = document.createElement('td');
    tdRace.style.textAlign = 'center';
    const raceKey = cand.race || 'HUMAN';
    const raceIcon = RACE_ICONS[raceKey] || 'Achievement_Character_Human_Male.png';
    tdRace.innerHTML = `<img src="./assets/icons/${raceIcon}" width="20" height="20" alt="${raceKey}" title="${raceKey}" style="border-radius:2px; border:1px solid #4a3e2e; display:block; margin:0 auto;">`;
    tr.appendChild(tdRace);

    // 4. Pet / Sac
    const tdPet = document.createElement('td');
    tdPet.style.textAlign = 'center';
    const petIcon = cand.pet === 'none' ? null : cand.pet === 'imp' ? 'Spell_Shadow_SummonImp.png' : 'Spell_Shadow_SummonSuccubus.png';
    let sacTxt = cand.sacImp ? 'Sac Imp' : cand.sacSuccubus ? 'Sac Succubus' : 'No Sac';
    tdPet.innerHTML = `
      <div style="display:inline-flex; align-items:center; justify-content:center; gap:3px;">
        ${petIcon ? `<img src="./assets/icons/${petIcon}" width="18" height="18" alt="${cand.pet}" title="Active Pet: ${cand.pet}" style="border-radius:2px;">` : '<span class="empty-slot-icon" title="No active pet"></span>'}
        ${cand.sacImp || cand.sacSuccubus ? `<img src="./assets/icons/${cand.sacImp ? 'Spell_Shadow_SummonImp.png' : 'Spell_Shadow_SummonSuccubus.png'}" width="18" height="18" alt="Sac" title="${sacTxt}" style="border-radius:2px; border:1px solid #ef4444;">` : ''}
      </div>
    `;
    tr.appendChild(tdPet);

    // 5. Damage Split
    const tdSplit = document.createElement('td');
    const sPct = Math.round(cand.shadow_pct || 0);
    const fPct = Math.round(cand.fire_pct || 0);
    const pPct = Math.round(cand.pet_pct || 0);
    tdSplit.innerHTML = `
      <div class="damage-split-bar" style="height: 14px;" title="Shadow: ${sPct}% | Fire: ${fPct}% | Pet: ${pPct}%">
        ${sPct > 0 ? `<div class="split-seg shadow" style="width: ${sPct}%;"></div>` : ''}
        ${fPct > 0 ? `<div class="split-seg fire" style="width: ${fPct}%;"></div>` : ''}
        ${pPct > 0 ? `<div class="split-seg pet" style="width: ${pPct}%;"></div>` : ''}
      </div>
    `;
    tr.appendChild(tdSplit);

    // 6. Mean DPS
    const tdDps = document.createElement('td');
    tdDps.style.textAlign = 'right';
    tdDps.style.fontWeight = 'bold';
    if (showPct) {
      const diff = ((cand.mean_dps - leaderDps) / leaderDps) * 100;
      if (cand.rank === 1) {
        tdDps.innerHTML = '<span style="color: #ffd100; font-family: var(--font-mono);">= Leader</span>';
      } else {
        tdDps.innerHTML = `<span style="color: #f87171; font-family: var(--font-mono);">${diff.toFixed(2)}%</span>`;
      }
    } else {
      tdDps.innerHTML = `<span style="color: #4ade80; font-family: var(--font-mono);">${cand.mean_dps.toFixed(1)}</span>`;
    }
    if (showSd && cand.std_dev) {
      tdDps.innerHTML += `<br><small style="color: #94a3b8; font-weight: normal; font-family: var(--font-mono);">±${cand.std_dev.toFixed(1)}</small>`;
    }
    tr.appendChild(tdDps);

    // 7. Action Apply Button
    const tdAction = document.createElement('td');
    tdAction.style.textAlign = 'center';
    const applyBtn = document.createElement('button');
    applyBtn.type = 'button';
    applyBtn.className = 'wow-button wow-btn-small';
    applyBtn.textContent = 'Apply';
    applyBtn.style.padding = '2px 6px';
    applyBtn.style.fontSize = '0.72rem';
    applyBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      applyCandidate(cand);
    });
    tdAction.appendChild(applyBtn);
    tr.appendChild(tdAction);

    // Row selection on click
    tr.addEventListener('click', () => {
      selectedCandidate = cand;
      renderGALeaderboard();
      renderSelectedCandidateDetails(cand);
    });

    tbody.appendChild(tr);
  });
}

export function renderSelectedCandidateDetails(cand) {
  const container = document.getElementById('ga-selected-spec-details-panel');
  if (!container || !cand) return;

  const a = TalentGraph.countTreePoints(cand.talentsVector || [], 0);
  const d = TalentGraph.countTreePoints(cand.talentsVector || [], 1);
  const x = TalentGraph.countTreePoints(cand.talentsVector || [], 2);

  const meanDps = cand.mean_dps || 0;
  const minDps = cand.min_dps || Math.round(meanDps * 0.85);
  const maxDps = cand.max_dps || Math.round(meanDps * 1.15);
  const sd = cand.std_dev || 0;

  const gen0Mean = currentEvolutionHistory.length > 0 ? currentEvolutionHistory[0].avgDps : meanDps;
  const netGain = meanDps - gen0Mean;
  const netGainPct = (netGain / Math.max(1, gen0Mean)) * 100;

  container.innerHTML = `
    <!-- TOP HEADER -->
    <div style="display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #332a40; padding-bottom: 0.5rem; margin-bottom: 0.75rem;">
      <div style="display: flex; align-items: center; gap: 0.75rem;">
        <img src="./assets/icons/${RACE_ICONS[cand.race] || 'Achievement_Character_Human_Male.png'}" width="32" height="32" alt="${cand.race}" style="border-radius:4px; border:1px solid #ffd100;">
        <div>
          <div style="font-size: 1.05rem; font-weight: 700; color: var(--text-gold);">${cand.name}</div>
          <div style="font-size: 0.75rem; color: var(--text-muted);">${cand.category} · ${a} Aff / ${d} Demo / ${x} Destro · ${cand.race}</div>
        </div>
      </div>
      <div style="display: flex; gap: 0.5rem;">
        <button type="button" class="wow-button" id="btn-ga-apply-active" style="min-width: 140px;">Apply to Current Build</button>
      </div>
    </div>

    <!-- 3 KPI CARDS -->
    <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 0.75rem; margin-bottom: 0.75rem;">
      <div class="metric-card" style="text-align: left; padding: 0.6rem 0.8rem;">
        <span style="font-size: 0.72rem; color: var(--text-muted); text-transform: uppercase;">1. Generation 0 Population Baseline</span>
        <strong style="font-size: 1.25rem; color: #cbd5e1; margin: 0.2rem 0;">${gen0Mean.toFixed(1)} DPS</strong>
        <small style="color: var(--text-dim);">Random legal population average</small>
      </div>

      <div class="metric-card major" style="text-align: left; padding: 0.6rem 0.8rem; border-color: #10b981;">
        <span style="font-size: 0.72rem; color: #6ee7b7; text-transform: uppercase;">2. Evolved Candidate Fitness</span>
        <strong style="font-size: 1.25rem; color: #4ade80; margin: 0.2rem 0;">${meanDps.toFixed(1)} DPS</strong>
        <small style="color: #6ee7b7;">±${sd.toFixed(1)} SD · [${minDps.toFixed(0)} - ${maxDps.toFixed(0)}]</small>
      </div>

      <div class="metric-card" style="text-align: left; padding: 0.6rem 0.8rem;">
        <span style="font-size: 0.72rem; color: var(--text-muted); text-transform: uppercase;">3. Net Realized Gain</span>
        <strong style="font-size: 1.25rem; color: ${netGain >= 0 ? '#4ade80' : '#f87171'}; margin: 0.2rem 0;">
          ${netGain >= 0 ? `+${netGain.toFixed(1)}` : netGain.toFixed(1)} DPS
        </strong>
        <small style="color: var(--text-dim);">${netGainPct >= 0 ? `+${netGainPct.toFixed(2)}%` : `${netGainPct.toFixed(2)}%`} vs Gen 0</small>
      </div>
    </div>

    <!-- TABS BAR FOR DEEP DIVE -->
    <div style="display: flex; gap: 0.5rem; border-bottom: 1px solid #332a40; margin-bottom: 0.75rem;">
      <button type="button" class="wow-button wow-btn-small active" id="btn-ga-tab-talents">Spec & Talents</button>
      <button type="button" class="wow-button wow-btn-small" id="btn-ga-tab-apl">Priority Chain</button>
      <button type="button" class="wow-button wow-btn-small" id="btn-ga-tab-spells">Spell Breakdown</button>
      <button type="button" class="wow-button wow-btn-small" id="btn-ga-tab-history">Evolution History (${currentEvolutionHistory.length} Gens)</button>
    </div>

    <!-- TAB CONTENTS -->
    <div id="ga-subtab-talents" class="ga-subtab-pane">
      <div style="font-size: 0.8rem; color: var(--text-parchment); margin-bottom: 0.5rem; font-weight: 700;">
        Talent Allocation Breakdown (${a} / ${d} / ${x}):
      </div>
      <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 0.75rem;">
        <!-- Affliction -->
        <div style="background: #0f0e14; border: 1px solid #332a40; border-radius: 4px; padding: 0.5rem;">
          <div style="color: #c084fc; font-weight: 700; font-size: 0.85rem; border-bottom: 1px solid #221c2e; padding-bottom: 0.25rem; margin-bottom: 0.4rem;">
            Affliction (${a} pts)
          </div>
          <div style="font-size: 0.75rem; color: #cbd5e1; line-height: 1.5;">
            ${renderTalentList(cand.talents?.affliction)}
          </div>
        </div>
        <!-- Demonology -->
        <div style="background: #0f0e14; border: 1px solid #332a40; border-radius: 4px; padding: 0.5rem;">
          <div style="color: #4ade80; font-weight: 700; font-size: 0.85rem; border-bottom: 1px solid #221c2e; padding-bottom: 0.25rem; margin-bottom: 0.4rem;">
            Demonology (${d} pts)
          </div>
          <div style="font-size: 0.75rem; color: #cbd5e1; line-height: 1.5;">
            ${renderTalentList(cand.talents?.demonology)}
          </div>
        </div>
        <!-- Destruction -->
        <div style="background: #0f0e14; border: 1px solid #332a40; border-radius: 4px; padding: 0.5rem;">
          <div style="color: #fb923c; font-weight: 700; font-size: 0.85rem; border-bottom: 1px solid #221c2e; padding-bottom: 0.25rem; margin-bottom: 0.4rem;">
            Destruction (${x} pts)
          </div>
          <div style="font-size: 0.75rem; color: #cbd5e1; line-height: 1.5;">
            ${renderTalentList(cand.talents?.destruction)}
          </div>
        </div>
      </div>
    </div>

    <div id="ga-subtab-apl" class="ga-subtab-pane" style="display: none;">
      <div class="table-scroll" style="max-height: 280px;">
        <table class="damage-table">
          <thead>
            <tr>
              <th style="width: 35px;">#</th>
              <th style="width: 160px;">Spell / Action</th>
              <th>Trigger Condition</th>
              <th style="width: 70px; text-align: center;">Status</th>
            </tr>
          </thead>
          <tbody>
            ${renderAPLRows(cand)}
          </tbody>
        </table>
      </div>
    </div>

    <div id="ga-subtab-spells" class="ga-subtab-pane" style="display: none;">
      <div class="table-scroll" style="max-height: 280px;">
        <table class="damage-table">
          <thead>
            <tr>
              <th style="width: 140px;">Spell</th>
              <th style="text-align: right; width: 100px;">Damage</th>
              <th style="text-align: right; width: 80px;">DPS</th>
              <th style="text-align: right; width: 60px;">Casts</th>
              <th style="text-align: right; width: 60px;">Crits</th>
              <th style="text-align: right; width: 60px;">Misses</th>
            </tr>
          </thead>
          <tbody>
            ${renderSpellBreakdownRows(cand)}
          </tbody>
        </table>
      </div>
    </div>

    <div id="ga-subtab-history" class="ga-subtab-pane" style="display: none;">
      <div class="table-scroll" style="max-height: 280px;">
        <table class="damage-table">
          <thead>
            <tr>
              <th style="width: 80px;">Generation</th>
              <th style="text-align: right; width: 140px;">Best Policy DPS</th>
              <th style="text-align: right; width: 140px;">Population Mean DPS</th>
              <th style="text-align: right;">Gain vs Baseline</th>
            </tr>
          </thead>
          <tbody>
            ${renderEvolutionHistoryRows()}
          </tbody>
        </table>
      </div>
    </div>
  `;

  // Hook apply button
  document.getElementById('btn-ga-apply-active')?.addEventListener('click', () => {
    applyCandidate(cand);
  });

  // Hook subtabs
  const subtabs = [
    { btn: 'btn-ga-tab-talents', pane: 'ga-subtab-talents' },
    { btn: 'btn-ga-tab-apl', pane: 'ga-subtab-apl' },
    { btn: 'btn-ga-tab-spells', pane: 'ga-subtab-spells' },
    { btn: 'btn-ga-tab-history', pane: 'ga-subtab-history' }
  ];

  subtabs.forEach(({ btn, pane }) => {
    document.getElementById(btn)?.addEventListener('click', () => {
      subtabs.forEach(s => {
        document.getElementById(s.btn)?.classList.toggle('active', s.btn === btn);
        const p = document.getElementById(s.pane);
        if (p) p.style.display = (s.btn === btn) ? 'block' : 'none';
      });
    });
  });
}

function renderTalentList(treeMap = {}) {
  const activeEntries = Object.entries(treeMap).filter(([, val]) => Number(val) > 0);
  if (activeEntries.length === 0) return '<em style="color: var(--text-dim);">0 points spent</em>';
  return activeEntries.map(([k, v]) => {
    const formattedName = k.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase());
    return `<div style="display:flex; justify-content:space-between; margin-bottom:2px;">
      <span>${formattedName}</span>
      <strong style="color: var(--text-gold);">${v}</strong>
    </div>`;
  }).join('');
}

function renderAPLRows(cand) {
  const { aplRules, shaderRotation } = getPolicyAPLAndActions(cand.individual || cand);
  if (aplRules.length === 0) return fallbackRow(shaderRotation);

  return aplRules.map((r, i) => {
    const spellName = getSpellNameForAPL(r.action);
    const iconKey = getIconForSpell(spellName);
    const condSummary = getConditionSummary(r);
    return `
      <tr>
        <td style="color: var(--text-dim);">#${i + 1}</td>
        <td>
          <div style="display: flex; align-items: center; gap: 6px;">
            <img src="./assets/icons/${SPELL_ICONS[iconKey] || 'Spell_Shadow_ShadowBolt.png'}" width="16" height="16" alt="${spellName}">
            <span style="color: #67e8f9;">${spellName}</span>
          </div>
        </td>
        <td style="color: var(--text-parchment);">${condSummary}</td>
        <td style="text-align: center; color: #4ade80;">ACTIVE</td>
      </tr>
    `;
  }).join('') + fallbackRow(shaderRotation);
}

function renderSpellBreakdownRows(cand) {
  const spells = (cand.summary?.spells || []).filter(sp => (sp.damage > 0 || sp.casts > 0));
  if (spells.length === 0) {
    return `<tr><td colspan="6" style="text-align: center; color: var(--text-dim);">No spell breakdown recorded.</td></tr>`;
  }
  const duration = cand.summary?.duration || 180;
  return spells.map(sp => {
    const dps = (sp.damage || 0) / duration;
    return `
      <tr>
        <td><strong>${sp.name}</strong></td>
        <td style="text-align: right; color: var(--text-gold);">${Math.round(sp.damage || 0).toLocaleString()}</td>
        <td style="text-align: right; color: #4ade80;">${dps.toFixed(1)}</td>
        <td style="text-align: right;">${sp.casts > 0 ? (sp.casts || 0).toFixed(1) : '-'}</td>
        <td style="text-align: right;">${sp.casts > 0 ? (sp.crits || 0).toFixed(1) : '-'}</td>
        <td style="text-align: right;">${sp.casts > 0 ? (sp.misses || 0).toFixed(1) : '-'}</td>
      </tr>
    `;
  }).join('');
}

function renderEvolutionHistoryRows() {
  if (currentEvolutionHistory.length === 0) {
    return `<tr><td colspan="4" style="text-align: center; color: var(--text-dim);">No evolution history yet.</td></tr>`;
  }
  const baseDps = currentEvolutionHistory[0]?.avgDps || 1;
  return currentEvolutionHistory.map(h => {
    const delta = h.bestDps - baseDps;
    const deltaPct = (delta / baseDps) * 100;
    return `
      <tr>
        <td>Gen #${h.gen}</td>
        <td style="text-align: right; color: #4ade80; font-weight: bold;">${h.bestDps.toFixed(1)} DPS</td>
        <td style="text-align: right; color: #67e8f9;">${h.avgDps.toFixed(1)} DPS</td>
        <td style="text-align: right; color: ${delta >= 0 ? '#4ade80' : '#f87171'};">
          ${delta >= 0 ? `+${delta.toFixed(1)} DPS (+${deltaPct.toFixed(2)}%)` : `${delta.toFixed(1)} DPS (${deltaPct.toFixed(2)}%)`}
        </td>
      </tr>
    `;
  }).join('');
}

function getSpellNameForAPL(action) {
  const map = {
    1: 'Life Tap',
    2: 'Nightfall Shadow Bolt',
    3: 'Decimation Searing Pain',
    4: 'Decimation Soul Fire',
    5: 'Demonic Brand Searing Pain',
    6: 'Corruption',
    7: 'Bane of Doom',
    8: 'Bane of Agony',
    9: 'Immolate',
    10: 'Conflagrate',
    11: 'Shadowburn',
    12: 'Incinerate',
    13: 'Searing Pain',
    14: 'Drain Soul',
    15: 'Drain Life',
    16: 'Shadow Bolt',
    17: 'Siphon Life',
    18: 'Drain Hope / Wrack'
  };
  return map[action] || 'Action';
}

function getIconForSpell(name) {
  const clean = name.toUpperCase().replace(/[\s\/-]/g, '_');
  if (clean.includes('LIFE_TAP')) return 'LIFE_TAP';
  if (clean.includes('NIGHTFALL')) return 'NIGHTFALL';
  if (clean.includes('SOUL_FIRE')) return 'DECIMATION_SOUL_FIRE';
  if (clean.includes('CORRUPTION')) return 'CORRUPTION';
  if (clean.includes('DOOM')) return 'BANE_OF_DOOM';
  if (clean.includes('AGONY')) return 'BANE_OF_AGONY';
  if (clean.includes('IMMOLATE')) return 'IMMOLATE';
  if (clean.includes('CONFLAGRATE')) return 'CONFLAGRATE';
  if (clean.includes('SHADOWBURN')) return 'SHADOWBURN';
  if (clean.includes('INCINERATE')) return 'INCINERATE';
  if (clean.includes('SEARING')) return 'SEARING_PAIN';
  if (clean.includes('SIPHON')) return 'SIPHON_LIFE';
  if (clean.includes('WRACK') || clean.includes('DRAIN_HOPE')) return 'DRAIN_HOPE';
  return 'SHADOW_BOLT';
}

function getConditionSummary(rule) {
  if (rule.action === 1) return `Emergency / Maintenance Tap (Mana <= ${rule.param || 25}%)`;
  if (rule.action === 2) return 'Shadow Trance proc active (Instant Cast)';
  if (rule.action === 4) return 'Decimation execute active (Boss HP <= 35%)';
  if (rule.action === 5) return 'Demonic Brand debuff missing on target';
  if (rule.action === 7) return 'Fight Time Remaining >= 60s';
  if (rule.action === 9) return 'Immolate debuff missing or <= 2.5s';
  if (rule.action === 6) return 'Corruption debuff missing or <= 2.5s';
  if (rule.action === 8) return 'Bane of Agony missing & Doom inactive';
  if (rule.action === 10) return 'Immolate active on target & CD ready';
  if (rule.action === 11) return 'Shadowburn CD ready (Finisher)';
  return 'Rotational Fallback Filler Cast';
}

function exportGASpecs() {
  if (!currentCandidates || currentCandidates.length === 0) {
    alert('No evolved spec candidates to export.');
    return;
  }
  const blob = new Blob([JSON.stringify({
    timestamp: new Date().toISOString(),
    engine: 'WebGL2 Genetic Algorithm Spec Optimizer',
    candidates: currentCandidates,
    evolutionHistory: currentEvolutionHistory
  }, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `warlock_genetic_specs_${Date.now()}.json`;
  a.click();
  URL.revokeObjectURL(url);
}
