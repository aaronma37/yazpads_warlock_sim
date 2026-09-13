# Classic WoW Warlock Discrete Event Simulator & Multi-Threaded Optimizer

A high-performance **Discrete Event Simulator (DES)** written in C++20 for Classic World of Warcraft Warlocks. Built for extreme parallel execution, theoretical modeling, and brute-force optimization of specs, gear, rotation policies, and toggleable game mechanics (such as snapshotting).

Featuring **Raylib + Dear ImGui + ImPlot** for graphical visualization and a complete **Headless CLI mode** for batch execution and automated optimization sweeps.

---

## Key Highlights

- **Ultra-Fast Zero-Allocation DES Engine**:
  - Implements a flat binary min-heap priority queue with zero runtime dynamic memory allocations inside simulation loops.
  - Generates sub-millisecond event timing for cast completions, missile travel, periodic ticks, GCDs, cooldowns, and buff drops.
  - Powered by thread-local `xoshiro256**` PRNGs for zero mutex lock contention.
  - **Throughput**: Benchmarked at **~300,000 to 400,000 full fight simulations per second** across 8 threads.
- **Dual Mode**:
  - **GUI Mode**: Interactive window powered by Raylib, Dear ImGui, and ImPlot.
  - **Headless Mode**: High-throughput CLI runner with customizable iterations, duration, thread count, and JSON/CSV export.
- **Toggleable Game Mechanics (Classic vs Custom/Future Versions)**:
  - **Snapshotting**: Toggle between Classic WoW DoT snapshotting (stats captured at cast time) and Modern dynamic recalculation per tick.
  - **Spell Batching**: Configurable batching window (e.g. 400ms vs instant).
  - **Debuff Limit**: 16 slots (Patch 1.12), 8 slots (early vanilla), or unlimited slots.
  - **Partial Resists**: Classic 4-roll boss resistance tables vs binary hit/miss.
  - **ISB Rules**: All shadow damage sources (wands, SW:P) vs warlock direct spells only.
  - **Projectile Travel Time**: Configurable missile speed (24 yd/s) and boss distance.
- **Authentic World of Warcraft Character Armory & Paperdoll**:
  - Exact in-game WoW character sheet layout: 8 left equipment slots (Head to Wrists), 8 right slots (Hands to Trinket 2), and bottom weapons (Main Hand, Off Hand, Wand).
  - High-resolution 3D Warlock character model renders (Undead Male Warlock in Tier 2 Nemesis Raiment with Staff of the Shadow Flame, Orc Warlock in Tier 3 Plagueheart).
  - Authentic Blizzard icon assets for every equipment slot and classic item.
  - Interactive item tooltips with Epic/Rare quality borders, stat bonuses, set bonuses, and on-use effects.
  - Complete live character stats sheet: Base attributes (Health, Mana, Stamina, Intellect, Spirit, Armor), Spell combat stats (Effective Shadow/Fire damage, Hit %, Crit %, MP5), and Resistances.
- **Unified Non-Draggable Layout Flow**:
  - Replaced floating/movable windows with a fixed, anchored 2-pane dashboard:
    - **Left Pane**: Character Armory, paperdoll model, and character stats.
    - **Right Pane**: Tabbed workspace for Combat Simulation & ImPlot Charts, Talent Tree (51 points), Multi-Threaded Optimizer, and Mechanics Toggles.
- **Analysis & Visualizations (ImPlot)**:
  - **DPS Frequency Histogram**: Bell curve distribution with Mean, Median (P50), P5, and P95 confidence intervals.
  - **Combat Timeline**: Mana curves and individual spell impact events over time.
  - **Comparison Graphs**: Side-by-side spec and gear bar charts with error bars (StdDev) and ranking tables.
  - **Damage Breakdown**: Percentage shares for Shadow Bolt, Corruption, Curses, Immolate, and Shadowburn.
- **Multi-Threaded Brute-Force Optimizer**:
  - Automatically sweeps across talent trees, gear combinations, and rotation policies.
  - Computes the exact DPS delta between game mechanic variations (such as Snapshotting ON vs OFF).
  - One-click "Apply Best Configuration" button to instantly adopt winning loadouts.

---

## Project Structure

