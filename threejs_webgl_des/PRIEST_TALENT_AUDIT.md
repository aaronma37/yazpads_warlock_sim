# Priest talent and DPS audit — October 7, 2026

Audited all 53 talent nodes against the repository's WoW Forever 1.60.1 rank descriptions, exact prerequisites and row requirements; followed every damage/stat effect through `talents.js`, `ui_config.js` and both GPU shader variants. Also checked the current [talent calculator](https://foreverchanges.pro/talents/priest) and [spellbook](https://foreverchanges.pro/spellbook/priest), which advertise build 1.60.1.70245. Local spell data identify builds 69893/69913. This is an audit of the browser runtime, not a claim of full live-client equivalence.

## Confirmed correction

Holy Nova was missing its spell-owned 5% free-cast chance on Holy Fire damage. Previously only Searing Light's 0/5/10% periodic chance was used, so a Priest with Holy Nova but no Searing Light could never proc a free Nova. The GPU now rolls 5% on Holy Fire direct hits and 5% plus Searing Light's bonus on periodic ticks, gated by learned Holy Nova. The existing native simulator already adds the base chance on periodic ticks, but also lacks the direct-hit proc. Native behavior was not changed in this browser task. Base and bonus are modeled additively, consistent with the native implementation; separate live-client proc records would be needed to prove that interpretation.

Regression coverage executes 256 fights per scenario, checks direct-only and periodic procs, verifies the Searing Light bonus and unlearned gate, and compares fast/detailed results exactly.

Measured correction: the 64-fight Holy core fixture rises from 181.69 to 185.74 DPS (+4.05), and the Power Infusion fixture rises from 195.98 to 201.98 DPS (+6.01). Shadow fixture results are unchanged. These figures are specific to the stated fixture inputs and do not explain a 200-DPS shortfall by themselves.

## What does and does not explain a large shortfall

No missing Shadow damage talent multiplier was found. Shadowform (+10% damage, half Shadow mana cost, 2x direct crits), Darkness, Improved Mind Flay, Shadow Weaving's dynamic stacks, Twin Disciplines on instant spells, Shadow Focus, Improved SW:P/MB, Meditation and Inner Focus are applied. Early Demise uses the final 20% of the encounter, as previously requested. Holy Precision, Divine Fury, Power in Light, Spiritual Guidance, Mental Strength, Searing Light, Penance and Improved Healing are applied.

Priest mode currently takes **direct stats** only. It does not apply the shared gear/buff selections, mana potions/runes, external cooldowns or Undead Dark Sacrifice/Touch of the Grave. Human and Undead are the only search races; Night Elf Starshards and Troll Berserking are unavailable. Those omissions are comparison constraints, not talent bonuses that should be silently added. In a no-death/no-incoming-damage single-target encounter, healing/defense/control talents and Spirit Tap's kill trigger provide no extra DPS.

The local spellbook's low Smite/Holy Fire base damage is intentional Forever data, not accidental Classic downranking. The current web spellbook lists modestly different Smite/MB/SW:D direct ranges (167–186, 477–503, 444–472 versus local 160–180, 472–498, 434–462). Whether this is patch drift or level scaling needs spell-record verification; this audit does not replace pinned constants based on rendered tooltips. These small base differences alone do not explain hundreds of DPS.

## Controlled measurements

GPU fixture inputs: 500 generic SP, 0 school-only SP, 12% bonus hit, 15% bonus crit, 200 Intellect, 100 Spirit, 220 Stamina, 20 MP5, 180 seconds, seed 42, 64 fights. These are diagnostic fixtures, not the user's unknown build. “Unlimited mana” changes only maximum mana to isolate the resource bottleneck; it is not a suggested game configuration.

| Fixture before the Nova correction | Mean DPS | Same policy with unlimited mana | Resource gap |
|---|---:|---:|---:|
| Human Shadow core | 409.52 | 550.28 | 140.75 |
| Undead Shadow | 396.88 | 572.29 | 175.40 |

At these inputs, Shadow finishes almost empty and spends substantial time waiting for mana. Missing mana support can therefore account for a gap of roughly this size without any missing talent damage bonus. This does not establish the cause of the user's comparison. The exact build, direct stats, duration and reference DPS remain needed.

