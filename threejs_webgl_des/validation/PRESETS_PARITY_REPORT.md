# WebGL2 vs Authoritative C++ CPU Oracle: Spec Presets Parity Report

**Generated:** 2026-10-02 17:25:15 UTC  
**Total Presets Tested:** 22  
**Passed Presets (<= 1.5% tolerance):** 0 / 22

## Summary Table

| # | Spec Preset Name | Status | WebGL DPS | Native CPU DPS | DPS Delta | Pet DPS | Diagnostics |
|---|------------------|--------|-----------|----------------|-----------|---------|-------------|
| 1 | 5/11/35 DS/AF DS-Imp | ❌ FAIL | 524.3 | 689.7 | -165.4 (-24.0%) | 0.0 | total: GPU 94375.9140625, CPU 124142.63236899086<br>mana: GPU 1717, CPU 2250.95<br>spent: GPU 23695, CPU 22772<br>gained: GPU 18979, CPU 17625<br>taps: GPU 27, CPU 25<br>isbProcs: GPU 5, CPU 6<br>isbConsumed: GPU 33, CPU 36<br>damage0: GPU 35896.64453125, CPU 39975.91106698479<br>casts0: GPU 34, CPU 26<br>hits0: GPU 34, CPU 26<br>crits0: GPU 5, CPU 6<br>damage1: GPU 13391.9375, CPU 14265.045740000014<br>casts1: GPU 9, CPU 8<br>hits1: GPU 50, CPU 47<br>crits1: GPU 16, CPU 14<br>damage2: GPU 19381.369140625, CPU 3211.49503125<br>casts2: GPU 5, CPU 2<br>hits2: GPU 28, CPU 19<br>crits2: GPU 12, CPU 1<br>damage3: GPU 25705.892578125, CPU 16442.819250000008<br>casts3: GPU 24, CPU 11<br>hits3: GPU 68, CPU 62<br>crits3: GPU 23, CPU 17<br>Next RNG output differs: 1970448001,4059752685 / 202945356,3414949987<br>First differing damage event 0: {"time":5,"kind":1,"spell":3,"damage":1004.4548950195312,"mana":5228,"flags":0,"total":1004.4548950195312,"rngCalls":8} / [1.5,8,418.27500000000003,0] |
| 2 | 9/11/31 Incinerate - Suppression + DS | ❌ FAIL | 497.9 | 658.3 | -160.4 (-24.4%) | 0.0 | total: GPU 89621.46875, CPU 118497.22519777744<br>mana: GPU 1993, CPU 1902.1999999999978<br>spent: GPU 25470, CPU 25828.74999999998<br>gained: GPU 21030, CPU 20333<br>taps: GPU 30, CPU 29<br>damage0: GPU 7735.470703125, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 3, CPU 0<br>damage1: GPU 11797.138671875, CPU 13705.653059999999<br>hits1: GPU 52, CPU 50<br>crits1: GPU 11, CPU 17<br>damage2: GPU 0, CPU 3855.7406250000026<br>casts2: GPU 0, CPU 2<br>hits2: GPU 0, CPU 23<br>crits2: GPU 0, CPU 6<br>damage3: GPU 34165.08984375, CPU 17186.2488875<br>casts3: GPU 27, CPU 11<br>hits3: GPU 74, CPU 61<br>crits3: GPU 29, CPU 7<br>damage4: GPU 35923.84765625, CPU 39356.16422858817<br>casts4: GPU 31, CPU 30<br>crits4: GPU 8, CPU 6<br>Next RNG output differs: 1970448001,4059752685 / 3481551094,1553824663<br>First differing damage event 0: {"time":3.5,"kind":1,"spell":3,"damage":1176.13525390625,"mana":5508,"flags":0,"total":1176.13525390625,"rngCalls":7} / [1.5,8,592.02,0] |
| 3 | 7/11/33 Incinerate - Suppression + DS (No Corruption) | ❌ FAIL | 524.4 | 594.2 | -69.8 (-11.7%) | 0.0 | total: GPU 94398.921875, CPU 106960.06908230291<br>mana: GPU 2197.79736328125, CPU 1994.650000000002<br>spent: GPU 26870, CPU 24808.5<br>gained: GPU 22634.806640625, CPU 19405.2<br>taps: GPU 27, CPU 23<br>damage0: GPU 6492.5625, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 1, CPU 0<br>damage3: GPU 34985.66796875, CPU 20723.96370000002<br>casts3: GPU 27, CPU 10<br>hits3: GPU 77, CPU 60<br>crits3: GPU 28, CPU 20<br>damage4: GPU 52920.73046875, CPU 58082.268341329414<br>casts4: GPU 42, CPU 45<br>hits4: GPU 42, CPU 45<br>crits4: GPU 14, CPU 8<br>Next RNG output differs: 311579884,393305375 / 95403924,2872338106<br>First differing damage event 0: {"time":1.5,"kind":1,"spell":3,"damage":1169.78515625,"mana":5798,"flags":0,"total":1169.78515625,"rngCalls":6} / [1.5,8,592.02,0] |
| 4 | 3/17/31 Incinerate - DS + Decimate | ❌ FAIL | 491.4 | 572.4 | -81.0 (-14.2%) | 0.0 | total: GPU 88449.9765625, CPU 103038.79784008041<br>mana: GPU 1743, CPU 2436.9500000000007<br>spent: GPU 25700, CPU 23283.000000000004<br>gained: GPU 21010, CPU 18322<br>taps: GPU 30, CPU 26<br>damage0: GPU 6510.1611328125, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 1, CPU 0<br>damage3: GPU 33788.3046875, CPU 17492.0942625<br>casts3: GPU 27, CPU 10<br>hits3: GPU 77, CPU 58<br>crits3: GPU 24, CPU 17<br>damage4: GPU 48151.515625, CPU 41039.26447347735<br>casts4: GPU 39, CPU 32<br>hits4: GPU 39, CPU 32<br>crits4: GPU 11, CPU 5<br>damage5: GPU 0, CPU 3695.2564600478154<br>casts5: GPU 0, CPU 5<br>hits5: GPU 0, CPU 5<br>crits5: GPU 0, CPU 2<br>Next RNG output differs: 785349194,2754653148 / 3183009327,3299378543<br>First differing damage event 0: {"time":1.5,"kind":1,"spell":3,"damage":1169.78515625,"mana":5798,"flags":0,"total":1169.78515625,"rngCalls":6} / [1.5,8,481.01624999999996,0] |
| 5 | 5/11/35 DS/Searing Pain DS-Succ | ❌ FAIL | 411.2 | 586.6 | -175.4 (-29.9%) | 0.0 | total: GPU 74024.9765625, CPU 105595.76534864146<br>mana: GPU 2247, CPU 1530.4500000000053<br>spent: GPU 21831, CPU 21461.500000000015<br>gained: GPU 17645, CPU 15594<br>taps: GPU 25, CPU 22<br>damage0: GPU 6494.830078125, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 1, CPU 0<br>damage1: GPU 10828.291015625, CPU 10936.827000000003<br>casts1: GPU 9, CPU 8<br>hits1: GPU 50, CPU 46<br>crits1: GPU 14, CPU 10<br>damage2: GPU 0, CPU 4155.215625000003<br>casts2: GPU 0, CPU 3<br>hits2: GPU 0, CPU 24<br>crits2: GPU 0, CPU 7<br>damage3: GPU 28985.24609375, CPU 18708.341162500008<br>casts3: GPU 25, CPU 11<br>hits3: GPU 75, CPU 62<br>crits3: GPU 19, CPU 12<br>damage5: GPU 27716.67578125, CPU 32400.39283173438<br>casts5: GPU 47, CPU 45<br>hits5: GPU 47, CPU 45<br>crits5: GPU 13, CPU 17<br>Next RNG output differs: 2203682328,4224033072 / 2211600309,251840177<br>First differing damage event 0: {"time":3.5,"kind":1,"spell":3,"damage":1176.13525390625,"mana":5508,"flags":0,"total":1176.13525390625,"rngCalls":7} / [1.5,8,592.02,0] |
| 6 | 2/31/18 DP/AF Shadow | ❌ FAIL | 650.7 | 705.0 | -54.3 (-7.7%) | 57.0 | total: GPU 117124.4453125, CPU 126896.96150147656<br>mana: GPU 1703, CPU 1998.9499999999998<br>spent: GPU 22355, CPU 21670<br>gained: GPU 17625, CPU 16271<br>taps: GPU 25, CPU 23<br>isbProcs: GPU 13, CPU 10<br>isbConsumed: GPU 60, CPU 65<br>petDamage: GPU 10261.2802734375, CPU 9242.371951324345<br>damage0: GPU 62660.78515625, CPU 73213.8924574995<br>casts0: GPU 40, CPU 41<br>hits0: GPU 40, CPU 39<br>crits0: GPU 13, CPU 10<br>misses0: GPU 0, CPU 2<br>damage1: GPU 14363.5546875, CPU 16267.201757949997<br>hits1: GPU 48, CPU 49<br>crits1: GPU 11, CPU 8<br>misses1: GPU 1, CPU 0<br>damage2: GPU 17243.98828125, CPU 5774.330838144001<br>casts2: GPU 5, CPU 3<br>hits2: GPU 27, CPU 24<br>crits2: GPU 1, CPU 6<br>damage3: GPU 12594.76953125, CPU 0<br>casts3: GPU 10, CPU 0<br>hits3: GPU 59, CPU 0<br>crits3: GPU 15, CPU 0<br>Next RNG output differs: 2434845742,2561167086 / 1975815257,424353251<br>First differing damage event 0: {"time":1,"kind":6,"spell":201,"damage":135.58497619628906,"mana":6133,"flags":0,"total":135.58497619628906,"rngCalls":3} / [5,2,321.62017800000007,0] |
| 7 | 2/31/18 DP/AF Shadow Corruption | ❌ FAIL | 624.6 | 646.9 | -22.3 (-3.4%) | 57.2 | total: GPU 112427.6484375, CPU 116434.31504267736<br>mana: GPU 2165, CPU 2290.95<br>spent: GPU 22570, CPU 22075<br>gained: GPU 18302, CPU 16968<br>taps: GPU 26, CPU 24<br>isbProcs: GPU 9, CPU 7<br>isbConsumed: GPU 42, CPU 41<br>petDamage: GPU 10299.7158203125, CPU 9812.870954052112<br>damage0: GPU 54167.91015625, CPU 65333.50916515727<br>casts0: GPU 39, CPU 43<br>crits0: GPU 9, CPU 7<br>misses0: GPU 1, CPU 4<br>damage1: GPU 15190.0048828125, CPU 17193.020124852002<br>hits1: GPU 48, CPU 50<br>crits1: GPU 15, CPU 11<br>misses1: GPU 1, CPU 0<br>damage2: GPU 20927.30859375, CPU 4264.267345932001<br>casts2: GPU 6, CPU 3<br>hits2: GPU 27, CPU 20<br>crits2: GPU 8, CPU 6<br>damage3: GPU 11842.685546875, CPU 0<br>casts3: GPU 11, CPU 0<br>hits3: GPU 57, CPU 0<br>crits3: GPU 14, CPU 0<br>misses3: GPU 1, CPU 0<br>Next RNG output differs: 2898738106,1039601296 / 3601773102,3920663226<br>First differing damage event 0: {"time":1,"kind":6,"spell":201,"damage":135.58497619628906,"mana":6133,"flags":0,"total":135.58497619628906,"rngCalls":3} / [4.2,2,334.48498512000003,0] |
| 8 | 12/31/8 Aff/DP | ❌ FAIL | 585.8 | 694.8 | -109.0 (-15.7%) | 57.4 | total: GPU 105451.9609375, CPU 125070.6165175748<br>mana: GPU 2228.39794921875, CPU 2321.700000000002<br>spent: GPU 21985, CPU 22515<br>gained: GPU 17780.404296875, CPU 17760.399999999998<br>isbConsumed: GPU 51, CPU 74<br>petDamage: GPU 10335.96484375, CPU 9589.558465060129<br>damage0: GPU 57639.79296875, CPU 66119.11934566183<br>casts0: GPU 49, CPU 45<br>hits0: GPU 49, CPU 45<br>damage1: GPU 14316.4306640625, CPU 18005.489450982004<br>hits1: GPU 49, CPU 52<br>crits1: GPU 11, CPU 9<br>damage2: GPU 23159.806640625, CPU 6281.880271673602<br>casts2: GPU 5, CPU 3<br>hits2: GPU 29, CPU 25<br>crits2: GPU 8, CPU 6<br>Next RNG output differs: 2905008737,801382198 / 2910000209,3323117351<br>First differing damage event 0: {"time":1,"kind":6,"spell":201,"damage":123.25907897949219,"mana":6133,"flags":0,"total":123.25907897949219,"rngCalls":3} / [3.4,2,347.3497922400001,0] |
| 9 | 12/31/8 Aff/DP Brand | ❌ FAIL | 532.0 | 672.1 | -140.1 (-20.8%) | 57.4 | total: GPU 95755.3125, CPU 120977.06212927007<br>mana: GPU 2228.39794921875, CPU 2367.550000000003<br>spent: GPU 21985, CPU 20373.60000000001<br>gained: GPU 17780.404296875, CPU 15343.199999999995<br>taps: GPU 21, CPU 18<br>petDamage: GPU 10335.96484375, CPU 19310.487960601098<br>damage0: GPU 52618.046875, CPU 50527.65028323287<br>casts0: GPU 49, CPU 36<br>hits0: GPU 49, CPU 36<br>damage1: GPU 12840.876953125, CPU 17292.712920615006<br>hits1: GPU 49, CPU 53<br>crits1: GPU 11, CPU 12<br>damage2: GPU 19960.458984375, CPU 5980.301445157603<br>casts2: GPU 5, CPU 3<br>hits2: GPU 29, CPU 27<br>crits2: GPU 8, CPU 6<br>damage5: GPU 0, CPU 8394.07595266154<br>casts5: GPU 0, CPU 18<br>hits5: GPU 0, CPU 18<br>crits5: GPU 0, CPU 3<br>Next RNG output differs: 2905008737,801382198 / 83145216,2269574781<br>First differing damage event 0: {"time":1,"kind":6,"spell":201,"damage":123.25907897949219,"mana":6133,"flags":0,"total":123.25907897949219,"rngCalls":3} / [3.8,2,344.1335904600001,0] |
| 10 | 0/31/20 DP/AF Fire | ❌ FAIL | 645.1 | 722.7 | -77.5 (-10.7%) | 135.4 | total: GPU 116121.9765625, CPU 130080.8934898054<br>mana: GPU 2030, CPU 3409.750000000012<br>spent: GPU 26090, CPU 18925.200000000023<br>gained: GPU 21687, CPU 14937<br>taps: GPU 31, CPU 21<br>petDamage: GPU 24380.6484375, CPU 29531.54439618995<br>damage0: GPU 7555.38720703125, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 3, CPU 0<br>damage1: GPU 0, CPU 13346.003550000012<br>casts1: GPU 0, CPU 11<br>hits1: GPU 0, CPU 56<br>crits1: GPU 0, CPU 11<br>misses1: GPU 0, CPU 1<br>damage2: GPU 15169.376953125, CPU 3828.932712<br>casts2: GPU 5, CPU 4<br>hits2: GPU 30, CPU 27<br>crits2: GPU 6, CPU 5<br>damage3: GPU 32007.28125, CPU 22486.43878909998<br>casts3: GPU 34, CPU 12<br>hits3: GPU 65, CPU 64<br>crits3: GPU 16, CPU 23<br>misses3: GPU 1, CPU 0<br>damage4: GPU 37009.37890625, CPU 0<br>casts4: GPU 29, CPU 0<br>hits4: GPU 28, CPU 0<br>crits4: GPU 6, CPU 0<br>misses4: GPU 1, CPU 0<br>damage5: GPU 0, CPU 50962.44084251536<br>casts5: GPU 0, CPU 66<br>hits5: GPU 0, CPU 61<br>crits5: GPU 0, CPU 27<br>misses5: GPU 0, CPU 4<br>Next RNG output differs: 472152422,1792697842 / 1144405203,2071831624<br>First differing damage event 0: {"time":1.5,"kind":6,"spell":100,"damage":266.1134338378906,"mana":6133,"flags":0,"total":266.1134338378906,"rngCalls":4} / [5,8,567.35003325,0] |
| 11 | 2/31/18 DP Fire - Searing Pain | ❌ FAIL | 528.1 | 759.6 | -231.5 (-30.5%) | 139.2 | total: GPU 95052.953125, CPU 136727.46898972476<br>mana: GPU 1651, CPU 2871.3500000000113<br>spent: GPU 19022, CPU 19463.600000000017<br>gained: GPU 14240, CPU 14937<br>taps: GPU 20, CPU 21<br>petDamage: GPU 25051.341796875, CPU 29336.97957409784<br>damage1: GPU 0, CPU 13953.612188999989<br>casts1: GPU 0, CPU 10<br>hits1: GPU 0, CPU 58<br>crits1: GPU 0, CPU 11<br>damage3: GPU 16139.77734375, CPU 21473.31372972498<br>casts3: GPU 11, CPU 13<br>hits3: GPU 61, CPU 66<br>crits3: GPU 13, CPU 16<br>misses3: GPU 0, CPU 1<br>damage5: GPU 53861.875, CPU 48639.34869859819<br>casts5: GPU 89, CPU 63<br>hits5: GPU 84, CPU 62<br>crits5: GPU 21, CPU 20<br>misses5: GPU 4, CPU 1<br>Next RNG output differs: 3900347733,1962987867 / 3431659539,1421055428<br>First differing damage event 0: {"time":1.5,"kind":6,"spell":100,"damage":258.0919189453125,"mana":6433,"flags":0,"total":258.0919189453125,"rngCalls":3} / [4.6,8,567.35003325,0] |
| 12 | 40/11/0 Deep Affliction DS-Imp | ❌ FAIL | 445.2 | 501.2 | -56.0 (-11.2%) | 0.0 | total: GPU 80140.3046875, CPU 90216.42324794804<br>mana: GPU 2353.19775390625, CPU 1979.7500000000005<br>spent: GPU 23465, CPU 11805<br>gained: GPU 19385.205078125, CPU 6386.799999999999<br>taps: GPU 23, CPU 7<br>damage0: GPU 43243.375, CPU 0<br>casts0: GPU 44, CPU 0<br>hits0: GPU 44, CPU 0<br>crits0: GPU 16, CPU 0<br>damage1: GPU 13734.884765625, CPU 20164.504774380275<br>hits1: GPU 53, CPU 52<br>crits1: GPU 20, CPU 18<br>damage2: GPU 15099.787109375, CPU 6906.216257883694<br>casts2: GPU 5, CPU 3<br>hits2: GPU 29, CPU 25<br>crits2: GPU 10, CPU 8<br>damage3: GPU 8062.25, CPU 0<br>casts3: GPU 9, CPU 0<br>hits3: GPU 53, CPU 0<br>crits3: GPU 9, CPU 0<br>Next RNG output differs: 3183009327,3299378543 / 714754880,271462550<br>First differing damage event 0: {"time":4.5,"kind":3,"spell":1,"damage":218.01414489746094,"mana":5843,"flags":0,"total":218.01414489746094,"rngCalls":3} / [3,2,320.6016000000001,0] |
| 13 | 35/6/10 Deep Affliction Imp | ❌ FAIL | 524.8 | 536.1 | -11.3 (-2.1%) | 96.0 | total: GPU 94466.1484375, CPU 96496.80697202889<br>mana: GPU 1713.19775390625, CPU 1792.000000000001<br>spent: GPU 24125, CPU 13465<br>gained: GPU 19405.205078125, CPU 8823.999999999998<br>taps: GPU 23, CPU 10<br>procs: GPU 2, CPU 10<br>isbProcs: GPU 13, CPU 4<br>isbConsumed: GPU 76, CPU 46<br>petDamage: GPU 17271.3125, CPU 6991.6031428571405<br>damage0: GPU 40745.890625, CPU 9136.948271874693<br>casts0: GPU 45, CPU 8<br>hits0: GPU 45, CPU 8<br>crits0: GPU 13, CPU 4<br>damage1: GPU 12022.505859375, CPU 15411.725217341953<br>casts1: GPU 10, CPU 9<br>hits1: GPU 54, CPU 51<br>crits1: GPU 11, CPU 10<br>damage2: GPU 15685.42578125, CPU 5718.679303009368<br>casts2: GPU 5, CPU 2<br>hits2: GPU 27, CPU 24<br>damage3: GPU 8741, CPU 0<br>casts3: GPU 9, CPU 0<br>hits3: GPU 54, CPU 0<br>crits3: GPU 15, CPU 0<br>Next RNG output differs: 3656188059,1352042593 / 3900347733,1962987867<br>First differing damage event 0: {"time":1.5,"kind":6,"spell":100,"damage":170.32958984375,"mana":6133,"flags":0,"total":170.32958984375,"rngCalls":4} / [3,2,267.16800000000006,0] |
| 14 | 32/0/19 SM/AF | ❌ FAIL | 596.6 | 629.5 | -32.8 (-5.2%) | 79.2 | total: GPU 107395.4140625, CPU 113305.41445832008<br>mana: GPU 2277.99755859375, CPU 1768.200000000002<br>spent: GPU 25185, CPU 24070<br>gained: GPU 21030.005859375, CPU 19405.2<br>taps: GPU 25, CPU 23<br>procs: GPU 3, CPU 1<br>isbProcs: GPU 10, CPU 16<br>isbConsumed: GPU 62, CPU 85<br>petDamage: GPU 14251.4482421875, CPU 2145.628571428572<br>damage0: GPU 49311.78125, CPU 64965.05995047398<br>casts0: GPU 48, CPU 43<br>hits0: GPU 48, CPU 43<br>crits0: GPU 12, CPU 16<br>damage1: GPU 15014.5341796875, CPU 19191.0839776334<br>hits1: GPU 54, CPU 55<br>crits1: GPU 23, CPU 17<br>damage2: GPU 18751.97265625, CPU 5525.313742138782<br>casts2: GPU 5, CPU 3<br>hits2: GPU 29, CPU 24<br>crits2: GPU 13, CPU 9<br>damage3: GPU 10065.5546875, CPU 0<br>casts3: GPU 9, CPU 0<br>hits3: GPU 54, CPU 0<br>crits3: GPU 11, CPU 0<br>Next RNG output differs: 2735815256,3691402726 / 551600562,4294594783<br>First differing damage event 0: {"time":1.5,"kind":6,"spell":100,"damage":138.58177185058594,"mana":6133,"flags":0,"total":138.58177185058594,"rngCalls":4} / [3,2,278.7840000000001,0] |
| 15 | 1/17/33 Incinerate - Decimate + Imp | ❌ FAIL | 530.3 | 601.4 | -71.1 (-11.8%) | 121.7 | total: GPU 95458.171875, CPU 108250.81976459338<br>mana: GPU 1993, CPU 2263.95<br>spent: GPU 25470, CPU 24790<br>gained: GPU 21030, CPU 19656<br>taps: GPU 30, CPU 28<br>petDamage: GPU 21897.2421875, CPU 11150.700000000006<br>damage0: GPU 5888.00439453125, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>damage1: GPU 10353.3671875, CPU 11516.356500000002<br>hits1: GPU 50, CPU 48<br>crits1: GPU 9, CPU 12<br>damage2: GPU 0, CPU 2218.8375000000005<br>casts2: GPU 0, CPU 2<br>hits2: GPU 0, CPU 16<br>crits2: GPU 0, CPU 4<br>damage3: GPU 28165.568359375, CPU 13665.951749999997<br>casts3: GPU 27, CPU 11<br>hits3: GPU 74, CPU 59<br>crits3: GPU 22, CPU 12<br>misses3: GPU 0, CPU 1<br>damage4: GPU 29154.021484375, CPU 23852.66601678528<br>casts4: GPU 31, CPU 20<br>hits4: GPU 28, CPU 19<br>crits4: GPU 8, CPU 6<br>misses4: GPU 2, CPU 1<br>damage5: GPU 0, CPU 2150.5123860104322<br>casts5: GPU 0, CPU 5<br>hits5: GPU 0, CPU 5<br>Next RNG output differs: 3198164516,1548328651 / 1551463470,451020513<br>First differing damage event 0: {"time":1.5,"kind":6,"spell":100,"damage":211.21702575683594,"mana":6433,"flags":0,"total":211.21702575683594,"rngCalls":3} / [1.5,8,353.92500000000007,0] |
| 16 | 10/10/31 Incinerate - Suppression + Imp | ❌ FAIL | 637.3 | 643.5 | -6.2 (-1.0%) | 118.6 | total: GPU 114722.1953125, CPU 115837.47326480241<br>mana: GPU 1715.3974609375, CPU 1316.600000000001<br>spent: GPU 26540, CPU 26958.79999999998<br>gained: GPU 21822.40625, CPU 21842.400000000005<br>petDamage: GPU 21355.4296875, CPU 11143.000000000007<br>damage0: GPU 7776.0244140625, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 3, CPU 0<br>damage1: GPU 11477.9873046875, CPU 12220.869320000016<br>hits1: GPU 50, CPU 48<br>crits1: GPU 14, CPU 11<br>damage2: GPU 15028.95703125, CPU 3256.7906250000015<br>casts2: GPU 5, CPU 2<br>hits2: GPU 26, CPU 20<br>crits2: GPU 8, CPU 9<br>damage3: GPU 26442.162109375, CPU 17216.641750000017<br>casts3: GPU 25, CPU 11<br>hits3: GPU 71, CPU 61<br>crits3: GPU 20, CPU 19<br>damage4: GPU 32641.65234375, CPU 35900.57892043965<br>casts4: GPU 32, CPU 33<br>hits4: GPU 32, CPU 33<br>crits4: GPU 8, CPU 5<br>Next RNG output differs: 4144896821,3038820918 / 472152422,1792697842<br>First differing damage event 0: {"time":1.5,"kind":6,"spell":100,"damage":217.98910522460938,"mana":6133,"flags":0,"total":217.98910522460938,"rngCalls":4} / [1.5,8,514.8000000000001,0] |
| 17 | 7/13/31 Incinerate - Suppression + Succubus | ❌ FAIL | 551.3 | 649.1 | -97.8 (-15.1%) | 56.6 | total: GPU 99229.5390625, CPU 116841.36862296252<br>mana: GPU 2088, CPU 2266.55<br>spent: GPU 25375, CPU 25484.399999999983<br>gained: GPU 21030, CPU 20353<br>taps: GPU 30, CPU 29<br>petDamage: GPU 10179.26953125, CPU 8372.991999783346<br>damage0: GPU 6519.01123046875, CPU 0<br>casts0: GPU 9, CPU 0<br>hits0: GPU 9, CPU 0<br>crits0: GPU 2, CPU 0<br>damage1: GPU 11162.6328125, CPU 12630.49788<br>crits1: GPU 13, CPU 12<br>damage2: GPU 15199.11328125, CPU 3107.0531250000017<br>casts2: GPU 5, CPU 2<br>hits2: GPU 28, CPU 21<br>crits2: GPU 9, CPU 3<br>damage3: GPU 26354.47265625, CPU 16891.030750000013<br>casts3: GPU 26, CPU 11<br>hits3: GPU 71, CPU 62<br>crits3: GPU 18, CPU 16<br>damage4: GPU 29814.943359375, CPU 33245.90388620973<br>crits4: GPU 7, CPU 6<br>Next RNG output differs: 3343887230,1977148270 / 1532269639,2661409785<br>First differing damage event 0: {"time":1,"kind":6,"spell":201,"damage":109.33216094970703,"mana":6133,"flags":0,"total":109.33216094970703,"rngCalls":3} / [1.5,8,514.8000000000001,0] |
| 18 | 2/17/32 Shadow and Flame Shadow - Decimate | ❌ FAIL | 572.6 | 600.9 | -28.2 (-4.7%) | 113.1 | total: GPU 103074.96875, CPU 108155.09714881792<br>mana: GPU 2104, CPU 2536.95<br>spent: GPU 23985, CPU 23860<br>gained: GPU 19656, CPU 18999<br>taps: GPU 28, CPU 27<br>isbProcs: GPU 7, CPU 6<br>isbConsumed: GPU 43, CPU 18<br>petDamage: GPU 20360.349609375, CPU 10751.840000000007<br>damage0: GPU 32759.810546875, CPU 25107.01950334987<br>casts0: GPU 33, CPU 17<br>hits0: GPU 31, CPU 17<br>crits0: GPU 9, CPU 6<br>misses0: GPU 2, CPU 0<br>damage1: GPU 10809.294921875, CPU 11276.086800000006<br>casts1: GPU 10, CPU 9<br>hits1: GPU 48, CPU 44<br>crits1: GPU 9, CPU 11<br>damage2: GPU 14133.435546875, CPU 2133.759375000001<br>casts2: GPU 5, CPU 1<br>hits2: GPU 28, CPU 12<br>crits2: GPU 7, CPU 4<br>damage3: GPU 25012.09765625, CPU 13386.41150000001<br>casts3: GPU 25, CPU 10<br>hits3: GPU 70, CPU 59<br>crits3: GPU 19, CPU 10<br>damage5: GPU 0, CPU 3155.939488425406<br>casts5: GPU 0, CPU 5<br>hits5: GPU 0, CPU 5<br>crits5: GPU 0, CPU 2<br>Next RNG output differs: 4285131950,1173200125 / 1950743543,1956617504<br>First differing damage event 0: {"time":1.5,"kind":6,"spell":100,"damage":217.98910522460938,"mana":6133,"flags":0,"total":217.98910522460938,"rngCalls":4} / [1.5,8,353.92500000000007,0] |
| 19 | 8/13/30 Shadow and Flame Shadow | ❌ FAIL | 533.5 | 641.6 | -108.0 (-16.8%) | 57.8 | total: GPU 96036.3359375, CPU 115483.38729564154<br>mana: GPU 1645.59765625, CPU 2496.550000000002<br>spent: GPU 24985, CPU 24238.89999999998<br>gained: GPU 20197.60546875, CPU 19337.500000000007<br>taps: GPU 24, CPU 25<br>isbConsumed: GPU 24, CPU 26<br>petDamage: GPU 10405.9775390625, CPU 8426.865578072859<br>damage0: GPU 36288.61328125, CPU 36802.6665473745<br>casts0: GPU 38, CPU 28<br>hits0: GPU 38, CPU 28<br>crits0: GPU 7, CPU 6<br>damage1: GPU 10787.2548828125, CPU 13475.576400000005<br>casts1: GPU 8, CPU 9<br>hits1: GPU 48, CPU 49<br>crits1: GPU 7, CPU 16<br>damage2: GPU 12642.5341796875, CPU 4140.241875000002<br>casts2: GPU 5, CPU 3<br>hits2: GPU 27, CPU 25<br>crits2: GPU 4, CPU 5<br>damage3: GPU 25911.87890625, CPU 13677.779500000011<br>casts3: GPU 24, CPU 10<br>hits3: GPU 71, CPU 59<br>crits3: GPU 20, CPU 11<br>Next RNG output differs: 216393481,2125520398 / 150359604,1919830352<br>First differing damage event 0: {"time":1,"kind":6,"spell":201,"damage":109.33216094970703,"mana":6133,"flags":0,"total":109.33216094970703,"rngCalls":3} / [1.5,8,353.92500000000007,0] |
| 20 | 19/11/21 NF/DS/Ruin DS-Imp | ❌ FAIL | 502.3 | 699.3 | -197.0 (-28.2%) | 0.0 | total: GPU 90421.0234375, CPU 125879.60106464328<br>mana: GPU 1830.59765625, CPU 1936.0500000000002<br>spent: GPU 24820, CPU 23222.299999999977<br>gained: GPU 20217.60546875, CPU 17760.399999999998<br>taps: GPU 24, CPU 21<br>isbProcs: GPU 9, CPU 16<br>isbConsumed: GPU 67, CPU 93<br>damage0: GPU 51153.25, CPU 73255.42474333255<br>casts0: GPU 47, CPU 44<br>hits0: GPU 47, CPU 44<br>crits0: GPU 10, CPU 16<br>damage1: GPU 14571.65234375, CPU 18534.606062499992<br>hits1: GPU 54, CPU 56<br>crits1: GPU 8, CPU 14<br>damage2: GPU 14270.306640625, CPU 5605.04896875<br>casts2: GPU 5, CPU 3<br>hits2: GPU 30, CPU 26<br>damage3: GPU 10425.8037109375, CPU 0<br>casts3: GPU 9, CPU 0<br>hits3: GPU 53, CPU 0<br>crits3: GPU 16, CPU 0<br>Next RNG output differs: 2273724992,4031887805 / 1716409947,1157202318<br>First differing damage event 0: {"time":4.5,"kind":3,"spell":1,"damage":228.39576721191406,"mana":5843,"flags":0,"total":228.39576721191406,"rngCalls":4} / [3,2,307.2432,0] |
| 21 | 23/10/18 NF/AF | ❌ FAIL | 603.4 | 629.5 | -26.1 (-4.1%) | 111.6 | total: GPU 108612.6328125, CPU 113306.07445274116<br>mana: GPU 1713.19775390625, CPU 1620.800000000002<br>spent: GPU 24125, CPU 23385<br>gained: GPU 19405.205078125, CPU 18572.8<br>taps: GPU 23, CPU 22<br>procs: GPU 2, CPU 6<br>isbConsumed: GPU 76, CPU 81<br>petDamage: GPU 20094.505859375, CPU 10879.000000000011<br>damage0: GPU 48143.3359375, CPU 68165.61711274125<br>casts0: GPU 45, CPU 52<br>hits0: GPU 45, CPU 52<br>damage1: GPU 13053.0146484375, CPU 15880.480440000014<br>crits1: GPU 11, CPU 18<br>damage2: GPU 16432.3515625, CPU 4254.1785<br>casts2: GPU 5, CPU 3<br>hits2: GPU 27, CPU 25<br>crits2: GPU 9, CPU 5<br>damage3: GPU 10889.4541015625, CPU 0<br>casts3: GPU 9, CPU 0<br>hits3: GPU 54, CPU 0<br>crits3: GPU 15, CPU 0<br>Next RNG output differs: 3656188059,1352042593 / 3408691609,3204568218<br>First differing damage event 0: {"time":1.5,"kind":6,"spell":100,"damage":198.1719207763672,"mana":6133,"flags":0,"total":198.1719207763672,"rngCalls":4} / [3,2,264.8448000000001,0] |
| 22 | 13/7/31 Aff Incinerate | ❌ FAIL | 507.2 | 656.0 | -148.8 (-22.7%) | 53.5 | total: GPU 91300.7265625, CPU 118078.91310795117<br>mana: GPU 1895.3974609375, CPU 1768.4000000000024<br>spent: GPU 26360, CPU 25505.5<br>gained: GPU 21822.40625, CPU 20197.600000000002<br>taps: GPU 26, CPU 24<br>petDamage: GPU 9625.5927734375, CPU 7186.6130957797495<br>damage0: GPU 7781.11572265625, CPU 0<br>casts0: GPU 10, CPU 0<br>hits0: GPU 10, CPU 0<br>crits0: GPU 3, CPU 0<br>damage1: GPU 11469.4462890625, CPU 12727.487849999994<br>crits1: GPU 13, CPU 10<br>damage2: GPU 0, CPU 4522.352010000002<br>casts2: GPU 0, CPU 2<br>hits2: GPU 0, CPU 22<br>crits2: GPU 0, CPU 7<br>damage3: GPU 28593.7734375, CPU 15934.200425000012<br>casts3: GPU 25, CPU 11<br>hits3: GPU 74, CPU 61<br>crits3: GPU 21, CPU 14<br>damage4: GPU 33830.83203125, CPU 38570.847398438855<br>casts4: GPU 35, CPU 34<br>Next RNG output differs: 3185423592,3765689556 / 3512916336,492443627<br>First differing damage event 0: {"time":1,"kind":6,"spell":201,"damage":107.34429931640625,"mana":6433,"flags":0,"total":107.34429931640625,"rngCalls":2} / [1.5,8,450.4500000000001,0] |

