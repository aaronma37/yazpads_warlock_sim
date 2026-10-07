# Multiclass boundary contracts — v1 design

Status: design recorded; pure envelope identity/version validation is implemented in `src/contracts/saved_build.js`. Pure legacy classification/staged mapping is implemented in `src/contracts/legacy_build.js`; initial Warlock direct-stat normalization is implemented; complete resolution and runtime integration are not implemented. This document fixes the outer shapes for incremental implementation. Class payload validation remains class-owned. Existing optimizer candidates, GPU packing, shaders, and evaluation loops remain unchanged until each boundary is integrated and checked.

## Logical candidate

A candidate is JSON-serializable build input, independent of run settings and GPU packing:

```js
{
  schemaVersion: 1,
  classId: 'warlock',
  candidateId: 'candidate-0',
  encounter: { duration: 180, distance: 30, target: { level: 63 } },
  stats: { intellect: 300, spirit: 200 },
  equipment: { mode: 'direct', items: {} },
  buffs: {},
  talents: { affliction: {}, demonology: {}, destruction: {} },
  apl: { kind: 'policy', policyId: 'SHADOW_DESTRO' },
  classOptions: { race: 'ORC', pet: 'none', sacrifice: 'none' }
}
```

The example illustrates shape, not a complete validated configuration. `schemaVersion`, `classId`, `candidateId`, and each outer payload field are required in normalized candidates. Adapters supply defaults from the class module before validation. Candidate IDs are opaque nonempty strings, unique within an evaluation request; they are not content hashes, preset IDs, or diversity keys. IDs must survive batching and finalist reevaluation.

`encounter` owns duration, distance, and target settings. Shared fields retain existing app units (seconds, yards, target-level integer); unsupported encounter mechanics are rejected. `stats` contains class-defined stat keys and values in existing app units. `equipment.mode` distinguishes direct stats from gear-derived stats; the class adapter must document whether the saved stats are editable direct inputs or a display snapshot. Do not apply gear or buffs again to a resolved configuration.

`talents` stores named class trees/ranks; typed talent vectors remain internal to class search. `classOptions` owns racial choices, pets, sacrifice, and any future specialization/ruleset identifier. An unsupported specialization/ruleset must fail class validation; this design does not choose the additional class's ruleset.

`apl` is a tagged union:

- `{ kind: 'policy', policyId }`: class-owned named policy.
- `{ kind: 'text', text }`: editable APL text compiled by the selected class.
- `{ kind: 'rules', rules }`: class-owned logical action/condition rules, including disabled rules and both clauses.

Action IDs and condition interpretation are scoped by `classId`; numeric GPU/CPU IDs never form a global action namespace. An action from another class is rejected by the selected class. Preserve the current class rules' ordering, disabled state, parameter precision, and fallbacks during adaptation.

## Evaluation request

```js
{
  schemaVersion: 1,
  requestId: 'evaluation-0',
  candidates: [/* normalized candidates */],
  settings: {
    iterations: 4096,
    seedSchedule: { kind: 'existing-warlock-v1', seed: 42 },
    mode: 'fast',
    objective: 'mean-dps',
    batchSize: 524288
  }
}
```

Run settings belong here, not in logical candidates. Iterations are positive integers; seeds follow the existing unsigned integer domain. Output mode is `fast` or `detailed`. Batch size is an execution setting, not combat behavior. Objective and seed-schedule identifiers must be supported by the class/runner; there is no automatic substitution for unknown identifiers.

`existing-warlock-v1` means preserve the current Warlock lane mapping and RNG schedule. Before wiring this identifier, document the actual mapping for single/multi-config execution and optimizer screening/finalist phases. The current multi-config runner takes its seed uniform from the first configuration (`configs[0].seed || 42`); adapters must preserve effective behavior, including zero-seed handling, rather than silently assigning independent candidate seeds. Cross-class comparable seed scheduling needs its own verified implementation.

Resolve class dispatch once per class batch. Class validation/resolution translates logical candidates into current native candidates/configs outside hot evaluation loops. Keep initial searches fixed to one class. Future mixed-class evaluation groups by class and restores original input order using candidate identity and original index. Callbacks, cancellation signals, and progress functions are runtime options, not serialized request fields.

