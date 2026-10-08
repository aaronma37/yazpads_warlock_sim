# Warlock · Three.js WebGL2 DES

A new, standalone browser app using **Three.js `WebGLRenderer` and GLSL ES 3.00 fragment shaders**. No WebGPU API, WGSL, WASM, ImGui, server-side simulation, or runtime CDN dependency. The existing simulator implementations are references; the production app imports none of their code.

This is a correctness-oriented **single-target core foundation**, not a complete port of every warlock mechanic.

## Run and publish

From this directory:

```sh
python3 -m http.server 8080
```

Open `http://localhost:8080`. Node/npm is needed only for development checks, not for the app. Use HTTP serving rather than opening `index.html` as a file; ES modules and reference-fixture loading need it.

For GitHub Pages, place this folder inside the chosen Pages publishing directory, or upload its contents with a Pages artifact workflow. All app paths are relative, including imports, styles, and fixtures, so repository subpaths work. The runtime files are `index.html`, `style.css`, `src/`, `vendor/`, and `validation/{compare.js,cpu-fixtures.json}`; include this README and `.nojekyll` too. No cross-origin isolation headers or build step are needed. Existing repository Pages files have not been changed by this app.

WebGL2 and four integer color attachments are required. **WebGPU can be absent or disabled.** A browser that disables WebGL2 or loses its graphics context will report an error, not silently switch to another simulator. Mobile layout was exercised at 390px; actual mobile GPU performance and driver coverage still need device testing.

## Supported contract

- Level-60 Human, one level-63 target. Raw intellect/spirit are bonuses added to Human base stats; maximum mana is derived, not entered directly. Crit includes base crit, intellect, and the CPU's Human direct-stat sword bonus.
- Shadow Bolt, Corruption, Agony, Immolate, Incinerate, Searing Pain, and Life Tap.
- Bane fixed at 5/5. Toggles for Improved Corruption 0/5 or 5/5, Nightfall 0/2 or 2/2, Improved Shadow Bolt 0/5 or 5/5, Ruin 0/5 or 5/5, and Improved Life Tap 0/2 or 2/2.
- Cast completion, independent projectile impact, GCD readiness, periodic ticks, Nightfall expiry, and five-second mana regeneration are actual events.
- Agony's tick ramp; Immolate's Incinerate bonus; missed applications; DoT generation IDs; timed or four-charge ISB; CPU resistance rolls and below-zero penetration; normal/book ranks.
- Explicit priority: Life Tap → Nightfall → Agony → Corruption → Immolate → selected filler. Disabled maintenance entries are skipped. Unaffordable actions fall through, then Life Tap is the resource fallback.
- Integer fight durations 1–1800 seconds. Distances 0/30/60/120 yards, no haste. All supported event intervals are exactly representable in the CPU's double clock and the shader's integer microsecond clock.

Pets, gear/buff procs, raid buffs, race selection, arbitrary talents, haste, dynamic buffs, multi-target effects, other spells, APL editing/analysis/synthesis, training, and MCTS are **not implemented**. Input validation rejects unknown options. These mechanics are not approximated in reported DPS.

## Standard Specs Comparison Dashboard

- **Simulation-Only Results**: The comparison table is strictly a live simulation output view. Presets are pure input specifications defined by `talents × stats/gear × APL × race`.
- **No Pre-Baked Data**: Preset files (`data/presets.json`) and JavaScript runtime never store or prefill static DPS values or stat weights into the table. All output columns (`Mean DPS`, `DPS/SP`, `DPS/Hit`, `DPS/Crit`, `DPS/Haste`, `DPS/Int`, `DPS/Spirit`) start blank (`--` / `-`) and are strictly computed on-demand when the user executes **"Simulate Specs"** on the WebGL2 GPU engine.

## GPU execution

