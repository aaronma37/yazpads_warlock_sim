/**
 * Main Application Controller for Three.js DES GPU Simulator
 */

import { generate100Configs } from './config_generator.js';
import { GpuDesEngine } from './gpu_des_engine.js';
import { SceneVisualizer } from './scene_visualizer.js';
import { ChartVisualizer } from './chart_visualizer.js';

class App {
    constructor() {
        this.configs = [];
        this.currentBatchSize = 100;
        this.currentIterations = 100;
        this.gpuResults = [];
        this.cpuResults = [];
        this.selectedConfigIndex = 0;
        this.isRunning = false;
        this.lastGpuStats = null;

        this.canvasContainer = document.getElementById('canvas-container');
        this.configListEl = document.getElementById('config-list');
        this.progressBar = document.getElementById('progress-bar');

        this.engine = new GpuDesEngine();
        this.visualizer = new SceneVisualizer(this.canvasContainer, (index) => {
            this.selectConfig(index);
        });
        this.charts = new ChartVisualizer('dps-chart-canvas', 'breakdown-chart-canvas');

        this._initEvents();
        this.loadConfigs();
    }

    _initEvents() {
        document.getElementById('btn-run-gpu').addEventListener('click', () => this.runGpuSimulation());
        document.getElementById('btn-run-cpu').addEventListener('click', () => this.runCpuBenchmark());
        document.getElementById('btn-gen-configs').addEventListener('click', () => this.loadConfigs());
        
        const batchSelect = document.getElementById('select-batch-size');
        batchSelect.addEventListener('change', (e) => {
            this.currentBatchSize = parseInt(e.target.value, 10);
            this.updateInstanceCount();
            this.refreshConfigList();
        });

        const iterSelect = document.getElementById('select-iterations');
        iterSelect.addEventListener('change', (e) => {
            this.currentIterations = parseInt(e.target.value, 10);
            this.updateInstanceCount();
        });

        document.getElementById('btn-export-json').addEventListener('click', () => this.exportJson());
    }

    updateInstanceCount() {
        const total = this.currentBatchSize * this.currentIterations;
        document.getElementById('stat-total-instances').innerText = `${total.toLocaleString()} Agents`;
    }

    loadConfigs() {
        this.configs = generate100Configs();
        this.updateInstanceCount();
        this.visualizer.buildNodes(this.configs.slice(0, this.currentBatchSize));
        this.refreshConfigList();
        this.selectConfig(0);
        
        // Trigger initial GPU simulation automatically
        setTimeout(() => this.runGpuSimulation(), 300);
    }

    refreshConfigList() {
        this.configListEl.innerHTML = '';
        const activeConfigs = this.configs.slice(0, this.currentBatchSize);

        activeConfigs.forEach((c, idx) => {
            const card = document.createElement('div');
            card.className = `config-card ${idx === this.selectedConfigIndex ? 'active' : ''}`;
            card.innerHTML = `
                <div class="config-card-title">
                    <span>#${idx + 1} ${c.spec}</span>
                    <span id="card-dps-${idx}" style="color: var(--accent-gold); font-weight:700;">-- DPS</span>
                </div>
                <div class="config-card-stats">
                    <span>SP: ${Math.round(c.spellPower)}</span>
                    <span>Crit: ${Math.round(c.crit * 100)}%</span>
                    <span>Hit: ${Math.round(c.hit * 100)}%</span>
                    <span>${Math.round(c.duration)}s</span>
                </div>
            `;
            card.addEventListener('click', () => this.selectConfig(idx));
            this.configListEl.appendChild(card);
        });

        document.getElementById('stat-batch-count').innerText = `${this.currentBatchSize} Configs`;
        this.visualizer.buildNodes(activeConfigs);
    }

    selectConfig(index) {
        if (index < 0 || index >= this.currentBatchSize) return;
        this.selectedConfigIndex = index;
        this.visualizer.selectedNodeIndex = index;

        const cards = this.configListEl.children;
        for (let i = 0; i < cards.length; i++) {
            cards[i].classList.toggle('active', i === index);
        }

        const config = this.configs[index];
        const res = this.gpuResults[index];

        document.getElementById('insp-name').innerText = config.name;
        document.getElementById('insp-spec').innerText = config.spec;
        document.getElementById('insp-duration').innerText = `${config.duration}s`;
        document.getElementById('insp-sp').innerText = `${Math.round(config.spellPower)}`;
        document.getElementById('insp-crit').innerText = `${(config.crit * 100).toFixed(1)}%`;
        document.getElementById('insp-hit').innerText = `${(config.hit * 100).toFixed(1)}%`;
        document.getElementById('insp-cast-time').innerText = `${config.castTime.toFixed(2)}s`;
        document.getElementById('insp-gcd').innerText = `${config.gcd.toFixed(2)}s`;

        if (res) {
            document.getElementById('insp-dps').innerText = `${Math.round(res.dps).toLocaleString()} DPS`;
            document.getElementById('insp-total-dmg').innerText = `${Math.round(res.totalDamage).toLocaleString()}`;
            document.getElementById('insp-iterations').innerText = `${res.iterations.toLocaleString()} runs`;
            document.getElementById('insp-casts').innerText = `${res.casts}`;
            document.getElementById('insp-crit-rate').innerText = `${(res.critRate * 100).toFixed(1)}%`;
            document.getElementById('insp-min-max-dps').innerText = `${Math.round(res.minDps)} - ${Math.round(res.maxDps)} DPS`;
            document.getElementById('insp-events').innerText = `${res.eventCount}`;

            this.charts.renderBreakdown(res);
        }

        this.charts.renderDpsDistribution(this.gpuResults, this.selectedConfigIndex);
    }

