import { candidateFallbackRotation, fallbackRow, fallbackSummary } from './apl_fallback_view.js';
// APL Synthesis Dashboard & View Controller for Three.js WebGL DES
// Layout and interactive flow matching Constrained Spec Search

import {
  APL_SYNTHESIS_ACTIONS,
  TOTAL_APL_RULES,
  CONDITION_TYPES,
  ACTION_VALID_CONDITIONS,
  createRule,
  formatAPLName
} from './apl_genetic_optimizer.js';
import { showTooltip, hideTooltip } from './tooltips.js';
import { getEnabledAPLRules, getAPLUnreachableFlags } from './apl_rules.js';
import { SPELLS } from './model.js';

let currentAPLCandidates = [];
let selectedAPLCandidate = null;
let currentAPLEvolutionHistory = [];
let onApplyAPLCallback = null;

export function initAPLSynthesisView(onApplyAPL) {
  onApplyAPLCallback = onApplyAPL;

  const spellSelection = document.getElementById('apl-ga-spell-selection');
  if (spellSelection) {
    spellSelection.replaceChildren(...APL_SYNTHESIS_ACTIONS.map(action => {
      const label = document.createElement('label');
      label.className = 'wow-checkbox-label';
      const checkbox = document.createElement('input');
      checkbox.type = 'checkbox';
      checkbox.checked = true;
      checkbox.dataset.actionId = action.id;
      label.append(checkbox, document.createTextNode(` ${action.spell}`));
      return label;
    }));
  }

  // Advanced tuning toggle
  const tuningChk = document.getElementById('apl-ga-advanced-tuning-chk');
  const tuningBox = document.getElementById('apl-ga-advanced-tuning-box');
  if (tuningChk && tuningBox) {
    tuningChk.addEventListener('change', () => {
      tuningBox.style.display = tuningChk.checked ? 'block' : 'none';
    });
  }

  // Percentage from Leader / Std Dev checkboxes
  ['apl-ga-chk-pct-leader', 'apl-ga-chk-show-sd'].forEach(id => {
    document.getElementById(id)?.addEventListener('change', () => {
      renderAPLGALeaderboard();
      if (selectedAPLCandidate) renderSelectedAPLCandidateDetails(selectedAPLCandidate);
    });
  });

  // Window resize for convergence canvas
  window.addEventListener('resize', () => {
    if (currentAPLEvolutionHistory.length > 0) {
      renderAPLGAGraph(currentAPLEvolutionHistory);
    }
  });

  // Tab switch listener
  document.getElementById('btn-apl-synthesis')?.addEventListener('click', () => {
    setTimeout(() => {
      if (currentAPLEvolutionHistory.length > 0) {
        renderAPLGAGraph(currentAPLEvolutionHistory);
      }
    }, 50);
  });
}

export function readAPLGAConfig() {
  const getNum = (id, fallback) => {
    const val = Number(document.getElementById(id)?.value);
    return Number.isFinite(val) ? val : fallback;
  };

  const getBool = (id, fallback) => {
    const el = document.getElementById(id);
    return el ? el.checked : fallback;
  };

  return {
    selectedActionIds: document.getElementById('apl-ga-spell-selection')
      ? Array.from(document.querySelectorAll('#apl-ga-spell-selection input:checked'), el => el.dataset.actionId)
      : undefined,
    generations: getNum('apl-ga-generations', 15),
    populationSize: getNum('apl-ga-pop-size', 500),
    screeningSims: getNum('apl-ga-screening-sims', 100),
    finalSims: getNum('apl-ga-final-sims', 2000),
    lockConditions: getBool('apl-ga-lock-conditions', true),
    seedPresets: getBool('apl-ga-seed-presets', true),
    swapRate: getNum('apl-ga-swap-rate', 0.40),
    condMutRate: getNum('apl-ga-cond-rate', 0.35),
    jitterRate: getNum('apl-ga-jitter-rate', 0.45),
    toggleNeverRate: getNum('apl-ga-never-rate', 0.20),
    seed: 0x41504C53 + Math.floor(Math.random() * 100000)
  };
}

