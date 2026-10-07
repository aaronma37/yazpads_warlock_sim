# Warlock Simulation: Talents Implementation Roadmap & TODO Checklist

This document tracks the audit and implementation status of all 50 Warlock talents across the **C++ CPU Oracle** (`src/sim/warlock/`) and the **Three.js WebGL Discrete Event Simulator** (`threejs_webgl_des/`).

---

## 🎯 Phase 1: High-Priority Core DPS & Mana Talents

These talents directly affect DPS rotation, spell coefficients, and mana pacing.

- [x] **1. `Aftermath` (Destruction R2C3 — 5 Ranks)**
  - **Effect**: Increases the initial direct damage of *Immolate* by 10% per rank (+50% at 5/5).
  - **Status**: **Fully Implemented** (C++ and WebGL shader & talent mapper).

- [x] **2. `Cataclysm` (Destruction R2C2 — 3 Ranks)**
  - **Effect**: Reduces the Mana cost of all Destruction spells (*Shadow Bolt, Searing Pain, Immolate, Incinerate, Conflagrate, Shadowburn, Soul Fire, Hellfire*) by 3% (Rank 1), 6% (Rank 2), and 10% (Rank 3).
  - **Status**: **Fully Implemented** (C++ and WebGL shader & talent mapper).

- [x] **3. `Improved Bane of Agony` (Affliction R3C1 — 2 Ranks)**
  - **Effect**: Increases the damage done by *Bane of Agony* by 5% per rank (+10% at 2/2).
  - **Status**: **Fully Implemented** (C++ and WebGL shader & talent mapper).

- [x] **4. `Fel Vitality` (Demonology R2C3 — 3 Ranks)**
  - **Effect**: Increases maximum Mana for both Player and active Demon Pet by 5% per rank (+15% at 3/3).
  - **Status**: **Fully Implemented** (C++ and WebGL shader & talent mapper).

- [x] **5. `Demonic Pact` (Demonology R7C2 — 1 Rank)**
  - **Effect**: *Demonic Sacrifice* effect is no longer cancelled by summoning an active Demon Pet.
  - **Status**: **Fully Implemented** (C++ and WebGL simulation & optimizer).

---

## 🎯 Phase 2: Active Cooldowns & Multi-Target Talents

- [x] **6. `Amplify Curse` (Affliction R3C3 — 1 Rank)**
  - **Effect**: 3-minute cooldown that increases the base damage of the next *Bane of Agony* by 50% (does not multiply spell power scaling).
  - **Status**: **Fully Implemented** (C++ and WebGL shader & talent mapper).

- [ ] **7. `Bane of Havoc` (Destruction R5C2 — 1 Rank)**
  - **Effect**: Afflicts the target for 5 min, causing 15% of all damage done to other targets to also be dealt to the cursed target.
  - **Action Items**:
    - Implement multi-target cleave damage in `kernel.js` when `targets >= 2` (matching C++ oracle implementation).

---

## 🎯 Phase 3: Defensive, Pet Utility & Pushback Talents

*Note: In standard single-target raid simulation where the player takes no pushback or lethal incoming damage, these have minimal DPS impact, but can be added for completeness.*

- [ ] **8. `Demonic Embrace` (Demonology R1C3 — 5 Ranks)**: +3% to +15% total Stamina (affects Touch of the Grave racial scaling).
- [ ] **9. `Soul Harvesting` (Affliction R2C2 — 2 Ranks)**: Mana regeneration proc upon killing an enemy.
- [ ] **10. `Fel Concentration` (Affliction R3C2 — 3 Ranks)**: 23% to 70% pushback resistance on Drains.
- [ ] **11. `Intensity` (Destruction R4C1 — 3 Ranks)**: 23% to 70% pushback resistance on Destruction spells.
- [ ] **12. `Curse of Exhaustion` (Affliction R4C3 — 1 Rank)**: Movement speed reduction.
- [ ] **13. `Master Summoner` & `Fel Domination` (Demonology R3C3 / R4C3)**: Pet summon cast time reduction and instant summon cooldown.
- [ ] **14. `Improved Voidwalker` & `Improved Felhunter` (Demonology R2C2 / R5C1)**: Voidwalker shield / Felhunter AP debuff utility.
- [ ] **15. `Demonic Aegis` (Demonology R2C1)** & **`Molten Skin` (Destruction R2C1)**: Demon Armor effectiveness / passive damage reduction.
- [ ] **16. `Destructive Reach` (Destruction R1C1 — 2 Ranks)**: +10% / +20% spell range.
- [ ] **17. `Pyroclasm` (Destruction R5C1 — 2 Ranks)**: Stun proc chance on Soul Fire / Hellfire.

---

## 📊 Complete 50-Talent Audit Table

