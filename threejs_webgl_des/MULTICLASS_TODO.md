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

Share scheduling, seeded randomness, GPU batching, progress/cancellation, and result presentation only where their semantics match. Keep class mechanics explicit rather than adding unrelated class flags to the Warlock configuration. Choose the shader composition and dispatch strategy after auditing the current kernel and configuration layout.

Preserve the roadmap's table-driven APL bytecode requirement: different APLs must remain executable per GPU lane without recompiling a shader for every rotation. Class-specific kernels may be compiled and cached per class. Initially, comparisons can batch by class and combine results; mixed-class lanes in one draw are a separate design decision.

## TODO

### 1. Define scope and capture the baseline

- [ ] Choose the first additional class, specialization, level/ruleset, and initial supported mechanics.
- [ ] Audit available spell, talent, racial, gear, and CPU reference data for that scope; record gaps and authoritative sources.
- [ ] Inventory Warlock assumptions across UI, configuration, shaders, APL tools, optimization, results, and saved builds.
- [ ] Capture representative Warlock configurations and current simulation output, including known parity failures.
- [ ] Define acceptance tolerances for new-class CPU comparisons and Warlock regression checks.

### 2. Establish class modules and configuration ownership

- [ ] Define and document the class module contract and class registry.
- [ ] Move existing Warlock definitions behind a Warlock module incrementally.
- [ ] Add class ID and schema version to saved builds, presets, and exported results.
- [ ] Treat legacy builds without a class ID as Warlock; document migration and reject unsupported versions or classes clearly.
- [ ] Separate common encounter settings from class stats, resources, and mechanics.
- [ ] Namespace action/spell IDs and CPU mappings so classes cannot collide or interpret each other's actions.
- [ ] Validate each configuration through its class module before packing GPU data.

### 3. Make the interface class aware

- [ ] Add class selection and load the selected class's defaults, talents, spells, presets, and assets.
- [ ] Render stats, resources, damage schools, and class controls from metadata, including optional pet controls.
- [ ] Filter legal races, gear, buffs, and consumables using explicit restrictions.
- [ ] Define class-switch behavior for unsaved edits and reset incompatible choices.
- [ ] Route APL editing, fallback analysis, synthesis, and optimization through the selected class's supported actions and predicates.
- [ ] Include class identity in comparisons, result labels, combat logs, and exports; clear stale results when the build changes.

### 4. Generalize GPU execution and result contracts

- [ ] Specify class dispatch, packed configuration layout/versioning, and shader cache ownership.
- [ ] Separate reusable engine infrastructure from Warlock event and mechanic code.
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
- [ ] Cover class switching, invalid cross-class inputs, saved-build migration, and supported APL analysis/optimization paths.
- [ ] Check GPU batching, shader reuse, mobile layout, and static/offline asset loading for both classes.
- [ ] Document the supported mechanics and remaining gaps for each class.
- [ ] Document how to add a third class using the module contract, including any remaining shared-code changes.

## Completion criteria

This preparation is complete when Warlock and one additional class work through the same app, each uses explicit class-owned data and simulation mechanics, legacy Warlock builds still load, and results are validated within documented scope and tolerances. Adding another class should have a documented extension path with no need to rewrite the shared interface or GPU runner.

Full support for every class, every specialization, healing/tanking metrics, and multi-target simulation require separate scope decisions. The initial milestone is a validated path for adding classes to the existing damage simulator.
