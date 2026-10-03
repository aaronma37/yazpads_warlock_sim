import { DEFAULTS, SPELLS } from './model.js';
import { buildFightConfig } from './config_builder.js';
import { runSimulation, getEngineMode } from './engine.js';
import { compare } from '../validation/compare.js';
import { initTalents, applyTalentsObject, applyTalentPreset, resetTalents, getSimTalentFlags } from './talents.js';
import { initBuffs, getActiveBuffStats } from './buffs.js';
import { initPresets, getPresets, getPresetPetAndSac, runBatchPresetSimulation, renderPresetsLeaderboard } from './presets.js';
import { initGear, setGearMode, calculateEquippedStats } from './gear.js';
import { initAPL, setAPLPreset, getActiveAPL, getActiveBytecodeRules } from './apl.js';
import { initTooltips, showTooltip, hideTooltip } from './tooltips.js';

const $ = id => document.getElementById(id);
const form = $('setup-form');
const format = (n, d = 0) => n.toLocaleString(undefined, { maximumFractionDigits: d });
let currentResult = null, controller = null;
let importedConfig = null;

// Service Worker Registration for PWA / Offline support
if ('serviceWorker' in navigator && location.protocol.startsWith('http')) {
  window.addEventListener('load', () => {
    navigator.serviceWorker.register('./sw.js').catch((err) => console.log('SW registration skipped:', err));
  });
}

// Check WebGL2 Support & Execution Mode
let engineModeName = 'WebGL2 Active';
let capable = false;
try {
  const probe = document.createElement('canvas').getContext('webgl2');
  capable = !!probe;
  probe?.getExtension('WEBGL_lose_context')?.loseContext();
  if (capable) {
    const mode = getEngineMode();
    engineModeName = `WebGL2 (${mode})`;
  }
} catch {
  capable = false;
}

if ($('gpu-badge')) {
  $('gpu-badge').innerHTML = capable
    ? `<span class="status-dot">●</span> ${engineModeName}`
    : '<span class="status-dot" style="color:#ef4444;">●</span> WebGL2 Unavailable';
}
if ($('top-sim-status')) {
  $('top-sim-status').textContent = 'Ready to simulate';
}
if (!capable) {
  $('run').disabled = true;
  $('validate').disabled = true;
  setStatus('WebGL2 is unavailable. Enable hardware acceleration or try another browser.', true);
}

// Navigation Tabs Matching Desktop App
const tabs = [
  { btn: 'btn-current-build', pane: 'tab-current-build' },
  { btn: 'btn-compare-specs', pane: 'tab-compare-specs' },
  { btn: 'btn-talents', pane: 'tab-talents' },
  { btn: 'btn-buffs', pane: 'tab-buffs' },
  { btn: 'btn-checks', pane: 'tab-checks' }
];

tabs.forEach(({ btn, pane }) => {
  $(btn)?.addEventListener('click', () => {
    tabs.forEach(t => {
      const b = $(t.btn);
      const p = $(t.pane);
      if (b && p) {
        const isActive = (t.btn === btn);
        b.classList.toggle('active', isActive);
        b.setAttribute('aria-selected', String(isActive));
        p.hidden = !isActive;
        p.classList.toggle('active', isActive);
      }
    });
  });
});

