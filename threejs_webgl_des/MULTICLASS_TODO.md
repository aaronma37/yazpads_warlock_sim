# Three.js WebGL: Support for Additional Character Classes

Status: Warlock class foundation and shared UI implemented; runnable scoped Shadow Priest includes dynamic damage, Meditation and Inner Focus. Full Shadow preset, registry integration and CPU parity remain pending. Work resumed on 2026-10-07 after the user reported that performance looks good.

## Resume here

The shared configuration layout supports a runnable Priest Shadow core (20/0/31), with class-owned fast/detailed DES shaders, dynamic Shadow Weaving, Meditation and Inner Focus. Priest dispatch is scoped to the browser core path; global import/search registration remains unavailable. The standard 14/0/37 build still needs Devouring Plague/Devouring Contagion and health-based Early Demise. Next prioritize those effects, channel clipping and resistance/CPU validation. Existing Warlock GPU sources and hot loops remain unchanged.

Performance preservation is a primary requirement. Continue in small, separately reviewable steps. The user already has a performance baseline; use it for comparison before further refactoring.

### Performance checkpoint

- [ ] Locate the user's existing baseline and record its workloads, hardware/browser, settings, and acceptable regression threshold here.
- [ ] Compare representative fast and detailed runs using the same builds, seeds, iterations, and batch sizes as the baseline.
- [ ] Compare a representative genetic algorithm workload, including candidate throughput and total search time.
- [ ] Record shader compile, packing, execution, and readback timings where available; distinguish initial startup from warmed runs.
- [ ] Compare simulation output and confirm the existing layout, startup, and offline loading still behave correctly in the browser.
- [ ] Investigate any regression before making another simulation or search refactor.

Latest validation: syntax/diff checks and direct-reference contract checks passed. The existing Node suite passed 23 of 25 tests. The two known failures are `Command applies once to every pet attack and Gnome gains maximum mana` and `regret diagnostics do not change simulation shaders`; both also reproduced before these refactors. Browser appearance and throughput have not been measured for these changes. These checks do not establish full CPU parity.

The user accepted current performance on 2026-10-07 and authorized the next extraction. Baseline workload/hardware/settings and browser correctness details above remain unrecorded; this acceptance is not a measured benchmark report.

### Completed: incremental Warlock search-rule extraction

- [x] Inventory class assumptions in `src/genetic_optimizer.js` and `src/apl_genetic_optimizer.js`: legal actions, talent constraints, mutation ranges, initialization, crossover, and repair rules.
- [x] Extract talent definitions and the unchanged `TalentGraph` behind `registry.search.talents` in `src/classes/warlock_search.js`. Keep existing optimizer exports compatible.
- [x] Resolve talent rules once at optimizer module startup using direct references; retain candidate representation, seed schedule, and evaluation loop.
- [x] Check generated/crossed/repaired talent legality and exact fixed-seed parity against the pre-extraction implementation (1,000 candidates and final RNG state match).
- [x] User confirmed browser search throughput remains good after the talent extraction (2026-10-07).
- [x] Extract pet choice metadata, forced pet/sacrifice requirements, and required-talent repair into `registry.search.pets`; preserve direct references and existing exports.
- [x] Compare 4,500 pet/talent repairs with the pre-extraction implementation; candidates and final RNG state match exactly. Test all forced pet modes and sacrifice/pact prerequisites.
- [ ] Record browser search throughput after the pet, race/rotation, policy APL, synthesis metadata, encoding, initialization/repair, crossover/mutation, build-operation, candidate-conversion, identity, preset-seeding, and result extractions. The user authorized continuing with race/rotation extraction; no new throughput measurement was supplied.
- [x] Extract Warlock race/rotation choices, labels, and locked-choice enforcement into `registry.search.choices`; preserve ordering and existing exports.
- [x] Compare 7,000 static-APL mutation/enforcement cases against the pre-extraction implementation; candidates and final RNG state match exactly.
- [x] Extract handcrafted Warlock policy APL construction into `src/classes/warlock_policy_apl.js` through `registry.search.policyAPL.build`; retain the existing optimizer candidate API and talent resolution.
- [x] Compare all 5,120 rotation/talent-flag combinations (including rotation fallback cases); exact shader rotation, action ordering, and bytecode rules match the pre-extraction implementation.
- [x] Extract synthesized APL action metadata, spell IDs, condition types/ranges, and talent-gated action/condition eligibility into `src/classes/warlock_apl_search.js` through `registry.search.apl`; preserve direct references and public exports.
- [x] Compare metadata and eligibility for all 256 relevant talent-flag combinations plus missing/null flags; 516 generated APLs, bytecode outputs, and final RNG states match exactly with locked and unlocked conditions.
- [x] Extract condition creation, two-condition rule creation, handcrafted per-action rules, and individual-to-bytecode encoding through `registry.search.apl`; preserve public optimizer exports and legacy single-condition compatibility.
- [x] Compare 12,474 two-condition cases and every handcrafted action against the original, plus 516 generated APLs/bytecode/RNG states; all match exactly.
- [x] Extract APL default/random initialization, random condition helpers, and action repair through `registry.search.apl`; preserve public exports, ordering, and RNG calls.
- [x] Compare 256 defaults and 512 initialization/repair/crossover/mutation sequences against the pre-extraction implementation; candidates, bytecode, and final RNG states match exactly.
- [x] Extract synthesized APL crossover and mutation through `registry.search.apl`; preserve public exports, RNG order, candidate representation, and evaluation loops.
- [x] Compare 512 initialization/repair/crossover/mutation sequences against the pre-extraction implementation; exact candidate, bytecode, and RNG parity. Test 2,000 mutation steps for eligible permutations/conditions, locked rules, and unchanged parents, plus small-population behavior.
- [x] Extract build initialization, full constraint enforcement, talent/APL crossover, and mutation into `src/classes/warlock_build_search.js`. Bind the talent resolver once through `registry.search.createBuildSearch`, avoiding a talent-UI/registry dependency cycle; retain public exports and evaluation loops.
- [x] Compare 3,200 generation/crossover/mutation sequences using the real talent resolver across static/evolved APLs, condition locks, pet modes, race/rotation locks, explicit talent requirements, and missing-parent APL branches; exact candidate and RNG parity.
- [x] Extract build/APL candidate-to-config conversion and build-policy resolution into `src/classes/warlock_candidate_config.js` via `registry.search.createCandidateConfig`; bind dependencies once and retain public exports.
- [x] Compare 1,200 build and 1,000 APL configurations against the original; exact configuration parity across race baselines, candidate races, policies, custom APLs, disabled actions, and channel fallbacks.
- [x] Audit remaining class assumptions in diversity/deduplication keys, build/result labels, and preset search seeding.
- [x] Extract build names and build/APL diversity/deduplication keys into `src/classes/warlock_search_identity.js` through `registry.search.identity`; retain public exports and exact key semantics. Compare 7,200 candidate cases with the original.
- [x] Extract preset-to-build-candidate conversion into `src/classes/warlock_search_presets.js` through `registry.search.createPresetCandidate`; bind talent/pet resolution and enforcement once, retain optimizer ordering, deduplication, half-population limit, and RNG schedule.
- [x] Compare 1,464 preset conversions using repository presets and inference edge cases; exact candidate/RNG parity across APL modes, condition locks, and forced selections. APL-only default seeding already uses the class-owned initializer.
- [x] Extract build/APL result payload construction, Warlock classification, and APL naming into `src/classes/warlock_search_results.js` through `registry.search.results`; retain public exports and payload/reference semantics.
- [x] Compare 1,000 build and 1,000 APL results/names with the original; exact payload parity.
- [x] Define v1 logical candidate/evaluation/result shapes and legacy Warlock migration in [MULTICLASS_CONTRACTS.md](MULTICLASS_CONTRACTS.md). Design only; runtime adapters and migration are not implemented.
- [ ] Pure saved-envelope identity/version validation completed in `src/contracts/saved_build.js`; next is staged legacy logical/resolved migration. Preserve current transport and runtime formats until integration is verified.
- [ ] Recheck browser correctness and throughput for the completed search-rule extractions before broader contract integration.

