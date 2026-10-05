import { DEFAULTS, SPELLS } from './model.js';
import { buildFightConfig } from './config_builder.js';
import { runSimulation, preloadShader } from './engine.js';
import { compare } from '../validation/compare.js';
import { initTalents, applyTalentsObject, applyTalentPreset, resetTalents, getSimTalentFlags } from './talents.js';
import { initBuffs, getActiveBuffStats } from './buffs.js';
import { initPresets, getPresets, getPresetPetAndSac, runBatchPresetSimulation, renderPresetsLeaderboard } from './presets.js';
import { initGear, setGearMode, calculateEquippedStats } from './gear.js';
import { refreshAPLFallback, initAPL, setAPLPreset, getActiveAPL, getActiveBytecodeRules, applySynthesizedAPL } from './apl.js';
import { initTooltips, showTooltip, hideTooltip } from './tooltips.js';
import { initConstrainedSearchView, readGAConfig, updateGALiveView, setGAExecutionResults } from './constrained_search_view.js';
import { runConstrainedGeneticSearch } from './genetic_optimizer.js';
import { initAPLSynthesisView, readAPLGAConfig, updateAPLGALiveView, setAPLGAExecutionResults } from './apl_synthesis_view.js';
import { runAPLGeneticSynthesis } from './apl_genetic_optimizer.js';

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

// Check WebGL2 Support
let capable = false;
try {
  const probe = document.createElement('canvas').getContext('webgl2');
  capable = !!probe;
  probe?.getExtension('WEBGL_lose_context')?.loseContext();
} catch {
  capable = false;
}

if ($('gpu-badge')) {
  $('gpu-badge').innerHTML = capable
    ? '<span class="status-dot">●</span> WebGL2 Active'
    : '<span class="status-dot" style="color:#ef4444;">●</span> WebGL2 Unavailable';
}
if ($('top-sim-status')) {
  $('top-sim-status').textContent = 'Ready to simulate';
}
if (!capable) {
  if ($('run')) $('run').disabled = true;
  if ($('validate')) $('validate').disabled = true;
  if ($('status')) $('status').textContent = 'WebGL2 is unavailable. Enable hardware acceleration or try another browser.';
}

// Navigation Tabs Matching Desktop App
export const tabs = [
  { btn: 'btn-current-build', pane: 'tab-current-build' },
  { btn: 'btn-compare-specs', pane: 'tab-compare-specs' },
  { btn: 'btn-constrained-search', pane: 'tab-constrained-search' },
  { btn: 'btn-apl-synthesis', pane: 'tab-apl-synthesis' }
];

export function switchTab(targetId) {
  const match = tabs.find(t => t.btn === targetId || t.pane === targetId);
  const activeBtnId = match ? match.btn : 'btn-current-build';
  tabs.forEach(t => {
    const b = $(t.btn);
    const p = $(t.pane);
    if (b && p) {
      const isActive = (t.btn === activeBtnId);
      b.classList.toggle('active', isActive);
      b.setAttribute('aria-selected', String(isActive));
      p.hidden = !isActive;
      p.classList.toggle('active', isActive);
      p.style.display = isActive ? 'block' : 'none';
    }
  });
}

