// WoW Classic Authentic Floating Tooltip Engine
let tooltipEl = null;

export function initTooltips() {
  if (document.getElementById('wow-tooltip')) {
    tooltipEl = document.getElementById('wow-tooltip');
    return;
  }

  tooltipEl = document.createElement('div');
  tooltipEl.id = 'wow-tooltip';
  tooltipEl.className = 'wow-tooltip';
  tooltipEl.style.display = 'none';
  document.body.appendChild(tooltipEl);

  // Global delegate for [data-wow-tooltip]
  document.addEventListener('mouseover', (e) => {
    const target = e.target.closest('[data-wow-tooltip]');
    if (target) {
      const text = target.getAttribute('data-wow-tooltip');
      const title = target.getAttribute('data-wow-tooltip-title') || '';
      const subtitle = target.getAttribute('data-wow-tooltip-subtitle') || '';
      const footer = target.getAttribute('data-wow-tooltip-footer') || '';
      showTooltip(e, { title, subtitle, desc: text, footer });
    }
  });

  document.addEventListener('mousemove', (e) => {
    if (tooltipEl && tooltipEl.style.display !== 'none') {
      positionTooltip(e);
    }
  });

  document.addEventListener('mouseout', (e) => {
    const target = e.target.closest('[data-wow-tooltip]');
    if (target) {
      hideTooltip();
    }
  });
}

export function showTooltip(e, { title = '', subtitle = '', desc = '', nextRankDesc = '', footer = '', icon = '' }) {
  if (!tooltipEl) initTooltips();
  if (!tooltipEl) return;

  let html = '';
  if (title) {
    html += `<div class="wow-tooltip-title">${title}</div>`;
  }
  if (subtitle) {
    html += `<div class="wow-tooltip-subtitle">${subtitle}</div>`;
  }
  if (desc) {
    html += `<div class="wow-tooltip-desc">${desc.replace(/\n/g, '<br>')}</div>`;
  }
  if (nextRankDesc) {
    html += `<div class="wow-tooltip-next-header">Next Rank:</div><div class="wow-tooltip-next-desc">${nextRankDesc.replace(/\n/g, '<br>')}</div>`;
  }
  if (footer) {
    html += `<div class="wow-tooltip-footer">${footer}</div>`;
  }

  tooltipEl.innerHTML = html;
  tooltipEl.style.display = 'block';
  positionTooltip(e);
}

export function positionTooltip(e) {
  if (!tooltipEl) return;
  const padding = 14;
  let x = e.clientX + padding;
  let y = e.clientY + padding;

  const rect = tooltipEl.getBoundingClientRect();
  const winWidth = window.innerWidth;
  const winHeight = window.innerHeight;

  if (x + rect.width > winWidth - 10) {
    x = e.clientX - rect.width - padding;
    if (x < 10) x = 10;
  }

  if (y + rect.height > winHeight - 10) {
    y = e.clientY - rect.height - padding;
    if (y < 10) y = 10;
  }

  tooltipEl.style.left = `${Math.max(0, x)}px`;
  tooltipEl.style.top = `${Math.max(0, y)}px`;
}

export function hideTooltip() {
  if (tooltipEl) {
    tooltipEl.style.display = 'none';
  }
}
