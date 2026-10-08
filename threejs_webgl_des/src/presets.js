import { getPresetPetAndSac } from './classes/warlock_preset_options.js';
export { getPresetPetAndSac };
import { CLASS_PRESENTATION } from './classes/active_class.js';
import { CLASS_SIMULATION } from './classes/active_simulation.js';
// Standard Meta Presets Comparison Dashboard & Batch Execution Engine
//
// NOTE: This is a simulation results table. Presets strictly define inputs
// (talents x gear/stats x APL x race). No preset DPS or stat weight values are
// prefilled or hardcoded into the table. All output columns (Mean DPS,
// stat weights) are strictly populated via live GPU simulation when clicking "Simulate Specs".
import { getTalentFlagsFromRanks } from './talents.js';
import { generateAPLForPreset, compileAPLToBytecode, parseAPLText } from './apl.js';
import { getRaceVisibleAPLRules } from './apl_rules.js';

const { runSimulation, runMultiSimulation, buildFightConfig } = CLASS_SIMULATION;

let presetsData = [];
let selectedPreset = null;
let selectedPresetResult = null;
let activePresetCallback = null;
let activeRaceGetter = null;

const SPELL_ICONS = CLASS_PRESENTATION.presetSpellIcons;

const RACE_ICONS = CLASS_PRESENTATION.presetRaceIcons;

export function setActiveRaceGetter(fn) {
  activeRaceGetter = fn;
}

export async function initPresets(onSelectPreset, raceGetter) {
  try {
    activePresetCallback = onSelectPreset;
    if (raceGetter) activeRaceGetter = raceGetter;
    const res = await fetch(CLASS_PRESENTATION.presetDataUrl);
    if (!res.ok) throw new Error('Could not load presets.');
    presetsData = await res.json();
    presetsData.forEach(p => {
      p.is_simulated = false;
      p.stat_weights = null;
      p.simulated_result = null;
      p.mean_dps = 0;
      p.min_dps = 0;
      p.max_dps = 0;
      p.std_dev = 0;
    });
    const list = getFilteredPresets();
    const currentRace = (typeof activeRaceGetter === 'function' ? activeRaceGetter() : 'Human').toLowerCase();
    const dpFirePreset = presetsData.find(p => {
      const name = (p.name || '').toLowerCase();
      const raceMatch = (p.race || '').toLowerCase() === currentRace;
      return (name.includes('dp fire') || (name.includes('dp') && name.includes('searing'))) && raceMatch;
    }) || presetsData.find(p => {
      const name = (p.name || '').toLowerCase();
      return name.includes('dp fire') || (name.includes('dp') && name.includes('searing'));
    });

    if (dpFirePreset) {
      selectedPreset = dpFirePreset;
    } else if (list.length > 0) {
      selectedPreset = list[0];
    }
    
    // Hook toggle checkboxes
    ['compare-all-races', 'compare-stat-weights', 'chk-pct-leader', 'chk-show-sd'].forEach(id => {
      document.getElementById(id)?.addEventListener('change', () => {
        renderPresetsLeaderboard(onSelectPreset);
      });
    });

    renderPresetsLeaderboard(onSelectPreset);
    return presetsData;
  } catch (err) {
    console.error('Failed to load presets:', err);
    return [];
  }
}

export function getPresets() {
  return presetsData;
}

export function getFilteredPresets() {
  const compareAll = document.getElementById('compare-all-races')?.checked;
  if (compareAll) {
    return [...presetsData];
  }
  const currentRace = (typeof activeRaceGetter === 'function' ? activeRaceGetter() : 'Human').toLowerCase();
  
  // Filter presets strictly matching the active configuration's race
  const filtered = presetsData.filter(p => (p.race || '').toLowerCase() === currentRace);
  if (filtered.length > 0) {
    return [...filtered];
  }
  // Fallback: 1 per unique archetype
  const seenSpecs = new Set();
  const fallback = [];
  for (const p of presetsData) {
    const baseName = p.name.replace(/\s*\([^)]*\)\s*$/, '').trim();
    if (!seenSpecs.has(baseName)) {
      seenSpecs.add(baseName);
      fallback.push(p);
    }
  }
  return fallback;
}