```
warlock_sim/
├── CMakeLists.txt              # CMake build file
├── build.sh                    # High-speed optimized build script (-O3 -march=native)
├── bin/
│   └── warlock_sim             # Compiled standalone executable
├── src/
│   ├── main.cpp                # Dual-mode entry point (GUI / Headless)
│   ├── sim/
│   │   ├── des_engine.hpp      # Zero-allocation Event Queue & fast xoshiro256** PRNG
│   │   ├── stats.hpp           # Combat attributes, spell power, hit, crit, haste
│   │   ├── spells.hpp          # Spell definitions (Shadow Bolt, Corruption, Immolate, Life Tap, etc.)
│   │   ├── talents.hpp         # 51-point talent trees and calculators
│   │   ├── gear.hpp / .cpp     # Equipment slots, Item database, Phase BiS presets
│   │   ├── buffs.hpp           # Raid buffs, consumables, world buffs, and target debuffs
│   │   ├── mechanics.hpp       # Toggleable game rules (snapshotting, resists, batching, ISB)
│   │   ├── policy.hpp          # Action Priority List (APL) / combat rotation rules
│   │   ├── warlock_sim.hpp/.cpp# Complete Discrete Event Simulation implementation
│   │   ├── parallel_runner.hpp/.cpp # Multi-threaded parallel batch execution
│   │   └── optimizer.hpp / .cpp# Brute-force & heuristic parameter optimizer
│   └── ui/
│       ├── ui_theme.hpp        # Warlock shadow/fel aesthetic styling
│       ├── panel_sim_control.hpp# Run simulation, duration, threads, raid buffs
│       ├── panel_talents.hpp   # Interactive talent trees and preset allocator
│       ├── panel_gear.hpp      # Slot-by-slot gear viewer and item picker
│       ├── panel_mechanics.hpp # Toggleable mechanics panel
│       ├── panel_policy.hpp    # Combat rotation policy settings
│       ├── panel_results.hpp   # ImPlot DPS histogram, timeline, damage breakdown
│       ├── panel_comparison.hpp# ImPlot comparison bar charts and leaderboard
│       ├── panel_optimizer.hpp # Brute-force optimizer UI
│       └── ui_app.hpp          # Main GUI coordinator
└── third_party/
    ├── raylib/                 # Raylib 5.0 (headers & static library)
    ├── imgui/                  # Dear ImGui
    ├── implot/                 # ImPlot plotting library
    └── rlImGui/                # Raylib ImGui backend integration
```

---

## Building

### Quick Build with script:
```bash
./build.sh
```

### Build with CMake:
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

---

## Running

### 1. Graphical Mode (GUI)
Simply launch without arguments:
```bash
./bin/warlock_sim
```
*(If `$DISPLAY` or `$WAYLAND_DISPLAY` is not set, it automatically falls back to headless CLI mode).*

### 2. Headless CLI Mode

#### Single Batch Simulation:
```bash
./bin/warlock_sim --headless --iterations 20000 --threads 8 --duration 120
```

#### Brute-Force Talent Build Sweep:
```bash
./bin/warlock_sim --headless --optimize-talents --iterations 5000
```

#### Gear Progression Sweep:
```bash
./bin/warlock_sim --headless --optimize-gear --iterations 5000
```

#### Snapshotting Impact Study:
```bash
./bin/warlock_sim --headless --snapshotting-study --iterations 5000
```

#### Custom Spec, Gear, and JSON Export:
```bash
./bin/warlock_sim --headless \
    --spec ds_ruin \
    --gear p5 \
    --snapshotting 1 \
    --iterations 50000 \
    --json results.json
```

---

## CLI Options

| Flag | Description | Default |
|------|-------------|---------|
| `--headless` | Run in headless CLI mode | Auto-detected |
| `--iterations <N>` | Number of fight simulations | 10000 |
| `--duration <seconds>` | Fight length in seconds | 120.0 |
| `--threads <N>` | Number of worker threads | All hardware cores |
| `--spec <name>` | Spec preset (`ds_ruin`, `sm_ruin`, `fire_destro`, `md_ruin`) | `ds_ruin` |
| `--gear <name>` | Gear preset (`preraid`, `p3`, `p5`, `p6`) | `p3` |
| `--snapshotting <0\|1>`| Toggle DoT snapshotting (1: Classic, 0: Modern) | 1 |
| `--optimize-talents` | Run brute-force talent optimization sweep | - |
| `--optimize-gear` | Run gear progression optimization sweep | - |
| `--optimize-policy` | Run combat policy optimization sweep | - |
| `--snapshotting-study` | Run comparative snapshotting impact study | - |
| `--json <path>` | Export simulation metrics to JSON | - |
| `--help` | Display help message | - |
# yazpads_warlock_sim