## Detailed Per-Spell & Pet Breakdowns

### 1. 5/11/35 DS/AF DS-Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 28287.7 | 39975.9 | 157.2 | 222.1 | -64.9 (-29.2%) | 25 / 26 | DIFF (-29.2%) |
| Corruption | 13391.9 | 14265.0 | 74.4 | 79.3 | -4.9 (-6.1%) | 9 / 8 | DIFF (-6.1%) |
| Bane of Agony | 2611.8 | 3211.5 | 14.5 | 17.8 | -3.3 (-18.7%) | 2 / 2 | DIFF (-18.7%) |
| Curse of Doom | 16769.5 | 20619.7 | 93.2 | 114.6 | -21.4 (-18.7%) | 2 / 2 | DIFF (-18.7%) |
| Immolate | 11967.9 | 16442.8 | 66.5 | 91.3 | -24.9 (-27.2%) | 10 / 11 | DIFF (-27.2%) |
| Conflagrate | 13738.0 | 18874.7 | 76.3 | 104.9 | -28.5 (-27.2%) | 14 / 16 | DIFF (-27.2%) |
| Shadowburn | 7609.0 | 10752.9 | 42.3 | 59.7 | -17.5 (-29.2%) | 9 / 10 | DIFF (-29.2%) |

### 2. 9/11/31 Incinerate - Suppression + DS (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 11797.1 | 13705.7 | 65.5 | 76.1 | -10.6 (-13.9%) | 9 / 9 | DIFF (-13.9%) |
| Bane of Agony | 0.0 | 3855.7 | 0.0 | 21.4 | -21.4 (-100.0%) | 0 / 2 | MISSING (-100%) |
| Curse of Doom | 0.0 | 9961.2 | 0.0 | 55.3 | -55.3 (-100.0%) | 0 / 2 | MISSING (-100%) |
| Immolate | 14475.2 | 17186.2 | 80.4 | 95.5 | -15.1 (-15.8%) | 11 / 11 | DIFF (-15.8%) |
| Incinerate | 35923.8 | 39356.2 | 199.6 | 218.6 | -19.1 (-8.7%) | 31 / 30 | DIFF (-8.7%) |
| Conflagrate | 19689.9 | 23377.5 | 109.4 | 129.9 | -20.5 (-15.8%) | 16 / 17 | DIFF (-15.8%) |
| Shadowburn | 7735.5 | 11054.7 | 43.0 | 61.4 | -18.4 (-30.0%) | 10 / 10 | DIFF (-30.0%) |