1. JS validates and packs the configuration into unsigned uniform words. Float configuration values preserve their bits.
2. Three.js draws a fullscreen triangle into four `RGBA32UI` attachments. A fragment owns an independent fight, its xoshiro state, and a 64-entry binary event heap.
3. The shader repeatedly pops the earliest event and advances directly to its timestamp. There is no fixed timestep, per-event JS callback, or CPU combat simulation.
4. Each fragment runs the fight to its end. Two output rows repeat that computation to emit 32 words per fight through WebGL2's portable four-target output budget. This deliberate 2× arithmetic duplication avoids serializing an entire event queue through many texture passes.
5. The CPU reads compact reports asynchronously through Three.js, checks completion, aggregates statistics, and starts the next batch. Default batches contain at most 256 fights and shrink for long durations. Cancellation is checked between batches; it cannot interrupt an in-flight GPU draw.
6. The first fight is replayed for its full diagnostic state. Its first 256 events can be inspected via bounded GPU prefix replays. This trace has additional cost and is explicitly marked as truncated when necessary. `runSimulation(config, {trace:false})` skips it for throughput measurements.

The complete-fight strategy avoids ping-pong state traffic but may encounter register pressure, slow compilation, or GPU watchdog limits on weaker drivers. Smaller batches limit submitted work; they do not bound a single fight's duration on an arbitrary device. The maximum supported duration is enforced. Future chunking should be justified by measurements and retain the same CPU checks.

Results are never accepted if the heap overflows, events go backwards, the queue empties unexpectedly, the event budget is exhausted, packed counters overflow, the shader fails, or the context is lost. Completed fights must reach the configured end event. No assumption of success is made from submitting a draw.

## CPU fidelity and precision

`src/sim/warlock/warlock_sim.cpp` is authoritative, including details that differ from the old WebGPU implementation:

- Heap comparisons use **time only**, including the CPU's non-stable tie behavior. A fight-end event competes in that same heap; events at exactly the end are not arbitrarily included/excluded by a new rule.
- Cast-time spells spend mana at completion. Instant spells spend immediately. Misses resolve at impact/application.
- The CPU checks Incinerate affordability at **325 mana but spends 355** at completion. This inconsistency is reproduced and covered by fixtures rather than silently corrected.
- The supported periodic spells do not make partial-resist rolls. Direct spells follow the CPU's discrete resist table and RNG ordering.
- SplitMix64 seeding and xoshiro256** state transitions are implemented with pairs of 32-bit integers. A fight uses `baseSeed + globalIteration`, even across batch boundaries and the 32-bit seed boundary. Seed zero uses the CPU's special seed.

Damage and resources are `highp` shader floats, not C++ doubles. Random probability samples use the high 24 bits, while the CPU converts 53 bits. Rare near-threshold decisions and long floating-point resource accumulation can therefore diverge for inputs/seeds beyond the tested cases; this is **not a claim of bit-for-bit parity for every possible input**. Timestamps and integer RNG transitions are exact for the supported contract. Do not introduce fractional haste/cast intervals without revisiting time ordering.

## Validation

The UI's **CPU checks** runs checked-in cases on the user's own GPU. Expected values come from a small adapter that calls the actual C++ simulator; there is no second JavaScript simulation serving as the oracle.

Checks compare every spell's casts/hits/crits/misses, Life Taps, Nightfall and ISB counters, total/per-spell damage, mana spent/gained/final mana, the next 64-bit random output, and the ordered positive-damage trace prefix. Counters and RNG output must match exactly. Float totals allow `max(0.02, 0.002% of reference)`; individual event damage allows `max(0.01, 0.002%)`. The full trace suffix is not checked when the preview is truncated, but all end-of-fight counters/totals and the RNG continuation are checked.

```sh
npm install
npm test
npx playwright install chromium firefox
# Keep the local HTTP server running in another terminal:
npm run test:browser
BROWSER=firefox npm run test:browser
```

`APP_URL` can override the default localhost URL. Browser checks forbid access to `navigator.gpu`, compare fixtures, exercise batch boundaries and every lane of a five-fight CPU comparison, repeatability, cancellation after submission, event-budget rejection, and the desktop/mobile UI. Screenshots and machine-readable results go to ignored `validation/artifacts/`.

Regenerate fixtures from the repository root after changing the CPU source:

