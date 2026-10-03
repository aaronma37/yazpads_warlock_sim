# WebGL2 vs Authoritative C++ CPU Oracle: Spec Presets Parity Report

**Generated:** 2026-10-02 18:42:03 UTC  
**Total Presets Tested:** 22  
**Passed Presets (<= 1.5% tolerance):** 7 / 22

## Summary Table

| # | Spec Preset Name | Status | WebGL DPS | Native CPU DPS | DPS Delta | Pet DPS | Diagnostics |
|---|------------------|--------|-----------|----------------|-----------|---------|-------------|
| 1 | 5/11/35 DS/AF DS-Imp | ❌ FAIL | 531.5 | 517.0 | +14.5 (+2.8%) | 0.0 | total: GPU 95671.2734375, CPU 93055.81173309695<br>mana: GPU 1979, CPU 2250.95<br>spent: GPU 24110, CPU 22772<br>gained: GPU 19656, CPU 17625<br>taps: GPU 28, CPU 25<br>isbConsumed: GPU 35, CPU 28<br>damage0: GPU 40279.47265625, CPU 30242.587367564734<br>casts0: GPU 34, CPU 26<br>hits0: GPU 34, CPU 26<br>crits0: GPU 10, CPU 6<br>damage1: GPU 11638.2578125, CPU 11645.96949999999<br>casts1: GPU 9, CPU 8<br>hits1: GPU 49, CPU 47<br>crits1: GPU 11, CPU 13<br>damage2: GPU 14384.8525390625, CPU 2775.09375<br>casts2: GPU 4, CPU 2<br>hits2: GPU 22, CPU 19<br>crits2: GPU 6, CPU 5<br>damage3: GPU 29368.69921875, CPU 13529.814999999993<br>casts3: GPU 26, CPU 11<br>hits3: GPU 76, CPU 62<br>crits3: GPU 27, CPU 13<br>Next RNG output differs: 1716409947,1157202318 / 2211600309,251840177<br>First differing damage event 0: {"time":1.5,"kind":1,"spell":3,"damage":924.7313842773438,"mana":5798,"flags":0,"total":924.7313842773438,"rngCalls":6} / [1.5,8,380.25,0] |
| 2 | 9/11/31 Incinerate - Suppression + DS | ❌ FAIL | 520.1 | 537.6 | -17.6 (-3.3%) | 0.0 | total: GPU 93612.7265625, CPU 96771.8039457946<br>mana: GPU 2013, CPU 1902.1999999999978<br>spent: GPU 25450, CPU 25828.74999999998<br>gained: GPU 21030, CPU 20333<br>taps: GPU 30, CPU 29<br>damage0: GPU 6476.23583984375, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 1, CPU 0<br>damage1: GPU 9978.609375, CPU 10798.325999999995<br>hits1: GPU 49, CPU 50<br>crits1: GPU 9, CPU 12<br>damage2: GPU 11047.712890625, CPU 3186.5625000000005<br>casts2: GPU 4, CPU 2<br>crits2: GPU 7, CPU 6<br>damage3: GPU 35652.8359375, CPU 16371.762250000014<br>casts3: GPU 28, CPU 11<br>hits3: GPU 78, CPU 61<br>crits3: GPU 25, CPU 13<br>damage4: GPU 30457.359375, CPU 34278.45746655826<br>casts4: GPU 27, CPU 30<br>hits4: GPU 27, CPU 30<br>crits4: GPU 5, CPU 6<br>Next RNG output differs: 2836344487,3531707767 / 1692951165,1603835672<br>First differing damage event 0: {"time":1.5,"kind":1,"spell":3,"damage":1063.4410400390625,"mana":5798,"flags":0,"total":1063.4410400390625,"rngCalls":6} / [1.5,8,538.1999999999999,0] |
| 3 | 7/11/33 Incinerate - Suppression + DS (No Corruption) | ❌ FAIL | 523.5 | 524.8 | -1.3 (-0.2%) | 0.0 | total: GPU 94233.375, CPU 94464.71441378057<br>mana: GPU 2197.79736328125, CPU 1994.650000000002<br>spent: GPU 26870, CPU 24808.5<br>gained: GPU 22634.806640625, CPU 19405.2<br>taps: GPU 27, CPU 23<br>damage0: GPU 6492.5625, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 1, CPU 0<br>damage3: GPU 34820.12109375, CPU 16799.86987500001<br>casts3: GPU 27, CPU 10<br>hits3: GPU 77, CPU 60<br>crits3: GPU 28, CPU 16<br>damage4: GPU 52920.73046875, CPU 53284.26018353092<br>casts4: GPU 42, CPU 45<br>hits4: GPU 42, CPU 45<br>crits4: GPU 14, CPU 13<br>Next RNG output differs: 311579884,393305375 / 406920323,1934243443<br>First differing damage event 0: {"time":1.5,"kind":1,"spell":3,"damage":1063.4410400390625,"mana":5798,"flags":0,"total":1063.4410400390625,"rngCalls":6} / [1.5,8,538.1999999999999,0] |
| 4 | 3/17/31 Incinerate - DS + Decimate | ❌ FAIL | 484.6 | 485.1 | -0.5 (-0.1%) | 0.0 | total: GPU 87223.5078125, CPU 87313.55097519617<br>mana: GPU 1767, CPU 2436.9500000000007<br>spent: GPU 25019, CPU 23283.000000000004<br>gained: GPU 20353, CPU 18322<br>taps: GPU 29, CPU 26<br>damage0: GPU 7109.6103515625, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 2, CPU 0<br>damage3: GPU 41716.83203125, CPU 15664.89031250001<br>casts3: GPU 32, CPU 10<br>hits3: GPU 80, CPU 58<br>crits3: GPU 23, CPU 16<br>damage4: GPU 36481.6328125, CPU 36514.03766375567<br>casts4: GPU 31, CPU 32<br>crits4: GPU 7, CPU 8<br>misses4: GPU 0, CPU 1<br>damage5: GPU 1915.45556640625, CPU 2181.815406729962<br>casts5: GPU 3, CPU 5<br>hits5: GPU 3, CPU 5<br>crits5: GPU 1, CPU 0<br>Next RNG output differs: 144207672,345393269 / 2910000209,3323117351<br>First differing damage event 0: {"time":1.5,"kind":1,"spell":3,"damage":1063.4410400390625,"mana":5798,"flags":0,"total":1063.4410400390625,"rngCalls":6} / [1.5,8,437.28749999999997,0] |
| 5 | 5/11/35 DS/Searing Pain DS-Succ | ❌ FAIL | 510.7 | 512.3 | -1.6 (-0.3%) | 0.0 | total: GPU 91919.8046875, CPU 92209.18435039344<br>mana: GPU 2099, CPU 1530.4500000000053<br>spent: GPU 22656, CPU 21461.500000000015<br>gained: GPU 18322, CPU 15594<br>taps: GPU 26, CPU 22<br>damage0: GPU 6534.93408203125, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 1, CPU 0<br>damage1: GPU 9239.4541015625, CPU 9057.949999999999<br>casts1: GPU 9, CPU 8<br>hits1: GPU 48, CPU 46<br>crits1: GPU 11, CPU 10<br>damage2: GPU 13477.0625, CPU 3155.6250000000005<br>casts2: GPU 4, CPU 3<br>hits2: GPU 26, CPU 24<br>crits2: GPU 4, CPU 3<br>damage3: GPU 34545.609375, CPU 17918.541<br>casts3: GPU 27, CPU 11<br>hits3: GPU 78, CPU 62<br>crits3: GPU 22, CPU 17<br>damage5: GPU 28122.740234375, CPU 27305.844797540536<br>casts5: GPU 42, CPU 45<br>hits5: GPU 42, CPU 45<br>crits5: GPU 19, CPU 16<br>Next RNG output differs: 406920323,1934243443 / 1194327487,3317135259<br>First differing damage event 0: {"time":1.5,"kind":1,"spell":3,"damage":1063.4410400390625,"mana":5798,"flags":0,"total":1063.4410400390625,"rngCalls":6} / [1.5,8,538.1999999999999,0] |
| 6 | 2/31/18 DP/AF Shadow | ❌ FAIL | 585.4 | 565.2 | +20.2 (+3.6%) | 47.9 | total: GPU 105371.09375, CPU 101735.05174682454<br>mana: GPU 2051, CPU 2288.95<br>spent: GPU 21350, CPU 21400<br>gained: GPU 16968, CPU 16291<br>taps: GPU 24, CPU 23<br>isbProcs: GPU 9, CPU 12<br>isbConsumed: GPU 51, CPU 58<br>petDamage: GPU 8624.4853515625, CPU 9692.377936034383<br>damage0: GPU 56752.11328125, CPU 58328.21759977165<br>crits0: GPU 9, CPU 12<br>damage1: GPU 13233.9326171875, CPU 13308.722184999993<br>casts1: GPU 9, CPU 11<br>hits1: GPU 50, CPU 48<br>crits1: GPU 11, CPU 9<br>misses1: GPU 0, CPU 2<br>damage2: GPU 15472.27734375, CPU 3335.81259<br>casts2: GPU 5, CPU 2<br>hits2: GPU 23, CPU 20<br>crits2: GPU 5, CPU 4<br>misses2: GPU 1, CPU 0<br>damage3: GPU 11288.234375, CPU 0<br>casts3: GPU 7, CPU 0<br>hits3: GPU 7, CPU 0<br>crits3: GPU 3, CPU 0<br>Next RNG output differs: 3481551094,1553824663 / 2687673330,2764504330<br>First differing damage event 0: {"time":0.5,"kind":6,"spell":201,"damage":81.4000015258789,"mana":6433,"flags":0,"total":81.4000015258789,"rngCalls":2} / [7,2,265.80179999999996,0] |
| 7 | 2/31/18 DP/AF Shadow Corruption | ❌ FAIL | 543.4 | 561.8 | -18.4 (-3.3%) | 50.6 | total: GPU 97807.984375, CPU 101120.00000791441<br>mana: GPU 2061, CPU 2495.95<br>spent: GPU 21320, CPU 21850<br>isbProcs: GPU 9, CPU 10<br>isbConsumed: GPU 52, CPU 46<br>petDamage: GPU 9107.9619140625, CPU 9948.199822770204<br>damage0: GPU 51538.5390625, CPU 55654.96310488562<br>casts0: GPU 41, CPU 43<br>hits0: GPU 35, CPU 40<br>crits0: GPU 9, CPU 10<br>misses0: GPU 5, CPU 3<br>damage1: GPU 13900.6982421875, CPU 13165.163154000007<br>hits1: GPU 49, CPU 48<br>crits1: GPU 12, CPU 5<br>damage2: GPU 15495.826171875, CPU 2927.6661083999998<br>casts2: GPU 5, CPU 2<br>hits2: GPU 22, CPU 19<br>crits2: GPU 9, CPU 4<br>misses2: GPU 1, CPU 0<br>damage3: GPU 7764.951171875, CPU 0<br>casts3: GPU 6, CPU 0<br>hits3: GPU 6, CPU 0<br>crits3: GPU 1, CPU 0<br>Next RNG output differs: 3424214935,2879243950 / 2788416315,2349329614<br>First differing damage event 0: {"time":0.5,"kind":6,"spell":201,"damage":81.4000015258789,"mana":6433,"flags":0,"total":81.4000015258789,"rngCalls":2} / [5.7,2,276.433872,0] |
| 8 | 12/31/8 Aff/DP | ❌ FAIL | 546.2 | 541.8 | +4.4 (+0.8%) | 48.8 | total: GPU 98314.7734375, CPU 97515.80644751548<br>mana: GPU 2423.39794921875, CPU 2321.700000000002<br>spent: GPU 21790, CPU 22515<br>gained: GPU 17780.404296875, CPU 17760.399999999998<br>isbConsumed: GPU 50, CPU 47<br>petDamage: GPU 8784.5673828125, CPU 9407.032876125739<br>damage0: GPU 51488.90625, CPU 48714.64559354915<br>casts0: GPU 43, CPU 45<br>hits0: GPU 43, CPU 45<br>damage1: GPU 14578.025390625, CPU 14964.212930039994<br>hits1: GPU 48, CPU 52<br>crits1: GPU 15, CPU 12<br>damage2: GPU 15425.1865234375, CPU 5207.534662880001<br>casts2: GPU 4, CPU 3<br>hits2: GPU 24, CPU 25<br>crits2: GPU 7, CPU 9<br>damage3: GPU 8038.095703125, CPU 0<br>casts3: GPU 7, CPU 0<br>hits3: GPU 7, CPU 0<br>crits3: GPU 2, CPU 0<br>Next RNG output differs: 2805057535,395438610 / 2131383162,2753631304<br>First differing damage event 0: {"time":0.5,"kind":6,"spell":201,"damage":81.4000015258789,"mana":6433,"flags":0,"total":81.4000015258789,"rngCalls":2} / [3.4,2,287.065944,0] |
| 9 | 12/31/8 Aff/DP Brand | ❌ FAIL | 547.5 | 532.8 | +14.7 (+2.8%) | 91.5 | total: GPU 98554.8125, CPU 95902.80653688947<br>mana: GPU 2324.998046875, CPU 2429.6500000000033<br>spent: GPU 21076, CPU 20311.50000000001<br>gained: GPU 16968.00390625, CPU 15343.199999999995<br>taps: GPU 20, CPU 18<br>petDamage: GPU 16467.0703125, CPU 17860.266888237642<br>damage0: GPU 41044.46875, CPU 35576.4440453417<br>casts0: GPU 35, CPU 34<br>hits0: GPU 35, CPU 34<br>crits0: GPU 14, CPU 8<br>damage1: GPU 12369.6513671875, CPU 14173.69205725<br>casts1: GPU 8, CPU 9<br>hits1: GPU 48, CPU 53<br>crits1: GPU 9, CPU 11<br>damage2: GPU 13730.71875, CPU 4998.971847265<br>casts2: GPU 4, CPU 3<br>hits2: GPU 23, CPU 26<br>crits2: GPU 3, CPU 6<br>damage3: GPU 7786.4580078125, CPU 0<br>casts3: GPU 6, CPU 0<br>hits3: GPU 6, CPU 0<br>crits3: GPU 3, CPU 0<br>damage5: GPU 7156.5341796875, CPU 7039.359131953125<br>casts5: GPU 17, CPU 20<br>hits5: GPU 17, CPU 19<br>crits5: GPU 4, CPU 3<br>Next RNG output differs: 1840706597,2462478673 / 379130369,1257934734<br>First differing damage event 0: {"time":0.5,"kind":6,"spell":201,"damage":81.4000015258789,"mana":6433,"flags":0,"total":81.4000015258789,"rngCalls":2} / [3.8,2,284.40792600000003,0] |
| 10 | 0/31/20 DP/AF Fire | ❌ FAIL | 625.7 | 556.2 | +69.4 (+12.5%) | 108.3 | total: GPU 112617.1484375, CPU 100124.93997307974<br>mana: GPU 2261, CPU 3000.350000000012<br>spent: GPU 23828, CPU 18657.600000000024<br>gained: GPU 19656, CPU 14260<br>taps: GPU 28, CPU 20<br>petDamage: GPU 19488.353515625, CPU 23207.33925129313<br>damage1: GPU 9225.708984375, CPU 10957.654999999992<br>casts1: GPU 8, CPU 12<br>hits1: GPU 47, CPU 56<br>crits1: GPU 12, CPU 11<br>misses1: GPU 0, CPU 2<br>damage2: GPU 11491.4208984375, CPU 0<br>casts2: GPU 5, CPU 0<br>hits2: GPU 26, CPU 0<br>crits2: GPU 4, CPU 0<br>damage3: GPU 15724.998046875, CPU 16622.57681012499<br>hits3: GPU 58, CPU 62<br>crits3: GPU 16, CPU 13<br>misses3: GPU 2, CPU 0<br>damage4: GPU 47982.26953125, CPU 0<br>casts4: GPU 37, CPU 0<br>hits4: GPU 35, CPU 0<br>crits4: GPU 10, CPU 0<br>misses4: GPU 2, CPU 0<br>damage5: GPU 8704.47265625, CPU 41134.44891166154<br>casts5: GPU 16, CPU 68<br>hits5: GPU 14, CPU 64<br>crits5: GPU 3, CPU 20<br>misses5: GPU 2, CPU 4<br>Next RNG output differs: 2705813499,2624871404 / 826826535,1774663367<br>First differing damage event 0: {"time":0.3,"kind":6,"spell":100,"damage":108.67999267578125,"mana":6433,"flags":0,"total":108.67999267578125,"rngCalls":3} / [5,8,515.7727574999999,0] |
| 11 | 2/31/18 DP Fire - Searing Pain | ❌ FAIL | 607.2 | 601.4 | +5.8 (+1.0%) | 117.9 | total: GPU 109293.2734375, CPU 108251.9247391429<br>mana: GPU 2063, CPU 2926.750000000011<br>spent: GPU 19984, CPU 19408.20000000002<br>gained: GPU 15614, CPU 14937<br>taps: GPU 22, CPU 21<br>petDamage: GPU 21222.333984375, CPU 23932.660244529387<br>damage1: GPU 9410.2255859375, CPU 11727.847800000009<br>casts1: GPU 10, CPU 12<br>hits1: GPU 48, CPU 57<br>crits1: GPU 10, CPU 15<br>misses1: GPU 1, CPU 2<br>damage2: GPU 13107.822265625, CPU 0<br>casts2: GPU 4, CPU 0<br>hits2: GPU 23, CPU 0<br>crits2: GPU 9, CPU 0<br>damage3: GPU 28777.052734375, CPU 17924.03493237499<br>casts3: GPU 18, CPU 12<br>hits3: GPU 69, CPU 66<br>damage5: GPU 36775.890625, CPU 37962.521804376316<br>casts5: GPU 58, CPU 61<br>hits5: GPU 54, CPU 59<br>crits5: GPU 17, CPU 18<br>misses5: GPU 4, CPU 1<br>Next RNG output differs: 2393764084,3728945019 / 3699401648,863222301<br>First differing damage event 0: {"time":0.3,"kind":6,"spell":100,"damage":108.67999267578125,"mana":6433,"flags":0,"total":108.67999267578125,"rngCalls":3} / [4.6,8,515.7727574999999,0] |
| 12 | 40/11/0 Deep Affliction DS-Imp | ❌ FAIL | 390.7 | 386.6 | +4.1 (+1.1%) | 0.0 | total: GPU 70329.703125, CPU 69583.96891787634<br>mana: GPU 1827.19921875, CPU 1979.7500000000005<br>gained: GPU 7199.19970703125, CPU 6386.799999999999<br>taps: GPU 8, CPU 7<br>damage1: GPU 56173.0078125, CPU 14324.951990526311<br>casts1: GPU 39, CPU 9<br>hits1: GPU 241, CPU 52<br>crits1: GPU 72, CPU 8<br>damage2: GPU 14156.7138671875, CPU 4783.755621671944<br>casts2: GPU 5, CPU 3<br>hits2: GPU 26, CPU 25<br>Next RNG output differs: 2684971629,233027422 / 1880423486,3369377268<br>First differing damage event 0: {"time":3,"kind":3,"spell":1,"damage":218.01414489746094,"mana":5843,"flags":0,"total":218.01414489746094,"rngCalls":3} / [3,2,264.96000000000004,0] |
| 13 | 35/6/10 Deep Affliction Imp | ❌ FAIL | 392.7 | 359.4 | +33.3 (+9.3%) | 31.6 | total: GPU 70694.34375, CPU 64700.867269901675<br>mana: GPU 2139.59912109375, CPU 1484.6000000000008<br>spent: GPU 12305, CPU 12960<br>procs: GPU 2, CPU 5<br>isbProcs: GPU 0, CPU 1<br>isbConsumed: GPU 0, CPU 19<br>petDamage: GPU 5690.76025390625, CPU 5938.114285714286<br>damage0: GPU 1426.116455078125, CPU 3685.2904722024673<br>casts0: GPU 2, CPU 5<br>hits0: GPU 2, CPU 5<br>crits0: GPU 0, CPU 1<br>damage1: GPU 46834.5234375, CPU 12952.150986242093<br>casts1: GPU 38, CPU 9<br>hits1: GPU 237, CPU 50<br>crits1: GPU 74, CPU 14<br>damage2: GPU 16742.931640625, CPU 3762.281245275<br>casts2: GPU 5, CPU 2<br>hits2: GPU 28, CPU 23<br>Next RNG output differs: 2755163908,962891789 / 4158923176,1598654583<br>First differing damage event 0: {"time":0.3,"kind":6,"spell":100,"damage":98.11656951904297,"mana":6143,"flags":0,"total":98.11656951904297,"rngCalls":4} / [3,2,220.8,0] |
| 14 | 32/0/19 SM/AF | ❌ FAIL | 549.0 | 481.5 | +67.6 (+14.0%) | 38.3 | total: GPU 98826.5546875, CPU 86662.35232229735<br>mana: GPU 1768.19775390625, CPU 2425.600000000002<br>spent: GPU 24070, CPU 24225<br>gained: GPU 19405.205078125, CPU 20217.600000000002<br>taps: GPU 23, CPU 24<br>procs: GPU 2, CPU 4<br>isbProcs: GPU 14, CPU 11<br>isbConsumed: GPU 85, CPU 68<br>petDamage: GPU 6894.2822265625, CPU 1878.6428571428576<br>damage0: GPU 58360.95703125, CPU 45951.19995688165<br>casts0: GPU 54, CPU 44<br>hits0: GPU 54, CPU 44<br>crits0: GPU 15, CPU 11<br>damage1: GPU 15166.2041015625, CPU 15655.679981366391<br>hits1: GPU 54, CPU 55<br>damage2: GPU 18405.189453125, CPU 3551.62499690625<br>casts2: GPU 5, CPU 2<br>hits2: GPU 27, CPU 23<br>Next RNG output differs: 3793595972,301269332 / 1807897608,781102792<br>First differing damage event 0: {"time":0.3,"kind":6,"spell":100,"damage":72.57142639160156,"mana":6143,"flags":0,"total":72.57142639160156,"rngCalls":4} / [3,2,230.40000000000003,0] |
| 15 | 1/17/33 Incinerate - Decimate + Imp | ❌ FAIL | 516.4 | 498.7 | +17.7 (+3.6%) | 54.5 | total: GPU 92956.5, CPU 89763.62294210352<br>mana: GPU 2224, CPU 2268.95<br>spent: GPU 24562, CPU 24785<br>gained: GPU 20353, CPU 19656<br>taps: GPU 29, CPU 28<br>petDamage: GPU 9806.9462890625, CPU 8993.600000000006<br>damage0: GPU 5920.455078125, CPU 0<br>casts0: GPU 8, CPU 0<br>hits0: GPU 7, CPU 0<br>crits0: GPU 3, CPU 0<br>misses0: GPU 1, CPU 0<br>damage1: GPU 9239.453125, CPU 9508.949999999999<br>hits1: GPU 48, CPU 44<br>crits1: GPU 11, CPU 19<br>misses1: GPU 0, CPU 1<br>damage2: GPU 10243.337890625, CPU 3712.5000000000005<br>casts2: GPU 4, CPU 2<br>hits2: GPU 19, CPU 24<br>crits2: GPU 3, CPU 12<br>damage3: GPU 36019.96875, CPU 11284.459999999995<br>casts3: GPU 33, CPU 10<br>hits3: GPU 80, CPU 59<br>crits3: GPU 25, CPU 7<br>misses3: GPU 2, CPU 0<br>damage4: GPU 20120.06640625, CPU 19181.311226181955<br>hits4: GPU 20, CPU 19<br>misses4: GPU 0, CPU 1<br>damage5: GPU 1606.33203125, CPU 2629.854442202998<br>casts5: GPU 4, CPU 5<br>hits5: GPU 3, CPU 5<br>crits5: GPU 1, CPU 2<br>misses5: GPU 1, CPU 0<br>Next RNG output differs: 1179599726,3231588270 / 2175477858,154969505<br>First differing damage event 0: {"time":0.3,"kind":6,"spell":100,"damage":103.77713775634766,"mana":6433,"flags":0,"total":103.77713775634766,"rngCalls":3} / [1.5,8,241.3125,0] |
| 16 | 10/10/31 Incinerate - Suppression + Imp | ❌ FAIL | 561.9 | 551.3 | +10.6 (+1.9%) | 56.8 | total: GPU 101138.2421875, CPU 99238.91658226802<br>mana: GPU 1960.3974609375, CPU 1316.600000000001<br>spent: GPU 26315, CPU 26958.79999999998<br>petDamage: GPU 10222.0576171875, CPU 9073.300000000005<br>damage0: GPU 7144.96484375, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 2, CPU 0<br>damage1: GPU 9336.162109375, CPU 10221.739000000001<br>casts1: GPU 8, CPU 9<br>hits1: GPU 46, CPU 48<br>crits1: GPU 10, CPU 13<br>damage2: GPU 15658.9130859375, CPU 2784.3750000000005<br>casts2: GPU 4, CPU 2<br>hits2: GPU 26, CPU 20<br>crits2: GPU 8, CPU 9<br>damage3: GPU 28440.861328125, CPU 14515.754999999994<br>casts3: GPU 27, CPU 11<br>hits3: GPU 77, CPU 61<br>crits3: GPU 20, CPU 16<br>damage4: GPU 30335.349609375, CPU 32594.9121058259<br>casts4: GPU 31, CPU 33<br>hits4: GPU 31, CPU 33<br>crits4: GPU 6, CPU 7<br>Next RNG output differs: 92133884,834723365 / 2509432519,690523661<br>First differing damage event 0: {"time":0.3,"kind":6,"spell":100,"damage":103.77713775634766,"mana":6433,"flags":0,"total":103.77713775634766,"rngCalls":3} / [1.5,8,351,0] |
| 17 | 7/13/31 Incinerate - Suppression + Succubus | ❌ FAIL | 524.7 | 529.7 | -5.0 (-0.9%) | 44.5 | total: GPU 94453.1640625, CPU 95354.5746544007<br>mana: GPU 2018, CPU 2266.55<br>spent: GPU 25445, CPU 25484.399999999983<br>gained: GPU 21030, CPU 20353<br>taps: GPU 30, CPU 29<br>petDamage: GPU 8006.6201171875, CPU 7736.323911148263<br>damage0: GPU 6515.11669921875, CPU 0<br>casts0: GPU 11, CPU 0<br>hits0: GPU 11, CPU 0<br>damage1: GPU 9609.029296875, CPU 10508.212000000007<br>hits1: GPU 49, CPU 50<br>crits1: GPU 9, CPU 13<br>damage2: GPU 10954.900390625, CPU 2846.2500000000005<br>casts2: GPU 4, CPU 2<br>hits2: GPU 24, CPU 21<br>crits2: GPU 2, CPU 7<br>damage3: GPU 28717.072265625, CPU 13457.872499999994<br>casts3: GPU 27, CPU 11<br>hits3: GPU 76, CPU 62<br>crits3: GPU 24, CPU 11<br>damage4: GPU 30650.39453125, CPU 26792.019898112332<br>casts4: GPU 27, CPU 29<br>hits4: GPU 27, CPU 29<br>crits4: GPU 10, CPU 4<br>Next RNG output differs: 83145216,2269574781 / 450857452,1908419802<br>First differing damage event 0: {"time":0.5,"kind":6,"spell":201,"damage":78.5714340209961,"mana":6433,"flags":0,"total":78.5714340209961,"rngCalls":2} / [1.5,8,468,0] |
| 18 | 2/17/32 Shadow and Flame Shadow - Decimate | ❌ FAIL | 526.3 | 494.8 | +31.6 (+6.4%) | 54.2 | total: GPU 94739.75, CPU 89059.32429412963<br>mana: GPU 1994, CPU 2126.95<br>spent: GPU 24095, CPU 23593<br>gained: GPU 19656, CPU 18322<br>taps: GPU 28, CPU 26<br>isbProcs: GPU 4, CPU 7<br>isbConsumed: GPU 16, CPU 36<br>petDamage: GPU 9755.0576171875, CPU 9415.300000000007<br>damage0: GPU 29990.048828125, CPU 18793.15403370413<br>casts0: GPU 32, CPU 17<br>hits0: GPU 31, CPU 15<br>misses0: GPU 0, CPU 2<br>damage1: GPU 9101.29296875, CPU 9439.869999999994<br>crits1: GPU 15, CPU 11<br>damage2: GPU 12211.265625, CPU 2165.6250000000005<br>casts2: GPU 4, CPU 2<br>hits2: GPU 24, CPU 18<br>crits2: GPU 9, CPU 3<br>damage3: GPU 33682.1875, CPU 13134.59125<br>casts3: GPU 29, CPU 11<br>hits3: GPU 79, CPU 60<br>crits3: GPU 23, CPU 20<br>misses3: GPU 0, CPU 1<br>damage5: GPU 0, CPU 1815.6279381473<br>casts5: GPU 0, CPU 6<br>hits5: GPU 0, CPU 5<br>Next RNG output differs: 2227856834,258005075 / 2735815256,3691402726<br>First differing damage event 0: {"time":0.3,"kind":6,"spell":100,"damage":103.77713775634766,"mana":6433,"flags":0,"total":103.77713775634766,"rngCalls":3} / [1.5,8,241.3125,0] |
| 19 | 8/13/30 Shadow and Flame Shadow | ❌ FAIL | 574.5 | 499.3 | +75.1 (+15.0%) | 42.4 | total: GPU 103406.5625, CPU 89879.69729163282<br>mana: GPU 1775.59765625, CPU 2496.550000000002<br>spent: GPU 24855, CPU 24238.89999999998<br>gained: GPU 20197.60546875, CPU 19337.500000000007<br>taps: GPU 24, CPU 25<br>isbProcs: GPU 10, CPU 6<br>isbConsumed: GPU 39, CPU 33<br>petDamage: GPU 7635.75732421875, CPU 8145.192080515455<br>damage0: GPU 41383.0546875, CPU 29079.089639277976<br>casts0: GPU 36, CPU 28<br>hits0: GPU 36, CPU 28<br>crits0: GPU 13, CPU 6<br>damage1: GPU 10219.7001953125, CPU 11086.847200000006<br>hits1: GPU 48, CPU 49<br>crits1: GPU 8, CPU 15<br>damage2: GPU 14124.111328125, CPU 3557.8125000000005<br>casts2: GPU 4, CPU 3<br>hits2: GPU 24, CPU 25<br>crits2: GPU 1, CPU 7<br>damage3: GPU 30043.921875, CPU 12154.972499999993<br>casts3: GPU 26, CPU 10<br>hits3: GPU 76, CPU 59<br>crits3: GPU 26, CPU 16<br>Next RNG output differs: 1680986527,2989983168 / 4113453868,147554694<br>First differing damage event 0: {"time":0.5,"kind":6,"spell":201,"damage":78.5714340209961,"mana":6433,"flags":0,"total":78.5714340209961,"rngCalls":2} / [1.5,8,321.75,0] |
| 20 | 19/11/21 NF/DS/Ruin DS-Imp | ❌ FAIL | 459.1 | 482.1 | -23.0 (-4.8%) | 0.0 | total: GPU 82635.21875, CPU 86772.66329313228<br>mana: GPU 2425.59765625, CPU 1803.2500000000005<br>spent: GPU 24225, CPU 23355.099999999977<br>gained: GPU 20217.60546875, CPU 17760.399999999998<br>taps: GPU 24, CPU 21<br>procs: GPU 5, CPU 1<br>isbProcs: GPU 7, CPU 10<br>isbConsumed: GPU 55, CPU 58<br>damage0: GPU 54664.44140625, CPU 48133.85083395155<br>casts0: GPU 55, CPU 45<br>hits0: GPU 55, CPU 45<br>crits0: GPU 7, CPU 10<br>damage1: GPU 14056.7255859375, CPU 14210.526999999987<br>hits1: GPU 55, CPU 54<br>crits1: GPU 16, CPU 13<br>damage2: GPU 13914.0517578125, CPU 3844.3781249999997<br>casts2: GPU 4, CPU 2<br>hits2: GPU 25, CPU 23<br>crits2: GPU 8, CPU 6<br>Next RNG output differs: 2209789895,698999206 / 1487656629,3356081335<br>First differing damage event 0: {"time":3,"kind":3,"spell":1,"damage":207.6325225830078,"mana":5843,"flags":0,"total":207.6325225830078,"rngCalls":4} / [3,2,253.91999999999996,0] |
| 21 | 23/10/18 NF/AF | ❌ FAIL | 492.5 | 485.2 | +7.4 (+1.5%) | 53.6 | total: GPU 88654.203125, CPU 87330.63921500517<br>procs: GPU 3, CPU 1<br>isbProcs: GPU 11, CPU 16<br>isbConsumed: GPU 63, CPU 79<br>petDamage: GPU 9651.28125, CPU 9115.80000000001<br>damage0: GPU 51851.30078125, CPU 56114.095215005174<br>crits0: GPU 11, CPU 16<br>damage1: GPU 12528.6044921875, CPU 12368.657999999996<br>hits1: GPU 54, CPU 53<br>crits1: GPU 17, CPU 14<br>damage2: GPU 14623.1552734375, CPU 4089.15<br>casts2: GPU 5, CPU 3<br>hits2: GPU 28, CPU 25<br>Next RNG output differs: 851104903,83143744 / 3692821299,3725494485<br>First differing damage event 0: {"time":0.3,"kind":6,"spell":100,"damage":103.77713775634766,"mana":6143,"flags":0,"total":103.77713775634766,"rngCalls":4} / [3,2,218.88000000000002,0] |
| 22 | 13/7/31 Aff Incinerate | ❌ FAIL | 533.8 | 570.0 | -36.2 (-6.4%) | 40.7 | total: GPU 96085.375, CPU 102606.75744149319<br>mana: GPU 1960.3974609375, CPU 1768.4000000000024<br>spent: GPU 26315, CPU 25505.5<br>gained: GPU 21842.40625, CPU 20197.600000000002<br>taps: GPU 26, CPU 24<br>petDamage: GPU 7327.04833984375, CPU 7904.785470541807<br>damage0: GPU 7071.9921875, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 2, CPU 0<br>damage1: GPU 9610.75390625, CPU 10468.342499999999<br>casts1: GPU 8, CPU 9<br>hits1: GPU 46, CPU 51<br>crits1: GPU 14, CPU 9<br>damage2: GPU 11501.908203125, CPU 3717.181875000001<br>casts2: GPU 4, CPU 2<br>hits2: GPU 26, CPU 22<br>crits2: GPU 3, CPU 7<br>damage3: GPU 29221.083984375, CPU 14485.77425<br>casts3: GPU 27, CPU 11<br>hits3: GPU 77, CPU 61<br>crits3: GPU 23, CPU 15<br>damage4: GPU 31352.548828125, CPU 33442.148124151485<br>casts4: GPU 31, CPU 34<br>hits4: GPU 31, CPU 34<br>crits4: GPU 7, CPU 8<br>Next RNG output differs: 3343696371,2469145611 / 342997025,767940299<br>First differing damage event 0: {"time":0.5,"kind":6,"spell":201,"damage":77.14286041259766,"mana":6433,"flags":0,"total":77.14286041259766,"rngCalls":2} / [1.5,8,409.50000000000006,0] |

