# Priest implementation preparation

Class choice: Priest, explicitly selected by the user. Initial specialization remains pending; Shadow DPS is the recommended first milestone. This is a local repository audit, not a live verification of external game data. No Priest class is registered in the WebGL app yet.

## Existing CPU reference

| Area | Repository reference | Audit result |
| --- | --- | --- |
| Combat engine | `../src/sim/priest/priest_sim.cpp`, `.hpp` | Event-driven simulator, mana/cooldowns/DoTs/channels, per-spell results and optional event logs |
| Spell IDs/values | `../src/sim/priest/spells.hpp` | Priest-local IDs; Shadow and Holy spells, utility, consumables, racial actions |
| Talents | `../src/sim/priest/talents.hpp`, `talent_graph.hpp` | 53 nodes: Discipline 18, Holy 17, Shadow 18; 51-point builds, prerequisites, repair |
| Policies | `../src/sim/priest/policy.hpp` | Seven rotation choices; standard/custom rule display API |
| Presets | `../src/sim/priest/spec_presets.hpp` | Shadow 14/0/37, deep Shadow 10/0/41, Smite 14/37/0, PI Smite 21/30/0 |
| Search | `../src/sim/priest/optimizer.cpp`, `.hpp` | Preset comparison and genetic search with talent constraints |
| Batch execution | `../src/sim/priest/parallel_runner.cpp`, `.hpp` | CPU batch reference; seed/scheduling audit still required |
| Saved results/builds | `../src/sim/priest/build_export.hpp` | CPU `priest-build/1` / `priest-specs-batch/1`; not browser v1 envelopes |
| Tests | `../tests/test_priest_sim.cpp` | 19 Priest tests; all passed during this audit |

Validation command: `cmake --build build --target sim_tests -j2`, then `bin/sim_tests`. Full CPU suite: 224/231 passed. The seven failures were GnomeEurekaManaAndDamageBonus, CorruptionSpellPowerCoefficient, WrackShadowDotAmplification, CustomDemonologyFireSearingPain, ContrastiveTrajectoryDiffEvaluation, MultiDotAndCurseBreakdown, and PetsAndDemonicSacrifice. No failure was in the PriestSim group. This is regression evidence, not full Priest mechanics or exact GPU parity coverage.

Priest tests cover Shadow/Smite damage, Devouring Plague healing, SW:Death backlash, Holy hit talents, Holy Fire/Penance, talent legality/repair, optimizer legality, Undead damage procs, and rotation coverage. Most combat assertions are broad output/accounting checks, not exact deterministic event/RNG fixtures.

## Ruleset and data

The CPU implementation and stored references use WoW Forever modifications. Do not silently substitute stock Classic, TBC, or current retail Priest rules.

- `../data/forever/priest_talents.json`: WoW Forever 1.60.1; 18/17/18 nodes agree with CPU graph counts. JSON trees are objects, rows/columns are zero-based, and node IDs/descriptions/prerequisites need explicit conversion to the WebGL talent schema. Counts alone do not establish node/effect parity.
- `../data/forever/priest_spells.json`: recorded builds 1.60.1.69893 / 1.60.1.69913, 52 spell entries across Discipline/Holy/Shadow Magic. Rank records contain readable Forever/Classic comparisons. Top-level spell fields can contain serialized reference strings; extract concrete rank records, not those reference strings.
- `../data/forever/priest_racials.json`: includes Undead, Dwarf, Gnome, Human, Troll, Night Elf references and per-entry client-build/source metadata. A legal playable race list must be audited independently; a reference entry or shared CPU race enum is not proof of class legality.
- Matching `.md` files provide readable reference descriptions. Their source links are provenance only; this audit did not check live pages.
- All 53 talent icon names have matching root `assets/icons` files when matched case-insensitively. Exact filename case and copying/caching browser-local assets need attention on Linux. Priest talent backgrounds and complete spell/racial asset coverage remain unverified.

Examples of CPU Shadow values: SW:P rank 8 costs 470 mana, six 3-second ticks with 127 base damage and 0.20 coefficient per tick; Mind Flay rank 6 costs 205 mana and uses three one-second ticks with 130 base damage and 0.1667 coefficient; Mind Blast rank 9 costs 350 mana, 1.5-second cast, 8-second base cooldown, 472–498 base damage. These are observed local implementation values, not independently certified game facts.