// Race / Pet / DS Interactive Popover Pickers
const RACE_ICONS_DATA = {
  HUMAN: {
    icon: './assets/icons/Achievement_Character_Human_Male.png',
    traits: [
      { name: 'The Human Spirit', icon: 'Spell_Holy_MagicalSentry.png', title: 'The Human Spirit (+5% Spirit)', desc: 'Spirit increased by 5%.' },
      { name: 'Perception', icon: 'Spell_Holy_MindVision.png', title: 'Perception (Stealth Detect)', desc: 'Dramatically increases stealth detection for 20 sec.' }
    ]
  },
  ORC: {
    icon: './assets/icons/Achievement_Character_Orc_Male.png',
    traits: [
      { name: 'Blood Fury', icon: 'Racial_Orc_BerserkerStrength.png', title: 'Blood Fury (+25% Base Melee/Spell AP)', desc: 'Increases base melee and spell attack power by 25% for 15 sec.' },
      { name: 'Command', icon: 'Ability_Hunter_Pet_Gorilla.png', title: 'Command (+5% Pet Damage)', desc: 'Damage dealt by Warlock and Hunter pets increased by 5%.' }
    ]
  },
  UNDEAD: {
    icon: './assets/icons/Achievement_Character_Undead_Male.png',
    traits: [
      { name: 'Will of the Forsaken', icon: 'Spell_Shadow_RaiseDead.png', title: 'Will of the Forsaken (Charm/Fear/Sleep Immunity)', desc: 'Provides immunity to Charm, Fear, and Sleep effects for 5 sec.' },
      { name: 'Cannibalize', icon: 'Spell_Shadow_Cannibalize.png', title: 'Cannibalize', desc: 'When activated, regenerates 7% of total health every 2 sec for 10 sec.' }
    ]
  },
  TROLL: {
    icon: './assets/icons/Achievement_Character_Troll_Male.png',
    traits: [
      { name: 'Berserking', icon: 'Racial_Troll_Berserk.png', title: 'Berserking (+10-30% Haste)', desc: 'Increases casting and attack speed by 10% to 30% depending on health.' },
      { name: 'Beast Slaying', icon: 'Ability_Hunter_BeastSoothe.png', title: 'Beast Slaying (+5% vs Beasts)', desc: 'Damage dealt versus Beasts increased by 5%.' }
    ]
  },
  GNOME: {
    icon: './assets/icons/Achievement_Character_Gnome_Male.png',
    traits: [
      { name: 'Expansive Mind', icon: 'Spell_Nature_EnchantWater.png', title: 'Expansive Mind (+5% Intellect)', desc: 'Intellect increased by 5%.' },
      { name: 'Escape Artist', icon: 'Spell_Holy_Silence.png', title: 'Escape Artist', desc: 'Escape the effects of any immobilization or movement speed reduction.' }
    ]
  }
};

const PET_ICONS_DATA = {
  imp: './assets/icons/Spell_Shadow_SummonImp.png',
  succubus: './assets/icons/Spell_Shadow_SummonSuccubus.png',
  none: './assets/icons/Spell_Shadow_SacrificialShield.png'
};

const DS_ICONS_DATA = {
  succubus: './assets/icons/Spell_Shadow_SummonSuccubus.png',
  imp: './assets/icons/Spell_Shadow_SummonImp.png',
  none: './assets/icons/Spell_Shadow_SacrificialShield.png'
};

let activeRace = 'HUMAN';
let activePet = 'imp';
let activeDS = 'none';
let activeRotation = 'shadow';

export function setRotation(rot) {
  const norm = (rot || 'shadow').toLowerCase();
  activeRotation = ['shadow', 'fire', 'searing', 'bolt'].includes(norm) ? norm : 'shadow';
}

export function getRotation() {
  return activeRotation;
}

export function setRace(raceKey) {
  const norm = (raceKey || 'HUMAN').toUpperCase();
  if (RACE_ICONS_DATA[norm]) {
    activeRace = norm;
    if ($('slot-race-icon')) $('slot-race-icon').src = RACE_ICONS_DATA[norm].icon;
    updateRacialsDisplay();
    updateCombatStatsSummary();
    renderPresetsLeaderboard();
  }
}

export function setPet(petKey) {
  const norm = (petKey || 'imp').toLowerCase();
  activePet = PET_ICONS_DATA[norm] ? norm : 'none';
  if ($('slot-pet-icon')) $('slot-pet-icon').src = PET_ICONS_DATA[activePet];
  updateCombatStatsSummary();
}

export function setDS(dsKey) {
  const norm = (dsKey || 'none').toLowerCase();
  activeDS = DS_ICONS_DATA[norm] ? norm : 'none';
  if ($('slot-ds-icon')) $('slot-ds-icon').src = DS_ICONS_DATA[activeDS];
  updateCombatStatsSummary();
}