Search-rule inventory:

- **Talent rules (extracted):** 52-node ordering, rank caps, prerequisites, row gates, 51-point budget, legal donors/receivers, generation, repair, and object/vector conversion. Both optimizers share the registry-owned graph; existing exports remain available.
- **Pet rules (extracted):** pet choice lists, forced pet/sacrifice requirements, required-talent repair (including explicit user requirements), and talent-dependent active-pet/sacrifice enforcement. Mutation donor protection retains its existing behavior; forced requirements are restored by enforcement afterward.
- **Race/rotation choices (extracted):** race and rotation lists, rotation labels, and locked-choice enforcement. Resolved once at optimizer startup; unlocked-choice and ALL behavior remain unchanged.
- **Policy APL (extracted):** handcrafted rotation policies, action ordering, talent gates, and bytecode rules. The class builder takes rotation and resolved talent flags; the optimizer retains candidate-to-talent conversion. No UI, engine, or shader dependencies are introduced by the policy builder.
- **Build operations (extracted):** initialization, constraint orchestration, talent crossover/repair, race/rotation/pet inheritance and mutation, and APL generation/crossover/mutation orchestration. Real talent conversion is injected once at optimizer startup. Existing mutation donor/receiver behavior is unchanged.
- **Candidate conversion (extracted):** build and APL candidate-to-config conversion, race baseline normalization, talent effects, pet/sacrifice resolution, first-enabled-nuke rotation inference, policy fallback, and bytecode forwarding. The two existing channel fallback behaviors remain distinct.
- **Search identity (extracted):** build names and build/APL diversity/deduplication keys, including talent tree bins, pet/sacrifice bins, active filler priority, and tap position. Existing key precision and omitted fields are unchanged; these keys are not a new universal cache contract.
- **Preset seeding (extracted):** rotation/name inference, talent-vector conversion, race/pet/sacrifice selection, talent-gated default APL initialization, and enforcement before optimizer deduplication. The existing preset pet/sacrifice resolver is injected once; population ordering and limits remain shared.
- **Search results (extracted):** build categories, APL names/tags, damage shares, and existing result payload construction. The two existing payload shapes remain distinct pending common result-contract design.
- **Build search integration (remaining):** common candidate/result boundaries and runtime class selection; talent flag and preset pet/sacrifice resolution still use existing resolvers bound at startup.
- **APL metadata/eligibility (extracted):** action/spell IDs, talent-gated legal actions, valid condition lists, and parameter ranges. The synthesis optimizer resolves registry-owned references once at startup and preserves its public exports.
- **APL encoding/handcrafted rules (extracted):** condition parameter cleanup, condition text and target IDs, two-condition combination, per-action handcrafted rules, and bytecode conversion including legacy rules. Existing clamps and fallback behavior are preserved.
- **APL initialization/repair (extracted):** default action order, random action permutation, random condition sampling, duplicate/unavailable action removal, and missing-action insertion. Unlocked repair preserves existing rule objects/conditions; locked repair reinstates handcrafted rules.
- **APL crossover/mutation (extracted):** permutation crossover, condition type mutation, parameter jitter, action swap/insertion, and NEVER toggles. The optimizer uses direct class-owned references; population selection, evaluation, and seed scheduling remain unchanged.
- **APL search integration (remaining):** common candidate/result boundaries and runtime class selection. Default APL seeding uses the class-owned initializer.
- **Preset/result audit:** build preset seeding infers Warlock rotations from names and explicit rotation strings, resolves pet/sacrifice via presets, converts three talent trees, and initializes talent-gated APLs before enforcement/deduplication. Build result classification assumes three Warlock trees and pet/school output; APL names map Warlock action IDs to tags and fillers. Preset ordering, population limits, and selection/evaluation mechanics stay shared.
- **Shared search mechanics to preserve:** population/elite selection, RNG call order and seed schedule, candidate identity, evaluation batching, progress/cancellation, and fitness/result mapping. The current search contract is deliberately partial; it does not yet enable another class.

Extraction validation: all twenty-four search-rule tests pass, including legal donor/receiver moves, malformed-rank repair, forced pet prerequisites, unforced pet repair, locked race/rotation choices, resource/filler safeguards, talent-gated policy priority, synthesis action eligibility, proc-condition eligibility, clause encoding, legacy bytecode conversion, handcrafted action safeguards, initialization, action repair, crossover/mutation legality, small-population behavior, build operations with forced selections/parent preservation, race normalization, candidate conversion fallbacks, search identity, diversity fallback behavior, preset inference/enforcement, build result accounting, and APL result naming. The full Node suite including five saved-envelope, four legacy-mapping, and three Warlock normalization tests passes 85/87 tests with the same two previously recorded failures. Syntax and diff checks pass. The user confirmed throughput after the talent extraction; throughput/offline loading have not been remeasured after the pet, race/rotation, policy APL, synthesis metadata, encoding, initialization/repair, crossover/mutation, build-operation, candidate-conversion, identity, preset-seeding, and result extractions. The service worker caches the search module and advances to v32 and caches the policy and synthesis metadata modules.

