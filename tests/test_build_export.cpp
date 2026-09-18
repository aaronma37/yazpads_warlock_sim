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
    std::string json = build_export::export_build_json(sim);
    CHECK(json.find("\"format\": \"warlock-build/1\"") != std::string::npos);
    CHECK(json.find("\"race\": \"Human\"") != std::string::npos);
    CHECK(json.find("\"fight_duration\": 180") != std::string::npos);
    CHECK(json.find("\"stats_mode\": \"raw\"") != std::string::npos);
    CHECK(json.find("\"talents\"") != std::string::npos);
    CHECK(json.find("\"buffs\"") != std::string::npos);
    CHECK(json.find("\"policy\"") != std::string::npos);
    CHECK(json.find("\"mechanics\"") != std::string::npos);
    CHECK(json.find("\"target\"") != std::string::npos);
    CHECK(json.find("\"gear\"") != std::string::npos);
    CHECK(json.find("\"Head\"") != std::string::npos);
    // Braces balance (no braces appear inside string values here).
    CHECK_EQ(count_char(json, '{'), count_char(json, '}'));
    CHECK_EQ(count_char(json, '['), count_char(json, ']'));
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
    sim.gear.name = "Test \"Build\" \\ mod";
    std::string json = build_export::export_build_json(sim);
    CHECK(json.find("Test \\\"Build\\\" \\\\ mod") != std::string::npos);
    CHECK_EQ(count_char(json, '{'), count_char(json, '}'));
}
