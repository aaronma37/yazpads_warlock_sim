# Three.js GPU Discrete Event Simulation (DES) Engine

A high-performance browser-based Discrete Event Simulator (DES) prototype built with **Three.js** and **WebGL GPGPU compute shaders**.

---

## 🚀 Key Features

1. **True GPU-Accelerated Discrete Event Scheduler**
   - Implements variable-step event queue resolution directly inside GLSL fragment shaders (`min(ready, castEnd, dotNext, agonyNext, doomNext, siphonNext, immolateNext, wrackNext, regenNext, trinketReady)`).
   - Time advances non-linearly directly to the timestamp of the next pending event (`s.now = next_now`), eliminating fixed-interval polling or wasted sub-microsecond empty ticks.
   - Handles multi-event chunks (32 to 128 event steps per draw dispatch) on GPU without CPU round-trips.

2. **100 Concurrent Batch Configurations**
   - Packs 100 distinct character configurations into a dynamic 2D DataTexture (12 `vec4` texels / 48 floats per config) across varying gear tiers, talents (Affliction, Destruction, Ruin, Demonology, Speed), haste scalings, APL priority rules, and fight lengths.
   - All 100 configurations simulate in parallel across GPU threads with isolated LCG PRNG state streams.

3. **Interactive 3D Visualizer & HUD in Three.js**
   - 3D Grid Arena displaying 100 combatant nodes color-coded by talent specialization.
   - Real-time height scaling and dynamic glowing particle rings driven by GPU DPS output.
   - Mouse OrbitControls and Raycasting (click any 3D node to inspect that combatant's live telemetry, APL, and damage breakdown).

4. **Performance & Parity Benchmarking**
   - Real-time telemetry: GPU execution time (ms), total discrete events processed, event throughput (Millions of events/sec), and simulation time-warp factor.
   - Built-in JavaScript CPU Reference DES runner for 1:1 parity checks and speedup measurement.
   - Export full simulation results to JSON.

---

## 🛠️ How to View & Run the Simulation

To launch the local web server:

```bash
cd /home/deck/warlock_sim/threejs_des_sim
python3 -m http.server 8080
```

Open your browser to:
👉 **`http://localhost:8080`**

### Controls & Actions
- **`▶ Run GPU Simulation`**: Triggers full batch execution on GPU and updates the 3D scene & analytics.
- **`⚡ CPU Benchmark`**: Runs the identical 100-config DES on the CPU to report speedup factor and parity.
- **`🎲 Randomize 100`**: Regenerates a fresh diverse set of 100 configurations.
- **`Batch Selector`**: Test performance across 10, 25, 50, or 100 concurrent configurations.
- **`3D Viewport`**: Click and drag to rotate the camera, scroll to zoom, click any node to inspect.

---

## 🔍 Shortcuts & Technical Design Decisions

In compliance with the project requirements, the following implementation choices were made:

1. **WebGL Float Texture GPGPU vs WebGPU Compute**:
   - *Design*: Three.js supports WebGL2 GPGPU via floating-point ping-pong `WebGLRenderTarget` textures. This was chosen over experimental WebGPURenderer compute shaders to guarantee universal zero-install compatibility across all browser engines (Chrome, Firefox, Safari, Edge, Android/iOS) without requiring experimental flags.
2. **Timestamp Representation**:
   - *Design*: GLSL uses single-precision float seconds with fractional microsecond precision (`now = 12.345s`) instead of 64-bit integer microseconds (`uint64`). Float32 mantissa supports exact microsecond resolution up to fight durations of ~300+ seconds.
3. **Random Number Generation**:
   - *Design*: Uses a deterministic Linear Congruential Generator (LCG) in GLSL seeded per texel coordinates to ensure fast, branchless PRNG on the GPU.
4. **Readback Strategy**:
   - *Design*: Texture readback via `gl.readPixels` / `readRenderTargetPixels` is batched at the conclusion of the simulation chunk run to avoid CPU-GPU pipeline stalls during stepping.