Defer packed-format changes, generic shader state, additional classes, and broad candidate conversion until these checkpoints are complete. The user selected Priest as the first additional class. Initial specialization/mechanics scope remains pending; see [PRIEST_PREPARATION.md](PRIEST_PREPARATION.md).

### Next: integrate versioned boundaries incrementally

Envelope validation checks identity, version, kind, and required identity containers only. Class payload correctness and resolved packing/simulation version compatibility remain later adapter responsibilities. No browser import/export path or service-worker cache changes are needed for this standalone module yet.

- [x] Audit existing UI build strings, resolved-config imports, simulation exports, and seed/completion handling before defining the contracts.
- [x] Document candidate identity, class-scoped APL payloads, common fitness status, and failure/null-objective behavior.
- [x] Document separate logical/resolved saved envelopes and migration without double-applying talent, race, gear, or buff effects.
- [ ] Complete the browser checkpoint before broad runtime integration.
- [x] Implement/test pure envelope identity/version validation in `src/contracts/saved_build.js`; reject unknown versions/classes/kinds, missing versioned identities, and class mismatches. Preserve source payloads, including frozen input. Not yet connected to UI import/export.
- [x] Implement pure legacy classification and staged mapping in `src/contracts/legacy_build.js`; preserve resolved precedence, copy source inputs, separate UI encounter/run settings, and reject malformed containers/unknown formats. Four tests cover JSON round trips, inference precedence, and immutable inputs.
- [x] Implement initial class-owned import normalization in `src/classes/warlock_import.js`, deferred via `registry.loadImports`: direct-stat defaults, stat/encounter validation, talent caps/prerequisites/point budget, race/pet/sacrifice legality, and text/policy APL validation. Resolved inputs use existing validation directly; no effects are reapplied. Three tests cover immutable/idempotent normalization and invalid explicit inputs.
- [x] Add pure equipped-item resolution in `src/classes/warlock_equipment.js`: validate IDs/slots, reuse current enchant/set arithmetic, ignore saved equipped-stat snapshots, and require supplied item data. All four repository gear presets match existing arithmetic.
- [x] Validate saved buff keys against the actual combat-form fields and require boolean selections. Three additional tests cover equipment normalization/idempotence, invalid gear, and buff keys.
- [x] Resolve combat-form buffs/racial stats in `src/classes/warlock_import_buffs.js`; 500 combinations match current app arithmetic with sacrifice reserved for the class builder.
- [x] Validate rules-form APL IDs/conditions and produce simulation configs through `resolveWarlockImport`, deferred via `registry.loadImportResolver`; text/policy/rules inputs use existing compilers/builders, resolved imports bypass effects. Three tests cover effect ordering, idempotence, and final resolution.
- [x] Add pure decoded build I/O in `src/contracts/build_io.js`, dispatching through the registry import resolver. Verify legacy/v1 logical JSON/base64 round trips for direct/equipped, text/policy/rules inputs with exact config and packed-word equality; verify legacy resolved round trips without reconstruction/effect duplication.
- [x] Fix numeric-action injection in rules-form APLs by rebuilding canonical rules from names/conditions. Reject empty/multiline conditions and malformed resolved bytecode IDs/parameters/flags. Four new tests pass.
- [x] Register conservative packing/simulation source fingerprints in `src/classes/warlock_versions.js`; enable exact-match versioned resolved imports/exports through pure build I/O. Three tests check source fingerprints, packed-word round trips, and mismatch rejection. Legacy resolved formats remain supported.
- [x] User reported browser appearance looks good before UI import/export integration; throughput remains unconfirmed for recent changes.
- [x] Integrate async staged import/export into `src/app.js`: validate before UI setters, export v1 logical/resolved builds, await import before closing the modal, and preserve legacy JSON/base64 decoding. Four tests exercise the actual app import function with instrumented UI setters.
- [x] User confirmed integrated browser import/export looks fine. This is user-reported flow validation, not automated offline coverage or a throughput measurement.
- [x] Add standalone common evaluation/request/result wrapper in `src/contracts/evaluation.js`: unique identities, fixed-class validation, class resolution outside native batching, existing seeds/settings, finite completed fitness, and audited standard error from native sample sd. Four tests verify native settings/order/fitness, invalid requests, incomplete rejection, and runner cancellation.
- [x] Connect APL finalist evaluation through `src/contracts/native_evaluation.js`, bound once at startup. Pass exact config/options references to the native runner, preserve native result payloads, and read common finite fitness only after completion checks. Screening/generation evaluation remains direct.
- [x] Verify exact config/options forwarding, seeds/packed words/order, runner errors, and incomplete-result rejection. Instrumented actual APL search produces identical submitted batches/final output for locked/unlocked conditions, with exactly one adapter call per search.
- [x] User reported APL finalist-adapter throughput looks good; record as user-reported performance acceptance, not an automated GPU comparison.
- [x] Adapt build-search finalist evaluation through the same native adapter; preserve exact native batches/results, and read fitness only from completed common results.
- [x] Instrumented actual build searches (static APL, evolved locked/unlocked conditions) match submitted batches, rankings, and full final output with exactly one finalist adapter call.
- [x] User reported build-finalist throughput looks good.
- [x] Route APL initial screening and generation batches through the shared native adapter; use completed common objective values for fitness while retaining native batch payloads and double buffering.
- [x] Instrumented APL searches across two generations preserve submitted batches and complete output for locked/unlocked conditions; initial, generation, and finalist stages all use the boundary.
- [x] Migrate build initial screening and generation batches through the shared native adapter; read completed common fitness while retaining native payloads, seed schedules, and double buffering.
- [x] Instrumented two-generation build searches preserve batches, rankings, and final outputs for static/evolved locked/unlocked APLs.
- [x] User confirmed throughput remains good after all-stage evaluation integration. Automated GPU/offline validation remains pending.
- [x] Add versioned class capabilities (features, modes, objectives, seed schedules) and cached per-class simulation resolution in `src/classes/capabilities.js` / `runtime.js`. Gate common evaluation/imports and resolve startup simulation through the runtime. Three tests cover per-class concurrent caching, retries, identity mismatch, and unsupported capabilities.
- [x] Replace optimizer direct engine dependencies with class-bound implementations in `src/search/`; public search wrappers accept explicit `classId`, capability-check, and resolve simulation/search dependencies once per search. Preserve startup Warlock helper exports and native loops.
- [x] Verify orchestration parity after factory binding, independent class-rule/runner instances, and rejection of unsupported explicit class identities. Both public facades load; two binding tests pass.
- [ ] Next: audit and specify the remaining UI/APL/gear/buff/result class assumptions and stable class-switch state rules before adding a class selector.
- [ ] Audit class-specific UI/APL/gear/buff/result assumptions before runtime class selection and a second shader.
- [ ] Confirm throughput and offline behavior; run a real GPU wrapper-vs-native comparison before broader evaluation integration.
- [x] Implement/test legacy format classification and immutable staged mapping, including JSON round trips and malformed containers. Full class normalization and export/import round trips remain pending.
- [ ] Introduce common evaluation/result wrappers outside hot loops and verify effective seeds, packed words, candidate ordering, and fitness.