export function updateAPLGALiveView(genData) {
  if (!genData) return;
  const { gen, maxGens, bestDps, avgDps, elites, evolutionHistory, uniqueConfigsCount, totalEvaluations, totalSimulations } = genData;

  currentAPLEvolutionHistory = evolutionHistory || [];
  currentAPLCandidates = elites || [];

  // Update Metric Cards
  const uniqueEl = document.getElementById('apl-ga-stat-unique-configs');
  if (uniqueEl) uniqueEl.textContent = (uniqueConfigsCount || totalEvaluations || 0).toLocaleString();

  const totalSimsEl = document.getElementById('apl-ga-stat-total-fights');
  if (totalSimsEl) totalSimsEl.textContent = (totalSimulations || 0).toLocaleString();

  const bestDpsEl = document.getElementById('apl-ga-stat-best-dps');
  if (bestDpsEl) bestDpsEl.textContent = bestDps ? `${bestDps.toFixed(1)} DPS` : '—';

  const genEl = document.getElementById('apl-ga-stat-generations');
  if (genEl) genEl.textContent = `${gen} / ${maxGens}`;

  const gainEl = document.getElementById('apl-ga-stat-gain');
  if (gainEl && evolutionHistory && evolutionHistory.length > 0) {
    const baseline = evolutionHistory[0].avgDps || evolutionHistory[0].bestDps;
    const gain = bestDps - baseline;
    const pct = baseline > 0 ? (gain / baseline) * 100 : 0;
    gainEl.textContent = gain >= 0 ? `+${gain.toFixed(1)} (+${pct.toFixed(1)}%) vs init` : `${gain.toFixed(1)} DPS`;
  }

  // Render Leaderboard & Graph
  renderAPLGALeaderboard();
  renderAPLGAGraph(currentAPLEvolutionHistory);

  // Auto-select rank 1 candidate initially if none selected
  if (!selectedAPLCandidate && currentAPLCandidates.length > 0) {
    selectAPLCandidate(currentAPLCandidates[0]);
  } else if (selectedAPLCandidate && currentAPLCandidates.length > 0) {
    const updated = currentAPLCandidates.find(c => c.rank === selectedAPLCandidate.rank) || currentAPLCandidates[0];
    selectAPLCandidate(updated, false);
  }
}

export function setAPLGAExecutionResults(results) {
  if (!results) return;
  currentAPLCandidates = results.candidates || [];
  currentAPLEvolutionHistory = results.evolutionHistory || [];

  const uniqueEl = document.getElementById('apl-ga-stat-unique-configs');
  if (uniqueEl) uniqueEl.textContent = (results.uniqueConfigsCount || results.totalEvaluations || 0).toLocaleString();

  const totalSimsEl = document.getElementById('apl-ga-stat-total-fights');
  if (totalSimsEl) totalSimsEl.textContent = (results.totalSimulations || 0).toLocaleString();

  if (results.bestCandidate) {
    const bestDpsEl = document.getElementById('apl-ga-stat-best-dps');
    if (bestDpsEl) bestDpsEl.textContent = `${results.bestCandidate.meanDps.toFixed(1)} DPS`;
  }

  renderAPLGALeaderboard();
  renderAPLGAGraph(currentAPLEvolutionHistory);

  if (currentAPLCandidates.length > 0) {
    selectAPLCandidate(currentAPLCandidates[0]);
  }
}

export function getEffectiveRules(rules = []) {
  return getEnabledAPLRules(rules);
}