## Detailed Per-Spell & Pet Breakdowns

### 1. 5/11/35 DS/AF DS-Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 32757.0 | 30242.6 | 182.0 | 168.0 | +14.0 (+8.3%) | 25 / 26 | DIFF (+8.3%) |
| Corruption | 11638.3 | 11646.0 | 64.7 | 64.7 | -0.0 (-0.1%) | 9 / 8 | PASS |
| Bane of Agony | 2824.0 | 2775.1 | 15.7 | 15.4 | +0.3 (+1.8%) | 2 / 2 | DIFF (+1.8%) |
| Curse of Doom | 11560.9 | 11360.7 | 64.2 | 63.1 | +1.1 (+1.8%) | 2 / 2 | DIFF (+1.8%) |
| Immolate | 13207.1 | 13529.8 | 73.4 | 75.2 | -1.8 (-2.4%) | 11 / 11 | DIFF (-2.4%) |
| Conflagrate | 16161.6 | 16556.6 | 89.8 | 92.0 | -2.2 (-2.4%) | 15 / 16 | DIFF (-2.4%) |
| Shadowburn | 7522.5 | 6945.1 | 41.8 | 38.6 | +3.2 (+8.3%) | 9 / 10 | DIFF (+8.3%) |

### 2. 9/11/31 Incinerate - Suppression + DS (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 9978.6 | 10798.3 | 55.4 | 60.0 | -4.6 (-7.6%) | 9 / 9 | DIFF (-7.6%) |
| Bane of Agony | 3083.0 | 3186.6 | 17.1 | 17.7 | -0.6 (-3.3%) | 2 / 2 | DIFF (-3.3%) |
| Curse of Doom | 7964.8 | 8232.4 | 44.2 | 45.7 | -1.5 (-3.3%) | 2 / 2 | DIFF (-3.3%) |
| Immolate | 17922.6 | 16371.8 | 99.6 | 91.0 | +8.6 (+9.5%) | 11 / 11 | DIFF (+9.5%) |
| Incinerate | 30457.4 | 34278.5 | 169.2 | 190.4 | -21.2 (-11.1%) | 27 / 30 | DIFF (-11.1%) |
| Conflagrate | 17730.2 | 16196.0 | 98.5 | 90.0 | +8.5 (+9.5%) | 17 / 17 | DIFF (+9.5%) |
| Shadowburn | 6476.2 | 7708.3 | 36.0 | 42.8 | -6.8 (-16.0%) | 10 / 10 | DIFF (-16.0%) |

