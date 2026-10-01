#include "webgpu_sim_runner.hpp"
#include "combat_types.hpp"
#include "native_gpu.hpp"
#include <chrono>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>

EM_JS(int, js_check_webgpu_support, (), {
    return (typeof navigator !== 'undefined' && !!navigator.gpu) ? 1 : 0;
});
#endif

namespace warlock {

BatchSimResult WebGPUSimRunner::run_batch(
    const WarlockSimulator& sim,
    int iterations,
    uint32_t step_us,
    uint32_t seed
) {
    auto results = run_batch_candidates({sim}, iterations, step_us, seed);
    return results.empty() ? BatchSimResult{} : std::move(results.front());
}

} // namespace warlock
