# WebGPU vs CPU DES Parity Report

**Generated at**: Live Evaluation  
**Iterations**: 10000  
**Summary**: 0/10 Passed  

### Scenario: pure_sb_baseline
**Description**: Pure Shadow Bolt filler with 500 SP, 12% Hit, 15% Crit (180s)  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 284.8 DPS | 265.9 DPS | -18.9 | -6.65% | ❌ FAIL |
| Median (p50) DPS | 284.8 DPS | 265.9 DPS | -18.9 | -6.64% | ❌ FAIL |
| Std Dev DPS | 12.3 DPS | 11.5 DPS | -0.9 | -7.00% | ✅ PASS |
| p5 DPS | 264.0 DPS | 246.7 DPS | -17.4 | -6.59% | ❌ FAIL |
| p95 DPS | 304.6 DPS | 284.7 DPS | -20.0 | -6.56% | ❌ FAIL |
| ISB Uptime % | 0.0% | 0.0% | 0.0 | +0.00% | ✅ PASS |
| Mean Life Taps | 28.0 | 28.0 | 0.0 | +0.00% | ✅ PASS |
| Total Crit % | 22.1% | 22.2% | 0.1 | +0.32% | ✅ PASS |
| Total Miss % | 5.0% | 5.0% | 0.0 | +0.51% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 100.0% | 100.0% | 0.0% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 284.8 | 265.9 | 55.0 | 55.0 | 981.1 | 916.1 | 22.1% | 221961.6% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=981.1 vs WebGPU=916.1 (-6.6%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=22.1% vs WebGPU=221961.6%


---

### Scenario: isb_classic_charges
**Description**: 5/5 Improved Shadow Bolt with Classic 4-Charge consumption  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 372.9 DPS | 354.9 DPS | -18.0 | -4.82% | ❌ FAIL |
| Median (p50) DPS | 372.8 DPS | 355.1 DPS | -17.8 | -4.76% | ❌ FAIL |
| Std Dev DPS | 22.0 DPS | 20.8 DPS | -1.2 | -5.54% | ✅ PASS |
| p5 DPS | 337.1 DPS | 320.5 DPS | -16.6 | -4.91% | ❌ FAIL |
| p95 DPS | 409.3 DPS | 389.2 DPS | -20.1 | -4.91% | ❌ FAIL |
| ISB Uptime % | 68.3% | 70.7% | 2.3 | +3.43% | ✅ PASS |
| Mean Life Taps | 28.0 | 28.0 | 0.0 | +0.00% | ✅ PASS |
| Total Crit % | 32.1% | 32.2% | 0.1 | +0.44% | ✅ PASS |
| Total Miss % | 4.9% | 5.0% | 0.1 | +2.49% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 100.0% | 100.0% | 0.0% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 372.9 | 354.9 | 55.0 | 55.0 | 1308.3 | 1222.9 | 32.1% | 322266.6% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=1308.3 vs WebGPU=1222.9 (-6.5%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=32.1% vs WebGPU=322266.6%


---

### Scenario: isb_forever_window
**Description**: 5/5 Improved Shadow Bolt with WoW Forever 12s chargeless debuff window  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 372.9 DPS | 354.9 DPS | -18.0 | -4.82% | ❌ FAIL |
| Median (p50) DPS | 372.8 DPS | 355.1 DPS | -17.8 | -4.76% | ❌ FAIL |
| Std Dev DPS | 22.0 DPS | 20.8 DPS | -1.2 | -5.54% | ✅ PASS |
| p5 DPS | 337.1 DPS | 320.5 DPS | -16.6 | -4.91% | ❌ FAIL |
| p95 DPS | 409.3 DPS | 389.2 DPS | -20.1 | -4.91% | ❌ FAIL |
| ISB Uptime % | 68.6% | 71.0% | 2.5 | +3.59% | ✅ PASS |
| Mean Life Taps | 28.0 | 28.0 | 0.0 | +0.00% | ✅ PASS |
| Total Crit % | 32.1% | 32.2% | 0.1 | +0.44% | ✅ PASS |
| Total Miss % | 4.9% | 5.0% | 0.1 | +2.49% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 100.0% | 100.0% | 0.0% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 372.9 | 354.9 | 55.0 | 55.0 | 1308.3 | 1222.9 | 32.1% | 322266.6% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=1308.3 vs WebGPU=1222.9 (-6.5%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=32.1% vs WebGPU=322266.6%


---

### Scenario: corruption_nightfall
**Description**: Maintained Corruption with 2/2 Nightfall Shadow Trance instant procs  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 420.8 DPS | 352.0 DPS | -68.8 | -16.36% | ❌ FAIL |
| Median (p50) DPS | 421.0 DPS | 352.2 DPS | -68.7 | -16.32% | ❌ FAIL |
| Std Dev DPS | 16.9 DPS | 13.8 DPS | -3.1 | -18.28% | ✅ PASS |
| p5 DPS | 392.7 DPS | 329.3 DPS | -63.5 | -16.16% | ❌ FAIL |
| p95 DPS | 448.6 DPS | 374.3 DPS | -74.3 | -16.56% | ❌ FAIL |
| ISB Uptime % | 0.0% | 0.0% | 0.0 | +0.00% | ✅ PASS |
| Mean Life Taps | 29.8 | 31.4 | 1.6 | +5.51% | ❌ FAIL |
| Total Crit % | 22.1% | 23.9% | 1.8 | +8.16% | ❌ FAIL |
| Total Miss % | 7.0% | 7.0% | -0.1 | -1.04% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 61.0% | 63.0% | 2.0% | ❌ FAIL |
| Corruption | 19.0% | 21.0% | 2.0% | ❌ FAIL |
| Curse of Agony | 5.4% | 0.0% | -5.4% | ❌ FAIL |
| Curse of Doom | 14.7% | 0.0% | -14.7% | ❌ FAIL |
| Immolate | 0.0% | 16.1% | 16.1% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 256.6 | 221.6 | 45.9 | 42.0 | 1088.7 | 1020.2 | 22.1% | 239291.6% | ❌ FAIL |
| Corruption | 79.9 | 73.8 | 10.2 | 9.7 | 266.8 | 227.7 | 22.2% | 0.0% | ❌ FAIL |
| Curse of Agony | 22.6 | 0.0 | 2.8 | 0.0 | 165.1 | 0.0 | 22.1% | 0.0% | ❌ FAIL |
| Curse of Doom | 61.8 | 0.0 | 2.1 | 0.0 | 5560.0 | 0.0 | 22.0% | 0.0% | ❌ FAIL |
| Immolate | 0.0 | 56.6 | 0.0 | 9.7 | 0.0 | 175.2 | 0.0% | 0.0% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Cast count mismatch: CPU=45.88 vs WebGPU=42.02 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=1088.7 vs WebGPU=1020.2 (-6.3%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=22.1% vs WebGPU=239291.6%
- ⚠️ [Corruption] Cast count mismatch: CPU=10.23 vs WebGPU=9.72 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Corruption] Damage per hit mismatch: CPU=266.8 vs WebGPU=227.7 (-14.7%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Corruption] Crit rate mismatch: CPU=22.2% vs WebGPU=0.0%
- ⚠️ [Corruption] Miss rate mismatch: CPU=7.1% vs WebGPU=0.0%
- ⚠️ [Curse of Agony] Cast count mismatch: CPU=2.81 vs WebGPU=0.00 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Curse of Agony] Damage per hit mismatch: CPU=165.1 vs WebGPU=0.0 (-100.0%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Curse of Agony] Crit rate mismatch: CPU=22.1% vs WebGPU=0.0%
- ⚠️ [Curse of Agony] Miss rate mismatch: CPU=7.2% vs WebGPU=0.0%
- ⚠️ [Curse of Doom] Cast count mismatch: CPU=2.15 vs WebGPU=0.00 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Curse of Doom] Damage per hit mismatch: CPU=5560.0 vs WebGPU=0.0 (-100.0%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Curse of Doom] Crit rate mismatch: CPU=22.0% vs WebGPU=0.0%
- ⚠️ [Curse of Doom] Miss rate mismatch: CPU=6.8% vs WebGPU=0.0%
- ⚠️ [Immolate] Cast count mismatch: CPU=0.00 vs WebGPU=9.69 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Life Tap] Tap count differs: CPU=29.76 vs WebGPU=31.40 (Check Life Tap mana gain formula or tap threshold)