## Common result

```js
{
  schemaVersion: 1,
  requestId: 'evaluation-0',
  candidateId: 'candidate-0',
  classId: 'warlock',
  status: 'complete',
  sampleCount: 4096,
  objective: { id: 'mean-dps', value: 1234.5 },
  uncertainty: null,
  detail: null,
  error: null
}
```

Status is `complete`, `invalid`, `incomplete`, `cancelled`, or `error`. Only complete results with the requested sample count and a finite objective value may enter fitness selection. Other statuses have `objective.value: null`; never replace failure with zero DPS or the previous candidate fitness. Preserve current engine rejection of incomplete fights.

`sampleCount` records completed samples, not requested iterations; use zero when none completed. `uncertainty` is null when unavailable, otherwise an explicitly typed object (for example `{ kind: 'standard-error', value }`); do not reinterpret the existing `sd`, `stdDev`, or confidence fields without auditing their calculation and units. `detail` holds class-owned diagnostics/legacy payloads; fast mode may omit them. `error` is null for success, otherwise `{ code, message }` with no successful fitness score.

Return one result per submitted candidate in original order when the runner completes normally. Cancellation or fatal errors may reject the request following current runner behavior; a UI may separately report request cancellation. Do not invent per-candidate completion if the underlying runner has no partial result data.

## Saved builds and legacy migration

Version saved envelopes separately from candidate content:

```js
{
  schemaVersion: 1,
  classId: 'warlock',
  kind: 'logical-build',
  candidate: {/* candidate schemaVersion: 1, matching classId */},
  evaluationDefaults: {/* serializable settings; optional */}
}
```

An already-resolved import uses `kind: 'resolved-config'` with `resolvedConfig`, `packingVersion`, and `simulationVersion`. A class validates resolved input directly without reapplying talents, race, buffs, or equipment. Implementation must establish real version identifiers before emitting this envelope; packed-layout identifiers do not imply CPU parity.

Migration order:

1. Decode existing JSON/base64 syntax without changing its transport encoding.
2. If a versioned envelope is present, reject unknown versions/kinds, unsupported classes, and envelope/candidate class mismatches before changing UI state.
3. Unversioned payloads without `classId` are legacy Warlock. An explicit class ID must resolve through the registry and must never be silently coerced to Warlock. A versioned payload missing class identity is invalid.
4. Classify existing legacy formats: UI builds (`race`, `pet`, `ds`, `rotation`, `gearMode`, `stats`, `gear`, `talents`, `buffs`, `target`, `sim`, `aplText`); `{ resolvedConfig }`; simulation exports containing `{ config }`; and raw resolved configs accepted by today's importer. Preserve their existing precedence.
5. Map UI `target` plus `sim.duration/distance` into encounter, `sim.iterations/detailedResults` into evaluation defaults, `gear`/`gearMode` into equipment, `aplText` into text APL, and race/pet/DS/rotation into class options or policy. Keep legacy racial-policy behavior until audited; do not move it solely based on its current storage under target settings.
6. Validate and stage the complete migrated build before applying UI changes. Missing optional legacy fields use current Warlock defaults; invalid explicit values fail clearly. Migration must not mutate the source payload.
7. Adapt preset and result exports at separate boundaries. Do not rewrite repository preset data or serialized result payloads as a side effect of build import.

Do not infer logical build inputs from a resolved config; preserve it as resolved input. Keep legacy import support until migration tests cover each accepted format.

## Search and cache boundaries

The extracted Warlock functions are the first class-owned search contract, not a generic declarative search descriptor. A future descriptor enumerates gene types/ranges, legal actions, conditions, talent prerequisites/budgets, and repair semantics. Preserve internal candidates and call order while introducing it incrementally.

Current deduplication/diversity keys remain search-local and retain their exact existing semantics. They omit fields and round parameters; they cannot identify universal simulation cache entries. Future cache keys include class, candidate schema, actual packing/simulation versions, complete resolved simulation input, effective seed schedule, iterations, objective, and relevant evaluation settings. No cross-class reuse is allowed.

