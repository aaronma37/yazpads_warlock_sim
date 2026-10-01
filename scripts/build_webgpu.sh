#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if ! command -v em++ >/dev/null; then
  if [[ -f /home/deck/.local/opt/emsdk/emsdk_env.sh ]]; then
    source /home/deck/.local/opt/emsdk/emsdk_env.sh
  elif [[ -f "${EMSDK:-}/emsdk_env.sh" ]]; then
    source "${EMSDK}/emsdk_env.sh"
  else
    echo 'Activate Emscripten (em++) before building.' >&2; exit 1
  fi
fi
OUT="${ROOT}/docs/webgpu"
mkdir -p "$OUT"
cp "${ROOT}"/src/sim/webgpu/{combat.wgsl,runner.mjs,benchmark.mjs,async.mjs,worker.mjs,index.html} "$OUT/"
em++ "${ROOT}/src/sim/webgpu/reference.cpp" -O3 -ffp-contract=off \
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,worker,node -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS='["_simulate_batch","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["HEAPF32","HEAPU8"]' \
  -o "${OUT}/reference.mjs"
echo "Built browser experiment: ${OUT}/index.html"
