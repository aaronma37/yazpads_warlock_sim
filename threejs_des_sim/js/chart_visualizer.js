/**
 * High Performance Canvas Chart Visualizer
 * Renders DPS distributions, damage breakdowns, and GPU vs CPU benchmark metrics.
 */

export class ChartVisualizer {
    constructor(dpsCanvasId, breakdownCanvasId) {
        this.dpsCanvas = document.getElementById(dpsCanvasId);
        this.breakdownCanvas = document.getElementById(breakdownCanvasId);
        this.dpsCtx = this.dpsCanvas?.getContext('2d');
        this.breakdownCtx = this.breakdownCanvas?.getContext('2d');
    }

    /**
     * Render DPS Distribution and Ranking Bar Chart
     */
    renderDpsDistribution(results, selectedIndex = 0) {
        if (!this.dpsCtx || !results || results.length === 0) return;
        
        const canvas = this.dpsCanvas;
        const ctx = this.dpsCtx;
        const w = canvas.width = canvas.parentElement.clientWidth;
        const h = canvas.height = 200;

        ctx.clearRect(0, 0, w, h);

        const padding = { top: 25, right: 20, bottom: 30, left: 50 };
        const chartW = w - padding.left - padding.right;
        const chartH = h - padding.top - padding.bottom;

        // Find max DPS
        let maxDps = 1;
        for (const r of results) {
            if (r.dps > maxDps) maxDps = r.dps;
        }
        maxDps = Math.ceil(maxDps / 200) * 200;

        // Draw background grid
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
        ctx.lineWidth = 1;
        ctx.fillStyle = 'rgba(255, 255, 255, 0.4)';
        ctx.font = '10px Inter, system-ui, sans-serif';

        const gridLines = 4;
        for (let i = 0; i <= gridLines; i++) {
            const yVal = (maxDps / gridLines) * i;
            const yPos = padding.top + chartH - (i / gridLines) * chartH;
            ctx.beginPath();
            ctx.moveTo(padding.left, yPos);
            ctx.lineTo(w - padding.right, yPos);
            ctx.stroke();

            ctx.fillText(`${Math.round(yVal)}`, 10, yPos + 3);
        }

        // Draw Bars for 100 configs
        const barW = Math.max(2, (chartW / results.length) - 1.5);

        for (let i = 0; i < results.length; i++) {
            const r = results[i];
            const barH = (r.dps / maxDps) * chartH;
            const x = padding.left + (i / results.length) * chartW;
            const y = padding.top + chartH - barH;

            // Bar color
            if (i === selectedIndex) {
                ctx.fillStyle = '#f6e05e'; // Bright Gold for selected
            } else if (r.spec.includes('Affliction')) {
                ctx.fillStyle = '#9f7aea';
            } else if (r.spec.includes('Destruction') || r.spec.includes('Fire')) {
                ctx.fillStyle = '#ed8936';
            } else if (r.spec.includes('Ruin')) {
                ctx.fillStyle = '#667eea';
            } else if (r.spec.includes('Demo')) {
                ctx.fillStyle = '#48bb78';
            } else {
                ctx.fillStyle = '#38b2ac';
            }

            ctx.fillRect(x, y, barW, barH);
        }

        // Title
        ctx.fillStyle = '#e2e8f0';
        ctx.font = 'bold 11px Inter, system-ui, sans-serif';
        ctx.fillText(`DPS Distribution (${results.length} Batched Configurations)`, padding.left, 15);
    }

    /**
     * Render Spell Breakdown Horizontal Bar / Donut
     */
    renderBreakdown(result) {
        if (!this.breakdownCtx || !result) return;
        
        const canvas = this.breakdownCanvas;
        const ctx = this.breakdownCtx;
        const w = canvas.width = canvas.parentElement.clientWidth;
        const h = canvas.height = 180;

        ctx.clearRect(0, 0, w, h);

        const breakdown = result.damageBreakdown || {};
        const entries = Object.entries(breakdown)
            .filter(([_, val]) => val > 0)
            .sort((a, b) => b[1] - a[1]);

        const totalDmg = result.totalDamage || 1;
        const colors = [
            '#9f7aea', '#ed8936', '#48bb78', '#38b2ac', '#e53e3e',
            '#ecc94b', '#667eea', '#ed64a6', '#4fd1c5', '#9ae6b4'
        ];

        let startY = 25;
        const barH = 14;

        ctx.font = 'bold 11px Inter, system-ui, sans-serif';
        ctx.fillStyle = '#e2e8f0';
        ctx.fillText(`Damage Breakdown: ${result.name}`, 10, 15);

        for (let i = 0; i < entries.length && i < 6; i++) {
            const [name, val] = entries[i];
            const pct = (val / totalDmg) * 100;
            const barW = ((w - 180) * (val / totalDmg));

            ctx.fillStyle = 'rgba(255, 255, 255, 0.7)';
            ctx.font = '10px Inter, sans-serif';
            ctx.fillText(`${name}`, 10, startY + 10);

            // Bar background
            ctx.fillStyle = 'rgba(255, 255, 255, 0.08)';
            ctx.fillRect(100, startY, w - 170, barH);

            // Active bar
            ctx.fillStyle = colors[i % colors.length];
            ctx.fillRect(100, startY, barW, barH);

            // Pct text
            ctx.fillStyle = '#ffffff';
            ctx.fillText(`${pct.toFixed(1)}% (${Math.round(val)})`, w - 65, startY + 11);

            startY += 24;
        }
    }
}