Reproduce current GPU measurements with `node validation/priest_dps_audit.mjs` from this directory. Results include spell damage/casts and mana spent/gained/remaining. The script has no GPU skip path.

## Per-talent disposition

“Supported” means the damage-runtime effect is implemented when its action/trigger is available. It does not imply healing, slowing, threat targets, incoming damage or kill triggers exist in the production search encounter.

| Tree | Talent | Damage-runtime disposition |
|---|---|---|
| Discipline | Power in Light | Supported |
| Discipline | Wand Specialization | Supported wand damage; Wand absent from search action set |
| Discipline | Twin Disciplines | Supported |
| Discipline | Silent Resolve | Holy threat supported; CC duration reduction excluded |
| Discipline | Holy Precision | Supported |
| Discipline | Improved Power Word: Shield | Outside single-target damage scope: healing/defense/control |
| Discipline | Martyrdom | Outside single-target damage scope: healing/defense/control |
| Discipline | Mental Agility | Supported |
| Discipline | Inner Focus | Supported |
| Discipline | Meditation | Supported |
| Discipline | Improved Inner Fire | Outside single-target damage scope: healing/defense/control |
| Discipline | Mental Strength | Supported |
| Discipline | Soul Warding | Outside single-target damage scope: healing/defense/control |
| Discipline | Improved Mana Burn | Supported cast reduction; Mana Burn absent from search action set |
| Discipline | Penance | Supported |
| Discipline | Renewed Hope | Outside single-target damage scope: healing/defense/control |
| Discipline | Divine Aegis | Outside single-target damage scope: healing/defense/control |
| Discipline | Power Infusion | Supported |
| Holy | Twilight Focus | Outside single-target damage scope: healing/defense/control |
| Holy | Improved Renew | Outside single-target damage scope: healing/defense/control |
| Holy | Holy Specialization | Supported |
| Holy | Spell Warding | Outside single-target damage scope: healing/defense/control |
| Holy | Divine Fury | Supported |
| Holy | Holy Nova | Supported damage and corrected free-cast proc; healing excluded |
| Holy | Blessed Recovery | Outside single-target damage scope: healing/defense/control |
| Holy | Inspiration | Outside single-target damage scope: healing/defense/control |
| Holy | Holy Reach | Supported range; no gain at default zero distance |
| Holy | Improved Healing | Supported |
| Holy | Searing Light | Supported |
| Holy | Binding Heal | Outside single-target damage scope: healing/defense/control |
| Holy | Litany of Light | Outside single-target damage scope: healing/defense/control |
| Holy | Spirit of Redemption | Outside single-target damage scope: healing/defense/control |
| Holy | Spiritual Guidance | Supported |
| Holy | Spiritual Healing | Outside single-target damage scope: healing/defense/control |
| Holy | Prayer of Mending | Outside single-target damage scope: healing/defense/control |
| Shadow | Shadow Focus | Supported |
| Shadow | Blackout | Outside single-target damage scope: healing/defense/control |
| Shadow | Spirit Tap | Supported kill-trigger regen/SP; inactive with no target deaths |
| Shadow | Shadow Affinity | Supported |
| Shadow | Improved Shadow Word: Pain | Supported |
| Shadow | Shadow Reach | Supported range; no gain at default zero distance |
| Shadow | Improved Mind Blast | Supported |
| Shadow | Improved Psychic Scream | Outside single-target damage scope: healing/defense/control |
| Shadow | Mind Flay | Supported |
| Shadow | Improved Mind Flay | Supported |
| Shadow | Improved Fade | Outside single-target damage scope: healing/defense/control |
| Shadow | Vampiric Embrace | Outside single-target damage scope: healing/defense/control |
| Shadow | Shadow Weaving | Supported |
| Shadow | Silence | Outside single-target damage scope: healing/defense/control |
| Shadow | Devouring Contagion | Supported Plague cost; spread inactive with no deaths |
| Shadow | Early Demise | Supported |
| Shadow | Darkness | Supported |
| Shadow | Shadowform | Supported |
