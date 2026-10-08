// Reuse the existing configuration markup, with independent controls and no
// copied event listeners. Runtime-dependent panels receive class overrides.
export function createSharedConfigurationPanel(source, { prefix, rename = {}, unsupported = [], remove = [] } = {}) {
  if (!source) throw new Error('Shared configuration panel is missing.');
  const panel = source.cloneNode(true);
  for (const selector of remove) panel.querySelectorAll(selector).forEach(node => node.remove());
  const ids = new Map();
  for (const node of [panel, ...panel.querySelectorAll('[id]')]) {
    if (node.id) { const id = `${prefix}-${node.id}`; ids.set(node.id, id); node.id = id; }
  }
  for (const label of panel.querySelectorAll('label[for]')) {
    const original = label.getAttribute('for');
    if (ids.has(original)) label.setAttribute('for', ids.get(original));
  }
  for (const control of panel.querySelectorAll('input, select, textarea')) {
    const originalName = control.name;
    if (control.tagName === 'SELECT') {
      const options = Array.from(control.options);
      control.value = (options.find(option => option.defaultSelected) || options[0])?.value || '';
    } else if (control.type === 'checkbox') control.checked = control.defaultChecked;
    else control.value = control.defaultValue;
    const override = rename[originalName];
    if (override) {
      const label = control.closest('.stat-field')?.querySelector('label');
      if (label) label.textContent = override.label;
    }
    const name = override?.name || originalName;
    control.name = name ? `${prefix}-${name}` : '';
    if (!control.id) control.id = `${prefix}-${name || control.type}`;
    const label = control.closest('.stat-field')?.querySelector('label');
    if (label && !label.querySelector('input')) label.setAttribute('for', control.id);
    if (unsupported.includes(originalName)) {
      control.disabled = true;
      control.title = 'Not available in the initial Priest scope.';
    }
  }
  for (const button of panel.querySelectorAll('button')) {
    button.type = 'button'; button.disabled = true;
    button.title = 'Available when Priest simulation is ready.';
  }
  return panel;
}