---

### Scenario: full_multidot_affliction
**Description**: Full Multi-DoT APL: Bane of Agony (ramp ticks) + Corruption + Siphon Life + Immolate  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 492.6 DPS | 482.2 DPS | -10.5 | -2.12% | ❌ FAIL |
| Median (p50) DPS | 492.4 DPS | 482.4 DPS | -10.0 | -2.02% | ❌ FAIL |
| Std Dev DPS | 16.6 DPS | 12.9 DPS | -3.7 | -22.48% | ✅ PASS |
| p5 DPS | 466.1 DPS | 460.5 DPS | -5.5 | -1.19% | ✅ PASS |
| p95 DPS | 520.4 DPS | 502.8 DPS | -17.6 | -3.37% | ❌ FAIL |
| ISB Uptime % | 0.0% | 0.0% | 0.0 | +0.00% | ✅ PASS |
| Mean Life Taps | 29.1 | 32.0 | 2.9 | +9.83% | ❌ FAIL |
| Total Crit % | 27.1% | 28.1% | 1.0 | +3.84% | ✅ PASS |
| Total Miss % | 3.0% | 3.0% | -0.0 | -0.31% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 61.0% | 42.4% | -18.6% | ❌ FAIL |
| Corruption | 18.6% | 17.5% | -1.1% | ❌ FAIL |
| Curse of Agony | 6.3% | 20.3% | 14.0% | ❌ FAIL |
| Curse of Doom | 14.1% | 0.0% | -14.1% | ❌ FAIL |
| Siphon Life | 0.0% | 6.7% | 6.7% | ✅ PASS |
| Immolate | 0.0% | 13.2% | 13.2% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 300.6 | 204.3 | 45.0 | 32.5 | 1242.5 | 1164.9 | 27.1% | 281456.9% | ❌ FAIL |
| Corruption | 91.5 | 84.3 | 10.1 | 9.8 | 304.7 | 259.1 | 27.1% | 0.0% | ❌ FAIL |
| Curse of Agony | 30.9 | 97.8 | 3.1 | 8.2 | 206.2 | 178.0 | 27.0% | 0.0% | ❌ FAIL |
| Curse of Doom | 69.6 | 0.0 | 2.1 | 0.0 | 6261.4 | 0.0 | 27.0% | 0.0% | ❌ FAIL |
| Siphon Life | 0.0 | 32.1 | 0.0 | 6.2 | 0.0 | 93.6 | 0.0% | 0.0% | ❌ FAIL |
| Immolate | 0.0 | 63.6 | 0.0 | 9.3 | 0.0 | 204.4 | 0.0% | 0.0% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Cast count mismatch: CPU=44.98 vs WebGPU=32.54 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=1242.5 vs WebGPU=1164.9 (-6.2%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=27.1% vs WebGPU=281456.9%
- ⚠️ [Corruption] Damage per hit mismatch: CPU=304.7 vs WebGPU=259.1 (-15.0%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Corruption] Crit rate mismatch: CPU=27.1% vs WebGPU=0.0%
- ⚠️ [Corruption] Miss rate mismatch: CPU=3.0% vs WebGPU=0.0%
- ⚠️ [Curse of Agony] Cast count mismatch: CPU=3.06 vs WebGPU=8.25 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Curse of Agony] Damage per hit mismatch: CPU=206.2 vs WebGPU=178.0 (-13.7%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Curse of Agony] Crit rate mismatch: CPU=27.0% vs WebGPU=0.0%
- ⚠️ [Curse of Agony] Miss rate mismatch: CPU=3.0% vs WebGPU=0.0%
- ⚠️ [Curse of Doom] Cast count mismatch: CPU=2.06 vs WebGPU=0.00 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Curse of Doom] Damage per hit mismatch: CPU=6261.4 vs WebGPU=0.0 (-100.0%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Curse of Doom] Crit rate mismatch: CPU=27.0% vs WebGPU=0.0%
- ⚠️ [Curse of Doom] Miss rate mismatch: CPU=3.0% vs WebGPU=0.0%
- ⚠️ [Siphon Life] Cast count mismatch: CPU=0.00 vs WebGPU=6.18 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Immolate] Cast count mismatch: CPU=0.00 vs WebGPU=9.33 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Life Tap] Tap count differs: CPU=29.15 vs WebGPU=32.01 (Check Life Tap mana gain formula or tap threshold)


