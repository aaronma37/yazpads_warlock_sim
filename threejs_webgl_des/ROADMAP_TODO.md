# Three.js WebGL Simulation Lab: Roadmap & TODO Checklist

This roadmap outlines the plan for migrating the native desktop/WASM simulation features into a modern, responsive, mobile-friendly Three.js WebGL web application deployable on **GitHub Pages**.

Additional character class support is tracked in [Multi-Class Goal & TODO](MULTICLASS_TODO.md), including class modules, UI and GPU boundaries, saved-build compatibility, and validation of a first additional class.

---

## 🎯 High-Level Objectives

1. **Native Web & Mobile UI**: Replace the desktop ImGui layout with responsive CSS Grid/Flexbox components, mobile touch-friendly controls, interactive talent tree widgets, and bundled texture assets.
2. **Full Engine Parity**: Achieve bit-level / epsilon-bounded parity against the native C++ CPU DES model across standard specs and advanced mechanics.
3. **Compare Standard Presets**: Deliver a high-throughput multi-preset comparison dashboard evaluating dozens of meta specs simultaneously on the GPU.
4. **GitHub Pages Deployment**: Pure static bundle ready for single-click CI/CD deployment and mobile PWA installation.

---

## Phase 1: Native Web Layout & Mobile-First Widgets

- [x] **1.1 Responsive Layout Shell**
  - [x] Replace ImGui desktop windows with a responsive CSS Grid / Flexbox layout that automatically adapts to desktop, tablet, and mobile screen sizes.
  - [x] Build touch-friendly collapsibles/accordions, bottom-sheet drawers, and tab navigation.
  - [x] Add dark-mode aesthetic matching the classic Warlock interface.

- [x] **1.2 Interactive Talent Tree Widget**
  - [x] Port the 3-tree Warlock talent system (Affliction, Demonology, Destruction) to pure DOM/SVG/Canvas (`src/talents.js`, `data/talents.json`).
  - [x] Bundle and render talent icon textures with rank badges, prerequisite arrows, and point counters.
  - [x] Implement desktop right/left-click point allocation and mobile-friendly tap-to-allocate controls.
  - [x] Add talent preset switching (DS/Ruin, SM/Ruin, Deep Affliction, Fire Destro) and dynamic simulation flag synchronization.

- [x] **1.3 Gear, Consumables & Buff Selectors**
  - [x] Port consumable selection (Flask of Supreme Power, Greater Arcane Elixir, Shadow Power, Wizard Oil, etc.) with automatic stat aggregation (`src/buffs.js`).
  - [x] Integrate raid buffs (Arcane Intellect, Mark of the Wild, Blessing of Kings) and world buffs (Songflower, Dragonslayer).

- [x] **1.4 High-Performance Charting & Visualizations**
  - [x] Responsive DPS distribution histogram (Canvas / SVG).
  - [x] Interactive chronological event timeline with scrubbable combat log.
  - [x] Real-time confidence interval and percentile badges ($p_{05}, p_{50}, p_{95}$).

---

## Phase 2: Complete Simulation Parity with CPU DES Model