export function renderAPLGALeaderboard() {
  const tbody = document.getElementById('apl-ga-leaderboard-body');
  if (!tbody) return;

  if (!currentAPLCandidates || currentAPLCandidates.length === 0) {
    tbody.innerHTML = '<tr><td colspan="6" style="text-align: center; color: var(--text-dim); padding: 2rem;">Ready to synthesize. Adjust settings and click "Run APL Genetic Synthesis".</td></tr>';
    return;
  }

  const leaderDps = currentAPLCandidates[0]?.meanDps || 1;
  const isPctLeader = document.getElementById('apl-ga-chk-pct-leader')?.checked;
  const isShowSd = document.getElementById('apl-ga-chk-show-sd')?.checked;

  tbody.innerHTML = '';
  currentAPLCandidates.forEach((cand, idx) => {
    const tr = document.createElement('tr');
    tr.className = 'leaderboard-row';
    if (selectedAPLCandidate && selectedAPLCandidate.rank === cand.rank) {
      tr.classList.add('selected-row');
    }

    const rankDisplay = cand.rank === 1 ? '<span class="rank-badge gold">1</span>' :
      cand.rank === 2 ? '<span class="rank-badge silver">2</span>' :
      cand.rank === 3 ? '<span class="rank-badge bronze">3</span>' :
      `<span class="rank-badge">${cand.rank}</span>`;

    // Action priority chain preview (effective rules without truncation)
    const effectiveRules = getEffectiveRules(cand.rules);
    const chainIcons = effectiveRules.map(r => `
      <img src="./assets/icons/${r.id === 'eureka' ? getSpellIcon('Eureka!') : r.icon}" alt="${r.spell}" title="${r.spell}: ${r.condition}" class="prio-chain-icon" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
    `).join('');

    // Damage Split calculation
    const batch = cand.batch;
    let shadowPct = 0, firePct = 0, petPct = 0;
    if (batch && batch.spells) {
      const spells = batch.spells;
      const total = batch.summary?.mean || 1;
      const shadowDmg = (spells[0]?.dps || 0) + (spells[1]?.dps || 0) + (spells[2]?.dps || 0);
      const fireDmg = (spells[3]?.dps || 0) + (spells[4]?.dps || 0) + (spells[5]?.dps || 0) + (spells[SPELLS.indexOf('Hellfire')]?.dps || 0);
      const petDmg = batch.petDps || Math.max(0, total - shadowDmg - fireDmg);
      shadowPct = Math.min(100, Math.max(0, Math.round((shadowDmg / total) * 100)));
      firePct = Math.min(100, Math.max(0, Math.round((fireDmg / total) * 100)));
      petPct = Math.max(0, 100 - shadowPct - firePct);
    }

    // DPS metric formatting
    let dpsDisplay = `${cand.meanDps.toFixed(1)}`;
    if (isPctLeader && cand.rank > 1) {
      const diffPct = ((cand.meanDps - leaderDps) / leaderDps) * 100;
      dpsDisplay = `${cand.meanDps.toFixed(1)} <span style="font-size:0.75rem; color:#f87171;">(${diffPct.toFixed(1)}%)</span>`;
    } else if (isPctLeader && cand.rank === 1) {
      dpsDisplay = `${cand.meanDps.toFixed(1)} <span style="font-size:0.75rem; color:#4ade80;">(Leader)</span>`;
    }

    if (isShowSd && cand.stdDev > 0) {
      dpsDisplay += `<br><span style="font-size:0.68rem; color:var(--text-dim);">±${cand.stdDev.toFixed(1)}</span>`;
    }

    tr.innerHTML = `
      <td style="text-align: center;">${rankDisplay}</td>
      <td>
        <div style="font-weight: 700; color: var(--text-parchment); font-size: 0.82rem;">${escapeHtml(cand.name)}</div>
        <div class="priority-chain-preview" style="margin-top: 3px;">
          ${chainIcons}
          <span class="apl-fallback-preview">${fallbackSummary(candidateFallbackRotation(cand))}</span>
        </div>
      </td>
      <td style="text-align: center; color: var(--text-gold); font-weight: 700; font-size: 0.8rem;">
        ${effectiveRules.length} <span style="font-size:0.7rem; color:var(--text-dim);">/ ${cand.rules?.length || 16}</span>
      </td>
      <td>
        <div class="damage-split-bar" style="height: 14px; margin: 0;" title="${batch?.summary?.detailed === false ? 'Damage breakdown unavailable' : 'Damage split'}">
          <div class="split-seg shadow" style="width: ${shadowPct}%;" title="Shadow: ${shadowPct}%"></div>
          <div class="split-seg fire" style="width: ${firePct}%;" title="Fire: ${firePct}%"></div>
          <div class="split-seg pet" style="width: ${petPct}%;" title="Pet: ${petPct}%"></div>
        </div>
      </td>
      <td style="text-align: right; font-weight: 700; color: #4ade80; font-size: 0.85rem;">
        ${dpsDisplay}
      </td>
      <td style="text-align: center;">
        <button type="button" class="wow-button wow-btn-small btn-apply-apl-cand" style="padding: 2px 6px; font-size: 0.7rem;" data-rank="${cand.rank}" title="Apply this synthesized APL to live simulator">
          Apply
        </button>
      </td>
    `;

    tr.addEventListener('click', (e) => {
      if (e.target.closest('.btn-apply-apl-cand')) return;
      selectAPLCandidate(cand);
    });

    tr.querySelector('.btn-apply-apl-cand')?.addEventListener('click', (e) => {
      e.stopPropagation();
      applyCandidateAPL(cand);
    });

    tbody.appendChild(tr);
  });
}