function setupInteractiveSlots() {
  // Popover toggles
  setupPopover('slot-race-picker', 'popover-race');
  setupPopover('slot-pet-picker', 'popover-pet');
  setupPopover('slot-ds-picker', 'popover-ds');

  // Hover tooltips on slot items
  const raceSlot = $('slot-race-picker');
  if (raceSlot) {
    raceSlot.addEventListener('mouseenter', (e) => {
      const raceData = RACE_ICONS_DATA[activeRace];
      const desc = raceData ? raceData.traits.map(t => `• ${t.title}`).join('<br>') : '';
      showTooltip(e, {
        title: `Race: ${activeRace.charAt(0) + activeRace.slice(1).toLowerCase()}`,
        subtitle: 'Click to Change Race',
        desc,
        footer: 'Affects base stats and racial trait synergies.'
      });
    });
    raceSlot.addEventListener('mousemove', (e) => raceSlot.dispatchEvent(new MouseEvent('mouseenter', e)));
    raceSlot.addEventListener('mouseleave', () => hideTooltip());
  }

  const petSlot = $('slot-pet-picker');
  if (petSlot) {
    petSlot.addEventListener('mouseenter', (e) => {
      const petName = activePet === 'none' ? 'None' : activePet.charAt(0).toUpperCase() + activePet.slice(1);
      const desc = activePet === 'imp' ? 'Imp casts Firebolt and benefits from Demonic talents.' : activePet === 'succubus' ? 'Succubus casts Lash of Pain on cooldown.' : 'No demon companion summoned.';
      showTooltip(e, {
        title: `Pet: ${petName}`,
        subtitle: 'Click to Select Pet',
        desc,
        footer: 'Pet damage scales with master spell power and talents.'
      });
    });
    petSlot.addEventListener('mousemove', (e) => petSlot.dispatchEvent(new MouseEvent('mouseenter', e)));
    petSlot.addEventListener('mouseleave', () => hideTooltip());
  }

  const dsSlot = $('slot-ds-picker');
  if (dsSlot) {
    dsSlot.addEventListener('mouseenter', (e) => {
      const dsName = activeDS === 'succubus' ? 'Succubus (+15% Shadow Damage)' : activeDS === 'imp' ? 'Imp (+15% Fire Damage)' : 'No Demonic Sacrifice';
      const desc = activeDS === 'succubus' ? 'Sacrifices your Succubus to increase Shadow damage by 15%.' : activeDS === 'imp' ? 'Sacrifices your Imp to increase Fire damage by 15%.' : 'Demonic Sacrifice buff is currently not active.';
      showTooltip(e, {
        title: `Sacrifice: ${activeDS.charAt(0).toUpperCase() + activeDS.slice(1)}`,
        subtitle: 'Click to Select Sacrifice Buff',
        desc,
        footer: 'Requires 1 point in Demonic Sacrifice (Demonology Tier 3).'
      });
    });
    dsSlot.addEventListener('mousemove', (e) => dsSlot.dispatchEvent(new MouseEvent('mouseenter', e)));
    dsSlot.addEventListener('mouseleave', () => hideTooltip());
  }

  // Race option clicks
  document.querySelectorAll('#popover-race .picker-opt-btn').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      setRace(btn.dataset.race);
      $('popover-race').style.display = 'none';
      hideTooltip();
    });
  });

  // Pet option clicks
  document.querySelectorAll('#popover-pet .picker-opt-btn').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      setPet(btn.dataset.pet);
      $('popover-pet').style.display = 'none';
      hideTooltip();
    });
  });

  // DS option clicks
  document.querySelectorAll('#popover-ds .picker-opt-btn').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      setDS(btn.dataset.ds);
      $('popover-ds').style.display = 'none';
      hideTooltip();
    });
  });

  // Close popovers on click outside
  document.addEventListener('click', (e) => {
    if (!e.target.closest('.slot-item-interactive')) {
      $('popover-race').style.display = 'none';
      $('popover-pet').style.display = 'none';
      $('popover-ds').style.display = 'none';
    }
  });
}

function setupPopover(triggerId, popoverId) {
  const trigger = $(triggerId);
  const popover = $(popoverId);
  if (!trigger || !popover) return;
  trigger.addEventListener('click', (e) => {
    e.stopPropagation();
    hideTooltip();
    const isVisible = popover.style.display === 'flex';
    $('popover-race').style.display = 'none';
    $('popover-pet').style.display = 'none';
    $('popover-ds').style.display = 'none';
    popover.style.display = isVisible ? 'none' : 'flex';
  });
}