### 3. 7/11/33 Incinerate - Suppression + DS (No Corruption) (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Immolate | 18157.3 | 20724.0 | 100.9 | 115.1 | -14.3 (-12.4%) | 10 / 10 | DIFF (-12.4%) |
| Incinerate | 52920.7 | 58082.3 | 294.0 | 322.7 | -28.7 (-8.9%) | 42 / 45 | DIFF (-8.9%) |
| Conflagrate | 16828.3 | 19207.1 | 93.5 | 106.7 | -13.2 (-12.4%) | 17 / 16 | DIFF (-12.4%) |
| Shadowburn | 6492.6 | 8946.7 | 36.1 | 49.7 | -13.6 (-27.4%) | 10 / 10 | DIFF (-27.4%) |

### 4. 3/17/31 Incinerate - DS + Decimate (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Immolate | 16357.6 | 17492.1 | 90.9 | 97.2 | -6.3 (-6.5%) | 11 / 10 | DIFF (-6.5%) |
| Incinerate | 48151.5 | 41039.3 | 267.5 | 228.0 | +39.5 (+17.3%) | 39 / 32 | DIFF (+17.3%) |
| Searing Pain | 0.0 | 3695.3 | 0.0 | 20.5 | -20.5 (-100.0%) | 0 / 5 | MISSING (-100%) |
| Conflagrate | 17430.7 | 18639.6 | 96.8 | 103.6 | -6.7 (-6.5%) | 16 / 15 | DIFF (-6.5%) |
| Soul Fire | 0.0 | 12586.1 | 0.0 | 69.9 | -69.9 (-100.0%) | 0 / 7 | MISSING (-100%) |
| Shadowburn | 6510.2 | 9586.5 | 36.2 | 53.3 | -17.1 (-32.1%) | 10 / 10 | DIFF (-32.1%) |