Current limits: the optimizer still selects Warlock at startup, current saved/exported formats have no integrated class/schema migration; a standalone pure envelope validator is implemented, and current result shapes remain distinct. The multi-config runner uses the first config's seed with a zero-seed fallback; preserve this behavior until a separately verified seed contract is integrated. The new v1 design does not claim runtime support or a completed generic search descriptor.

## Goal

Prepare the Three.js WebGL2 app to simulate character classes beyond Warlock through explicit class modules. Each class should own its spells, talents, resources, rotation rules, presets, and presentation data while sharing the browser UI, GPU execution infrastructure, and results tools where their behavior is common.

Keep existing Warlock builds working throughout the migration. Demonstrate the architecture with one additional class before expanding further. The user selected Priest as the first additional class. The local CPU/data audit is recorded in [PRIEST_PREPARATION.md](PRIEST_PREPARATION.md); Shadow DPS is the recommended initial specialization, pending user preference. Scope-specific data/CPU fixtures are required before implementation.

## Current starting point

- `src/model.js` defines Warlock spells, CPU spell IDs, APL actions, and default configuration.
- `src/config_builder.js` translates Warlock talents, pets, and sacrifice choices into simulation configuration.
- `src/kernel.js` contains the GLSL simulation mechanics; a class selector alone cannot provide another class's behavior.
- `src/talents.js`, `src/apl.js`, `src/gear.js`, `src/buffs.js`, `src/presets.js`, and `src/app.js` need an audit for assumptions about Warlock data and mechanics.
- The existing [roadmap](ROADMAP_TODO.md) records unresolved Warlock integration and parity work. Record an explicit baseline rather than assuming all current mechanics have CPU parity.

## Intended design

Use a class registry with a stable class ID and an explicit module contract. A class module supplies supported specializations, defaults and validation, spell/action metadata, talent data and effects, legal races and gear, resource rules, presets, configuration packing, and its simulation implementation.

Share browser GPU batching, progress/cancellation, and result presentation where their semantics match. Keep class mechanics explicit rather than adding unrelated class flags to the Warlock configuration. Each class owns a completely separate simulation shader implementation, following a common style and execution contract.

Preserve the roadmap's table-driven APL bytecode requirement: different APLs must remain executable per GPU lane without recompiling a shader for every rotation. Compile and cache kernels by class and mode. Comparisons batch by class and combine results in original candidate order, so the shared runner can evaluate a population containing multiple classes through separate draws.

### Stable layout across classes

Keep the same application shell, panel locations, navigation, and responsive layout when switching classes. Talent trees, presets, and APLs change their contents in their existing locations. Gear, stats, buffs, simulation settings, optimization controls, and results retain familiar positions and interaction patterns.

Class-specific controls fit into designated areas inside the existing panels. Define how those areas behave when a class has no corresponding feature, such as pets, so switching classes does not unexpectedly rearrange the main layout. Labels, icons, talent backgrounds, resources, and applicable stat fields can change with class metadata.

### Separate shaders with a common style

Every class supplies two shader variants:

- **Slow / detailed:** full supported accounting and diagnostic output for inspecting builds, explaining results, and validating mechanics.
- **Fast:** compact fitness output for large simulation batches and genetic algorithm candidate evaluation.

Both variants must simulate the same combat mechanics, event ordering, APL decisions, resource rules, and RNG sequence for the same inputs and seed. Fast mode reduces instrumentation and readback; it must preserve the fitness objective within defined numerical tolerances. Detailed accounting must not consume randomness or alter combat decisions.

Within each class, generate both variants from one class-owned implementation with instrumentation switches, as the current Warlock `buildFragment(detailed)` does. Across classes, use consistent naming, source organization, clock units, RNG conventions, event-loop structure, APL VM conventions, completion/error handling, and output conventions. Mechanics and simulation state remain owned by each class. Document differences required by a class's mechanics.

### Common input shape for genetic algorithms

The v1 design in [MULTICLASS_CONTRACTS.md](MULTICLASS_CONTRACTS.md) specifies `schemaVersion`, `classId`, `candidateId`, `encounter`, `stats`, `equipment`, `buffs`, `talents`, `apl`, and `classOptions`. Seeds, iterations, objective, and output mode belong to a common evaluation request. These shapes and migration rules are documented but not yet implemented.

Keep the outer structure and evaluation API consistent while letting each class define valid stat keys, talent trees, action IDs, predicates, and class options. A class validates and resolves a candidate into its own packed GPU configuration. Packed layouts may differ by class; the common contract is at the candidate and runner boundaries.

Each class also supplies a search-space descriptor: mutable genes, types/ranges, legal actions and conditions, talent prerequisites and point budgets, and validation/repair rules. The shared genetic algorithm uses those descriptors for initialization, mutation, crossover, and candidate validation. Initial searches keep `classId` fixed; class changes require an explicit search policy because talents and actions have different meanings.

Return a common fitness result containing candidate identity, class identity, objective value, sample count, validity/completion status, and uncertainty where available. Preserve candidate-to-result mapping across class batches and use a consistent encounter, iteration count, seed schedule, and objective for comparable evaluations. Invalid or incomplete simulations must not become successful fitness scores.

