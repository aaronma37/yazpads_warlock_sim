/**
 * Configuration Generator for Three.js DES GPU Simulation
 * Generates 100 unique configurations across gear sets, talent specs, haste levels,
 * APL priorities, fight durations, and combat buffs.
 */

export const CONFIG_FLOAT_COUNT = 48; // 48 float parameters per configuration (packed into 12 vec4 texels)

export const SPELL_IDS = {
    NONE: 0,
    SHADOW_BOLT: 1,
    IMMOLATE: 2,
    CORRUPTION: 3,
    INCINERATE: 4,
    SEARING_PAIN: 5,
    SOUL_FIRE: 6,
    LIFE_TAP: 7,
    CONFLAGRATE: 8,
    SHADOWBURN: 9,
    CURSE_OF_AGONY: 10,
    CURSE_OF_DOOM: 11,
    SIPHON_LIFE: 12,
    WRACK: 13
};

export const RULE_IDS = {
    LIFE_TAP: 0,
    CURSE_OF_AGONY: 5,
    CURSE_OF_DOOM: 6,
    NIGHTFALL_SHADOW_BOLT: 8,
    DECIMATION_SEARING: 9,
    DECIMATION_SOUL_FIRE: 10,
    DEMONIC_BRAND_SEARING: 11,
    CORRUPTION: 12,
    SIPHON_LIFE: 13,
    WRACK: 14,
    IMMOLATE: 15,
    CONFLAGRATE: 16,
    SHADOWBURN: 17,
    INCINERATE_FILLER: 19,
    SEARING_FILLER: 20,
    SHADOW_BOLT_FILLER: 23,
    NONE: 255
};

export function createBaseConfig(index = 0) {
    return {
        id: index,
        name: `Config #${index + 1}`,
        spec: 'Affliction', // Affliction, Destruction, Demonology, Hybrid
        duration: 120.0,    // Fight length in seconds
        castTime: 2.5,      // Base SB cast time
        gcd: 1.5,           // Global Cooldown (s)
        maxMana: 4500.0,    // Base Mana pool
        spellPower: 650.0,  // Bonus Spell Power
        shadowPower: 650.0,
        firePower: 650.0,
        hit: 0.94,          // Spell Hit chance (capped at 99%)
        crit: 0.22,         // General Spell Crit chance
        boltCrit: 1.5,      // Crit damage multiplier (1.5x base or 2.0x with Ruin)
        dotCrit: 1.0,       // DoT crit multiplier
        tapGain: 750.0,     // Mana gained per Life Tap
        mp5: 45.0,          // Passive mana regen per 5s
        tapThreshold: 0.25, // Life tap when mana below this ratio
        boltCost: 380.0,
        corruptionCost: 290.0,
        agonyCost: 265.0,
        doomCost: 380.0,
        siphonCost: 365.0,
        immolateCost: 380.0,
        conflagCost: 290.0,
        shadowburnCost: 400.0,
        searingCost: 190.0,
        soulFireCost: 995.0,
        wrackCost: 420.0,
        boltMin: 482.0,
        boltMax: 538.0,
        dotBase: 137.0,     // Corruption tick base
        dotMultiplier: 1.0,
        isbBonus: 0.20,     // Improved Shadow Bolt +20% shadow damage
        isbCharges: 4.0,    // 4 charges per ISB crit
        executeBonus: 0.0,  // Execute phase damage boost
        agonyBase: 113.0,
        agonyMultiplier: 1.0,
        doomBase: 3200.0,
        doomMultiplier: 1.0,
        siphonBase: 63.0,
        siphonMultiplier: 1.0,
        immolateCastTime: 1.5,
        immolateMultiplier: 1.0,
        immolateDirectMultiplier: 1.0,
        immolateMin: 279.0,
        immolateMax: 349.0,
        immolateDotBase: 130.0,
        corruptionCastTime: 0.0,
        corruptionMultiplier: 1.0,
        shadowBoltMultiplier: 1.0,
        fireCrit: 0.22,
        conflagMultiplier: 1.0,
        conflagMin: 579.0,
        conflagMax: 681.0,
        shadowburnMultiplier: 1.0,
        sburnMin: 450.0,
        sburnMax: 502.0,
        searingCastTime: 1.5,
        searingMultiplier: 1.0,
        searingCritBonus: 0.10,
        soulFireCastTime: 4.0,
        soulFireMultiplier: 1.0,
        soulFireCooldown: 60.0,
        wrackChannelTime: 5.0,
        wrackTickInterval: 1.0,
        wrackBase: 110.0,
        wrackMultiplier: 1.0,
        targetResist: 0.0,
        spellPen: 0.0,
        trinketBonus: 150.0,
        trinketDuration: 20.0,
        trinketCooldown: 120.0,
        trinketEnabled: 1.0,
        fillerType: 0.0,    // 0: SB, 1: Incinerate, 2: Searing
        snfShadowBonus: 0.0,
        snfFireBonus: 0.0,
        snfConflagSaveChance: 0.0,
        fnbCritBonus: 0.0,
        rules: [
            RULE_IDS.LIFE_TAP,
            RULE_IDS.CURSE_OF_DOOM,
            RULE_IDS.CORRUPTION,
            RULE_IDS.IMMOLATE,
            RULE_IDS.CONFLAGRATE,
            RULE_IDS.SHADOWBURN,
            RULE_IDS.SHADOW_BOLT_FILLER,
            RULE_IDS.NONE
        ]
    };
}