export function selectAPLCandidate(cand, render = true) {
  selectedAPLCandidate = cand;
  document.querySelectorAll('#apl-ga-leaderboard-body tr').forEach(r => r.classList.remove('selected-row'));
  const rows = document.querySelectorAll('#apl-ga-leaderboard-body tr');
  if (cand && rows[cand.rank - 1]) {
    rows[cand.rank - 1].classList.add('selected-row');
  }
  if (render) {
    renderSelectedAPLCandidateDetails(cand);
  }
}

export function renderSelectedAPLCandidateDetails(cand) {
  const panel = document.getElementById('apl-ga-selected-spec-details-panel');
  if (!panel) return;

  if (!cand) {
    panel.innerHTML = '<div style="text-align: center; color: var(--text-dim); padding: 1.5rem;">Select an APL solution to inspect its full priority list, conditions, and damage breakdown.</div>';
    return;
  }

  const batch = cand.batch || {};
  const spells = batch.spells || [];
  const totalDps = cand.meanDps || 0;

  // Spell breakdown table rows
  let spellRows = '';
  if (spells.length > 0) {
    spellRows = spells.filter(s => s.casts > 0 || s.damage > 0).map(s => {
      const pct = totalDps > 0 ? ((s.dps / totalDps) * 100).toFixed(1) : '0.0';
      const critPct = s.hits > 0 ? ((s.crits / (s.hits + (s.misses || 0))) * 100).toFixed(1) : '0.0';
      return `
        <tr>
          <td>
            <div style="display:flex; align-items:center; gap:0.35rem;">
              <img src="./assets/icons/${getSpellIcon(s.name)}" width="16" height="16" style="border-radius:2px;" alt="${s.name}" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
              <span>${s.name}</span>
            </div>
          </td>
          <td style="text-align: right; color: #cbd5e1;">${Math.round(s.damage).toLocaleString()}</td>
          <td style="text-align: right; font-weight: 700; color: #4ade80;">${s.dps.toFixed(1)}</td>
          <td style="text-align: right; color: var(--text-muted);">${s.casts}</td>
          <td style="text-align: right; color: #facc15;">${critPct}%</td>
          <td style="text-align: right; color: var(--text-parchment);">${pct}%</td>
        </tr>
      `;
    }).join('');
  } else {
    spellRows = '<tr><td colspan="6" style="text-align:center; color:var(--text-dim);">No damage data recorded.</td></tr>';
  }

  // Show every enabled rule; availability is evaluated by the simulator.
  const effectiveRules = getEffectiveRules(cand.rules || []);
  const unreachableFlags = getAPLUnreachableFlags(effectiveRules);
  const rulesHtml = effectiveRules.map((r, idx) => {
    const isUnreachable = unreachableFlags[idx];
    const codeColor = isUnreachable ? '#64748b' : '#6ee7b7';
    const code2Color = isUnreachable ? '#64748b' : '#93c5fd';
    let statusBadge = '';
    if (r.condKey2 && r.condKey2 !== 'ALWAYS' && r.condition2 && r.condition2 !== 'Always') {
      statusBadge = `<code style="color: ${codeColor}; font-size: 0.75rem;">${escapeHtml(r.condition1)}</code> <span style="color: ${isUnreachable ? '#64748b' : 'var(--text-gold)'}; font-size: 0.7rem; margin: 0 3px; font-weight: 700;">&</span> <code style="color: ${code2Color}; font-size: 0.75rem;">${escapeHtml(r.condition2)}</code>`;
    } else {
      statusBadge = `<code style="color: ${codeColor}; font-size: 0.75rem;">${escapeHtml(r.condition1 || r.condition)}</code>`;
    }

    const rowStyle = isUnreachable
      ? 'background: #14121a; opacity: 0.45; filter: grayscale(85%);'
      : 'background: #14121a;';
    const prioColor = isUnreachable ? '#71717a' : 'var(--text-gold)';
    const nameColor = isUnreachable ? '#71717a' : 'var(--text-parchment)';
    const unreachableTag = isUnreachable ? '<span class="apl-unreachable-tag">Unreachable</span>' : '';
    const statusIcon = isUnreachable
      ? '<span style="font-size: 0.62rem; color: #71717a; font-weight: 700; text-transform: uppercase;">Unreachable</span>'
      : '<span style="font-size: 0.75rem; color: #4ade80;">●</span>';

    return `
      <tr class="apl-row ${isUnreachable ? 'unreachable-row' : ''}" style="${rowStyle}" ${isUnreachable ? 'title="Unreachable: This action will never execute because an earlier unconditional action always fires."' : ''}>
        <td style="width: 32px; text-align: center; color: ${prioColor}; font-weight: 700; font-size: 0.75rem;">#${idx + 1}</td>
        <td style="width: 175px;">
          <div class="apl-spell-cell">
            <img src="./assets/icons/${r.id === 'eureka' ? getSpellIcon('Eureka!') : r.icon}" alt="${r.spell}" class="apl-spell-icon" style="width:20px; height:20px;" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
            <span class="apl-spell-name" style="font-size: 0.8rem; font-weight: 700; color: ${nameColor};">${r.spell}</span>
            ${unreachableTag}
          </div>
        </td>
        <td>
          <div class="apl-condition-badge" style="padding: 2px 6px; ${isUnreachable ? 'border-color: #27272a; background: #100f16;' : ''}">
            ${statusBadge}
          </div>
        </td>
        <td style="width: 50px; text-align: center;">
          ${statusIcon}
        </td>
      </tr>
    `;
  }).join('');

  panel.innerHTML = `
    <div class="wow-panel-header" style="padding: 0.6rem 0.85rem; border-bottom: 1px solid #383446; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 0.5rem;">
      <div style="display: flex; align-items: center; gap: 0.75rem;">
        <span class="rank-badge ${cand.rank === 1 ? 'gold' : cand.rank === 2 ? 'silver' : cand.rank === 3 ? 'bronze' : ''}">${cand.rank}</span>
        <div>
          <div style="font-size: 0.95rem; font-weight: 700; color: var(--text-gold);">${escapeHtml(cand.name)}</div>
          <div style="font-size: 0.72rem; color: var(--text-dim);">Synthesized Genetic APL · ${effectiveRules.length} Executable Action Rules</div>
        </div>
      </div>
      <div style="display: flex; gap: 0.5rem; align-items: center;">
        <button type="button" class="wow-button wow-btn-big" id="btn-apply-selected-apl" style="color: #ffd100; min-width: 160px;">
          Apply to Current APL
        </button>
        <button type="button" class="wow-button" id="btn-export-selected-apl-json">
          Export JSON
        </button>
      </div>
    </div>

    <div style="display: grid; grid-template-columns: minmax(360px, 1.2fr) minmax(320px, 1fr); gap: 1rem; padding: 0.85rem;">
      <!-- Left: Synthesized Effective Action Priority List -->
      <div>
        <div style="font-size: 0.78rem; font-weight: 700; color: var(--text-parchment); margin-bottom: 0.4rem; display: flex; justify-content: space-between;">
          <span>Effective Action Priority List</span>
          <span style="font-size: 0.72rem; color: var(--text-dim);">${effectiveRules.length} active rules</span>
        </div>
        <div class="table-scroll" style="max-height: 400px; border: 1px solid #2f2a3a; border-radius: 4px;">
          <table class="leaderboard-table" style="font-size: 0.78rem;">
            <thead>
              <tr>
                <th style="width: 32px; text-align: center;">Prio</th>
                <th style="width: 175px;">Action</th>
                <th>Continuous / Discrete Condition</th>
                <th style="width: 50px; text-align: center;">State</th>
              </tr>
            </thead>
            <tbody>
              ${rulesHtml}
              ${fallbackRow(candidateFallbackRotation(cand))}
            </tbody>
          </table>
        </div>
      </div>

      <!-- Right: Damage Breakdown & Performance Summary -->
      <div>
        <div style="font-size: 0.78rem; font-weight: 700; color: var(--text-parchment); margin-bottom: 0.4rem;">
          Simulated Spell Damage Breakdown
        </div>
        <div class="table-scroll" style="max-height: 400px; border: 1px solid #2f2a3a; border-radius: 4px;">
          <table class="damage-table" style="font-size: 0.78rem;">
            <thead>
              <tr>
                <th>Spell</th>
                <th style="text-align: right;">Damage</th>
                <th style="text-align: right;">DPS</th>
                <th style="text-align: right;">Casts</th>
                <th style="text-align: right;">Crits</th>
                <th style="text-align: right;">Split</th>
              </tr>
            </thead>
            <tbody>
              ${spellRows}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  `;

  // Wire up Apply and Export buttons
  document.getElementById('btn-apply-selected-apl')?.addEventListener('click', () => {
    applyCandidateAPL(cand);
  });

  document.getElementById('btn-export-selected-apl-json')?.addEventListener('click', () => {
    const dataStr = 'data:text/json;charset=utf-8,' + encodeURIComponent(JSON.stringify(cand, null, 2));
    const a = document.createElement('a');
    a.setAttribute('href', dataStr);
    a.setAttribute('download', `apl_synthesis_rank_${cand.rank}.json`);
    document.body.appendChild(a);
    a.click();
    a.remove();
  });
}