## CPU behavior requiring explicit decisions/fixtures

1. **Custom APL oracle gap:** `PolicyConfig` returns custom rules for display/export, but `PriestSimulator::run_single_simulation` executes a switch on `policy.rotation`; it does not consume `use_custom_apl`/`custom_rules`. Arbitrary per-lane APLs cannot yet be validated against this CPU implementation. Add CPU execution support or scope the first oracle to fixed policies; do not advertise custom-APL parity prematurely.
2. **Accounting duplication:** `record_spell_miss` increments aggregate misses, and several call sites increment `result.misses` again. Per-spell misses and aggregate misses may disagree. Separate combat parity from diagnostic-accounting fixes.
3. **Mana accounting:** mana is capped on regen/potions/runes while `mana_gained` accumulates the nominal grant. Define gross/accepted/overflow reporting before comparing resource totals.
4. **Resistance:** Shadow resistance is reduced for Curse of Shadows before passing to `calculate_partial_resist_multiplier`, which also subtracts the curse. `deal_damage` passes the same `shadow_res` variable for Holy spells. Isolate curse/penetration/school fixtures before carrying this into Priest shaders.
5. **Execute timing:** execute-only SW:Death tests remaining-duration fraction (`<= 0.20`), not simulated target health depletion. Declare this encounter convention explicitly.
6. **Channel timing/clipping:** Mind Flay clipping checks at tick two; channel events share one mutable channel state. Event ties, immediate first ticks, remaining ticks after clipping, and reaction/GCD scheduling need deterministic event fixtures.
7. **Stats/defaults:** Priest starts with class/race base attributes and applies buffs before racial/talent modifiers. Default constructor uses shared gear data and folds shadow power into spell power. Preserve a clear raw-stat/base-attribute contract; do not reuse the Warlock stat/race conversion blindly.
8. **Healing/self-damage:** CPU records Vampiric Embrace/Plague healing and SW:Death backlash. Health-death semantics are not established merely by having self-damage counters. First scope should state whether these are diagnostics or combat survival constraints.

The CPU is a valuable behavioral reference, but a port should not silently fix these behaviors or treat them as verified mechanics. Record fixes or unsupported cases separately and generate scoped oracle fixtures before shader implementation.

## Selected first milestone: Shadow DPS

Level-60 WoW Forever Shadow DPS, one level-63 target, fixed encounter duration, direct stats, no pet controls. Start with the existing Shadow 14/0/37 preset and SW:P/Mind Blast/Mind Flay policy; add SW:Death, Shadowform, Shadow Weaving, Inner Focus and mana regen through separately verified steps. Mind Flay needs tick/clipping coverage before the first end-to-end readiness claim.

Use one race initially (Human is the simplest CPU default). Disable unsupported racial actions and consumables explicitly until their fixtures exist. Devouring Plague, Berserking, Dark Sacrifice, Starshards, Touch of the Grave, Power Infusion, additional Shadow policies, and Smite/Holy can follow with audited data. If a supported preset allocates talents whose effects are deferred, reject or explicitly scope those effects rather than silently ignoring them.

Healing/tanking objectives, multi-target/AoE output, incoming-damage reactive racials, all-race support, and full Holy/Discipline support are not implied by this first milestone. The user selected Priest, not all Priest mechanics.

## Remaining shared app assumptions exposed by Priest

- **Talents/presets:** Warlock tree names/node counts/default helpers and data URLs; introduce Priest metadata/schema adapters and stable panel contents.
- **APL tooling:** Warlock action aliases, spell/condition IDs, two-clause rule construction, fallback actions, regret alternatives, and synthesis assumptions; declare Priest capabilities and class-owned mappings.
- **Stats/resources/buffs:** add Holy power where supported; Priest base mana, spirit regen/5SR, Meditation and no Life Tap; prevent Warlock racial/gear/buff normalization from handling Priest inputs.
- **Controls:** pet/sacrifice area becomes a reserved no-pet/class-options area without moving panels. Shadowform/Inner Focus and channel choices belong in existing class-control locations.
- **Execution/results:** class-owned shader fast/detailed implementation, packing/layout/version IDs, per-spell output/resource mappings, timelines and class identity in comparisons. Current runtime resolver validates class contracts, but engine internals still implement Warlock.
- **Search:** existing factories accept class references but still assume particular graph/identity/config function names and preset flow. Priest needs compatible class-owned operations or explicit capabilities that reject unavailable features.
- **Migration:** legacy untagged browser builds stay Warlock. CPU Priest export formats need a separate audited adapter if requested; do not reinterpret them as browser logical builds.

