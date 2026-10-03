# High-Fidelity WebGL2 vs CPU DES Simulation Parity Report

**Generated:** 2026-10-02 16:38:10 UTC  
**Status:** PASS (100%)  
**Total Execution Wall Time:** 3470.50 ms  
**Simulated Test Cases:** 47

## Overall Summary Table

| # | Fixture Name | Status | WebGL2 Dmg | Native CPU Dmg | GPU Time | Delta Diagnostics |
|---|--------------|--------|------------|----------------|----------|-------------------|
| 1 | shadow baseline | ✅ PASS | 69045.2 | 69045.2 | 288.5ms | Exact match |
| 2 | seed zero | ✅ PASS | 65358.5 | 65358.6 | 62.2ms | Exact match |
| 3 | largest seed | ✅ PASS | 65306.6 | 65306.6 | 63.7ms | Exact match |
| 4 | bolt only | ✅ PASS | 52585.2 | 52585.2 | 66.1ms | Exact match |
| 5 | charged ISB | ✅ PASS | 63988.1 | 63988.1 | 64.4ms | Exact match |
| 6 | no ISB | ✅ PASS | 61213.6 | 61213.6 | 65.8ms | Exact match |
| 7 | no Nightfall | ✅ PASS | 63772.6 | 63772.6 | 68.5ms | Exact match |
| 8 | hardcast Corruption | ✅ PASS | 68759.1 | 68759.1 | 69.6ms | Exact match |
| 9 | book ranks | ✅ PASS | 65241.3 | 65241.3 | 70.2ms | Exact match |
| 10 | positive resistance | ✅ PASS | 60147.9 | 60147.9 | 68.4ms | Exact match |
| 11 | partial disabled | ✅ PASS | 69045.2 | 69045.2 | 68.2ms | Exact match |
| 12 | negative resistance | ✅ PASS | 94814.6 | 94814.6 | 74.6ms | Exact match |
| 13 | piercing disabled | ✅ PASS | 69045.2 | 69045.2 | 65.7ms | Exact match |
| 14 | no travel | ✅ PASS | 70986.8 | 70986.8 | 66.5ms | Exact match |
| 15 | overlapping missiles | ✅ PASS | 61505.0 | 61505.0 | 66.3ms | Exact match |
| 16 | fire | ✅ PASS | 58351.4 | 58351.4 | 68.6ms | Exact match |
| 17 | fire with shadow dots | ✅ PASS | 74575.4 | 74575.4 | 66.4ms | Exact match |
| 18 | searing | ✅ PASS | 44666.4 | 44666.4 | 67.2ms | Exact match |
| 19 | fire resists | ✅ PASS | 67944.4 | 67944.4 | 73.7ms | Exact match |
| 20 | low mana | ✅ PASS | 64744.3 | 64744.4 | 64.9ms | Exact match |
| 21 | all taps | ✅ PASS | 0.0 | 0.0 | 67.3ms | Exact match |
| 22 | no improved tap | ✅ PASS | 67394.9 | 67395.0 | 68.6ms | Exact match |
| 23 | no ruin | ✅ PASS | 64078.7 | 64078.7 | 63.1ms | Exact match |
| 24 | guaranteed crits | ✅ PASS | 107904.0 | 107904.0 | 64.6ms | Exact match |
| 25 | hit cap | ✅ PASS | 72709.6 | 72709.6 | 66.2ms | Exact match |
| 26 | zero spell power | ✅ PASS | 26309.7 | 26309.7 | 63.5ms | Exact match |
| 27 | one second cutoff | ✅ PASS | 0.0 | 0.0 | 63.4ms | Exact match |
| 28 | cast cutoff | ✅ PASS | 685.2 | 685.2 | 79.3ms | Exact match |
| 29 | regen and end tie | ✅ PASS | 285.2 | 285.2 | 69.4ms | Exact match |
| 30 | dot and end tie | ✅ PASS | 9879.3 | 9879.3 | 67.2ms | Exact match |
| 31 | long fight | ✅ PASS | 660534.6 | 660533.5 | 92.9ms | Exact match |
| 32 | no dots | ✅ PASS | 52585.2 | 52585.2 | 66.9ms | Exact match |
| 33 | demonic sacrifice succubus | ✅ PASS | 79402.0 | 79402.0 | 66.6ms | Exact match |
| 34 | demonic sacrifice imp fire | ✅ PASS | 82564.5 | 82564.5 | 66.8ms | Exact match |
| 35 | master demonologist shadow | ✅ PASS | 75949.7 | 75949.7 | 66.7ms | Exact match |
| 36 | master demonologist fire | ✅ PASS | 79901.5 | 79901.5 | 65.6ms | Exact match |
| 37 | shadow mastery 5/5 | ✅ PASS | 75949.7 | 75949.7 | 66.1ms | Exact match |
| 38 | fire emberstorm 5/5 | ✅ PASS | 79901.5 | 79901.5 | 82.6ms | Exact match |
| 39 | active trinket 175 SP | ✅ PASS | 72105.7 | 72105.7 | 64.8ms | Exact match |
| 40 | shadow seed 1 | ✅ PASS | 66928.0 | 66928.1 | 65.7ms | Exact match |
| 41 | shadow seed 7 | ✅ PASS | 69303.2 | 69303.2 | 90.8ms | Exact match |
| 42 | shadow seed 1337 | ✅ PASS | 64906.1 | 64906.1 | 65.4ms | Exact match |
| 43 | shadow seed 9001 | ✅ PASS | 67028.8 | 67028.9 | 67.7ms | Exact match |
| 44 | shadow seed 43 | ✅ PASS | 70525.7 | 70525.7 | 65.8ms | Exact match |
| 45 | shadow seed 44 | ✅ PASS | 70282.7 | 70282.8 | 75.7ms | Exact match |
| 46 | shadow seed 45 | ✅ PASS | 69011.6 | 69011.6 | 76.4ms | Exact match |
| 47 | shadow seed 46 | ✅ PASS | 66892.4 | 66892.5 | 66.3ms | Exact match |