### 3. 7/11/33 Incinerate - Suppression + DS (No Corruption) (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Immolate | 16710.3 | 16799.9 | 92.8 | 93.3 | -0.5 (-0.5%) | 10 / 10 | PASS |
| Incinerate | 52920.7 | 53284.3 | 294.0 | 296.0 | -2.0 (-0.7%) | 42 / 45 | PASS |
| Conflagrate | 18109.8 | 18206.9 | 100.6 | 101.1 | -0.5 (-0.5%) | 17 / 16 | PASS |
| Shadowburn | 6492.6 | 6173.7 | 36.1 | 34.3 | +1.8 (+5.2%) | 10 / 10 | DIFF (+5.2%) |

### 4. 3/17/31 Incinerate - DS + Decimate (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Immolate | 15170.1 | 15664.9 | 84.3 | 87.0 | -2.7 (-3.2%) | 10 / 10 | DIFF (-3.2%) |
| Incinerate | 36481.6 | 36514.0 | 202.7 | 202.9 | -0.2 (-0.1%) | 31 / 32 | PASS |
| Searing Pain | 1915.5 | 2181.8 | 10.6 | 12.1 | -1.5 (-12.2%) | 3 / 5 | DIFF (-12.2%) |
| Conflagrate | 16822.4 | 17371.1 | 93.5 | 96.5 | -3.0 (-3.2%) | 15 / 15 | DIFF (-3.2%) |
| Soul Fire | 9724.3 | 10041.5 | 54.0 | 55.8 | -1.8 (-3.2%) | 7 / 7 | DIFF (-3.2%) |
| Shadowburn | 7109.6 | 5540.2 | 39.5 | 30.8 | +8.7 (+28.3%) | 10 / 10 | DIFF (+28.3%) |