## Next work

- [x] User selected Shadow DPS first and required the Warlock fast/detailed DES structure.
- [ ] Build a standalone CPU Priest fixture exporter using explicit raw stats, disabled random duration, fixed seed, and recordable events; keep it separate from Warlock fixture generation.
- [ ] Record source hashes, RNG conventions, baseline output, event ordering, and accounting limitations in fixtures.
- [ ] Verify talent node identity/geometry/prerequisite mapping and copy class assets with exact case into browser data.
- [ ] Define Priest metadata/defaults/search/import contracts without registering unsupported simulation capabilities.
- [ ] Implement runtime UI class selection only after its dependent views/controls can consume Priest metadata.
- [ ] Implement scoped Priest detailed/fast shaders and compare against explicit CPU cases; establish tolerances before expanding mechanics.

## UI preview follow-up

The browser Priest button now opens a presentation-only preview with the three 51-point CPU presets and 53 talents. Existing Warlock controls and results are retained when switching back. Priest simulation and search remain unavailable until class-owned runtime integration is implemented.

Additional audit finding: `create_forever_pi_smite()` spends 22 Discipline and 30 Holy points, totaling 52, despite the preset's advertised 21/30/0. It is excluded from the browser selector pending correction and legality review.

Automated checks cover icon availability, preset rank bounds/counts, blocked switching during runs, and restoration of original DOM nodes/tab over repeated switches. They do not establish visual browser correctness.

The preview now shares the existing direct-stat, target and simulation panel markup, including input defaults and responsive column wrappers. Holy Power replaces Fire Power. Input values/preset selection persist locally between switches. Simulation remains disabled; local input state is not yet a validated Priest build envelope. The initial presentation-only preview exposed that configuration UI separation was incomplete even though runtime/search contracts were extracted.

## Shadow shader foundation

The user selected Shadow first and explicitly required the Warlock DES structure and fast/detailed variants. Both Priest variants are generated by `src/priest/kernel.js` from one combat implementation. Detailed mode adds full-width per-spell damage/cast/hit/crit/miss counters; fast mode omits those counters without changing combat decisions or RNG consumption.

Implemented bootstrap modules:

- `src/priest/model.js`: independent packed config/state layouts, strict resolved-core validation, texture packing and incomplete-result rejection.
- `src/priest/kernel.js`: bounded binary event heap, integer millisecond timestamps, cast impacts, GCD decisions, SW:P periodic damage, Mind Flay channel ticks, cooldowns and capped mana grants. Heap overflow and event-budget exhaustion have explicit failure statuses.
- `src/priest/rng.js`: independent copy of the existing shader splitmix64/xoshiro256** seed schedule. Candidate-local lanes share seeds in native batches; seed zero uses the generator's sentinel directly rather than the current Warlock runner's `0 -> 42` fallback.
- `src/priest/results.js`: common mean-DPS/sample-standard-deviation summaries, accounting and detailed spell breakdowns.
- `src/gpu/compact_des_engine.js`: class-bound Three.js packed DES transport, separate cached fast/detailed materials, async sequential attachment readback, chunked multi-config execution, cancellation and resource disposal. Warlock's current engine remains unchanged.
- `src/classes/priest_simulation.js`: standalone native simulation contract. It is not registered or wired to Priest UI actions yet.

Current bootstrap scope is the four spell implementations SW:P, Mind Flay, Mind Blast and SW:Death, using local CPU spell constants with resolved hit/crit/damage/cost/mana parameters. Isolated spell policies exercise fixtures; the provisional Shadow priority maintains SW:P, casts MB, uses SW:D in the final 20% of encounter time, then channels MF. It is not yet the full CPU default policy. Successful channels currently run all three ticks, with no clipping.