## Detailed Per-Spell Casts & Damage Breakdowns

### 1. shadow baseline (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 44816.3 | 44816.3 | +0.00 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 12140.8 | 12140.8 | +0.01 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 12088.1 | 12088.1 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 2. seed zero (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 42067.0 | 42067.0 | -0.01 | 50 / 50 | 45 / 45 | 11 / 11 | 4 / 4 |
| Corruption | 11242.8 | 11242.8 | +0.01 | 9 / 9 | 52 / 52 | 15 / 15 | 0 / 0 |
| Bane of Agony | 12048.8 | 12048.8 | +0.00 | 8 / 8 | 87 / 87 | 22 / 22 | 0 / 0 |

### 3. largest seed (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 41858.4 | 41858.4 | +0.01 | 50 / 50 | 44 / 44 | 12 / 12 | 6 / 6 |
| Corruption | 11208.2 | 11208.2 | +0.01 | 9 / 9 | 54 / 54 | 8 / 8 | 0 / 0 |
| Bane of Agony | 12240.0 | 12240.0 | +0.00 | 8 / 8 | 88 / 88 | 26 / 26 | 0 / 0 |

### 4. bolt only (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 52585.2 | 52585.2 | +0.01 | 59 / 59 | 55 / 55 | 13 / 13 | 4 / 4 |

### 5. charged ISB (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 41501.7 | 41501.7 | -0.01 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 11208.2 | 11208.2 | +0.00 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 11278.1 | 11278.1 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 6. no ISB (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 39987.5 | 39987.5 | -0.01 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 10707.4 | 10707.4 | +0.00 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 10518.8 | 10518.8 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 7. no Nightfall (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 40681.7 | 40681.7 | -0.00 | 48 / 48 | 47 / 47 | 8 / 8 | 1 / 1 |
| Corruption | 10586.5 | 10586.5 | +0.01 | 11 / 11 | 54 / 54 | 6 / 6 | 1 / 1 |
| Bane of Agony | 12504.4 | 12504.4 | +0.00 | 8 / 8 | 86 / 86 | 33 / 33 | 0 / 0 |