### 5. 5/11/35 DS/Searing Pain DS-Succ (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 9239.5 | 9057.9 | 51.3 | 50.3 | +1.0 (+2.0%) | 9 / 8 | DIFF (+2.0%) |
| Bane of Agony | 3425.0 | 3155.6 | 19.0 | 17.5 | +1.5 (+8.5%) | 2 / 3 | DIFF (+8.5%) |
| Curse of Doom | 10052.1 | 9261.5 | 55.8 | 51.5 | +4.4 (+8.5%) | 2 / 2 | DIFF (+8.5%) |
| Immolate | 17071.7 | 17918.5 | 94.8 | 99.5 | -4.7 (-4.7%) | 11 / 11 | DIFF (-4.7%) |
| Searing Pain | 28122.7 | 27305.8 | 156.2 | 151.7 | +4.5 (+3.0%) | 42 / 45 | DIFF (+3.0%) |
| Conflagrate | 17473.9 | 18340.8 | 97.1 | 101.9 | -4.8 (-4.7%) | 16 / 16 | DIFF (-4.7%) |
| Shadowburn | 6534.9 | 7169.0 | 36.3 | 39.8 | -3.5 (-8.8%) | 10 / 11 | DIFF (-8.8%) |

### 6. 2/31/18 DP/AF Shadow (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 56752.1 | 58328.2 | 315.3 | 324.0 | -8.8 (-2.7%) | 41 / 41 | DIFF (-2.7%) |
| Corruption | 13233.9 | 13308.7 | 73.5 | 73.9 | -0.4 (-0.6%) | 9 / 11 | PASS |
| Bane of Agony | 3626.7 | 3335.8 | 20.1 | 18.5 | +1.6 (+8.7%) | 2 / 2 | DIFF (+8.7%) |
| Curse of Doom | 11845.6 | 10895.5 | 65.8 | 60.5 | +5.3 (+8.7%) | 2 / 2 | DIFF (+8.7%) |
| Soul Fire | 11288.2 | 6174.4 | 62.7 | 34.3 | +28.4 (+82.8%) | 7 / 6 | DIFF (+82.8%) |
| Succubus Melee (Pet) | 6446.4 | 7244.6 | 35.8 | 40.2 | -4.4 (-11.0%) | 0 / 90 | DIFF (-11.0%) |
| Succubus Lash of Pain (Pet) | 2178.0 | 2447.7 | 12.1 | 13.6 | -1.5 (-11.0%) | 0 / 15 | DIFF (-11.0%) |