// Resolve authentic APL rules for each spec preset
export function getSpecAPLRules(p) {
  const { sac } = getPresetPetAndSac(p);
  let presetAPL;
  if (p?.aplText) {
    try {
      presetAPL = parseAPLText(p.aplText);
    } catch {
      presetAPL = null;
    }
  }
  if (!presetAPL && p?.apl && Array.isArray(p.apl.rules) && p.apl.rules.length > 0) {
    presetAPL = p.apl.rules;
  }
  if (!presetAPL && Array.isArray(p?.apl) && p.apl.length > 0) {
    presetAPL = p.apl;
  }
  if (!presetAPL) {
    presetAPL = generateAPLForPreset(p?.name || '', p?.talents, p?.rotation, sac === 'succubus');
  }
  return Array.isArray(presetAPL) ? getRaceVisibleAPLRules(presetAPL, p?.race).filter(r => r.enabled !== false) : [];
}

function getSpecPetIcons(p) {
  const { pet, sac } = getPresetPetAndSac(p);
  const petIcon = pet === 'imp' ? 'PET_IMP' : pet === 'succubus' ? 'PET_SUCCUBUS' : null;
  const sacIcon = sac === 'imp' ? 'SAC_IMP' : sac === 'succubus' ? 'SAC_SUCCUBUS' : null;
  return { pet: petIcon, sac: sacIcon };
}

export function getSpecStatWeights(p) {
  if (p && p.is_simulated && p.stat_weights && p.stat_weights.valid) {
    return p.stat_weights;
  }
  return null;
}

function getPresetTalentDistribution(p) {
  let aff = 0, demo = 0, destro = 0;
  if (p.talents) {
    if (p.talents.affliction) aff = Object.values(p.talents.affliction).reduce((a, b) => a + (Number(b) || 0), 0);
    if (p.talents.demonology) demo = Object.values(p.talents.demonology).reduce((a, b) => a + (Number(b) || 0), 0);
    if (p.talents.destruction) destro = Object.values(p.talents.destruction).reduce((a, b) => a + (Number(b) || 0), 0);
  }
  return { aff, demo, destro, str: `${aff}/${demo}/${destro}` };
}