These resolved debug defaults are not Priest class defaults or the 14/0/37 preset. Raw UI builds/talents are rejected by the core validator rather than accepted with ignored effects. Shadowform/Weaving, talent-specific spell modifiers, Inner Focus, spirit/5SR regen, resistances, racial spells/Devouring Plague, buffs/consumables, health/backlash and custom APL execution remain unimplemented. SW:D time-based execute is provisional pending the prior CPU timing audit. Same-time events use explicit ordering and fight-end damage is inclusive; CPU queue tie/end behavior still needs equivalence fixtures. Detailed mode currently provides accounting, not Warlock-equivalent diagnostic traces/regret.

Validation: both fragment variants and vertex shader compile with `glslangValidator`. Actual headless EGL/GLES execution on llvmpipe passes analytic SW:P/MF/direct-crit/mana-starvation/miss fixtures, bit-for-bit fast/detailed common-state equality, multi-config lane mapping, batch offsets and event-budget rejection. Mocked transport tests cover candidate grouping across readback chunks, attachment read order, variant caching and cancellation/retry. This is shader execution evidence, not browser/Three.js integration, user-GPU throughput or CPU parity.

Next: implement and validate the selected preset's class configuration/talent modifiers, Shadowform/Weaving, mana/5SR and channel clipping; establish deterministic CPU fixtures and the shared diagnostic path; then register the capability-gated Priest runtime and connect the existing UI controls.

## WoW Forever talent implementation: first pass

The user reaffirmed WoW Forever as the target and authorized talent implementation. `src/priest/talent_data.js` records all 53 nodes from the local 1.60.1 data. `validateTalentAllocation` validates explicit ranks, 51-point cap, points in preceding tiers and prerequisite ranks across all three trees. No automatic rank repair occurs. `resolveShadowTalentBuild` accepts untalented resolved core settings and canonical tree allocations, emits the resolved shader config and an effect-disposition report for every allocated talent. It rejects allocated pending effects unless the caller explicitly lists those talents in a fixture-only deferred scope. Derived fields cannot be resubmitted as baseline inputs. This does not yet implement the raw-stat/UI/gear logical build adapter.

Implemented effects use Forever values from the repository talent descriptions and CPU implementation:

| Talent | Shadow core effect |
| --- | --- |
| Shadow Focus | +1% hit per point, capped at 100% |
| Darkness | +2% Shadow damage per point |
| Shadowform | +10% Shadow damage, 50% reduced Shadow spell costs, 2.0 total direct-spell crit multiplier |
| Twin Disciplines | +1% per point to SW:P/SW:D damage, excluding MB/MF |
| Mental Agility | 3/7/10% reduced instant-spell mana costs, excluding MB/MF |
| Improved SW:P | One extra 3-second tick per point |
| Improved Mind Blast | 0.5-second cooldown reduction per point |
| Mind Flay | Learned-talent gate for channel use |
| Improved Mind Flay | +10% channel damage per point |

Talent fixtures exposed a scheduling issue: idle decisions originally woke only at 2-second mana boundaries, which delayed the 5.5-second talented MB cooldown. Idle scheduling now also wakes at applicable cooldown, DoT-expiry and provisional execute transitions. Active-channel clipping remains a separate mechanic. Both shader variants share this decision scheduling.

Pending effects remain explicit: Inner Focus, Meditation/5SR, Mental Strength and Spiritual Guidance stat resolution, Shadow Weaving, Early Demise/target-health execute, Devouring Contagion/DP, Vampiric Embrace and Power Infusion. Healing/defense/threat/control/range/Holy/wand effects are outside the four-spell mean-DPS core; Spirit Tap has no in-fight kill/add trigger in the current encounter. No claim is made that all allocated 14/0/37 talents now have complete combat implementation.

Information requested from the user: confirm whether Forever SW:P and Mind Flay damage dynamically updates with Shadow Weaving, or snapshots at application/channel start (possibly different behavior per spell). The CPU snapshots their damage while its mechanics defaults declare dynamic behavior. Dynamic Weaving implementation is deferred pending that answer; static talent work proceeds independently. Local Forever spell records agree with the implemented SW:P/MB/MF/SW:D base values, so no additional base-spell information was needed for this pass.

## Runnable browser Shadow core and dynamic damage

