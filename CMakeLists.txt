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
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sim/webgpu
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/warlock
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/priest
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/rlImGui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/rl-tools/include
)

set(SIM_SOURCES
    src/sim/common/gear.cpp
    src/sim/warlock/warlock_sim.cpp
    src/sim/warlock/parallel_runner.cpp
    src/sim/warlock/optimizer.cpp
    src/sim/warlock/genetic_optimizer.cpp
    src/sim/warlock/surrogate_dataset_generator.cpp
    src/sim/warlock/ppo_training.cpp
    src/sim/warlock/imitation_training.cpp
    src/sim/priest/priest_sim.cpp
    src/sim/priest/parallel_runner.cpp
    src/sim/priest/optimizer.cpp
    src/sim/webgpu/reference.cpp
    src/sim/webgpu/native_gpu.cpp
    src/sim/webgpu/webgpu_sim_runner.cpp
    src/sim/webgpu/parity_comparator.cpp
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
        -O3
        -pthread
        -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency
        -sPTHREAD_POOL_SIZE_STRICT=0
        -sUSE_GLFW=3
        -sWASM=1
        -sINITIAL_MEMORY=512MB
        -sSTACK_SIZE=2MB
        -sALLOW_MEMORY_GROWTH=1
        -sMAX_WEBGL_VERSION=2
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets_web@/assets
        --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/web_shell.html
    )
    target_compile_options(warlock_sim PRIVATE -pthread)

    # Browser build of the actual C++ parity pipeline. Its WebGPU calls are
    # bridged to navigator.gpu in native_gpu.cpp and require Asyncify while
    # the browser completes GPU submissions and result readback.
    add_executable(webgpu_parity_pipeline_web
        src/sim/webgpu/parity_pipeline_main.cpp
        ${SIM_SOURCES}
    )
    set_target_properties(webgpu_parity_pipeline_web PROPERTIES
        OUTPUT_NAME "pipeline"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/webgpu"
    )
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sim/webgpu/pipeline.html
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/webgpu/pipeline.html
        COPYONLY
    )
    target_link_options(webgpu_parity_pipeline_web PRIVATE
        -O3
        -sASYNCIFY
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=512MB
        -sFILESYSTEM=1
        -sINVOKE_RUN=0
        -sEXIT_RUNTIME=1
        -sEXPORTED_RUNTIME_METHODS=ccall,FS
    )
else()
    set_target_properties(warlock_sim PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    )

    # Standalone WebGPU vs CPU Parity Pipeline Executable
    add_executable(webgpu_parity_pipeline
        src/sim/webgpu/parity_pipeline_main.cpp
        ${SIM_SOURCES}
    )
    if(UNIX)
        target_link_libraries(webgpu_parity_pipeline PRIVATE pthread m)
    endif()
    set_target_properties(webgpu_parity_pipeline PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    )

    # Test Suite
    enable_testing()

    set(TEST_SOURCES
        tests/main_tests.cpp
        tests/test_races.cpp
        tests/test_icons.cpp
        tests/test_cover_uv.cpp
        tests/test_frame_geometry.cpp
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
        tests/test_viper_pipeline.cpp
        tests/test_apl_analyzer.cpp
        tests/test_gbdt_policy.cpp
        tests/test_ppo_training.cpp
        tests/test_imitation_training.cpp
        tests/test_webgpu_runner.cpp
        tests/test_webgpu_parity.cpp
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

# Native WebGPU backend for the desktop app and parity pipeline. The bundled
# runtime is wgpu-native; other targets retain the C++ reference fallback.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(WGPU_NATIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/wgpu-native/linux-x86_64")
    if(EXISTS "${WGPU_NATIVE_DIR}/libwgpu_native.so")
        add_library(wgpu_native SHARED IMPORTED GLOBAL)
        set_target_properties(wgpu_native PROPERTIES IMPORTED_LOCATION "${WGPU_NATIVE_DIR}/libwgpu_native.so")
        foreach(target warlock_sim webgpu_parity_pipeline sim_tests webgpu_oracle)
            if(TARGET ${target})
                target_include_directories(${target} PRIVATE "${WGPU_NATIVE_DIR}/include")
                target_compile_definitions(${target} PRIVATE WARLOCK_HAS_NATIVE_WEBGPU=1
                    WARLOCK_WEBGPU_SHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/src/sim/webgpu/combat.wgsl")
                target_link_libraries(${target} PRIVATE wgpu_native)
                set_property(TARGET ${target} APPEND PROPERTY BUILD_RPATH "${WGPU_NATIVE_DIR}")
            endif()
        endforeach()
    endif()
endif()

# Opt-in independent CPU fixtures for the WebGPU scheduler experiment.
option(BUILD_WEBGPU_ORACLE "Build legacy CPU fixture generator for WebGPU" OFF)
if(BUILD_WEBGPU_ORACLE AND NOT EMSCRIPTEN)
    add_executable(webgpu_oracle tests/webgpu/oracle.cpp ${SIM_SOURCES})
    if(UNIX)
        target_link_libraries(webgpu_oracle PRIVATE pthread m)
    endif()
    set_target_properties(webgpu_oracle PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif()
