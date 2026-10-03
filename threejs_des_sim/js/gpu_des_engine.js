/**
 * Three.js GPU Discrete Event Simulation (DES) Engine
 * 
 * Supports:
 * - Dynamic texture sizing: Width = Iterations per Config (up to 1,000+), Height = Total Configs (up to 100).
 * - True parallel Monte Carlo DES execution on GPU (100 configs x 1000 iterations = 100,000 parallel agents).
 * - Multi-pass GPGPU ping-pong stepping.
 * - Statistical aggregation across iterations (Mean DPS, StdDev, Min, Max, Hit/Crit rates).
 * - High-speed CPU Reference runner for benchmark comparisons.
 */

import * as THREE from 'https://unpkg.com/three@0.160.0/build/three.module.js';
import { DES_FRAGMENT_SHADER, DES_PASS_NAMES } from './shaders/des_kernel.glsl.js';
import { packConfigsToFloatArray, RULE_IDS } from './config_generator.js';

export class GpuDesEngine {
    constructor(renderer = null) {
        this.renderer = renderer;
        
        if (!this.renderer) {
            const canvas = document.createElement('canvas');
            this.renderer = new THREE.WebGLRenderer({
                canvas,
                antialias: false,
                alpha: false,
                powerPreference: 'high-performance'
            });
        }

        this.texWidth = 100;  // Iterations per config
        this.texHeight = 100; // Number of configs
        this.totalConfigs = 100;
        this.iterationsPerConfig = 100;

        this.targetsA = {};
        this.targetsB = {};
        this.currentTargets = 'A';

        this.scene = new THREE.Scene();
        this.camera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);
        
        this.configTexture = null;
        this.material = null;
        this.quad = null;
        this.isInitialized = false;