---

### Scenario: curse_of_doom_burst
**Description**: Bane of Doom (60s delayed burst) + Corruption + Shadow Bolt  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 412.1 DPS | 393.5 DPS | -18.6 | -4.51% | ❌ FAIL |
| Median (p50) DPS | 411.8 DPS | 393.1 DPS | -18.6 | -4.53% | ❌ FAIL |
| Std Dev DPS | 16.6 DPS | 15.6 DPS | -1.0 | -5.93% | ✅ PASS |
| p5 DPS | 385.2 DPS | 368.2 DPS | -17.1 | -4.43% | ❌ FAIL |
| p95 DPS | 440.0 DPS | 420.2 DPS | -19.8 | -4.51% | ❌ FAIL |
| ISB Uptime % | 0.0% | 0.0% | 0.0 | +0.00% | ✅ PASS |
| Mean Life Taps | 29.0 | 28.3 | -0.7 | -2.27% | ✅ PASS |
| Total Crit % | 25.2% | 25.3% | 0.2 | +0.65% | ✅ PASS |
| Total Miss % | 5.0% | 5.0% | 0.0 | +0.09% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 67.0% | 67.4% | 0.4% | ✅ PASS |
| Corruption | 17.0% | 16.5% | -0.5% | ✅ PASS |
| Curse of Doom | 16.0% | 16.1% | 0.1% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 276.0 | 265.2 | 45.5 | 45.9 | 1169.5 | 1094.3 | 25.1% | 253214.5% | ❌ FAIL |
| Corruption | 70.2 | 65.0 | 9.3 | 8.4 | 262.2 | 231.5 | 25.2% | 0.0% | ❌ FAIL |
| Curse of Doom | 65.9 | 63.2 | 3.2 | 3.0 | 5929.3 | 3800.3 | 25.3% | 0.0% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=1169.5 vs WebGPU=1094.3 (-6.4%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=25.1% vs WebGPU=253214.5%
- ⚠️ [Corruption] Cast count mismatch: CPU=9.31 vs WebGPU=8.43 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Corruption] Damage per hit mismatch: CPU=262.2 vs WebGPU=231.5 (-11.7%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Corruption] Crit rate mismatch: CPU=25.2% vs WebGPU=0.0%
- ⚠️ [Corruption] Miss rate mismatch: CPU=5.1% vs WebGPU=0.0%
- ⚠️ [Curse of Doom] Damage per hit mismatch: CPU=5929.3 vs WebGPU=3800.3 (-35.9%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Curse of Doom] Crit rate mismatch: CPU=25.3% vs WebGPU=0.0%
- ⚠️ [Curse of Doom] Miss rate mismatch: CPU=5.0% vs WebGPU=0.0%