- [x] **2.1 Extended Mechanic & Spell Pipeline in GLSL**
  - [x] Expand the fragment shader kernel to support additional talents & procs (*Demonic Sacrifice*, *Master Demonologist*, *Shadow and Flame*, *Evolving Affliction*, *Firestone/Spellstone*).
  - [x] Add pet combat simulation state (Imp Firebolt, Succubus Lash of Pain, pet mana/scaling).
  - [x] Add active trinket usage (Neltharion's Tear, MQG, ZHC, ToEP) and potion/mana gem scheduling.

- [x] **2.2 Parity Validation Pipeline**
  - [x] Expand `validation/cpu-fixtures.json` to cover multi-buff interactions, pet contributions, and execute-phase mechanics.
  - [x] Ensure bit-level / epsilon-bounded parity against the native C++ oracle for all standard spec presets.

---

## Phase 3: Standard Presets & "Compare Presets" Dashboard

- [x] **3.1 Preset Data Ingestion**
  - [x] Package standard meta presets (`warlock_specs_comparison/*.json`: Deep Affliction, DS/Ruin, SM/Ruin, Fire Incinerate, Shadow & Flame, etc.) as client-side static JSON fixtures (`data/presets.json`).

- [x] **3.2 Multi-Preset Batch Execution Engine**
  - [x] Implement a batch runner in `src/presets.js` that evaluates meta spec configurations sequentially or concurrently across GPU simulation batches.

- [x] **3.3 Preset Comparison View & UI**
  - [x] Build the **"Compare Standard Presets"** tab with sorted leaderboard tables (Mean DPS, Max DPS, Variance, Range).
  - [x] Box-plot / distribution visualizer comparing DPS ranges across specs simultaneously.
  - [x] Spec delta inspector showing side-by-side stat differences and DPS deltas between any two selected specs.

---

## Phase 4: GitHub Pages Deployment & Build CI

- [x] **4.1 Asset Bundling & Optimizations**
  - [x] Bundle all icon textures into `assets/icons/`.
  - [x] Configure GitHub Actions workflow ([`.github/workflows/deploy-pages.yml`](file:///home/deck/warlock_sim/.github/workflows/deploy-pages.yml)) to publish the web app to GitHub Pages on commit to `main`.

- [x] **4.2 Offline / PWA Support**
  - [x] Add a Service Worker ([`sw.js`](file:///home/deck/warlock_sim/threejs_webgl_des/sw.js)) and Web App Manifest ([`manifest.webmanifest`](file:///home/deck/warlock_sim/threejs_webgl_des/manifest.webmanifest)) so users can install it as a standalone mobile app on iOS/Android and use it offline.

---

## Phase 5: Desktop App UI & Texture Parity

- [x] **5.1 WoW Classic Theme & Textures Ingestion**
  - [x] Bundle high-resolution talent backdrops (`affliction_bg.png`, `demonology_bg.png`, `destruction_bg.png`) into web distribution.
  - [x] Port classic Blizzard frame borders, parchment panels, and red bevel button textures (`assets/wow_classic/Buttons/UI-Panel-Button-*`, `DialogFrame/UI-DialogBox-*`).
  - [x] Port Blizzard tab styles (`assets/wow_classic/Tabs/UI-Tab-*`) and gold header plaques.
  - [x] Implement authentic WoW tooltips with golden borders and dark translucent backing.
  - [x] Load authentic Blizzard fonts matching the desktop app: Friz Quadrata (`FRIZQT__.TTF`) and Arial Narrow (`ARIALN.TTF`).
  - [x] Fix case-sensitive icon resolution across all talent nodes, item slots, and action priority icons (33,000+ icon aliases).

- [x] **5.2 Interactive Paperdoll & Compare Specs Layout Parity**
  - [x] Build the interactive character paperdoll layout with 17 equipment slot frames (Helm, Neck, Shoulders, Back, Chest, Wrists, Hands, Waist, Legs, Feet, Rings, Trinkets, Main Hand, Off Hand, Wand) matching `panel_gear.hpp`.
  - [x] Implement slot hover tooltips, item stat aggregation, and BiS gear preset selection (`Pre-Raid`, `Phase 3/4`, `Phase 5`, `Phase 6 BiS`).
  - [x] Build direct 3-column desktop app layout matching `ui_app.hpp` (Armory & Direct Stats, Buffs Summary, Talents & Sim Results).
  - [x] Build Compare Standard Specs leaderboard matching desktop ImGui app screenshot:
    - Settings bar with number of simulations and WebGL2 GPU engine indicator.
    - Base stats subheader with Shadow/Fire SP, Hit %, Crit %, MP5, and Pet scaling.
    - Results table with Rank, Spec Name, Race portrait, Pet/Sac icons, Action Priority Chain, segmented Damage Split bar, and Mean DPS.
    - Selected row details panel with expanded Action Priority Chain, detailed Damage Breakdown progress bars, and Opener Cast Sequence with chronological timestamps.
  - [x] Flicker-free talent allocation engine with in-place DOM updates.

- [x] **5.3 APL Priority Customizer & Desktop Layout Parity**
  - [x] Implement customizable Action Priority List (APL) priority table with drag-and-drop reordering, up/down move buttons, and enable/disable toggles matching `panel_policy.hpp`.
  - [x] Condition expression editor modal and right-click popover for APL rules.
  - [x] Interactive Race, Pet, and Demonic Sacrifice (DS) slot buttons with authentic popover pickers and active racial trait badges.
  - [x] Authentically textured accordions with Blizzard `UI-PlusButton-Up` and `UI-MinusButton-Up` button icons.
  - [x] Tree-level `[✕]` talent reset buttons on tree headers in addition to global reset.
  - [x] Side-by-side Sim Configuration and Simulation Results layout with segmented damage split bars and per-spell damage statistics.

---

## Phase 6: High-Fidelity WebGL vs CPU DES Parity Pipeline (CRITICAL)

- [x] **6.1 Full Spec Presets & APL Parity Suite**
  - [x] Port the complete matrix comparison concept from `webgpu_parity_pipeline` / `parity_comparator.hpp` to WebGL2 without any WebGPU dependency (`scripts/compare_webgl_cpu.py`).
  - [x] Execute all standard spec presets (`data/presets.json`: Deep Affliction, DS/Ruin, SM/Ruin, Fire Destro, DP Brand, Shadow and Flame, etc.) across both WebGL2 GPU and native C++ CPU DES simultaneously.
  - [x] Rigorously validate dynamic Action Priority List (APL) execution chains, trigger conditions, and fallback priority branches against the CPU engine.

- [x] **6.2 Deep Damage Breakdown & Diagnostic Inspector**
  - [x] Compare full per-spell damage share breakdown (Shadow vs Fire vs Pet, and individual spells: Shadow Bolt, Corruption, Immolate, Searing Pain, Incinerate, Conflagrate, Siphon Life, Drain Soul, Pet spells).
  - [x] Compare granular spell metrics: cast counts, hit counts, crit counts, miss counts, and periodic tick counts against the CPU DES ground truth.
  - [x] Implement parameter sweeps (Spell Power, Spell Hit, Spell Crit, and Fight Duration sweeps) comparing WebGL vs CPU scaling curves.
  - [x] Generate automated parity reports with Markdown / JSON diff matrices and visual comparison charts (`--markdown parity_report_presets.md`).

- [x] **6.3 Customizable Shader APL Architecture: Table-Driven Bytecode VM (EXPLICITLY MANDATED)**
  - > [!IMPORTANT]
  - > **Architectural Requirement**: We **must** use **Approach B (Table-Driven APL Bytecode VM)**. Approach A (JIT GLSL Shader Generation) hardcodes one APL per shader compilation and cannot evaluate multiple distinct/mutated APLs concurrently across lanes in a single GPU draw call. Only Approach B satisfies our core requirements for multi-spec comparisons, genetic APL optimization, and instant interactive rule tweaking.
  - [x] **Table-Driven APL Bytecode VM Architecture**:
    - [x] Encode priority rules into fixed 2-word GPU structs (`header_word` for action ID & condition predicate type, `param_word` for threshold values like remaining duration, health %, or stack count) stored in `configTex` or uniform buffers.
    - [x] Each GPU lane reads its own allocated APL bytecode table from `configTex`, allowing thousands of distinct, mutated, or user-customized APL candidate profiles to execute simultaneously in a single GPU draw call with zero shader recompilation overhead.
    - [x] Implement the bytecode evaluation loop in GLSL `decide()`:
      - [x] Action definitions (Life Tap, Nightfall Shadow Bolt, Decimation Searing Pain, Decimation Soul Fire, Demonic Brand, Corruption, Curse of Doom, Curse of Agony, Immolate, Conflagrate, Shadowburn, Incinerate, Searing Pain, Shadow Bolt).
      - [x] Condition predicates (`ALWAYS`, `MANA_LE`, `MANA_GE`, `TARGET_HP_LE`, `TARGET_HP_GE`, `DOT_REM_LE`, `FIGHT_TIME_GE`, `FIGHT_TIME_LE`, `SHADOW_TRANCE`, `DECIMATION_ACTIVE`, `DECIMATION_INACTIVE`, `DEMONIC_BRAND_MISSING`, `DOOM_MISSING`, `ISB_ACTIVE`).
- [x] **Approach Comparison & Rationale**:
    - *Approach A (JIT GLSL Shader Recompilation)*: **INSUFFICIENT** — requires compiling a new shader program for each APL variant, restricting each draw call to a single uniform APL across all threads.
    - *Approach B (Table-Driven VM)*: **REQUIRED & IMPLEMENTED** — enables arbitrary per-lane APL dispatch, batch preset evaluation, and genetic algorithm APL evolution in real time.

---

## Current Build Integration Gaps

- [~] **Match CPU action and cast timing.** The WebGL config now carries the preset's Bane and Decimation ranks, and the kernel derives Shadow Bolt, Immolate, Incinerate, and Soul Fire cast durations using the CPU formulas. The 22 standard presets showed no measurable change because their ranks reproduce the previous hardcoded durations. Continue parity work by comparing same-seed action timelines, including GCD availability, cast completion, projectile impact, and equal-time event ordering. Add spell haste only after the baseline timing rules match; haste remains a separate integration gap below.
- [ ] **Complete rotational and execute parity.** Align Soul Fire, pet actions, and Decimation trigger/uptime behavior with CPU event timing, cooldowns, resource use, and proc transitions; these actions exist in the shader but still show preset discrepancies.
- [ ] **Match DoT lifecycle and refresh rules.** Align application, tick timestamps, refresh eligibility, pandemic behavior, snapshots, and damage modifiers with the CPU event model.
- [ ] **Expose per-spell GPU accounting.** Replace shared-channel allocation in parity output with direct per-spell casts, hits, crits, misses, periodic ticks, and damage, then report the first divergent event for a seed.
- [ ] **Regenerate the committed preset parity report.** The checked-in report does not describe the latest comparison run; regenerate it after the timing and accounting changes so it remains a trustworthy baseline.

- [x] **Pass the edited APL into current-build simulations.** Enabled actions, rule order, and supported raw conditions are encoded into each fight config. Each config keeps its own APL table in multi-config shader runs. Unsupported condition expressions and Drain Soul now fail with an explicit message instead of being silently treated as another action.
- [x] **Derive supported talent effects from the allocated ranks.** The current-build config now receives rank-aware values for Improved Life Tap, Suppression, Improved Corruption, Malediction, Improved Drains, Malevolence, Nightfall, Improved Shadow Bolt, Ruin, Shadowburn, Siphon Life, Soul Siphon, Decimation, Demonic Brand, Demonic Energies, Demonic Knowledge, Master Demonologist, Soul Link, Improved Imp, Unholy Power, Improved Sayaad, Agonizing Flames, Fire and Brimstone, Shadow and Flame, Conflagrate, Incinerate, and Wrack.
- [ ] **Implement the remaining talent effects.** The shader still does not model talents including Improved Bane of Agony, Amplify Curse, Pandemic's rank-specific behavior beyond the current crit multiplier approximation, Improved Health Funnel, Improved Voidwalker, Fel Vitality, Master Summoner, Improved Felhunter, Demonic Pact, Bane cast-time reduction, Cataclysm mana reduction, Aftermath, Intensity, Pyroclasm, and Bane of Havoc.
- [ ] **Implement spell haste.** The form and gear summary expose haste, but haste does not enter the DES config or change cast and GCD times.
- [ ] **Model race effects in the simulator.** Race selection is not part of the shader config. Current-build handling only adjusts Gnome intellect and Human spirit; Orc pet damage and Troll haste are not applied. The shader's base-stat and crit assumptions remain Human-oriented.
- [ ] **Implement Drain Soul.** The APL row is present, but the shader does not simulate Drain Soul. Enabling the row reports this limitation.
- [ ] **Activate drain talent coefficients.** Improved Drains and Soul Siphon values now reach the config, but their damage coefficients are only consumed by the unimplemented Drain Soul event. They do not currently change DPS.
- [ ] **Use the edited APL in the preset comparison batch.** “Simulate Specs” creates APLs from preset config rules and does not reuse the current-build editor's APL.
- [ ] **Expand APL condition support.** The condition editor accepts free-form text, while the shader compiler supports a defined subset of predicates and conjunctions. Add an explicit condition builder or support more expression forms before advertising arbitrary APL expressions.
- [ ] **Complete gear, buff, and proc integration for current-build runs.** The README lists gear and buff procs, raid buffs, trinkets, and other dynamic effects as unsupported or incomplete; the displayed selectors should not imply that every selected effect changes DPS until its fields are wired through.
