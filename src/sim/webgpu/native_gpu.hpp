#pragma once

#include "combat_types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace warlock {
bool run_native_webgpu(const std::vector<Config>& configs, uint32_t replicas, uint32_t seed, uint32_t step_us,
                      std::vector<State>& states, double& elapsed_seconds,
                      std::string& error);
}
