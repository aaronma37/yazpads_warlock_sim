#include "webgpu_sim_runner.hpp"
#include "combat_types.hpp"
#include "native_gpu.hpp"
#include <chrono>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>

extern "C" {
EM_JS(int, js_webgpu_is_available, (), {
    return (typeof window !== 'undefined' && window.WarlockWebGPU && window.WarlockWebGPU.isSupported) ? 1 : 0;
});

EM_JS(void, js_webgpu_start_batch, (const float* configs, uint32_t candidate_count, uint32_t replicas, uint32_t seed, uint32_t step_us, void* states_out), {
    if (typeof window !== 'undefined' && window.WarlockWebGPU) {
        window.WarlockWebGPU.run(configs, candidate_count, replicas, seed, step_us, states_out);
    }
});

EM_JS(int, js_webgpu_check_status, (), {
    if (typeof window === 'undefined' || !window.WarlockWebGPU) return -1;
    if (window.WarlockWebGPU.isRunning) return 1;
    if (window.WarlockWebGPU.hasFinished) return 2;
    if (window.WarlockWebGPU.lastError) return -1;
    return 0;
});

EM_JS(double, js_webgpu_get_elapsed_seconds, (), {
    if (typeof window === 'undefined' || !window.WarlockWebGPU) return 0.0;
    return window.WarlockWebGPU.elapsedSeconds || 0.0;
});
}
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
