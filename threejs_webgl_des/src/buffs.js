// Consumables and Raid Buffs Selector & Stat Aggregator

export const BUFF_DEFINITIONS = [
  { id: 'flask_supreme_power', name: 'Flask of Supreme Power', type: 'flask', icon: 'inv_potion_41', stats: { spellPower: 150 } },
  { id: 'greater_arcane_elixir', name: 'Greater Arcane Elixir', type: 'elixir_battle', icon: 'inv_potion_25', stats: { spellPower: 35 } },
  { id: 'elixir_shadow_power', name: 'Elixir of Shadow Power', type: 'elixir_battle', icon: 'inv_potion_46', stats: { spellPower: 40 } },
  { id: 'elixir_greater_firepower', name: 'Elixir of Greater Firepower', type: 'elixir_battle', icon: 'inv_potion_60', stats: { spellPower: 40 } },
  { id: 'brilliant_wizard_oil', name: 'Brilliant Wizard Oil', type: 'oil', icon: 'inv_potion_105', stats: { spellPower: 36, crit: 1.0 } },
  { id: 'mageblood_elixir', name: 'Mageblood Potion', type: 'elixir_guardian', icon: 'inv_potion_45', stats: { mp5: 12 } },
  { id: 'nightfin_soup', name: 'Nightfin Soup (Food)', type: 'food', icon: 'inv_misc_fish_03', stats: { mp5: 8 } },
  { id: 'arcane_intellect', name: 'Arcane Intellect (Raid)', type: 'buff', icon: 'spell_holy_magicalsentry', stats: { intellect: 31 } },
  { id: 'mark_of_the_wild', name: 'Mark of the Wild (Raid)', type: 'buff', icon: 'spell_nature_regeneration', stats: { intellect: 12, spirit: 12 } },
  { id: 'blessing_of_kings', name: 'Blessing of Kings (Paladin)', type: 'buff', icon: 'spell_magic_magearmor', stats: { intellectPct: 10 } },
  { id: 'songflower', name: 'Songflower Serenade (World)', type: 'world', icon: 'spell_holy_mindvision', stats: { crit: 5.0, intellect: 15 } },
  { id: 'rallying_cry', name: 'Rallying Cry of the Dragonslayer', type: 'world', icon: 'inv_misc_head_dragon_01', stats: { crit: 10.0 } },
];

let activeBuffs = new Set(['flask_supreme_power', 'greater_arcane_elixir', 'elixir_shadow_power', 'brilliant_wizard_oil', 'arcane_intellect', 'mark_of_the_wild']);

export function initBuffs(onBuffChange) {
  renderBuffSelector(onBuffChange);
}

export function getActiveBuffStats(baseStats = { spellPower: 500, intellect: 200, spirit: 100, hit: 12, crit: 15, mp5: 20 }) {
  let addSP = 0, addInt = 0, addSpirit = 0, addCrit = 0, addHit = 0, addMP5 = 0;
  let intMultiplier = 1.0;

  for (const buffId of activeBuffs) {
    const buff = BUFF_DEFINITIONS.find(b => b.id === buffId);
    if (!buff) continue;
    if (buff.stats.spellPower) addSP += buff.stats.spellPower;
    if (buff.stats.intellect) addInt += buff.stats.intellect;
    if (buff.stats.spirit) addSpirit += buff.stats.spirit;
    if (buff.stats.crit) addCrit += buff.stats.crit;
    if (buff.stats.hit) addHit += buff.stats.hit;
    if (buff.stats.mp5) addMP5 += buff.stats.mp5;
    if (buff.stats.intellectPct) intMultiplier *= (1 + buff.stats.intellectPct / 100);
  }

  const finalInt = Math.round((baseStats.intellect + addInt) * intMultiplier);
  return {
    addedSpellPower: addSP,
    addedIntellect: finalInt - baseStats.intellect,
    addedCrit: addCrit,
    addedHit: addHit,
    addedMP5: addMP5,
    totalSpellPower: baseStats.spellPower + addSP,
    totalIntellect: finalInt,
    totalCrit: baseStats.crit + addCrit,
    totalHit: baseStats.hit + addHit,
    totalMP5: baseStats.mp5 + addMP5,
  };
}

export function renderBuffSelector(onBuffChange) {
  const container = document.getElementById('buffs-selector-container');
  if (!container) return;

  container.innerHTML = '';
  const grid = document.createElement('div');
  grid.className = 'buff-grid';

  BUFF_DEFINITIONS.forEach(buff => {
    const item = document.createElement('label');
    item.className = `buff-item ${activeBuffs.has(buff.id) ? 'selected' : ''}`;
    
    const iconSrc = `./assets/icons/${buff.icon.toLowerCase()}.png`;
    const statSummary = Object.entries(buff.stats).map(([k, v]) => `+${v} ${k}`).join(', ');

    item.innerHTML = `
      <input type="checkbox" name="buff_${buff.id}" ${activeBuffs.has(buff.id) ? 'checked' : ''} class="sr-only">
      <img src="${iconSrc}" alt="${buff.name}" class="buff-icon" onerror="this.style.display='none'">
      <div class="buff-info">
        <span class="buff-name">${buff.name}</span>
        <span class="buff-stats">${statSummary}</span>
      </div>
    `;

    item.querySelector('input').addEventListener('change', (e) => {
      if (e.target.checked) activeBuffs.add(buff.id);
      else activeBuffs.delete(buff.id);
      item.classList.toggle('selected', e.target.checked);
      if (onBuffChange) onBuffChange(getActiveBuffStats());
    });

    grid.appendChild(item);
  });

  container.appendChild(grid);
}