---

### Scenario: pet_active_imp
**Description**: Active Imp Pet with modern Firebolt scaling and +30% Improved Imp talent  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 286.5 DPS | 318.1 DPS | 31.6 | +11.02% | ❌ FAIL |
| Median (p50) DPS | 286.8 DPS | 318.3 DPS | 31.5 | +10.98% | ❌ FAIL |
| Std Dev DPS | 13.1 DPS | 12.4 DPS | -0.7 | -5.04% | ✅ PASS |
| p5 DPS | 264.1 DPS | 297.2 DPS | 33.0 | +12.51% | ❌ FAIL |
| p95 DPS | 307.5 DPS | 337.9 DPS | 30.4 | +9.90% | ❌ FAIL |
| ISB Uptime % | 0.0% | 0.0% | 0.0 | +0.00% | ✅ PASS |
| Mean Life Taps | 29.0 | 29.0 | 0.0 | +0.00% | ✅ PASS |
| Total Crit % | 20.2% | 22.9% | 2.7 | +13.52% | ❌ FAIL |
| Total Miss % | 6.9% | 7.0% | 0.1 | +1.44% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 94.6% | 81.5% | -13.1% | ❌ FAIL |
| Total Pet | 5.4% | 18.5% | 13.1% | ❌ FAIL |
| Pet Firebolt | 5.4% | 18.5% | 13.1% | ❌ FAIL |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 271.1 | 259.3 | 55.0 | 55.0 | 972.1 | 912.6 | 20.2% | 228836.8% | ❌ FAIL |
| Pet: Firebolt | 15.4 | 58.8 | 23.0 | 90.0 | 129.8 | 117.6 | 20.0% | 0.0% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=972.1 vs WebGPU=912.6 (-6.1%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=20.2% vs WebGPU=228836.8%
- ⚠️ [Pet: Firebolt] Cast count mismatch: CPU=23.00 vs WebGPU=90.00 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Pet: Firebolt] Damage per hit mismatch: CPU=129.8 vs WebGPU=117.6 (-9.4%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Pet: Firebolt] Crit rate mismatch: CPU=20.0% vs WebGPU=0.0%
- ⚠️ [Pet: Firebolt] Miss rate mismatch: CPU=7.0% vs WebGPU=0.0%


