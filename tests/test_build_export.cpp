#include "test_framework.hpp"
#include "src/sim/build_export.hpp"

using namespace warlock;

static int count_char(const std::string& s, char c) {
    int n = 0;
    for (char ch : s) if (ch == c) ++n;
    return n;
}

TEST_CASE(BuildExport, ContainsCoreSections) {
    WarlockSimulator sim;
    sim.use_raw_stats = true;
    std::string json_raw = build_export::export_build_json(sim);
    CHECK(json_raw.find("\"format\": \"warlock-build/1\"") != std::string::npos);
    CHECK(json_raw.find("\"class\": \"Warlock\"") != std::string::npos);
    CHECK(json_raw.find("\"race\": \"Human\"") != std::string::npos);
    CHECK(json_raw.find("\"fight_duration\": 180") != std::string::npos);
    CHECK(json_raw.find("\"stats_mode\": \"raw\"") != std::string::npos);
    CHECK(json_raw.find("\"raw_stats\"") != std::string::npos);
    CHECK(json_raw.find("\"gear\"") == std::string::npos);
    CHECK(json_raw.find("\"talents\"") != std::string::npos);
    CHECK(json_raw.find("\"buffs\"") != std::string::npos);
    CHECK(json_raw.find("\"policy\"") != std::string::npos);
    CHECK(json_raw.find("\"mechanics\"") != std::string::npos);
    CHECK(json_raw.find("\"target\"") != std::string::npos);
    // Braces balance (no braces appear inside string values here).
    CHECK_EQ(count_char(json_raw, '{'), count_char(json_raw, '}'));
    CHECK_EQ(count_char(json_raw, '['), count_char(json_raw, ']'));

    // Test gear mode
    sim.use_raw_stats = false;
    std::string json_gear = build_export::export_build_json(sim);
    CHECK(json_gear.find("\"stats_mode\": \"gear\"") != std::string::npos);
    CHECK(json_gear.find("\"gear\"") != std::string::npos);
    CHECK(json_gear.find("\"Head\"") != std::string::npos);
    CHECK(json_gear.find("\"raw_stats\"") == std::string::npos);
    CHECK_EQ(count_char(json_gear, '{'), count_char(json_gear, '}'));
}

TEST_CASE(BuildExport, ReflectsModifiedState) {
    WarlockSimulator sim;
    sim.talents.demo.unholy_power = 5;
    sim.talents.destro.ruin = 5;
    sim.buffs.flask_of_supreme_power = false;
    sim.policy.pet = PetChoice::IMP;
    sim.fight_duration = 180.0;

    std::string json = build_export::export_build_json(sim);
    CHECK(json.find("\"unholy_power\": 5") != std::string::npos);
    CHECK(json.find("\"ruin\": 5") != std::string::npos);
    CHECK(json.find("\"flask_of_supreme_power\": false") != std::string::npos);
    CHECK(json.find("\"pet\": \"Imp (Firebolt)\"") != std::string::npos);
    CHECK(json.find("\"fight_duration\": 180") != std::string::npos);
    // Zero-point talents are omitted (missing key = 0).
    CHECK(json.find("\"drain_hope\"") == std::string::npos);
}

TEST_CASE(BuildExport, EscapesStrings) {
    WarlockSimulator sim;
    sim.use_raw_stats = false;
    sim.gear.name = "Test \"Build\" \\ mod";
    std::string json = build_export::export_build_json(sim);
    CHECK(json.find("Test \\\"Build\\\" \\\\ mod") != std::string::npos);
    CHECK_EQ(count_char(json, '{'), count_char(json, '}'));
}

