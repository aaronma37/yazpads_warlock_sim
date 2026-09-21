#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=========================================================="
echo "  1/2: Building Native Desktop App (Linux / OpenGL 3.3)"
echo "=========================================================="
cmake -B "${ROOT_DIR}/build" -DCMAKE_BUILD_TYPE=Release
make -C "${ROOT_DIR}/build" -j$(nproc)
echo "Native build ready at: ${ROOT_DIR}/bin/warlock_sim"

echo ""
echo "=========================================================="
echo "  2/2: Building WebAssembly ImGui App (Web / WebGL 2.0)"
echo "=========================================================="
"${ROOT_DIR}/scripts/build_desktop_wasm.sh"

echo ""
echo "=========================================================="
echo "  All Builds Finished Successfully!"
echo "  - Native binary: bin/warlock_sim"
echo "  - Web application: docs/index.html"
echo "=========================================================="