function updateRacialsDisplay() {
  const container = $('racials-list-container');
  if (!container || !RACE_ICONS_DATA[activeRace]) return;
  container.innerHTML = '';
  RACE_ICONS_DATA[activeRace].traits.forEach(t => {
    const badge = document.createElement('div');
    badge.className = 'racial-icon-badge';
    badge.innerHTML = `<img src="./assets/icons/${t.icon.toLowerCase()}" alt="${t.name}" onerror="this.src='./assets/icons/Spell_Holy_MagicalSentry.png'">`;
    
    // Interactive WoW Tooltips for Racial Traits
    badge.addEventListener('mouseenter', (e) => {
      showTooltip(e, {
        title: t.name,
        subtitle: 'Racial Passive / Ability',
        desc: t.desc,
        footer: `Active for ${activeRace.charAt(0) + activeRace.slice(1).toLowerCase()} Warlock.`
      });
    });
    badge.addEventListener('mousemove', (e) => {
      showTooltip(e, {
        title: t.name,
        subtitle: 'Racial Passive / Ability',
        desc: t.desc,
        footer: `Active for ${activeRace.charAt(0) + activeRace.slice(1).toLowerCase()} Warlock.`
      });
    });
    badge.addEventListener('mouseleave', () => hideTooltip());
    
    container.appendChild(badge);
  });
}

// Direct Stats vs Equipped Items Toggle
$('btn-mode-direct')?.addEventListener('click', () => {
  setGearMode('direct');
  updateCombatStatsSummary();
});

$('btn-mode-equipped')?.addEventListener('click', () => {
  setGearMode('equipped');
  const equipped = calculateEquippedStats();
  applyEquippedStatsToForm(equipped);
  updateCombatStatsSummary();
});

window.addEventListener('gear-updated', (e) => {
  if ($('btn-mode-equipped')?.classList.contains('active')) {
    applyEquippedStatsToForm(e.detail);
  }
  updateCombatStatsSummary();
});

function applyEquippedStatsToForm(stats) {
  if ($('in-spellPower')) $('in-spellPower').value = Math.round(stats.spell_power);
  if ($('in-shadowPower')) $('in-shadowPower').value = Math.round(stats.shadow_power);
  if ($('in-firePower')) $('in-firePower').value = Math.round(stats.fire_power);
  if ($('in-hit')) $('in-hit').value = stats.spell_hit.toFixed(1);
  if ($('in-crit')) $('in-crit').value = stats.spell_crit.toFixed(1);
  if ($('in-haste')) $('in-haste').value = stats.spell_haste.toFixed(1);
  if ($('in-intellect')) $('in-intellect').value = Math.round(stats.intellect);
  if ($('in-stamina')) $('in-stamina').value = Math.round(stats.stamina);
  if ($('in-spirit')) $('in-spirit').value = Math.round(stats.spirit);
  if ($('in-mp5')) $('in-mp5').value = Math.round(stats.mp5);
}

function updateCombatStatsSummary() {
  const sp = Number($('in-spellPower')?.value || 500);
  const shadowSpBonus = Number($('in-shadowPower')?.value || 0);
  const fireSpBonus = Number($('in-firePower')?.value || 0);
  const hit = Number($('in-hit')?.value || 12);
  const crit = Number($('in-crit')?.value || 15);
  let int = Number($('in-intellect')?.value || 200);
  let stam = Number($('in-stamina')?.value || 220);
  let spirit = Number($('in-spirit')?.value || 100);
  const mp5 = Number($('in-mp5')?.value || 20);

  // Racial adjustments
  if (activeRace === 'GNOME') int = Math.round(int * 1.05);
  if (activeRace === 'HUMAN') spirit = Math.round(spirit * 1.05);

  const effectiveShadow = sp + shadowSpBonus;
  const effectiveFire = sp + fireSpBonus;
  const maxMana = Math.round(1400 + (int * 15));
  const maxHealth = Math.round(1500 + (stam * 10));

  let shadowMult = 1.0;
  let fireMult = 1.0;
  if (activeDS === 'succubus') shadowMult *= 1.15;
  if (activeDS === 'imp') fireMult *= 1.15;

  if ($('sum-shadow-sp')) $('sum-shadow-sp').textContent = effectiveShadow;
  if ($('sum-fire-sp')) $('sum-fire-sp').textContent = effectiveFire;
  if ($('sum-max-mana')) $('sum-max-mana').textContent = format(maxMana);
  if ($('sum-max-health')) $('sum-max-health').textContent = format(maxHealth);
  if ($('sum-spell-hit')) $('sum-spell-hit').textContent = `${hit.toFixed(1)}%`;
  if ($('sum-spell-crit')) $('sum-spell-crit').textContent = `${crit.toFixed(1)}%`;
  if ($('sum-intellect')) $('sum-intellect').textContent = int;
  if ($('sum-stamina')) $('sum-stamina').textContent = stam;
  if ($('sum-mp5')) $('sum-mp5').textContent = mp5;
  if ($('sum-shadow-mult')) $('sum-shadow-mult').textContent = `${shadowMult.toFixed(2)}x`;
  if ($('sum-fire-mult')) $('sum-fire-mult').textContent = `${fireMult.toFixed(2)}x`;
}

