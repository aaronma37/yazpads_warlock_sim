# Installation

## Experimental WebGPU simulator

A browser experiment compares event-driven GPU simulation with 1/10/50 ms fixed
steps in batches of 10,000 fights. It implements a restricted Warlock combat slice
and retains the existing CPU simulator as an independent reference. See the
[measured results, supported mechanics, and run instructions](docs/research/webgpu.md).
Build the standalone demo with `./scripts/build_webgpu.sh`.

The full C++ parity pipeline also runs in a browser through WebGPU. Build the
WASM app with `./scripts/build_desktop_wasm.sh`, then serve `docs/` over
localhost or HTTPS and open `webgpu/pipeline.html`. Choose **Spec presets** to
run the browser equivalent of `./bin/webgpu_parity_pipeline --presets`; it
executes both the C++ CPU reference and WebGPU kernel and lets you download the
JSON and Markdown reports. The browser build uses Asyncify to wait for GPU
completion and readback.

## Training recurrent policies on CPU

The **PPO + GRU** approach in the Warlock **Train Policies** tab uses the vendored [rl-tools](https://github.com/rl-tools/rl-tools)
CPU implementation of clipped PPO with separate 32-unit GRU actor and critic networks.
It trains directly on damage from the combat simulator; it does not use VIPER,
oracle labels, decision-tree distillation, or an imitation-learning dataset.

Configure your character and encounter, then select **Train new policy**. Training
runs in the background with progress and reward history. **Stop** finishes the
current PPO update. **Evaluate policy vs captured APL** compares deterministic
recurrent inference against the original configuration on a separate seed list.
**Save policy** and **Load policy** persist actor weights in a versioned TAR file.
Loading uses the current character configuration; the file does not contain the
character build or a resumable optimizer state. Training a new policy starts from
random weights. Policies are evaluated in this tab and do not replace the APL in
the main simulator.

Algorithm details:

- Four environments collect 128 consecutive decisions each. Rewards are damage
  increments divided by 1,000, including final damage at episode termination.
- PPO uses GAE (gamma 0.99, lambda 0.95), a 0.2 probability-ratio clip, normalized
  advantages, learned Gaussian standard deviations, entropy coefficient 0.001,
  and Adam. Four full-sequence optimization passes reuse each rollout's original
  actions, log probabilities, advantages and value targets. The upstream
  recurrent trainer permits one epoch per call, so we invoke it four times.
- Sequences are never shuffled. GRU memory resets at episode boundaries and the
  start of each rollout; backpropagation runs through the sequence with reset
  masks. Encounters exceeding 128 decisions are truncated and bootstrapped, then
  restarted for the next rollout. This limits learning of late phases in longer
  encounters. Prefer encounters that fit within this horizon.
- This is continuous-action PPO: a 19-dimensional Gaussian preference vector
  ranks combat rules, and the simulator executes the first admissible rule.
  PPO likelihoods refer to the complete sampled vector, not to a categorical
  distribution over spells. Existing spell eligibility, pets, multi-target
  upkeep, consumables and cooldown automation remain part of the environment.
- Observations contain 24 scaled combat features. A policy is trained for the
  captured build, rather than conditioned on arbitrary gear and talents.

Run `ctest --test-dir build --output-on-failure` after building to verify the
training update, GRU parameter changes, deterministic evaluation, checkpoint
round-trip, cancellation and independence from the oracle controller.

## Training a C++ GBDT policy from search

Select **C++ GBDT + search imitation** in **Train Policies**. This uses the existing
native C++ LightGBM-style trainer; no Python or external ML library is required.
The teacher replays the exact fight history and RNG up to each decision, then
resamples only future outcomes. Candidate labels are accepted only when the
simulator actually executes the requested root action. Training distills relative
action values (candidate DPS minus best candidate DPS), with weights reflecting
the action gap and sampling uncertainty. Whole episodes are held out for label
validation. Later DAgger rounds aggregate labels from learner-visited states.

The controls let you trade compute against search quality and model capacity:

- **Future rollouts / action** controls the training teacher's budget.
- **Search depth** 1 compares actions with APL/learner continuation. Depth 2+
  uses open-loop UCT MCTS over future action preferences, adding one node per
  trial before continuing with the APL/learner. Stochastic outcomes share tree
  nodes, while actual execution checks resources and cooldowns.
- **UCT exploration** balances exploration and estimated return; returns are
  scaled as DPS / 1,000. Deeper trees require larger rollout budgets.
- **Reference rollouts / action** independently controls the evaluation teacher.
- **DAgger rounds**, training fights, label stride/cap and teacher mix control
  data coverage. Raise the cap to include late-fight decisions. Mix 0 collects
  learner trajectories after the initial teacher-assisted round.
- Boosting iterations, maximum depth, minimum leaf weight, learning rate and
  L2 regularization tune the native per-action GBDT models.
- **Training threads** sets one shared worker budget; `0` uses hardware threads.
  Collection fights, root-action searches, held-out evaluation and per-action
  GBDT fitting share this pool. Each MCTS tree keeps its sequential rollout order,
  and episode results are merged deterministically. The panel reports actual
  thread count, fights/search replays per second and collection/fit/evaluation
  timings. Small datasets may not keep every thread busy during fitting.

Each round evaluates complete fights on a separate seed list and plots policy,
APL and online search DPS. Results include policy/search ratio, an approximate
paired confidence interval for their DPS difference, teacher-state agreement,
action regret and episode-held-out label accuracy. The teacher uses the current
learner beyond the tree horizon, so its reference can change across rounds.
Search is a finite-budget empirical reference, not a proven upper bound or a
guarantee of global optimality. Increase reference budget/depth and repeat with
fresh seeds to assess whether the reference and learned policy stabilize.

**Save GBDT bundle** writes a new directory containing `policy.gbdt` (portable
C++ inference with feature/action schema), `build.json`, `config.json`,
`samples.csv`, completed-round `metrics.csv`, and `deployment.json` with the
captured build and embedded trained model. Cancellation preserves the last
fitted model. Workers check cancellation between decisions, search rollouts and
boosting iterations, then join before training stops.
After training, click **Use trained policy in current configuration** to activate
an immutable model snapshot for normal simulations. **Load bundle / policy and
activate** loads a saved bundle directory or `policy.gbdt`. The **Policy** tab
also provides loading and an **Active controller** selector to switch between
the existing APL and the loaded trained GBDT without discarding either.

Run a normal simulation, then open **Observed Spell Cast Sequence** to see the
casts, or **GBDT Decision Trace** to inspect each decision's combat state,
ranked model scores and actual executed action. The trace belongs to the sample
fight recorded with those results, even if you subsequently switch controllers.

Configuration exports embed the model, its name and enabled state under
`policy.trained_gbdt`, so the configuration does not depend on an external model
file. The CLI also accepts `--gbdt-policy <bundle-or-file>`, and JSON configuration
can reference a bundle/file with `policy.trained_gbdt.path` (relative paths resolve
from the configuration file's directory). For example:

```bash
bin/warlock_sim --headless --config imitation_policy/deployment.json --iterations 1000
bin/warlock_sim --headless --config character.json --gbdt-policy imitation_policy
```

## Prerequisites

### Linux
- **C++20 compiler** (GCC 11+, Clang 13+)
- **CMake** (>= 3.20)
- **Development libraries**: OpenGL, X11, pthread

```bash
# Debian / Ubuntu
sudo apt-get update && sudo apt-get install -y build-essential cmake libgl1-mesa-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev

# Fedora / RHEL
sudo dnf install -y gcc-c++ cmake mesa-libGL-devel libX11-devel libXcursor-devel libXinerama-devel libXrandr-devel libXi-devel

# Arch Linux
sudo pacman -S --needed base-devel cmake libgl xorg-server-devel
```

### Windows
- **Visual Studio 2022** (with *Desktop development with C++*) or **MSYS2 / MinGW-w64** (GCC 11+)
- **CMake** (>= 3.20)
- **Git**

*(Alternatively, you can build and run seamlessly via **WSL2** using the Linux instructions).*

---

## Clone Repository

Clone the repository with submodules initialized. This includes the required
`rl-tools` dependency used by the recurrent PPO training code:

```bash
git clone --recurse-submodules https://github.com/aaronma37/yazpads_warlock_sim.git
cd yazpads_warlock_sim
```

If already cloned without `--recurse-submodules`, initialize the submodules
(including `rl-tools`):
```bash
git submodule update --init --recursive
```

To install only the PPO dependency in an existing checkout:
```bash
git submodule update --init --recursive third_party/rl-tools
```

---

## Build Instructions

### Linux / macOS / WSL2

1. **Build the Raylib static library:**
   ```bash
   cd third_party/raylib/src
   make PLATFORM=PLATFORM_DESKTOP
   cd ../../..
   ```

2. **Configure and compile with CMake:**
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . -j$(nproc)
   ```

---

### Windows

#### Option A: Visual Studio / MSVC (Developer Command Prompt / PowerShell)

1. **Build Raylib with CMake:**
   ```cmd
   cd third_party\raylib
   mkdir build && cd build
   cmake -DPLATFORM=Desktop -DBUILD_EXAMPLES=OFF ..
   cmake --build . --config Release
   copy /Y raylib\Release\raylib.lib ..\src\raylib.lib
   cd ..\..\..
   ```

2. **Build the simulator:**
   ```cmd
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --config Release
   ```

#### Option B: MSYS2 (MinGW 64-bit)

```bash
# Inside MSYS2 MINGW64 shell:
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make git

cd third_party/raylib/src
make PLATFORM=PLATFORM_DESKTOP
cd ../../..

mkdir -p build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

---

## Binaries

Compiled binaries will be located in the `bin/` directory:
- `bin/warlock_sim` (or `bin/Release/warlock_sim.exe` on MSVC)
- `bin/sim_tests` (or `bin/Release/sim_tests.exe` on MSVC)