---

### Scenario: pet_active_succubus
**Description**: Active Succubus with Melee swings, Lash of Pain shadow nuke, and armor reduction  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 334.9 DPS | 320.5 DPS | -14.4 | -4.29% | ❌ FAIL |
| Median (p50) DPS | 335.4 DPS | 320.9 DPS | -14.4 | -4.30% | ❌ FAIL |
| Std Dev DPS | 14.4 DPS | 13.5 DPS | -0.9 | -6.41% | ✅ PASS |
| p5 DPS | 310.4 DPS | 297.7 DPS | -12.7 | -4.08% | ❌ FAIL |
| p95 DPS | 357.7 DPS | 342.2 DPS | -15.5 | -4.33% | ❌ FAIL |
| ISB Uptime % | 0.0% | 0.0% | 0.0 | +0.00% | ✅ PASS |
| Mean Life Taps | 28.0 | 28.0 | 0.0 | +0.00% | ✅ PASS |
| Total Crit % | 22.0% | 30.6% | 8.6 | +39.07% | ❌ FAIL |
| Total Miss % | 6.9% | 11.3% | 4.4 | +64.01% | ❌ FAIL |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 86.6% | 86.1% | -0.6% | ✅ PASS |
| Total Pet | 13.4% | 13.9% | 0.6% | ✅ PASS |
| Pet Melee | 10.6% | 11.1% | 0.5% | ✅ PASS |
| Pet Lash of Pain | 2.8% | 2.9% | 0.1% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 290.2 | 275.9 | 55.0 | 55.0 | 1040.0 | 1017.6 | 22.0% | 306304.6% | ❌ FAIL |
| Pet: Melee | 35.5 | 35.4 | 90.0 | 90.0 | 83.0 | 83.0 | 26.8% | 26.7% | ✅ PASS |
| Pet: Lash of Pain | 9.2 | 9.2 | 15.0 | 15.0 | 119.2 | 118.9 | 22.0% | 22.2% | ✅ PASS |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=22.0% vs WebGPU=306304.6%
- ⚠️ [Shadow Bolt] Miss rate mismatch: CPU=6.9% vs WebGPU=11.3%


---

