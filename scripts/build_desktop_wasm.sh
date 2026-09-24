#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=== 1. Bundling & Filtering Web Assets ==="
python3 "${ROOT_DIR}/scripts/bundle_web_assets.py"

echo "=== 2. Activating Emscripten SDK ==="
if [ -f "${HOME}/emsdk/emsdk_env.sh" ]; then
    source "${HOME}/emsdk/emsdk_env.sh"
elif [ -f "${HOME}/.emsdk/emsdk_env.sh" ]; then
    source "${HOME}/.emsdk/emsdk_env.sh"
elif [ -f "/home/deck/.local/opt/emsdk/emsdk_env.sh" ]; then
    source /home/deck/.local/opt/emsdk/emsdk_env.sh
elif [ -f "${EMSDK:-}/emsdk_env.sh" ]; then
    source "${EMSDK}/emsdk_env.sh"
else
    echo "Warning: emsdk_env.sh not found at standard path, assuming emcmake/emmake are in PATH."
fi

echo "=== 3. Configuring WebAssembly Target with CMake ==="
emcmake cmake -B "${ROOT_DIR}/build-web" -DPLATFORM=Web -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release

echo "=== 4. Compiling Desktop ImGui Application for Web ==="
emmake make -C "${ROOT_DIR}/build-web" -j$(nproc)

echo "=== Build Complete! ==="
echo "Output files created in ${ROOT_DIR}/docs/:"
ls -lh "${ROOT_DIR}/docs/index"*
