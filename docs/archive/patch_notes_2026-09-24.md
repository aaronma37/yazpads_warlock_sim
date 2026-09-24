# WoW Forever Beta — Patch Notes (2026-09-24)

> [!NOTE] Archived for reference. See [CHANGELOG.md](../../CHANGELOG.md) for sim-specific impact notes.

---

## Bugfixes

- Fixed a bug with casters not playing their precast animations and sometimes standing while casting.
- Changed the sound effect that plays while jumping in water to be more aesthetically pleasing.
- Adjusted Human death sounds.
- Fixed an issue where attempting to craft items or cast spells was causing players to stand up while waiting to obtain their Campfire buff.
  - Players may now buff each other, cast, and craft while waiting for their Campfire buffs.
  - Players may now obtain a Campfire buff if they are sitting, sitting in a chair, sleeping, lying down, or using the kneeling emote.
- Resolved an issue where Huey Sunnydale's legs failed to render properly.
- Fixed an issue that made a Season of Discovery rune ability visible.
- Fixed an issue where Pet Happiness was not decreasing once your pet was at max loyalty.
- Fixed an issue causing the Undercity Canal ooze to be much brighter than intended.
- Removed a setting that could disable fog in an unintended manner.
- Resolved a water display issue and fixed various other display artifacts on Mac.
- Fixed a memory leak causing gradual performance degradation for some players.
- Fixed an issue where in-game mail could ignore surnames.

---

## Classes

> If you have not recently changed your talents, the cost is now **1 silver** to re-spec. This is temporary for the Beta only. After your first talent reset, the subsequent cost will be much higher, but it will drop to 1 silver after 1 hour has passed.

### Druid

- Corrected an issue and now **Rejuvenation** can land critical hits.
- Corrected an issue and now **Tranquility** can land critical hits.
- **Wrath** base damage increased ~50%.
- **Thorns** will now dynamically update its values based on the caster's spell power.
  - If the caster cannot be found or is too far away, Thorns reverts to its base values.

**Feral**
- Mangle has been renamed **Primal Bite**. Its signature debuff has been removed, so the former name no longer accurately represented the ability.
  - All talents referencing Mangle now reference Primal Bite.
- The **Primal Fury** talent has been renamed and the icon replaced with the original **Blood Frenzy** name and icon.
- Fixed a bug where **Feral Charge (Cat)** could be cast on allies.

**Restoration**
- Corrected an issue and now **Wild Growth** can land critical hits.

---

### Hunter

**Pets**
- **Furious Howl** Attack Power bonus reduced 40%.
- **Sonic Blast** ability now properly available to Bat pets.
- **Tame Beast** no longer works on Beasts that are higher than the Hunter's level.

**Marksmanship**
- Talenting out of **Lone Wolf** will no longer cause the Hunter to have -4% damage while their pet is active.

**Survival**
- Corrected an issue and now **Lacerating Strikes** can land critical hits.
- **Strider Kick** now increases movement speed by 30% for 3 seconds.

---

### Mage

- **Arcane Missiles** no longer checks line of sight on each missile, just once at the beginning of the channel. So targets moving out of line of sight during the channel will not prevent the missiles.

**Fire**
- **Wake of Fire** buff duration has been increased to **30 seconds** (was 20 seconds).
  > Developers' notes: Wake of Fire was difficult to make proper use of while leveling, due to the Mage needing to restore mana between combat. The increased duration should help smooth out the experience.
- **Hot Streak** buff duration has been increased to **20 seconds** (was 15 seconds).
- **Ignite** no longer double dips on % damage increase modifiers.

---

### Paladin

- **Retribution Aura** will now dynamically update its values based on the caster's spell power.
- Fixed a bug causing the **Flash of Light** cast animation to not play at the end of the spell cast.
- Fixed various issues with **Consecration**:
  - Fixed an issue where Consecration would fail to apply its increased damage effect to some targets.
  - Fixed an issue where Consecration's Z axis wasn't functioning consistently.
  - Fixed an issue where Consecration could pull targets vertically above or below the caster.