Evaluate populations in fast mode, then reevaluate selected finalists in detailed mode. Cache keys include class identity, schema/packing version, shader version, resolved candidate, and evaluation settings to prevent reuse of results from another class or contract.

## TODO

### Incremental preparation: presentation metadata

- [x] Extract Warlock labels, talent/preset data URLs, talent backgrounds, and existing race/pet/spell icon maps into `src/classes/warlock_presentation.js`.
- [x] Consume that metadata from the existing application shell, talent view, and preset view while retaining panel locations and current data.
- [x] Include the module in the service worker cache and advance the cache version.

This extraction leaves simulation configuration, shader source, packing, GPU execution, APL rules, and genetic algorithm implementation unchanged. Individual talent icons/descriptions remain in `data/talents.json`; the module owns the data reference. Static HTML retains initial Warlock labels as startup fallback. Runtime class switching and the complete class module contract remain future steps.

### Incremental preparation: presentation registry

- [x] Register Warlock in `src/classes/registry.js` with a stable ID and presentation reference; reject unsupported class IDs.
- [x] Resolve the default class once in `src/classes/active_class.js` and share that presentation reference across the app, talent view, and preset view.
- [x] Cache the registry and selection modules for offline loading.

The registry resolves presentation and provides deferred simulation loading. It adds no lookup to simulation or candidate evaluation loops. Runtime class switching and additional classes remain future steps.

### Incremental preparation: Warlock simulation contract

- [x] Expose direct references to the existing config builder, validator, packers, full-state decoder, shader sources, layout metadata, runner, summary, and diagnostic/lifecycle functions in `src/classes/warlock_simulation.js`.
- [x] Resolve that contract once through `src/classes/active_simulation.js`; use its function references in the application and preset view.
- [x] Cache the simulation contract, selection module, and config builder for offline loading.

The existing engine retains its compact/fast decoders, GPU resource ownership, and batching. Shader generation, packed configuration, and function implementations are unchanged. Genetic algorithms retain their current direct engine imports until the search-space extraction. Presentation-only consumers do not load the simulation contract or Three.js. Browser throughput still needs comparison against the user's performance baseline before further simulation refactoring.

### 1. Define scope and capture the baseline

- [x] Choose Priest as the first additional class (user request).
- [ ] Finalize specialization, level/ruleset, and initial supported mechanics; recommend level-60 WoW Forever Shadow DPS from the CPU implementation.
- [ ] Audit available spell, talent, racial, gear, and CPU reference data for that scope; record gaps and authoritative sources.
- [ ] Inventory Warlock assumptions across UI, configuration, shaders, APL tools, optimization, results, and saved builds.
- [ ] Capture representative Warlock configurations and current simulation output, including known parity failures.
- [ ] Define acceptance tolerances for new-class CPU comparisons and Warlock regression checks.

### 2. Establish class modules and configuration ownership

- [ ] Define and document the class module contract and class registry.
- [x] Document v1 common candidate/evaluation/result shapes and legacy migration (design only).
- [ ] Define class-specific declarative search-space descriptors and integrate the common boundaries.
- [ ] Move existing Warlock definitions behind a Warlock module incrementally.
- [ ] Add class ID and schema version to saved builds, presets, and exported results.
- [ ] Treat legacy builds without a class ID as Warlock; document migration and reject unsupported versions or classes clearly.
- [ ] Separate common encounter settings from class stats, resources, and mechanics.
- [ ] Namespace action/spell IDs and CPU mappings so classes cannot collide or interpret each other's actions.
- [ ] Validate each configuration through its class module before packing GPU data.

### 3. Make the interface class aware

- [ ] Add class selection and load the selected class's defaults, talents, spells, presets, and assets.
- [ ] Preserve panel positions and responsive behavior across class switches; replace talent, preset, and APL contents in place.
- [ ] Define reserved areas for class-specific controls and behavior when those controls do not apply.
- [ ] Render stats, resources, damage schools, and class controls from metadata, including optional pet controls.
- [ ] Filter legal races, gear, buffs, and consumables using explicit restrictions.
- [ ] Define class-switch behavior for unsaved edits and reset incompatible choices.
- [ ] Route APL editing, fallback analysis, synthesis, and optimization through the selected class's supported actions and predicates.
- [ ] Include class identity in comparisons, result labels, combat logs, and exports; clear stale results when the build changes.

### 4. Generalize GPU execution and result contracts

- [ ] Specify class dispatch, packed configuration layout/versioning, and shader cache ownership.
- [ ] Provide separate class-owned shader implementations, each generating slow/detailed and fast variants.
- [ ] Document a common shader style and execution contract, including clock, RNG, APL, and completion/error conventions.
- [ ] Separate reusable engine infrastructure from Warlock event and mechanic code.
- [ ] Group candidates by class for GPU execution and restore original candidate/result order.
- [ ] Adapt the shared genetic algorithm to class search-space descriptors and the common fitness result contract.
- [ ] Preserve per-lane APL tables, deterministic seeds, and class-specific resource fallback behavior.
- [ ] Replace fixed Warlock spell/result assumptions with class-provided mappings for damage, casts, hits, crits, misses, and ticks.
- [ ] Make timelines and diagnostics understand class actions, resources, procs, and pets when supported.
- [ ] Define which analysis and optimization features each class supports; reject unavailable features explicitly.

### 5. Implement one additional class end to end

- [ ] Add audited class data, assets, defaults, and at least one usable preset.
- [ ] Implement the scoped spells, talents, resource mechanics, cooldowns, procs, and APL behavior in the GPU simulator.
- [ ] Add or connect a CPU reference for the supported mechanics; identify any mechanic without an oracle.
- [ ] Exercise the full flow: choose class, configure build, edit APL, simulate, inspect results, compare presets, and save/reload.

### 6. Validate and document readiness

- [ ] Check legacy Warlock build loading and representative Warlock simulations against the captured baseline.
- [ ] Compare the new class against its CPU reference using fixed seeds, event timing, resource accounting, and per-spell output.
- [ ] Compare fast and detailed variants for each class using identical configurations and seeds; verify fitness and completion status agree within documented tolerances.
- [ ] Verify genetic algorithm mutation/crossover produces legal class candidates and finalists can be reevaluated in detailed mode.
- [ ] Check consistent panel placement on desktop and mobile when switching classes.
- [ ] Cover class switching, invalid cross-class inputs, saved-build migration, and supported APL analysis/optimization paths.
- [ ] Check GPU batching, shader reuse, mobile layout, and static/offline asset loading for both classes.
- [ ] Document the supported mechanics and remaining gaps for each class.
- [ ] Document how to add a third class using the module contract, including any remaining shared-code changes.

