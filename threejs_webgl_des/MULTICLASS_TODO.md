# Three.js WebGL: Support for Additional Character Classes

Status: proposed goal and implementation checklist. No class expansion is implemented by this document.

## Goal

Prepare the Three.js WebGL2 app to simulate character classes beyond Warlock through explicit class modules. Each class should own its spells, talents, resources, rotation rules, presets, and presentation data while sharing the browser UI, GPU execution infrastructure, and results tools where their behavior is common.

Keep existing Warlock builds working throughout the migration. Demonstrate the architecture with one additional class before expanding further. The first additional class and supported specialization remain to be chosen; Priest is a candidate because this repository already contains Priest reference data, but that data needs an audit before use.

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

Use a versioned logical candidate shape across classes. Proposed fields are `schemaVersion`, `classId`, `encounter`, `stats`, `equipment`, `buffs`, `talents`, `apl`, and `classOptions`. Run settings such as seeds, iterations, and output mode belong to a common evaluation request. Final field names and migration details are an implementation task.

Keep the outer structure and evaluation API consistent while letting each class define valid stat keys, talent trees, action IDs, predicates, and class options. A class validates and resolves a candidate into its own packed GPU configuration. Packed layouts may differ by class; the common contract is at the candidate and runner boundaries.

Each class also supplies a search-space descriptor: mutable genes, types/ranges, legal actions and conditions, talent prerequisites and point budgets, and validation/repair rules. The shared genetic algorithm uses those descriptors for initialization, mutation, crossover, and candidate validation. Initial searches keep `classId` fixed; class changes require an explicit search policy because talents and actions have different meanings.

Return a common fitness result containing candidate identity, class identity, objective value, sample count, validity/completion status, and uncertainty where available. Preserve candidate-to-result mapping across class batches and use a consistent encounter, iteration count, seed schedule, and objective for comparable evaluations. Invalid or incomplete simulations must not become successful fitness scores.

Evaluate populations in fast mode, then reevaluate selected finalists in detailed mode. Cache keys include class identity, schema/packing version, shader version, resolved candidate, and evaluation settings to prevent reuse of results from another class or contract.

## TODO

### Incremental preparation: presentation metadata

- [x] Extract Warlock labels, talent/preset data URLs, talent backgrounds, and existing race/pet/spell icon maps into `src/classes/warlock_presentation.js`.
- [x] Consume that metadata from the existing application shell, talent view, and preset view while retaining panel locations and current data.
- [x] Include the module in the service worker cache and advance the cache version.

This extraction leaves simulation configuration, shader source, packing, GPU execution, APL rules, and genetic algorithm implementation unchanged. Individual talent icons/descriptions remain in `data/talents.json`; the module owns the data reference. Static HTML retains initial Warlock labels as startup fallback. Class selection and the broader class module contract remain future steps.

### Incremental preparation: presentation registry

- [x] Register Warlock in `src/classes/registry.js` with a stable ID and presentation reference; reject unsupported class IDs.
- [x] Resolve the default class once in `src/classes/active_class.js` and share that presentation reference across the app, talent view, and preset view.
- [x] Cache the registry and selection modules for offline loading.

This registry currently covers presentation only. It adds no lookup to simulation or candidate evaluation loops. Runtime class switching, additional classes, and simulation module contracts remain future steps.

### 1. Define scope and capture the baseline

- [ ] Choose the first additional class, specialization, level/ruleset, and initial supported mechanics.
- [ ] Audit available spell, talent, racial, gear, and CPU reference data for that scope; record gaps and authoritative sources.
- [ ] Inventory Warlock assumptions across UI, configuration, shaders, APL tools, optimization, results, and saved builds.
- [ ] Capture representative Warlock configurations and current simulation output, including known parity failures.
- [ ] Define acceptance tolerances for new-class CPU comparisons and Warlock regression checks.

### 2. Establish class modules and configuration ownership

- [ ] Define and document the class module contract and class registry.
- [ ] Define the common candidate/evaluation/result shapes and class-specific search-space descriptors.
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