### 5. 5/11/35 DS/Searing Pain DS-Succ (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 10828.3 | 10936.8 | 60.2 | 60.8 | -0.6 (-1.0%) | 9 / 8 | PASS |
| Bane of Agony | 0.0 | 4155.2 | 0.0 | 23.1 | -23.1 (-100.0%) | 0 / 3 | MISSING (-100%) |
| Curse of Doom | 0.0 | 9961.2 | 0.0 | 55.3 | -55.3 (-100.0%) | 0 / 2 | MISSING (-100%) |
| Immolate | 14047.9 | 18708.3 | 78.0 | 103.9 | -25.9 (-24.9%) | 10 / 11 | DIFF (-24.9%) |
| Searing Pain | 27716.7 | 32400.4 | 154.0 | 180.0 | -26.0 (-14.5%) | 47 / 45 | DIFF (-14.5%) |
| Conflagrate | 14937.4 | 19892.9 | 83.0 | 110.5 | -27.5 (-24.9%) | 15 / 16 | DIFF (-24.9%) |
| Shadowburn | 6494.8 | 9540.9 | 36.1 | 53.0 | -16.9 (-31.9%) | 10 / 11 | DIFF (-31.9%) |

### 6. 2/31/18 DP/AF Shadow (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 62660.8 | 73213.9 | 348.1 | 406.7 | -58.6 (-14.4%) | 40 / 41 | DIFF (-14.4%) |
| Corruption | 14363.6 | 16267.2 | 79.8 | 90.4 | -10.6 (-11.7%) | 9 / 9 | DIFF (-11.7%) |
| Bane of Agony | 5083.9 | 5774.3 | 28.2 | 32.1 | -3.8 (-12.0%) | 2 / 3 | DIFF (-12.0%) |
| Curse of Doom | 12160.1 | 13811.4 | 67.6 | 76.7 | -9.2 (-12.0%) | 2 / 3 | DIFF (-12.0%) |
| Soul Fire | 0.0 | 8587.8 | 0.0 | 47.7 | -47.7 (-100.0%) | 0 / 7 | MISSING (-100%) |
| Succubus Melee (Pet) | 6830.1 | 6151.9 | 37.9 | 34.2 | +3.8 (+11.0%) | 0 / 90 | DIFF (+11.0%) |
| Succubus Lash of Pain (Pet) | 3431.2 | 3090.5 | 19.1 | 17.2 | +1.9 (+11.0%) | 0 / 15 | DIFF (+11.0%) |