## Completion criteria

This preparation is complete when Warlock and one additional class work through the same stable layout, each has separate class-owned shaders with equivalent fast and detailed behavior, both can use the common genetic algorithm evaluation contract, legacy Warlock builds still load, and results are validated within documented scope and tolerances. Adding another class should have a documented extension path with no need to rewrite the shared interface or GPU runner.

Full support for every class, every specialization, healing/tanking metrics, and multi-target simulation require separate scope decisions. The initial milestone is a validated path for adding classes to the existing damage simulator.

Legacy migration progress: staged mapping is implemented but not connected to UI import/export. Sparse UI payloads retain missing defaults and `apl: null` until class normalization; legacy rotation modes remain in class options rather than becoming GA policy IDs. Resolved imports retain their original configs without invented packing/simulation versions. Combat validation, equipment-mode translation, full round-trip export, and browser integration remain pending. No new service-worker asset is needed until these standalone modules enter the browser dependency graph.

Import normalization scope: implemented for direct-stat, level-63 single-target staged builds with text/policy APLs; equipment mode imports explicitly reject pending gear resolution. Buff values must be boolean, but buff names/effects are not yet validated or applied. Rules-form APLs and full evaluation/export resolution remain pending. Defaults preserve supplied direct stats and allow legal talent allocations below 51 points. Missing APL uses a class-owned shadow policy; no UI defaults are silently repaired. The service-worker cache advances to v33 and includes the deferred Warlock import module. Browser import/export remains unchanged.

Equipment/buff progress: staged equipped imports now resolve with an explicitly supplied item database; direct imports retain supplied stats while validating any saved items. Current enchants and Bloodvine/Nemesis bonuses match the existing gear path; slot checks support second ring/trinket slots. Haste remains unsupported. Buff names now follow the saved combat-form schema (not the unrelated selector IDs), but effects are not yet applied. Import output still requires final buff/race/APL resolution before evaluation. UI gear and simulation code remain unchanged; cache v34 includes the equipment module.

Import effect/resolution progress: buff/racial effects and text/policy/rules APL resolution are implemented outside the UI path; cache v35 includes the effects module. Logical resolution produces a model-validated config while resolved inputs skip effects. Existing `buildFightConfig` multiplier behavior is preserved, including its replacement of supplied school multipliers; this extraction does not establish buff-mechanics correctness/CPU parity. Full versioned export/import round trips, stricter semantic edge cases and resolved-version compatibility remain pending before browser integration.

Round-trip checkpoint: four build-I/O tests pass; whole Node suite is 69/71 with the two known failures. Pure legacy/v1 logical exports reimport to identical configs and GPU packed words, including equipped snapshots, explicit zero stats, disabled rules, and canonical action IDs. Input payloads remain unchanged. Unknown versioned resolved compatibility rejects before class loading; no packing/simulation IDs are fabricated. Current browser import/export remains unchanged, browser correctness/throughput and complete semantic coverage are not established, and mechanics parity remains separate. Cache v36 refreshes the import-rule fixes.

Resolved-version checkpoint: `exportResolvedBuild` emits v1 envelopes with registry-owned packing/simulation IDs; exact-match imports validate directly without reapplying effects. Fingerprints cover model/layout and model/kernel/config-builder sources and require compatibility review when those sources change; they do not certify CPU parity or support older resolved versions automatically. Full suite: 72/74, same two known failures. Browser checkpoint could not run in this environment: no Playwright package or browser executable is installed. UI import/export remains unchanged. Cache v37 includes the registry version metadata. Next: provision browser validation and integrate pure build I/O with UI staging/failure-before-mutation tests.

UI integration checkpoint: editable text/rules inputs stage/validate fully before applying panel data, clear old buff selections, retain exact resolved config until a subsequent edit, and retain the logical envelope for reexport. Legacy/v1 resolved inputs and named policy APLs use resolved mode; policy bytecode is not converted into editable text. Successful imports invalidate the stored simulation result. The duplicate export-button handler was removed so a click produces one versioned build download. Legacy `haste: 0` is accepted/removed during normalization; nonzero haste still rejects. Cache v38 includes all new build-I/O modules and item data. Tests are 76/78 with the same known failures; instrumented setter tests are not a real browser check. Throughput and the newly integrated browser flow remain pending.

Evaluation boundary progress: common wrapper implemented and tested with an injected native runner; real GPU and optimizer integration remain pending. It initially accepts one supported class per request and resolves logical inputs outside native GPU loops. Native runner exceptions/cancellation still reject the whole request. Successful results require exact requested completed sample count and finite totals/mean; incomplete results have null fitness. Native `summary.sd` is sample standard deviation, so uncertainty is typed standard error `sd/sqrt(n)` only for n>1. Seed audit: multi-config runner uses the first seed with `0 -> 42` fallback, and shader uses lane modulo iterations for each multi-config candidate; wrapper preserves this by sending one seed to all configs and retaining native execution. No optimizer candidate conversions or loops have changed in this step.

Native finalist adapter checkpoint: APL search uses the common result mapping only for finalists and retains legacy native batch results for existing display/accounting. Stable IDs are local to the finalist request, fixed to the active class. Already-resolved candidates are not migrated/rebuilt or cloned; seeds and options pass through exactly. Per-sample completion validation adds a CPU scan to finalist results; measure its throughput cost before expanding. Four tests pass, including instrumented native-vs-adapter search parity. Real GPU comparison/performance remains pending; cache v39 includes both evaluation modules.

Build finalist adapter checkpoint: both APL and build optimizers now evaluate finalists through the shared native boundary; initial screening and generation batches still use direct native calls. Candidate configs/seeds/options and existing result payloads remain unchanged. Five native adapter tests pass, including native-vs-adapter orchestration parity for both optimizers. APL throughput was accepted by the user; build-finalist throughput and real GPU comparisons remain pending. Cache v40 refreshes the changed optimizer.

## Current readiness landscape

