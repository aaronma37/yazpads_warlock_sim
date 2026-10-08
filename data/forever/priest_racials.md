# WoW Classic Forever - Priest Racial Spells & Abilities Reference
Source: https://hyjal.cc/racials
Original import: build 1.60.1.69876; racial spell details verified 2026-10-07.
Updated simulation coverage and source links: [Priest racials](../../threejs_webgl_des/PRIEST_RACIALS.md).
Devouring Plague and Fear Ward are shared spells for all Priest races.

---

## 1. Undead
### **Touch of Weakness**
- **Spell / triggered effect**: 19266 / 19254
- **Cost**: 195 Mana | Instant
- **Damage**: 56 + 0.107 × Shadow spell power on the next incoming melee hit.
- **Debuff**: -204 melee attack power for 2 min.

### **Dark Sacrifice**
- **Spell ID**: 1277328
- **Icon**: `spell_holy_powerinfusion_shadow`
- **Cost**: Instant
- **Cooldown**: 10 min cooldown
- **Tooltip**: Cannibalize 1600 of your own Health over 15 sec to gain 1600 (+100% of Spirit) Mana.

*(Also gains: Will of the Forsaken, Cannibalize, Touch of the Grave - 10% chance to drain up to 5% Max HP, Underwater Breathing)*

---

## 2. Dwarf
### **Chastise**
- **Spell ID**: 1277335
- **Icon**: `spell_holy_chastise`
- **Cost**: 225 Mana | 20 yd range | Instant
- **Cooldown**: 2 min cooldown
- **Tooltip**: Chastise the target, causing 272 to 306 Holy damage and Immobilizing them for up to 2 sec. Only works against Humanoids. This spell causes very low threat.

### **Desperate Prayer**
- **Spell ID**: 19243
- **Icon**: `spell_holy_restoration`
- **Cost**: Instant
- **Cooldown**: 10 min cooldown
- **Tooltip**: Instantly heals the caster for 1318 to 1546.

*(Also gains: Stoneform - +10% Armor, Bleed/Poison/Disease immune for 8s, Mace Specialization - +1% spell crit with a mace, Big Game Hunter - +5% damage to Beasts, Find Treasure)*

---

## 3. Gnome
### **Confounding Flash**
- **Spell ID**: 1277455
- **Icon**: `ability_paladin_blindinglight2`
- **Cost**: 3% base Mana | 0.5 sec cast
- **Cooldown**: 2 min cooldown
- **Tooltip**: Confuses up to 5 enemies within 8 yds for 3 sec. Any damage taken will break the effect.

### **Contingency Plan**
- **Spell ID**: 1277640
- **Icon**: `ability_priest_soulwarding`
- **Cost**: 30 yd range | Instant
- **Cooldown**: 10 min cooldown
- **Tooltip**: Place a Holy ward on an ally for 30 sec. The next time this ally takes damage dropping their Health below 35%, they will gain a shield absorbing 926 damage and begin healing for 670 Health over 15 sec. A target may be affected by only one Contingency Plan.

*(Also gains: Expansive Mind - +5% Mana, Eureka! - 3 non-periodic spell charges in 15 sec; +10% damage/healing and -10% mana on a 2 min cooldown, Escape Artist, Engineering Specialization)*

---

## 4. Human
### **Divine Grace**
- **Spell ID**: 1277378
- **Icon**: `ability_priest_savinggrace`
- **Cost**: 40 yd range | Instant
- **Cooldown**: 10 min cooldown
- **Tooltip**: Instantly heals a friendly target below 50% Health for 1318 to 1546 and removes Weakened Soul from that target. Cannot be cast on self.

### **Feedback**
- **Spell ID**: 19275
- **Icon**: `spell_shadow_ritualofsacrifice`
- **Cost**: 230 Mana | Instant
- **Cooldown**: 3 min cooldown
- **Tooltip**: The priest becomes surrounded with anti-magic energy. Any successful spell cast against the priest will burn 105 of the attacker's Mana, causing 1 Shadow damage for each point of Mana burned. Lasts 15 sec.

*(Also gains: The Human Spirit - +5% Spirit, Perception, Sword Specialization)*

---

## 5. Troll
### **Hex of Weakness**
- **Spell ID**: 19285
- **Cost**: 240 Mana | 30 yd | Instant
- **Debuff**: -204 melee attack power and -20% healing received for 2 min.

### **Shadowguard**
- **Spell ID**: 19312
- **Icon**: `spell_nature_lightningshield`
- **Cost**: 250 Mana | Instant
- **Cooldown**: None (Buff)
- **Tooltip**: The caster is surrounded by shadows. When a spell, melee or ranged attack hits the caster, the attacker will be struck for 96 Shadow damage. Attackers can only be damaged once every few seconds. This damage causes no threat. 3 charges. Lasts 10 min.

*(Also gains: Berserking - +10% Haste for 10s on a 3 min cooldown, Beast Slaying - +5% vs Beasts, Regeneration)*

---

## 6. Night Elf
### **Starshards**
- **Updated source**: https://foreverdb.net/spell/19305
- **Required level / rank**: 58 / 7
- **Ticks**: six Arcane ticks, every 1 sec; 300 + 0.167 × spell power per tick (1.002 total coefficient).
- **Spell ID**: 19305
- **Icon**: `spell_arcane_starfire`
- **Cost**: 350 Mana | 30 yd range | Instant
- **Cooldown**: 30 sec cooldown
- **Tooltip**: Rains starshards down on the enemy target's head, causing 1800 Arcane damage over 6 sec.

### **Elune's Grace**
- **Spell ID**: 2651
- **Icon**: `spell_holy_elunesgrace`
- **Cost**: 3% base Mana | Instant
- **Cooldown**: 5 min cooldown
- **Tooltip**: Reduces the chance you'll be hit by melee and ranged attacks by 50% for 15 sec or until you are missed 3 times.

*(Also gains: Elune's Light - +10% crit for 15 sec on a 3 min cooldown, Shadowmeld, Quickness - +1% Dodge, Wisp Spirit)*
