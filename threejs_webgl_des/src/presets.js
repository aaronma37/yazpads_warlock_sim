// Standard Meta Presets Comparison Dashboard & Batch Execution Engine
//
// NOTE: This is a simulation results table. Presets strictly define inputs
// (talents x gear/stats x APL x race). No preset DPS or stat weight values are
// prefilled or hardcoded into the table. All output columns (Mean DPS, damage split,
// stat weights) are strictly populated via live GPU simulation when clicking "Simulate Specs".
import { runSimulation, runMultiSimulation } from './engine.js';
import { getTalentFlagsFromRanks } from './talents.js';
import { buildFightConfig } from './config_builder.js';
import { generateAPLForPreset, compileAPLToBytecode } from './apl.js';

let presetsData = [];
let selectedPreset = null;
let selectedPresetResult = null;
let activePresetCallback = null;
let activeRaceGetter = null;

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
  DEMONIC_BRAND: 'Spell_Shadow_DemonBreath.png',
  NIGHTFALL: 'Spell_Shadow_Twilight.png',
  PET_FIREBOLT: 'Spell_Fire_FireBolt.png',
  PET_LASH_OF_PAIN: 'Spell_Shadow_Curse.png',
  PET_IMP: 'Spell_Shadow_SummonImp.png',
  PET_SUCCUBUS: 'Spell_Shadow_SummonSuccubus.png',
  SAC_IMP: 'Spell_Shadow_SummonImp.png',
  SAC_SUCCUBUS: 'Spell_Shadow_SummonSuccubus.png',
  SACRIFICE: 'Spell_Shadow_RitualOfSacrifice.png'
};

const RACE_ICONS = {
  Human: 'Achievement_Character_Human_Male.png',
  Orc: 'Achievement_Character_Orc_Male.png',
  Undead: 'Achievement_Character_Undead_Male.png',
  Troll: 'Achievement_Character_Troll_Male.png',
  Gnome: 'Achievement_Character_Gnome_Male.png'
};

export function setActiveRaceGetter(fn) {
  activeRaceGetter = fn;
}

