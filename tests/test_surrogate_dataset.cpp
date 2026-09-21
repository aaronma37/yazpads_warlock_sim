#include "test_framework.hpp"
#include "src/sim/warlock/surrogate_dataset_generator.hpp"
#include <fstream>
#include <filesystem>

using namespace warlock;

TEST_CASE(SurrogateDataset, GenerationSmokeTest) {
    SurrogateConfig cfg;
    cfg.num_samples = 10;
    cfg.iters_per_sample = 20;
    cfg.output_path = "build/test_surrogate_dataset.csv";
    cfg.num_threads = 2;

    bool ok = SurrogateDatasetGenerator::generate_dataset(cfg);
    CHECK(ok);
    CHECK(std::filesystem::exists(cfg.output_path));

    std::ifstream in(cfg.output_path);
    CHECK(in.is_open());

    std::string header;
    std::getline(in, header);
    CHECK(!header.empty());
    CHECK(header.find("build_desc") != std::string::npos);
    CHECK(header.find("dps") != std::string::npos);

    int row_count = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) row_count++;
    }
    CHECK_EQ(row_count, 10);

    // Clean up
    std::filesystem::remove(cfg.output_path);
}