tabs.forEach(({ btn }) => {
  $(btn)?.addEventListener('click', (e) => {
    e?.preventDefault();
    switchTab(btn);
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
      { name: 'Blood Fury', icon: 'Racial_Orc_BerserkerStrength.png', title: 'Blood Fury (+10% Spell Power)', desc: 'Increases base spell power by 10% for 15 sec. 120 sec cooldown; aligned with Bane of Doom when available.' },
      { name: 'Command', icon: 'Ability_Warrior_WarCry.png', title: 'Command (+5% Pet Damage)', desc: 'Damage dealt by Warlock and Hunter pets increased by 5%.' }
    ]
  },
  UNDEAD: {
    icon: './assets/icons/Achievement_Character_Undead_Male.png',
    traits: [
      { name: 'Will of the Forsaken', icon: 'Spell_Shadow_RaiseDead.png', title: 'Will of the Forsaken (Charm/Fear/Sleep Immunity)', desc: 'Provides immunity to Charm, Fear, and Sleep effects for 5 sec.' },
      { name: 'Touch of the Grave', icon: 'Spell_Shadow_ChillTouch.png', title: 'Touch of the Grave', desc: 'Spell casts have a 10% chance to deal 5% of maximum health as damage. 1 sec cooldown.' },
      { name: 'Cannibalize', icon: 'Spell_Shadow_Cannibalize.png', title: 'Cannibalize', desc: 'When activated, regenerates 7% of total health every 2 sec for 10 sec.' }
    ]
  },
  TROLL: {
    icon: './assets/icons/Achievement_Character_Troll_Male.png',
    traits: [
      { name: 'Berserking', icon: 'Racial_Troll_Berserk.png', title: 'Berserking (+10% Haste)', desc: 'Increases casting speed by 10% for 10 sec. 180 sec cooldown.' },
      { name: 'Beast Slaying', icon: 'Ability_Hunter_BeastSoothe.png', title: 'Beast Slaying (+5% vs Beasts)', desc: 'Damage dealt versus Beasts increased by 5%.' }
    ]
  },
  GNOME: {
    icon: './assets/icons/Achievement_Character_Gnome_Male.png',
    traits: [
      { name: 'Expansive Mind', icon: 'Spell_Nature_EnchantWater.png', title: 'Expansive Mind (+5% Intellect)', desc: 'Intellect increased by 5%.' },
      { name: 'Eureka!', icon: 'Spell_Nature_EnchantWater.png', title: 'Eureka! (+10% Damage, -10% Mana)', desc: 'Empowers the next 3 spells. 120 sec cooldown; aligned with Bane of Doom when available.' },
      { name: 'Escape Artist', icon: 'Spell_Holy_Silence.png', title: 'Escape Artist', desc: 'Escape the effects of any immobilization or movement speed reduction.' }
    ]
  }
};

const PET_ICONS_DATA = {
  imp: './assets/icons/Spell_Shadow_SummonImp.png',
  succubus: './assets/icons/Spell_Shadow_SummonSuccubus.png',
  none: null
};

const DS_ICONS_DATA = {
  succubus: './assets/icons/Spell_Shadow_SummonSuccubus.png',
  imp: './assets/icons/Spell_Shadow_SummonImp.png',
  none: null
};

let activeRace = 'HUMAN';
let activePet = 'imp';
let activeDS = 'none';
let activeRotation = 'shadow';

export function setRotation(rot) {
  const norm = (rot || 'shadow').toLowerCase();
  activeRotation = ['shadow', 'fire', 'searing', 'bolt'].includes(norm) ? norm : 'shadow';
  refreshAPLFallback();
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
  const icon = $('slot-pet-icon');
  if (icon) {
    icon.style.display = PET_ICONS_DATA[activePet] ? 'block' : 'none';
    if (PET_ICONS_DATA[activePet]) icon.src = PET_ICONS_DATA[activePet];
  }
  updateCombatStatsSummary();
}

export function setDS(dsKey) {
  const norm = (dsKey || 'none').toLowerCase();
  activeDS = DS_ICONS_DATA[norm] ? norm : 'none';
  const icon = $('slot-ds-icon');
  if (icon) {
    icon.style.display = DS_ICONS_DATA[activeDS] ? 'block' : 'none';
    if (DS_ICONS_DATA[activeDS]) icon.src = DS_ICONS_DATA[activeDS];
  }
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
      const dsName = activeDS === 'imp' ? 'Imp (+15% Shadow Damage)' : activeDS === 'succubus' ? 'Succubus (+15% Fire Damage)' : 'No Demonic Sacrifice';
      const desc = activeDS === 'imp' ? 'Sacrifices your Imp to increase Shadow damage by 15%.' : activeDS === 'succubus' ? 'Sacrifices your Succubus to increase Fire damage by 15%.' : 'Demonic Sacrifice buff is currently not active.';
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

function getEffectiveBuffsAndStats() {
  const sp = Number($('in-spellPower')?.value || 500);
  const shadowSpBonus = Number($('in-shadowPower')?.value || 0);
  const fireSpBonus = Number($('in-firePower')?.value || 0);
  const hit = Number($('in-hit')?.value || 12);
  const crit = Number($('in-crit')?.value || 15);
  const haste = Number($('in-haste')?.value || 0);
  const baseInt = Number($('in-intellect')?.value || 200);
  const baseStam = Number($('in-stamina')?.value || 220);
  const baseSpirit = Number($('in-spirit')?.value || 100);
  const baseMp5 = Number($('in-mp5')?.value || 20);
  const baseResistance = Number($('in-resistance')?.value || 0);
  const bossArmor = Number($('in-boss-armor')?.value || 3731);

  // Read checkboxes from the form
  const isFlaskSupreme = !!form.elements.namedItem('flaskSupremePower')?.checked;
  const isGreaterArcane = !!form.elements.namedItem('greaterArcaneElixir')?.checked;
  const isShadowPowerElixir = !!form.elements.namedItem('shadowPowerElixir')?.checked;
  const isGreaterFirepowerElixir = !!form.elements.namedItem('greaterFirepowerElixir')?.checked;
  const isWizardOil = !!form.elements.namedItem('wizardOil')?.checked;
  const isMageblood = !!form.elements.namedItem('magebloodElixir')?.checked;
  const isNightfin = !!form.elements.namedItem('nightfinSoup')?.checked;

  const isArcaneIntellect = !!form.elements.namedItem('arcaneIntellect')?.checked;
  const isMarkOfTheWild = !!form.elements.namedItem('markOfTheWild')?.checked;
  const isBlessingOfKings = !!form.elements.namedItem('blessingOfKings')?.checked;
  const isBlessingOfWisdom = !!form.elements.namedItem('blessingOfWisdom')?.checked;
  const isManaSpringTotem = !!form.elements.namedItem('manaSpringTotem')?.checked;

  const isDragonslayer = !!form.elements.namedItem('dragonslayer')?.checked;
  const isSongflower = !!form.elements.namedItem('songflower')?.checked;
  const isWarchiefsBlessing = !!form.elements.namedItem('warchiefsBlessing')?.checked;
  const isSpiritOfZandalar = !!form.elements.namedItem('spiritOfZandalar')?.checked;
  const isSaygesFortune = !!form.elements.namedItem('saygesFortune')?.checked;

  const isCurseOfShadow = !!form.elements.namedItem('curseOfShadow')?.checked;
  const isCurseOfElements = !!form.elements.namedItem('curseOfElements')?.checked;
  const isShadowWeaving = !!form.elements.namedItem('shadowWeaving')?.checked;
  const isImprovedScorch = !!form.elements.namedItem('improvedScorch')?.checked;
  const isNightfallDebuff = !!form.elements.namedItem('nightfallProcDebuff')?.checked;

  // Stat Additions
  let addedSP = 0;
  if (isFlaskSupreme) addedSP += 150;
  if (isGreaterArcane) addedSP += 35;
  if (isWizardOil) addedSP += 36;

  let addedShadowSP = isShadowPowerElixir ? 40 : 0;
  let addedFireSP = isGreaterFirepowerElixir ? 40 : 0;

  let addedInt = 0, addedStam = 0, addedSpirit = 0;
  if (isArcaneIntellect) addedInt += 31;
  if (isMarkOfTheWild) { addedInt += 12; addedStam += 12; addedSpirit += 12; }
  if (isSongflower) { addedInt += 15; addedStam += 15; addedSpirit += 15; }

  let statMultiplier = 1.0;
  if (isBlessingOfKings) statMultiplier *= 1.10;
  if (isSpiritOfZandalar) statMultiplier *= 1.15;

  let finalInt = (baseInt + addedInt) * statMultiplier;
  let finalStam = (baseStam + addedStam) * statMultiplier;
  let finalSpirit = (baseSpirit + addedSpirit) * statMultiplier;

  // Racial modifiers
  if (activeRace === 'GNOME') finalInt *= 1.05;
  if (activeRace === 'HUMAN') finalSpirit *= 1.05;

  finalInt = Math.round(finalInt);
  finalStam = Math.round(finalStam);
  finalSpirit = Math.round(finalSpirit);

  let addedCrit = 0;
  if (isWizardOil) addedCrit += 1.0;
  if (isDragonslayer) addedCrit += 10.0;
  if (isSongflower) addedCrit += 5.0;
  const finalCrit = crit + addedCrit;

  let addedMP5 = 0;
  if (isMageblood) addedMP5 += 12;
  if (isNightfin) addedMP5 += 8;
  if (isBlessingOfWisdom) addedMP5 += 30;
  if (isManaSpringTotem) addedMP5 += 25;
  if (isWarchiefsBlessing) addedMP5 += 10;
  const finalMP5 = baseMp5 + addedMP5;

  const finalSP = sp + addedSP;
  const finalShadowSP = shadowSpBonus + addedShadowSP;
  const finalFireSP = fireSpBonus + addedFireSP;

  const maxMana = Math.round(1400 + (finalInt * 15));
  const maxHealth = Math.round(1500 + (finalStam * 10) + (isWarchiefsBlessing ? 300 : 0));

  // Multipliers
  let allDamageMult = 1.0;
  if (isSaygesFortune) allDamageMult *= 1.10;
  if (isNightfallDebuff) allDamageMult *= 1.15;

  let shadowMult = allDamageMult;
  let fireMult = allDamageMult;

  if (isCurseOfShadow) shadowMult *= 1.10;
  if (isCurseOfElements) fireMult *= 1.10;
  if (isShadowWeaving) shadowMult *= 1.15;
  if (isImprovedScorch) fireMult *= 1.15;

  if (activeDS === 'imp') shadowMult *= 1.15;
  if (activeDS === 'succubus') fireMult *= 1.15;

  const finalResistance = Math.max(0, baseResistance - (isCurseOfShadow || isCurseOfElements ? 75 : 0));

  return {
    spellPower: finalSP,
    shadowPower: finalShadowSP,
    firePower: finalFireSP,
    hit,
    crit: finalCrit,
    haste,
    intellect: finalInt,
    stamina: finalStam,
    spirit: finalSpirit,
    mp5: finalMP5,
    maxMana,
    maxHealth,
    shadowMultiplier: shadowMult,
    fireMultiplier: fireMult,
    resistance: finalResistance,
    bossArmor
  };
}

function updateCombatStatsSummary() {
  refreshAPLFallback();
  const eff = getEffectiveBuffsAndStats();
  const effectiveShadow = eff.spellPower + eff.shadowPower;
  const effectiveFire = eff.spellPower + eff.firePower;

  if ($('sum-shadow-sp')) $('sum-shadow-sp').textContent = effectiveShadow;
  if ($('sum-fire-sp')) $('sum-fire-sp').textContent = effectiveFire;
  if ($('sum-max-mana')) $('sum-max-mana').textContent = format(eff.maxMana);
  if ($('sum-max-health')) $('sum-max-health').textContent = format(eff.maxHealth);
  if ($('sum-spell-hit')) $('sum-spell-hit').textContent = `${eff.hit.toFixed(1)}%`;
  if ($('sum-spell-crit')) $('sum-spell-crit').textContent = `${eff.crit.toFixed(1)}%`;
  if ($('sum-intellect')) $('sum-intellect').textContent = eff.intellect;
  if ($('sum-stamina')) $('sum-stamina').textContent = eff.stamina;
  if ($('sum-mp5')) $('sum-mp5').textContent = eff.mp5;
  if ($('sum-shadow-mult')) $('sum-shadow-mult').textContent = `${eff.shadowMultiplier.toFixed(2)}x`;
  if ($('sum-fire-mult')) $('sum-fire-mult').textContent = `${eff.fireMultiplier.toFixed(2)}x`;

  // Update Constrained Spec Search Subheader Base Stats
  if ($('ga-base-shadow-sp')) $('ga-base-shadow-sp').textContent = effectiveShadow;
  if ($('ga-base-fire-sp')) $('ga-base-fire-sp').textContent = effectiveFire;
  if ($('ga-base-hit')) $('ga-base-hit').textContent = `${eff.hit.toFixed(1)}%`;
  if ($('ga-base-crit')) $('ga-base-crit').textContent = `${eff.crit.toFixed(1)}%`;
  const dur = Number(form.elements.namedItem('duration')?.value || 180);
  if ($('ga-base-fight')) $('ga-base-fight').textContent = `${dur}s`;

  // Same live base inputs passed to runBatchPresetSimulation; presets add their talents.
  const comparisonStats = {
    'compare-base-shadow-sp': effectiveShadow,
    'compare-base-fire-sp': effectiveFire,
    'compare-base-hit': `${eff.hit.toFixed(1)}%`,
    'compare-base-crit': `${eff.crit.toFixed(1)}%`,
    'compare-base-mp5': eff.mp5,
    'compare-base-fight': `${dur}s`,
  };
  for (const [id, value] of Object.entries(comparisonStats)) {
    if ($(id)) $(id).textContent = value;
  }

  // Update APL Synthesis Subheader Base Stats & Target Spec Label
  if ($('apl-ga-base-shadow-sp')) $('apl-ga-base-shadow-sp').textContent = effectiveShadow;
  if ($('apl-ga-base-fire-sp')) $('apl-ga-base-fire-sp').textContent = effectiveFire;
  if ($('apl-ga-base-hit')) $('apl-ga-base-hit').textContent = `${eff.hit.toFixed(1)}%`;
  if ($('apl-ga-base-crit')) $('apl-ga-base-crit').textContent = `${eff.crit.toFixed(1)}%`;
  if ($('apl-ga-base-fight')) $('apl-ga-base-fight').textContent = `${dur}s`;
  if ($('apl-ga-active-spec-label')) {
    const tf = getSimTalentFlags();
    const aff = tf.shadowMasteryBonus > 0 ? 'Aff' : '';
    const demo = tf.demonicPact ? 'DP' : tf.masterDemo > 0 ? 'MD' : tf.sacSucc ? 'DS' : '';
    const destro = tf.incinerate ? 'Incin' : tf.ruinRank > 0 ? 'Ruin' : '';
    const specStr = [aff, demo, destro].filter(Boolean).join('/') || activeRotation.toUpperCase();
    $('apl-ga-active-spec-label').textContent = `${activeRace} · ${specStr} · ${activePet !== 'none' ? activePet : activeDS !== 'none' ? `DS ${activeDS}` : 'No Pet'}`;
  }
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

form.addEventListener('input', () => { importedConfig = null; updateCombatStatsSummary(); });
form.addEventListener('change', () => { importedConfig = null; updateCombatStatsSummary(); });

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

  const eff = getEffectiveBuffsAndStats();
  base.spellPower = eff.spellPower;
  base.shadowPower = eff.shadowPower;
  base.firePower = eff.firePower;
  base.hit = eff.hit;
  base.crit = eff.crit;
  base.intellect = eff.intellect;
  base.stamina = eff.stamina;
  base.spirit = eff.spirit;
  base.mp5 = eff.mp5;
  base.resistance = eff.resistance;
  base.bossArmor = eff.bossArmor;
  base.shadowMultiplier = eff.shadowMultiplier;
  base.fireMultiplier = eff.fireMultiplier;

  base.race = activeRace;
  base.maxHealth = eff.maxHealth;
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
    if (engineEl) engineEl.innerHTML = '<span class="status-dot">●</span> WebGL2 Active';
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

const SPELL_ICONS_MAP = {
  'Shadow Bolt': 'Spell_Shadow_ShadowBolt.png',
  'Corruption': 'Spell_Shadow_AbominationExplosion.png',
  'Bane of Doom': 'spell_shadow_auraofdarkness.png',
  'Soul Fire': 'spell_fire_fireball02.png',
  'Conflagrate': 'Spell_Fire_Fireball.png',
  'Shadowburn': 'Spell_Shadow_ScourgeBuild.png',
  'Siphon Life': 'Spell_Shadow_Requiem.png',
  'Touch of the Grave': 'spell_shadow_chilltouch.png',
  'Bane of Agony': 'Spell_Shadow_CurseOfSargeras.png',
  'Immolate': 'Spell_Fire_Immolation.png',
  'Incinerate': 'Spell_Fire_Burnout.png',
  'Searing Pain': 'Spell_Fire_SoulBurn.png',
  'Succubus Melee': 'Ability_MeleeDamage.png',
  'Melee (Pet)': 'Ability_MeleeDamage.png',
  'Pet Melee': 'Ability_MeleeDamage.png',
  'Melee': 'Ability_MeleeDamage.png',
  'Succubus Lash of Pain': 'Spell_Shadow_Curse.png',
  'Lash of Pain (Pet)': 'Spell_Shadow_Curse.png',
  'Imp Firebolt': 'Spell_Fire_FireBolt.png',
  'Firebolt (Pet)': 'Spell_Fire_FireBolt.png',
  'Demonic Brand': 'ability_demonhunter_chaoticimprint_fire.png',
  'Hellfire': 'Spell_Fire_Incinerate.png',
  'Wrack': 'ability_deathknight_hemorrhagicfever.png',
  'Pet': 'Spell_Shadow_SummonImp.png',
  'Imp': 'Spell_Shadow_SummonImp.png',
  'Succubus': 'Spell_Shadow_SummonSuccubus.png',
  'Life Tap': 'Spell_Shadow_BurningSpirit.png'
};

function busy(active) {
  if ($('inputs')) $('inputs').disabled = active;
  if ($('run')) $('run').disabled = active || !capable;
  if ($('validate')) $('validate').disabled = active || !capable;
  const batchBtn = $('btn-batch-sim');
  if (batchBtn) batchBtn.disabled = active || !capable;
  if ($('cancel')) {
    $('cancel').hidden = !active;
    $('cancel').disabled = false;
  }
  
  const progContainer = $('progress-container');
  if (progContainer) progContainer.style.display = active ? 'block' : 'none';
  const topProgContainer = $('topbar-progress-container');
  if (topProgContainer && !active) {
    topProgContainer.style.display = 'none';
  }
  if ($('export')) $('export').disabled = active || !currentResult;
}

function progress(p) {
  const pct = Math.round((p.completed / p.total) * 100);
  const fill = $('progress-bar-fill');
  const text = $('progress-text');
  if (fill) fill.style.width = `${pct}%`;
  if (text) text.textContent = `${p.phase}: ${format(p.completed)} / ${format(p.total)} (${pct}%)`;

  const topProgContainer = $('topbar-progress-container');
  const topFill = $('topbar-progress-fill');
  const topText = $('topbar-progress-text');
  if (topProgContainer) topProgContainer.style.display = 'block';
  if (topFill) topFill.style.width = `${pct}%`;
  if (topText) topText.textContent = `${pct}%`;

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
    currentResult = await runSimulation(config, { signal: controller.signal, onProgress: progress, detailedResults: $('detailed-results')?.checked !== false });
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
  const totalDmg = s.damage || (s.mean * c.duration) || 1;
  const shadowDmg = s.shadowDamage || 0;
  const fireDmg = s.fireDamage || 0;
  const physicalDmg = s.physicalDamage || s.petMeleeDamage || 0;
  
  const shadowPct = s.detailed ? Math.round((shadowDmg / totalDmg) * 100) : 0;
  const firePct = s.detailed ? Math.round((fireDmg / totalDmg) * 100) : 0;
  const physPct = s.detailed ? Math.round((physicalDmg / totalDmg) * 100) : 0;

  const splitBar = $('current-sim-damage-split');
  if (splitBar) {
    splitBar.innerHTML = '';
    if (shadowPct > 0) splitBar.innerHTML += `<div class="split-seg shadow" style="width:${shadowPct}%;" title="Shadow: ${shadowPct}% (${format(shadowDmg / c.duration, 1)} DPS)">${shadowPct}% Shadow</div>`;
    if (firePct > 0) splitBar.innerHTML += `<div class="split-seg fire" style="width:${firePct}%;" title="Fire: ${firePct}% (${format(fireDmg / c.duration, 1)} DPS)">${firePct}% Fire</div>`;
    if (physPct > 0) splitBar.innerHTML += `<div class="split-seg physical" style="width:${physPct}%;" title="Physical (Melee): ${physPct}% (${format(physicalDmg / c.duration, 1)} DPS)">${physPct}% Physical</div>`;
  }

  // Update Per-Spell Stats Breakdown Table
  const tbody = $('breakdown');
  if (tbody) {
    tbody.innerHTML = '';
    if (!s.detailed) {
      tbody.innerHTML = '<tr><td colspan="6" style="text-align:center;color:var(--text-dim);padding:0.75rem;">Fast mode was used. Enable Detailed spell breakdown to collect per-spell results.</td></tr>';
    }
    const activeSpells = (s.spells || []).filter(sp => (sp.damage > 0 || sp.casts > 0));
    
    // Find max damage for relative progress bar scaling
    const maxSpellDmg = Math.max(...activeSpells.map(sp => sp.damage), 1);

    activeSpells.forEach(sp => {
      const spellDps = sp.damage / c.duration;
      const pctOfTotal = totalDmg > 0 ? (sp.damage / totalDmg * 100).toFixed(1) : '0.0';
      const critPct = sp.hits > 0 ? ((sp.crits / sp.hits) * 100).toFixed(1) : '0.0';
      const icon = SPELL_ICONS_MAP[sp.name] || 'Spell_Shadow_ShadowBolt.png';
      const isFire = ['Immolate', 'Incinerate', 'Searing Pain', 'Imp Firebolt', 'Demonic Brand'].includes(sp.name) || sp.school === 'fire';
      const isPhys = ['Succubus Melee', 'Melee (Pet)', 'Pet Melee', 'Melee'].includes(sp.name) || sp.school === 'physical';
      const barClass = isPhys ? 'physical' : (isFire ? 'fire' : 'shadow');
      const barWidth = Math.min(100, Math.max(4, Math.round((sp.damage / maxSpellDmg) * 100)));

      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td>
          <div class="spell-breakdown-cell">
            <img src="./assets/icons/${icon}" class="spell-breakdown-icon" alt="${sp.name}" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
            <span class="spell-breakdown-name" ${sp.name === 'Hellfire' ? `title="Self damage: ${format(sp.selfDamage, 0)} (excluded from enemy DPS)"` : ''}>${sp.name}</span>
          </div>
        </td>
        <td style="text-align: right; font-family: var(--font-mono); color: var(--text-gold); font-weight: 600; white-space: nowrap;">
          ${format(sp.damage, 0)} <span style="font-size: 0.7rem; color: var(--text-dim); white-space: nowrap;">(${pctOfTotal}%)</span>
        </td>
        <td style="text-align: right; font-family: var(--font-mono); color: #4ade80; font-weight: 700; white-space: nowrap;">
          ${format(spellDps, 1)}
        </td>
        <td style="text-align: right; font-family: var(--font-mono); color: var(--text-parchment); white-space: nowrap;">
          ${sp.casts > 0 ? sp.casts.toFixed(1) : '-'}
        </td>
        <td style="text-align: right; font-family: var(--font-mono); color: #fbbf24; white-space: nowrap;">
          ${sp.casts > 0 ? `${sp.crits.toFixed(1)} <span style="font-size: 0.7rem; color: var(--text-dim); white-space: nowrap;">(${critPct}%)</span>` : '-'}
        </td>
        <td>
          <div class="spell-mini-bar-track">
            <div class="spell-mini-bar-fill ${barClass}" style="width: ${barWidth}%;"></div>
          </div>
        </td>
      `;
      tbody.appendChild(tr);
    });

    if (activeSpells.length === 0) {
      tbody.innerHTML = '<tr><td colspan="6" style="text-align: center; color: var(--text-dim); padding: 0.75rem;">No damage dealt in simulation. Check your APL rules and mana stats.</td></tr>';
    }
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
  const eff = getEffectiveBuffsAndStats();
  const duration = Number(form.elements.namedItem('duration')?.value || 180);
  const distance = Number(form.elements.namedItem('distance')?.value || 30);
  const penetration = Number(form.elements.namedItem('penetration')?.value || 0);
  const tapThreshold = Number(form.elements.namedItem('tapThreshold')?.value || 25);
  return {
    racialPolicy: form.elements.namedItem('racialPolicy')?.value || 'execute',
    targetIsBeast: !!form.elements.namedItem('targetIsBeast')?.checked,
    race: activeRace,
    maxHealth: eff.maxHealth,
    spellPower: eff.spellPower,
    shadowPower: eff.shadowPower,
    firePower: eff.firePower,
    hit: eff.hit,
    crit: eff.crit,
    haste: eff.haste,
    intellect: eff.intellect,
    stamina: eff.stamina,
    spirit: eff.spirit,
    mp5: eff.mp5,
    resistance: eff.resistance,
    bossArmor: eff.bossArmor,
    shadowMultiplier: eff.shadowMultiplier,
    fireMultiplier: eff.fireMultiplier,
    duration,
    distance,
    penetration,
    tapThreshold
  };
}

// The active talent allocation is translated into supported simulator fields
// when readForm() builds each fight config.
initTalents((tf) => {
  updateCombatStatsSummary();
});

initAPL((aplList) => {
  importedConfig = null;
  const enabledSpells = aplList.filter(e => e.enabled).map(e => e.spell);
  console.log('APL updated:', enabledSpells);
}, () => importedConfig?.rotation || activeRotation);

// Populate Preset Dropdown Dynamically
async function populateTalentPresetsDropdown() {
  const select = $('talent-preset-select');
  if (!select) return;
  const presets = await getPresets();
  if (!presets || presets.length === 0) return;

  select.innerHTML = '<option value="">Load Preset…</option>';
  let defaultPresetId = '';
  presets.forEach(p => {
    const opt = document.createElement('option');
    opt.value = p.id;
    opt.textContent = p.name;
    const nameLower = p.name.toLowerCase();
    if ((nameLower.includes('dp fire') || (nameLower.includes('dp') && nameLower.includes('searing'))) && (p.race || '').toLowerCase() === activeRace.toLowerCase()) {
      defaultPresetId = String(p.id);
    }
    select.appendChild(opt);
  });

  if (defaultPresetId) {
    select.value = defaultPresetId;
  }

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
  switchTab('btn-current-build');
}, () => activeRace);

$('btn-batch-sim')?.addEventListener('click', async (e) => {
  e?.preventDefault();
  if (controller) return;
  controller = new AbortController();
  busy(true);
  try {
    setStatus('Running batch GPU simulation across standard meta presets with current stats…');
    const currentStats = getActiveStatsConfig();
    const results = await runBatchPresetSimulation(controller.signal, progress, (selectedPreset) => loadFullPreset(selectedPreset), currentStats);
    const numSims = Number(document.getElementById('compare-num-sims')?.value || 1000);
    const totalFights = results.totalFights || (results.length * numSims);
    const elapsed = results.timing?.elapsedMs || 300;
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

// Constrained Spec Search Execution & Candidate Application
let gaController = null;

function applyCandidateBuild(cand) {
  if (!cand) return;
  try {
    // An imported config takes precedence in readForm(). Applying a search
    // candidate updates the live controls/state, so discard that override or
    // the next simulation would continue using the imported build.
    importedConfig = null;

    // 1. Load 51-point talents
    if (cand.talents) {
      applyTalentsObject(cand.talents);
    }

    // 2. Load Race and racials
    if (cand.race) {
      setRace(cand.race.toUpperCase());
    }

    // 3. Load Pet and Demonic Sacrifice
    const pet = cand.pet || 'none';
    const sac = cand.sacImp ? 'imp' : cand.sacSuccubus ? 'succubus' : (cand.sac || 'none');
    setPet(pet);
    setDS(sac);

    // 4. Determine and set active rotation
    let rot = 'shadow';
    const rotLower = (cand.rotation || '').toLowerCase();
    const nameLower = (cand.name || '').toLowerCase();
    if (rotLower.includes('searing') || rotLower === 'dp_af_fire' || nameLower.includes('searing') || nameLower.includes('dp fire')) {
      rot = 'searing';
    } else if (rotLower.includes('incinerate') || rotLower === 'fire_destro' || rotLower === 'incinerate_decimation' || nameLower.includes('incinerate') || nameLower.includes('fire')) {
      rot = 'fire';
    } else if (nameLower.includes('bolt only') || nameLower.includes('pure shadow bolt')) {
      rot = 'bolt';
    }
    setRotation(rot);

    if ($('talent-preset-select')) $('talent-preset-select').value = '';

    // 5. Load APL Rotation tailored to this evolved candidate spec
    setAPLPreset(cand.name, cand.talents, cand.rotation, cand.sacSuccubus || sac === 'succubus');

    // 6. Update Summary Views
    updateCombatStatsSummary();
    setStatus(`Applied evolved spec: ${cand.name}. Switched to Current Configuration.`);
    switchTab('btn-current-build');
  } catch (err) {
    console.error('Error applying candidate build:', err);
    setStatus(`Error applying candidate spec: ${err.message}`, true);
  }
}

initConstrainedSearchView((cand) => {
  applyCandidateBuild(cand);
});

$('btn-run-ga')?.addEventListener('click', async (e) => {
  e?.preventDefault();
  if (gaController) return;
  gaController = new AbortController();

  const runBtn = $('btn-run-ga');
  const stopBtn = $('btn-stop-ga');
  if (runBtn) runBtn.style.display = 'none';
  if (stopBtn) stopBtn.style.display = 'inline-block';

  const progContainer = $('topbar-progress-container');
  const progFill = $('topbar-progress-fill');
  const progText = $('topbar-progress-text');
  if (progContainer) progContainer.style.display = 'block';

  try {
    setStatus('Running genetic algorithm spec search across multi-config GPU shader simulation…');
    setTopStatus('compiling');

    const baseStats = getActiveStatsConfig();
    baseStats.race = activeRace;
    const gaConfig = readGAConfig();
    gaConfig.presetsList = getPresets();

    const startTime = performance.now();
    const results = await runConstrainedGeneticSearch(baseStats, gaConfig, {
      signal: gaController.signal,
      onProgress: (p) => {
        const pct = Math.round((p.completed / Math.max(1, p.total)) * 100);
        if (progFill) progFill.style.width = `${pct}%`;
        if (progText) progText.textContent = `${pct}%`;
        setStatus(`${p.phase} (${p.completed}/${p.total})`);
        setTopStatus('in-progress', { completed: p.completed, total: p.total });
      },
      onGeneration: (genData) => {
        updateGALiveView(genData);
        const pct = Math.round((genData.gen / Math.max(1, genData.maxGens)) * 100);
        if (progFill) progFill.style.width = `${pct}%`;
        if (progText) progText.textContent = `${pct}%`;
        if ($('top-sim-status')) {
          $('top-sim-status').textContent = `Gen ${genData.gen}/${genData.maxGens} · Best ${genData.bestDps.toFixed(1)} DPS`;
        }
      }
    });

    const elapsedMs = performance.now() - startTime;
    const totalFights = results.totalSimulations || (results.totalEvaluations * (gaConfig.screeningSims || 100));
    const tp = Math.round(totalFights / (Math.max(1, elapsedMs) / 1000));

    setGAExecutionResults(results);
    setStatus(`Genetic optimization completed (${totalFights.toLocaleString()} total fights across ${results.totalEvaluations.toLocaleString()} evaluated candidate specs).`);
    setTopStatus('done', { fights: totalFights, throughput: tp, elapsedMs });
  } catch (err) {
    if (err.name === 'AbortError') {
      setStatus('Genetic optimization stopped by user. Best discovered candidates preserved.');
    } else {
      setStatus(`Genetic optimization failed: ${err.message}`, true);
      setTopStatus('error', { message: err.message });
    }
  } finally {
    gaController = null;
    if (runBtn) runBtn.style.display = 'inline-block';
    if (stopBtn) stopBtn.style.display = 'none';
    if (progContainer) progContainer.style.display = 'none';
  }
});

$('btn-stop-ga')?.addEventListener('click', () => {
  if (gaController) {
    gaController.abort();
  }
});

// APL Synthesis Execution & Candidate Application
let aplGaController = null;

initAPLSynthesisView((cand) => {
  if (!cand) return;
  try {
    applySynthesizedAPL(cand.rules);
    updateCombatStatsSummary();
    setStatus(`Applied synthesized APL: ${cand.name}. Switched to Current Configuration.`);
    switchTab('btn-current-build');
  } catch (err) {
    console.error('Error applying synthesized APL:', err);
    setStatus(`Error applying synthesized APL: ${err.message}`, true);
  }
});

$('btn-run-apl-ga')?.addEventListener('click', async (e) => {
  e?.preventDefault();
  if (aplGaController) return;
  aplGaController = new AbortController();

  const runBtn = $('btn-run-apl-ga');
  const stopBtn = $('btn-stop-apl-ga');
  if (runBtn) runBtn.style.display = 'none';
  if (stopBtn) stopBtn.style.display = 'inline-block';

  const progContainer = $('topbar-progress-container');
  const progFill = $('topbar-progress-fill');
  const progText = $('topbar-progress-text');
  if (progContainer) progContainer.style.display = 'block';

  try {
    setStatus('Running genetic algorithm APL synthesis across GPU shader simulation…');
    setTopStatus('compiling');

    const baseStats = getActiveStatsConfig();
    baseStats.race = activeRace;
    baseStats.pet = activePet;
    baseStats.sac = activeDS;
    baseStats.talentFlags = getSimTalentFlags();

    const gaConfig = readAPLGAConfig();

    const startTime = performance.now();
    const results = await runAPLGeneticSynthesis(baseStats, gaConfig, {
      signal: aplGaController.signal,
      onProgress: (p) => {
        const pct = Math.round((p.completed / Math.max(1, p.total)) * 100);
        if (progFill) progFill.style.width = `${pct}%`;
        if (progText) progText.textContent = `${pct}%`;
        setStatus(`${p.phase} (${p.completed}/${p.total})`);
        setTopStatus('in-progress', { completed: p.completed, total: p.total });
      },
      onGeneration: (genData) => {
        updateAPLGALiveView(genData);
        const pct = Math.round((genData.gen / Math.max(1, genData.maxGens)) * 100);
        if (progFill) progFill.style.width = `${pct}%`;
        if (progText) progText.textContent = `${pct}%`;
        if ($('top-sim-status')) {
          $('top-sim-status').textContent = `APL Gen ${genData.gen}/${genData.maxGens} · Best ${genData.bestDps.toFixed(1)} DPS`;
        }
      }
    });

    const elapsedMs = performance.now() - startTime;
    const totalFights = results.totalSimulations || (results.totalEvaluations * (gaConfig.screeningSims || 100));
    const tp = Math.round(totalFights / (Math.max(1, elapsedMs) / 1000));

    setAPLGAExecutionResults(results);
    setStatus(`APL genetic synthesis completed (${totalFights.toLocaleString()} total fights across ${results.totalEvaluations.toLocaleString()} evaluated candidate APLs).`);
    setTopStatus('done', { fights: totalFights, throughput: tp, elapsedMs });
  } catch (err) {
    if (err.name === 'AbortError') {
      setStatus('APL genetic synthesis stopped by user. Best discovered APLs preserved.');
    } else {
      setStatus(`APL genetic synthesis failed: ${err.message}`, true);
      setTopStatus('error', { message: err.message });
    }
  } finally {
    aplGaController = null;
    if (runBtn) runBtn.style.display = 'inline-block';
    if (stopBtn) stopBtn.style.display = 'none';
    if (progContainer) progContainer.style.display = 'none';
  }
});

$('btn-stop-apl-ga')?.addEventListener('click', () => {
  if (aplGaController) {
    aplGaController.abort();
  }
});

// Initial Setup
initTooltips();
setupInteractiveSlots();
updateRacialsDisplay();
updateCombatStatsSummary();
populateTalentPresetsDropdown();

// Preload & compile WebGL2 shader asynchronously in the background on page load
if (capable) {
  if (typeof requestIdleCallback === 'function') {
    requestIdleCallback(() => preloadShader());
  } else {
    setTimeout(() => preloadShader(), 50);
  }
}