- **Implemented foundation:** Warlock class registry/presentation/simulation references; class-owned search rules; logical/resolved build migration, validation and UI import/export; source-version compatibility; common evaluation/result boundary.
- **Evaluation integration:** Both APL and build search use common fitness for initial screening, generations, and finalists. Native candidate representation, configs, GPU packing, seeds, selection, and double buffering remain unchanged. Latest suite: 90/92 with the same two known failures.
- **Before runtime additional-class integration:** select runtime class dependencies rather than startup Warlock references, define capabilities/search descriptors, and audit class assumptions in APL tools, gear/buffs, UI controls, and result presentation.
- **Dispatch/UI:** add class selection with stable panels, class-specific legal data/actions, class-owned fast/detailed shader dispatch/cache, and class-aware result mappings. Mixed-class batching can remain deferred for initial fixed-class searches; the broader roadmap still tracks it.
- **Second class:** choose class/spec/ruleset/mechanics, audit data and CPU oracle, then implement scoped mechanics and presets through the established boundaries. Data/scope auditing can start now; end-to-end runtime support still requires the integration above.
- **Validation remaining:** real GPU parity/timing, throughput for newly wrapped screening/generation batches, offline loading, legacy/browser flows, and new-class CPU/fast-detailed comparisons. User browser/import-export and finalist throughput reports are recorded; they do not replace these broader checks.

Next immediate step: confirm all-stage adapter throughput, then audit/introduce runtime class dispatch and capabilities. Build-search screening/generation migration is complete. Cache v41 refreshes APL evaluation integration.

All-stage evaluation checkpoint: both optimizers now submit every evaluation batch through the common native boundary and consume only completed finite common objectives. Native payload references, candidate configs, GPU packing, seeds, batching, RNG call order, population selection, and double buffering remain preserved in instrumented comparisons. Suite now passes 90/92 with the two known failures. Latest all-stage throughput, real GPU parity, and offline checks remain pending. Cache v42 refreshes build evaluation integration.

Capabilities/runtime checkpoint: Warlock capabilities explicitly advertise existing interfaces without claiming full CPU parity. Runtime loading caches by class ID, shares concurrent loads, validates class identity/function/shader contracts, and evicts failed loads for retry. `active_simulation` and logical evaluation use this resolver; common requests/imports are capability-gated. Tests use synthetic classes to prove isolation without adding a real second class. UI class switching and optimizer per-search class dependency selection are still unimplemented. User accepted all-stage throughput before this checkpoint; cache v43 includes runtime/capability modules. Full suite: 90/92, the same two known failures.

Optimizer binding checkpoint: both search implementations now take explicit class-module/simulation dependencies and contain no direct Warlock engine imports. Public run wrappers resolve capability-checked class runtime/dependencies once before starting; defaults still use startup Warlock. Existing helper exports remain startup-class references for compatibility. Build talent/preset resolvers come from class-owned deferred dependencies; their current implementations still delegate to existing Warlock modules. Two binding tests plus existing orchestration parity checks pass; suite 90/92, same known failures. This does not add UI class switching or a second class. Cache v44 includes the factory modules and dependency loader.

Dependency follow-up: preset pet/sacrifice inference moved unchanged into `warlock_preset_options.js` and reexported from presets. Search dependency loading no longer pulls the presentation/simulation preset view into optimizer helper imports.

Priest selection/audit checkpoint: user selected Priest and pointed to the CPU simulator. Audited 53 talent nodes (18/17/18), seven CPU DPS policies, four presets, 52 stored spell entries, racial data and icons. Rebuilt/reran CPU tests: all 19 PriestSim tests passed; full suite 224/231, seven non-PriestSim failures. CPU custom APL execution is absent despite policy display/export support; duplicate aggregate misses, gross mana gain accounting, resistance handling, execute timing, and channel scheduling need scoped fixtures/decisions. No Priest runtime registration or shader was added. Next: finalize scope and create explicit CPU Priest reference fixtures, then metadata/UI adapters and scoped shader implementation.

Priest UI preview checkpoint: enabled the existing Priest button and added isolated preview content in all four tabs. Current Build shows all 53 talent nodes, rank descriptions/icons, and three CPU reference presets (Standard Shadow 14/0/37, Deep Shadow 10/0/41, Smite 14/37/0). The CPU Power Infusion Smite function actually allocates 22/30/0 (52 points); excluded pending correction rather than displaying the advertised 21/30/0. Priest allocations are read-only, with simulation/search/APL editing/gear/import-export unavailable. The preview does not register a Priest simulation runtime or alter startup Warlock bindings. Switching back restores the original DOM controls, build/results and selected tab; switching during simulation or either optimizer is blocked. Cache v45 includes preview data/module/icons. Two preview checks and four UI import checks pass; visual browser/mobile/offline checks remain pending. Next: verify this preview in the browser, then implement validated Priest configuration/runtime and scoped Shadow reference fixtures.

Shared configuration UI correction: the first Priest preview bypassed the normal configuration panels and was too sparse. Priest now reuses the existing direct-stat, target and simulation markup through `shared_configuration.js`, using the same armory/talents/simulation column wrappers and talent-tree layout/responsive rules. Class overrides replace Fire Power with Holy Power; controls have isolated names/IDs, no copied Warlock listeners and class-scoped unavailable controls. Editable preview values and selected preset persist across switches, while Warlock retains its original form/results. Preview values are local only, not yet normalized into Priest logical builds or evaluated. Runtime-dependent combat summaries, APL, gear/buffs and results still require class adapters; this is incremental UI standardization, not completed runtime class switching. Three preview/shared-panel checks and four UI import checks pass. Cache v46 includes shared configuration code; real browser layout checks remain pending.

Configuration layout enforcement follow-up: removed the separate Priest configuration template and its panel/select CSS overrides. Both classes now use the complete existing `current-configuration` grid as the source, including stats, APL frame, consumable/raid/world/debuff/target accordions, talent header/badge/preset/reset controls, combat summary and simulation/results frames. Priest changes only class content and enabled controls; Fire-school consumable/debuff rows and Warlock pet/sacrifice/APL references are omitted. Removed development notices and placeholder paragraphs. Priest summaries/results remain blank rather than copying Warlock values; action controls remain disabled. A structural contract checks that Priest takes one copy of the whole shared grid and retains it across class switches. Four preview/shared-panel tests and four UI import tests pass; browser visual validation remains pending. Cache v47.

