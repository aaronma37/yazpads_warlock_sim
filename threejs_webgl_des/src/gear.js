// Classic WoW Equipment & Paperdoll State Manager

let itemDb = { slots: [], items: [], presets: {} };
let activeGear = {}; // slot -> item
let gearMode = 'direct'; // 'equipped' or 'direct'
let activeRace = 'HUMAN';

const DEFAULT_ENCHANTS = {
  spell_power: 30 + 16 + 18, // Weapon (+30), Head/Legs (+16), Shoulders (+18)
  shadow_power: 20, // Gloves (+20)
  stamina: 4,
  intellect: 4
};

export async function initGear(onStatsChange) {
  try {
    const res = await fetch(new URL('../data/items.json', import.meta.url));
    itemDb = await res.json();
  } catch (err) {
    console.error('Failed to load items.json:', err);
    return;
  }

  // Set default to Phase 6 BiS
  loadBiSPreset('p6', false);
  renderPaperdoll();
  renderBiSButtons();

  if (onStatsChange) {
    onStatsChange(calculateEquippedStats());
  }
}

export function setGearMode(mode) {
  gearMode = mode;
  const paperdollContainer = document.getElementById('paperdoll-view');
  const directStatsContainer = document.getElementById('direct-stats-view');
  const btnDirect = document.getElementById('btn-mode-direct');
  const btnEquipped = document.getElementById('btn-mode-equipped');

  if (paperdollContainer && directStatsContainer) {
    if (mode === 'equipped') {
      paperdollContainer.style.display = 'block';
      directStatsContainer.style.display = 'none';
      btnEquipped?.classList.add('active');
      btnDirect?.classList.remove('active');
    } else {
      paperdollContainer.style.display = 'none';
      directStatsContainer.style.display = 'block';
      btnDirect?.classList.add('active');
      btnEquipped?.classList.remove('active');
    }
  }
}

export function loadBiSPreset(presetKey, triggerCallback = true) {
  const preset = itemDb.presets?.[presetKey];
  if (!preset) return;
  activeGear = {};
  for (const [slot, itemId] of Object.entries(preset.items)) {
    const item = itemDb.items.find(it => it.id === itemId);
    if (item) {
      activeGear[slot] = item;
    }
  }
  renderPaperdoll();
  if (triggerCallback) {
    const stats = calculateEquippedStats();
    window.dispatchEvent(new CustomEvent('gear-updated', { detail: stats }));
  }
}

export function getEquippedItem(slot) {
  return activeGear[slot] || null;
}

export function equipItem(slot, item) {
  if (item) {
    activeGear[slot] = item;
  } else {
    delete activeGear[slot];
  }
  renderPaperdoll();
  const stats = calculateEquippedStats();
  window.dispatchEvent(new CustomEvent('gear-updated', { detail: stats }));
}

export function calculateEquippedStats() {
  const stats = {
    stamina: DEFAULT_ENCHANTS.stamina,
    intellect: DEFAULT_ENCHANTS.intellect,
    spirit: 0,
    spell_power: DEFAULT_ENCHANTS.spell_power,
    shadow_power: DEFAULT_ENCHANTS.shadow_power,
    fire_power: 0,
    spell_hit: 0,
    spell_crit: 0,
    spell_haste: 0,
    spell_penetration: 0,
    mp5: 0
  };

  let bloodvine = 0;
  let nemesis = 0;
  let plagueheart = 0;

  for (const item of Object.values(activeGear)) {
    if (!item) continue;
    stats.stamina += item.stamina || 0;
    stats.intellect += item.intellect || 0;
    stats.spirit += item.spirit || 0;
    stats.spell_power += item.spell_power || 0;
    stats.shadow_power += item.shadow_power || 0;
    stats.fire_power += item.fire_power || 0;
    stats.spell_hit += item.spell_hit || 0;
    stats.spell_crit += item.spell_crit || 0;
    stats.spell_haste += item.spell_haste || 0;
    stats.spell_penetration += item.spell_penetration || 0;
    stats.mp5 += item.mp5 || 0;

    if (item.set_name === 'Bloodvine') bloodvine++;
    if (item.set_name === 'Nemesis') nemesis++;
    if (item.set_name === 'Plagueheart') plagueheart++;
  }

  // Set bonuses
  if (bloodvine >= 3) stats.spell_hit += 2.0;
  if (nemesis >= 3) stats.spell_power += 23.0;

  return stats;
}

const QUALITY_COLORS = {
  EPIC: '#a335ee',
  RARE: '#0070dd',
  UNCOMMON: '#1eff00',
  COMMON: '#ffffff'
};