// Copy build string
$('btn-copy-build')?.addEventListener('click', () => {
  const config = readForm();
  const str = btoa(JSON.stringify(config));
  navigator.clipboard.writeText(str).then(() => {
    setStatus('Build string copied to clipboard!');
  });
});

// Import a copied build string or an exported result JSON. Both carry the
// resolved simulation config and enter the same validated builder boundary.
$('btn-load-build')?.addEventListener('click', () => {
  const source = window.prompt('Paste a copied build string or exported simulation JSON:');
  if (!source) return;
  try {
    let payload;
    const trimmed = source.trim();
    if (trimmed.startsWith('{')) payload = JSON.parse(trimmed);
    else payload = JSON.parse(decodeURIComponent(escape(atob(trimmed))));
    const resolved = payload.config || payload;
    importedConfig = buildFightConfig({ resolvedConfig: resolved });
    setStatus('Imported resolved config. Submit to simulate it; edit a control to return to the current form build.');
  } catch (error) {
    importedConfig = null;
    setStatus(`Could not import build: ${error.message}`, true);
  }
});

form.addEventListener('input', () => { importedConfig = null; });
form.addEventListener('change', () => { importedConfig = null; });

function readForm() {
  if (importedConfig) return importedConfig;
  const base = Object.fromEntries(Object.entries(DEFAULTS).map(([key, value]) => {
    const input = form.elements.namedItem(key);
    if (!input) return [key, value];
    if (typeof value === 'boolean') {
      return [key, input.type === 'checkbox' ? input.checked : String(input.value).toLowerCase() === 'true'];
    }
    return [key, typeof value === 'number' ? Number(input.value) : input.value];
  }));
  base.rotation = activeRotation;
  const talent = getSimTalentFlags();
  const activeActions = new Set(getActiveAPL().filter(rule => rule.enabled).map(rule => rule.id));
  // Spell availability is derived from the editable action list so enabling a
  // supported action also enables its shader-side spell gate.
  return buildFightConfig({ base, talent, pet: activePet, sac: activeDS,
    actionIds: activeActions, aplRules: getActiveBytecodeRules(), rotation: activeRotation });
}

function setStatus(message, error = false) {
  $('status').textContent = message;
  $('status').parentElement.classList.toggle('error', error);
}

function setTopStatus(type, data) {
  const engineEl = $('gpu-badge');
  const metricEl = $('top-sim-status');
  if (!metricEl) return;
  if (type === 'compiling') {
    if (engineEl) engineEl.innerHTML = '<span class="status-dot compiling">●</span> Compiling Shader…';
    metricEl.className = 'topbar-status-metric in-progress';
    metricEl.textContent = 'Building WebGL2 pipeline…';
  } else if (type === 'in-progress') {
    if (engineEl) engineEl.innerHTML = '<span class="status-dot compiling">●</span> WebGL2 Simulating…';
    metricEl.className = 'topbar-status-metric in-progress';
    metricEl.textContent = `${data.completed} / ${data.total} fights`;
  } else if (type === 'done') {
    if (engineEl) engineEl.innerHTML = `<span class="status-dot">●</span> ${engineModeName}`;
    metricEl.className = 'topbar-status-metric done';
    const tpFormatted = Math.round(data.throughput) >= 1000000
      ? `${(data.throughput / 1000000).toFixed(2)}M`
      : `${Math.round(data.throughput / 1000)}k`;
    metricEl.textContent = `${format(data.fights)} fights · ${tpFormatted} fights/s`;
  } else if (type === 'error') {
    if (engineEl) engineEl.innerHTML = '<span class="status-dot" style="color:#ef4444;">●</span> Shader Error';
    metricEl.className = 'topbar-status-metric';
    metricEl.style.color = '#ef4444';
    metricEl.textContent = data.message || 'Simulation error';
  }
}

