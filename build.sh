#!/bin/bash
set -e

mkdir -p bin build build/third_party

CXX="g++"
FLAGS="-std=c++20 -O3 -march=native"
INCLUDES="-I. -Ithird_party/raylib/include -Ithird_party/imgui -Ithird_party/implot -Ithird_party/rlImGui"

echo "=== Checking third-party dependencies ==="
THIRD_PARTY_SRCS=(
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/implot/implot.cpp
    third_party/implot/implot_items.cpp
    third_party/rlImGui/rlImGui.cpp
)

for src in "${THIRD_PARTY_SRCS[@]}"; do
    obj="build/third_party/$(basename ${src%.cpp}.o)"
    if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ]; then
        echo "  Compiling $src..."
        $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" &
    fi
done
wait

echo "=== Compiling simulator sources in parallel ==="
SIM_SRCS=(
    src/sim/gear.cpp
    src/sim/warlock_sim.cpp
    src/sim/parallel_runner.cpp
    src/sim/optimizer.cpp
    src/main.cpp
)

for src in "${SIM_SRCS[@]}"; do
    obj="build/$(basename ${src%.cpp}.o)"
    echo "  Compiling $src..."
    $CXX $FLAGS $INCLUDES -c "$src" -o "$obj" &
done
wait

echo "=== Linking bin/warlock_sim ==="
$CXX $FLAGS build/third_party/*.o build/*.o \
    third_party/raylib/lib/libraylib.a \
    -lm -lpthread -ldl \
    -o bin/warlock_sim

echo "=== Build Succeeded! Output binary: bin/warlock_sim ==="