The user confirmed that WoW Forever does not snapshot SW:P/Mind Flay and that their ticks update with Shadow Weaving stacks. The Priest shader now calculates periodic damage on each tick, applies the current Weaving multiplier before the damage, then rolls the rank's 33/67/100% proc chance. Stacks cap at five, grant 2% each, refresh for 15 seconds and expire at the exact boundary before subsequent damage. Both fast and detailed paths use the same logic/RNG. The CPU remains a snapshot-based implementation; this intentional correction requires adjusted CPU reference fixtures before claiming parity.

A new legal `Shadow core (20/0/31)` preset contains implemented damage talents and talents without an applicable effect in the four-spell policy. It includes Shadowform and 3/3 Weaving but excludes unfinished Inner Focus, Meditation, Early Demise and Devouring Contagion. Vampiric Embrace's allocated prerequisite rank is classified outside the damage-only policy because its activation/healing is not simulated; no healing output is advertised. The original three CPU presets are retained for inspection and rejected for execution while they allocate unfinished combat effects. No UI execution path uses the fixture-only `defer` escape hatch.

`src/priest/ui_config.js` translates Human level-60 direct stats against one level-63, zero-resistance target. It uses the local CPU 1456 base mana / 1.24% base crit / 59.2 Int per crit-percent values, plus Human Spirit and the CPU spirit-regeneration coefficient. The crit input is the supplied bonus, as in the existing Warlock form. MP5 contributes 40% per 2-second tick; spirit regeneration obeys an explicit five-second rule and current casting-regeneration fraction (zero for the core preset). Regen accounting records actual capped gains. These formulas still need Forever/browser/CPU validation beyond the scoped fixtures. Equipment, buffs, resistance, custom APL, health survival and additional Priest abilities remain outside this run scope.

The shared Run button now dispatches this validated build to the standalone Priest contract with a strict class-identity check. Detailed spell breakdown selects the existing fast/detailed variants. Progress, Stop/cancellation, mean DPS/confidence/range, spell rows, throughput and combat-stat summaries use the existing panels. Class switching and input changes are locked during a run; changing inputs or talents clears obsolete displayed results. Controls/results are retained across class switches. A fixed four-spell priority is displayed read-only in the existing APL box. No preparation banners or duplicate configuration panels are added. The core preset is selected initially; unfinished CPU presets disable Run with the reason in its tooltip.

The browser run is scoped and class-local: Priest is still not advertised in the global import/search registry. GA/APL search, build import/export, gear and broader presets remain disabled. This enables native shader inspection while those class contracts are completed.

Validation: dynamic-tick analytic fixtures, exact 15-second Weaving expiry, 5SR regeneration, and all prior shader fixtures execute successfully on headless llvmpipe with bit-for-bit fast/detailed common state. UI/controller checks cover supported vs unsupported presets, stat translation, correct class dispatch, mode/progress forwarding, result rendering, duplicate-click protection and cancellation during loading. Suite across all files: 107/109 pass, no skipped checks; only the two previously documented Warlock failures remain. Real browser/Three.js execution and user-GPU throughput still need verification.

## Meditation and Inner Focus

Meditation now resolves to the local Forever tooltip values 17/33/50% at ranks 1/2/3. The shader applies this fraction only to spirit regeneration while inside the five-second rule; MP5 stays fully active. It restores full spirit regeneration after the rule expires and continues to cap gains at maximum mana. The existing CPU uses fractions of 50% (1/6 and 1/3 at the first two ranks), so exact lower-rank parity requires updating the reference expectation.

Inner Focus is learned-talent gated. The current fixed policy activates it off the GCD, on cooldown, immediately before the next selected spell. The 180-second cooldown comes from the current CPU implementation. One spell receives zero mana cost; direct MB/SW:D impacts also receive +25 percentage points of crit chance, capped at 100%. SW:P and Mind Flay consume the effect without gaining periodic critical strikes. Misses consume it as well. Cast-time spells retain the focused-cast flag through their impact. A zero-cost cast does not restart the five-second rule. Detailed output appends an `innerFocusUses` counter; fast output omits that counter, and both modes share the same combat decisions.

This intentionally fixes two CPU-reference issues: Mind Flay does not clear `inner_focus_active`, and free cast completions call the 5SR tracker despite spending zero mana. Full shader/CPU parity should compare corrected reference behavior, not preserve these discrepancies. Configured APL/activation timing and damage-spell alignment remain later policy work; the current fixed policy can consume Inner Focus on the opening SW:P.

