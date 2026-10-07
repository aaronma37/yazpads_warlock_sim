// Pure Warlock gear validation; item data is supplied by the caller.
const DEFAULT_ENCHANTS = {
  spell_power: 30 + 16 + 18, // Weapon (+30), Head/Legs (+16), Shoulders (+18)
  shadow_power: 20, // Gloves (+20)
  stamina: 4,
  intellect: 4
};

function calculateStats(activeGear) {
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

export function resolveWarlockEquipment(items, database) {
  if (!database || !Array.isArray(database.slots) || !Array.isArray(database.items)) throw new Error('Equipment resolution requires the item database.');
  const activeGear = {};
  for (const [slot, id] of Object.entries(items)) {
    if (!database.slots.includes(slot)) throw new Error(`Unsupported equipment slot: ${slot}`);
    if (!Number.isInteger(id) || id <= 0) throw new Error(`Invalid item ID for ${slot}`);
    const item = database.items.find(item => item.id === id);
    if (!item) throw new Error(`Unknown item ID: ${id}`);
    const canonicalSlot = slot === 'RING2' ? 'RING1' : slot === 'TRINKET2' ? 'TRINKET1' : slot;
    if (item.slot !== canonicalSlot) throw new Error(`Item ${id} cannot equip in ${slot}`);
    activeGear[slot] = item;
  }
  const stats = calculateStats(activeGear);
  if (stats.spell_haste !== 0) throw new Error('Equipped haste is unsupported by the simulation.');
  return { spellPower: Math.round(stats.spell_power), shadowPower: Math.round(stats.shadow_power),
    firePower: Math.round(stats.fire_power), intellect: Math.round(stats.intellect),
    stamina: Math.round(stats.stamina), spirit: Math.round(stats.spirit),
    hit: Number(stats.spell_hit.toFixed(1)), crit: Number(stats.spell_crit.toFixed(1)),
    mp5: Math.round(stats.mp5), penetration: stats.spell_penetration };
}