### 7. 2/31/18 DP/AF Shadow Corruption (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 54167.9 | 65333.5 | 300.9 | 363.0 | -62.0 (-17.1%) | 39 / 43 | DIFF (-17.1%) |
| Corruption | 15190.0 | 17193.0 | 84.4 | 95.5 | -11.1 (-11.7%) | 9 / 9 | DIFF (-11.7%) |
| Bane of Agony | 4937.0 | 4264.3 | 27.4 | 23.7 | +3.7 (+15.8%) | 3 / 3 | DIFF (+15.8%) |
| Curse of Doom | 15990.3 | 13811.4 | 88.8 | 76.7 | +12.1 (+15.8%) | 3 / 3 | DIFF (+15.8%) |
| Soul Fire | 0.0 | 6019.3 | 0.0 | 33.4 | -33.4 (-100.0%) | 0 / 6 | MISSING (-100%) |
| Succubus Melee (Pet) | 7329.8 | 6983.3 | 40.7 | 38.8 | +1.9 (+5.0%) | 0 / 90 | DIFF (+5.0%) |
| Succubus Lash of Pain (Pet) | 2969.9 | 2829.5 | 16.5 | 15.7 | +0.8 (+5.0%) | 0 / 15 | DIFF (+5.0%) |

### 8. 12/31/8 Aff/DP (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 57639.8 | 66119.1 | 320.2 | 367.3 | -47.1 (-12.8%) | 49 / 45 | DIFF (-12.8%) |
| Corruption | 14316.4 | 18005.5 | 79.5 | 100.0 | -20.5 (-20.5%) | 9 / 9 | DIFF (-20.5%) |
| Bane of Agony | 6296.3 | 6281.9 | 35.0 | 34.9 | +0.1 (+0.2%) | 3 / 3 | PASS |
| Curse of Doom | 16863.5 | 16824.8 | 93.7 | 93.5 | +0.2 (+0.2%) | 2 / 2 | PASS |
| Soul Fire | 0.0 | 8249.8 | 0.0 | 45.8 | -45.8 (-100.0%) | 0 / 6 | MISSING (-100%) |
| Succubus Melee (Pet) | 7199.5 | 6679.6 | 40.0 | 37.1 | +2.9 (+7.8%) | 0 / 90 | DIFF (+7.8%) |
| Succubus Lash of Pain (Pet) | 3136.5 | 2910.0 | 17.4 | 16.2 | +1.3 (+7.8%) | 0 / 15 | DIFF (+7.8%) |

