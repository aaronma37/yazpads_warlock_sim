// Presentation only: never append these entries to executable APL bytecode.
const FILLERS = {
  fire: 'Incinerate',
  searing: 'Searing Pain',
  shadow: 'Shadow Bolt',
  bolt: 'Shadow Bolt',
};

export function fallbackName(rotation) {
  return FILLERS[rotation] || 'Unknown filler';
}

export function candidateFallbackRotation(candidate) {
  // The evaluated config is authoritative, including synthesis's inherited rotation.
  return candidate.batch?.config?.rotation ?? candidate.config?.rotation;
}

export function fallbackSummary(rotation) {
  return `Automatic fallback: ${fallbackName(rotation)} if affordable; otherwise Life Tap. Used only when no APL rule takes an action.`;
}

export function fallbackRow(rotation, columns = 4, isUnreachable = false) {
  const unreachableClass = isUnreachable ? ' unreachable-fallback' : '';
  const unreachableNote = isUnreachable
    ? ' <span style="color: #71717a; font-weight: bold;">(Unreachable — an earlier unconditional action always executes)</span>'
    : '';
  return `<tr class="apl-fallback-row${unreachableClass}"><td colspan="${columns}">
    <strong>Automatic fallback · ${fallbackName(rotation)}</strong>${unreachableNote}
    <div>When no rule takes an action: cast this filler if affordable; otherwise Life Tap.</div>
    <small>Built in · not editable · checked after the listed rules. “Always” rules can still fall through when unavailable.</small>
  </td></tr>`;
}
