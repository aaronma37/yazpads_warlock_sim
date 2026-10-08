#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=========================================================="
echo "  Building Native Desktop App (Linux / OpenGL 3.3)"
echo "=========================================================="
cmake -B "${ROOT_DIR}/build" -DCMAKE_BUILD_TYPE=Release
make -C "${ROOT_DIR}/build" -j$(nproc)
echo "Native build ready at: ${ROOT_DIR}/bin/warlock_sim"


echo "Browser app: threejs_webgl_des (no compilation required)."
echo "Run scripts/serve_web.sh to serve it locally."