### 7. 2/31/18 DP/AF Shadow Corruption (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 51538.5 | 55655.0 | 286.3 | 309.2 | -22.9 (-7.4%) | 41 / 43 | DIFF (-7.4%) |
| Corruption | 13900.7 | 13165.2 | 77.2 | 73.1 | +4.1 (+5.6%) | 10 / 10 | DIFF (+5.6%) |
| Bane of Agony | 2949.8 | 2927.7 | 16.4 | 16.3 | +0.1 (+0.8%) | 2 / 2 | PASS |
| Curse of Doom | 12546.1 | 12452.0 | 69.7 | 69.2 | +0.5 (+0.8%) | 2 / 2 | PASS |
| Soul Fire | 7765.0 | 6972.0 | 43.1 | 38.7 | +4.4 (+11.4%) | 6 / 6 | DIFF (+11.4%) |
| Succubus Melee (Pet) | 6688.7 | 7305.7 | 37.2 | 40.6 | -3.4 (-8.4%) | 0 / 90 | DIFF (-8.4%) |
| Succubus Lash of Pain (Pet) | 2419.3 | 2642.5 | 13.4 | 14.7 | -1.2 (-8.4%) | 0 / 15 | DIFF (-8.4%) |

### 8. 12/31/8 Aff/DP (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 51488.9 | 48714.6 | 286.0 | 270.6 | +15.4 (+5.7%) | 43 / 45 | DIFF (+5.7%) |
| Corruption | 14578.0 | 14964.2 | 81.0 | 83.1 | -2.1 (-2.6%) | 9 / 9 | DIFF (-2.6%) |
| Bane of Agony | 4272.5 | 5207.5 | 23.7 | 28.9 | -5.2 (-18.0%) | 2 / 3 | DIFF (-18.0%) |
| Curse of Doom | 11152.7 | 13593.5 | 62.0 | 75.5 | -13.6 (-18.0%) | 2 / 2 | DIFF (-18.0%) |
| Soul Fire | 8038.1 | 5628.9 | 44.7 | 31.3 | +13.4 (+42.8%) | 7 / 6 | DIFF (+42.8%) |
| Succubus Melee (Pet) | 6615.1 | 7083.9 | 36.8 | 39.4 | -2.6 (-6.6%) | 0 / 90 | DIFF (-6.6%) |
| Succubus Lash of Pain (Pet) | 2169.4 | 2323.2 | 12.1 | 12.9 | -0.9 (-6.6%) | 0 / 15 | DIFF (-6.6%) |