TEST_CASE(BuildExport, ExportsSimulationResults) {
    WarlockSimulator sim;
    BatchSimResult res;
    res.total_iterations = 2500;
    res.total_sim_time_seconds = 0.45;
    res.iterations_per_second = 5555.5;
    res.mean_dps = 525.4;
    res.min_dps = 410.0;
    res.max_dps = 670.0;
    res.std_dev_dps = 28.1;
    res.p50_dps = 524.8;
    res.crit_percent = 19.5;
    res.miss_percent = 1.0;
    res.pct_shadow_bolt = 72.5;
    res.pct_corruption = 18.0;

    std::string json = build_export::export_build_json(sim, &res, 2500, 4);
    CHECK(json.find("\"simulation_results\"") != std::string::npos);
    CHECK(json.find("\"iterations\": 2500") != std::string::npos);
    CHECK(json.find("\"mean_dps\": 525.4") != std::string::npos);
    CHECK(json.find("\"median_dps\": 524.8") != std::string::npos);
    CHECK(json.find("\"crit_percent\": 19.5") != std::string::npos);
    CHECK(json.find("\"pct_shadow_bolt\": 72.5") != std::string::npos);
    CHECK(json.find("\"pct_corruption\": 18") != std::string::npos);
    CHECK(json.find("\"sim_config\"") != std::string::npos);
    CHECK_EQ(count_char(json, '{'), count_char(json, '}'));
}

TEST_CASE(BuildExport, ExportsSpecsBatchJsonAndCsv) {
    WarlockSimulator sim;
    std::vector<CandidateResult> results;

    CandidateResult c1;
    c1.rank = 1;
    c1.name = "5/11/35 DS/AF DS-Imp";
    c1.race = Race::HUMAN;
    c1.mean_dps = 540.2;
    c1.min_dps = 420.0;
    c1.max_dps = 680.0;
    c1.std_dev_dps = 27.5;
    c1.isb_uptime = 0.65;
    c1.stat_weights.valid = true;
    c1.stat_weights.dps_per_sp = 0.85;
    c1.stat_weights.dps_per_hit = 12.4;
    c1.stat_weights.dps_per_crit = 9.8;
    results.push_back(c1);

    std::string json = build_export::export_specs_batch_json(results, &sim);
    CHECK(json.find("\"format\": \"warlock-specs-batch/1\"") != std::string::npos);
    CHECK(json.find("\"total_specs\": 1") != std::string::npos);
    CHECK(json.find("\"name\": \"5/11/35 DS/AF DS-Imp\"") != std::string::npos);
    CHECK(json.find("\"dps_per_sp\": 0.85") != std::string::npos);

    std::string csv = build_export::export_specs_batch_csv(results);
    CHECK(csv.find("Rank,Spec Name,Race,Mean DPS") != std::string::npos);
    CHECK(csv.find("1,\"5/11/35 DS/AF DS-Imp\",Human,540.2") != std::string::npos);
    CHECK(csv.find("0.85,12.4,9.8") != std::string::npos);
}

TEST_CASE(BuildExport, GeneratesValidZipArchive) {
    WarlockSimulator sim;
    std::vector<CandidateResult> results;
    CandidateResult c1;
    c1.rank = 1;
    c1.name = "DS/AF Test";
    c1.mean_dps = 500.0;
    results.push_back(c1);

    sim::ZipArchive zip = build_export::create_specs_batch_zip(results, &sim);
    std::vector<uint8_t> zip_data = zip.build_zip();

    CHECK(zip_data.size() > 100);
    // Check ZIP local file header signature 0x04034b50 (PK\x03\x04)
    CHECK_EQ(zip_data[0], 0x50);
    CHECK_EQ(zip_data[1], 0x4B);
    CHECK_EQ(zip_data[2], 0x03);
    CHECK_EQ(zip_data[3], 0x04);
}

TEST_CASE(BuildExport, PriestExportBuildAndBatch) {
    priest::PriestSimulator sim;
    std::string json = priest::build_export::export_build_json(sim);
    CHECK(json.find("\"format\": \"priest-build/1\"") != std::string::npos);
    CHECK(json.find("\"class\": \"Priest\"") != std::string::npos);
    CHECK(json.find("\"discipline\"") != std::string::npos);
    CHECK(json.find("\"holy\"") != std::string::npos);
    CHECK(json.find("\"shadow\"") != std::string::npos);

    std::vector<priest::CandidateResult> results;
    priest::CandidateResult c1;
    c1.rank = 1;
    c1.name = "Deep Shadow";
    c1.mean_dps = 480.0;
    results.push_back(c1);

    std::string batch_json = priest::build_export::export_specs_batch_json(results, &sim);
    CHECK(batch_json.find("\"format\": \"priest-specs-batch/1\"") != std::string::npos);
}
