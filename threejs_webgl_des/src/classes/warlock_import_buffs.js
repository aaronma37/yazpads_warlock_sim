// Combat-form buff/racial arithmetic, preserving current app ordering and rounding.
// Sacrifice/talent effects belong to the configuration builder and are excluded here.
export function resolveWarlockBuffEffects(stats, buffs, race, target) {
  const sp = stats.spellPower, shadowSpBonus = stats.shadowPower, fireSpBonus = stats.firePower;
  const hit = stats.hit, crit = stats.crit, haste = 0;
  const baseInt = stats.intellect, baseStam = stats.stamina, baseSpirit = stats.spirit, baseMp5 = stats.mp5;
  const baseResistance = target.resistance, bossArmor = target.bossArmor;
  const activeRace = race;
  const isFlaskSupreme = buffs.flaskSupremePower === true;
  const isGreaterArcane = buffs.greaterArcaneElixir === true;
  const isShadowPowerElixir = buffs.shadowPowerElixir === true;
  const isGreaterFirepowerElixir = buffs.greaterFirepowerElixir === true;
  const isWizardOil = buffs.wizardOil === true;
  const isMageblood = buffs.magebloodElixir === true;
  const isNightfin = buffs.nightfinSoup === true;
  const isArcaneIntellect = buffs.arcaneIntellect === true;
  const isMarkOfTheWild = buffs.markOfTheWild === true;
  const isBlessingOfKings = buffs.blessingOfKings === true;
  const isBlessingOfWisdom = buffs.blessingOfWisdom === true;
  const isManaSpringTotem = buffs.manaSpringTotem === true;
  const isDragonslayer = buffs.dragonslayer === true;
  const isSongflower = buffs.songflower === true;
  const isWarchiefsBlessing = buffs.warchiefsBlessing === true;
  const isSpiritOfZandalar = buffs.spiritOfZandalar === true;
  const isSaygesFortune = buffs.saygesFortune === true;
  const isCurseOfShadow = buffs.curseOfShadow === true;
  const isCurseOfElements = buffs.curseOfElements === true;
  const isShadowWeaving = buffs.shadowWeaving === true;
  const isImprovedScorch = buffs.improvedScorch === true;
  const isNightfallDebuff = buffs.nightfallProcDebuff === true;
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

