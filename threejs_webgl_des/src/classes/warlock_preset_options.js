// Pure Warlock preset pet/sacrifice inference; preserves existing precedence.
export function getPresetPetAndSac(p) {
  if (p && p.pet !== undefined && p.sac !== undefined) {
    return { pet: p.pet, sac: p.sac };
  }
  const name = (p?.name || '').toLowerCase();
  const demoTalents = p?.talents?.demonology || {};
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

