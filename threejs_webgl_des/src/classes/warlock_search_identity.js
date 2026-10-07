// Bind Warlock diversity and deduplication semantics once; preserve existing keys.
export function createWarlockSearchIdentity({ talents, choices }) {
  const TalentGraph = talents.graph;
  const ROTATION_CHOICES = choices.rotations;

// Format Spec Name strictly as X/Y/Z (Affliction / Demonology / Destruction)
function formatBuildName(ind) {
  const a = TalentGraph.countTreePoints(ind.talents, 0);
  const d = TalentGraph.countTreePoints(ind.talents, 1);
  const x = TalentGraph.countTreePoints(ind.talents, 2);
  return `${a}/${d}/${x}`;
}

// MAP-Elites Quality-Diversity Key
function getMapElitesKey(ind) {
  const a = TalentGraph.countTreePoints(ind.talents, 0);
  const d = TalentGraph.countTreePoints(ind.talents, 1);
  const x = TalentGraph.countTreePoints(ind.talents, 2);

  const u = Math.min(7, Math.max(0, Math.floor(((x - a + 51) / 102.0) * 8)));
  const v = Math.min(7, Math.max(0, Math.floor((d / 51.0) * 8)));
  const petIdx = ind.pet === 'none' ? 0 : ind.pet === 'imp' ? 1 : 2;
  const sacIdx = ind.sacImp ? 1 : ind.sacSuccubus ? 2 : 0;
  const p = petIdx * 3 + sacIdx;
  
  let rotIdx = Math.max(0, ROTATION_CHOICES.indexOf(ind.rotation));
  if (ind.apl && Array.isArray(ind.apl.rules)) {
    const activeSpells = ind.apl.rules.filter(r => r.enabled).map(r => r.id);
    const firstNuke = activeSpells.find(id => id === 'incinerate' || id === 'searing' || id === 'bolt' || id === 'wrack' || id === 'hellfire') || 'bolt';
    rotIdx = firstNuke === 'incinerate' ? 1 : firstNuke === 'searing' ? 3 : 0;
  }

  return `${u}_${v}_${p}_${rotIdx}`;
}

// Generate unique individual key for duplicate prevention
function getIndUniqueKey(ind) {
  const aplKey = (ind.apl && Array.isArray(ind.apl.rules)) ? `_${getAPLUniqueKey(ind.apl)}` : '';
  return `${ind.race}_${ind.rotation}_${ind.pet}_${ind.sacImp ? 1 : 0}_${ind.sacSuccubus ? 1 : 0}_${Array.from(ind.talents).join(',')}${aplKey}`;
}

// Generate unique hash key for deduplication and MAP-Elites
function getAPLUniqueKey(ind) {
  return ind.rules.map(r => `${r.id}:${r.condKey1 || r.condKey || ''}:${r.condKey2 || ''}:${r.enabled ? Number(r.param1 !== undefined ? r.param1 : r.param || 0).toFixed(1) : 'OFF'}`).join('|');
}

// MAP-Elites Quality-Diversity Key for APLs
function getAPLMapElitesKey(ind) {
  const active = ind.rules.filter(r => r.enabled);
  const activeIds = active.map(r => r.id);

  // Feature 1: Top priority action
  const topAction = activeIds[0] || 'none';

  // Feature 2: Top DoT / Curse applied first
  const firstDot = activeIds.find(id => id === 'curse' || id === 'agony' || id === 'corr' || id === 'immo' || id === 'siphon') || 'none';

  // Feature 3: Primary filler
  const firstNuke = activeIds.find(id => id === 'incinerate' || id === 'searing' || id === 'bolt' || id === 'wrack' || id === 'hellfire') || 'bolt';

  // Feature 4: Tap position relative to filler
  const tapIdx = activeIds.indexOf('tap');
  const nukeIdx = activeIds.indexOf(firstNuke);
  const tapPos = (tapIdx >= 0 && (nukeIdx < 0 || tapIdx < nukeIdx)) ? 'early' : 'late';

  return `${topAction}_${firstDot}_${firstNuke}_${tapPos}`;
}

  return Object.freeze({ formatBuildName, getMapElitesKey, getIndUniqueKey, getAPLUniqueKey, getAPLMapElitesKey });
}
