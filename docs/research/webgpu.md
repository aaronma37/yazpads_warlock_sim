# WebGPU simulation: first implementation and scheduler experiment

**Decision: use next-event scheduling as the default for the experimental combat slice.**
Both next-event and fixed-timestep implementations are retained for measurement as
mechanics are added. These results support the default on this hardware; they do
not establish a universal GPU winner or predict full-simulator throughput.

## Measured browser results

Measured September 30, 2026 with headless Chrome 152 on AMD Custom GPU 0405
(RADV VANGOGH, RDNA 2 integrated GPU). Chrome reported an AMD hardware adapter,
not a software fallback. Host Vulkan inspection reported Mesa 24.1.0-devel.

Each row is the median of five warmed **10,000-fight** batches. Times include
buffer allocation/upload, dispatch, readback, and JavaScript aggregation, excluding
shader compilation, adapter creation, asset loading and configuration validation.
Runs rotate scheduler order after a full-size warmup. The WASM baseline is the
single-threaded reduced reference with the same combat rules, compiled with -O3.
It is **not** a benchmark against the full production simulator or WASM threads.

| Scheduler | 180.123 s fight, 2.5 s cast | 300.123 s fight, 2.173913 s cast | Haste-case DPS change vs next-event |
| --- | ---: | ---: | ---: |
| GPU next-event | 8.7 ms | 8.6 ms | reference |
| GPU fixed 1 ms | 91.1 ms | 147.7 ms | -0.0004% |
| GPU fixed 10 ms | 18.1 ms | 22.6 ms | -0.1978% |
| GPU fixed 50 ms | 11.9 ms | 14.9 ms | -0.5497% |
| CPU WASM next-event | 45.0 ms | 64.4 ms | — |

The GPU next-event implementation reached approximately 1.15–1.16 million fights/s
at this batch size, 5.2–7.5x the reduced single-threaded WASM reference. The small
50 ms advantage/disadvantage should not be overinterpreted given browser noise;
the 1 ms cost and haste-dependent timing bias are clearer findings.

The fixed implementation rounds scheduled delays upward to the next timestep.
Round cast durations align with all three grids, giving identical results in that
case. Haste introduces rounding and can change casts, refreshes and random draws.
A fixed loop that preserves substep event ordering would be another design, with
additional scheduling work; it has not been implemented here.

100,000-fight event-driven smoke runs completed with workgroup sizes 32, 64 and
128 (34–43 ms). These were **single samples**, not a reliable workgroup tuning
study. A three-candidate batch of 10,000 replicas each also completed (22.1 ms).
The portable default remains 64 invocations/workgroup.

Raw samples, adapter information, validation results, and summary metrics are in
[the captured report](webgpu-amd-rdna2-2026-09-30.json).
Compilation timings in that report are pipeline creation measurements within the
session; browser/driver shader caches may already be warm.

## Implemented scope

Source: `src/sim/webgpu/`. Runnable artifacts: `docs/webgpu/`.

- One independent fight per GPU invocation. No synchronization between fights.
- Single target; Shadow Bolt with hit, damage range, crit, mana cost and cast time.
- Instant Corruption, six three-second ticks, tick crits, refresh after expiration.
- Nightfall chance per Corruption tick and a ten-second instant-cast buff.
- Life Tap, configurable mana threshold and gain, and five-second MP5 ticks.
- Fixed priority: threshold Life Tap, Nightfall, Corruption upkeep, Shadow Bolt.
- Configurable resolved statistics; candidate-major batches with per-candidate
  means, standard errors, damage, cast/tick/proc/miss counters.
- Per-replica 32-bit RNG streams independent of batch shape and workgroup size.
- Pipeline cache, device-limit checks, bounded dispatch chunks, cancellation
  between submissions, device-loss reporting, and incomplete-result detection.
- Packed C++/WASM input ABI (`combat_types.hpp`) and a JavaScript WebGPU runner.

No changes to the existing CPU combat implementation or normal UI backend.
This is a separate browser experiment, not a drop-in replacement for full builds.

Not implemented: pets, projectile travel/overlap, ISB, other spells, resistances,
racial effects, dynamic buffs, multiple targets, arbitrary APLs, build-to-GPU
configuration translation, optimizer, or integration into the existing ImGui UI.
Resolved inputs must not be mistaken for support for arbitrary gear/talent builds.
Unknown fields are rejected instead of silently ignored.

## Timing and correctness contract

The next-event kernel scans four timestamps: decision readiness, cast completion,
next Corruption tick, next mana-regeneration tick. It does not carry over the CPU's
256-event heap. For this restricted model, one slot per event source is sufficient.
Adding overlapping missiles or multiple targets will require additional slots or
queues; overwriting a pending event would be incorrect.

Both kernels use the same WGSL combat functions. State remains on the GPU across
chunks (256 event iterations or 4,096 fixed steps). Up to 16 chunks are submitted
before yielding. Fixed steps run *inside* the shader, not as one dispatch per step.
The host uses a conservative iteration bound derived from the supported minimum
one-second cast/GCD and the fixed periodic sources. Completion flags are verified.