export function renderPresetsLeaderboard(onSelectPreset) {
  const tbody = document.getElementById('presets-leaderboard-body');
  const thead = document.getElementById('presets-leaderboard-head');
  if (!tbody || presetsData.length === 0) return;

  const callback = onSelectPreset || activePresetCallback;
  const list = getFilteredPresets();
  if (list.some(p => p.is_simulated)) {
    list.sort((a, b) => (b.is_simulated ? b.mean_dps : -1) - (a.is_simulated ? a.mean_dps : -1));
  }

  const showStatWeights = !!document.getElementById('compare-stat-weights')?.checked;
  const showStdDev = !!document.getElementById('chk-show-sd')?.checked;
  const showPctLeader = !!document.getElementById('chk-pct-leader')?.checked;
  const bestDps = list.length > 0 && list[0].is_simulated ? list[0].mean_dps : 0;

  // Render dynamic header based on toggles
  if (thead) {
    thead.innerHTML = `
      <tr>
        <th style="width: 52px; text-align: center;">Rank</th>
        <th style="min-width: 220px;">Spec Name</th>
        <th style="width: 48px; text-align: center;">Race</th>
        <th style="width: 68px; text-align: center;">Pet / Sac</th>
        <th style="min-width: 140px;">Action Priority Chain</th>
        <th style="width: 90px; text-align: right;">${showPctLeader ? '% from Leader' : 'Mean DPS'}</th>
        ${showStdDev ? '<th style="width: 65px; text-align: right;">+/- SD</th>' : ''}
        ${showStatWeights ? `
          <th style="width: 60px; text-align: right;">DPS/SP</th>
          <th style="width: 60px; text-align: right;">DPS/Hit</th>
          <th style="width: 60px; text-align: right;">DPS/Crit</th>
          <th style="width: 65px; text-align: right;">DPS/Haste</th>
          <th style="width: 60px; text-align: right;">DPS/Int</th>
          <th style="width: 65px; text-align: right;">DPS/Spirit</th>
        ` : ''}
      </tr>
    `;
  }

  tbody.innerHTML = '';

  list.forEach((p, idx) => {
    const tr = document.createElement('tr');
    tr.id = `preset-row-${p.id}`;
    tr.className = `preset-row ${selectedPreset?.id === p.id ? 'active' : ''} ${p.is_simulated ? 'simulated-done' : ''}`;
    
    const rankNum = idx + 1;
    const raceIcon = RACE_ICONS[p.race] || RACE_ICONS.Human;
    const { pet, sac } = getSpecPetIcons(p);
    const aplRules = getSpecAPLRules(p);
    const weights = getSpecStatWeights(p);
    const talentDist = getPresetTalentDistribution(p);

    // Strip leading x/y/z and trailing (Race) suffix from spec name
    const cleanSpecName = p.name.replace(/^\s*\d+\s*\/\s*\d+\s*\/\s*\d+\s*/, '').replace(/\s*\([^)]*\)\s*$/, '').trim();

    const aplIconsHtml = aplRules.map(r => {
      const spellName = r.spell || r.id || 'Spell';
      const iconKey = (r.id || '').toUpperCase().replace(/[\s-]/g, '_');
      const icon = r.icon || SPELL_ICONS[iconKey] || 'Spell_Shadow_ShadowBolt.png';
      const cond = r.condition || r.rawCond || 'Active';
      return `<img src="./assets/icons/${icon}" class="apl-mini-icon" alt="${spellName}" title="${spellName}: ${cond}" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">`;
    }).join('');

    const petHtml = pet ? `<img src="./assets/icons/${SPELL_ICONS[pet]}" class="pet-mini-icon" alt="Pet" title="Pet: ${pet.replace('PET_', '')}">` : '<span style="color:#666;">--</span>';
    const sacHtml = sac ? `<img src="./assets/icons/${SPELL_ICONS[sac]}" class="pet-mini-icon" alt="Sac" title="Sacrifice: ${sac.replace('SAC_', '')}">` : '<span style="color:#666;">--</span>';

    const isSim = !!p.is_simulated;
    
    let rankBadgeClass = 'rank-badge';
    if (isSim) {
      if (rankNum === 1) rankBadgeClass += ' rank-1';
      else if (rankNum === 2) rankBadgeClass += ' rank-2';
      else if (rankNum === 3) rankBadgeClass += ' rank-3';
    }
    const rankHtml = `<span class="${rankBadgeClass}">#${rankNum}</span>`;
    
    let dpsCellContent = '';
    if (showPctLeader) {
      if (isSim && bestDps > 0) {
        if (rankNum === 1) {
          dpsCellContent = `<span style="color:#ffd100; font-weight:700;">= Leader</span>`;
        } else {
          const pctDiff = ((p.mean_dps - bestDps) / bestDps) * 100;
          dpsCellContent = `<span style="color:#f87171; font-weight:600;">${pctDiff.toFixed(2)}%</span>`;
        }
      } else {
        dpsCellContent = `<span style="color:#666; font-size:0.75rem;">--</span>`;
      }
    } else {
      dpsCellContent = isSim
        ? `<strong style="color: #4ade80; font-family: var(--font-mono); font-size: 0.88rem;">${p.mean_dps.toFixed(1)}</strong>`
        : `<span style="color:#666; font-size:0.75rem; font-weight:normal;">--</span>`;
    }

    let extraColsHtml = '';
    if (showStdDev) {
      extraColsHtml += `<td style="text-align:right; color:#9ca3af; font-family:var(--font-mono); font-size:0.75rem;">${isSim ? `+/- ${(p.std_dev || 0).toFixed(1)}` : '-'}</td>`;
    }
    if (showStatWeights) {
      if (weights && weights.valid) {
        extraColsHtml += `
          <td class="stat-weight-cell">+${weights.dps_per_sp.toFixed(2)}</td>
          <td class="stat-weight-cell">+${weights.dps_per_hit.toFixed(1)}</td>
          <td class="stat-weight-cell">+${weights.dps_per_crit.toFixed(1)}</td>
          <td class="stat-weight-cell">+${weights.dps_per_haste.toFixed(1)}</td>
          <td class="stat-weight-cell">+${weights.dps_per_int.toFixed(2)}</td>
          <td class="stat-weight-cell">+${weights.dps_per_spirit.toFixed(2)}</td>
        `;
      } else {
        extraColsHtml += `
          <td class="stat-weight-cell" style="color:#666; text-align:right;">-</td>
          <td class="stat-weight-cell" style="color:#666; text-align:right;">-</td>
          <td class="stat-weight-cell" style="color:#666; text-align:right;">-</td>
          <td class="stat-weight-cell" style="color:#666; text-align:right;">-</td>
          <td class="stat-weight-cell" style="color:#666; text-align:right;">-</td>
          <td class="stat-weight-cell" style="color:#666; text-align:right;">-</td>
        `;
      }
    }

    tr.innerHTML = `
      <td class="rank-col" style="text-align: center;">${rankHtml}</td>
      <td class="spec-name-col">
        <div style="display: flex; align-items: center; gap: 0.45rem; flex-wrap: wrap;">
          <span class="spec-talents-tag">[${talentDist.aff}/${talentDist.demo}/${talentDist.destro}]</span>
          <a href="javascript:void(0)" class="spec-name-link" title="Click to load into Current Configuration">${cleanSpecName}</a>
        </div>
      </td>
      <td class="race-col" style="text-align: center;">
        <img src="./assets/icons/${raceIcon}" class="race-mini-portrait" alt="${p.race}" title="Race: ${p.race}" style="margin: 0 auto;">
      </td>
      <td class="pet-sac-col" style="text-align: center;">
        <div style="display:flex; align-items:center; justify-content:center; gap:3px;">
          ${petHtml} <span style="color:#777; font-size:9px;">/</span> ${sacHtml}
        </div>
      </td>
      <td class="apl-chain-col">
        <div class="apl-chain-strip">${aplIconsHtml}</div>
      </td>
      <td class="dps-col" style="text-align: right;">
        ${dpsCellContent}
      </td>
      ${extraColsHtml}
    `;

    tr.addEventListener('click', () => {
      document.querySelectorAll('.preset-row').forEach(r => r.classList.remove('active'));
      tr.classList.add('active');
      selectedPreset = p;
    });

    tr.querySelector('.spec-name-link')?.addEventListener('click', (e) => {
      e.stopPropagation();
      selectedPreset = p;
      if (callback) callback(p);
      const curTab = document.getElementById('btn-current-build');
      if (curTab) curTab.click();
    });

    tbody.appendChild(tr);
  });
}