### 9. 12/31/8 Aff/DP Brand (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 52618.0 | 50527.7 | 292.3 | 280.7 | +11.6 (+4.1%) | 49 / 36 | DIFF (+4.1%) |
| Corruption | 12840.9 | 17292.7 | 71.3 | 96.1 | -24.7 (-25.7%) | 9 / 9 | DIFF (-25.7%) |
| Bane of Agony | 6396.5 | 5980.3 | 35.5 | 33.2 | +2.3 (+7.0%) | 3 / 3 | DIFF (+7.0%) |
| Curse of Doom | 13563.9 | 12681.4 | 75.4 | 70.5 | +4.9 (+7.0%) | 2 / 2 | DIFF (+7.0%) |
| Searing Pain | 0.0 | 8394.1 | 0.0 | 46.6 | -46.6 (-100.0%) | 0 / 18 | MISSING (-100%) |
| Soul Fire | 0.0 | 6790.5 | 0.0 | 37.7 | -37.7 (-100.0%) | 0 / 6 | MISSING (-100%) |
| Succubus Melee (Pet) | 7289.8 | 6979.7 | 40.5 | 38.8 | +1.7 (+4.4%) | 0 / 90 | DIFF (+4.4%) |
| Succubus Lash of Pain (Pet) | 3046.1 | 2916.5 | 16.9 | 16.2 | +0.7 (+4.4%) | 0 / 15 | DIFF (+4.4%) |

### 10. 0/31/20 DP/AF Fire (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 0.0 | 13346.0 | 0.0 | 74.1 | -74.1 (-100.0%) | 0 / 11 | MISSING (-100%) |
| Bane of Agony | 4222.8 | 3828.9 | 23.5 | 21.3 | +2.2 (+10.3%) | 3 / 4 | DIFF (+10.3%) |
| Curse of Doom | 10946.6 | 9925.5 | 60.8 | 55.1 | +5.7 (+10.3%) | 2 / 3 | DIFF (+10.3%) |
| Immolate | 32007.3 | 22486.4 | 177.8 | 124.9 | +52.9 (+42.3%) | 34 / 12 | DIFF (+42.3%) |
| Searing Pain | 0.0 | 50962.4 | 0.0 | 283.1 | -283.1 (-100.0%) | 0 / 66 | MISSING (-100%) |
| Imp Firebolt (Pet) | 24380.6 | 17510.3 | 135.4 | 97.3 | +38.2 (+39.2%) | 0 / 84 | DIFF (+39.2%) |

