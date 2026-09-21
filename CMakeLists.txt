cmake_minimum_required(VERSION 3.20)
project(warlock_sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization flags
if (MSVC)
    add_compile_options(/O2 /permissive-)
elseif (EMSCRIPTEN)
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR NOT CMAKE_BUILD_TYPE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    endif()
else()
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR NOT CMAKE_BUILD_TYPE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sim
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sim/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sim/warlock
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sim/priest
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/warlock
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/priest
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/rlImGui
)

set(SIM_SOURCES
    src/sim/common/gear.cpp
    src/sim/warlock/warlock_sim.cpp
    src/sim/warlock/parallel_runner.cpp
    src/sim/warlock/optimizer.cpp
    src/sim/warlock/genetic_optimizer.cpp
    src/sim/warlock/surrogate_dataset_generator.cpp
    src/sim/priest/priest_sim.cpp
    src/sim/priest/parallel_runner.cpp
    src/sim/priest/optimizer.cpp
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

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/raylib)

add_executable(warlock_sim
    src/main.cpp
    ${SIM_SOURCES}
    ${UI_SOURCES}
)

target_link_libraries(warlock_sim PRIVATE
    raylib
)

if (EMSCRIPTEN)
    set_target_properties(warlock_sim PROPERTIES
        OUTPUT_NAME "index"
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs"
    )
    target_link_options(warlock_sim PRIVATE
        -sUSE_GLFW=3
        -sWASM=1
        -sINITIAL_MEMORY=64MB
        -sSTACK_SIZE=16MB
        -sALLOW_MEMORY_GROWTH=1
        -sMAX_WEBGL_VERSION=2
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets_web@/assets
        --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/web_shell.html
    )
else()
    set_target_properties(warlock_sim PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    )

    # Test Suite
    enable_testing()

    set(TEST_SOURCES
        tests/main_tests.cpp
        tests/test_races.cpp
        tests/test_icons.cpp
        tests/test_cover_uv.cpp
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
        tests/test_genetic_optimizer.cpp
        tests/test_mana_regen.cpp
        tests/test_priest_sim.cpp
        tests/test_surrogate_dataset.cpp
    )

    add_executable(sim_tests
        ${TEST_SOURCES}
        ${SIM_SOURCES}
    )

    if (WIN32)
        target_link_libraries(sim_tests PRIVATE)
    else()
        target_link_libraries(sim_tests PRIVATE
            pthread
            m
        )
    endif()

    set_target_properties(sim_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    )

    add_test(NAME AllUnitTests COMMAND sim_tests)
endif()

