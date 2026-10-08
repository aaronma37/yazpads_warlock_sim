# Priest racials in the WebGL simulator

The six Forever Priest races are supported in the current build and constrained search. Race effects start from the same neutral direct stats as talents. Human Spirit is applied before Spiritual Guidance and regeneration; Gnome maximum mana is applied after Mental Strength. A mace must be selected for Dwarf's 1% spell crit. Dwarf and Troll gain 5% damage only against a Beast target.

Devouring Plague is a shared Priest spell, as stated by the [requested guide](https://conquestcapped.com/guides/wow-forever/wow-forever-priest-talents/) and [Forever trainer data](https://foreverdb.net/guide/leveling-priest). The guide's channeled Starshards wording is older than the linked [instant-cast spell data](https://foreverdb.net/spell/19305); Starshards remains instant.

| Race | Class spells | General damage/resource effects |
| --- | --- | --- |
| Human | Divine Grace, Feedback | 5% Spirit |
| Undead | Dark Sacrifice, Touch of Weakness | Touch of the Grave |
| Night Elf | Starshards, Elune's Grace | Elune's Light |
| Dwarf | Chastise, Desperate Prayer | Big Game Hunter, conditional Mace Specialization |
| Gnome | Confounding Flash, Contingency Plan | Eureka!, 5% maximum mana |
| Troll | Hex of Weakness, Shadowguard | Berserking, Beast Slaying |

Class spell tooltips, coefficients, icons, source URLs and coverage are in `src/priest/racials.js`. Structured reference data is also in `../data/forever/priest_racials.json`. The engine uses max-rank level-60 values.

## Combat behavior

Elune's Light grants 10 percentage points of crit for 15 seconds every 180 seconds. Berserking gives 10% haste for 10 seconds every 180 seconds. Cast and channel times and the spell GCD are sped up; GCD has a one-second floor. DoT tick intervals and spell cooldowns stay fixed. See [Elune's Light](https://www.wowhead.com/forever/spell=1259799/elunes-light) and [racial data](https://foreverdb.net/races).

[Eureka!](https://www.wowhead.com/forever/spell=1259823/eureka) has three charges, a 15-second window and a 120-second cooldown. Qualifying direct spells and channels consume one charge at spell start, including misses, and snapshot 10% bonus damage and a 10% mana discount. Pure DoTs neither consume charges nor gain bonus damage; Holy Fire's direct hit qualifies, its DoT does not.

[Touch of the Grave](https://foreverdb.net/spell/1260198) rolls a 10% chance once per damaging spell cast, including DoT applications, without recursion or rolls on periodic ticks. Each proc deals a fixed 5% of maximum Health and heals the caster by that amount, capped by missing Health. An internal cooldown is not specified by the available data, so the default is zero; `graveProcInterval` allows explicit encounter experimentation. [Blizzard's notes](https://foreverdb.net/news/post/forums-us-30185131) confirm that periodic ticks do not trigger it.

## Encounter-dependent spells

The Racial encounter settings expose one enemy's incoming attack interval/type/damage, mana, healing, distance, creature type and control immunity. An independent incoming-hit schedule for a 5000-Health ally exercises Divine Grace and Contingency Plan. Zero attack intervals disable their schedules. Priests take damage and can die; completed DPS still uses the configured encounter duration. There is no implicit incoming damage in the default boss encounter.

Utility spells are conditional support actions before the damage APL. They pay their mana/GCD costs and respect cooldowns, ranges, immunity and Shadowform's healing restriction. They are not coevolved APL actions. Chastise and Starshards are damage APL actions and can be disabled/reordered by the existing policy controls.

- Dark Sacrifice activates when mana deficit covers its return and Health exceeds 1600. Five three-second ticks spend 320 Health and restore 320 + 20% Spirit mana each, capped at maximum mana. Health loss is limited to leave one Health. [Updated tooltip](https://foreverdb.net/spell/1277328).
- Feedback requires incoming spells and enemy mana. It burns at most the remaining enemy mana, with one Shadow damage per mana burned, and no spell-power coefficient. [Spell data](https://foreverdb.net/spell/13896).
- Touch of Weakness arms one melee retaliation. At rank 6 it deals 56 + 0.107 × Shadow power and applies -204 melee attack power. Hex applies the same attack-power reduction and cuts modeled enemy healing by 20%; these attack-power debuffs do not stack. The attack-power component of each melee swing is reduced by `204 / 14 × enemyAttackSpeed`. [Buff](https://www.wowhead.com/forever/spell=19266/touch-of-weakness), [trigger coefficient](https://foreverdb.net/spell/19254), [Hex](https://foreverdb.net/spell/19285).
- Shadowguard has three charges and a 3.5-second proc interval, then is recast if incoming attacks continue. It generates zero threat. Its current tooltip says 96, while the aura still says 116 and its trigger is a server script. The model follows 96 + 0.267 × Shadow power. This disagreement remains an explicit limitation. [Buff data](https://www.wowhead.com/forever/spell=19312/shadowguard), [server-side trigger](https://www.wowhead.com/forever/spell=28376/shadowguard).
- Chastise deals 272–306 + 0.143 × Holy power at 20 yd, only to Humanoids, every 120 seconds. A non-immune rooted enemy cannot perform an out-of-reach melee swing for two seconds. The “very low threat” tooltip is approximated as 10% of damage. [Spell data](https://www.wowhead.com/forever/spell=1277335/chastise).
- Desperate Prayer is used below 50% player Health. Divine Grace requires a living ally below 50%, cannot heal the caster, and clears modeled Weakened Soul. Both use 1318–1546 + 0.429 × healing power at level 60, and can crit. Healing talents, instant-spell bonuses and Power Infusion apply. [Prayer](https://www.wowhead.com/forever/spell=19243/desperate-prayer), [Grace](https://www.wowhead.com/forever/spell=1277378/divine-grace).
- Contingency Plan lasts 30 seconds. After a hit leaves a living ally below 35% Health, it gives a 926 shield for subsequent hits and five healing ticks totaling 670 base healing over 15 seconds. It does not absorb the triggering hit or resurrect an ally. Its healing can crit; Spiritual Healing applies. Client entries are server-side placeholders, so unverified spell-power scaling is not invented. [Level-60 tooltip](https://www.wowhead.com/forever/spell=1277640/contingency-plan).
- Confounding Flash casts for 0.5 seconds, has no GCD, costs 3% base mana, and confuses the single modeled enemy within 8 yd for three seconds; any damage breaks it. Control-immune enemies are excluded. [Spell data](https://www.wowhead.com/forever/spell=1277455/confounding-flash).
- Elune's Grace subtracts 50 percentage points from incoming melee/ranged hit chance, for 15 seconds or three misses, on a 300-second cooldown. It costs 3% base mana. Night Elf Quickness also supplies 1% melee dodge. [Spell data](https://foreverdb.net/spell/2651).

Detailed results include casts, damage, healing, absorbed damage, mana gained from Dark Sacrifice, health spent, damage taken/prevented and enemy healing prevented. Healing and absorption never contribute to damage fitness. Fast and detailed shaders execute the same mechanics and RNG.

## Scope

This is a single-target damage encounter with supporting racial health/control events, not a full healing or PvP simulator. It does not model movement, stealth, corpse availability, player CC, disease/poison/bleed applications, normal Health regeneration, incoming cast pushback, threat-driven targeting or a party shielding rotation. Thus Perception, Will to Survive, Will of the Forsaken, Shadowmeld, Wisp Spirit, Escape Artist, Cannibalize, Stoneform, Regeneration and profession/weapon-melee bonuses without applicable actions do not receive invented DPS contributions. The ally's modeled Weakened Soul can be cleared, but it does not imply a complete Power Word: Shield implementation.

`validation/priest_racials.test.mjs` executes numerical combat fixtures on actual GLES for these racials, checks conditional triggers and compares fast/detailed output.