const DEFAULT_SLOT_ICONS = {
  HEAD: 'INV_Helmet_01.png',
  NECK: 'INV_Jewelry_Necklace_01.png',
  SHOULDERS: 'INV_Shoulder_02.png',
  BACK: 'INV_Misc_Cape_18.png',
  CHEST: 'INV_Chest_Cloth_21.png',
  WRISTS: 'INV_Bracer_07.png',
  HANDS: 'INV_Gauntlets_19.png',
  WAIST: 'INV_Belt_12.png',
  LEGS: 'INV_Pants_Cloth_05.png',
  FEET: 'INV_Boots_05.png',
  RING1: 'INV_Jewelry_Ring_37.png',
  RING2: 'INV_Jewelry_Ring_34.png',
  TRINKET1: 'INV_Jewelry_Talisman_14.png',
  TRINKET2: 'INV_Trinket_Naxxramas04.png',
  MAIN_HAND: 'INV_Sword_27.png',
  OFF_HAND: 'INV_Misc_Bag_10.png',
  RANGED: 'INV_Wand_11.png'
};

const SLOT_NAMES = {
  HEAD: 'Head',
  NECK: 'Neck',
  SHOULDERS: 'Shoulders',
  BACK: 'Back',
  CHEST: 'Chest',
  WRISTS: 'Wrists',
  HANDS: 'Hands',
  WAIST: 'Waist',
  LEGS: 'Legs',
  FEET: 'Feet',
  RING1: 'Ring 1',
  RING2: 'Ring 2',
  TRINKET1: 'Trinket 1',
  TRINKET2: 'Trinket 2',
  MAIN_HAND: 'Main Hand',
  OFF_HAND: 'Off Hand',
  RANGED: 'Wand'
};

function renderBiSButtons() {
  const container = document.getElementById('bis-presets-bar');
  if (!container) return;
  container.innerHTML = `
    <button class="wow-button wow-btn-small" data-bis="preraid">Pre-Raid</button>
    <button class="wow-button wow-btn-small" data-bis="p3">Phase 3/4</button>
    <button class="wow-button wow-btn-small" data-bis="p5">Phase 5</button>
    <button class="wow-button wow-btn-small active" data-bis="p6">Phase 6 BiS</button>
  `;

  container.querySelectorAll('button[data-bis]').forEach(btn => {
    btn.addEventListener('click', () => {
      container.querySelectorAll('button').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      loadBiSPreset(btn.dataset.bis);
    });
  });
}

function renderPaperdoll() {
  const table = document.getElementById('gear-table-body');
  if (!table) return;

  const slots = itemDb.slots || Object.keys(DEFAULT_SLOT_ICONS);
  table.innerHTML = '';

  for (const slot of slots) {
    const equipped = activeGear[slot];
    const availableItems = itemDb.items.filter(it => it.slot === slot || (slot.startsWith('RING') && it.slot.startsWith('RING')) || (slot.startsWith('TRINKET') && it.slot.startsWith('TRINKET')));
    
    const row = document.createElement('tr');
    row.className = 'gear-row';

    const slotLabel = SLOT_NAMES[slot] || slot;
    const iconName = equipped?.icon || DEFAULT_SLOT_ICONS[slot] || 'INV_Misc_QuestionMark.png';
    const itemName = equipped?.name || '(Empty Slot)';
    const itemColor = equipped ? (QUALITY_COLORS[equipped.quality] || '#fff') : '#888';

    row.innerHTML = `
      <td class="slot-name-col">
        <div class="slot-cell">
          <div class="wow-icon-slot">
            <img src="./assets/icons/${iconName}" alt="${slotLabel}" onerror="this.src='./assets/icons/INV_Misc_QuestionMark.png'">
          </div>
          <span>${slotLabel}</span>
        </div>
      </td>
      <td class="slot-item-col">
        <select class="wow-select item-dropdown" data-slot="${slot}" style="color: ${itemColor};">
          <option value="" style="color: #888;">(Empty Slot)</option>
          ${availableItems.map(it => `
            <option value="${it.id}" ${equipped?.id === it.id ? 'selected' : ''} style="color: ${QUALITY_COLORS[it.quality] || '#fff'};">
              ${it.name} [P${it.phase}]
            </option>
          `).join('')}
        </select>
      </td>
    `;

    const select = row.querySelector('select');
    select.addEventListener('change', (e) => {
      const val = parseInt(e.target.value);
      if (val) {
        const item = itemDb.items.find(it => it.id === val);
        equipItem(slot, item);
      } else {
        equipItem(slot, null);
      }
    });

    table.appendChild(row);
  }
}