function busy(active) {
  $('inputs').disabled = active;
  $('run').disabled = active || !capable;
  $('validate').disabled = active || !capable;
  const batchBtn = $('btn-batch-sim');
  if (batchBtn) batchBtn.disabled = active || !capable;
  $('cancel').hidden = !active;
  $('cancel').disabled = false;
  
  const progContainer = $('progress-container');
  if (progContainer) progContainer.style.display = active ? 'block' : 'none';
  $('export').disabled = active || !currentResult;
}

function progress(p) {
  const pct = Math.round((p.completed / p.total) * 100);
  const fill = $('progress-bar-fill');
  const text = $('progress-text');
  if (fill) fill.style.width = `${pct}%`;
  if (text) text.textContent = `${p.phase}: ${format(p.completed)} / ${format(p.total)} (${pct}%)`;
  setStatus(`${p.phase} · ${format(p.completed)} / ${format(p.total)} completed`);
  if (p.phase && p.phase.toLowerCase().includes('compil')) {
    setTopStatus('compiling', p);
  } else {
    setTopStatus('in-progress', { completed: format(p.completed), total: format(p.total) });
  }
}

form.addEventListener('submit', async event => {
  event.preventDefault();
  if (controller) return;
  try {
    const config = readForm();
    controller = new AbortController();
    busy(true);
    currentResult = await runSimulation(config, { signal: controller.signal, onProgress: progress });
    render(currentResult);
    setStatus(`Simulation Complete · ${format(currentResult.summary.events)} events · ${format(currentResult.config.iterations)} fights.`);
    const tp = currentResult.config.iterations / (Math.max(1, currentResult.timing.elapsedMs) / 1000);
    setTopStatus('done', { fights: currentResult.config.iterations, throughput: tp, elapsedMs: currentResult.timing.elapsedMs });
  } catch (error) {
    setStatus(error.name === 'AbortError' ? 'Run cancelled. Previous results retained.' : error.message, error.name !== 'AbortError');
    if (error.name !== 'AbortError') setTopStatus('error', { message: error.message });
  } finally {
    controller = null;
    busy(false);
  }
});

$('cancel')?.addEventListener('click', () => {
  controller?.abort();
  $('cancel').disabled = true;
  setStatus('Cancelling after the current GPU batch…');
});

