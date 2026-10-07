// Warlock presentation data only. No simulation mechanics or engine imports.
// URLs resolve relative to this module so static hosting under a subpath works.
const raceDetails = {
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
      { name: 'Hardiness', icon: 'Spell_Shadow_AntiShadow.png', title: 'Hardiness (+25% Stun Resist)', desc: 'Chance to resist Stun effects increased by an additional 25%.' }
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

const petIcons = {
  imp: './assets/icons/Spell_Shadow_SummonImp.png',
  succubus: './assets/icons/Spell_Shadow_SummonSuccubus.png',
  none: null
};

const sacrificeIcons = {
  succubus: './assets/icons/Spell_Shadow_SummonSuccubus.png',
  imp: './assets/icons/Spell_Shadow_SummonImp.png',
  none: null
};

const resultSpellIcons = {
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

const presetSpellIcons = {
  LIFE_TAP: 'Spell_Shadow_BurningSpirit.png',
  CORRUPTION: 'Spell_Shadow_AbominationExplosion.png',
  IMMOLATE: 'Spell_Fire_Immolation.png',
  SEARING_PAIN: 'Spell_Fire_SoulBurn.png',
  SHADOW_BOLT: 'Spell_Shadow_ShadowBolt.png',
  CONFLAGRATE: 'Spell_Fire_Fireball.png',
  INCINERATE: 'Spell_Fire_Burnout.png',
  DRAIN_SOUL: 'Spell_Shadow_Haunting.png',
  SIPHON_LIFE: 'Spell_Shadow_Requiem.png',
  AMPLIFY_CURSE: 'Spell_Shadow_Contagion.png',
  CURSE_OF_AGONY: 'Spell_Shadow_CurseOfSargeras.png',
  CURSE_OF_DOOM: 'Spell_Shadow_AuraOfDarkness.png',
  BANE_OF_AGONY: 'Spell_Shadow_CurseOfSargeras.png',
  BANE_OF_DOOM: 'Spell_Shadow_AuraOfDarkness.png',
  DEMONIC_BRAND: 'ability_demonhunter_chaoticimprint_fire.png',
  HELLFIRE: 'Spell_Fire_Incinerate.png',
  DRAIN_HOPE: 'ability_deathknight_hemorrhagicfever.png',
  NIGHTFALL: 'Spell_Shadow_Twilight.png',
  SHADOWBURN: 'Spell_Shadow_ScourgeBuild.png',
  WRACK: 'ability_deathknight_hemorrhagicfever.png',
  DECIMATESEARING: 'Spell_Fire_SoulBurn.png',
  DECIMATESOULFIRE: 'Spell_Fire_Fireball.png',
  DECIMATION_SEARING_PAIN: 'Spell_Fire_SoulBurn.png',
  DECIMATION_SOUL_FIRE: 'Spell_Fire_Fireball.png',
  PET_FIREBOLT: 'Spell_Fire_FireBolt.png',
  IMP_FIREBOLT: 'Spell_Fire_FireBolt.png',
  PET_LASH_OF_PAIN: 'Spell_Shadow_Curse.png',
  SUCCUBUS_LASH_OF_PAIN: 'Spell_Shadow_Curse.png',
  PET_MELEE: 'Ability_MeleeDamage.png',
  SUCCUBUS_MELEE: 'Ability_MeleeDamage.png',
  MELEE: 'Ability_MeleeDamage.png',
  PET_IMP: 'Spell_Shadow_SummonImp.png',
  PET_SUCCUBUS: 'Spell_Shadow_SummonSuccubus.png',
  SAC_IMP: 'Spell_Shadow_SummonImp.png',
  SAC_SUCCUBUS: 'Spell_Shadow_SummonSuccubus.png',
  SACRIFICE: 'Spell_Shadow_RitualOfSacrifice.png'
};

const presetRaceIcons = {
  Human: 'Achievement_Character_Human_Male.png',
  Orc: 'Achievement_Character_Orc_Male.png',
  Undead: 'Achievement_Character_Undead_Male.png',
  Troll: 'Achievement_Character_Troll_Male.png',
  Gnome: 'Achievement_Character_Gnome_Male.png'
};

export const WARLOCK_PRESENTATION = Object.freeze({
  id: 'warlock',
  label: 'Warlock',
  pageTitle: 'Classic WoW Warlock DES Simulator & Web Armory',
  brandTitle: 'WARLOCK SIMULATOR',
  activeButtonTitle: 'Warlock (Active)',
  icon: new URL('../../assets/icons/Class_Warlock.png', import.meta.url).href,
  talentDataUrl: new URL('../../data/talents.json', import.meta.url),
  presetDataUrl: new URL('../../data/presets.json', import.meta.url),
  talentBackgrounds: Object.freeze({
    affliction: new URL('../../assets/backgrounds/affliction_bg.png', import.meta.url).href,
    demonology: new URL('../../assets/backgrounds/demonology_bg.png', import.meta.url).href,
    destruction: new URL('../../assets/backgrounds/destruction_bg.png', import.meta.url).href,
  }),
  talentDefaultIcon: 'Spell_Shadow_DeathCoil.png',
  talentFallbackIcon: 'spell_shadow_burningspirit.png',
  raceDetails,
  petIcons,
  sacrificeIcons,
  resultSpellIcons,
  presetSpellIcons,
  presetRaceIcons,
});
