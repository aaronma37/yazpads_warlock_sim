#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-web"
GENERATOR_ARGS=()
BUILD_COMMAND=(make -C "${BUILD_DIR}" -j"$(nproc)")

# Some managed Linux environments expose a make wrapper without GNU make.
# Use a separate Ninja build tree there so an existing Makefiles tree remains intact.
if [ ! -x /usr/bin/make ] && command -v ninja >/dev/null; then
    BUILD_DIR="${ROOT_DIR}/build-web-ninja"
    GENERATOR_ARGS=(-G Ninja)
    BUILD_COMMAND=(ninja -C "${BUILD_DIR}" -j"$(nproc)")
fi

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

if [ -n "${EMSDK:-}" ] && [ ! -w "${EMSDK}/upstream/emscripten/cache" ]; then
    export EM_CACHE="${EM_CACHE:-${TMPDIR:-/tmp}/emscripten-cache}"
fi

echo "=== 3. Configuring WebAssembly Target with CMake ==="
emcmake cmake "${GENERATOR_ARGS[@]}" -B "${BUILD_DIR}" -DPLATFORM=Web -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release

echo "=== 4. Compiling Desktop ImGui Application for Web ==="
emmake "${BUILD_COMMAND[@]}"

echo "=== Build Complete! ==="
echo "Output files created in ${ROOT_DIR}/docs/:"
ls -lh "${ROOT_DIR}/docs/index"*
echo "Browser WebGPU parity runner: ${ROOT_DIR}/docs/webgpu/pipeline.html"