Shadow Priest shader checkpoint: user selected Shadow and required the Warlock DES/fast-detailed structure. Added class-owned packed model/state, one shader generator with fast/detailed observation variants, the same splitmix64/xoshiro256** family, bounded event heap, four Shadow spells, cast/GCD/DoT/channel/cooldown/mana events, explicit overflow/budget failures and common summaries. A class-bound packed Three.js transport and standalone Priest simulation contract are implemented; Warlock sources/runtime are unchanged. Actual headless GLES fixtures on llvmpipe pass bit-for-bit common-state fast/detailed parity, analytic spell cases, multi-config mapping/offsets and failure rejection; mocked transport checks cover readback chunks/cancellation. This is a resolved four-spell bootstrap, not the fully implemented 14/0/37 preset or CPU default policy. No Priest registry capability or UI run button is enabled. Class defaults/talents, Shadowform/Weaving, spirit/5SR, clipping, resistances, Inner Focus, diagnostic traces and CPU parity remain next. See PRIEST_PREPARATION.md for exact scope and timing/RNG caveats. Cache v48 includes standalone bootstrap modules.

Shadow bootstrap validation total: 100/102 Node checks pass, zero skipped. The only failures remain the existing Warlock racial-accounting and regret-shader-fingerprint checks. All six new core/transport checks pass, including actual headless GLES execution. Post-suite cancellation refinement was rechecked with both transport tests. Browser Three.js execution, CPU equivalence and user GPU timing are still pending.

Forever talent checkpoint: user reaffirmed WoW Forever. Added full 53-node allocation legality and per-allocated-talent effect reporting. Nine verified effects now resolve into the packed core: Shadow Focus, Darkness, Shadowform, Twin Disciplines, Mental Agility, improved SW:P/MB, Mind Flay gating and improved MF. Instant/channel multipliers and costs are distinct; unresolved combat effects reject unless explicitly deferred for scoped fixtures. No Priest UI/runtime registration changes. Idle DES scheduling now wakes on exact cooldown/DoT transitions after a 5.5-second MB fixture exposed mana-tick polling delay. Awaiting the user's confirmation of Forever SW:P/MF snapshot vs dynamic Shadow Weaving behavior before implementing that dependent mechanic. Mental stat effects, Meditation/5SR, Inner Focus and execute-health modeling remain pending. Cache v49 includes talent modules.

Talent-pass validation: all nine targeted checks pass (three talent/schema checks, four core checks including actual headless GLES, two packed-transport checks). GPU fixtures verify eight SW:P ticks and 235 mana with Shadowform, improved MF damage and 102.5 mana, two MB impacts by 7 seconds with a 5.5-second cooldown versus one untalented, learned-MF gating and Mental Agility instant costs. Fast/detailed common state remains bit-identical for all added talent fixtures. Existing whole-suite baseline remains 100/102 from the previous checkpoint; this pass ran targeted checks rather than repeating the full suite.

Runnable Shadow core checkpoint: user confirmed dynamic SW:P/MF damage with Shadow Weaving. Added current-stack damage/procs/15-second expiry to both shader variants, removed the SW:P damage snapshot and added positive-spend 5SR plus spirit/casting regeneration parameters. A legal `Shadow core (20/0/31)` preset runs with all applicable damage effects implemented and no deferred talents; original CPU presets remain gated. Human direct-stat translation, native Priest dispatch, Run/Stop, mode/progress, summaries and detailed result rows now work through the shared layout. No development banners were added. Native imports/searches still do not advertise Priest; full presets, IF/Meditation/stat talents/Early Demise/DP/clipping/resistance/gear/buffs/diagnostics remain pending. Headless dynamic/expiry/regen fixtures and UI dispatch/cancellation checks pass; total 107/109 with the same two known Warlock failures and no skips. Browser/Three.js validation remains pending. Cache v50 includes the browser run modules and refreshed core preset/shaders. Next: verify the runnable core in browser, then implement the full Shadow preset's remaining effects and corrected CPU fixtures.

Meditation/Inner Focus checkpoint: added Forever 17/33/50% casting-spirit regeneration and learned Inner Focus with 180-second CPU-reference cooldown, one free spell, +25-point direct crit bonus, consumption on SW:P/MF/misses, and cast-start-to-impact tracking. Zero-cost spells do not extend 5SR. Detailed output records activations; common fast/detailed state remains identical in actual GLES fixtures. The runnable core preset now allocates IF/Meditation while retaining legal 20/0/31 and 51 total points. Original Shadow builds are still gated by DP/Devouring Contagion and Early Demise. Tests intentionally diverge from CPU MF consumption/free-cast 5SR bugs and lower-rank Meditation fractions; these need corrected parity fixtures. Cache v51.

Validation after IF/Meditation: 108/110 checks pass, no skips. All actual GLES fixtures and UI/class/transport checks pass; the only failures remain the previously documented Warlock racial-accounting and regret-shader-fingerprint checks. Real browser throughput remains pending.

Remaining damage talent checkpoint: 32/53 talents now have damage-scope effects, including Devouring Contagion/Undead Plague, Power Infusion, Searing Light/free Nova, Shadow/Holy Reach, Wand Specialization, Shadow Affinity/Silent Resolve Holy threat, Improved Mana Burn, and Spirit Tap. The original Shadow and Smite presets run without deferral; new Undead and 31/20/0 PI presets are available. Configured credited target replacement events test Plague jumps and Spirit Tap; this is not yet health-driven multi-target combat. Detailed observations include threat, drains, activation/proc/spread counters. The 21 healing/defense/control talents and Silent Resolve's control-duration component still need their corresponding encounter systems. Config scope v3/cache v53. All Priest/GPU/UI tests pass, and the full suite passes 21/23 files with only the same known Warlock racial and regret fingerprint failures. See PRIEST_PREPARATION.md for conditions and limits.

Priest constrained-search checkpoint: user fixed scope to single-target, no deaths/spreading or incoming damage, and duration-based target health. Enabled the existing constrained dashboard for Priest with class-owned 53-node/51-point legal generation, maxed required talents and prereqs, Human/Undead race locks, fixed Shadow/Holy/mixed policies, batched shared-optimizer evaluations, MAP-Elites diversity, higher-precision finalists, live ranks/convergence, Stop-and-preserve, and exact Apply to current build. Neutral stats are resolved once per candidate. Preset/input/result state is retained independently across class switches; running locks inputs/class switching. Global Priest imports and APL synthesis stay gated. New allocation/evolution/controller fixtures and an actual headless GLES search pass, including exact packed winning-build replay. Full suite: 22/24 files pass, only the existing Warlock racial and regret hash failures remain. Cache v54 refreshes search modules. User-GPU large-search throughput/browser appearance remain to be observed.