The runnable `Shadow core (20/0/31)` preset now allocates 1/1 Inner Focus and 3/3 Meditation, replacing Wand Specialization/Martyrdom points while keeping a legal 51-point build. No deferred effects are used for browser execution. Standard Shadow remains gated by Early Demise and Devouring Contagion; the latter requires Devouring Plague. Mental Strength, Spiritual Guidance, Power Infusion and broader activation/healing mechanics remain outstanding.

New actual GLES fixtures cover free MB/SW:D with guaranteed crit at 75% base chance, one-use SW:P/MF consumption, miss consumption, reuse at the 180-second boundary, and a later free MB not restarting an existing 5SR. Rank-specific Meditation fixtures produce the analytic mana totals; every added fixture checks bit-identical fast/detailed common state. Shader compilation and all core fixtures pass on llvmpipe. The headless test harness now reuses linked programs across fixtures, avoiding repeated compilation without changing simulation execution. Browser performance and appearance still require user-GPU checks. Cache v51 refreshes talent/model/shader/core-preset changes.

## Ten additional damage talents (October 2026)

The WebGL damage core now implements Mental Strength, Spiritual Guidance, Early Demise, Holy Precision, Holy Specialization, Divine Fury, Power in Light, Penance, Improved Healing, and Holy Nova. Mental Strength resolves baseline intellect into mana, crit and spirit regeneration; Spiritual Guidance uses the Forever 1/3/5/6/8% damage table. Their raw intellect/spirit inputs are mandatory when allocated. The UI supplies them and displays the resulting stats.

Holy damage has independent spell power, hit, crit, cost and damage multipliers. The new legal 26/25/0 Holy damage preset maintains Holy Fire, channels Penance on its 12-second cooldown, then casts Smite. Isolated policies also exercise Holy Nova, which remains a single-target damage spell here. Spell constants follow the local level-60 Forever/CPU definitions. Divine Fury preserves the original spell coefficients. Power in Light checks for an active Holy Fire on damage impact. Penance fires at 0/1/2 seconds and Improved Healing reduces its cost by 5% per rank. Healing effects and additional healing spells remain outside this damage runtime.

Early Demise adds 15/30% SW:D crit only during the existing final-20%-of-duration execute proxy. Actual target health remains unmodeled. Devouring Contagion and Power Infusion are pending; Searing Light is now explicitly pending because its Holy damage and free-Nova proc must not be silently omitted in a Holy rotation. Detailed/fast configurations share all combat logic; the expanded result schema includes eight spells and uses four compact stripes. Shader and headless GLES fixtures cover cast timing, spell gating, cost, school isolation, Holy Fire/Power in Light interaction and execute crit behavior.

## Remaining damage talents (October 2026)

Ten further talents now have damage-runtime implementations: Devouring Contagion, Power Infusion, Searing Light, Shadow Reach, Holy Reach, Wand Specialization, Shadow Affinity, Silent Resolve (Holy threat component), Improved Mana Burn, and Spirit Tap. This brings the damage-scope implemented count to 32 of 53. The 21 healing/defense/control talents and Silent Resolve's incoming-control-duration component still require an encounter model for those mechanics; they are not claimed complete.

Devouring Plague is enabled only for the Undead UI preset and can be enabled explicitly in a resolved fixture. It costs 985 base mana, has a 60-second cooldown and eight 3-second ticks over 24 seconds; damage updates dynamically with spell power, Weaving and Infusion. Contagion reduces cost 25/50% and spreads the remaining ticks/duration to a replacement within 5/10 yards. Optional `targetDeaths` and `targetDeathInterval` fixtures represent credited non-trivial kills with immediate replacement; `nextTargetDistance` checks the jump. Generation IDs prevent expired-target SW:P, Holy Fire and channel events from damaging replacements. These fixtures are a deterministic encounter input, not health-driven kills or simultaneous multi-target combat.

Spirit Tap rolls 20% per point on those credited deaths, doubles the supplied spirit-regeneration component for 15 seconds, allows at least 50% while in the five-second rule, and doubles the Spirit contribution from Spiritual Guidance dynamically. The boss-only browser encounter has no mid-fight kill trigger. Kill-trigger healing/party mechanics are not implied.