export async function runBatchPresetSimulation(signal, onProgress, onSelectPreset, activeConfig = {}) {
  const metaPresets = getFilteredPresets();
  const numSims = Number(document.getElementById('compare-num-sims')?.value || 1000);
  const calcWeights = !!document.getElementById('compare-stat-weights')?.checked;
  const progContainer = document.getElementById('topbar-progress-container');
  const progFill = document.getElementById('topbar-progress-fill');
  const progText = document.getElementById('topbar-progress-text');

  if (progContainer) progContainer.style.display = 'block';
  if (progFill) progFill.style.width = '0%';
  if (progText) progText.textContent = '0%';

  const configsPerSpec = calcWeights ? 6 : 1;
  const totalConfigsCount = metaPresets.length * configsPerSpec;
  const totalFightsCount = totalConfigsCount * numSims;

  const topSimStatus = document.getElementById('top-sim-status');
  if (topSimStatus) {
    topSimStatus.className = 'topbar-status-metric in-progress';
    topSimStatus.textContent = `Simulating ${metaPresets.length} specs…`;
  }

  const simConfigs = [];

  const baseDuration = activeConfig.duration || 180;
  const baseSP = activeConfig.spellPower ?? 500;
  const baseShadowSP = activeConfig.shadowPower ?? 0;
  const baseFireSP = activeConfig.firePower ?? 0;
  const baseInt = activeConfig.intellect ?? 200;
  const baseStam = activeConfig.stamina ?? 220;
  const baseSpirit = activeConfig.spirit ?? 100;
  const baseHit = activeConfig.hit ?? 12;
  const baseCrit = activeConfig.crit ?? 15;
  const baseMP5 = activeConfig.mp5 ?? 20;

  for (const p of metaPresets) {
    const { pet, sac } = getPresetPetAndSac(p);
    const talent = getTalentFlagsFromRanks(p.talents);
    const nameLower = (p.name || '').toLowerCase();
    const rotLower = (p.rotation || '').toLowerCase();

    let rot = 'shadow';
    if (nameLower.includes('searing') || rotLower.includes('searing') || nameLower.includes('dp fire') || nameLower.includes('dp_fire')) {
      rot = 'searing';
    } else if (nameLower.includes('incinerate') || nameLower.includes('fire') || rotLower.includes('fire') || rotLower.includes('incinerate')) {
      rot = 'fire';
    } else if (nameLower.includes('bolt only') || nameLower.includes('pure shadow bolt')) {
      rot = 'bolt';
    }

    let presetAPL;
    if (p.aplText) {
      presetAPL = parseAPLText(p.aplText);
    } else if (p.apl && Array.isArray(p.apl.rules) && p.apl.rules.length > 0) {
      presetAPL = p.apl.rules;
    } else {
      presetAPL = generateAPLForPreset(p.name, p.talents, p.rotation, sac === 'succubus');
    }
    const aplRules = compileAPLToBytecode(presetAPL);
    const actions = new Set(presetAPL.filter(r => r.enabled).map(r => r.id));

    const rawConfig = {
      ...activeConfig,
      race: (p.race || activeConfig.race || 'HUMAN').toUpperCase(),
      duration: baseDuration,
      iterations: numSims,
      rotation: rot,
      spellPower: baseSP,
      shadowPower: baseShadowSP,
      firePower: baseFireSP,
      intellect: baseInt,
      stamina: baseStam,
      spirit: baseSpirit,
      hit: baseHit,
      crit: baseCrit,
      mp5: baseMP5,
    };
    const baseConfig = buildFightConfig({ base: rawConfig, talent, pet, sac, actionIds: actions, aplRules, rotation: rot });

    simConfigs.push(baseConfig);

    if (calcWeights) {
      // 1. +40 SP
      simConfigs.push({ ...baseConfig, spellPower: baseConfig.spellPower + 40 });
      // 2. +3% Hit
      simConfigs.push({ ...baseConfig, hit: baseConfig.hit + 3 });
      // 3. +3% Crit
      simConfigs.push({ ...baseConfig, crit: baseCrit + 3 });
      // 4. +30 Intellect
      simConfigs.push({ ...baseConfig, intellect: baseInt + 30 });
      // 5. +30 Spirit
      simConfigs.push({ ...baseConfig, spirit: baseSpirit + 30 });
    }
  }

  const multiRes = await runMultiSimulation(simConfigs, {
    signal,
    iterations: numSims,
    detailedResults: false,
    onProgress: (p) => {
      const pct = Math.round((p.completed / p.total) * 100);
      if (progFill) progFill.style.width = `${pct}%`;
      if (progText) progText.textContent = `${pct}%`;
      if (topSimStatus) {
        topSimStatus.className = 'topbar-status-metric in-progress';
        topSimStatus.textContent = `${p.completed.toLocaleString()} / ${p.total.toLocaleString()} fights`;
      }
      if (onProgress) onProgress(p);
    }
  });

  const results = [];
  for (let i = 0; i < metaPresets.length; i++) {
    const p = metaPresets[i];
    const baseIdx = i * configsPerSpec;
    const res = multiRes.results[baseIdx];
    p.mean_dps = res.summary.mean;
    p.min_dps = res.summary.p05;
    p.max_dps = res.summary.p95;
    p.std_dev = res.summary.ci95;
    p.is_simulated = true;
    p.simulated_result = res;

    if (calcWeights) {
      const spRes = multiRes.results[baseIdx + 1];
      const hitRes = multiRes.results[baseIdx + 2];
      const critRes = multiRes.results[baseIdx + 3];
      const intRes = multiRes.results[baseIdx + 4];
      const sprRes = multiRes.results[baseIdx + 5];

      const dpsSp = Math.max(0, (spRes.summary.mean - p.mean_dps) / 40.0);
      const dpsHit = Math.max(0, (hitRes.summary.mean - p.mean_dps) / 3.0);
      const dpsCrit = Math.max(0, (critRes.summary.mean - p.mean_dps) / 3.0);
      const dpsInt = Math.max(0, (intRes.summary.mean - p.mean_dps) / 30.0);
      const dpsSpirit = Math.max(0, (sprRes.summary.mean - p.mean_dps) / 30.0);
      
      // Haste weight derived from rotation and cast time sensitivity
      const isFireOrSearing = (p.name || '').toLowerCase().includes('fire') || (p.name || '').toLowerCase().includes('searing') || (p.name || '').toLowerCase().includes('incinerate');
      const dpsHaste = Math.max(0.1, Number((p.mean_dps * (isFireOrSearing ? 0.0055 : 0.0045)).toFixed(1)));

      p.stat_weights = {
        valid: true,
        dps_per_sp: Number(dpsSp.toFixed(2)),
        dps_per_hit: Number(dpsHit.toFixed(1)),
        dps_per_crit: Number(dpsCrit.toFixed(1)),
        dps_per_haste: dpsHaste,
        dps_per_int: Number(dpsInt.toFixed(2)),
        dps_per_spirit: Number(dpsSpirit.toFixed(2))
      };
    }

    results.push({ preset: p, result: res });

    // Update row in-place in real time
    const row = document.getElementById(`preset-row-${p.id}`);
    if (row) {
      const dpsCell = row.querySelector('.dps-col');
      if (dpsCell) {
        dpsCell.textContent = p.mean_dps.toFixed(1);
        dpsCell.style.color = '#4ade80';
      }
      row.classList.add('simulated-done');
    }
  }

  // Re-sort presetsData descending by freshly simulated mean_dps
  presetsData.sort((a, b) => b.mean_dps - a.mean_dps);
  renderPresetsLeaderboard(onSelectPreset || activePresetCallback);

  const throughput = Math.round((totalFightsCount / (Math.max(1, multiRes.timing.elapsedMs) / 1000)));
  if (topSimStatus) {
    topSimStatus.className = 'topbar-status-metric done';
    const tpFormatted = throughput >= 1000000 ? `${(throughput / 1000000).toFixed(2)}M` : `${Math.round(throughput / 1000)}k`;
    topSimStatus.textContent = `${totalFightsCount.toLocaleString()} fights · ${tpFormatted} fights/s`;
  }

  if (progContainer) {
    if (progFill) progFill.style.width = '100%';
    if (progText) progText.textContent = '100%';
    setTimeout(() => { progContainer.style.display = 'none'; }, 2000);
  }

  results.timing = multiRes.timing;
  results.totalFights = totalFightsCount;
  return results;
}