## Implementation sequence and acceptance

1. Browser correctness/throughput checkpoint for search extractions; record measured or user-reported evidence in `MULTICLASS_TODO.md`.
2. Pure saved-envelope identity/version checks (implemented, five tests; not UI-integrated) with tests for legacy Warlock, unsupported classes/versions, mismatches, and immutable input. Keep current import/export payloads otherwise intact in this first boundary.
3. Stage legacy logical/resolved migration adapters and verify round trips before UI integration.
4. Add class-owned logical/native adapters and common request/result wrappers outside GPU hot loops; compare packed words, seeds, ordering, completion, and fitness with existing behavior.
5. Add runtime class selection and second-class dispatch after choosing/auditing its scope.

This design is not runtime multiclass support. Shader/packed-format changes, numerical tolerances, search-descriptor integration, cache implementation, and additional-class mechanics remain separately tracked work.

Implemented validator scope: returns an identity descriptor retaining the original payload. It checks envelope/candidate schema and class identity, candidate ID, kind, required resolved-config version strings, and optional evaluation-default object shape. It does not validate combat fields, interpret legacy formats, verify supported packing/simulation versions, or normalize candidates. Those checks belong to staged class adapters before runtime/UI integration.

Legacy adapter scope: recognizes the existing resolved-input precedence and accepted UI containers, clones payloads, and stages the field mappings. It preserves missing defaults, uses `apl: null` when text is absent, and retains legacy simulator rotation under class options rather than treating it as a named GA policy. This staged candidate is not ready for evaluation. Resolved mappings omit v1 envelope/packing/simulation versions until actual compatibility is established. Class semantic validation, defaults, mode normalization, and UI/export round trips remain required.

Warlock normalization progress: `registry.loadImports` resolves a pure staged-import normalizer. It validates direct-stat values, single-target encounter scope, named talent ranks/prerequisites/budget, pet/sacrifice legality, and text/policy APL actions while preserving explicit inputs. Resolved configs use existing model validation directly. Equipment resolution, buff names/effects, rules-form APLs, full evaluation resolution, version compatibility, export/import round trips, and UI integration remain pending.

Equipment/buff-name validation progress: equipped normalization accepts an explicitly supplied item database, checks IDs/slots, and derives current item/enchant/set stats instead of trusting saved display snapshots. Direct-stat inputs preserve supplied stats. Saved buff names use the combat form fields and boolean values; buff/racial effect resolution remains pending. The existing gear UI path is unchanged.

Import resolution progress: `resolveWarlockImport` now combines normalized gear/direct inputs, combat-form buffs/racial stats, and class-owned APL resolution through existing configuration building. Rules-form imports require known action IDs, boolean enabled flags, and compilable string `rawCond`; numeric action/condition fields are not trusted as the import source. Resolved imports bypass effects. Current builder replacement of external school multipliers is preserved; correcting that behavior requires separate mechanics validation. UI integration, full export/import round trips, semantic edge-case audit, and resolved packing/simulation-version compatibility remain pending.

Round-trip checks implemented: pure `resolveSavedBuild`/`exportLogicalBuild` dispatch decoded payloads through registry class resolution. Logical v1 and legacy inputs preserve final configs/packed words across JSON/base64 round trips. Legacy resolved formats remain direct; versioned resolved envelopes explicitly reject until compatibility identifiers are registered. Rules imports rebuild numeric fields from action names/conditions, rejecting malformed multiline/empty conditions; resolved imports validate numeric bytecode domains. This is not UI integration or a browser/performance checkpoint.

Resolved version compatibility implemented: registry `versions` uses conservative source SHA-256 fingerprints for packing/model and simulation model/kernel/config-builder. Pure build I/O accepts exact matches and rejects mismatches before class resolution; `exportResolvedBuild` emits those IDs. Tests verify fingerprints against current sources and exact config/packed-word round trips without double effects. Older versions need explicit migration/compatibility rules; fingerprints are not a claim of mechanics correctness. Browser/UI integration remains pending.

