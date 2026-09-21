#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-8080}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCS_DIR="$(cd "${SCRIPT_DIR}/../docs" && pwd)"

echo "=========================================================="
echo "  WoW Forever Simulator Local Web Server"
echo "=========================================================="
echo "Serving: ${DOCS_DIR}"
echo ""
echo "  🖥️ WebAssembly ImGui Simulator: http://localhost:${PORT}/index.html"
echo ""
echo "Press Ctrl+C to stop the server."
echo "=========================================================="

# Try to open in default browser if in graphical session
if [ -n "${DISPLAY:-}" ] || [ -n "${WAYLAND_DISPLAY:-}" ]; then
    (sleep 0.8 && xdg-open "http://localhost:${PORT}/index.html" >/dev/null 2>&1) &
fi

python3 -m http.server --directory "${DOCS_DIR}" "${PORT}"
