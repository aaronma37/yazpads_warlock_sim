#include "test_framework.hpp"
#include "src/sim/spec_presets.hpp"

using namespace warlock;

TEST_CASE(SpecPresets, CountAndUniqueNames) {
    const auto& presets = standard_spec_presets();
    CHECK_EQ(presets.size(), (size_t)14);
    for (size_t i = 0; i < presets.size(); ++i) {
        CHECK(presets[i].display_name != nullptr && std::string(presets[i].display_name).size() > 0);
        CHECK(presets[i].short_label != nullptr && std::string(presets[i].short_label).size() > 0);
        CHECK(presets[i].make_talents != nullptr);
        for (size_t j = i + 1; j < presets.size(); ++j) {
            CHECK(std::string(presets[i].display_name) != std::string(presets[j].display_name));
            CHECK(std::string(presets[i].id) != std::string(presets[j].id));
        }
    }
}

TEST_CASE(SpecPresets, TalentsAreValid51PointBuilds) {
    for (const auto& p : standard_spec_presets()) {
        Talents t = p.make_talents();
        CHECK_EQ(t.total_points(), 51);
        CHECK(t.is_valid());
    }
}

TEST_CASE(SpecPresets, ApplyAndLookupRoundTrip) {
    WarlockSimulator sim;
    for (const auto& p : standard_spec_presets()) {
        const SpecPreset* found = find_spec_preset(p.id);
        CHECK(found != nullptr);
        CHECK(std::string(found->display_name) == std::string(p.display_name));
        apply_spec_preset(sim, p);
        Talents expected = p.make_talents();
        CHECK_EQ(sim.talents.total_points(), expected.total_points());
        CHECK(sim.policy.rotation == p.rotation);
        CHECK(sim.policy.pet == p.pet);
        CHECK(sim.buffs.sacrifice_succubus == p.sac_succubus);
        CHECK(sim.buffs.sacrifice_imp == p.sac_imp);
        CHECK(sim.policy.maintain_immolate == p.maintain_immolate);
    }
    CHECK(find_spec_preset("no_such_spec") == nullptr);
}