| Tree | Talent Name | Max | In C++ | In WebGL | Notes |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **Affliction** | Improved Life Tap | 2 | ✅ | ✅ | +10% / +20% Mana per tap |
| **Affliction** | Suppression | 5 | ✅ | ✅ | +1% to +5% Spell Hit |
| **Affliction** | Improved Corruption | 5 | ✅ | ✅ | Instant cast + 2%–10% damage |
| **Affliction** | Malediction | 5 | ✅ | ✅ | +1% to +5% Periodic damage |
| **Affliction** | Soul Harvesting | 2 | ❌ | ❌ | On-kill mana regen proc |
| **Affliction** | Improved Drains | 3 | ✅ | ✅ | Drain Life/Soul damage bonus |
| **Affliction** | Improved Bane of Agony | 2 | ✅ | ✅ | +5% / +10% Agony damage |
| **Affliction** | Fel Concentration | 3 | ❌ | ❌ | Pushback resistance |
| **Affliction** | Amplify Curse | 1 | ✅ | ❌ | +50% Agony damage on 3m CD |
| **Affliction** | Pandemic | 3 | ✅ | ✅ | DoT critical strike damage multiplier |
| **Affliction** | Malevolence | 5 | ✅ | ✅ | +1% to +5% Shadow crit chance |
| **Affliction** | Nightfall | 2 | ✅ | ✅ | 2% / 4% instant Shadow Bolt proc |
| **Affliction** | Curse of Exhaustion | 1 | ❌ | ❌ | Movement speed slow utility |
| **Affliction** | Siphon Life | 1 | ✅ | ✅ | Siphon Life spell & DoT debuff |
| **Affliction** | Soul Siphon | 3 | ✅ | ✅ | +17% to +50% Drain damage tick rate |
| **Affliction** | Shadow Mastery | 5 | ✅ | ✅ | +1% to +5% Shadow damage |
| **Affliction** | Wrack | 1 | ✅ | ✅ | 6s channel + 10% Shadow DoT amp |
| **Demonology** | Improved Health Funnel | 2 | ❌ | ❌ | Pet healing utility |
| **Demonology** | Improved Imp | 3 | ✅ | ✅ | +10% to +30% Imp Firebolt damage |
| **Demonology** | Demonic Embrace | 5 | ✅ | ❌ | +3% to +15% total Stamina |
| **Demonology** | Unholy Power | 5 | ✅ | ✅ | +2% to +10% Pet damage |
| **Demonology** | Demonic Aegis | 2 | ❌ | ❌ | Armor / buff effectiveness |
| **Demonology** | Improved Voidwalker | 3 | ❌ | ❌ | Voidwalker threat / shield |
| **Demonology** | Fel Vitality | 3 | ✅ | ❌ | +5% to +15% Player & Pet Max Mana |
| **Demonology** | Demonic Energies | 2 | ✅ | ✅ | Pet leech / healing |
| **Demonology** | Improved Sayaad | 3 | ✅ | ✅ | +10% to +30% Lash of Pain damage |
| **Demonology** | Demonic Sacrifice | 1 | ✅ | ✅ | +15% Shadow / Fire sacrifice buffs |
| **Demonology** | Master Summoner | 2 | ❌ | ❌ | Summon cast time reduction |
| **Demonology** | Decimation | 2 | ✅ | ✅ | Soul Fire CD (-45%) & sub-35% proc |
| **Demonology** | Fel Domination | 1 | ❌ | ❌ | Instant summon CD |
| **Demonology** | Demonic Brand | 3 | ✅ | ✅ | Searing Pain brand debuff |
| **Demonology** | Improved Felhunter | 3 | ❌ | ❌ | AP debuff utility |
| **Demonology** | Soul Link | 1 | ✅ | ✅ | +3% damage & damage share |
| **Demonology** | Demonic Knowledge | 3 | ✅ | ✅ | Pet SP scaling (33% of level) |
| **Demonology** | Master Demonologist | 5 | ✅ | ✅ | +2% to +10% Pet/Warlock damage |
| **Demonology** | Demonic Pact | 1 | ✅ | ✅ | Retains sacrifice with active pet |
| **Destruction** | Destructive Reach | 2 | ❌ | ❌ | Spell range extension |
| **Destruction** | Improved Shadow Bolt | 5 | ✅ | ✅ | +4% to +20% Shadow debuff on crit |
| **Destruction** | Bane | 5 | ✅ | ✅ | -0.1s to -0.5s cast time reduction |
| **Destruction** | Molten Skin | 5 | ❌ | ❌ | 2% to 10% damage reduction |
| **Destruction** | Cataclysm | 3 | ✅ | ✅ | -3% / -6% / -10% Destro mana cost |
| **Destruction** | Aftermath | 5 | ✅ | ✅ | +10%–50% initial Immolate damage |
| **Destruction** | Ruin | 5 | ✅ | ✅ | +20% to +100% Destro crit damage bonus |
| **Destruction** | Shadowburn | 1 | ✅ | ✅ | Instant Shadowburn spell (15s CD) |
| **Destruction** | Intensity | 3 | ❌ | ❌ | Pushback resistance |
| **Destruction** | Agonizing Flames | 3 | ✅ | ✅ | +3%–9% SP crit & +3%–10% Destro dmg |
| **Destruction** | Conflagrate | 1 | ✅ | ✅ | 10s CD spell consuming Immolate |
| **Destruction** | Pyroclasm | 2 | ❌ | ❌ | Stun proc on Soul Fire / Hellfire |
| **Destruction** | Bane of Havoc | 1 | ✅ | ❌ | 15% cleave damage to secondary target |
| **Destruction** | Fire and Brimstone | 3 | ✅ | ✅ | +8% to +25% Conflagrate crit |
| **Destruction** | Shadow and Flame | 5 | ✅ | ✅ | Conflag/Shadowburn elemental buffs |
| **Destruction** | Incinerate | 1 | ✅ | ✅ | Incinerate spell (+25% on Immolate) |