- **Holy Strike's** % of weapon damage per rank changed to **25%/29%/32%/36%/39%/43%/46%/50%** (was 25%/25%/30%/30%/35%/35%/40%/40%).
  > Developers' notes: This increases Holy Strike's damage at all points while leveling.
- Fixed a bug with **Echo of Justice**, causing it to have a higher than intended chance to activate.
- **Righteous Fury**: Holy threat increase increased to **60%** (was 90%).

**Holy**
- **Holy Strike's** cooldown lowered to **10 seconds** (was 12 seconds).
- **Improved Holy Strike** removed from the Talent Tree.
  > Developers' notes: Its behavior is now baseline.
- **Light's Vigil's** tooltip has been reorganized to make it more clear that the damage component is the only version that returns some of the Mana cost.
- **Holy Power** now increases the Critical Strike chance of Holy Strike by 15% and retains all of its old effects.

**Retribution**
- **Vengeance** now activates off of non-periodic critical strikes.
- **Vengeance's** maximum stacks lowered to **3** (was 5).
- **Two-Handed Weapon Specialization's** damage increase reduced to **2/4/6%** (was 3/6/9%).
- **Sacred Arbiter** now increases the damage of Holy Strike by **20%** (was 10%).
- **Twist of Light** now additionally reduces the Mana cost of your Seal spells by 20%.

---

### Priest

- Corrected an issue and now **Renew** can land critical hits.
- **Power Word: Shield** can now always overwrite an existing Power Word: Shield on a target that does not have Weakened Soul.

**Gnome**
- Corrected an issue and now **Contingency Plan** can land critical hits.
- **Confounding Flash** now has a cast visual.

**Night Elf**
- **Starshards** now correctly gains resistance to spell pushback from the Twilight Focus talent.

**Undead**
- **Dark Sacrifice** no longer breaks Crowd Control that breaks on damage.
- **Dark Sacrifice's** tooltip now correctly displays that the Mana gained from the ability scales with Spirit.

---

### Rogue

- **Sap** now correctly flags the Rogue for PvP when used on a target that is PvP flagged.

---

### Shaman

- **Lightning Bolt**: Damage on ranks 3 and 4 increased to make these spells always upgrades over previous ranks.
- **Windfury Totem**: Buff now displays as "Windfury Totem" instead of "Windfury Totem Passive".
- **Flametongue Totem**: Buff now displays as "Flametongue Totem" instead of "Flametongue Totem Effect".
- **Flametongue Totem** no longer has a duration and is instantly replaced when placing another totem.
- **Flametongue Totem** is no longer able to be stacked by repeatedly casting Flametongue Totem.
- **Flametongue Totem** no longer stacks with Flametongue Weapon.
- **Flametongue Totem** no longer stacks with Windfury Totem.
- **Tranquil Air Totem, Windfury Totem, and Grace of Air Totem** no longer stack together even if used by different Shamans in the same group.

**Elemental**
- Swapped the positions of the **Elemental Fury** (Row 3) and **Elemental Alacrity** (Row 6) talents.
- **Lava Burst** damage on ranks 1 and 2 increased to be about 10% more base damage than the Lightning Bolt rank learned around the same level.

**Enhancement**
- **Rage of the Far Seer** (Row 7) no longer increases the Shaman's Spell Casting Speed.

**Restoration**
- Corrected an issue and now **Riptide** can land critical hits.

---

### Warlock

- Tooltip of **Life Tap** has been updated to correctly display the amount of Life converted, and only states that it scales with Spirit.
- **Voidwalker Sacrifice** now correctly scales with 10% of the Warlock's spell healing.

---

### Warrior

- **Sunder Armor**: Corrected the threat values on all ranks, including a small increase to threat generated from Attack Power.

**Protection**
- Swapped the positions of the **Bastion** (Row 5) and **Focused Rage** (Row 6) talents.