### 8. hardcast Corruption (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 46721.9 | 46721.9 | -0.00 | 49 / 49 | 47 / 47 | 13 / 13 | 2 / 2 |
| Corruption | 10016.6 | 10016.6 | -0.00 | 9 / 9 | 49 / 49 | 15 / 15 | 0 / 0 |
| Bane of Agony | 12020.6 | 12020.6 | +0.00 | 8 / 8 | 84 / 84 | 22 / 22 | 0 / 0 |

### 9. book ranks (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 41369.1 | 41369.1 | -0.00 | 49 / 49 | 47 / 47 | 8 / 8 | 1 / 1 |
| Corruption | 12217.3 | 12217.3 | -0.00 | 9 / 9 | 54 / 54 | 10 / 10 | 0 / 0 |
| Bane of Agony | 11655.0 | 11655.0 | +0.00 | 8 / 8 | 86 / 86 | 23 / 23 | 0 / 0 |

### 10. positive resistance (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 37515.0 | 37515.0 | +0.01 | 49 / 49 | 46 / 46 | 9 / 9 | 3 / 3 |
| Corruption | 11467.3 | 11467.3 | +0.01 | 10 / 10 | 53 / 53 | 15 / 15 | 1 / 1 |
| Bane of Agony | 11165.6 | 11165.6 | +0.00 | 9 / 9 | 84 / 84 | 14 / 14 | 1 / 1 |

### 11. partial disabled (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 44816.3 | 44816.3 | +0.00 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 12140.8 | 12140.8 | +0.01 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 12088.1 | 12088.1 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 12. negative resistance (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 70585.6 | 70585.7 | -0.01 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 12140.8 | 12140.8 | +0.01 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 12088.1 | 12088.1 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 13. piercing disabled (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 44816.3 | 44816.3 | +0.00 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 12140.8 | 12140.8 | +0.01 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 12088.1 | 12088.1 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 14. no travel (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 46659.5 | 46659.5 | +0.01 | 48 / 48 | 47 / 47 | 13 / 13 | 1 / 1 |
| Corruption | 12261.7 | 12261.7 | +0.01 | 10 / 10 | 54 / 54 | 17 / 17 | 1 / 1 |
| Bane of Agony | 12065.6 | 12065.6 | +0.00 | 8 / 8 | 87 / 87 | 15 / 15 | 0 / 0 |

### 15. overlapping missiles (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 39477.1 | 39477.1 | +0.00 | 50 / 50 | 47 / 47 | 7 / 7 | 1 / 1 |
| Corruption | 11087.3 | 11087.3 | +0.01 | 10 / 10 | 53 / 53 | 14 / 14 | 1 / 1 |
| Bane of Agony | 10940.6 | 10940.6 | +0.00 | 8 / 8 | 84 / 84 | 18 / 18 | 0 / 0 |

### 16. fire (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Immolate | 9969.5 | 9969.5 | +0.00 | 10 / 10 | 60 / 60 | 12 / 12 | 0 / 0 |
| Incinerate | 48381.9 | 48381.9 | +0.00 | 63 / 63 | 59 / 59 | 11 / 11 | 4 / 4 |

### 17. fire with shadow dots (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Corruption | 10880.1 | 10880.1 | +0.00 | 10 / 10 | 54 / 54 | 18 / 18 | 0 / 0 |
| Bane of Agony | 10434.4 | 10434.4 | +0.00 | 8 / 8 | 86 / 86 | 16 / 16 | 0 / 0 |
| Immolate | 9525.0 | 9525.0 | +0.00 | 9 / 9 | 54 / 54 | 13 / 13 | 0 / 0 |
| Incinerate | 43735.9 | 43735.9 | -0.00 | 51 / 51 | 50 / 50 | 13 / 13 | 1 / 1 |