Time is integer microseconds derived from float32 durations. Events at or after
the fight endpoint are excluded. Ties resolve as regeneration, DoT, cast completion,
then player decision. The old heap has no explicit stable tie order, so exact
legacy event traces are not promised. Damage/mana use float32; totals and variance
are aggregated using JavaScript numbers and Welford variance. CPU and GPU RNGs do
not need to match. The same starting stream across candidates aids comparisons,
but different decisions can consume different numbers of random draws.

Validation performed:

1. Eight scenarios, 129 replicas each, for all four schedulers (4,128 fights).
   GPU state/counters/RNG checked against a separately written reduced C++ WASM
   reference; float fields use `max(0.1, abs(expected)*2e-6)` tolerance.
   Covers endpoint exclusion, short fights, haste, OOM stalls, guaranteed procs,
   guaranteed misses, 600-second fights, and rounding at a dispatch boundary. Loop instrumentation is excluded from
   integer comparison.
2. Independent no-randomness analytic damage and endpoint assertions.
3. Three legacy CPU fixtures with 10,000 CPU and 10,000 GPU fights each.
   DPS tolerance: six combined standard errors plus 0.2% of CPU mean;
   count tolerance: `max(0.25, 1.5% of CPU mean count)`.
   These check statistical agreement for the supported configurations, not full
   simulator parity. Both spell counts and proc/tick/tap means are checked.
4. Candidate packing through the WASM-compatible ABI, padded workgroup tails,
   replica independence when batching candidates, invalid parameters, cancellation,
   100,000-fight runs, workgroup-size invariance and multi-candidate evaluation.

## Build and run

Activate Emscripten, then:

```sh
./scripts/build_webgpu.sh
cmake -S . -B build -DBUILD_WEBGPU_ORACLE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target webgpu_oracle -j4
./bin/webgpu_oracle docs/webgpu/oracle.json
python3 -m http.server 8087 --bind 127.0.0.1 --directory .
```

Open `http://localhost:8087/docs/webgpu/index.html` in a WebGPU-capable browser.
The button runs validation and benchmarks and can export its JSON report.
The reference is single-threaded WASM; cross-origin isolation/pthreads are not
required by this standalone experiment. WebGPU needs HTTPS or localhost.

For repeatable automation, start an isolated Chrome profile with remote debugging:

```sh
chromium --headless=new --user-data-dir=/tmp/warlock-webgpu-chrome \
  --remote-debugging-port=9223 --enable-unsafe-webgpu \
  --enable-features=Vulkan --use-angle=vulkan --disable-vulkan-surface about:blank
node tests/webgpu/browser.mjs
```

Those flags were needed for this Linux headless test; they are not deployment
requirements for supported browsers. This machine used `flatpak run
com.google.Chrome` in place of `chromium`. The harness requires Node 22+ and no npm
packages. `CDP_URL`, `PAGE_URL`, `QUICK=1`, and `REPORT_PATH` can override defaults.
The default report path is `/tmp/warlock-webgpu-report.json`. Rebuild the WASM/module
artifacts after changing sources. Regenerate CPU fixtures after CPU mechanics changes.
The browser can run without `oracle.json`, but reports that legacy validation was skipped.

## Candidate API for later optimization

```js
import {GpuSimulator, defaults} from './runner.mjs';
const gpu = await GpuSimulator.create();
try {
  const result = await gpu.run([
    defaults,
    {...defaults, spellPower: 600},
    {...defaults, tapThreshold: 0.4},
  ], {perCandidate: 10000, seed: 42});
  console.log(result.summaries);
} finally {
  gpu.destroy();
}
```

For configurations already in Emscripten memory:

```js
const result = await gpu.runPacked(Module.HEAPF32, configPointer / 4,
                                  candidateCount, {perCandidate: 10000});
```

Callers must reacquire the current WASM heap view after memory growth. The runner
copies resolved inputs before awaiting GPU work. `fields` specifies the 20-float
input order; `combat_types.hpp` provides the C++ structure and size assertions.
All candidates in a call have the same replica count. Batches exceeding adapter
limits fail clearly and should be split by the caller. No optimizer is implied by
this API; it is the evaluation building block for one.

Next development should expand the combat model with independent CPU fixtures at
each step, then translate full builds/APLs into GPU inputs and integrate the browser
UI. Re-run both schedulers as state size, target count and event density grow.
Simulated annealing can then use independent chains or batched candidate evaluations;
one sequential chain alone does not expose parallelism across its dependent steps.

## API references