### 11. 2/31/18 DP Fire - Searing Pain (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 0.0 | 13953.6 | 0.0 | 77.5 | -77.5 (-100.0%) | 0 / 10 | MISSING (-100%) |
| Curse of Doom | 0.0 | 12406.9 | 0.0 | 68.9 | -68.9 (-100.0%) | 0 / 3 | MISSING (-100%) |
| Immolate | 16139.8 | 21473.3 | 89.7 | 119.3 | -29.6 (-24.8%) | 11 / 13 | DIFF (-24.8%) |
| Searing Pain | 53861.9 | 48639.3 | 299.2 | 270.2 | +29.0 (+10.7%) | 89 / 63 | DIFF (+10.7%) |
| Soul Fire | 0.0 | 10917.3 | 0.0 | 60.7 | -60.7 (-100.0%) | 0 / 6 | MISSING (-100%) |
| Imp Firebolt (Pet) | 25051.3 | 17501.5 | 139.2 | 97.2 | +41.9 (+43.1%) | 0 / 84 | DIFF (+43.1%) |

### 12. 40/11/0 Deep Affliction DS-Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 13734.9 | 20164.5 | 76.3 | 112.0 | -35.7 (-31.9%) | 9 / 9 | DIFF (-31.9%) |
| Bane of Agony | 5507.6 | 6906.2 | 30.6 | 38.4 | -7.8 (-20.3%) | 3 / 3 | DIFF (-20.3%) |
| Curse of Doom | 9592.2 | 12028.2 | 53.3 | 66.8 | -13.5 (-20.3%) | 2 / 2 | DIFF (-20.3%) |
| Siphon Life | 0.0 | 7361.2 | 0.0 | 40.9 | -40.9 (-100.0%) | 0 / 6 | MISSING (-100%) |
| Wrack / Drain Hope | 0.0 | 43756.3 | 0.0 | 243.1 | -243.1 (-100.0%) | 0 / 24 | MISSING (-100%) |

### 13. 35/6/10 Deep Affliction Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 40745.9 | 9136.9 | 226.4 | 50.8 | +175.6 (+345.9%) | 45 / 8 | DIFF (+345.9%) |
| Corruption | 12022.5 | 15411.7 | 66.8 | 85.6 | -18.8 (-22.0%) | 10 / 9 | DIFF (-22.0%) |
| Bane of Agony | 4099.1 | 5718.7 | 22.8 | 31.8 | -9.0 (-28.3%) | 2 / 2 | DIFF (-28.3%) |
| Curse of Doom | 11586.4 | 16164.3 | 64.4 | 89.8 | -25.4 (-28.3%) | 2 / 2 | DIFF (-28.3%) |
| Siphon Life | 0.0 | 6539.3 | 0.0 | 36.3 | -36.3 (-100.0%) | 0 / 5 | MISSING (-100%) |
| Wrack / Drain Hope | 0.0 | 36534.3 | 0.0 | 203.0 | -203.0 (-100.0%) | 0 / 21 | MISSING (-100%) |
| Imp Firebolt (Pet) | 17271.3 | 6991.6 | 96.0 | 38.8 | +57.1 (+147.0%) | 0 / 59 | DIFF (+147.0%) |

### 14. 32/0/19 SM/AF (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 43006.4 | 64965.1 | 238.9 | 360.9 | -122.0 (-33.8%) | 38 / 43 | DIFF (-33.8%) |
| Corruption | 15014.5 | 19191.1 | 83.4 | 106.6 | -23.2 (-21.8%) | 10 / 10 | DIFF (-21.8%) |
| Bane of Agony | 5927.8 | 5525.3 | 32.9 | 30.7 | +2.2 (+7.3%) | 3 / 3 | DIFF (+7.3%) |
| Curse of Doom | 12824.2 | 11953.4 | 71.2 | 66.4 | +4.8 (+7.3%) | 2 / 2 | DIFF (+7.3%) |
| Shadowburn | 6305.4 | 9524.9 | 35.0 | 52.9 | -17.9 (-33.8%) | 10 / 11 | DIFF (-33.8%) |
| Imp Firebolt (Pet) | 14251.4 | 2145.6 | 79.2 | 11.9 | +67.3 (+564.2%) | 0 / 23 | DIFF (+564.2%) |

### 15. 1/17/33 Incinerate - Decimate + Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 10353.4 | 11516.4 | 57.5 | 64.0 | -6.5 (-10.1%) | 9 / 9 | DIFF (-10.1%) |
| Bane of Agony | 0.0 | 2218.8 | 0.0 | 12.3 | -12.3 (-100.0%) | 0 / 2 | MISSING (-100%) |
| Curse of Doom | 0.0 | 12451.5 | 0.0 | 69.2 | -69.2 (-100.0%) | 0 / 2 | MISSING (-100%) |
| Immolate | 14045.7 | 13666.0 | 78.0 | 75.9 | +2.1 (+2.8%) | 11 / 11 | DIFF (+2.8%) |
| Incinerate | 29154.0 | 23852.7 | 162.0 | 132.5 | +29.5 (+22.2%) | 31 / 20 | DIFF (+22.2%) |
| Searing Pain | 0.0 | 2150.5 | 0.0 | 11.9 | -11.9 (-100.0%) | 0 / 5 | MISSING (-100%) |
| Conflagrate | 14119.9 | 13738.2 | 78.4 | 76.3 | +2.1 (+2.8%) | 16 / 16 | DIFF (+2.8%) |
| Soul Fire | 0.0 | 10193.2 | 0.0 | 56.6 | -56.6 (-100.0%) | 0 / 7 | MISSING (-100%) |
| Shadowburn | 5888.0 | 7312.9 | 32.7 | 40.6 | -7.9 (-19.5%) | 10 / 7 | DIFF (-19.5%) |
| Imp Firebolt (Pet) | 21897.2 | 11150.7 | 121.7 | 61.9 | +59.7 (+96.4%) | 0 / 87 | DIFF (+96.4%) |

### 16. 10/10/31 Incinerate - Suppression + Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 11478.0 | 12220.9 | 63.8 | 67.9 | -4.1 (-6.1%) | 9 / 9 | DIFF (-6.1%) |
| Bane of Agony | 3703.0 | 3256.8 | 20.6 | 18.1 | +2.5 (+13.7%) | 2 / 2 | DIFF (+13.7%) |
| Curse of Doom | 11326.0 | 9961.2 | 62.9 | 55.3 | +7.6 (+13.7%) | 2 / 2 | DIFF (+13.7%) |
| Immolate | 13519.3 | 17216.6 | 75.1 | 95.6 | -20.5 (-21.5%) | 10 / 11 | DIFF (-21.5%) |
| Incinerate | 32641.7 | 35900.6 | 181.3 | 199.4 | -18.1 (-9.1%) | 32 / 33 | DIFF (-9.1%) |
| Conflagrate | 12922.9 | 16457.1 | 71.8 | 91.4 | -19.6 (-21.5%) | 15 / 16 | DIFF (-21.5%) |
| Shadowburn | 7776.0 | 9681.3 | 43.2 | 53.8 | -10.6 (-19.7%) | 10 / 11 | DIFF (-19.7%) |
| Imp Firebolt (Pet) | 21355.4 | 11143.0 | 118.6 | 61.9 | +56.7 (+91.6%) | 0 / 88 | DIFF (+91.6%) |

