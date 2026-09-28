# Installation

## Training recurrent policies on CPU

The Warlock **Train Policies** tab uses the vendored [rl-tools](https://github.com/rl-tools/rl-tools)
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

Clone the repository with submodules initialized:

```bash
git clone --recurse-submodules https://github.com/aaronma37/yazpads_warlock_sim.git
cd yazpads_warlock_sim
```

If already cloned without `--recurse-submodules`, initialize them:
```bash
git submodule update --init --recursive
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