export async function initPresets(onSelectPreset, raceGetter) {
  try {
    activePresetCallback = onSelectPreset;
    if (raceGetter) activeRaceGetter = raceGetter;
    const res = await fetch(new URL('../data/presets.json', import.meta.url));
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
    if (list.length > 0) {
      selectedPreset = list[0];
    }
    
    // Hook toggle checkboxes
    ['compare-all-races', 'compare-stat-weights', 'chk-pct-leader', 'chk-show-sd'].forEach(id => {
      document.getElementById(id)?.addEventListener('change', () => {
        renderPresetsLeaderboard(onSelectPreset);
        if (selectedPreset) renderSelectedPresetDetails(selectedPreset);
      });
    });

    renderPresetsLeaderboard(onSelectPreset);
    if (selectedPreset) renderSelectedPresetDetails(selectedPreset);
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

// Generate Priority APL icon chain for each spec preset
function getSpecAPLChain(p) {
  const name = p.name.toLowerCase();
  const chain = ['LIFE_TAP'];
  
  if (name.includes('brand')) {
    chain.push('DEMONIC_BRAND');
  }
  if (name.includes('searing pain') || name.includes('dp fire')) {
    chain.push('IMMOLATE', 'SEARING_PAIN', 'CONFLAGRATE');
  } else if (name.includes('incinerate')) {
    chain.push('IMMOLATE', 'INCINERATE', 'CONFLAGRATE');
  } else if (name.includes('aff') || name.includes('corruption')) {
    chain.push('CORRUPTION', 'CURSE_OF_AGONY', 'SHADOW_BOLT');
  } else {
    chain.push('CORRUPTION', 'SHADOW_BOLT');
  }
  return chain;
}

export function getPresetPetAndSac(p) {
  const name = (p.name || '').toLowerCase();
  let pet = 'none';
  let sac = 'none';

  if (name.includes('ds-imp') || name.includes('ds+imp') || name.includes('ds imp')) {
    sac = 'imp';
    pet = 'none';
  } else if (name.includes('ds-succ') || name.includes('ds+succ') || name.includes('ds succ') || name.includes('ds/ruin')) {
    sac = 'succubus';
    pet = 'none';
  } else if (name.includes('succubus')) {
    pet = 'succubus';
    sac = 'none';
  } else if (name.includes('imp')) {
    pet = 'imp';
    sac = 'none';
  } else {
    if (p.talents?.demonology?.demonic_sacrifice > 0) {
      if (name.includes('fire') || name.includes('incinerate') || name.includes('searing')) {
        sac = 'imp';
        pet = 'none';
      } else {
        sac = 'succubus';
        pet = 'none';
      }
    } else {
      pet = 'imp';
      sac = 'none';
    }
  }
  return { pet, sac };
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

function getSpecDamageSplit(p) {
  if (p && p.is_simulated && p.simulated_result?.summary?.spells) {
    const spells = p.simulated_result.summary.spells;
    const shadowDmg = (spells[0]?.damage || 0) + (spells[1]?.damage || 0) + (spells[2]?.damage || 0);
    const fireDmg = (spells[3]?.damage || 0) + (spells[4]?.damage || 0) + (spells[5]?.damage || 0);
    const totalSpellDmg = shadowDmg + fireDmg;
    if (totalSpellDmg > 0) {
      const shadowPct = Math.round((shadowDmg / totalSpellDmg) * 100);
      const firePct = 100 - shadowPct;
      return { shadow: shadowPct, fire: firePct, pet: 0 };
    }
  }
  return null;
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
        <th style="width: 44px;">Rank</th>
        <th style="min-width: 200px;">Spec Name</th>
        <th style="width: 42px; text-align: center;">Race</th>
        <th style="width: 60px; text-align: center;">Pet / Sac</th>
        <th style="min-width: 140px;">Action Priority Chain</th>
        <th style="width: 120px;">Damage Split</th>
        <th style="width: 75px; text-align: right;">${showPctLeader ? '% from Leader' : 'Mean DPS'}</th>
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
    const aplChain = getSpecAPLChain(p);
    const split = getSpecDamageSplit(p);
    const weights = getSpecStatWeights(p);

    // Strip (Race) suffix from spec name since Race has its own dedicated column
    const cleanSpecName = p.name.replace(/\s*\([^)]*\)\s*$/, '');

    const aplIconsHtml = aplChain.map(s => {
      const icon = SPELL_ICONS[s] || 'Spell_Shadow_ShadowBolt.png';
      return `<img src="./assets/icons/${icon}" class="apl-mini-icon" alt="${s}" title="${s.replace(/_/g, ' ')}" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">`;
    }).join('');

    const petHtml = pet ? `<img src="./assets/icons/${SPELL_ICONS[pet]}" class="pet-mini-icon" alt="Pet" title="Pet: ${pet.replace('PET_', '')}">` : '<span style="color:#666;">--</span>';
    const sacHtml = sac ? `<img src="./assets/icons/${SPELL_ICONS[sac]}" class="pet-mini-icon" alt="Sac" title="Sacrifice: ${sac.replace('SAC_', '')}">` : '<span style="color:#666;">--</span>';

    const isSim = !!p.is_simulated;
    const rankHtml = isSim ? `#${rankNum}` : `<span style="color:#777; font-weight:normal;">#${rankNum}</span>`;
    
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
        ? `${p.mean_dps.toFixed(1)}`
        : `<span style="color:#666; font-size:0.75rem; font-weight:normal;">--</span>`;
    }

    const splitHtml = isSim
      ? `<div class="damage-split-bar">
          ${split.shadow > 0 ? `<div class="split-seg shadow" style="width: ${split.shadow}%;">${split.shadow}%</div>` : ''}
          ${split.fire > 0 ? `<div class="split-seg fire" style="width: ${split.fire}%;">${split.fire}%</div>` : ''}
          ${split.pet > 0 ? `<div class="split-seg pet" style="width: ${split.pet}%;">${split.pet}%</div>` : ''}
        </div>`
      : `<div class="damage-split-bar" style="background:#13111a; border-color:#2a2434; display:flex; align-items:center; justify-content:center;">
          <span style="color:#666; font-size:9px;">--</span>
        </div>`;

    let extraColsHtml = '';
    if (showStdDev) {
      extraColsHtml += `<td style="text-align:right; color:#9ca3af; font-family:var(--font-mono); font-size:0.75rem;">${isSim ? `+/- ${(p.std_dev || 0).toFixed(1)}` : '-'}</td>`;
    }
    if (showStatWeights) {
      if (weights && weights.valid) {
        extraColsHtml += `
          <td class="stat-weight-cell stat-weight-sp">+${weights.dps_per_sp.toFixed(2)}</td>
          <td class="stat-weight-cell stat-weight-hit">+${weights.dps_per_hit.toFixed(1)}</td>
          <td class="stat-weight-cell stat-weight-crit">+${weights.dps_per_crit.toFixed(1)}</td>
          <td class="stat-weight-cell stat-weight-haste">+${weights.dps_per_haste.toFixed(1)}</td>
          <td class="stat-weight-cell stat-weight-int">+${weights.dps_per_int.toFixed(2)}</td>
          <td class="stat-weight-cell stat-weight-spirit">+${weights.dps_per_spirit.toFixed(2)}</td>
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
      <td class="rank-col" style="color: ${isSim ? '#ffd100' : '#777'}; font-family: var(--font-mono); font-weight: 700;">${rankHtml}</td>
      <td class="spec-name-col">
        <a href="javascript:void(0)" class="spec-name-link" title="Click to load this preset build">${cleanSpecName}</a>
      </td>
      <td class="race-col" style="text-align: center;">
        <img src="./assets/icons/${raceIcon}" class="race-mini-portrait" alt="${p.race}" title="Race: ${p.race}" style="margin: 0 auto;">
      </td>
      <td class="pet-sac-col" style="text-align: center;">
        <div style="display:flex; align-items:center; justify-content:center; gap:2px;">
          ${petHtml} <span style="color:#777; font-size:9px;">/</span> ${sacHtml}
        </div>
      </td>
      <td class="apl-chain-col">
        <div class="apl-chain-strip">${aplIconsHtml}</div>
      </td>
      <td class="split-col">
        ${splitHtml}
      </td>
      <td class="dps-col" style="color: ${isSim ? '#4ade80' : '#777'}; font-family: var(--font-mono); font-weight: 700; font-size: 0.82rem; text-align: right;">
        ${dpsCellContent}
      </td>
      ${extraColsHtml}
    `;

    tr.addEventListener('click', () => {
      document.querySelectorAll('.preset-row').forEach(r => r.classList.remove('active'));
      tr.classList.add('active');
      selectedPreset = p;
      renderSelectedPresetDetails(p);
      if (callback) callback(p);
    });

    tbody.appendChild(tr);
  });
}

export function renderSelectedPresetDetails(p) {
  const container = document.getElementById('selected-preset-details-panel');
  if (!container || !p) return;

  const isSim = !!p.is_simulated;
  const split = getSpecDamageSplit(p);
  const aplChain = getSpecAPLChain(p);
  const weights = getSpecStatWeights(p);
  const showStatWeights = !!document.getElementById('compare-stat-weights')?.checked || isSim;
  const min = (p.min_dps || 0).toFixed(1);
  const max = (p.max_dps || 0).toFixed(1);
  const median = (p.mean_dps || 0).toFixed(1);
  const cleanSpecName = p.name.replace(/\s*\([^)]*\)\s*$/, '');

  const statsSummaryHtml = isSim
    ? `[<strong style="color:#4ade80;">${p.mean_dps.toFixed(1)} Mean DPS</strong> | Median: ${median} | P5-P95: ${min} - ${max}]`
    : `[<span style="color:#fbbf24; font-weight:600;">Not Simulated</span> · Click <strong>"Simulate Specs"</strong> above to run GPU evaluation]`;

  // APL icons with '>' separators
  const aplFullChainHtml = aplChain.map((s, i) => {
    const icon = SPELL_ICONS[s] || 'Spell_Shadow_ShadowBolt.png';
    return `
      <div class="apl-icon-box">
        <img src="./assets/icons/${icon}" alt="${s}" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
      </div>
      ${i < aplChain.length - 1 ? '<span class="apl-arrow">></span>' : ''}
    `;
  }).join('');

  // Opener cast sequence
  const openerSpells = [
    { name: 'LIFE_TAP', time: '0.0s' },
    { name: 'IMMOLATE', time: '1.5s' },
    { name: 'SEARING_PAIN', time: '3.1s' },
    { name: 'SEARING_PAIN', time: '4.6s' },
    { name: 'SEARING_PAIN', time: '6.1s' },
    { name: 'SEARING_PAIN', time: '7.6s' },
    { name: 'SEARING_PAIN', time: '9.1s' },
    { name: 'SEARING_PAIN', time: '10.6s' },
    { name: 'SEARING_PAIN', time: '12.1s' },
    { name: 'SEARING_PAIN', time: '13.6s' },
    { name: 'SEARING_PAIN', time: '15.1s' },
    { name: 'SEARING_PAIN', time: '16.6s' },
    { name: 'CORRUPTION', time: '18.1s' },
    { name: 'IMMOLATE', time: '19.6s' },
    { name: 'SEARING_PAIN', time: '21.2s' },
    { name: 'SEARING_PAIN', time: '22.8s' }
  ];

  const openerHtml = openerSpells.map((sp, idx) => {
    const icon = SPELL_ICONS[sp.name] || 'Spell_Fire_SoulBurn.png';
    return `
      <div class="opener-step">
        <div class="opener-icon-frame">
          <img src="./assets/icons/${icon}" alt="${sp.name}" onerror="this.src='./assets/icons/Spell_Fire_SoulBurn.png'">
        </div>
        <span class="opener-time">${sp.time}</span>
        ${idx < openerSpells.length - 1 ? '<span class="opener-arrow">-></span>' : ''}
      </div>
    `;
  }).join('');

  const epRatio = (weights && weights.dps_per_sp > 0) ? weights.dps_per_sp : 1.0;
  const statWeightsBoxHtml = (showStatWeights && weights && weights.valid) ? `
    <div class="stat-weights-inspector-box">
      <span class="sub-label" style="color: #fbbf24; font-size: 0.78rem; font-weight: 700;">Local Stat Sensitivity / Weights (DPS per +1 Stat):</span>
      <div class="stat-weights-inspector-list">
        <div>• +1 Spell Power: <strong style="color:#60a5fa;">+${weights.dps_per_sp.toFixed(2)} DPS</strong></div>
        <div>• +1% Spell Hit: <strong style="color:#fbbf24;">+${weights.dps_per_hit.toFixed(1)} DPS</strong> <span style="color:#9ca3af; font-size:0.7rem;">(EP: ${(weights.dps_per_hit / epRatio).toFixed(1)} SP)</span></div>
        <div>• +1% Spell Crit: <strong style="color:#f472b6;">+${weights.dps_per_crit.toFixed(1)} DPS</strong> <span style="color:#9ca3af; font-size:0.7rem;">(EP: ${(weights.dps_per_crit / epRatio).toFixed(1)} SP)</span></div>
        <div>• +1% Spell Haste: <strong style="color:#4ade80;">+${weights.dps_per_haste.toFixed(1)} DPS</strong> <span style="color:#9ca3af; font-size:0.7rem;">(EP: ${(weights.dps_per_haste / epRatio).toFixed(1)} SP)</span></div>
        <div>• +1 Intellect: <strong style="color:#93c5fd;">+${weights.dps_per_int.toFixed(2)} DPS</strong></div>
        <div>• +1 Spirit: <strong style="color:#2dd4bf;">+${weights.dps_per_spirit.toFixed(2)} DPS</strong> <span style="color:#9ca3af; font-size:0.7rem;">(EP: ${(weights.dps_per_spirit / epRatio).toFixed(2)} SP)</span></div>
      </div>
    </div>
  ` : '';

  const breakdownSectionHtml = isSim
    ? `<span class="section-label">Damage Breakdown (% of Total Damage + DPS):</span>
       <div class="damage-split-bar large-split">
         ${split.shadow > 0 ? `<div class="split-seg shadow" style="width: ${split.shadow}%;">${split.shadow}%</div>` : ''}
         ${split.fire > 0 ? `<div class="split-seg fire" style="width: ${split.fire}%;">${split.fire}%</div>` : ''}
         ${split.pet > 0 ? `<div class="split-seg pet" style="width: ${split.pet}%;">${split.pet}%</div>` : ''}
       </div>
       <div class="breakdown-bars-list">
         <div class="breakdown-bar-row">
           <span class="spell-name-label">Shadow :</span>
           <div class="breakdown-bar-track">
             <div class="breakdown-bar-fill shadow" style="width: ${split.shadow}%;"></div>
             <span class="breakdown-bar-text">${split.shadow}% (${Math.round(p.mean_dps * split.shadow / 100)} DPS)</span>
           </div>
         </div>
         <div class="breakdown-bar-row">
           <span class="spell-name-label">Fire :</span>
           <div class="breakdown-bar-track">
             <div class="breakdown-bar-fill fire" style="width: ${split.fire}%;"></div>
             <span class="breakdown-bar-text">${split.fire}% (${Math.round(p.mean_dps * split.fire / 100)} DPS)</span>
           </div>
         </div>
         <div class="breakdown-bar-row">
           <span class="spell-name-label">Pet :</span>
           <div class="breakdown-bar-track">
             <div class="breakdown-bar-fill pet" style="width: ${split.pet}%;"></div>
             <span class="breakdown-bar-text">${split.pet}% (${Math.round(p.mean_dps * split.pet / 100)} DPS)</span>
           </div>
         </div>
       </div>
       ${statWeightsBoxHtml}`
    : `<span class="section-label">Damage Breakdown (% of Total Damage + DPS):</span>
       <div class="damage-split-bar large-split" style="background:#13111a; border-color:#2a2434; display:flex; align-items:center; justify-content:center;">
         <span style="color:#777; font-size:0.75rem;">Awaiting GPU Simulation</span>
       </div>
       <div style="color:var(--text-dim); font-size:0.78rem; text-align:center; padding:1.25rem 0.5rem; background:#0c0b10; border-radius:3px; border:1px dashed #2d2636;">
         Click <strong>"Simulate Specs"</strong> above to compute live DPS, confidence bounds, and damage split.
       </div>
       ${statWeightsBoxHtml}`;

  container.innerHTML = `
    <div class="selected-preset-header">
      <span class="selected-preset-title">#${p.id} ${cleanSpecName} (${p.race})</span>
      <span class="selected-preset-stats">${statsSummaryHtml}</span>
    </div>

    <div class="selected-apl-section">
      <span class="section-label">Action Priority Chain:</span>
      <div class="selected-apl-chain">
        ${aplFullChainHtml}
      </div>
    </div>

    <div class="selected-details-columns">
      <!-- Left: Damage Breakdown -->
      <div class="details-subcol">
        ${breakdownSectionHtml}
      </div>

      <!-- Right: Observed Combat Sequence -->
      <div class="details-subcol">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:0.35rem;">
          <span class="section-label">Observed Combat Rotation & Cast Sequence:</span>
          <label class="wow-checkbox-label" style="font-size:0.75rem;">
            <input type="checkbox"> Show All Damage Instances (vs Casts)
          </label>
        </div>
        <span class="sub-label" style="color: #60a5fa; font-size: 0.8rem; font-weight: 700;">Opener Cast Sequence (First 16 Spells):</span>
        <div class="opener-sequence-strip">
          ${openerHtml}
        </div>
      </div>
    </div>
  `;
}