        this._initGpuBuffers(this.texWidth, this.texHeight);
    }

    _initGpuBuffers(width, height) {
        this.texWidth = width;
        this.texHeight = height;

        // Dispose existing if already allocated
        for (const name of DES_PASS_NAMES) {
            if (this.targetsA[name]) this.targetsA[name].dispose();
            if (this.targetsB[name]) this.targetsB[name].dispose();
        }

        const renderTargetParams = {
            wrapS: THREE.ClampToEdgeWrapping,
            wrapT: THREE.ClampToEdgeWrapping,
            minFilter: THREE.NearestFilter,
            magFilter: THREE.NearestFilter,
            format: THREE.RGBAFormat,
            type: THREE.FloatType,
            stencilBuffer: false,
            depthBuffer: false
        };

        for (const name of DES_PASS_NAMES) {
            this.targetsA[name] = new THREE.WebGLRenderTarget(this.texWidth, this.texHeight, renderTargetParams);
            this.targetsB[name] = new THREE.WebGLRenderTarget(this.texWidth, this.texHeight, renderTargetParams);
        }

        this.uniforms = {
            tState0: { value: null },
            tState1: { value: null },
            tState2: { value: null },
            tState3: { value: null },
            tState4: { value: null },
            tState5: { value: null },
            tState6: { value: null },
            tDmg0: { value: null },
            tDmg1: { value: null },
            tDmg2: { value: null },
            tCounts0: { value: null },
            tCounts1: { value: null },
            tCounts2: { value: null },
            tConfigs: { value: null },
            uTotalConfigs: { value: Number(this.texHeight) },
            uIterationsPerConfig: { value: Number(this.texWidth) },
            uResolution: { value: new THREE.Vector2(this.texWidth, this.texHeight) },
            uChunkSize: { value: 64.0 },
            uSeed: { value: Math.floor(Math.random() * 1000000) },
            uOutputTarget: { value: 0 }
        };

        if (this.material) this.material.dispose();

        this.material = new THREE.ShaderMaterial({
            uniforms: this.uniforms,
            vertexShader: `
                varying vec2 vUv;
                void main() {
                    vUv = uv;
                    gl_Position = vec4(position, 1.0);
                }
            `,
            fragmentShader: DES_FRAGMENT_SHADER,
            depthTest: false,
            depthWrite: false
        });

        if (!this.quad) {
            const geometry = new THREE.PlaneGeometry(2, 2);
            this.quad = new THREE.Mesh(geometry, this.material);
            this.scene.add(this.quad);
        } else {
            this.quad.material = this.material;
        }
    }

    uploadConfigs(configs) {
        this.totalConfigs = configs.length;
        const floatData = packConfigsToFloatArray(configs);
        
        if (this.configTexture) this.configTexture.dispose();

        this.configTexture = new THREE.DataTexture(
            floatData,
            12,
            configs.length,
            THREE.RGBAFormat,
            THREE.FloatType
        );
        this.configTexture.needsUpdate = true;
        this.configTexture.minFilter = THREE.NearestFilter;
        this.configTexture.magFilter = THREE.NearestFilter;

        this.uniforms.tConfigs.value = this.configTexture;
        this.uniforms.uTotalConfigs.value = this.totalConfigs;
    }

    resetSimulation(seed = null) {
        this.uniforms.uSeed.value = (seed !== null) ? seed : Math.floor(Math.random() * 1000000);

        const oldTarget = this.renderer.getRenderTarget();

        for (const name of DES_PASS_NAMES) {
            this.renderer.setRenderTarget(this.targetsA[name]);
            this.renderer.clear(true, true, true);
            this.renderer.setRenderTarget(this.targetsB[name]);
            this.renderer.clear(true, true, true);
        }

        this.renderer.setRenderTarget(oldTarget);
        this.currentTargets = 'A';
        this.isInitialized = true;
    }

    stepChunk(chunkSize = 64) {
        this.uniforms.uChunkSize.value = chunkSize;
        
        const readTargets = (this.currentTargets === 'A') ? this.targetsA : this.targetsB;
        const writeTargets = (this.currentTargets === 'A') ? this.targetsB : this.targetsA;

        this.uniforms.tState0.value = readTargets.state0.texture;
        this.uniforms.tState1.value = readTargets.state1.texture;
        this.uniforms.tState2.value = readTargets.state2.texture;
        this.uniforms.tState3.value = readTargets.state3.texture;
        this.uniforms.tState4.value = readTargets.state4.texture;
        this.uniforms.tState5.value = readTargets.state5.texture;
        this.uniforms.tState6.value = readTargets.state6.texture;
        this.uniforms.tDmg0.value = readTargets.dmg0.texture;
        this.uniforms.tDmg1.value = readTargets.dmg1.texture;
        this.uniforms.tDmg2.value = readTargets.dmg2.texture;
        this.uniforms.tCounts0.value = readTargets.counts0.texture;
        this.uniforms.tCounts1.value = readTargets.counts1.texture;
        this.uniforms.tCounts2.value = readTargets.counts2.texture;

        const oldTarget = this.renderer.getRenderTarget();

        // Step all state targets
        for (let i = 0; i < DES_PASS_NAMES.length; i++) {
            const passName = DES_PASS_NAMES[i];
            this.uniforms.uOutputTarget.value = i;
            this.renderer.setRenderTarget(writeTargets[passName]);
            this.renderer.render(this.scene, this.camera);
        }

        this.renderer.setRenderTarget(oldTarget);
        this.currentTargets = (this.currentTargets === 'A') ? 'B' : 'A';
    }

    async runBatch(configs, options = {}) {
        const {
            iterationsPerConfig = 100,
            chunkSize = 64,
            maxDispatches = 100,
            onProgress = null,
            seed = 1337
        } = options;

        this.iterationsPerConfig = iterationsPerConfig;
        this.totalConfigs = configs.length;

        // Resize buffers if dimensions changed
        if (this.texWidth !== iterationsPerConfig || this.texHeight !== configs.length) {
            this._initGpuBuffers(iterationsPerConfig, configs.length);
        }

        this.uploadConfigs(configs);
        this.uniforms.uIterationsPerConfig.value = iterationsPerConfig;
        this.uniforms.uResolution.value.set(iterationsPerConfig, configs.length);
        this.resetSimulation(seed);

        const startTime = performance.now();
        let totalDispatches = 0;

        for (let d = 0; d < maxDispatches; d++) {
            this.stepChunk(chunkSize);
            totalDispatches++;

            if (onProgress && (d % 10 === 0 || d === maxDispatches - 1)) {
                onProgress({
                    progress: Math.min(1.0, (d + 1) / maxDispatches),
                    dispatches: totalDispatches,
                    estimatedEvents: totalDispatches * chunkSize * this.totalConfigs * iterationsPerConfig
                });
                await new Promise(resolve => setTimeout(resolve, 0));
            }
        }

        const gpuDurationMs = performance.now() - startTime;
        const results = this.readResults(configs, iterationsPerConfig);

        return {
            results,
            gpuDurationMs,
            totalDispatches,
            totalInstances: configs.length * iterationsPerConfig,
            eventsProcessed: results.reduce((acc, r) => acc + (r.eventCount * iterationsPerConfig), 0),
            totalDpsAvg: results.reduce((acc, r) => acc + r.dps, 0) / results.length,
            simulatedSeconds: configs.reduce((acc, c) => acc + (c.duration * iterationsPerConfig), 0)
        };
    }

    readResults(configs, iterations) {
        const currentTargets = (this.currentTargets === 'A') ? this.targetsA : this.targetsB;
        const pixelCount = this.texWidth * this.texHeight * 4;

        const bufferState0 = new Float32Array(pixelCount);
        const bufferDmg0 = new Float32Array(pixelCount);
        const bufferDmg1 = new Float32Array(pixelCount);
        const bufferDmg2 = new Float32Array(pixelCount);
        const bufferCounts0 = new Float32Array(pixelCount);
        const bufferCounts1 = new Float32Array(pixelCount);
        const bufferCounts2 = new Float32Array(pixelCount);

        this.renderer.readRenderTargetPixels(currentTargets.state0, 0, 0, this.texWidth, this.texHeight, bufferState0);
        this.renderer.readRenderTargetPixels(currentTargets.dmg0, 0, 0, this.texWidth, this.texHeight, bufferDmg0);
        this.renderer.readRenderTargetPixels(currentTargets.dmg1, 0, 0, this.texWidth, this.texHeight, bufferDmg1);
        this.renderer.readRenderTargetPixels(currentTargets.dmg2, 0, 0, this.texWidth, this.texHeight, bufferDmg2);
        this.renderer.readRenderTargetPixels(currentTargets.counts0, 0, 0, this.texWidth, this.texHeight, bufferCounts0);
        this.renderer.readRenderTargetPixels(currentTargets.counts1, 0, 0, this.texWidth, this.texHeight, bufferCounts1);
        this.renderer.readRenderTargetPixels(currentTargets.counts2, 0, 0, this.texWidth, this.texHeight, bufferCounts2);

        const results = [];

        for (let row = 0; row < configs.length; row++) {
            const config = configs[row];
            
            let sumDps = 0;
            let sumTotalDmg = 0;
            let sumBoltDmg = 0;
            let sumDotDmg = 0;
            let sumAgonyDmg = 0;
            let sumDoomDmg = 0;
            let sumSiphonDmg = 0;
            let sumImmolateDmg = 0;
            let sumIncinerateDmg = 0;
            let sumConflagDmg = 0;
            let sumShadowburnDmg = 0;
            let sumSearingDmg = 0;
            let sumWrackDmg = 0;

            let sumCasts = 0;
            let sumHits = 0;
            let sumCrits = 0;
            let sumMisses = 0;
            let sumTaps = 0;
            let sumEvents = 0;
            let minDps = Infinity;
            let maxDps = -Infinity;

            for (let col = 0; col < iterations; col++) {
                const idx = (row * this.texWidth + col) * 4;

                const now = bufferState0[idx + 0];
                const simTime = (now > 0) ? now : config.duration;
                const totalDmg = bufferDmg0[idx + 0];
                const dps = totalDmg / Math.max(1.0, simTime);

                sumDps += dps;
                sumTotalDmg += totalDmg;
                sumBoltDmg += bufferDmg0[idx + 1];
                sumDotDmg += bufferDmg0[idx + 2];
                sumAgonyDmg += bufferDmg0[idx + 3];

                sumDoomDmg += bufferDmg1[idx + 0];
                sumSiphonDmg += bufferDmg1[idx + 1];
                sumImmolateDmg += bufferDmg1[idx + 2];
                sumIncinerateDmg += bufferDmg1[idx + 3];

                sumConflagDmg += bufferDmg2[idx + 0];
                sumShadowburnDmg += bufferDmg2[idx + 1];
                sumSearingDmg += bufferDmg2[idx + 2];
                sumWrackDmg += bufferDmg2[idx + 3];

                sumCasts += bufferCounts0[idx + 0];
                sumHits += bufferCounts0[idx + 1];
                sumCrits += bufferCounts0[idx + 2];
                sumMisses += bufferCounts0[idx + 3];

                sumTaps += bufferCounts1[idx + 0];
                sumEvents += bufferCounts2[idx + 0];

                if (dps < minDps) minDps = dps;
                if (dps > maxDps) maxDps = dps;
            }

            const invN = 1.0 / Math.max(1, iterations);
            const meanDps = sumDps * invN;

            results.push({
                configId: config.id,
                name: config.name,
                spec: config.spec,
                duration: config.duration,
                iterations,
                simTime: config.duration,
                totalDamage: sumTotalDmg * invN,
                dps: meanDps,
                minDps: (minDps === Infinity) ? meanDps : minDps,
                maxDps: (maxDps === -Infinity) ? meanDps : maxDps,
                casts: Math.round(sumCasts * invN),
                hits: Math.round(sumHits * invN),
                crits: Math.round(sumCrits * invN),
                misses: Math.round(sumMisses * invN),
                critRate: (sumHits > 0) ? (sumCrits / sumHits) : 0,
                hitRate: ((sumHits + sumMisses) > 0) ? (sumHits / (sumHits + sumMisses)) : 0,
                taps: Math.round(sumTaps * invN),
                eventCount: Math.round(sumEvents * invN),
                damageBreakdown: {
                    shadowBolt: sumBoltDmg * invN,
                    corruption: sumDotDmg * invN,
                    agony: sumAgonyDmg * invN,
                    doom: sumDoomDmg * invN,
                    siphon: sumSiphonDmg * invN,
                    immolate: sumImmolateDmg * invN,
                    incinerate: sumIncinerateDmg * invN,
                    conflagrate: sumConflagDmg * invN,
                    shadowburn: sumShadowburnDmg * invN,
                    searingPain: sumSearingDmg * invN,
                    wrack: sumWrackDmg * invN
                }
            });
        }

        return results;
    }

    runCpuReference(configs, iterations = 10, seed = 1337) {
        const startTime = performance.now();
        const NEVER = 999999.0;
        const results = [];

        for (let cIdx = 0; cIdx < configs.length; cIdx++) {
            const c = configs[cIdx];
            let sumDps = 0;
            let sumTotalDamage = 0;
            let sumCasts = 0;
            let sumHits = 0;
            let sumCrits = 0;
            let sumMisses = 0;
            let sumEvents = 0;

            for (let iter = 0; iter < iterations; iter++) {
                let rng = ((seed + (cIdx * iterations + iter) * 1664525) % 4294967296) + 1;
                const nextRandom = () => {
                    rng = (rng * 1664525 + 1013904223) % 4294967296;
                    return rng / 4294967296;
                };

                let now = 0.0;
                let mana = c.maxMana;
                let ready = 0.0;
                let castEnd = NEVER;
                let castSpellId = 0;
                let regenNext = 5.0;
                let dotNext = NEVER;
                let dotTicks = 0;
                let agonyNext = NEVER;
                let agonyTicks = 0;
                let doomNext = NEVER;
                let siphonNext = NEVER;
                let siphonTicks = 0;
                let immolateNext = NEVER;
                let immolateTicks = 0;
                let wrackNext = NEVER;
                let wrackTicks = 0;
                let conflagCdReady = 0.0;
                let shadowburnCdReady = 0.0;
                let snfShadowEnd = 0.0;
                let snfFireEnd = 0.0;
                let isbExpire = 0.0;
                let isbCharges = 0;
                let trinketEnd = 0.0;
                let trinketReady = 0.0;

                let totalDamage = 0.0;
                let casts = 0;
                let hits = 0;
                let crits = 0;
                let misses = 0;
                let eventCount = 0;
                let loops = 0;

                while (now < c.duration && loops < 5000) {
                    loops++;
                    const curShadowPower = c.shadowPower + (now < trinketEnd ? c.trinketBonus : 0.0);
                    const curFirePower = c.firePower + (now < trinketEnd ? c.trinketBonus : 0.0);

                    // 1. Trinket
                    if (c.trinketEnabled > 0 && now >= trinketReady && c.trinketCooldown > 0) {
                        trinketEnd = now + c.trinketDuration;
                        trinketReady = now + c.trinketCooldown;
                        eventCount++;
                    }

                    // 2. MP5
                    if (now >= regenNext) {
                        mana = Math.min(c.maxMana, mana + c.mp5);
                        regenNext = now + 5.0;
                        eventCount++;
                    }

                    // 3. DoT Ticks
                    if (dotTicks > 0 && now >= dotNext) {
                        dotTicks--;
                        dotNext = (dotTicks > 0) ? now + 3.0 : NEVER;
                        let tickDmg = (137.0 + (curShadowPower * 0.1666)) * (now < snfShadowEnd ? 1.15 : 1.0);
                        if (isbExpire > now) {
                            tickDmg *= (1.0 + c.isbBonus);
                            if (isbCharges > 0) {
                                isbCharges--;
                                if (isbCharges <= 0) isbExpire = 0.0;
                            }
                        }
                        totalDamage += tickDmg;
                        eventCount++;
                    }

                    if (agonyTicks > 0 && now >= agonyNext) {
                        agonyTicks--;
                        agonyNext = (agonyTicks > 0) ? now + 2.0 : NEVER;
                        let agonyDmg = 113.0 + (curShadowPower * 0.0833);
                        if (isbExpire > now) agonyDmg *= (1.0 + c.isbBonus);
                        totalDamage += agonyDmg;
                        eventCount++;
                    }

                    if (now >= doomNext && doomNext < NEVER) {
                        doomNext = NEVER;
                        let dDmg = 3200.0 + curShadowPower * 2.0;
                        if (isbExpire > now) dDmg *= (1.0 + c.isbBonus);
                        if (nextRandom() < c.crit) {
                            dDmg *= c.boltCrit;
                            crits++;
                        }
                        totalDamage += dDmg;
                        eventCount++;
                    }

                    if (siphonTicks > 0 && now >= siphonNext) {
                        siphonTicks--;
                        siphonNext = (siphonTicks > 0) ? now + 3.0 : NEVER;
                        let sDmg = 63.0 + (curShadowPower * 0.10);
                        if (isbExpire > now) sDmg *= (1.0 + c.isbBonus);
                        totalDamage += sDmg;
                        eventCount++;
                    }

                    if (immolateTicks > 0 && now >= immolateNext) {
                        immolateTicks--;
                        immolateNext = (immolateTicks > 0) ? now + 3.0 : NEVER;
                        let iDmg = (c.immolateDotBase + (curFirePower * 0.13)) * c.immolateMultiplier;
                        if (now < snfFireEnd) iDmg *= 1.15;
                        totalDamage += iDmg;
                        eventCount++;
                    }

                    if (wrackTicks > 0 && now >= wrackNext) {
                        wrackTicks--;
                        wrackNext = (wrackTicks > 0) ? now + 1.0 : NEVER;
                        let wDmg = 110.0 + curShadowPower * 0.20;
                        if (isbExpire > now) wDmg *= (1.0 + c.isbBonus);
                        totalDamage += wDmg;
                        eventCount++;
                    }

                    // 4. Cast Completion
                    if (castSpellId > 0 && now >= castEnd) {
                        const spell = castSpellId;
                        castSpellId = 0;
                        castEnd = NEVER;
                        eventCount++;

                        if (spell === 1) { // SB
                            casts++;
                            if (nextRandom() < c.hit) {
                                hits++;
                                let dmg = (c.boltMin + nextRandom() * (c.boltMax - c.boltMin) + (3.0 / 3.5) * curShadowPower) * c.shadowBoltMultiplier;
                                if (now < snfShadowEnd) dmg *= 1.15;
                                if (now >= c.duration * 0.65) dmg *= (1.0 + c.executeBonus);
                                if (isbExpire > now) {
                                    dmg *= (1.0 + c.isbBonus);
                                    if (isbCharges > 0) {
                                        isbCharges--;
                                        if (isbCharges <= 0) isbExpire = 0.0;
                                    }
                                }
                                if (nextRandom() < c.crit) {
                                    dmg *= c.boltCrit;
                                    crits++;
                                    if (c.isbBonus > 0) {
                                        isbExpire = now + 12.0;
                                        isbCharges = c.isbCharges;
                                    }
                                }
                                totalDamage += dmg;
                            } else {
                                misses++;
                            }
                        } else if (spell === 2) { // Immolate
                            casts++;
                            if (nextRandom() < c.hit) {
                                hits++;
                                let dmg = (279.0 + nextRandom() * 70.0 + (1.5 / 3.5) * curFirePower) * c.immolateMultiplier;
                                if (now < snfFireEnd) dmg *= 1.15;
                                if (nextRandom() < c.fireCrit) {
                                    dmg *= c.boltCrit;
                                    crits++;
                                }
                                totalDamage += dmg;
                                immolateTicks = 5;
                                immolateNext = now + 3.0;
                            } else {
                                misses++;
                            }
                        } else if (spell === 4) { // Incinerate
                            casts++;
                            if (nextRandom() < c.hit) {
                                hits++;
                                let dmg = (201.0 + nextRandom() * 32.0 + (2.5 / 3.5) * curFirePower) * c.conflagMultiplier;
                                if (immolateTicks > 0) dmg *= 1.25;
                                if (now < snfFireEnd) dmg *= 1.15;
                                if (nextRandom() < c.fireCrit) {
                                    dmg *= c.boltCrit;
                                    crits++;
                                }
                                totalDamage += dmg;
                            } else {
                                misses++;
                            }
                        } else if (spell === 5) { // Searing Pain
                            casts++;
                            if (nextRandom() < c.hit) {
                                hits++;
                                let dmg = (108.0 + nextRandom() * 19.0 + (1.5 / 3.5) * curFirePower);
                                if (now >= c.duration * 0.65) dmg *= (1.0 + c.executeBonus);
                                if (now < snfFireEnd) dmg *= 1.15;
                                if (nextRandom() < (c.fireCrit + 0.10)) {
                                    dmg *= c.boltCrit;
                                    crits++;
                                }
                                totalDamage += dmg;
                            } else {
                                misses++;
                            }
                        }
                    }

                    // 5. APL
                    if (now >= ready && castSpellId === 0) {
                        let acted = false;
                        for (const rule of c.rules) {
                            if (acted || rule === RULE_IDS.NONE) break;

                            if (rule === RULE_IDS.LIFE_TAP) {
                                if (mana <= c.maxMana * c.tapThreshold) {
                                    mana = Math.min(c.maxMana, mana + c.tapGain);
                                    ready = now + c.gcd;
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.CURSE_OF_DOOM) {
                                if (c.doomCost > 0 && doomNext >= NEVER && agonyTicks === 0 && (now + 60.0 <= c.duration) && mana >= c.doomCost) {
                                    mana -= c.doomCost;
                                    if (nextRandom() < c.hit) doomNext = now + 60.0;
                                    else misses++;
                                    ready = now + c.gcd;
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.CURSE_OF_AGONY) {
                                if (c.agonyCost > 0 && agonyTicks === 0 && doomNext >= NEVER && mana >= c.agonyCost) {
                                    mana -= c.agonyCost;
                                    if (nextRandom() < c.hit) { agonyTicks = 12; agonyNext = now + 2.0; }
                                    else misses++;
                                    ready = now + c.gcd;
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.CORRUPTION) {
                                if (c.corruptionCost > 0 && dotTicks === 0 && mana >= c.corruptionCost) {
                                    mana -= c.corruptionCost;
                                    if (c.corruptionCastTime <= 0) {
                                        if (nextRandom() < c.hit) { dotTicks = 6; dotNext = now + 3.0; }
                                        else misses++;
                                        ready = now + c.gcd;
                                    } else {
                                        castEnd = now + c.corruptionCastTime;
                                        castSpellId = 3;
                                        ready = Math.max(now + c.gcd, castEnd);
                                    }
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.SIPHON_LIFE) {
                                if (c.siphonCost > 0 && siphonTicks === 0 && mana >= c.siphonCost) {
                                    mana -= c.siphonCost;
                                    if (nextRandom() < c.hit) { siphonTicks = 10; siphonNext = now + 3.0; }
                                    else misses++;
                                    ready = now + c.gcd;
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.IMMOLATE) {
                                if (c.immolateCost > 0 && immolateTicks === 0 && mana >= c.immolateCost) {
                                    mana -= c.immolateCost;
                                    castEnd = now + c.immolateCastTime;
                                    castSpellId = 2;
                                    ready = Math.max(now + c.gcd, castEnd);
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.CONFLAGRATE) {
                                if (c.conflagCost > 0 && immolateTicks > 0 && now >= conflagCdReady && mana >= c.conflagCost) {
                                    mana -= c.conflagCost;
                                    conflagCdReady = now + 10.0;
                                    if (nextRandom() < c.hit) {
                                        let cDmg = (c.conflagMin + nextRandom() * (c.conflagMax - c.conflagMin) + (1.5 / 3.5) * curFirePower) * c.conflagMultiplier;
                                        if (now < snfFireEnd) cDmg *= 1.15;
                                        if (nextRandom() < c.fireCrit) { cDmg *= c.boltCrit; crits++; }
                                        totalDamage += cDmg;
                                        snfShadowEnd = now + 20.0;
                                    } else misses++;
                                    immolateTicks = 0;
                                    immolateNext = NEVER;
                                    ready = now + c.gcd;
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.SHADOWBURN) {
                                if (c.shadowburnCost > 0 && now >= shadowburnCdReady && mana >= c.shadowburnCost) {
                                    mana -= c.shadowburnCost;
                                    shadowburnCdReady = now + 15.0;
                                    if (nextRandom() < c.hit) {
                                        let sDmg = (450.0 + nextRandom() * 52.0 + (1.5 / 3.5) * curShadowPower);
                                        if (now < snfShadowEnd) sDmg *= 1.15;
                                        if (isbExpire > now) {
                                            sDmg *= (1.0 + c.isbBonus);
                                            if (isbCharges > 0) {
                                                isbCharges--;
                                                if (isbCharges <= 0) isbExpire = 0.0;
                                            }
                                        }
                                        if (nextRandom() < c.crit) { sDmg *= c.boltCrit; crits++; }
                                        totalDamage += sDmg;
                                        snfFireEnd = now + 20.0;
                                    } else misses++;
                                    ready = now + c.gcd;
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.SHADOW_BOLT_FILLER) {
                                if (mana >= c.boltCost) {
                                    mana -= c.boltCost;
                                    castEnd = now + c.castTime;
                                    castSpellId = 1;
                                    ready = Math.max(now + c.gcd, castEnd);
                                    acted = true;
                                    eventCount++;
                                }
                            } else if (rule === RULE_IDS.INCINERATE_FILLER) {
                                if (mana >= c.boltCost) {
                                    mana -= c.boltCost;
                                    castEnd = now + c.castTime;
                                    castSpellId = 4;
                                    ready = Math.max(now + c.gcd, castEnd);
                                    acted = true;
                                    eventCount++;
                                }
                            }
                        }

                        if (!acted) {
                            if (mana >= c.boltCost) {
                                mana -= c.boltCost;
                                castEnd = now + c.castTime;
                                castSpellId = (c.fillerType === 1) ? 4 : (c.fillerType === 2) ? 5 : 1;
                                ready = Math.max(now + c.gcd, castEnd);
                            } else {
                                mana = Math.min(c.maxMana, mana + c.tapGain);
                                ready = now + c.gcd;
                            }
                            eventCount++;
                        }
                    }

                    // 6. Next Discrete Event
                    const nextDot = Math.min(dotNext, Math.min(agonyNext, Math.min(doomNext, Math.min(siphonNext, Math.min(immolateNext, wrackNext)))));
                    const nextTrinket = (c.trinketEnabled > 0) ? trinketReady : NEVER;
                    let nextNow = Math.min(ready, Math.min(castEnd, Math.min(nextDot, Math.min(regenNext, nextTrinket))));
                    if (nextNow <= now) nextNow = now + 0.001;

                    now = nextNow;
                }

                const simTime = (now > 0) ? now : c.duration;
                sumDps += totalDamage / Math.max(1.0, simTime);
                sumTotalDamage += totalDamage;
                sumCasts += casts;
                sumHits += hits;
                sumCrits += crits;
                sumMisses += misses;
                sumEvents += eventCount;
            }

            const invN = 1.0 / iterations;
            results.push({
                configId: c.id,
                name: c.name,
                spec: c.spec,
                duration: c.duration,
                iterations,
                totalDamage: sumTotalDamage * invN,
                dps: sumDps * invN,
                casts: Math.round(sumCasts * invN),
                hits: Math.round(sumHits * invN),
                crits: Math.round(sumCrits * invN),
                misses: Math.round(sumMisses * invN),
                critRate: (sumHits > 0) ? (sumCrits / sumHits) : 0,
                hitRate: ((sumHits + sumMisses) > 0) ? (sumHits / (sumHits + sumMisses)) : 0,
                eventCount: Math.round(sumEvents * invN)
            });
        }

        const cpuDurationMs = performance.now() - startTime;
        return {
            results,
            cpuDurationMs,
            totalInstances: configs.length * iterations,
            eventsProcessed: results.reduce((acc, r) => acc + (r.eventCount * iterations), 0),
            totalDpsAvg: results.reduce((acc, r) => acc + r.dps, 0) / results.length
        };
    }
}
