#pragma once
#include <string>
#include <functional>
#include <cstdint>
#include "warlock_sim.hpp"

namespace warlock {

struct SurrogateConfig {
    int num_samples = 2500;
    int iters_per_sample = 500;
    std::string output_path = "data/surrogate_dataset.csv";
    uint64_t base_seed = 1337ULL;
    int num_threads = 0; // 0 = hardware concurrency
};

class SurrogateDatasetGenerator {
public:
    static bool generate_dataset(
        const SurrogateConfig& config,
        std::function<void(int current, int total)> progress_cb = nullptr
    );
};

} // namespace warlock