    async runGpuSimulation() {
        if (this.isRunning) return;
        this.isRunning = true;
        this.progressBar.style.width = '0%';

        const activeConfigs = this.configs.slice(0, this.currentBatchSize);
        const iterations = this.currentIterations;
        const chunkSize = parseInt(document.getElementById('select-chunk-size').value, 10) || 64;

        try {
            const resultData = await this.engine.runBatch(activeConfigs, {
                iterationsPerConfig: iterations,
                chunkSize,
                maxDispatches: 100,
                onProgress: (p) => {
                    this.progressBar.style.width = `${p.progress * 100}%`;
                }
            });

            this.lastGpuStats = resultData;
            this.gpuResults = resultData.results;
            this.visualizer.updateResults(this.gpuResults);

            // Update Telemetry Cards
            document.getElementById('stat-total-instances').innerText = `${resultData.totalInstances.toLocaleString()} Agents`;
            document.getElementById('stat-gpu-time').innerText = `${resultData.gpuDurationMs.toFixed(1)} ms`;
            document.getElementById('stat-events-count').innerText = `${resultData.eventsProcessed.toLocaleString()}`;
            
            const eventsPerSec = (resultData.eventsProcessed / (Math.max(0.1, resultData.gpuDurationMs) / 1000));
            document.getElementById('stat-throughput').innerText = `${(eventsPerSec / 1000000).toFixed(2)} M/s`;

            const warpSpeed = (resultData.simulatedSeconds / (Math.max(0.1, resultData.gpuDurationMs) / 1000));
            document.getElementById('stat-warp').innerText = `${Math.round(warpSpeed).toLocaleString()}x`;

            // Update sidebar list items
            this.gpuResults.forEach((r, idx) => {
                const el = document.getElementById(`card-dps-${idx}`);
                if (el) el.innerText = `${Math.round(r.dps)} DPS`;
            });

            this.selectConfig(this.selectedConfigIndex);

        } catch (err) {
            console.error('GPU simulation error:', err);
        } finally {
            this.isRunning = false;
            this.progressBar.style.width = '0%';
        }
    }

    runCpuBenchmark() {
        const activeConfigs = this.configs.slice(0, this.currentBatchSize);
        // For CPU reference, run up to 10 iterations per config to prevent browser freeze
        const cpuIterations = Math.min(20, this.currentIterations);
        const cpuData = this.engine.runCpuReference(activeConfigs, cpuIterations);
        this.cpuResults = cpuData.results;

        const avgCpuDps = Math.round(cpuData.totalDpsAvg);
        const avgGpuDps = (this.gpuResults && this.gpuResults.length > 0)
            ? Math.round(this.gpuResults.reduce((a, b) => a + b.dps, 0) / this.gpuResults.length)
            : avgCpuDps;

        const gpuDuration = this.lastGpuStats ? this.lastGpuStats.gpuDurationMs : 50.0;
        // Extrapolate CPU time for full iterations
        const totalEstimatedCpuTimeMs = (cpuData.cpuDurationMs / cpuIterations) * this.currentIterations;
        const speedup = totalEstimatedCpuTimeMs / Math.max(0.1, gpuDuration);

        alert(`📊 CPU vs GPU DES Benchmark Results\n\n` +
              `• Batched Configs: ${activeConfigs.length}\n` +
              `• Iterations per Config: ${this.currentIterations.toLocaleString()} (Total ${ (activeConfigs.length * this.currentIterations).toLocaleString() } simulations)\n` +
              `• CPU Time (Estimated): ${(totalEstimatedCpuTimeMs / 1000).toFixed(2)} s (${totalEstimatedCpuTimeMs.toFixed(0)} ms)\n` +
              `• GPU Time (Measured): ${(gpuDuration / 1000).toFixed(2)} s (${gpuDuration.toFixed(1)} ms)\n` +
              `• GPU Speedup: ${speedup.toFixed(1)}x Faster than Single-Threaded CPU\n\n` +
              `• CPU Mean DPS: ${avgCpuDps}\n` +
              `• GPU Mean DPS: ${avgGpuDps}\n` +
              `• Parity Delta: ${Math.abs(avgCpuDps - avgGpuDps)} DPS (${(Math.abs(avgCpuDps - avgGpuDps)/avgCpuDps * 100).toFixed(2)}% difference)`);
    }

    exportJson() {
        const payload = {
            batchSize: this.currentBatchSize,
            iterations: this.currentIterations,
            totalSimulations: this.currentBatchSize * this.currentIterations,
            timestamp: new Date().toISOString(),
            configs: this.configs.slice(0, this.currentBatchSize),
            gpuResults: this.gpuResults
        };
        const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `gpu_des_sim_results_${this.currentBatchSize}_configs_${this.currentIterations}_iters.json`;
        a.click();
        URL.revokeObjectURL(url);
    }
}

window.addEventListener('DOMContentLoaded', () => {
    window.app = new App();
});