function applyCandidateAPL(cand) {
  if (!cand) return;
  if (onApplyAPLCallback) {
    onApplyAPLCallback(cand);
  }
  const curTab = document.getElementById('btn-current-build');
  if (curTab) curTab.click();
}

// 2D Canvas Convergence Graph (DPS vs Generation)
export function renderAPLGAGraph(history) {
  const canvas = document.getElementById('apl-ga-evolution-chart');
  const placeholder = document.getElementById('apl-ga-chart-empty-placeholder');
  if (!canvas) return;

  if (!history || history.length === 0) {
    if (placeholder) placeholder.style.display = 'flex';
    canvas.style.display = 'none';
    return;
  }

  if (placeholder) placeholder.style.display = 'none';
  canvas.style.display = 'block';

  const wrapper = document.getElementById('apl-ga-chart-wrapper');
  const dpr = window.devicePixelRatio || 1;
  const width = (wrapper?.clientWidth || 400);
  const height = (wrapper?.clientHeight || 210);

  canvas.width = width * dpr;
  canvas.height = height * dpr;
  canvas.style.width = `${width}px`;
  canvas.style.height = `${height}px`;

  const ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);
  ctx.clearRect(0, 0, width, height);

  const padLeft = 45;
  const padRight = 15;
  const padTop = 15;
  const padBottom = 25;
  const chartW = width - padLeft - padRight;
  const chartH = height - padTop - padBottom;

  const maxGen = Math.max(1, history.length - 1);
  let minDps = Infinity, maxDps = -Infinity;
  history.forEach(h => {
    if (h.bestDps < minDps) minDps = h.bestDps;
    if (h.avgDps < minDps) minDps = h.avgDps;
    if (h.bestDps > maxDps) maxDps = h.bestDps;
    if (h.avgDps > maxDps) maxDps = h.avgDps;
  });

  const range = Math.max(20, maxDps - minDps);
  const yMin = Math.max(0, Math.floor(minDps - range * 0.10));
  const yMax = Math.ceil(maxDps + range * 0.10);

  const getX = (gen) => padLeft + (gen / maxGen) * chartW;
  const getY = (val) => padTop + chartH - ((val - yMin) / (yMax - yMin)) * chartH;

  // Draw Grid Lines & Y-Axis Labels
  ctx.strokeStyle = '#262232';
  ctx.lineWidth = 1;
  ctx.font = '10px ArialNarrow, sans-serif';
  ctx.fillStyle = '#787384';
  ctx.textAlign = 'right';

  const yTicks = 4;
  for (let i = 0; i <= yTicks; i++) {
    const val = yMin + (i / yTicks) * (yMax - yMin);
    const y = getY(val);
    ctx.beginPath();
    ctx.moveTo(padLeft, y);
    ctx.lineTo(padLeft + chartW, y);
    ctx.stroke();
    ctx.fillText(`${Math.round(val)}`, padLeft - 6, y + 3);
  }

  // X-Axis Labels (Generations)
  ctx.textAlign = 'center';
  ctx.fillText('Gen 0', padLeft, height - 6);
  if (maxGen > 0) {
    ctx.fillText(`Gen ${maxGen}`, padLeft + chartW, height - 6);
  }

  // Draw Population Mean curve (Cyan)
  ctx.beginPath();
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 2;
  history.forEach((h, i) => {
    const x = getX(h.gen ?? i);
    const y = getY(h.avgDps);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();

  // Draw Best DPS curve (Green / Emerald)
  ctx.beginPath();
  ctx.strokeStyle = '#4ade80';
  ctx.lineWidth = 2.5;
  history.forEach((h, i) => {
    const x = getX(h.gen ?? i);
    const y = getY(h.bestDps);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();

  // Highlight points
  history.forEach((h, i) => {
    const x = getX(h.gen ?? i);
    const yBest = getY(h.bestDps);
    ctx.fillStyle = '#4ade80';
    ctx.beginPath();
    ctx.arc(x, yBest, 3, 0, Math.PI * 2);
    ctx.fill();
  });
}

export function exportAPLGASpecs() {
  if (!currentAPLCandidates || currentAPLCandidates.length === 0) return;
  const payload = {
    exportedAt: new Date().toISOString(),
    totalDiscoveredElites: currentAPLCandidates.length,
    evolutionHistory: currentAPLEvolutionHistory,
    candidates: currentAPLCandidates
  };
  const dataStr = 'data:text/json;charset=utf-8,' + encodeURIComponent(JSON.stringify(payload, null, 2));
  const a = document.createElement('a');
  a.setAttribute('href', dataStr);
  a.setAttribute('download', `warlock_apl_synthesis_results_${Date.now()}.json`);
  document.body.appendChild(a);
  a.click();
  a.remove();
}

function getSpellIcon(spellName) {
  const name = String(spellName).toLowerCase();
  if (name.includes('eureka')) return 'Spell_Arcane_MindMastery.png';
  if (name.includes('shadow bolt')) return 'Spell_Shadow_ShadowBolt.png';
  if (name.includes('corruption')) return 'Spell_Shadow_AbominationExplosion.png';
  if (name.includes('immolate')) return 'Spell_Fire_Immolation.png';
  if (name.includes('searing')) return 'Spell_Fire_SoulBurn.png';
  if (name.includes('incinerate')) return 'Spell_Fire_Burnout.png';
  if (name.includes('conflagrate')) return 'Spell_Fire_Fireball.png';
  if (name.includes('shadowburn')) return 'Spell_Shadow_ScourgeBuild.png';
  if (name.includes('siphon')) return 'Spell_Shadow_Requiem.png';
  if (name.includes('drain')) return 'Spell_Shadow_Haunting.png';
  if (name.includes('bane of doom') || name.includes('curse of doom') || name.includes('doom')) return 'Spell_Shadow_AuraOfDarkness.png';
  if (name.includes('bane of agony') || name.includes('curse of agony') || name.includes('agony')) return 'Spell_Shadow_CurseOfSargeras.png';
  if (name.includes('soul fire')) return 'Spell_Fire_Fireball02.png';
  if (name.includes('brand')) return 'ability_demonhunter_chaoticimprint_fire.png';
  if (name.includes('hellfire')) return 'Spell_Fire_Incinerate.png';
  if (name.includes('wrack')) return 'ability_deathknight_hemorrhagicfever.png';
  if (name.includes('life tap') || name.includes('tap')) return 'Spell_Shadow_BurningSpirit.png';
  return 'Spell_Shadow_ShadowBolt.png';
}

function escapeHtml(str) {
  return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}
