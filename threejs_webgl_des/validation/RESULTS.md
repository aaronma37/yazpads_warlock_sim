# WebGL2 validation results

Validated on 2026-10-01 (America/Los_Angeles) in this workspace.

| Check | Chromium 134, Linux | Firefox 135, Linux |
| --- | --- | --- |
| Actual C++ reference fixtures | 40 / 40 | 40 / 40 |
| Batch-size invariance, 65 fights across partial batches | Pass | Pass |
| Repeatability | Pass | Pass |
| Five consecutive seeds compared with C++ across batch boundaries | Pass | Pass |
| Incomplete event-budget rejection | Pass | Pass |
| Cancellation after a submitted batch | Pass | Pass |
| UI renders results and enables JSON export | Pass | Pass |
| 390px viewport has no page overflow | Pass | Pass |
| Uncaught page errors | 0 | 0 |

WebGPU was disabled in the browser launch configuration. The test also installed a throwing `navigator.gpu` getter: accessing WebGPU would fail the run. Both browsers executed the Three.js WebGL2 shader.

Three Node contract checks passed: invalid/unsupported inputs, CPU raw-stat and integer-clock packing, and reference-source hash freshness.

The 40 reference cases cover the supported core spells, ranks, charged/timed ISB, Nightfall, resistance/piercing, hardcast/instant Corruption, projectile overlap, resource fallback, crit/hit extremes, end-event ties, a 1,800-second fight, and multiple seeds. This validates the supported slice, not the full CPU simulator. See the README for exact precision tolerances and excluded mechanics.

Browser execution was headless; Chromium used SwiftShader. These results establish functional coverage, not a hardware throughput claim. Mobile viewport checks are layout checks, not real-phone GPU tests. No WebGL-versus-WebGPU performance ratio was measured.

`browser.mjs` reproduces the checks and writes full JSON reports and desktop/mobile screenshots into the ignored `artifacts/` directory. Fixtures include CPU source hashes. Re-run after changing shader logic, supported mechanics, or the CPU reference.