### 18. searing (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Immolate | 11163.0 | 11163.0 | +0.00 | 12 / 12 | 64 / 64 | 17 / 17 | 1 / 1 |
| Searing Pain | 33503.4 | 33503.4 | +0.01 | 90 / 90 | 86 / 86 | 15 / 15 | 3 / 3 |

### 19. fire resists (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 0.0 | 0.0 | +0.00 | 1 / 1 | 0 / 0 | 0 / 0 | 1 / 1 |
| Corruption | 10189.3 | 10189.3 | +0.00 | 10 / 10 | 54 / 54 | 10 / 10 | 0 / 0 |
| Bane of Agony | 10575.0 | 10575.0 | +0.00 | 8 / 8 | 86 / 86 | 19 / 19 | 0 / 0 |
| Immolate | 9267.5 | 9267.5 | +0.00 | 10 / 10 | 54 / 54 | 13 / 13 | 1 / 1 |
| Incinerate | 37912.6 | 37912.6 | +0.00 | 49 / 49 | 46 / 46 | 14 / 14 | 3 / 3 |

### 20. low mana (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 41832.8 | 41832.8 | +0.00 | 45 / 45 | 43 / 43 | 12 / 12 | 2 / 2 |
| Corruption | 10862.8 | 10862.8 | +0.00 | 10 / 10 | 51 / 51 | 10 / 10 | 1 / 1 |
| Bane of Agony | 12048.8 | 12048.8 | +0.00 | 8 / 8 | 84 / 84 | 24 / 24 | 1 / 1 |

### 21. all taps (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|

### 22. no improved tap (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 44025.6 | 44025.6 | +0.01 | 48 / 48 | 45 / 45 | 12 / 12 | 2 / 2 |
| Corruption | 11450.0 | 11450.0 | +0.01 | 9 / 9 | 53 / 53 | 12 / 12 | 0 / 0 |
| Bane of Agony | 11919.4 | 11919.4 | +0.00 | 7 / 7 | 84 / 84 | 23 / 23 | 0 / 0 |

### 23. no ruin (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 39849.8 | 39849.8 | +0.01 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 12140.8 | 12140.8 | +0.01 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 12088.1 | 12088.1 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 24. guaranteed crits (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 73906.2 | 73906.2 | +0.00 | 48 / 48 | 45 / 45 | 45 / 45 | 3 / 3 |
| Corruption | 16734.6 | 16734.6 | +0.01 | 10 / 10 | 54 / 54 | 54 / 54 | 1 / 1 |
| Bane of Agony | 17263.1 | 17263.1 | +0.00 | 8 / 8 | 87 / 87 | 87 / 87 | 0 / 0 |

### 25. hit cap (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 48433.5 | 48433.5 | +0.01 | 49 / 49 | 49 / 49 | 14 / 14 | 0 / 0 |
| Corruption | 11743.6 | 11743.6 | +0.01 | 10 / 10 | 54 / 54 | 14 / 14 | 0 / 0 |
| Bane of Agony | 12532.5 | 12532.5 | +0.00 | 8 / 8 | 86 / 86 | 22 / 22 | 0 / 0 |

### 26. zero spell power (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 16959.1 | 16959.1 | -0.00 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 4407.8 | 4407.8 | +0.00 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 4942.7 | 4942.7 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 27. one second cutoff (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Bane of Agony | 0.0 | 0.0 | +0.00 | 1 / 1 | 0 / 0 | 0 / 0 | 0 / 0 |

### 28. cast cutoff (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 685.2 | 685.2 | -0.00 | 1 / 1 | 1 / 1 | 0 / 0 | 0 / 0 |

### 29. regen and end tie (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Corruption | 172.7 | 172.7 | +0.00 | 1 / 1 | 1 / 1 | 0 / 0 | 0 / 0 |
| Bane of Agony | 112.5 | 112.5 | +0.00 | 1 / 1 | 2 / 2 | 0 / 0 | 0 / 0 |