### 9. 12/31/8 Aff/DP Brand (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 41044.5 | 35576.4 | 228.0 | 197.6 | +30.4 (+15.4%) | 35 / 34 | DIFF (+15.4%) |
| Corruption | 12369.7 | 14173.7 | 68.7 | 78.7 | -10.0 (-12.7%) | 8 / 9 | DIFF (-12.7%) |
| Bane of Agony | 4844.2 | 4999.0 | 26.9 | 27.8 | -0.9 (-3.1%) | 2 / 3 | DIFF (-3.1%) |
| Curse of Doom | 8886.5 | 9170.4 | 49.4 | 50.9 | -1.6 (-3.1%) | 2 / 2 | DIFF (-3.1%) |
| Searing Pain | 7156.5 | 7039.4 | 39.8 | 39.1 | +0.7 (+1.7%) | 17 / 20 | DIFF (+1.7%) |
| Soul Fire | 7786.5 | 7083.7 | 43.3 | 39.4 | +3.9 (+9.9%) | 6 / 7 | DIFF (+9.9%) |
| Succubus Melee (Pet) | 12025.0 | 6848.3 | 66.8 | 38.0 | +28.8 (+75.6%) | 0 / 90 | DIFF (+75.6%) |
| Succubus Lash of Pain (Pet) | 4442.1 | 2529.8 | 24.7 | 14.1 | +10.6 (+75.6%) | 0 / 15 | DIFF (+75.6%) |

### 10. 0/31/20 DP/AF Fire (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 9225.7 | 10957.7 | 51.3 | 60.9 | -9.6 (-15.8%) | 8 / 12 | DIFF (-15.8%) |
| Curse of Doom | 11491.4 | 8202.9 | 63.8 | 45.6 | +18.3 (+40.1%) | 5 / 3 | DIFF (+40.1%) |
| Immolate | 15725.0 | 16622.6 | 87.4 | 92.3 | -5.0 (-5.4%) | 12 / 12 | DIFF (-5.4%) |
| Searing Pain | 8704.5 | 41134.4 | 48.4 | 228.5 | -180.2 (-78.8%) | 16 / 68 | DIFF (-78.8%) |
| Imp Firebolt (Pet) | 19488.4 | 13791.6 | 108.3 | 76.6 | +31.6 (+41.3%) | 0 / 83 | DIFF (+41.3%) |