UI boundary integration: app imports are asynchronous and staged before setters; logical text/rules builds populate existing panels and preserve a resolved config until edited. Exports emit v1 logical envelopes or compatible resolved envelopes, with unchanged JSON/base64 transport and legacy decoding. Named policy builds currently use resolved mode to preserve exact policy bytecode; editable policy conversion remains future work. Tests exercise actual import code with instrumented setters, not a real browser. New-flow browser/offline/performance checks and shared evaluation/result integration remain pending.

Common evaluation implementation: `src/contracts/evaluation.js` provides validation, a dependency-bound evaluator, and default registry dispatch. Initial requests require one supported class, unique candidate IDs, v1 objects, mean-DPS objective, fast/detailed mode, and `existing-warlock-v1` seeds. Native batch settings and lane order are preserved. Shader seeding uses candidate-local lane (`globalLane % fightsPerConfig`) for multi-config runs; engine zero seeds still fall back to 42. Results check every returned sample for `done === 1`/finite total and exact sample count, retain native mean, and expose standard error from audited sample `sd` for n>1. Runner cancellation/fatal errors reject rather than inventing per-candidate scores. Tests use an injected runner; real GPU comparisons and optimizer/UI evaluation entry point integration remain pending.

Native evaluation bridge: `createNativeEvaluator` binds a class ID/native runner once, accepts already-resolved configs plus request/candidate identity metadata, and returns the original native batch fields with common `fitnessResults`. It rejects incomplete results before fitness selection and leaves config/option references and effective native seeds untouched. The APL optimizer finalist phase uses this bridge; screening/generations and build search remain direct. Four tests include actual optimizer orchestration parity with an instrumented runner, not GPU timing or CPU mechanics parity.

Build finalist integration: the build optimizer now uses the same native bridge for finalist evaluation and consumes completed common objective values. Existing configs, seeds, order, batch payloads, rankings, and final exports are preserved in instrumented comparisons across static/evolved locked/unlocked APL modes. Both optimizers still call the native runner directly for screening/generations. APL-finalist throughput was user-accepted; build-finalist throughput and real GPU comparisons remain pending.

APL all-stage integration: initial screening and generation batches now use the native bridge as well as finalists; fitness uses completed common objectives. Instrumented two-generation locked/unlocked searches preserve native inputs and full output. Build screening/generations remain direct. User accepted build-finalist throughput; throughput for new APL screening/generation checks remains pending.

Build all-stage integration: build initial screening/generation batches now use the same native bridge as finalists. Both optimizers consume completed common objectives at all evaluation stages; internal candidate formats and legacy batch payloads remain unchanged. Instrumented two-generation build searches preserve native batches and final output across static/evolved locked/unlocked APLs. All-stage throughput and real GPU comparison remain pending. Next architecture work is class capabilities/runtime dispatch.

## Capabilities and runtime dispatch

Registry modules expose `capabilities.schemaVersion: 1`, boolean `features`, supported `modes`, `objectives`, and `seedSchedules`. Features currently advertised by Warlock are simulation, batchSimulation, buildSearch, aplSearch, importBuild, and regretScan. Unknown/unavailable features reject explicitly; these flags describe interfaces, not mechanics/CPU parity.

`resolveClassRuntime(classId)` loads and caches a simulation contract per class, shares concurrent loads, validates identity/functions/mode shaders, and retries failed loads. Returned simulation functions are direct references; there is no per-lane lookup or UI state switch. Startup Warlock simulation and common logical evaluation use it. Imports/evaluation gate supported capabilities before execution. Optimizer explicit per-search dependency selection and actual class switching remain future steps.

Optimizer dependency binding implemented: public search APIs accept `options.classId`, capability-check before loading, resolve class simulation/search dependencies once, and instantiate class-bound optimizer implementations in `src/search/`. Internal candidate representation, RNG calls, population loops, batching, and shared fitness remain unchanged. Existing public helper exports retain startup-class references. Warlock provides deferred talent/preset resolvers; actual UI class switching, fully class-aware UI/data/analysis tools, and another class remain pending.