### 30. dot and end tie (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 6912.3 | 6912.3 | +0.00 | 7 / 7 | 7 / 7 | 2 / 2 | 0 / 0 |
| Corruption | 1347.1 | 1347.1 | +0.00 | 2 / 2 | 7 / 7 | 0 / 0 | 0 / 0 |
| Bane of Agony | 1620.0 | 1620.0 | +0.00 | 1 / 1 | 12 / 12 | 1 / 1 | 0 / 0 |

### 31. long fight (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 427335.1 | 427334.8 | +0.22 | 475 / 475 | 446 / 446 | 117 / 117 | 29 / 29 |
| Corruption | 115968.3 | 115968.1 | +0.29 | 96 / 96 | 537 / 537 | 132 / 132 | 6 / 6 |
| Bane of Agony | 117230.6 | 117230.6 | +0.00 | 73 / 73 | 833 / 833 | 198 / 198 | 3 / 3 |

### 32. no dots (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 52585.2 | 52585.2 | +0.01 | 59 / 59 | 55 / 55 | 13 / 13 | 4 / 4 |

### 33. demonic sacrifice succubus (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 51538.7 | 51538.7 | -0.00 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 13961.9 | 13961.9 | +0.01 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 13901.3 | 13901.3 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 34. demonic sacrifice imp fire (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Corruption | 10880.1 | 10880.1 | +0.00 | 10 / 10 | 54 / 54 | 18 / 18 | 0 / 0 |
| Bane of Agony | 10434.4 | 10434.4 | +0.00 | 8 / 8 | 86 / 86 | 16 / 16 | 0 / 0 |
| Immolate | 10953.7 | 10953.7 | -0.00 | 9 / 9 | 54 / 54 | 13 / 13 | 0 / 0 |
| Incinerate | 50296.3 | 50296.3 | -0.00 | 51 / 51 | 50 / 50 | 13 / 13 | 1 / 1 |

### 35. master demonologist shadow (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 49297.9 | 49297.9 | +0.00 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 13354.9 | 13354.9 | +0.00 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 13296.9 | 13296.9 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 36. master demonologist fire (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Corruption | 10880.1 | 10880.1 | +0.00 | 10 / 10 | 54 / 54 | 18 / 18 | 0 / 0 |
| Bane of Agony | 10434.4 | 10434.4 | +0.00 | 8 / 8 | 86 / 86 | 16 / 16 | 0 / 0 |
| Immolate | 10477.5 | 10477.5 | +0.00 | 9 / 9 | 54 / 54 | 13 / 13 | 0 / 0 |
| Incinerate | 48109.5 | 48109.5 | +0.00 | 51 / 51 | 50 / 50 | 13 / 13 | 1 / 1 |

### 37. shadow mastery 5/5 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 49297.9 | 49297.9 | +0.00 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 13354.9 | 13354.9 | +0.00 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 13296.9 | 13296.9 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 38. fire emberstorm 5/5 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Corruption | 10880.1 | 10880.1 | +0.00 | 10 / 10 | 54 / 54 | 18 / 18 | 0 / 0 |
| Bane of Agony | 10434.4 | 10434.4 | +0.00 | 8 / 8 | 86 / 86 | 16 / 16 | 0 / 0 |
| Immolate | 10477.5 | 10477.5 | +0.00 | 9 / 9 | 54 / 54 | 13 / 13 | 0 / 0 |
| Incinerate | 48109.5 | 48109.5 | +0.00 | 51 / 51 | 50 / 50 | 13 / 13 | 1 / 1 |

### 39. active trinket 175 SP (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 46886.3 | 46886.3 | +0.00 | 48 / 48 | 45 / 45 | 13 / 13 | 3 / 3 |
| Corruption | 12691.4 | 12691.4 | +0.01 | 10 / 10 | 54 / 54 | 16 / 16 | 1 / 1 |
| Bane of Agony | 12528.0 | 12528.0 | +0.00 | 8 / 8 | 87 / 87 | 18 / 18 | 0 / 0 |