### 11. 2/31/18 DP Fire - Searing Pain (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 9410.2 | 11727.8 | 52.3 | 65.2 | -12.9 (-19.8%) | 10 / 12 | DIFF (-19.8%) |
| Curse of Doom | 13107.8 | 8202.9 | 72.8 | 45.6 | +27.2 (+59.8%) | 4 / 3 | DIFF (+59.8%) |
| Immolate | 19518.7 | 17924.0 | 108.4 | 99.6 | +8.9 (+8.9%) | 12 / 12 | DIFF (+8.9%) |
| Searing Pain | 36775.9 | 37962.5 | 204.3 | 210.9 | -6.6 (-3.1%) | 58 / 61 | DIFF (-3.1%) |
| Soul Fire | 9258.3 | 8501.9 | 51.4 | 47.2 | +4.2 (+8.9%) | 6 / 6 | DIFF (+8.9%) |
| Imp Firebolt (Pet) | 21222.3 | 14093.7 | 117.9 | 78.3 | +39.6 (+50.6%) | 0 / 84 | DIFF (+50.6%) |

### 12. 40/11/0 Deep Affliction DS-Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 14604.9 | 14325.0 | 81.1 | 79.6 | +1.6 (+2.0%) | 9 / 9 | DIFF (+2.0%) |
| Bane of Agony | 4674.5 | 4783.8 | 26.0 | 26.6 | -0.6 (-2.3%) | 3 / 3 | DIFF (-2.3%) |
| Curse of Doom | 9482.2 | 9703.9 | 52.7 | 53.9 | -1.2 (-2.3%) | 2 / 2 | DIFF (-2.3%) |
| Siphon Life | 6096.5 | 5979.6 | 33.9 | 33.2 | +0.6 (+2.0%) | 6 / 6 | DIFF (+2.0%) |
| Wrack / Drain Hope | 35471.6 | 34791.7 | 197.1 | 193.3 | +3.8 (+2.0%) | 24 / 24 | DIFF (+2.0%) |

### 13. 35/6/10 Deep Affliction Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 1426.1 | 3685.3 | 7.9 | 20.5 | -12.6 (-61.3%) | 2 / 5 | DIFF (-61.3%) |
| Corruption | 13772.0 | 12952.2 | 76.5 | 72.0 | +4.6 (+6.3%) | 9 / 9 | DIFF (+6.3%) |
| Bane of Agony | 5710.4 | 3762.3 | 31.7 | 20.9 | +10.8 (+51.8%) | 2 / 2 | DIFF (+51.8%) |
| Curse of Doom | 11032.6 | 7268.8 | 61.3 | 40.4 | +20.9 (+51.8%) | 2 / 2 | DIFF (+51.8%) |
| Siphon Life | 5177.7 | 4869.5 | 28.8 | 27.1 | +1.7 (+6.3%) | 6 / 6 | DIFF (+6.3%) |
| Wrack / Drain Hope | 27884.8 | 26224.7 | 154.9 | 145.7 | +9.2 (+6.3%) | 23 / 22 | DIFF (+6.3%) |
| Imp Firebolt (Pet) | 5690.8 | 5938.1 | 31.6 | 33.0 | -1.4 (-4.2%) | 0 / 55 | DIFF (-4.2%) |

### 14. 32/0/19 SM/AF (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 50382.6 | 45951.2 | 279.9 | 255.3 | +24.6 (+9.6%) | 43 / 44 | DIFF (+9.6%) |
| Corruption | 15166.2 | 15655.7 | 84.3 | 87.0 | -2.7 (-3.1%) | 10 / 10 | DIFF (-3.1%) |
| Bane of Agony | 4111.2 | 3551.6 | 22.8 | 19.7 | +3.1 (+15.8%) | 2 / 2 | DIFF (+15.8%) |
| Curse of Doom | 14294.0 | 12348.6 | 79.4 | 68.6 | +10.8 (+15.8%) | 2 / 2 | DIFF (+15.8%) |
| Shadowburn | 7978.3 | 7276.6 | 44.3 | 40.4 | +3.9 (+9.6%) | 11 / 11 | DIFF (+9.6%) |
| Imp Firebolt (Pet) | 6894.3 | 1878.6 | 38.3 | 10.4 | +27.9 (+267.0%) | 0 / 23 | DIFF (+267.0%) |

### 15. 1/17/33 Incinerate - Decimate + Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 9239.5 | 9508.9 | 51.3 | 52.8 | -1.5 (-2.8%) | 9 / 9 | DIFF (-2.8%) |
| Bane of Agony | 3183.7 | 3712.5 | 17.7 | 20.6 | -2.9 (-14.2%) | 2 / 2 | DIFF (-14.2%) |
| Curse of Doom | 7059.7 | 8232.4 | 39.2 | 45.7 | -6.5 (-14.2%) | 2 / 2 | DIFF (-14.2%) |
| Immolate | 12674.8 | 11284.5 | 70.4 | 62.7 | +7.7 (+12.3%) | 10 / 10 | DIFF (+12.3%) |
| Incinerate | 20120.1 | 19181.3 | 111.8 | 106.6 | +5.2 (+4.9%) | 20 / 20 | DIFF (+4.9%) |
| Searing Pain | 1606.3 | 2629.9 | 8.9 | 14.6 | -5.7 (-38.9%) | 4 / 5 | DIFF (-38.9%) |
| Conflagrate | 15314.7 | 13634.8 | 85.1 | 75.7 | +9.3 (+12.3%) | 16 / 16 | DIFF (+12.3%) |
| Soul Fire | 8030.4 | 7149.5 | 44.6 | 39.7 | +4.9 (+12.3%) | 7 / 7 | DIFF (+12.3%) |
| Shadowburn | 5920.5 | 5436.3 | 32.9 | 30.2 | +2.7 (+8.9%) | 8 / 8 | DIFF (+8.9%) |
| Imp Firebolt (Pet) | 9806.9 | 8993.6 | 54.5 | 50.0 | +4.5 (+9.0%) | 0 / 87 | DIFF (+9.0%) |

### 16. 10/10/31 Incinerate - Suppression + Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 9336.2 | 10221.7 | 51.9 | 56.8 | -4.9 (-8.7%) | 8 / 9 | DIFF (-8.7%) |
| Bane of Agony | 3957.6 | 2784.4 | 22.0 | 15.5 | +6.5 (+42.1%) | 2 / 2 | DIFF (+42.1%) |
| Curse of Doom | 11701.3 | 8232.4 | 65.0 | 45.7 | +19.3 (+42.1%) | 2 / 2 | DIFF (+42.1%) |
| Immolate | 14590.0 | 14515.8 | 81.1 | 80.6 | +0.4 (+0.5%) | 11 / 11 | PASS |
| Incinerate | 30335.3 | 32594.9 | 168.5 | 181.1 | -12.6 (-6.9%) | 31 / 33 | DIFF (-6.9%) |
| Conflagrate | 13850.8 | 13780.4 | 76.9 | 76.6 | +0.4 (+0.5%) | 16 / 16 | PASS |
| Shadowburn | 7145.0 | 8036.1 | 39.7 | 44.6 | -5.0 (-11.1%) | 10 / 11 | DIFF (-11.1%) |
| Imp Firebolt (Pet) | 10222.1 | 9073.3 | 56.8 | 50.4 | +6.4 (+12.7%) | 0 / 88 | DIFF (+12.7%) |