### 17. 7/13/31 Incinerate - Suppression + Succubus (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 11162.6 | 12630.5 | 62.0 | 70.2 | -8.2 (-11.6%) | 9 / 9 | DIFF (-11.6%) |
| Bane of Agony | 2616.5 | 3107.1 | 14.5 | 17.3 | -2.7 (-15.8%) | 2 / 2 | DIFF (-15.8%) |
| Curse of Doom | 12582.6 | 14941.8 | 69.9 | 83.0 | -13.1 (-15.8%) | 2 / 2 | DIFF (-15.8%) |
| Immolate | 11976.1 | 16891.0 | 66.5 | 93.8 | -27.3 (-29.1%) | 10 / 11 | DIFF (-29.1%) |
| Incinerate | 29814.9 | 33245.9 | 165.6 | 184.7 | -19.1 (-10.3%) | 29 / 29 | DIFF (-10.3%) |
| Conflagrate | 14378.4 | 20279.3 | 79.9 | 112.7 | -32.8 (-29.1%) | 16 / 17 | DIFF (-29.1%) |
| Shadowburn | 6519.0 | 7372.8 | 36.2 | 41.0 | -4.7 (-11.6%) | 9 / 10 | DIFF (-11.6%) |
| Succubus Melee (Pet) | 7863.7 | 6468.3 | 43.7 | 35.9 | +7.8 (+21.6%) | 0 / 90 | DIFF (+21.6%) |
| Succubus Lash of Pain (Pet) | 2315.5 | 1904.7 | 12.9 | 10.6 | +2.3 (+21.6%) | 0 / 15 | DIFF (+21.6%) |

### 18. 2/17/32 Shadow and Flame Shadow - Decimate (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 24583.4 | 25107.0 | 136.6 | 139.5 | -2.9 (-2.1%) | 22 / 17 | DIFF (-2.1%) |
| Corruption | 10809.3 | 11276.1 | 60.1 | 62.6 | -2.6 (-4.1%) | 10 / 9 | DIFF (-4.1%) |
| Bane of Agony | 2493.4 | 2133.8 | 13.9 | 11.9 | +2.0 (+16.9%) | 2 / 1 | DIFF (+16.9%) |
| Curse of Doom | 11640.1 | 9961.2 | 64.7 | 55.3 | +9.3 (+16.9%) | 3 / 2 | DIFF (+16.9%) |
| Immolate | 10735.7 | 13386.4 | 59.6 | 74.4 | -14.7 (-19.8%) | 10 / 10 | DIFF (-19.8%) |
| Searing Pain | 0.0 | 3155.9 | 0.0 | 17.5 | -17.5 (-100.0%) | 0 / 5 | MISSING (-100%) |
| Conflagrate | 14276.4 | 17801.3 | 79.3 | 98.9 | -19.6 (-19.8%) | 15 / 15 | DIFF (-19.8%) |
| Soul Fire | 0.0 | 6230.9 | 0.0 | 34.6 | -34.6 (-100.0%) | 0 / 7 | MISSING (-100%) |
| Shadowburn | 8176.4 | 8350.6 | 45.4 | 46.4 | -1.0 (-2.1%) | 11 / 9 | DIFF (-2.1%) |
| Imp Firebolt (Pet) | 20360.3 | 10751.8 | 113.1 | 59.7 | +53.4 (+89.4%) | 0 / 86 | DIFF (+89.4%) |

### 19. 8/13/30 Shadow and Flame Shadow (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 28823.5 | 36802.7 | 160.1 | 204.5 | -44.3 (-21.7%) | 28 / 28 | DIFF (-21.7%) |
| Corruption | 10787.3 | 13475.6 | 59.9 | 74.9 | -14.9 (-19.9%) | 8 / 9 | DIFF (-19.9%) |
| Bane of Agony | 3154.8 | 4140.2 | 17.5 | 23.0 | -5.5 (-23.8%) | 3 / 3 | DIFF (-23.8%) |
| Curse of Doom | 9487.8 | 12451.5 | 52.7 | 69.2 | -16.5 (-23.8%) | 2 / 2 | DIFF (-23.8%) |
| Immolate | 11561.5 | 13677.8 | 64.2 | 76.0 | -11.8 (-15.5%) | 10 / 10 | DIFF (-15.5%) |
| Conflagrate | 14350.4 | 16977.1 | 79.7 | 94.3 | -14.6 (-15.5%) | 14 / 15 | DIFF (-15.5%) |
| Shadowburn | 7465.1 | 9531.7 | 41.5 | 53.0 | -11.5 (-21.7%) | 10 / 10 | DIFF (-21.7%) |
| Succubus Melee (Pet) | 8046.9 | 6516.4 | 44.7 | 36.2 | +8.5 (+23.5%) | 0 / 90 | DIFF (+23.5%) |
| Succubus Lash of Pain (Pet) | 2359.1 | 1910.4 | 13.1 | 10.6 | +2.5 (+23.5%) | 0 / 15 | DIFF (+23.5%) |

### 20. 19/11/21 NF/DS/Ruin DS-Imp (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 43910.8 | 73255.4 | 243.9 | 407.0 | -163.0 (-40.1%) | 38 / 44 | DIFF (-40.1%) |
| Corruption | 14571.7 | 18534.6 | 81.0 | 103.0 | -22.0 (-21.4%) | 10 / 10 | DIFF (-21.4%) |
| Bane of Agony | 3634.5 | 5605.0 | 20.2 | 31.1 | -10.9 (-35.2%) | 3 / 3 | DIFF (-35.2%) |
| Curse of Doom | 10635.8 | 16402.0 | 59.1 | 91.1 | -32.0 (-35.2%) | 2 / 2 | DIFF (-35.2%) |
| Shadowburn | 7242.5 | 12082.5 | 40.2 | 67.1 | -26.9 (-40.1%) | 9 / 11 | DIFF (-40.1%) |

### 21. 23/10/18 NF/AF (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Shadow Bolt | 48143.3 | 68165.6 | 267.5 | 378.7 | -111.2 (-29.4%) | 45 / 52 | DIFF (-29.4%) |
| Corruption | 13053.0 | 15880.5 | 72.5 | 88.2 | -15.7 (-17.8%) | 10 / 10 | DIFF (-17.8%) |
| Bane of Agony | 3803.2 | 4254.2 | 21.1 | 23.6 | -2.5 (-10.6%) | 3 / 3 | DIFF (-10.6%) |
| Curse of Doom | 12629.2 | 14126.8 | 70.2 | 78.5 | -8.3 (-10.6%) | 2 / 2 | DIFF (-10.6%) |
| Imp Firebolt (Pet) | 20094.5 | 10879.0 | 111.6 | 60.4 | +51.2 (+84.7%) | 0 / 86 | DIFF (+84.7%) |

### 22. 13/7/31 Aff Incinerate (FAIL)

| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |
|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|
| Corruption | 11469.4 | 12727.5 | 63.7 | 70.7 | -7.0 (-9.9%) | 9 / 9 | DIFF (-9.9%) |
| Bane of Agony | 0.0 | 4522.4 | 0.0 | 25.1 | -25.1 (-100.0%) | 0 / 2 | MISSING (-100%) |
| Curse of Doom | 0.0 | 12576.0 | 0.0 | 69.9 | -69.9 (-100.0%) | 0 / 2 | MISSING (-100%) |
| Immolate | 13867.3 | 15934.2 | 77.0 | 88.5 | -11.5 (-13.0%) | 10 / 11 | DIFF (-13.0%) |
| Incinerate | 33830.8 | 38570.8 | 187.9 | 214.3 | -26.3 (-12.3%) | 35 / 34 | DIFF (-12.3%) |
| Conflagrate | 14726.5 | 16921.4 | 81.8 | 94.0 | -12.2 (-13.0%) | 15 / 17 | DIFF (-13.0%) |
| Shadowburn | 7781.1 | 9640.0 | 43.2 | 53.6 | -10.3 (-19.3%) | 10 / 10 | DIFF (-19.3%) |
| Succubus Melee (Pet) | 7544.0 | 5632.5 | 41.9 | 31.3 | +10.6 (+33.9%) | 0 / 90 | DIFF (+33.9%) |
| Succubus Lash of Pain (Pet) | 2081.6 | 1554.2 | 11.6 | 8.6 | +2.9 (+33.9%) | 0 / 15 | DIFF (+33.9%) |

