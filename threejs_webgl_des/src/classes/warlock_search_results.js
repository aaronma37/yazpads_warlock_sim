// Preserve the existing search result payloads while owning Warlock naming/accounting.
export function createWarlockSearchResults({ talents, apl, identity }) {
  const TalentGraph = talents.graph;
  const formatBuildName = identity.formatBuildName;
  const individualToBytecodeRules = apl.encodeIndividual;

// Convert simulated result object to standard CandidateResult representation
function createCandidateResult(ind, rank = 1) {
  const a = TalentGraph.countTreePoints(ind.talents, 0);
  const d = TalentGraph.countTreePoints(ind.talents, 1);
  const x = TalentGraph.countTreePoints(ind.talents, 2);

  let cat = 'Hybrid Peak';
  if (x >= 25 && x >= a && x >= d) cat = 'Destruction Peak';
  else if (a >= 25 && a >= x && a >= d) cat = 'Affliction Peak';
  else if (d >= 25 && d >= a && d >= x) cat = 'Demonology Peak';

  const sum = ind.batch?.summary || {};
  const shadowDmg = sum.shadowDamage || 0;
  const fireDmg = sum.fireDamage || 0;
  const petDmg = sum.petDamage || 0;
  const totalDmg = shadowDmg + fireDmg + petDmg || 1;

  const shadowPct = (shadowDmg / totalDmg) * 100;
  const firePct = (fireDmg / totalDmg) * 100;
  const petPct = (petDmg / totalDmg) * 100;

  let aplRules = [];
  if (ind.apl && Array.isArray(ind.apl.rules)) {
    aplRules = individualToBytecodeRules(ind.apl);
  } else if (ind.batch?.config?.aplRules) {
    aplRules = ind.batch.config.aplRules;
  }

  return {
    rank,
    name: formatBuildName(ind),
    category: cat,
    race: ind.race,
    pet: ind.pet,
    sacImp: ind.sacImp,
    sacSuccubus: ind.sacSuccubus,
    rotation: ind.rotation,
    mean_dps: sum.mean || ind.fitness,
    std_dev: sum.sd || 0,
    min_dps: sum.p05 || 0,
    max_dps: sum.p95 || 0,
    ci95: sum.ci95 || 0,
    shadow_pct: shadowPct,
    fire_pct: firePct,
    pet_pct: petPct,
    talents: TalentGraph.toTalentsObject(ind.talents),
    talentsVector: Array.from(ind.talents),
    apl: ind.apl ? { rules: ind.apl.rules.map(r => ({ ...r })) } : null,
    aplRules,
    summary: sum,
    states: ind.batch?.states || [],
    individual: ind
  };
}

// Generate human-readable summary name for a synthesized APL
function formatAPLName(ind) {
  const active = ind.rules.filter(r => r.enabled);
  const activeIds = active.map(r => r.id);

  const tags = [];
  if (activeIds.includes('curse')) tags.push('Doom');
  if (activeIds.includes('agony')) tags.push('Agony');
  if (activeIds.includes('corr')) tags.push('Corr');
  if (activeIds.includes('immo')) tags.push('Immo');
  if (activeIds.includes('conflag')) tags.push('Conflag');
  if (activeIds.includes('brand')) tags.push('Brand');
  if (activeIds.includes('decimateSoulFire') || activeIds.includes('decimateSearing')) tags.push('Deci');
  if (activeIds.includes('nightfall')) tags.push('Nightfall');
  if (activeIds.includes('siphon')) tags.push('Siphon');
  if (activeIds.includes('wrack')) tags.push('Wrack');
  if (activeIds.includes('hellfire')) tags.push('Hellfire');
  if (activeIds.includes('shadowburn')) tags.push('Sburn');

  let primary = 'Shadow Bolt';
  const firstNuke = activeIds.find(id => id === 'incinerate' || id === 'searing' || id === 'bolt' || id === 'wrack' || id === 'hellfire');
  if (firstNuke === 'incinerate') primary = 'Incinerate';
  else if (firstNuke === 'searing') primary = 'Searing Pain';
  else if (firstNuke === 'wrack') primary = 'Wrack';
  else if (firstNuke === 'hellfire') primary = 'Hellfire';

  const tagStr = tags.length > 0 ? tags.slice(0, 4).join('/') : 'Direct';
  return `${primary} (${tagStr} · ${active.length} Active Rules)`;
}

// Create elite candidate result payload
function createCandidateAPLResult(ind, rank = 1) {
  const activeRules = ind.rules.filter(r => r.enabled);
  const name = formatAPLName(ind);
  const summary = ind.batch?.summary || { mean: ind.fitness || 0, stdDev: 0, min: 0, max: 0, confidence: 0 };

  return {
    rank,
    name,
    rules: ind.rules,
    activeRulesCount: activeRules.length,
    activeRules: activeRules.map(r => ({
      id: r.id,
      spell: r.spell,
      icon: r.icon,
      condition: r.condition,
      rawCond: r.rawCond,
      enabled: r.enabled
    })),
    meanDps: summary.mean || ind.fitness,
    stdDev: summary.stdDev || 0,
    confidence: summary.confidence || 0,
    summary,
    batch: ind.batch,
    aplRules: individualToBytecodeRules(ind)
  };
}

  return Object.freeze({ createCandidateResult, formatAPLName, createCandidateAPLResult });
}