### Scenario: demonic_sacrifice_succubus
**Description**: Demonic Sacrifice: Succubus granting flat +15% Shadow Damage  
**Iterations**: 10000 | **Duration**: 180s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 321.8 DPS | 305.7 DPS | -16.1 | -5.00% | ❌ FAIL |
| Median (p50) DPS | 321.9 DPS | 305.8 DPS | -16.1 | -5.00% | ❌ FAIL |
| Std Dev DPS | 14.1 DPS | 13.2 DPS | -0.9 | -6.13% | ✅ PASS |
| p5 DPS | 298.5 DPS | 283.6 DPS | -14.8 | -4.97% | ❌ FAIL |
| p95 DPS | 344.6 DPS | 327.3 DPS | -17.3 | -5.02% | ❌ FAIL |
| ISB Uptime % | 0.0% | 0.0% | 0.0 | +0.00% | ✅ PASS |
| Mean Life Taps | 28.0 | 28.0 | 0.0 | +0.00% | ✅ PASS |
| Total Crit % | 22.1% | 22.2% | 0.1 | +0.28% | ✅ PASS |
| Total Miss % | 4.9% | 5.0% | 0.1 | +2.49% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 100.0% | 100.0% | 0.0% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 321.8 | 305.7 | 55.0 | 55.0 | 1129.2 | 1053.6 | 22.1% | 221961.6% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=1129.2 vs WebGPU=1053.6 (-6.7%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=22.1% vs WebGPU=221961.6%


---

### Scenario: mana_stress_600s
**Description**: 600s prolonged endurance fight testing Life Tap weaving & MP5 regeneration  
**Iterations**: 10000 | **Duration**: 600s | **Status**: ❌ **PARITY FAIL**

| Metric | CPU (DES) | WebGPU | Delta | % Diff | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Mean DPS | 449.6 DPS | 415.8 DPS | -33.7 | -7.50% | ❌ FAIL |
| Median (p50) DPS | 449.5 DPS | 415.9 DPS | -33.5 | -7.46% | ❌ FAIL |
| Std Dev DPS | 9.6 DPS | 7.0 DPS | -2.6 | -26.69% | ✅ PASS |
| p5 DPS | 433.8 DPS | 404.1 DPS | -29.7 | -6.86% | ❌ FAIL |
| p95 DPS | 465.2 DPS | 427.2 DPS | -38.0 | -8.17% | ❌ FAIL |
| ISB Uptime % | 0.0% | 0.0% | 0.0 | +0.00% | ✅ PASS |
| Mean Life Taps | 88.0 | 91.9 | 3.9 | +4.41% | ✅ PASS |
| Total Crit % | 22.1% | 23.1% | 1.0 | +4.66% | ✅ PASS |
| Total Miss % | 5.0% | 5.0% | 0.0 | +0.05% | ✅ PASS |

#### Damage Breakdown

| Spell | CPU Share | WebGPU Share | Delta | Status |
| :--- | :---: | :---: | :---: | :---: |
| Shadow Bolt | 63.6% | 52.7% | -10.8% | ❌ FAIL |
| Corruption | 15.8% | 15.1% | -0.8% | ✅ PASS |
| Curse of Agony | 0.6% | 18.9% | 18.4% | ❌ FAIL |
| Curse of Doom | 20.0% | 0.0% | -20.0% | ❌ FAIL |
| Immolate | 0.0% | 13.3% | 13.3% | ✅ PASS |

#### Detailed Spell Performance

| Spell | CPU DPS | GPU DPS | CPU Casts | GPU Casts | CPU Avg Hit | GPU Avg Hit | CPU Crit% | GPU Crit% | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Shadow Bolt | 285.8 | 219.3 | 156.7 | 128.6 | 1152.4 | 1077.2 | 22.1% | 231494.8% | ❌ FAIL |
| Corruption | 71.2 | 62.6 | 29.5 | 27.6 | 258.2 | 226.6 | 22.2% | 0.0% | ❌ FAIL |
| Curse of Agony | 2.6 | 78.8 | 1.5 | 27.8 | 152.9 | 141.7 | 22.1% | 0.0% | ❌ FAIL |
| Curse of Doom | 90.0 | 0.0 | 9.5 | 0.0 | 5998.8 | 0.0 | 22.0% | 0.0% | ❌ FAIL |
| Immolate | 0.0 | 55.1 | 0.0 | 29.0 | 0.0 | 189.8 | 0.0% | 0.0% | ❌ FAIL |

#### 🔍 Root-Cause Diagnostics & Discrepancies

- ⚠️ [Shadow Bolt] Cast count mismatch: CPU=156.72 vs WebGPU=128.60 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Shadow Bolt] Damage per hit mismatch: CPU=1152.4 vs WebGPU=1077.2 (-6.5%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Shadow Bolt] Crit rate mismatch: CPU=22.1% vs WebGPU=231494.8%
- ⚠️ [Corruption] Cast count mismatch: CPU=29.46 vs WebGPU=27.63 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Corruption] Damage per hit mismatch: CPU=258.2 vs WebGPU=226.6 (-12.3%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Corruption] Crit rate mismatch: CPU=22.2% vs WebGPU=0.0%
- ⚠️ [Corruption] Miss rate mismatch: CPU=5.0% vs WebGPU=0.0%
- ⚠️ [Curse of Agony] Cast count mismatch: CPU=1.52 vs WebGPU=27.79 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Curse of Agony] Damage per hit mismatch: CPU=152.9 vs WebGPU=141.7 (-7.3%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Curse of Agony] Crit rate mismatch: CPU=22.1% vs WebGPU=0.0%
- ⚠️ [Curse of Agony] Miss rate mismatch: CPU=5.0% vs WebGPU=0.0%
- ⚠️ [Curse of Doom] Cast count mismatch: CPU=9.48 vs WebGPU=0.00 (Check GCD/cast time/haste/clipping logic)
- ⚠️ [Curse of Doom] Damage per hit mismatch: CPU=5998.8 vs WebGPU=0.0 (-100.0%) (Check base damage/SP coeff/talents/school multiplier)
- ⚠️ [Curse of Doom] Crit rate mismatch: CPU=22.0% vs WebGPU=0.0%
- ⚠️ [Curse of Doom] Miss rate mismatch: CPU=5.0% vs WebGPU=0.0%
- ⚠️ [Immolate] Cast count mismatch: CPU=0.00 vs WebGPU=29.04 (Check GCD/cast time/haste/clipping logic)


---