$('export')?.addEventListener('click', () => {
  if (!currentResult) return;
  const blob = new Blob([JSON.stringify(currentResult, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob), a = document.createElement('a');
  a.href = url;
  a.download = `warlock-des-seed-${currentResult.config.seed}.json`;
  a.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
});

function render(result) {
  const { summary: s, timing: t, config: c } = result;
  $('mean').textContent = format(s.mean, 1);
  $('confidence').textContent = s.count > 1 ? `± ${format(s.ci95, 2)} · 95% CI` : 'One fight';
  $('range').textContent = `${format(s.p05)}–${format(s.p95)}`;
  $('elapsed').textContent = t.elapsedMs < 1000 ? `${format(t.elapsedMs)} ms` : `${format(t.elapsedMs / 1000, 2)} s`;
  $('throughput').textContent = `${format(c.iterations / (t.elapsedMs / 1000))} fights/s`;
  
  // Update Damage Split Segmented Bar
  const totalDmg = s.damage || 1;
  const shadowPct = Math.round(((s.shadowDamage || totalDmg) / totalDmg) * 100);
  const firePct = Math.round(((s.fireDamage || 0) / totalDmg) * 100);
  const petPct = Math.max(0, 100 - shadowPct - firePct);

  const splitBar = $('damage-split-fill');
  if (splitBar) {
    splitBar.innerHTML = '';
    if (shadowPct > 0) splitBar.innerHTML += `<div class="split-seg shadow" style="width:${shadowPct}%;">${shadowPct}% Shadow</div>`;
    if (firePct > 0) splitBar.innerHTML += `<div class="split-seg fire" style="width:${firePct}%;">${firePct}% Fire</div>`;
    if (petPct > 0) splitBar.innerHTML += `<div class="split-seg pet" style="width:${petPct}%;">${petPct}% Pet</div>`;
  }

  // Update Per-Spell Stats Breakdown Table
  const tbody = $('results-breakdown-body');
  if (tbody && s.spellBreakdown) {
    tbody.innerHTML = '';
    Object.entries(s.spellBreakdown).forEach(([spell, stats]) => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td style="color: var(--text-parchment);">${spell}</td>
        <td style="color: var(--text-gold); font-family: var(--font-mono);">${format(stats.damage || 0)}</td>
        <td style="font-family: var(--font-mono);">${stats.casts || 0}</td>
        <td style="color: #4ade80; font-family: var(--font-mono);">${stats.crits || 0}</td>
      `;
      tbody.appendChild(tr);
    });
  }
}

// Full Preset Loader Function
export function loadFullPreset(preset) {
  if (!preset) return;

  // 1. Load 51-point talents
  if (preset.talents) {
    applyTalentsObject(preset.talents);
  }

  // 2. Load Race and racials
  if (preset.race) {
    setRace(preset.race.toUpperCase());
  }

  // 3. Load Pet and Demonic Sacrifice
  const { pet, sac } = getPresetPetAndSac(preset);
  setPet(pet);
  setDS(sac);

  // 4. Determine and set active rotation
  let rot = 'shadow';
  const nameLower = (preset.name || '').toLowerCase();
  const rotLower = (preset.rotation || '').toLowerCase();
  if (nameLower.includes('searing') || rotLower.includes('searing') || nameLower.includes('dp fire') || nameLower.includes('dp_fire')) {
    rot = 'searing';
  } else if (nameLower.includes('incinerate') || nameLower.includes('fire') || rotLower.includes('fire') || rotLower.includes('incinerate')) {
    rot = 'fire';
  } else if (nameLower.includes('bolt only') || nameLower.includes('pure shadow bolt')) {
    rot = 'bolt';
  }
  setRotation(rot);

  // 5. Load APL Rotation tailored to this preset
  setAPLPreset(preset.name, preset.talents);

  // 6. Update Summary Views (preserves user's current configuration stats)
  updateCombatStatsSummary();
  setStatus(`Loaded preset: ${preset.name} (using current configured stats). Ready to simulate.`);
}

function getActiveStatsConfig() {
  const sp = Number($('in-spellPower')?.value || 500);
  const shadowSp = Number($('in-shadowPower')?.value || 0);
  const fireSp = Number($('in-firePower')?.value || 0);
  const hit = Number($('in-hit')?.value || 12);
  const crit = Number($('in-crit')?.value || 15);
  const int = Number($('in-intellect')?.value || 200);
  const stam = Number($('in-stamina')?.value || 220);
  const spirit = Number($('in-spirit')?.value || 100);
  const mp5 = Number($('in-mp5')?.value || 20);
  const duration = Number(form.elements.namedItem('duration')?.value || 180);
  const distance = Number(form.elements.namedItem('distance')?.value || 30);
  const resistance = Number(form.elements.namedItem('resistance')?.value || 0);
  const penetration = Number(form.elements.namedItem('penetration')?.value || 0);
  const tapThreshold = Number(form.elements.namedItem('tapThreshold')?.value || 25);
  return {
    spellPower: sp, shadowPower: shadowSp, firePower: fireSp,
    hit, crit, intellect: int, stamina: stam, spirit, mp5,
    duration, distance, resistance, penetration, tapThreshold
  };
}

// The active talent allocation is translated into supported simulator fields
// when readForm() builds each fight config.
initTalents();

initAPL((aplList) => {
  const enabledSpells = aplList.filter(e => e.enabled).map(e => e.spell);
  console.log('APL updated:', enabledSpells);
});

// Populate Preset Dropdown Dynamically
async function populateTalentPresetsDropdown() {
  const select = $('talent-preset-select');
  if (!select) return;
  const presets = await getPresets();
  if (!presets || presets.length === 0) return;

  select.innerHTML = '<option value="">Load Preset…</option>';
  presets.forEach(p => {
    const opt = document.createElement('option');
    opt.value = p.id;
    opt.textContent = p.name;
    select.appendChild(opt);
  });

  select.addEventListener('change', (e) => {
    const val = e.target.value;
    if (!val) return;
    const found = presets.find(p => String(p.id) === String(val) || p.name === val);
    if (found) {
      loadFullPreset(found);
    }
  });
}

$('btn-reset-talents')?.addEventListener('click', () => {
  resetTalents();
});
$('btn-reset-talents-2')?.addEventListener('click', () => {
  resetTalents();
});

initBuffs((buffStats) => {
  const badge = $('buffs-summary-badge');
  if (badge) badge.textContent = `+${buffStats.addedSpellPower} SP · +${buffStats.addedIntellect} Int · +${buffStats.addedCrit}% Crit`;
  updateCombatStatsSummary();
});

initGear((gearStats) => {
  if ($('btn-mode-equipped')?.classList.contains('active')) {
    applyEquippedStatsToForm(gearStats);
  }
  updateCombatStatsSummary();
});

initPresets((selectedPreset) => {
  loadFullPreset(selectedPreset);
  // Switch to Current Configuration tab
  $('btn-current-build')?.click();
}, () => activeRace);

$('btn-batch-sim')?.addEventListener('click', async () => {
  if (controller) return;
  controller = new AbortController();
  busy(true);
  try {
    setStatus('Running batch GPU simulation across standard meta presets with current stats…');
    const currentStats = getActiveStatsConfig();
    const results = await runBatchPresetSimulation(controller.signal, progress, (selectedPreset) => loadFullPreset(selectedPreset), currentStats);
    setStatus('Batch meta preset simulation complete! Leaderboard updated with live GPU results.');
    const numSims = Number(document.getElementById('compare-num-sims')?.value || 3000);
    const totalFights = results.length * numSims;
    const elapsed = results[0]?.result?.timing?.elapsedMs || 60;
    const tp = totalFights / (Math.max(1, elapsed) / 1000);
    setTopStatus('done', { fights: totalFights, throughput: tp, elapsedMs: elapsed });
  } catch (err) {
    setStatus(err.message, true);
    if (err.name !== 'AbortError') setTopStatus('error', { message: err.message });
  } finally {
    controller = null;
    busy(false);
  }
});

// CPU Parity Validation
$('validate')?.addEventListener('click', async () => {
  if (controller) return;
  controller = new AbortController();
  busy(true);
  $('validation-results').replaceChildren();
  try {
    const response = await fetch(new URL('../validation/cpu-fixtures.json', import.meta.url));
    if (!response.ok) throw new Error('CPU fixtures could not be loaded.');
    const { cases } = await response.json();
    let passed = 0;
    for (let i = 0; i < cases.length; i++) {
      const fixture = cases[i];
      $('validation-status').textContent = `Checking ${i + 1} / ${cases.length}: ${fixture.name}`;
      const result = await runSimulation(fixture.config, { signal: controller.signal });
      const check = compare(result, fixture);
      passed += +check.pass;
      const row = document.createElement('div');
      row.className = `check-row ${check.pass ? 'pass' : 'fail'}`;
      const name = document.createElement('span'), outcome = document.createElement('span');
      name.textContent = fixture.name;
      outcome.textContent = check.pass ? 'PASS' : check.failures.join('; ');
      row.append(name, outcome);
      $('validation-results').append(row);
      progress({ phase: 'CPU checks', completed: i + 1, total: cases.length });
    }
    $('validation-status').textContent = `${passed} / ${cases.length} passed on this device.`;
    setStatus(`CPU reference checks: ${passed} / ${cases.length} passed.`, passed !== cases.length);
    setTopStatus('done', { fights: cases.length, throughput: cases.length / 0.5, elapsedMs: 500 });
  } catch (error) {
    $('validation-status').textContent = error.message;
    setStatus(error.message, error.name !== 'AbortError');
    if (error.name !== 'AbortError') setTopStatus('error', { message: error.message });
  } finally {
    controller = null;
    busy(false);
  }
});

// Initial Setup
initTooltips();
setupInteractiveSlots();
updateRacialsDisplay();
updateCombatStatsSummary();
populateTalentPresetsDropdown();