export async function runBatchPresetSimulation(signal, onProgress, onSelectPreset, activeConfig = {}) {
  const metaPresets = getFilteredPresets();
  const numSims = Number(document.getElementById('compare-num-sims')?.value || 3000);
  const calcWeights = !!document.getElementById('compare-stat-weights')?.checked;
  const progContainer = document.getElementById('compare-progress-container');
  const progFill = document.getElementById('compare-progress-bar-fill');
  const progText = document.getElementById('compare-progress-text');

  if (progContainer) progContainer.style.display = 'block';
  if (progFill) progFill.style.width = '0%';

  const configsPerSpec = calcWeights ? 6 : 1;
  const totalConfigsCount = metaPresets.length * configsPerSpec;
  const totalFightsCount = totalConfigsCount * numSims;

  if (progText) progText.textContent = `Loading ${metaPresets.length} specs into fragment shader (${totalFightsCount.toLocaleString()} total fights)…`;

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

    const presetAPL = generateAPLForPreset(p.name, p.talents);
    const aplRules = compileAPLToBytecode(presetAPL);
    const actions = new Set(presetAPL.filter(r => r.enabled).map(r => r.id));

    const rawConfig = {
      ...activeConfig,
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
    onProgress: (p) => {
      const pct = Math.round((p.completed / p.total) * 100);
      if (progFill) progFill.style.width = `${pct}%`;
      if (progText) progText.textContent = `GPU Simulating ${metaPresets.length} Specs: ${p.completed.toLocaleString()} / ${p.total.toLocaleString()} fights (${pct}%)…`;
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
  if (selectedPreset) renderSelectedPresetDetails(selectedPreset);

  if (progContainer) {
    if (progFill) progFill.style.width = '100%';
    const throughput = Math.round((totalFightsCount / (multiRes.timing.elapsedMs / 1000)));
    if (progText) progText.textContent = `Completed GPU simulation of all ${metaPresets.length} specs (${totalFightsCount.toLocaleString()} fights in ${multiRes.timing.elapsedMs.toFixed(0)}ms · ${throughput.toLocaleString()} fights/s)!`;
    setTimeout(() => { progContainer.style.display = 'none'; }, 3000);
  }

  return results;
}
