import { UPTIME_EFFECTS } from './model.js';

// Uses counters already accumulated by the detailed summary's single pass.
export function summarizeDetails(totals, count, duration, sortedDps) {
  const average = key => (totals[key] || 0) / count;
  const min = sortedDps[0], max = sortedDps.at(-1);
  const binCount = max > min ? Math.min(32, Math.max(1, Math.ceil(Math.sqrt(count)))) : 1;
  const width = max > min ? (max - min) / binCount : 1;
  const bins = Array.from({length: binCount}, (_, i) => ({min: min + i * width, max: max > min ? min + (i + 1) * width : min, count: 0}));
  for (const dps of sortedDps) bins[Math.min(binCount - 1, Math.floor((dps - min) / width))].count++;
  return {
    uptimes: UPTIME_EFFECTS.map(([name, key]) => ({name, fraction: average(`uptime_${key}`)})),
    histogram: bins,
    manaSpent: average('spent'), manaWasted: average('manaWasted'),
    tapsPerMinute: average('taps') * 60 / duration,
    nightfallPerMinute: average('procs') * 60 / duration,
    isbPerMinute: average('isbProcs') * 60 / duration,
    isbApplications: average('isbConsumed'),
  };
}

export function renderDetailedResults(container, summary, duration) {
  if (!container) return;
  const details = summary.details;
  container.hidden = !details;
  if (!details) { container.replaceChildren(); return; }
  const fmt = (n, digits=1) => Number(n).toLocaleString(undefined, {maximumFractionDigits: digits});
  const peak = Math.max(...details.histogram.map(bin => bin.count), 1);
  const bars = details.histogram.map((bin, i) => {
    const height = bin.count / peak * 95;
    return `<rect x="${i * 600 / details.histogram.length}" y="${110-height}" width="${Math.max(1,600/details.histogram.length-2)}" height="${height}" fill="#a78bfa"><title>${fmt(bin.min)}–${fmt(bin.max)} DPS: ${bin.count} fights</title></rect>`;
  }).join('');
  container.innerHTML = `
    <h4>DPS distribution</h4>
    <svg viewBox="0 0 600 140" role="img" aria-label="DPS histogram" style="width:100%;max-height:220px">${bars}<text x="0" y="135" fill="currentColor" font-size="12">${fmt(summary.dps[0])} DPS</text><text x="600" y="135" text-anchor="end" fill="currentColor" font-size="12">${fmt(summary.dps.at(-1))} DPS</text></svg>
    <p>Median ${fmt(summary.p50)} DPS · Standard deviation ${fmt(summary.sd)} · 5th–95th percentile ${fmt(summary.p05)}–${fmt(summary.p95)} DPS</p>
    <h4>DoT and buff uptimes</h4>
    <div class="damage-table-wrapper"><table class="damage-table detail-uptimes"><thead><tr><th>Effect</th><th>Uptime</th><th>Active time</th><th>Inactive time</th></tr></thead><tbody>${details.uptimes.map(effect => `<tr><td>${effect.name}</td><td>${fmt(effect.fraction*100)}%</td><td>${fmt(effect.fraction*duration)}s</td><td>${fmt((1-effect.fraction)*duration)}s</td></tr>`).join('')}</tbody></table></div>
    <h4>Spell statistics</h4>
    <div class="damage-table-wrapper"><table class="damage-table"><thead><tr><th>Spell</th><th>Damage share</th><th>Casts/min</th><th>Misses/fight</th><th>Crits / damage events</th><th>Damage/event</th></tr></thead><tbody>${summary.spells.filter(spell=>spell.casts>0||spell.damage>0).map(spell=>`<tr><td>${spell.name}</td><td>${fmt(spell.damage/summary.damage*100)}%</td><td>${fmt(spell.casts*60/duration)}</td><td>${fmt(spell.misses)}</td><td>${fmt(spell.hits?spell.crits/spell.hits*100:0)}%</td><td>${spell.hits?fmt(spell.damage/spell.hits):'—'}</td></tr>`).join('')}</tbody></table></div>
    `;
}
