cmake_minimum_required(VERSION 3.20)
project(warlock_sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization flags
if (CMAKE_BUILD_TYPE STREQUAL "Release" OR NOT CMAKE_BUILD_TYPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/rlImGui
)

set(SIM_SOURCES
    src/sim/gear.cpp
    src/sim/warlock_sim.cpp
    src/sim/parallel_runner.cpp
    src/sim/optimizer.cpp
)

set(UI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/implot/implot.cpp
    third_party/implot/implot_items.cpp
    third_party/rlImGui/rlImGui.cpp
)

add_executable(warlock_sim
    src/main.cpp
    ${SIM_SOURCES}
    ${UI_SOURCES}
)

target_link_libraries(warlock_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/libraylib.a
    GL
    m
    pthread
    dl
    rt
    X11
)

set_target_properties(warlock_sim PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# Test Suite
enable_testing()

set(TEST_SOURCES
    tests/main_tests.cpp
    tests/test_races.cpp
    tests/test_icons.cpp
    tests/test_spell_stats.cpp
    tests/test_talents.cpp
    tests/test_gear.cpp
    tests/test_buffs.cpp
    tests/test_rotations.cpp
    tests/test_mechanics.cpp
    tests/test_spec_presets.cpp
    tests/test_imp_analysis.cpp
    tests/test_isb_analysis.cpp
    tests/test_build_export.cpp
)

add_executable(sim_tests
    ${TEST_SOURCES}
    ${SIM_SOURCES}
)

target_link_libraries(sim_tests PRIVATE
    pthread
    m
)

set_target_properties(sim_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

add_test(NAME AllUnitTests COMMAND sim_tests)