Power Infusion follows the current CPU off-GCD, no-cost self-use policy: 20% spell damage for 15 seconds, activated on a 180-second cooldown. Its expiration is exclusive at the 15-second boundary. It does not increase wand damage. Searing Light applies the Forever 2/5% Holy damage table and rolls 5/10% on Holy Fire periodic damage for one free Holy Nova. The flag persists until consumed, does not stack, ignores insufficient mana for that Nova, and does not restart the five-second rule when the resulting cost is zero. The Holy policy spends the proc when in range.

Range checks cover every damage policy. Shadow Reach adds 10/20% to offensive Shadow spell range; Improved Mind Flay first adds 5/10 yards to its 20-yard base. Holy Reach scales Smite/Holy Fire range and Holy Nova radius. `targetDistance` defaults to zero; the UI adapter also accepts an explicit target distance. Wand fixtures use supplied damage/interval and independent hit chance with the Forever 13/25% specialization table. Mana Burn uses a three-second base cast reduced by 0.5/1.0 seconds, drains up to the target's remaining mana and deals half the drain as Shadow damage. Threat is observed in detailed mode with 10% per point reductions for Shadow Affinity and Silent Resolve's Holy component; Holy Nova causes no threat. No threat cap, aggro or incoming control is modeled.

The original Standard/Deep Shadow and Smite presets are now executable without deferred talent effects; Smite selects the Holy policy. New legal presets expose Undead Shadow/Plague and 31/20/0 Power Infusion/Smite. Race display, Human Spirit, read-only policy text and cleared results follow preset changes. Detailed results include ability/proc/spread/threat counters; fast mode keeps identical combat decisions. GPU config/result scope is v3 and cache v53 refreshes the changed assets.

Validation for this checkpoint: shader compilation and actual headless GLES fixtures pass, with exact common-state agreement between fast/detailed variants across all new cases. Fixtures cover all Contagion ranks/radii, retained ticks across successive kills, an out-of-radius failed jump, Plague learning/hit gates, Infusion's 15-second boundary and 180-second reuse, free-Nova mana accounting, Spirit Tap's regen cap/dynamic spell power, range thresholds, 13/25% wand damage, school-specific threat, cast-time reduction and target-mana exhaustion. UI/allocation checks verify legality and execution of every preset, explicit Undead racial gating, Holy policy selection and all derived-field protections. Full test command reports 21/23 files passing; the same previously documented Warlock racial-accounting and regret fingerprint failures remain. No real-browser/user-GPU performance claim is made.

## Constrained Priest spec search (October 2026)

The user confirmed the intended encounter: one target, no mid-fight deaths/spreading, no incoming damage, and target health percentage declining linearly with encounter duration as in the Warlock sim. The existing final-20%-of-time SW:D/Early Demise behavior is therefore the intended encounter model. Kill/spread fixtures remain isolated tests; production spec-search configs keep target-death inputs at zero. Healing, defense and control mechanics are outside this DPS search rather than prerequisites for enabling it.

`priest/search.js` supplies class-owned rules to the existing shared genetic/MAP-Elites optimizer: all 53 talent ranks, 51-point legality, preceding-row gates, exact prerequisites, maximum-rank required talents, mutation/crossover repair, deduplication, diversity keys, candidate config conversion and result accounting. A deterministic feasibility pass rejects impossible requirements (such as both Shadowform and Power Infusion) before GPU evaluation. Candidate stats always begin with the unmodified direct inputs; each candidate applies its own race and talent modifiers once. Human and Undead are the current supported race choices; disabling race optimization locks the current race. No gear/buff/import/APL synthesis capabilities are implied.

Search compares fixed Shadow, Holy/Smite and mixed Holy/Shadow policies. Mixed uses racial Plague when available, a free Nova when available, Holy Fire, Penance, SW:P, MB, execute SW:D, MF, then Smite if no prior action is available. It uses the same cooldown/resource/target-health checks as the other policies. Search also supports joint spec/APL evolution, described below. The Warlock optimizer and its policy generation remain bound to their existing rules; native evaluation now accepts the explicit class module bound by the build optimizer so Priest batching can use the same finite-completed-result validation without enabling unfinished global Priest import contracts.

