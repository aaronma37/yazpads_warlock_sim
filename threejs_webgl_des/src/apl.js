// Authentic Action Priority List (APL) Engine & Interactive Manager
// Matches desktop ImGui simulator APL table with drag-and-drop, up/down reordering, condition editing, and presets.
import { APL_ACTION, APL_COND } from './model.js';

export const DEFAULT_APL = [
  { id: 'tap', spell: 'Life Tap', icon: 'Spell_Shadow_BurningSpirit.png', condition: 'Mana < 20%', rawCond: 'mana_pct < 20', enabled: true },
  { id: 'nightfall', spell: 'Nightfall: Shadow Bolt', icon: 'Spell_Shadow_Twilight.png', condition: 'Shadow Trance active', rawCond: 'buff.shadow_trance', enabled: true },
  { id: 'brand', spell: 'Demonic Brand Refresher', icon: 'Spell_Shadow_DemonBreath.png', condition: 'Brand missing', rawCond: 'debuff.demonic_brand_missing', enabled: false },
  { id: 'decimateSearing', spell: 'Decimation: Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Target HP < 35%, buff inactive', rawCond: 'decimation.inactive', enabled: false },
  { id: 'decimateSoulFire', spell: 'Decimation: Soul Fire', icon: 'Spell_Fire_Fireball.png', condition: 'Target HP < 35%, buff active', rawCond: 'decimation.active', enabled: false },
  { id: 'curse', spell: 'Curse of Doom', icon: 'Spell_Shadow_AuraOfDarkness.png', condition: 'Target TTDie >= 60s', rawCond: 'target_ttd >= 60 && !target.has_debuff("Curse of Doom")', enabled: true },
  { id: 'agony', spell: 'Bane of Agony', icon: 'Spell_Shadow_CurseOfSargeras.png', condition: 'Target TTDie >= 20s & No Doom', rawCond: 'target_ttd >= 20 && !target.has_debuff("Curse of Doom") && !target.has_debuff("Curse of Agony")', enabled: true },
  { id: 'corr', spell: 'Corruption', icon: 'Spell_Shadow_AbominationExplosion.png', condition: 'Target TTDie >= 12s & !Active', rawCond: 'target_ttd >= 12 && !target.has_debuff("Corruption")', enabled: true },
  { id: 'immo', spell: 'Immolate', icon: 'Spell_Fire_Immolation.png', condition: 'Target TTDie >= 15s & !Active', rawCond: 'target_ttd >= 15 && !target.has_debuff("Immolate")', enabled: false },
  { id: 'conflag', spell: 'Conflagrate', icon: 'Spell_Fire_Fireball.png', condition: 'Immolate Remaining < 6s', rawCond: 'target.debuff_remains("Immolate") < 6', enabled: false },
  { id: 'shadowburn', spell: 'Shadowburn', icon: 'Spell_Shadow_ScourgeBuild.png', condition: 'Always when available', rawCond: 'true', enabled: false },
  { id: 'incinerate', spell: 'Incinerate', icon: 'Spell_Fire_Burnout.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'searing', spell: 'Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'drain', spell: 'Wrack', icon: 'Spell_Shadow_ShadowBolt.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'siphon', spell: 'Siphon Life', icon: 'Spell_Shadow_Requiem.png', condition: 'Missing Siphon Life', rawCond: 'target.debuff_remains("Siphon Life") <= 0', enabled: false },
  { id: 'wrack', spell: 'Wrack', icon: 'Spell_Shadow_ShadowBolt.png', condition: 'Always', rawCond: 'true', enabled: false },
  { id: 'bolt', spell: 'Shadow Bolt', icon: 'Spell_Shadow_ShadowBolt.png', condition: 'Always', rawCond: 'true', enabled: true }
];

let currentAPL = JSON.parse(JSON.stringify(DEFAULT_APL));
let editingIndex = -1;
let dragSourceIndex = -1;
let onChangeCallback = null;

export function applySynthesizedAPL(rules) {
  if (!Array.isArray(rules)) return;
  let seenUnconditional = false;
  currentAPL = rules.map(r => {
    const cKey1 = r.condKey1 || r.condKey;
    const cKey2 = r.condKey2;
    const cEnum1 = r.cond1 !== undefined ? r.cond1 : r.cond;
    const cEnum2 = r.cond2;

    const isNever = !r.enabled ||
      cKey1 === 'NEVER' || cKey2 === 'NEVER' ||
      cEnum1 === 18 || cEnum2 === 18;

    const isCond1Always = (cKey1 === 'ALWAYS') || (cKey1 === undefined && cEnum1 === 0);
    const isCond2Always = (cKey2 === 'ALWAYS') || (cKey2 === undefined || cEnum2 === 0);

    let isEnabled = true;
    if (isNever || seenUnconditional) {
      isEnabled = false;
    } else if (isCond1Always && isCond2Always) {
      isEnabled = true;
      seenUnconditional = true;
    }

    return {
      id: r.id,
      spell: r.spell,
      icon: r.icon,
      condition: r.condition,
      condition1: r.condition1,
      condition2: r.condition2,
      rawCond: r.rawCond || (isEnabled ? 'true' : 'false'),
      enabled: isEnabled,
      action: r.action,
      cond: r.cond1 !== undefined ? r.cond1 : r.cond,
      param: r.param1 !== undefined ? r.param1 : r.param,
      targetSpell: r.targetSpell1 !== undefined ? r.targetSpell1 : r.targetSpell,
      cond1: r.cond1,
      param1: r.param1,
      targetSpell1: r.targetSpell1,
      cond2: r.cond2,
      param2: r.param2,
      targetSpell2: r.targetSpell2
    };
  });
  renderAPLTable();
  if (onChangeCallback) onChangeCallback(currentAPL);
}

export function compileAPLToBytecode(aplList = currentAPL) {
  const mapAction = {
    tap: APL_ACTION.LIFE_TAP,
    nightfall: APL_ACTION.NIGHTFALL_SHADOW_BOLT,
    brand: APL_ACTION.DEMONIC_BRAND_SEARING_PAIN,
    decimateSearing: APL_ACTION.DECIMATION_SEARING_PAIN,
    decimateSoulFire: APL_ACTION.DECIMATION_SOUL_FIRE,
    curse: APL_ACTION.CURSE_OF_DOOM,
    agony: APL_ACTION.CURSE_OF_AGONY,
    corr: APL_ACTION.CORRUPTION,
    immo: APL_ACTION.IMMOLATE,
    conflag: APL_ACTION.CONFLAGRATE,
    shadowburn: APL_ACTION.SHADOWBURN,
    incinerate: APL_ACTION.INCINERATE_FILLER,
    searing: APL_ACTION.SEARING_PAIN_FILLER,
    drain: APL_ACTION.DRAIN_SOUL_FILLER,
    siphon: APL_ACTION.SIPHON_LIFE,
    wrack: APL_ACTION.DRAIN_HOPE,
    bolt: APL_ACTION.SHADOW_BOLT_FILLER,
  };

  const spellIds = {
    'corruption': 1, 'curse of agony': 2, 'bane of agony': 2, 'curse of doom': 22,
    'immolate': 3, 'siphon life': 15,
  };
  const spellFromText = text => {
    const name = String(text).toLowerCase();
    const found = Object.keys(spellIds).find(spell => name.includes(spell));
    return found ? spellIds[found] : 0;
  };
  const parseCondition = (entry) => {
    const raw = String(entry.rawCond || 'true').trim().toLowerCase();
    const match = (pattern) => raw.match(pattern);
    if (entry.id === 'nightfall' && raw === 'buff.shadow_trance')
      return { cond: APL_COND.SHADOW_TRANCE, param: 0, targetSpell: 0 };
    if (entry.id === 'brand' && raw === 'debuff.demonic_brand_missing')
      return { cond: APL_COND.DEMONIC_BRAND_MISSING, param: 0, targetSpell: 0 };
    if (entry.id === 'decimateSearing' && raw === 'decimation.inactive')
      return { cond: APL_COND.DECIMATION_INACTIVE, param: 35, targetSpell: 5 };
    if (entry.id === 'decimateSoulFire' && raw === 'decimation.active')
      return { cond: APL_COND.DECIMATION_ACTIVE, param: 35, targetSpell: 13 };
    if (!raw || raw === 'true') return { cond: APL_COND.ALWAYS, param: 0, targetSpell: 0 };

    let m = match(/^mana_pct\s*(<=|<|>=|>)\s*(\d+(?:\.\d+)?)$/);
    if (m) return { cond: ({'<': APL_COND.MANA_LT, '<=': APL_COND.MANA_LE,
      '>': APL_COND.MANA_GT, '>=': APL_COND.MANA_GE})[m[1]],
      param: Number(m[2]), targetSpell: 0 };
    if (raw.match(/^mana_pct\s*<=\s*(\d+)$/))
      return { cond: APL_COND.MANA_LE, param: Number(raw.match(/^mana_pct\s*<=\s*(\d+)$/)[1]), targetSpell: 0 };
    m = match(/^target_hp_pct\s*(<=|<|>=|>)\s*(\d+(?:\.\d+)?)$/);
    if (m) return { cond: ({'<': APL_COND.TARGET_HP_LT, '<=': APL_COND.TARGET_HP_LE,
      '>': APL_COND.TARGET_HP_GT, '>=': APL_COND.TARGET_HP_GE})[m[1]],
      param: Number(m[2]), targetSpell: 0 };
    m = match(/^target_ttd\s*(>=|>)\s*(\d+(?:\.\d+)?)$/);
    if (m) return { cond: m[1] === '>' ? APL_COND.FIGHT_TIME_GT : APL_COND.FIGHT_TIME_GE, param: Number(m[2]), targetSpell: 0 };
    m = match(/^target_ttd\s*(<=|<)\s*(\d+(?:\.\d+)?)$/);
    if (m) return { cond: m[1] === '<' ? APL_COND.FIGHT_TIME_LT : APL_COND.FIGHT_TIME_LE, param: Number(m[2]), targetSpell: 0 };

    m = match(/^target\.debuff_remains\(\s*["']([^"']+)["']\s*\)\s*(<=|<)\s*(\d+(?:\.\d+)?)$/);
    if (m) {
      const targetSpell = spellFromText(m[1]);
      if (!targetSpell) throw new Error(`APL condition refers to unsupported debuff: ${m[1]}`);
      return { cond: m[2] === '<' ? APL_COND.DOT_REM_LT : APL_COND.DOT_REM_LE, param: Number(m[3]), targetSpell };
    }

    const hasNightfall = /talent\.nightfall/.test(raw);
    const ttd = raw.match(/target_ttd\s*>=?\s*(\d+(?:\.\d+)?)/);
    const missing = [...raw.matchAll(/!target\.has_debuff\(\s*["']([^"']+)["']\s*\)/g)]
      .map(x => spellFromText(x[1]));
    const onlySupportedAtoms = raw.split(/\s*&&\s*/).every(atom =>
      /^target_ttd\s*>=?\s*\d+(?:\.\d+)?$/.test(atom.trim()) ||
      /^!target\.has_debuff\(\s*["'][^"']+["']\s*\)$/.test(atom.trim()) ||
      /^talent\.nightfall$/.test(atom.trim()));
    if (onlySupportedAtoms && missing.length && missing.every(Boolean)) {
      if (hasNightfall && !ttd && missing.length === 1) {
        return { cond: APL_COND.NIGHTFALL_DOT_MISSING, param: 0, targetSpell: missing[0] };
      }
      if (ttd && missing.length === 1) {
        return { cond: APL_COND.FIGHT_GE_DOT_MISSING, param: Number(ttd[1]), targetSpell: missing[0] };
      }
      if (ttd && missing.length === 2 && missing.includes(2)) {
        return { cond: APL_COND.FIGHT_GE_DOOM_AGONY_MISSING, param: Number(ttd[1]), targetSpell: 2 };
      }
    }
    if (/^!target\.has_debuff\(\s*["']curse of doom["']\s*\)$/.test(raw))
      return { cond: APL_COND.DOOM_MISSING, param: 0, targetSpell: 2 };
    throw new Error(`APL condition is not supported by the shader: ${entry.rawCond}`);
  };

  return (aplList || currentAPL).map(entry => {
    if (entry.enabled && entry.id === 'drain')
      throw new Error('Drain Soul is not implemented by the WebGL shader yet.');
    const action = (entry.action !== undefined && entry.action !== null) ? entry.action : mapAction[entry.id];
    if (action === undefined) throw new Error(`Unsupported APL action: ${entry.id}`);
    
    let cond = APL_COND.ALWAYS, param = 0, targetSpell = 0;
    if (entry.enabled) {
      if (entry.cond !== undefined && entry.cond !== null) {
        cond = entry.cond;
        param = Number(entry.param || 0);
        targetSpell = Number(entry.targetSpell || 0);
      } else {
        const parsed = parseCondition(entry);
        cond = parsed.cond;
        param = parsed.param;
        targetSpell = parsed.targetSpell;
      }
    }

    return {
      action,
      cond,
      param,
      targetSpell,
      cond1: (entry.cond1 !== undefined ? entry.cond1 : cond),
      param1: (entry.param1 !== undefined ? entry.param1 : param),
      targetSpell1: (entry.targetSpell1 !== undefined ? entry.targetSpell1 : targetSpell),
      cond2: (entry.cond2 !== undefined ? entry.cond2 : APL_COND.ALWAYS),
      param2: (entry.param2 !== undefined ? entry.param2 : 0),
      targetSpell2: (entry.targetSpell2 !== undefined ? entry.targetSpell2 : 0),
      enabled: entry.enabled ? 1 : 0
    };
  });
}

export function getActiveBytecodeRules() {
  return compileAPLToBytecode(currentAPL);
}

export function initAPL(onAPLChange) {
  onChangeCallback = onAPLChange;
  renderAPLTable();
  setupConditionModal();
  setupMultiDotToggle();
}

export function getActiveAPL() {
  return currentAPL;
}

export function generateAPLForPreset(presetName = '', talentsObj = null, rotationChoice = null, isSacSuccubus = false) {
  const name = (presetName || '').toLowerCase();
  const rot = (rotationChoice || '').toLowerCase();
  
  const hasTalent = (tree, tal) => {
    if (!talentsObj) return false;
    const treeObj = talentsObj[tree];
    if (!treeObj) return false;
    for (const [k, v] of Object.entries(treeObj)) {
      if (k.toLowerCase().replace(/[\s\'-]/g, '_') === tal.toLowerCase().replace(/[\s\'-]/g, '_') && v > 0) return true;
    }
    return false;
  };

  const isBrand = name.includes('brand') || hasTalent('demonology', 'demonic_brand');
  const isDecimate = name.includes('decimate') || hasTalent('demonology', 'decimation');
  const isIncinerate = rot === 'fire_destro' || rot === 'incinerate_decimation' || name.includes('incinerate') || (hasTalent('destruction', 'incinerate') && !name.includes('searing') && rot !== 'dp_af_fire' && !rot.includes('shadow') && !rot.includes('affliction'));
  const isSearing = rot === 'dp_af_fire' || rot === 'searing' || name.includes('searing') || name.includes('dp fire') || name.includes('dp_fire');
  const isFire = isIncinerate || isSearing || rot.includes('fire') || name.includes('fire');
  const hasConflag = hasTalent('destruction', 'conflagrate');
  const hasShadowburn = hasTalent('destruction', 'shadowburn') && !name.includes('deep affliction');
  const hasSiphon = hasTalent('affliction', 'siphon_life') && (name.includes('deep affliction') || name.includes('affliction hybrid'));
  const hasWrack = hasTalent('affliction', 'wrack') && (name.includes('deep affliction') || name.includes('affliction hybrid'));
  const hasNightfall = hasTalent('affliction', 'nightfall') || (!isFire && !name.includes('pure shadow bolt'));
  const noCorruption = name.includes('no corruption') || name.includes('pure shadow bolt');
  const noBane = name.includes('no bane');

  if (isSearing && isDecimate) {
    // Dedicated DP_RUIN_FIRE APL
    return [
      { id: 'curse', spell: 'Curse of Doom', icon: 'Spell_Shadow_AuraOfDarkness.png', condition: 'Target TTDie >= 57s', rawCond: 'target_ttd >= 57', enabled: true },
      { id: 'agony', spell: 'Curse of Agony', icon: 'Spell_Shadow_CurseOfSargeras.png', condition: 'DoT Remains <= 2.5s', rawCond: 'target.debuff_remains("Curse of Agony") <= 2.5', enabled: true },
      { id: 'corr', spell: 'Corruption', icon: 'Spell_Shadow_AbominationExplosion.png', condition: 'DoT Remains <= 2.5s', rawCond: 'target.debuff_remains("Corruption") <= 2.5', enabled: true },
      { id: 'immo', spell: 'Immolate', icon: 'Spell_Fire_Immolation.png', condition: 'DoT Remains <= 2.5s', rawCond: 'target.debuff_remains("Immolate") <= 2.5', enabled: true },
      { id: 'brand', spell: 'Demonic Brand', icon: 'Spell_Shadow_DemonBreath.png', condition: 'Brand missing', rawCond: 'debuff.demonic_brand_missing', enabled: isBrand },
      { id: 'decimateSearing', spell: 'Decimation: Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Target HP <= 28%, buff inactive', rawCond: 'decimation.inactive', enabled: true },
      { id: 'tap', spell: 'Life Tap', icon: 'Spell_Shadow_BurningSpirit.png', condition: 'Mana <= 17%', rawCond: 'mana_pct <= 17', enabled: true },
      { id: 'decimateSoulFire', spell: 'Decimation: Soul Fire', icon: 'Spell_Fire_Fireball.png', condition: 'Target HP <= 28%, buff active', rawCond: 'decimation.active', enabled: true },
      { id: 'searing', spell: 'Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Filler (Fire)', rawCond: 'true', enabled: true }
    ];
  }

  // Standard Canonical Priority APL
  const list = [];
  // 1. Life Tap threshold
  list.push({ id: 'tap', spell: 'Life Tap', icon: 'Spell_Shadow_BurningSpirit.png', condition: 'Mana <= 25%', rawCond: 'mana_pct <= 25', enabled: true });

  // 2. Decimation execute triggers
  if (isDecimate) {
    if (isFire || name.includes('shadow and flame shadow - decimate')) {
      list.push({ id: 'decimateSearing', spell: 'Decimation: Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Target HP <= 35%, buff inactive', rawCond: 'decimation.inactive', enabled: true });
    }
    list.push({ id: 'decimateSoulFire', spell: 'Decimation: Soul Fire', icon: 'Spell_Fire_Fireball.png', condition: 'Target HP <= 35%, buff active', rawCond: 'decimation.active', enabled: true });
  }

  // 3. Demonic Brand
  if (isBrand) {
    list.push({ id: 'brand', spell: 'Demonic Brand', icon: 'Spell_Shadow_DemonBreath.png', condition: 'Brand missing', rawCond: 'debuff.demonic_brand_missing', enabled: true });
  }

  // 4. Immolate & Conflagrate
  const hasImmo = isFire || (hasConflag && hasTalent('destruction', 'shadow_and_flame'));
  if (hasImmo) {
    list.push({ id: 'immo', spell: 'Immolate', icon: 'Spell_Fire_Immolation.png', condition: 'DoT Expired', rawCond: 'target.debuff_remains("Immolate") <= 0', enabled: true });
  }
  if (hasConflag && hasImmo && !name.includes('dp/ruin fire')) {
    list.push({ id: 'conflag', spell: 'Conflagrate', icon: 'Spell_Fire_Fireball.png', condition: 'Always when available', rawCond: 'true', enabled: true });
  }

  // 5. Nightfall
  if (hasNightfall) {
    list.push({ id: 'nightfall', spell: 'Nightfall: Shadow Bolt', icon: 'Spell_Shadow_Twilight.png', condition: 'Shadow Trance active', rawCond: 'buff.shadow_trance', enabled: true });
  }

  // 6. Corruption
  if (!noCorruption) {
    list.push({ id: 'corr', spell: 'Corruption', icon: 'Spell_Shadow_AbominationExplosion.png', condition: 'DoT Expired', rawCond: 'target.debuff_remains("Corruption") <= 0', enabled: true });
  }

  // 7. Curses: Doom & Agony
  if (!noBane) {
    list.push({ id: 'curse', spell: 'Curse of Doom', icon: 'Spell_Shadow_AuraOfDarkness.png', condition: 'Target TTDie >= 60s', rawCond: 'target_ttd >= 60', enabled: true });
    list.push({ id: 'agony', spell: 'Curse of Agony', icon: 'Spell_Shadow_CurseOfSargeras.png', condition: 'DoT Expired', rawCond: 'target.debuff_remains("Curse of Agony") <= 0', enabled: true });
  }

  // 8. Siphon Life & Wrack
  if (hasSiphon) {
    list.push({ id: 'siphon', spell: 'Siphon Life', icon: 'Spell_Shadow_Requiem.png', condition: 'DoT Remains <= 0s', rawCond: 'target.debuff_remains("Siphon Life") <= 0', enabled: true });
  }
  if (hasWrack) {
    list.push({ id: 'wrack', spell: 'Wrack', icon: 'Spell_Shadow_ShadowBolt.png', condition: 'Always when available', rawCond: 'true', enabled: true });
  }

  // 9. Shadowburn
  if (hasShadowburn) {
    list.push({ id: 'shadowburn', spell: 'Shadowburn', icon: 'Spell_Shadow_ScourgeBuild.png', condition: 'Always when available', rawCond: 'true', enabled: true });
  }

  // 10. Primary Spells
  if (isIncinerate) {
    list.push({ id: 'incinerate', spell: 'Incinerate', icon: 'Spell_Fire_Burnout.png', condition: 'Always', rawCond: 'true', enabled: true });
  } else if (isSearing) {
    list.push({ id: 'searing', spell: 'Searing Pain', icon: 'Spell_Fire_SoulBurn.png', condition: 'Always', rawCond: 'true', enabled: true });
  } else {
    list.push({ id: 'bolt', spell: 'Shadow Bolt', icon: 'Spell_Shadow_ShadowBolt.png', condition: 'Always', rawCond: 'true', enabled: true });
  }

  return list;
}

export function setAPLPreset(presetName, talentsObj = null, rotationChoice = null, isSacSuccubus = false) {
  currentAPL = generateAPLForPreset(presetName, talentsObj, rotationChoice, isSacSuccubus);
  renderAPLTable();
  if (onChangeCallback) onChangeCallback(currentAPL);
}

export function renderAPLTable() {
  const tbody = document.getElementById('apl-table-body');
  if (!tbody) return;

  tbody.innerHTML = '';
  currentAPL.forEach((entry, idx) => {
    const tr = document.createElement('tr');
    tr.className = `apl-row ${entry.enabled ? '' : 'disabled-row'}`;
    tr.draggable = true;
    tr.dataset.index = idx;

    tr.innerHTML = `
      <td class="apl-col-reorder">
        <div class="apl-reorder-btns">
          <button type="button" class="apl-btn-move up-btn" title="Move Up" ${idx === 0 ? 'disabled' : ''}>▲</button>
          <button type="button" class="apl-btn-move down-btn" title="Move Down" ${idx === currentAPL.length - 1 ? 'disabled' : ''}>▼</button>
        </div>
      </td>
      <td class="apl-col-priority">
        <span class="apl-prio-badge">#${idx + 1}</span>
      </td>
      <td class="apl-col-spell">
        <div class="apl-spell-cell">
          <img src="./assets/icons/${entry.icon}" alt="${entry.spell}" class="apl-spell-icon" onerror="this.src='./assets/icons/Spell_Shadow_ShadowBolt.png'">
          <span class="apl-spell-name">${entry.spell}</span>
        </div>
      </td>
      <td class="apl-col-condition">
        <div class="apl-condition-badge" title="Right-click or click ↗ to edit condition">
          <code>${escapeHtml(entry.condition)}</code>
        </div>
      </td>
      <td class="apl-col-actions">
        <button type="button" class="apl-btn-edit edit-cond-btn" title="Edit Condition" data-index="${idx}">↗</button>
        <button type="button" class="apl-btn-toggle toggle-btn" title="${entry.enabled ? 'Disable' : 'Enable'}" data-index="${idx}">
          ${entry.enabled ? '●' : '○'}
        </button>
      </td>
    `;

    // Reorder Click Handlers
    tr.querySelector('.up-btn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      moveAPLEntry(idx, idx - 1);
    });

    tr.querySelector('.down-btn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      moveAPLEntry(idx, idx + 1);
    });

    // Edit Condition Button
    tr.querySelector('.edit-cond-btn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      openConditionEditor(idx);
    });

    // Toggle Enable/Disable
    tr.querySelector('.toggle-btn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      currentAPL[idx].enabled = !currentAPL[idx].enabled;
      renderAPLTable();
      if (onChangeCallback) onChangeCallback(currentAPL);
    });

    // Right-Click row to edit conditions
    tr.addEventListener('contextmenu', (e) => {
      e.preventDefault();
      openConditionEditor(idx);
    });

    // Drag-and-Drop Handlers
    tr.addEventListener('dragstart', (e) => {
      dragSourceIndex = idx;
      tr.classList.add('dragging');
      e.dataTransfer.effectAllowed = 'move';
    });

    tr.addEventListener('dragover', (e) => {
      e.preventDefault();
      e.dataTransfer.dropEffect = 'move';
      tr.classList.add('drag-over');
    });

    tr.addEventListener('dragleave', () => {
      tr.classList.remove('drag-over');
    });

    tr.addEventListener('drop', (e) => {
      e.preventDefault();
      tr.classList.remove('drag-over');
      const targetIdx = Number(tr.dataset.index);
      if (dragSourceIndex !== -1 && dragSourceIndex !== targetIdx) {
        moveAPLEntry(dragSourceIndex, targetIdx);
      }
    });

    tr.addEventListener('dragend', () => {
      tr.classList.remove('dragging');
      document.querySelectorAll('.apl-row').forEach(r => r.classList.remove('drag-over'));
      dragSourceIndex = -1;
    });

    tbody.appendChild(tr);
  });
}

function moveAPLEntry(fromIdx, toIdx) {
  if (fromIdx < 0 || fromIdx >= currentAPL.length || toIdx < 0 || toIdx >= currentAPL.length) return;
  const [removed] = currentAPL.splice(fromIdx, 1);
  currentAPL.splice(toIdx, 0, removed);
  renderAPLTable();
  if (onChangeCallback) onChangeCallback(currentAPL);
}

function setupConditionModal() {
  const modal = document.getElementById('apl-condition-modal');
  const closeBtn = document.getElementById('btn-close-apl-modal');
  const saveBtn = document.getElementById('btn-save-apl-cond');
  const cancelBtn = document.getElementById('btn-cancel-apl-cond');

  closeBtn?.addEventListener('click', closeConditionEditor);
  cancelBtn?.addEventListener('click', closeConditionEditor);

  saveBtn?.addEventListener('click', () => {
    if (editingIndex >= 0 && editingIndex < currentAPL.length) {
      const labelInput = document.getElementById('apl-modal-cond-label');
      const rawInput = document.getElementById('apl-modal-cond-raw');
      if (labelInput) currentAPL[editingIndex].condition = labelInput.value;
      if (rawInput) currentAPL[editingIndex].rawCond = rawInput.value;
      renderAPLTable();
      if (onChangeCallback) onChangeCallback(currentAPL);
    }
    closeConditionEditor();
  });
}

function openConditionEditor(index) {
  editingIndex = index;
  const entry = currentAPL[index];
  if (!entry) return;

  const modal = document.getElementById('apl-condition-modal');
  const title = document.getElementById('apl-modal-spell-title');
  const icon = document.getElementById('apl-modal-spell-icon');
  const labelInput = document.getElementById('apl-modal-cond-label');
  const rawInput = document.getElementById('apl-modal-cond-raw');

  if (title) title.textContent = `Edit Condition: ${entry.spell}`;
  if (icon) icon.src = `./assets/icons/${entry.icon}`;
  if (labelInput) labelInput.value = entry.condition;
  if (rawInput) rawInput.value = entry.rawCond || entry.condition;

  if (modal) {
    modal.style.display = 'flex';
  }
}

function closeConditionEditor() {
  const modal = document.getElementById('apl-condition-modal');
  if (modal) {
    modal.style.display = 'none';
  }
  editingIndex = -1;
}

function setupMultiDotToggle() {
  const toggle = document.getElementById('check-multidot-corruption');
  toggle?.addEventListener('change', () => {
    if (onChangeCallback) onChangeCallback(currentAPL);
  });
}

function escapeHtml(str) {
  return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}
