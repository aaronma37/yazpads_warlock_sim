# Warlock Sim — Changelog

Tracks game-mechanic changes from WoW Forever patch notes that affect simulation accuracy.
Full raw patch notes are archived in [`docs/archive/`](docs/archive/).

---

## 2026-09-24

### ✅ Implemented

#### Gnome — Eureka — Mana Discount Reduced
> **Source:** Races → Gnome

**Old behavior:** Eureka grants 3 charges, each giving **50% mana discount** and **+10% damage** on the next cast.

**New behavior:** Eureka grants a **10% mana discount** per charge. The **+10% damage** bonus is unchanged.

- [x] Updated Eureka mana cost multiplier from `0.5` → `0.9` across all 25 spell sites in `warlock_sim.cpp`
- [x] Updated log strings from `"-50% Mana"` → `"-10% Mana"`

#### Wands — No Spell Power Scaling
> **Source:** Items → Wands

**Change:** Wands no longer gain additional damage from the user's spell damage.

- [ ] Verify how wand items (Slot::RANGED) currently calculate damage in the sim
- [ ] If wand damage was adding any `stats.spell_power` coefficient, remove it
- [ ] Wand base DPS values in `gear.cpp` are unaffected (those are item stats, not scaling)

---

### ✅ No Sim Impact

The following changes from this patch have **no impact** on the Warlock sim:

| Change | Reason |
|--------|---------|
| Warlock — Life Tap tooltip update | Tooltip only; mechanics unchanged |
| Warlock — Voidwalker Sacrifice scales with 10% spell healing | The sim does not model Voidwalker Sacrifice healing |
| Caster animation fixes | Client-side only |
| Druid / Hunter / Mage / Paladin / Priest / Shaman / Warrior changes | Out of scope |
| Quest / UI / Gamepad / Cooldown Manager changes | Out of scope |
| Wizard Oil value reverts | Not currently modeled in the sim |

---

_For the full patch notes see [`docs/archive/patch_notes_2026-09-24.md`](docs/archive/patch_notes_2026-09-24.md)._
