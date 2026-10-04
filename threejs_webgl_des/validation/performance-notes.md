# Diagnostic performance investigation

Measured 2026-10-04 on Linux x86_64 (8 logical CPUs), Chrome with the benchmark's existing forced SwiftShader backend. These are end-to-end rates, including decoding, summaries, and the default event trace. SwiftShader is a software renderer; these numbers are not hardware GPU measurements.

The baseline is the working tree at the start of this investigation, including the uncommitted optional-diagnostics implementation. It is not a checkout of the parent of `88081fcc`.

| Fights | Before, full diagnostics | After, full diagnostics | After, fast mode |
|---:|---:|---:|---:|
| 100,000 | 56,465/s | 130,548/s | 348,311/s |
| 250,000 | 72,685/s | 155,589/s | 455,290/s |
| 500,000 | 78,361/s | 186,005/s | 506,791/s |
| 1,000,000 | 77,634/s | 209,411/s | 561,356/s |

Runs vary with warmup and system load. Fast-mode measurements came from a separate run with identical batch sizes. No simulation mechanics or shader calculations were changed by this optimization.

## Findings

- Detailed decoding copied all compact words into a new typed array for every fight, then appended spell fields dynamically. Direct attachment reads and a complete object literal remove those copies and keep a stable object shape.
- Fast decoding allocated an array, typed arrays, a closure, temporary objects, and object spreads for every fight. Shared attachment views and a single state literal remove that overhead.
- Summary generation repeatedly scanned the whole state array with dynamic property access for each counter. A single pass with explicit field access substantially reduces that cost.
- Numeric typed-array sorting removes the JavaScript comparator overhead while preserving the returned DPS array.
- The detailed compact payload is 53 words, requiring four 16-word stripes (previously two). Each stripe reruns the fight. That rendering cost remains, but JavaScript processing was the dominant remaining bottleneck.

## Final detailed timing at one million fights

- `executeMs`: 3524.5 ms
- `decodeMs`: 475.4 ms
- `summaryMs`: 716.6 ms
- `diagnosticMs`: 53.3 ms
- `elapsedMs`: 4775.1 ms

## Reproduce

```sh
python3 scripts/benchmark_sim.py --gpu-only --scales 100000 250000 500000 1000000 --json
python3 scripts/benchmark_sim.py --gpu-only --fast --scales 100000 250000 500000 1000000 --json
```

`--json` now appends actual result data, including execution, decoding, summary, and replay timings. Benchmark browser processes are terminated after each run.

## Correctness

- Browser comparison against the saved original engine: all 22 presets, cycling through all five races, had exactly equal states, summaries, and traces in both detailed and fast modes.
- `accounting.js` checks all 13 spell summaries against independent reductions, detailed/fast parity and traces, texture-row and partial-batch boundaries, mixed configurations, and incomplete-fight rejection. All six checks passed in Chrome/SwiftShader.
- The accounting checks are also included in `validation/browser.mjs` for subsequent browser-suite runs. The complete existing Playwright suite was not run in this environment.