/**
 * Generate an array of 100 diverse configurations
 */
export function generate100Configs() {
    const configs = [];
    const specs = ['Affliction (Shadow)', 'Destruction (Fire)', 'SM/Ruin Classic', 'Demonology/Hybrid', 'Fast Haste Burst'];
    
    for (let i = 0; i < 100; i++) {
        const c = createBaseConfig(i);
        const specIndex = i % specs.length;
        const tier = Math.floor(i / 20); // 5 gear tiers (Dungeon Blues, T1, T2, T2.5, T3 / Naxx BiS)
        
        // Tier scalings
        const spBase = 350.0 + tier * 140.0 + (i % 5) * 15.0;
        const critBase = 0.12 + tier * 0.04 + ((i * 7) % 10) * 0.008;
        const hitBase = Math.min(0.99, 0.86 + tier * 0.025 + ((i * 3) % 6) * 0.01);
        const hasteReduction = (i % 8) * 0.05; // 0% to 35% cast speed reduction
        
        c.spellPower = spBase;
        c.hit = hitBase;
        c.crit = critBase;
        c.maxMana = 3800 + tier * 500 + (i % 10) * 40;
        c.mp5 = 30 + tier * 12;
        c.duration = 45.0 + ((i * 17) % 180); // 45s to 225s fight lengths
        
        if (specIndex === 0) {
            // Affliction Spec
            c.spec = `Affliction T${tier + 1}`;
            c.shadowPower = spBase + 45.0;
            c.firePower = spBase;
            c.shadowBoltMultiplier = 1.10;
            c.dotMultiplier = 1.20;
            c.agonyMultiplier = 1.15;
            c.siphonMultiplier = 1.10;
            c.isbBonus = 0.20;
            c.boltCrit = 1.5;
            c.corruptionCastTime = 0.0; // Instant corruption
            c.fillerType = 0.0; // SB
            c.castTime = Math.max(1.8, 2.5 * (1.0 - hasteReduction));
            c.rules = [
                RULE_IDS.LIFE_TAP,
                RULE_IDS.CURSE_OF_AGONY,
                RULE_IDS.CORRUPTION,
                RULE_IDS.SIPHON_LIFE,
                RULE_IDS.WRACK,
                RULE_IDS.SHADOW_BOLT_FILLER,
                RULE_IDS.NONE,
                RULE_IDS.NONE
            ];
        } else if (specIndex === 1) {
            // Destruction (Fire) Spec
            c.spec = `Fire Destro T${tier + 1}`;
            c.shadowPower = spBase;
            c.firePower = spBase + 60.0;
            c.fireCrit = critBase + 0.08;
            c.boltCrit = 2.0; // Ruin
            c.conflagMultiplier = 1.25;
            c.immolateMultiplier = 1.25;
            c.immolateDirectMultiplier = 1.15;
            c.snfFireBonus = 0.15;
            c.snfShadowBonus = 0.15;
            c.snfConflagSaveChance = (tier >= 3) ? 0.40 : 0.0;
            c.fnbCritBonus = 0.05;
            c.fillerType = 1.0; // Incinerate filler
            c.castTime = Math.max(1.8, 2.25 * (1.0 - hasteReduction));
            c.immolateCastTime = Math.max(1.1, 1.5 * (1.0 - hasteReduction));
            c.rules = [
                RULE_IDS.LIFE_TAP,
                RULE_IDS.CURSE_OF_DOOM,
                RULE_IDS.IMMOLATE,
                RULE_IDS.CONFLAGRATE,
                RULE_IDS.INCINERATE_FILLER,
                RULE_IDS.SHADOWBURN,
                RULE_IDS.NONE,
                RULE_IDS.NONE
            ];
        } else if (specIndex === 2) {
            // SM/Ruin Classic Shadow Spec
            c.spec = `SM/Ruin T${tier + 1}`;
            c.shadowPower = spBase + 35.0;
            c.firePower = spBase;
            c.boltCrit = 2.0; // Ruin (+100% bonus crit damage)
            c.shadowBoltMultiplier = 1.15;
            c.dotMultiplier = 1.10;
            c.isbBonus = 0.20;
            c.isbCharges = 4.0;
            c.corruptionCastTime = 0.0;
            c.fillerType = 0.0; // Shadow Bolt
            c.castTime = Math.max(1.9, 2.5 * (1.0 - hasteReduction));
            c.rules = [
                RULE_IDS.LIFE_TAP,
                RULE_IDS.CURSE_OF_DOOM,
                RULE_IDS.CORRUPTION,
                RULE_IDS.SHADOWBURN,
                RULE_IDS.SHADOW_BOLT_FILLER,
                RULE_IDS.NONE,
                RULE_IDS.NONE,
                RULE_IDS.NONE
            ];
        } else if (specIndex === 3) {
            // Demonology / Searing Brand / Decimation Hybrid
            c.spec = `Decimation Demo T${tier + 1}`;
            c.shadowPower = spBase + 20.0;
            c.firePower = spBase + 20.0;
            c.boltCrit = 1.75;
            c.fireCrit = critBase + 0.05;
            c.executeBonus = 0.25;
            c.fillerType = 2.0; // Searing Pain
            c.castTime = Math.max(1.2, 1.5 * (1.0 - hasteReduction));
            c.searingCastTime = Math.max(1.1, 1.5 * (1.0 - hasteReduction));
            c.rules = [
                RULE_IDS.LIFE_TAP,
                RULE_IDS.DECIMATION_SOUL_FIRE,
                RULE_IDS.DECIMATION_SEARING,
                RULE_IDS.CORRUPTION,
                RULE_IDS.IMMOLATE,
                RULE_IDS.SEARING_FILLER,
                RULE_IDS.SHADOW_BOLT_FILLER,
                RULE_IDS.NONE
            ];
        } else {
            // Fast Haste Burst Spec
            c.spec = `Speed Haste T${tier + 1}`;
            c.shadowPower = spBase;
            c.firePower = spBase;
            c.hit = Math.min(0.99, hitBase + 0.04);
            c.crit = critBase + 0.04;
            c.boltCrit = 2.0;
            c.gcd = Math.max(1.0, 1.5 * (1.0 - hasteReduction * 0.8));
            c.castTime = Math.max(1.6, 2.3 * (1.0 - hasteReduction));
            c.fillerType = 0.0;
            c.trinketBonus = 225.0;
            c.trinketDuration = 20.0;
            c.trinketCooldown = 90.0;
            c.rules = [
                RULE_IDS.LIFE_TAP,
                RULE_IDS.CURSE_OF_DOOM,
                RULE_IDS.CORRUPTION,
                RULE_IDS.SHADOWBURN,
                RULE_IDS.SHADOW_BOLT_FILLER,
                RULE_IDS.NONE,
                RULE_IDS.NONE,
                RULE_IDS.NONE
            ];
        }

        c.name = `Cfg ${i + 1}: ${c.spec} (SP:${Math.round(c.spellPower)}, Crit:${Math.round(c.crit * 100)}%, Hit:${Math.round(c.hit * 100)}%, ${Math.round(c.duration)}s)`;
        configs.push(c);
    }
    
    return configs;
}