### 17. 7/13/31 Incinerate - Suppression + Succubus (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 9609.0 | 10508.2 | 53.4 | 58.4 | -5.0 (-8.6%) | 9 / 9 | DIFF (-8.6%) |
| Bane of Agony | 2373.5 | 2846.3 | 13.2 | 15.8 | -2.6 (-16.6%) | 2 / 2 | DIFF (-16.6%) |
| Curse of Doom | 8581.4 | 10290.5 | 47.7 | 57.2 | -9.5 (-16.6%) | 2 / 2 | DIFF (-16.6%) |
| Immolate | 12613.2 | 13457.9 | 70.1 | 74.8 | -4.7 (-6.3%) | 11 / 11 | DIFF (-6.3%) |
| Incinerate | 30650.4 | 26792.0 | 170.3 | 148.8 | +21.4 (+14.4%) | 27 / 29 | DIFF (+14.4%) |
| Conflagrate | 16103.9 | 17182.4 | 89.5 | 95.5 | -6.0 (-6.3%) | 16 / 17 | DIFF (-6.3%) |
| Shadowburn | 6515.1 | 6541.0 | 36.2 | 36.3 | -0.1 (-0.4%) | 11 / 10 | PASS |
| Succubus Melee (Pet) | 6499.2 | 6279.8 | 36.1 | 34.9 | +1.2 (+3.5%) | 0 / 90 | DIFF (+3.5%) |
| Succubus Lash of Pain (Pet) | 1507.4 | 1456.5 | 8.4 | 8.1 | +0.3 (+3.5%) | 0 / 15 | DIFF (+3.5%) |

### 18. 2/17/32 Shadow and Flame Shadow - Decimate (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 23249.7 | 18793.2 | 129.2 | 104.4 | +24.8 (+23.7%) | 23 / 17 | DIFF (+23.7%) |
| Corruption | 9101.3 | 9439.9 | 50.6 | 52.4 | -1.9 (-3.6%) | 8 / 8 | DIFF (-3.6%) |
| Bane of Agony | 2648.1 | 2165.6 | 14.7 | 12.0 | +2.7 (+22.3%) | 2 / 2 | DIFF (+22.3%) |
| Curse of Doom | 9563.2 | 7820.8 | 53.1 | 43.4 | +9.7 (+22.3%) | 2 / 2 | DIFF (+22.3%) |
| Immolate | 12950.6 | 13134.6 | 71.9 | 73.0 | -1.0 (-1.4%) | 10 / 11 | PASS |
| Searing Pain | 0.0 | 1815.6 | 0.0 | 10.1 | -10.1 (-100.0%) | 0 / 6 | MISSING (-100%) |
| Conflagrate | 14062.0 | 14261.7 | 78.1 | 79.2 | -1.1 (-1.4%) | 13 / 15 | PASS |
| Soul Fire | 6669.6 | 6764.3 | 37.1 | 37.6 | -0.5 (-1.4%) | 6 / 7 | PASS |
| Shadowburn | 6740.3 | 5448.3 | 37.4 | 30.3 | +7.2 (+23.7%) | 9 / 7 | DIFF (+23.7%) |
| Imp Firebolt (Pet) | 9755.1 | 9415.3 | 54.2 | 52.3 | +1.9 (+3.6%) | 0 / 86 | DIFF (+3.6%) |

### 19. 8/13/30 Shadow and Flame Shadow (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 34066.9 | 29079.1 | 189.3 | 161.6 | +27.7 (+17.2%) | 27 / 28 | DIFF (+17.2%) |
| Corruption | 10219.7 | 11086.8 | 56.8 | 61.6 | -4.8 (-7.8%) | 9 / 9 | DIFF (-7.8%) |
| Bane of Agony | 5163.4 | 3557.8 | 28.7 | 19.8 | +8.9 (+45.1%) | 2 / 3 | DIFF (+45.1%) |
| Curse of Doom | 8960.7 | 6174.3 | 49.8 | 34.3 | +15.5 (+45.1%) | 2 / 2 | DIFF (+45.1%) |
| Immolate | 14269.7 | 12155.0 | 79.3 | 67.5 | +11.7 (+17.4%) | 10 / 10 | DIFF (+17.4%) |
| Conflagrate | 15774.2 | 13436.5 | 87.6 | 74.6 | +13.0 (+17.4%) | 16 / 15 | DIFF (+17.4%) |
| Shadowburn | 7316.2 | 6245.0 | 40.6 | 34.7 | +6.0 (+17.2%) | 9 / 10 | DIFF (+17.2%) |
| Succubus Melee (Pet) | 6251.8 | 6668.9 | 34.7 | 37.0 | -2.3 (-6.3%) | 0 / 90 | DIFF (-6.3%) |
| Succubus Lash of Pain (Pet) | 1383.9 | 1476.3 | 7.7 | 8.2 | -0.5 (-6.3%) | 0 / 15 | DIFF (-6.3%) |

### 20. 19/11/21 NF/DS/Ruin DS-Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 46003.7 | 48133.9 | 255.6 | 267.4 | -11.8 (-4.4%) | 44 / 45 | DIFF (-4.4%) |
| Corruption | 14056.7 | 14210.5 | 78.1 | 78.9 | -0.9 (-1.1%) | 10 / 10 | PASS |
| Bane of Agony | 3481.0 | 3844.4 | 19.3 | 21.4 | -2.0 (-9.5%) | 2 / 2 | DIFF (-9.5%) |
| Curse of Doom | 10433.0 | 11522.1 | 58.0 | 64.0 | -6.1 (-9.5%) | 2 / 2 | DIFF (-9.5%) |
| Shadowburn | 8660.8 | 9061.8 | 48.1 | 50.3 | -2.2 (-4.4%) | 11 / 11 | DIFF (-4.4%) |

### 21. 23/10/18 NF/AF (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 51851.3 | 56114.1 | 288.1 | 311.7 | -23.7 (-7.6%) | 51 / 51 | DIFF (-7.6%) |
| Corruption | 12528.6 | 12368.7 | 69.6 | 68.7 | +0.9 (+1.3%) | 9 / 9 | PASS |
| Bane of Agony | 6144.2 | 4089.2 | 34.1 | 22.7 | +11.4 (+50.3%) | 3 / 3 | DIFF (+50.3%) |
| Curse of Doom | 8478.9 | 5642.9 | 47.1 | 31.3 | +15.8 (+50.3%) | 2 / 2 | DIFF (+50.3%) |
| Imp Firebolt (Pet) | 9651.3 | 9115.8 | 53.6 | 50.6 | +3.0 (+5.9%) | 0 / 86 | DIFF (+5.9%) |

### 22. 13/7/31 Aff Incinerate (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 9610.8 | 10468.3 | 53.4 | 58.2 | -4.8 (-8.2%) | 8 / 9 | DIFF (-8.2%) |
| Bane of Agony | 3553.4 | 3717.2 | 19.7 | 20.7 | -0.9 (-4.4%) | 2 / 2 | DIFF (-4.4%) |
| Curse of Doom | 7948.5 | 8314.7 | 44.2 | 46.2 | -2.0 (-4.4%) | 2 / 2 | DIFF (-4.4%) |
| Immolate | 13055.4 | 14485.8 | 72.5 | 80.5 | -7.9 (-9.9%) | 11 / 11 | DIFF (-9.9%) |
| Incinerate | 31352.5 | 33442.1 | 174.2 | 185.8 | -11.6 (-6.2%) | 31 / 34 | DIFF (-6.2%) |
| Conflagrate | 16165.7 | 17936.9 | 89.8 | 99.6 | -9.8 (-9.9%) | 16 / 17 | DIFF (-9.9%) |
| Shadowburn | 7072.0 | 6336.9 | 39.3 | 35.2 | +4.1 (+11.6%) | 10 / 10 | DIFF (+11.6%) |
| Succubus Melee (Pet) | 6253.6 | 6746.7 | 34.7 | 37.5 | -2.7 (-7.3%) | 0 / 90 | DIFF (-7.3%) |
| Succubus Lash of Pain (Pet) | 1073.5 | 1158.1 | 6.0 | 6.4 | -0.5 (-7.3%) | 0 / 15 | DIFF (-7.3%) |

