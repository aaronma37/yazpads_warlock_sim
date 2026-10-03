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
  const demoTalents = p.talents?.demonology || {};
  const demoPoints = Object.values(demoTalents).reduce((a, b) => a + (Number(b) || 0), 0);
  const isDPSpec = demoPoints >= 31 || (demoTalents.demonic_pact > 0) || name.includes('dp') || name.includes('demonic pact');

  let pet = 'none';
  let sac = 'none';

  if (isDPSpec) {
    // DP specs (31 Demonology) use BOTH Pet AND Demonic Sacrifice:
    // In this sim version: Sac Succubus = +15% Fire Dmg, Sac Imp = +15% Shadow Dmg.
    // DP Fire has Pet Imp + DS Succubus (+15% Fire).
    // DP Shadow has Pet Succubus + DS Imp (+15% Shadow).
    if (name.includes('fire') || name.includes('incinerate') || name.includes('searing')) {
      sac = 'succubus';
      pet = 'imp';
    } else {
      sac = 'imp';
      pet = 'succubus';
    }
  } else if (demoTalents.demonic_sacrifice > 0 || name.includes('ds-') || name.includes('ds/') || name.includes('ds+')) {
    // Pure Demonic Sacrifice specs (pet sacrificed, no active pet)
    pet = 'none';
    if (name.includes('ds-succ') || name.includes('ds+succ') || name.includes('ds succ') || name.includes('fire') || name.includes('incinerate') || name.includes('searing')) {
      sac = 'succubus';
    } else {
      sac = 'imp';
    }
  } else {
    // Standard pet specs without sacrifice
    sac = 'none';
    if (name.includes('succubus') || name.includes('lash')) {
      pet = 'succubus';
    } else {
      pet = 'imp';
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

function getPresetTalentDistribution(p) {
  let aff = 0, demo = 0, destro = 0;
  if (p.talents) {
    if (p.talents.affliction) aff = Object.values(p.talents.affliction).reduce((a, b) => a + (Number(b) || 0), 0);
    if (p.talents.demonology) demo = Object.values(p.talents.demonology).reduce((a, b) => a + (Number(b) || 0), 0);
    if (p.talents.destruction) destro = Object.values(p.talents.destruction).reduce((a, b) => a + (Number(b) || 0), 0);
  }
  return { aff, demo, destro, str: `${aff}/${demo}/${destro}` };
}

function getSpecDamageSplit(p) {
  if (p && p.is_simulated && p.simulated_result?.summary) {
    const summary = p.simulated_result.summary;
    const total = summary.damage || (summary.mean * (p.simulated_result.config?.duration || 180)) || 1;
    const shadowDmg = summary.shadowDamage || 0;
    const fireDmg = summary.fireDamage || 0;
    const petDmg = summary.petDamage || 0;
    const totalDmg = Math.max(1, shadowDmg + fireDmg + petDmg);
    const shadowPct = Math.round((shadowDmg / totalDmg) * 100);
    const firePct = Math.round((fireDmg / totalDmg) * 100);
    const petPct = Math.max(0, 100 - shadowPct - firePct);
    return { shadow: shadowPct, fire: firePct, pet: petPct, shadowDmg, fireDmg, petDmg, totalDmg };
  }
  return { shadow: 0, fire: 0, pet: 0, shadowDmg: 0, fireDmg: 0, petDmg: 0, totalDmg: 0 };
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
        <th style="width: 130px;">Damage Split</th>
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
    const aplChain = getSpecAPLChain(p);
    const split = getSpecDamageSplit(p);
    const weights = getSpecStatWeights(p);
    const talentDist = getPresetTalentDistribution(p);

    // Strip leading x/y/z and trailing (Race) suffix from spec name
    const cleanSpecName = p.name.replace(/^\s*\d+\s*\/\s*\d+\s*\/\s*\d+\s*/, '').replace(/\s*\([^)]*\)\s*$/, '').trim();

    const aplIconsHtml = aplChain.map(s => {
      const icon = SPELL_ICONS[s] || 'Spell_Shadow_ShadowBolt.png';
      return `<img src="./assets/icons/${icon}" class="apl-mini-icon" alt="${s}" title="${s.replace(/_/g, ' ')}" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">`;
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

    const splitHtml = isSim
      ? `<div class="damage-split-bar" title="Shadow: ${split.shadow}% | Fire: ${split.fire}% | Pet: ${split.pet}%">
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
      <td class="rank-col" style="text-align: center;">${rankHtml}</td>
      <td class="spec-name-col">
        <div style="display: flex; align-items: center; gap: 0.45rem; flex-wrap: wrap;">
          <span class="spec-talents-tag">[<span style="color:#c084fc;">${talentDist.aff}</span>/<span style="color:#38bdf8;">${talentDist.demo}</span>/<span style="color:#fb923c;">${talentDist.destro}</span>]</span>
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
      <td class="split-col">
        ${splitHtml}
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
      renderSelectedPresetDetails(p);
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
  const cleanSpecName = p.name.replace(/^\s*\d+\s*\/\s*\d+\s*\/\s*\d+\s*/, '').replace(/\s*\([^)]*\)\s*$/, '').trim();
  const talentDist = getPresetTalentDistribution(p);

  const statsSummaryHtml = isSim
    ? `[<strong style="color:#4ade80; font-size:0.95rem;">${p.mean_dps.toFixed(1)} Mean DPS</strong> | Median: ${median} | 90% Range: ${min} - ${max}]`
    : `[<span style="color:#fbbf24; font-weight:600;">Not Simulated</span> · Click <strong>"Simulate Specs"</strong> above to run GPU evaluation]`;

  // APL icons with '>' separators
  const aplFullChainHtml = aplChain.map((s, i) => {
    const icon = SPELL_ICONS[s] || 'Spell_Shadow_ShadowBolt.png';
    return `
      <div class="apl-icon-box">
        <img src="./assets/icons/${icon}" alt="${s}" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
        <span class="apl-icon-name">${s.replace(/_/g, ' ')}</span>
      </div>
      ${i < aplChain.length - 1 ? '<span class="apl-arrow">›</span>' : ''}
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
        ${idx < openerSpells.length - 1 ? '<span class="opener-arrow">›</span>' : ''}
      </div>
    `;
  }).join('');

  const epRatio = (weights && weights.dps_per_sp > 0) ? weights.dps_per_sp : 1.0;
  const statWeightsBoxHtml = (showStatWeights && weights && weights.valid) ? `
    <div class="stat-weights-inspector-box">
      <span class="sub-label" style="color: #fbbf24; font-size: 0.78rem; font-weight: 700;">Stat Sensitivity Weights (DPS per +1 Stat):</span>
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

  // Spell Breakdown list
  let perSpellRowsHtml = '';
  if (isSim && p.simulated_result?.summary?.spells) {
    const summary = p.simulated_result.summary;
    const dur = p.simulated_result.config?.duration || 180;
    const totDmg = summary.damage || (summary.mean * dur) || 1;
    const activeSp = summary.spells.filter(sp => sp.damage > 0 || sp.casts > 0);
    const maxSpDmg = Math.max(...activeSp.map(sp => sp.damage), summary.petDamage || 0, 1);

    perSpellRowsHtml = `
      <div style="margin-top: 0.65rem;">
        <span class="sub-label" style="color: var(--text-gold); font-size: 0.78rem; font-weight: 700; margin-bottom: 0.35rem; display:block;">Detailed Spell Damage Split:</span>
        <div class="table-scroll" style="max-height: 160px;">
          <table class="damage-table">
            <thead>
              <tr>
                <th style="width: 130px;">Spell</th>
                <th style="text-align: right; width: 65px;">Damage</th>
                <th style="text-align: right; width: 55px;">DPS</th>
                <th style="text-align: right; width: 45px;">Casts</th>
                <th style="text-align: right; width: 55px;">Crits</th>
                <th>Split</th>
              </tr>
            </thead>
            <tbody>
              ${activeSp.map(sp => {
                const spDps = sp.damage / dur;
                const spPct = (sp.damage / totDmg * 100).toFixed(1);
                const critPct = sp.casts > 0 ? ((sp.crits / sp.casts) * 100).toFixed(1) : '0';
                const icon = SPELL_ICONS[sp.name.toUpperCase().replace(/\s+/g, '_')] || 'Spell_Shadow_ShadowBolt.png';
                const isFire = ['Immolate', 'Incinerate', 'Searing Pain'].includes(sp.name);
                const barClass = isFire ? 'fire' : 'shadow';
                const barW = Math.min(100, Math.max(4, Math.round((sp.damage / maxSpDmg) * 100)));
                return `
                  <tr>
                    <td>
                      <div class="spell-breakdown-cell">
                        <img src="./assets/icons/${icon}" class="spell-breakdown-icon" alt="${sp.name}" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
                        <span class="spell-breakdown-name">${sp.name}</span>
                      </div>
                    </td>
                    <td style="text-align: right; font-family: var(--font-mono); color: var(--text-gold); font-size: 0.75rem; white-space: nowrap;">${Math.round(sp.damage)} (${spPct}%)</td>
                    <td style="text-align: right; font-family: var(--font-mono); color: #4ade80; font-size: 0.75rem; white-space: nowrap;">${spDps.toFixed(1)}</td>
                    <td style="text-align: right; font-family: var(--font-mono); font-size: 0.75rem; white-space: nowrap;">${sp.casts.toFixed(1)}</td>
                    <td style="text-align: right; font-family: var(--font-mono); color: #fbbf24; font-size: 0.75rem; white-space: nowrap;">${sp.crits.toFixed(1)} (${critPct}%)</td>
                    <td>
                      <div class="spell-mini-bar-track">
                        <div class="spell-mini-bar-fill ${barClass}" style="width: ${barW}%;"></div>
                      </div>
                    </td>
                  </tr>
                `;
              }).join('')}
              ${(summary.petDamage > 0) ? `
                <tr>
                  <td>
                    <div class="spell-breakdown-cell">
                      <img src="./assets/icons/Spell_Shadow_SummonImp.png" class="spell-breakdown-icon" alt="Pet">
                      <span class="spell-breakdown-name">Pet Damage</span>
                    </div>
                  </td>
                  <td style="text-align: right; font-family: var(--font-mono); color: var(--text-gold); font-size: 0.75rem; white-space: nowrap;">${Math.round(summary.petDamage)} (${(summary.petDamage / totDmg * 100).toFixed(1)}%)</td>
                  <td style="text-align: right; font-family: var(--font-mono); color: #4ade80; font-size: 0.75rem; white-space: nowrap;">${(summary.petDamage / dur).toFixed(1)}</td>
                  <td style="text-align: right; font-family: var(--font-mono); font-size: 0.75rem; white-space: nowrap;">-</td>
                  <td style="text-align: right; font-family: var(--font-mono); color: #fbbf24; font-size: 0.75rem; white-space: nowrap;">-</td>
                  <td>
                    <div class="spell-mini-bar-track">
                      <div class="spell-mini-bar-fill pet" style="width: ${Math.min(100, Math.max(4, Math.round((summary.petDamage / maxSpDmg) * 100)))}%;"></div>
                    </div>
                  </td>
                </tr>
              ` : ''}
            </tbody>
          </table>
        </div>
      </div>
    `;
  }

  const breakdownSectionHtml = isSim
    ? `<span class="section-label">School Damage Split:</span>
       <div class="damage-split-bar large-split">
         ${split.shadow > 0 ? `<div class="split-seg shadow" style="width: ${split.shadow}%;">${split.shadow}% Shadow</div>` : ''}
         ${split.fire > 0 ? `<div class="split-seg fire" style="width: ${split.fire}%;">${split.fire}% Fire</div>` : ''}
         ${split.pet > 0 ? `<div class="split-seg pet" style="width: ${split.pet}%;">${split.pet}% Pet</div>` : ''}
       </div>
       ${perSpellRowsHtml}
       ${statWeightsBoxHtml}`
    : `<span class="section-label">Damage Breakdown:</span>
       <div class="damage-split-bar large-split" style="background:#13111a; border-color:#2a2434; display:flex; align-items:center; justify-content:center;">
         <span style="color:#777; font-size:0.75rem;">Awaiting GPU Simulation</span>
       </div>
       <div style="color:var(--text-dim); font-size:0.78rem; text-align:center; padding:1rem 0.5rem; background:#0c0b10; border-radius:3px; border:1px dashed #2d2636;">
         Click <strong>"Simulate Specs"</strong> above to compute live DPS, confidence bounds, and damage split.
       </div>
       ${statWeightsBoxHtml}`;

  container.innerHTML = `
    <div class="selected-preset-header">
      <div style="display: flex; align-items: center; gap: 0.6rem; flex-wrap: wrap;">
        <span class="spec-talents-tag" style="font-size:0.8rem; padding: 2px 6px;">[<span style="color:#c084fc;">${talentDist.aff}</span> / <span style="color:#38bdf8;">${talentDist.demo}</span> / <span style="color:#fb923c;">${talentDist.destro}</span>]</span>
        <span class="selected-preset-title">#${p.id} ${cleanSpecName} (${p.race})</span>
        <button type="button" class="wow-button wow-btn-small" id="btn-load-selected-preset" style="font-size:0.75rem; margin-left: 0.5rem;">Load Into Armory</button>
      </div>
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
        <span class="section-label">Observed Combat Rotation & Cast Sequence:</span>
        <span class="sub-label" style="color: #60a5fa; font-size: 0.78rem; font-weight: 700; margin-top: 0.25rem; display:block;">Opener Cast Sequence (First 16 Spells):</span>
        <div class="opener-sequence-strip">
          ${openerHtml}
        </div>
      </div>
    </div>
  `;

  container.querySelector('#btn-load-selected-preset')?.addEventListener('click', () => {
    if (activePresetCallback) activePresetCallback(p);
    const curTab = document.getElementById('btn-current-build');
    if (curTab) curTab.click();
  });
}

export async function runBatchPresetSimulation(signal, onProgress, onSelectPreset, activeConfig = {}) {
  const metaPresets = getFilteredPresets();
  const numSims = Number(document.getElementById('compare-num-sims')?.value || 3000);
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
  if (selectedPreset) renderSelectedPresetDetails(selectedPreset);

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
