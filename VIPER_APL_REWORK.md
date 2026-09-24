# VIPER & Editable APL Architecture Design Document

## 1. Overview & Motivation

Currently, combat rotations in the simulator rely on hardcoded presets defined across 20+ `RotationChoice` enumeration variants. While presets provide quick defaults, this architecture presents two major limitations:
1. **User Inflexibility**: Users cannot easily reorder, enable/disable, or customize specific priority indices without introducing new C++ enum branches.
2. **Policy Extraction Bottleneck (VIPER)**: Machine learning/RL oracles (MCTS / Q-learning / Surrogates) produce complex or black-box policies. Verifiable Reinforcement Learning via Policy Extraction (VIPER) requires an interpretable, indexable decision structure (Decision Tree / APL) that can be iteratively sampled, evaluated, and compiled.

---

## 2. Theoretical Background: VIPER

Standard Imitation Learning / Behavioral Cloning fits decision trees directly onto logged state-action pairs $(s, a)$. In dynamic simulation environments (like WoW combat with procs, GCD, and cooldowns), this fails due to **covariate shift**:
- A slight sub-optimal decision puts the simulator into an unfamiliar state $\tilde{s}$.
- Errors cascade rapidly, collapsing rotation throughput.

### The VIPER Solution (DAgger with Q-Weighted Resampling)
VIPER addresses this via iterative policy extraction:
1. **Rollout with Candidate Policy $\pi_k$**: Run the current candidate APL / Decision Tree in the simulator to generate state trajectories.
2. **Oracle Query**: In each visited state $s_t$, query the Oracle (MCTS / Q-Value function) for $Q(s_t, a)$ across all legal actions.
3. **Q-Weighted Sample Loss**: Weight each training example by the cost of making a mistake:
   $$w(s) = \max_{a \in \mathcal{A}} Q(s, a) - \min_{a \in \mathcal{A}} Q(s, a)$$
   *(or the gap between the oracle's optimal action $a^*$ and the best alternative).*
4. **Weighted Tree Retraining**: Train a new decision tree $\pi_{k+1}$ on the aggregated dataset $\mathcal{D}$.
5. **APL Compilation**: Pruned decision tree rules are converted into sequential indexable APL rules or fast-eval branching tables.

---

## 3. Implementation Status & Milestones

| Milestone | Status | % Complete | Implementation Summary |
| :--- | :---: | :---: | :--- |
| **1. Indexable & Mutable APL Core** | **Done** | **100%** | Mutable `PolicyConfig` API (`swap_rules`, `move_rule_up/down`, `set_rule_enabled`), preset decoupling, simulation engine execution overrides, and UI editor tables. |
| **2. State Observation Extraction** | **Done** | **100%** | 24 continuous game-state features in `SimObservation`, runtime `get_current_observation()`, transition logging across all 20+ spell branches in `warlock_sim.cpp`, and CSV export. |
| **3. Monte Carlo Lookahead Oracle** | **Done** | **100%** | Replaced placeholder baseline logging with true **Monte Carlo Branching Lookahead** (`evaluate_action_q_value`), evaluating forward returns across legal actions to calculate empirical $Q(s, a)$ and regret weights $w(s)$. |
| **4. Data-Driven VIPER APL Synthesis** | **Done** | **100%** | Replaced static bucket sorting with **Guided Q-Search & Beam Hill Climbing**, optimizing rule orderings and pruning unselected rules to capture positive realized DPS gains over baseline. |
| **5. Live Expected Value UI Dashboard** | **Done** | **100%** | Background worker thread with live stage progress, 3-Card Expected Value comparison (Baseline vs Oracle Ceiling vs Extracted APL), MCTS action stats table, and side-by-side Rule Diff viewer. |
| **6. Pure C++ Embedded Tree Learner** | **Done** | **100%** | In-memory, self-contained CART Decision Tree classifier in `src/sim/common/decision_tree.hpp` with $Q$-weighted Gini optimization, multi-threaded rollouts, continuous threshold extraction, standalone C++ code transpilation, and UI tree viewer. |
| **7. Live Online MCTS Controller** | **Done** | **100%** | Live online greedy oracle simulation execution mode establishing the unconstrained empirical upper bound benchmark ($\mathbb{E}[V^*] \pm \sigma$, $[min - max]$) with standalone benchmark worker and single-sim controller. |
| **8. Iterative DAgger Aggregation** | **Done** | **100%** | Multi-iteration rollout aggregation ($\mathcal{D} \leftarrow \mathcal{D} \cup \mathcal{D}_k$) across synthesized policy states to completely eliminate covariate shift, with iteration history tracking and configurable passes. |

---

### Detailed Milestone Breakdown

#### Milestone 1: Indexable & Mutable APL Core (100% Complete)
- [x] Create project design document (`VIPER_APL_REWORK.md`).
- [x] Implement dynamic `APL` and `PriorityRule` mutation API on `PolicyConfig`:
  - `move_rule_up`, `move_rule_down`, `swap_rules`, `set_rule_enabled`, `insert_rule`, `remove_rule`, `reset_to_preset`.
- [x] Decouple preset generation (`build_preset_rules`) from dynamic evaluation (`get_priority_rules`).
- [x] Update simulation execution engines (`warlock_sim.cpp`, `priest_sim.cpp`) to respect dynamic rule ordering and overrides.
- [x] Add interactive APL editor tables with row indexing, Up/Down reordering buttons, and active checkboxes to UI (`panel_policy.hpp`).
- [x] Comprehensive unit tests in `test_rotations.cpp` validating index swapping, shifting, disabling, and simulation runtime overrides.

#### Milestone 2: State Observation Vector Extraction (100% Complete)
- [x] Implement `SimObservation` in `src/sim/common/sim_state_vector.hpp` capturing 24 normalized state features:
  - Resources: `player_mana_pct`, `player_hp_pct`.
  - Target states: `fight_progress_pct`, `time_remaining_sec`, `target_hp_pct`, `num_targets`, `target2_has_havoc`.
  - Active buff & proc timers: `nightfall_proc_active`, `decimation_rem_sec`, `shadow_and_flame_rem_sec`, `trinket_rem_sec`, `racial_rem_sec`, `eureka_charges`.
  - Primary target DoTs & debuffs: `dot_corruption_rem_sec`, `dot_agony_rem_sec`, `dot_doom_rem_sec`, `dot_immolate_rem_sec`, `dot_siphon_life_rem_sec`, `dot_wrack_rem_sec`, `isb_charges_rem`.
  - Cooldowns: `cd_conflagrate_sec`, `cd_shadowburn_sec`, `cd_curse_of_doom_sec`, `cd_amplify_curse_sec`, `cd_racial_sec`, `cd_potion_sec`, `cd_demonic_rune_sec`.
- [x] Connected continuous state observation extractor `get_current_observation()` and transition logger `log_viper_sample()` across every spell branch in `warlock_sim.cpp`.
- [x] Implement `VIPERDataset` with automatic CSV serialization (`to_csv()`).

#### Milestone 3: Monte Carlo Lookahead Oracle & Extraction (100% Complete)
- [x] Implement `VIPEROracle` in `src/sim/warlock/viper_oracle.hpp`:
  - Action legality verification across all warlock spells and abilities.
  - Native **Monte Carlo Forward Branching Lookahead** (`evaluate_action_q_value`): evaluates candidate actions via forward rollout horizons.
  - Empirical regret weighting: $w(s) = Q(s, a^*) - Q(s, a_{\text{sub}})$.
  - Measure true empirical Oracle Rollout Expected DPS.
- [x] Data-Driven **Guided Q-Search & Beam Hill-Climbing**:
  - Replaced placeholder bucket sort with beam search over candidate rule swaps and pruning passes.
  - Benchmarks synthesized rules against baseline with full standard deviation and variance tracking.

#### Milestone 4: Interactive Expected Value UI Dashboard (100% Complete)
- [x] Added **"VIPER APL Search"** directly into `panel_optimizer.hpp`:
  - Background worker thread with non-blocking UI and live progress bar across 4 phases.
  - 3-Card Expected Value Dashboard:
    - **1. Baseline Expected DPS** ($\mathbb{E}[V_{\text{base}}] \pm \sigma$).
    - **2. MCTS Oracle Ceiling** ($\mathbb{E}[V^*]$, potential gain %).
    - **3. Extracted VIPER APL** ($\mathbb{E}[V_{\text{APL}}]$, realized gain %, % of ceiling captured, and oracle fidelity %).
  - Inspection tabs:
    - **Tab 1: Extracted APL Rule Priority**: Table of synthesized priority rules with direct "Apply to Sim Policy" button.
    - **Tab 2: MCTS Action Values & Regret**: Oracle selection % and average regret weight $w(s)$ per ability.
    - **Tab 3: Rule Diff**: Side-by-side comparison highlighting `PROMOTED`, `DEMOTED`, and `UNCHANGED` rules against the standard preset.
    - **Tab 4: Decision Tree (CART & C++)**: Interactive view of the trained embedded decision tree, extracted decision rules, ASCII hierarchy, and exportable C++ code.

#### Milestone 6: Pure C++ Embedded CART Tree Learner & Parameterized Synthesis (100% Complete)
- [x] Implement self-contained, header-only $Q$-weighted CART Classifier in `src/sim/common/decision_tree.hpp`.
- [x] Multi-Threaded Rollout Parallelism in `collect_viper_dataset` and `compute_expected_dps`.
- [x] Continuous predicate evaluation directly inside the simulation engine:
  - `max_mana_pct`, `min_mana_pct`, `max_target_hp_pct`, `min_time_remaining`, `max_time_remaining`, `max_dot_rem_sec`.
- [x] Direct compilation of Decision Tree leaf paths into variable-length, multi-instance `PriorityRule` structures (`compile_tree_to_apl`).
- [x] Coordinate parameter sweeps over critical rotational levers (Life Tap %, Curse of Doom cutoff, Pandemic refresh window).
- [x] Standalone C++ if/else code transpilation (`to_cpp()`) and ASCII hierarchy formatting (`to_text_tree()`).
- [x] Comprehensive unit test suite covering synthetic splits, sample weight domination, tree depth, and code generation (`test_viper_pipeline.cpp`).

#### Milestone 7: Live Online MCTS Controller Benchmark (100% Complete)
- [x] Implement live greedy online oracle decision evaluation at every single GCD of the simulation (`use_oracle_execution_policy` in `WarlockSimulator` and `PolicyConfig`).
- [x] Exclude off-GCD instant abilities from blocking on-GCD action priorities.
- [x] Standalone multi-threaded oracle benchmark runner (`VIPEROracle::benchmark_live_mcts_oracle`).
- [x] UI integration: "Benchmark Live MCTS Controller" button and Card 2 displaying empirical expectation $\mathbb{E}[V^*] \pm \sigma$ and $[min - max]$ bounds.
- [x] Unit test validation in `test_viper_pipeline.cpp` (`LiveOnlineMCTSControllerExecution` and `StandaloneLiveMCTSOracleBenchmark`).

#### Milestone 8: Iterative DAgger Dataset Aggregation (100% Complete)
- [x] Implement multi-iteration DAgger loop in `VIPEROracle::extract_viper_apl` with dataset aggregation $\mathcal{D} \leftarrow \mathcal{D} \cup \mathcal{D}_k$.
- [x] Iterative rollout execution under candidate policy $\hat{\pi}_k$ collecting visited states and computing regret-weighted oracle queries.
- [x] Continuous predicate optimization and CART retraining per DAgger iteration pass.
- [x] Configurable DAgger iterations slider (1–5 passes) and iteration history tracking (`DAggerIterationLog`).
- [x] Tab 5: "DAgger Iteration History" in UI optimizer panel detailing iteration pass, transitions added, $|\mathcal{D}|$, candidate DPS, and CART tree fidelity %.
- [x] Unit test validation in `test_viper_pipeline.cpp` (`MultiIterationDAggerDatasetAggregation`).

---

## 4. Next Opportunities & Concrete Roadmap

### Milestone 9: Multi-Class Expansion (Priest Sim Pipeline)
- Port the 24-feature observation vector, CART learner, and parameterized APL compiler to Shadow/Disc Priest (`src/sim/priest/`).
- Optimize multi-DoT upkeep (Shadow Word: Pain, Devouring Plague) and Mind Flay channel tick clipping.

### Milestone 10: Dynamic Encounter Conditions (Multi-Target & Add Cleave)
- Apply VIPER extraction to multi-target encounters (2–5 targets).
- Discover dynamic add-switching cutoffs, Bane of Havoc upkeep windows, and multi-DoT viability thresholds that dramatically outperform static single-target presets.