The constrained-search tab reuses the existing dashboard markup with separate `priest-search-*` IDs and event handlers. It offers required-talent/race/policy locks, population/generation/screening/final-precision controls, preset seeding and mutation rate; Warlock pet/sacrifice controls are removed; APL strategy and condition locks are supported. The table shows Disc/Holy/Shadow counts, race, policy, DPS and observed uncertainty, with live best/mean convergence curves. Finalists get the existing higher-iteration rebenchmark and are reranked. Stop preserves the latest completed generation's candidates; duplicate starts, simultaneous Priest runs and class switching during search are blocked. Apply installs the exact allocation/race/policy as a runnable current build, clears old run results, and selects the current-build tab. Class switches preserve each class's controls/results. Direct-stat or constraint changes clear stale Priest search results.

Validation: property fixtures generate/cross/mutate legal allocations under several deep-tree constraints; full search tests exercise batched generations, finalist reranking, replay, invalid score rejection, stop preservation and exact candidate-to-build conversion. An actual headless GLES optimizer run executes all three batches on the Priest fast shader and verifies the winning spec's packed config survives Apply unchanged. UI harness checks dispatch, rendering, required Shadowform, mixed/Undead Apply, retained results after class switching, and cancellation during lazy runtime loading. All Priest/search/adapter tests pass; full suite is 22/24 files passing, with only the same known Warlock racial-accounting and frozen regret-shader-hash failures. Cache v54 includes the search modules. Real browser/user-GPU appearance and large-search throughput have not been measured here.


### Single-target policy corrections (October 2026)

Priest policies now share one rule list between the GPU selector and the displayed APL. Shadow Word: Death is usable whenever ready; the execute threshold only enables its talent crit bonus. Composite policies clip Mind Flay after its second tick when an affordable higher-priority action is ready. Channel tokens invalidate both the canceled third tick and its old ready event.

Composite policies require at least two remaining Shadow Word: Pain ticks, three Devouring Plague ticks, and two Holy Fire periodic ticks after impact. Casts must finish within the encounter. These are policy heuristics; spell coefficients are unchanged. Mana pooling projects known regeneration over at most six seconds, reserves for higher-priority spells due within three seconds, and skips reserves less efficient than the available filler. Impossible reserves and zero-regeneration states cannot create indefinite waits. Free Holy Nova precedes Holy Fire in both the displayed and executed Holy policy.

Resolved scope is `priest-damage-core-v4`; optional resolved flags `clipMindFlayEnabled` and `manaPoolingEnabled` default on for controlled comparisons. Detailed results report channel clips, pooling waits and time; fast and detailed damage/mana results remain identical. The encounter remains single target with duration-based target health and no production death or incoming-damage model.

Validation: all Priest test files pass, including actual headless GLES fast/detailed parity for clipping, pre-execute SW:D, DoT thresholds, impossible reserves, free Inner Focus and mixed schools. The bounded pooling fixture refreshes Pain twice instead of once and increases damage. Full suite: 22/24 files pass; existing Warlock racial scaling and frozen shader hash failures remain.

Priest joint-evolution update: constrained search now defaults to Co-Evolve APL with Spec. Priority order crosses over and mutates with talents/race/policy; unlocking conditions also evolves health ceilings, mana floors and Never Use. The selected Shadow/Holy/mixed policy defines the action set, with GPU cooldown, talent, race, resource and DoT eligibility checks retained. Static mode retains the original templates. Per-candidate packed policies survive elite retention, finalist evaluation and Apply, and appear in candidate details/current-build policy text. Standalone Priest APL synthesis remains gated.

October 7 talent/DPS audit: all 53 local talent nodes now have exact prerequisite-rank and row comparisons. The missing Holy Nova spell-owned 5% proc from Holy Fire is corrected on direct damage and added to Searing Light's periodic bonus, with learned-Nova gating. See `PRIEST_TALENT_AUDIT.md` for all talent dispositions, pinned/current spell-data differences and controlled DPS/mana measurements; `validation/priest_dps_audit.mjs` reproduces the GPU fixtures. The correction is about 4–6 DPS in the audited Holy builds and leaves Shadow unchanged. At the default direct-stat inputs, Shadow's mana limitation costs roughly 141–175 DPS relative to an unlimited-mana diagnostic. Priest gear/buff/consumable and additional racial support remain absent; the user's exact comparison is needed to establish the cause of their reported gap. Cache v58 includes the shader correction.