- [WGSL specification](https://www.w3.org/TR/WGSL/)
- [Chrome WebGPU compute overview](https://developer.chrome.com/docs/capabilities/web-apis/gpu-compute)
- [Chrome headless GPU testing](https://developer.chrome.com/blog/supercharge-web-ai-testing)
- [Emscripten WebGPU support](https://emscripten.org/docs/porting/multimedia_and_graphics/WebGPU-support.html)

## Follow-up: startup handling and matched batch scaling

The browser demo now defaults to event-driven mode. Fixed-timestep comparisons
are optional. The full benchmark runs in a dedicated module worker so CPU reference
runs cannot block the Stop button. Stop terminates that worker. Module loading,
adapter/device acquisition, shader validation, WASM loading, pipeline compilation,
GPU completion and readback have named progress stages and 30-second timeouts.
The page also watches for an unresponsive worker and reports startup import errors.
No stalled GPU request should leave an unexplained “Starting…” message forever.

The installed Firefox Flatpak 136.0.1 on this host did not expose `navigator.gpu`
in an isolated default profile. The new page reported “WebGPU unavailable” and
restored Run; see [the Firefox diagnostic](webgpu-firefox-136-startup.json).
This does not establish the cause of a hang in a different Firefox profile.
The adapter-stall case is separately covered using a deliberately unresolved API
promise. These changes diagnose unavailable browser support; they cannot provide
WebGPU in a browser that lacks it. Mozilla documents Linux support as experimental
in its [Firefox feature status](https://developer.mozilla.org/en-US/docs/Mozilla/Firefox/Experimental_features).
The functioning GPU measurements here use Chrome's AMD hardware adapter.

Enable “Measure 10,000 / 100,000 / 1,000,000 fights” to run the matched scaling test.
For each batch size, the event-driven GPU and reduced single-threaded WASM reference
receive the same 300.123-second haste configuration, warm up, and run three measured
samples. This follow-up used workgroup size 64 throughout:

| Fights | GPU median | WASM median | Speedup |
| ---: | ---: | ---: | ---: |
| 10,000 | 6.9 ms | 63.3 ms | 9.2x |
| 100,000 | 34.1 ms | 604.8 ms | 17.7x |
| 1,000,000 | 306.2 ms | 6,174.1 ms | 20.2x |

[Captured scaling report](webgpu-scaling-amd-rdna2-2026-09-30.json).
These are warmed browser measurements of the reduced slice, not the full combat
model. Timing noise and machine load account for differences from the first run.
The relative advantage increases as fixed overhead is amortized, then starts to
level off. It is not expected to grow without bound. Current readback copies 96
bytes per fight (96 MB at one million), and aggregation still runs on the CPU.
Compact outputs and GPU reductions are the next scaling improvements.

Startup regression and browser commands:

```sh
node tests/webgpu/startup.test.mjs
node tests/webgpu/cancel.mjs             # isolated Chrome on port 9223
COMPARE_SCHEDULERS=1 node tests/webgpu/browser.mjs
LARGE_BATCHES=1 node tests/webgpu/browser.mjs
node tests/webgpu/firefox.mjs            # isolated Firefox BiDi on port 9224
```

The Firefox script is a diagnostic and records either successful execution or the
reported failure. It does not enable experimental settings or modify a normal
browser profile.

## App integration sequence

1. **Asynchronous batch jobs.** The UI currently calls `RunnerType::run_batch`
   synchronously in `panel_sim_control.hpp` and `panel_target.hpp`; startup and
   preset changes also launch batches directly in `ui_app.hpp`. Route these through
   one job interface (`start`, progress, cancellation, completion/error) and return
   immediately to the render loop. The WASM app submits to a worker owning a
   persistent WebGPU device and pipeline cache, then polls/receives the result.
   CPU stays a selectable reference backend; unsupported GPU builds have an explicit
   reason and CPU fallback, never partial results presented as complete builds.
2. **Shared build preparation and a first complete preset.** Resolve gear, talents,
   buffs and encounter constants in C++/WASM, then pack a versioned GPU input.
   Extract reusable preparation rather than reproduce stat calculations in JS.
   Port a complete shadow preset, including ISB, curse, projectile timing and all
   required pet/buff interactions. Expand independent CPU fixtures with each mechanic.
   Unsupported spells, mechanics, targets and policies must be rejected until ported.
3. **Rotation execution and event storage.** Encode a bounded APL with shared action
   and condition identifiers. Keep decisions on GPU. Extend event storage to represent
   overlapping missiles, multiple DoTs and pet events without overwriting pending
   events. Establish tie ordering, cancellation/generation rules and overflow flags.
4. **Results and throughput.** Reuse GPU allocations; return compact per-fight DPS
   or GPU-reduced statistics, histogram and per-spell counts required by
   `BatchSimResult`. Mark unsupported result fields explicitly. Keep detailed CPU
   sample timelines as separately labeled examples. Chunk large requests with stable
   global replica IDs and progress/cancellation; choose chunk sizes by measured latency.
5. **Optimization after parity for supported builds.** Evaluate multiple candidates
   or independent annealing chains per GPU batch. Use consistent seed sets for
   comparisons, uncertainty-aware acceptance, and independent final validation.
   A single annealing chain cannot parallelize future dependent proposals merely
   by increasing the number of fight replicas.

The next concrete app milestone is one fully supported shadow preset runnable
through the normal Run button with CPU/WebGPU selection, responsive progress and
cancellation, and the existing result panels populated accurately. It is not yet
safe to route arbitrary current app builds through the restricted prototype.