/**
 * Packs configuration objects into a Float32Array suitable for a Three.js DataTexture
 * Format: 100 rows, each row has CONFIG_FLOAT_COUNT floats (48 floats = 12 vec4 pixels)
 */
export function packConfigsToFloatArray(configs) {
    const count = configs.length;
    const array = new Float32Array(count * CONFIG_FLOAT_COUNT);
    
    for (let i = 0; i < count; i++) {
        const c = configs[i];
        const offset = i * CONFIG_FLOAT_COUNT;
        
        // Vec4 #0: Basic Fight & Stat parameters
        array[offset + 0] = c.duration;
        array[offset + 1] = c.castTime;
        array[offset + 2] = c.gcd;
        array[offset + 3] = c.maxMana;
        
        // Vec4 #1: Spell Power & Hit/Crit
        array[offset + 4] = c.spellPower;
        array[offset + 5] = c.hit;
        array[offset + 6] = c.crit;
        array[offset + 7] = c.boltCrit;
        
        // Vec4 #2: Resource management & mana
        array[offset + 8] = c.dotCrit;
        array[offset + 9] = c.tapGain;
        array[offset + 10] = c.mp5;
        array[offset + 11] = c.tapThreshold;
        
        // Vec4 #3: Spell Costs (Primary)
        array[offset + 12] = c.boltCost;
        array[offset + 13] = c.corruptionCost;
        array[offset + 14] = c.agonyCost;
        array[offset + 15] = c.doomCost;
        
        // Vec4 #4: Spell Costs (Secondary)
        array[offset + 16] = c.siphonCost;
        array[offset + 17] = c.immolateCost;
        array[offset + 18] = c.conflagCost;
        array[offset + 19] = c.shadowburnCost;
        
        // Vec4 #5: Direct Spell Bases & Multipliers
        array[offset + 20] = c.boltMin;
        array[offset + 21] = c.boltMax;
        array[offset + 22] = c.shadowPower;
        array[offset + 23] = c.shadowBoltMultiplier;
        
        // Vec4 #6: ISB & Execute Mechanics
        array[offset + 24] = c.isbBonus;
        array[offset + 25] = c.isbCharges;
        array[offset + 26] = c.executeBonus;
        array[offset + 27] = c.corruptionCastTime;
        
        // Vec4 #7: Fire Mechanics
        array[offset + 28] = c.firePower;
        array[offset + 29] = c.fireCrit;
        array[offset + 30] = c.conflagMultiplier;
        array[offset + 31] = c.immolateMultiplier;
        
        // Vec4 #8: Fire Spell Bases & Timings
        array[offset + 32] = c.immolateCastTime;
        array[offset + 33] = c.immolateDotBase;
        array[offset + 34] = c.conflagMin;
        array[offset + 35] = c.conflagMax;
        
        // Vec4 #9: Trinket & Buff Modifiers
        array[offset + 36] = c.trinketBonus;
        array[offset + 37] = c.trinketDuration;
        array[offset + 38] = c.trinketCooldown;
        array[offset + 39] = c.trinketEnabled;
        
        // Vec4 #10: APL Rules [0..3]
        array[offset + 40] = c.rules[0] ?? RULE_IDS.NONE;
        array[offset + 41] = c.rules[1] ?? RULE_IDS.NONE;
        array[offset + 42] = c.rules[2] ?? RULE_IDS.NONE;
        array[offset + 43] = c.rules[3] ?? RULE_IDS.NONE;
        
        // Vec4 #11: APL Rules [4..7] & Filler Type
        array[offset + 44] = c.rules[4] ?? RULE_IDS.NONE;
        array[offset + 45] = c.rules[5] ?? RULE_IDS.NONE;
        array[offset + 46] = c.rules[6] ?? RULE_IDS.NONE;
        array[offset + 47] = c.fillerType;
    }
    
    return array;
}