### 40. shadow seed 1 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 43084.2 | 43084.2 | +0.00 | 49 / 49 | 46 / 46 | 11 / 11 | 2 / 2 |
| Corruption | 11536.4 | 11536.4 | +0.01 | 10 / 10 | 54 / 54 | 12 / 12 | 0 / 0 |
| Bane of Agony | 12307.5 | 12307.5 | +0.00 | 8 / 8 | 88 / 88 | 24 / 24 | 0 / 0 |

### 41. shadow seed 7 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 45452.2 | 45452.2 | +0.01 | 49 / 49 | 48 / 48 | 12 / 12 | 1 / 1 |
| Corruption | 11605.4 | 11605.4 | +0.00 | 10 / 10 | 54 / 54 | 14 / 14 | 0 / 0 |
| Bane of Agony | 12245.6 | 12245.6 | +0.00 | 8 / 8 | 86 / 86 | 26 / 26 | 0 / 0 |

### 42. shadow seed 1337 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 40622.9 | 40622.9 | +0.00 | 50 / 50 | 46 / 46 | 9 / 9 | 4 / 4 |
| Corruption | 12054.5 | 12054.5 | +0.00 | 9 / 9 | 54 / 54 | 19 / 19 | 0 / 0 |
| Bane of Agony | 12228.8 | 12228.8 | +0.00 | 8 / 8 | 87 / 87 | 28 / 28 | 0 / 0 |

### 43. shadow seed 9001 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 43262.3 | 43262.3 | +0.00 | 49 / 49 | 48 / 48 | 10 / 10 | 0 / 0 |
| Corruption | 11847.2 | 11847.2 | +0.01 | 10 / 10 | 55 / 55 | 15 / 15 | 0 / 0 |
| Bane of Agony | 11919.4 | 11919.4 | +0.00 | 8 / 8 | 86 / 86 | 23 / 23 | 0 / 0 |

### 44. shadow seed 43 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 47397.2 | 47397.2 | -0.01 | 49 / 49 | 47 / 47 | 14 / 14 | 2 / 2 |
| Corruption | 11001.0 | 11001.0 | +0.01 | 11 / 11 | 50 / 50 | 12 / 12 | 2 / 2 |
| Bane of Agony | 12127.5 | 12127.5 | +0.00 | 8 / 8 | 84 / 84 | 24 / 24 | 0 / 0 |

### 45. shadow seed 44 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 45825.9 | 45825.9 | -0.00 | 48 / 48 | 47 / 47 | 13 / 13 | 1 / 1 |
| Corruption | 11778.1 | 11778.1 | +0.00 | 11 / 11 | 54 / 54 | 14 / 14 | 1 / 1 |
| Bane of Agony | 12678.8 | 12678.8 | +0.00 | 8 / 8 | 86 / 86 | 25 / 25 | 0 / 0 |

### 46. shadow seed 45 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 44634.1 | 44634.1 | +0.00 | 49 / 49 | 47 / 47 | 12 / 12 | 2 / 2 |
| Corruption | 12244.4 | 12244.4 | +0.01 | 10 / 10 | 54 / 54 | 18 / 18 | 1 / 1 |
| Bane of Agony | 12133.1 | 12133.1 | +0.00 | 7 / 7 | 84 / 84 | 23 / 23 | 0 / 0 |

### 47. shadow seed 46 (PASS)

| Spell | GPU Damage | CPU Damage | Delta | Casts (G/C) | Hits (G/C) | Crits (G/C) | Misses (G/C) |
|-------|------------|------------|-------|-------------|------------|-------------|--------------|
| Shadow Bolt | 44326.2 | 44326.2 | +0.01 | 49 / 49 | 48 / 48 | 11 / 11 | 1 / 1 |
| Corruption | 11260.0 | 11260.0 | +0.00 | 10 / 10 | 52 / 52 | 14 / 14 | 1 / 1 |
| Bane of Agony | 11306.2 | 11306.2 | +0.00 | 9 / 9 | 84 / 84 | 15 / 15 | 1 / 1 |