**Arms**
- **Bloodthrill's** chance to activate increased to **4/8/12/16/20%** (was 2/4/6/8/10%).
- **Bloodthrill** only activates off of Main Hand melee attacks. This includes Cleave and Heroic Strike.
- **Slam's** cooldown has been increased to **18 seconds** (was 15 seconds).
- **Improved Slam** now reduces the cooldown of Slam by **1.5/3 seconds**.

---

## Races

### Gnome
- Changed **Eureka** on every class to a **10% discount on Mana, Rage, or Energy abilities**.

### Human
- Changed the **Will to Survive** visual to match Dispel Magic instead of Will of the Forsaken.

### Tauren
- **Cultivation** now has a player-level requirement for every Herb.

### Undead
- **Touch of the Grave** no longer breaks CC or activates from spells and abilities that don't have a damage component to them.

---

## Cooldown Manager

- Druid, Mage, Priest, Warrior, and Warlock are available for testing.
- Does not yet support spell ranks.

---

## Gamepad Support

- Class specific flyouts added for Druids, Hunters, Warlocks, Paladins, Warriors.
- New options: Use Compact Action Bar, Fixed Party Targeting, Swap Target Modifier Sides, Swap Friendly/Hostile Target Action Sides.
- Fixed an issue preventing edit mode layouts from being applied while the gamepad UI is enabled.
- Updated Map and Quest Log bindings with new hover functionality for POI pins.
- Updated support for Compact Raid Frame Manager and Role Poll Frames.

---

## Items

- A handful of guns and bows were not correctly consuming or using ammunition. They have been corrected.
- A number of quests have had their rewards updated to be the correct item level.
- A small number of early caster rewards have been adjusted.
- **Wizard Oil**: Minor Wizard Oil reverted to Classic Era value of 8 spell power, Lesser Wizard Oil to 16, Wizard Oil to 24. Brilliant Wizard Oil unchanged.
- **Wands**: Wands no longer gain any additional damage from the user's spell damage.

---

## Quests

- Dalaran — Faction adjustments. Spawns are now present in the area just outside the city entrance.
- Dun Morogh — Snow Leopard Prowler has a much faster respawn time.
- Dun Morogh — Resolved an issue where some players were unable to loot the rifle for "Treacherous Cold".
- Durotar — Reduced the respawn time for the escort NPC for "Lost in the Shadows".
- Elwynn Forest — Various quest fixes: Book Return, Stolen Enchanting Supplies, Crystal Lake Murlocs, Mother Fang, Shinyfinder Narf, Nimsy, Helene Peltskinner skinning, Jasperlode/Fargodeep Mines.
- Mulgore — Water Pitchers now respawn faster for the quest "A Humble Task".
- Silthus — Khonsu's arrival has been removed from the beta phase.
- Silverpine Forest — Pyrewood Village worgen respawn more quickly.
- Teldrassil — "Bounty: Gnarlpine Furbolg" objective count reduced to **15** (was 20).
- Undercity — Skyborne quest "Exploring the Horde" now completes the Lady Sylvanas objective.
- Westfall — Increased the spawn rate of the Defias Messenger.
- Zephras Isle — "Catching Wind" redesigned; "Wounds of Betrayal" and "A Firm Response" credit fixes.

---

## User Interface

- Updated the Nameplate selection display.
- Personal Resource Display textures are updated.

---

## Known Issues (New to This Build)

- Banes and Curses do not properly track if you drop your target and re-acquire a target with an existing bane or curse.
- Healthstone cooldown cannot be tracked.
- Priest tracks all spells by default.
- Arcane Blast cannot be tracked.
- Mana gem requires an additional spell to be cast by the mage to properly begin tracking its cooldown.
- Creating or changing party composition may transfer players to a different world instance.
- Pet cooldowns cannot be tracked.
- Item enhancements (such as Windfury or rogue poisons) cannot be tracked.
- The minimap in Stormwind has display issues and will be fixed in a later build.
- Personal Resource Display doesn't show combo points.