```sh
g++ -std=c++20 -O2 -ffunction-sections -fdata-sections \
  -I. -Isrc/sim -Isrc/sim/common -Ithird_party/rl-tools/include \
  threejs_webgl_des/validation/cpu_fixture.cpp \
  src/sim/warlock/warlock_sim.cpp src/sim/common/gear.cpp \
  src/sim/warlock/imitation_training.cpp \
  -Wl,--gc-sections -pthread -o /tmp/warlock-cpu-fixture
node threejs_webgl_des/tools/generate-fixtures.mjs /tmp/warlock-cpu-fixture
```

Only the validation executable links the original CPU code. `cpu-fixtures.json` records source hashes, which the contract tests check for staleness.

## Performance measurements

The displayed elapsed time is **wall time**, including setup, any initial compilation, submissions, readback, diagnostics, and JS aggregation. It is not a GPU timestamp measurement. First-run native driver compilation can occur in the first draw even after `compileAsync`; `compileMs` is only the time spent in Three.js's explicit compilation call.

Use warm runs with the same settings and `trace:false` for a useful throughput baseline, recording browser, actual GPU, batch size, duration, and iteration count. Software-renderer test timings are not representative of desktop/mobile GPU performance. Neither the translation report's speedup claims nor the earlier rough WebGL/WebGPU ratio have been established by this implementation.

Useful next performance work: benchmark 64/128/256-fight batches on real devices, specialize unused spell paths, compact the local heap after proving bounds, and offer a smaller DPS-only report when breakdowns are unnecessary. Any optimization must retain the CPU comparisons.

Three.js is vendored at 0.180.0 with its MIT license. Relevant API references: [WebGLRenderer](https://threejs.org/docs/pages/WebGLRenderer.html), [RenderTarget](https://threejs.org/docs/pages/RenderTarget.html).

## Decision regret diagnostics

After a detailed simulation, click **Action Regret** in Simulation Results. The result table appears when the scan starts. No decision index is required. The scan discovers and evaluates every actual decision in the first fight, comparing every available alternative with 32 continuation samples. It then confirms up to 20 promising decision states with 256 fresh continuation samples each, rechecking all available alternatives at those states. Progress reports scanning and confirmation separately; cancel works between GPU batches. Results are included in JSON exports.

The table shows up to five distinct decision states, with the chosen action icon, cast time, all available action icons ordered by expected ΔDPS from best to worst, and the maximum expected DPS gain. Hover or focus an icon to see its DPS change and adjusted confidence interval. It ranks by the lower confidence bound and only includes strictly positive lower bounds. Confirmation uses paired differences and approximate Student-t intervals with a Bonferroni adjustment across all confirmed alternatives, targeting 95% simultaneous confidence under the sampling assumptions. Screening and confirmation use disjoint seed ranges to avoid reusing lucky screening outcomes. It can show fewer than five items, including none; absence of a result does not prove the policy optimal. Screening can miss improvements, and confirmation intervals describe future randomness, not simulator fidelity. Displayed improvements are independent one-action changes and must not be summed.

A dedicated, lazily compiled diagnostic shader replays the first fight's original seed to each decision, switches to independent continuation RNG streams, executes one alternative action, then resumes the original APL and automatic racial/trinket behavior until the original fight end. Each alternative shares continuation seeds with a policy baseline; changing actions can shift RNG consumption, so paired seeds do not guarantee matching crits or procs. Forcing the recorded policy action must exactly reproduce baseline damage on every sample, otherwise the evaluation is rejected. Normal detailed and fast simulation shaders are unchanged. Diagnostics pack decision × alternative × continuation sample into GPU lanes and use 2D batches of up to 65,536 rollouts by default. A stage reads back once per batch, rather than once per alternative. Inspection prefixes are batched too. The `batchSize` API option supports 1–1,048,576 lanes, bounded by device texture limits; changing batch size preserves sample seeds and results. Returned `timing` includes draw count, rollout count, and requested batch size. Completion/error checks and cancellation between batches are preserved.

DPS differences divide final damage differences by the full configured fight duration. This evaluates states from the first fight, not regret averaged over different fight prefixes. Prefixes are replayed rather than restored from snapshots; long fights can therefore make full scans expensive. Alternatives include spells enabled by the current configuration, Life Tap, and fillers; waits, cast cancellation, and alternative off-GCD policies are excluded. The single-decision `analyzeRegret` API remains available for developer diagnostics.
