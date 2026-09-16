#include "test_framework.hpp"
#include "src/sim/spec_presets.hpp"

using namespace warlock;

TEST_CASE(SpecPresets, CountAndUniqueNames) {
    const auto& presets = standard_spec_presets();
    CHECK_EQ(presets.size(), (size_t)15);
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

TEST_CASE(SpecPresets, IncinSuppSuccubus) {
    const SpecPreset* p = find_spec_preset("incin_supp_succ");
    CHECK(p != nullptr);
    CHECK(std::string(p->display_name) == "7/13/31 Incinerate - Suppression + Succubus");
    CHECK(p->rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_2);
    CHECK(p->pet == PetChoice::SUCCUBUS);
    CHECK(p->sac_succubus == false);
    CHECK(p->sac_imp == false);

    Talents t = p->make_talents();
    CHECK_EQ(t.aff.total_points(), 7);
    CHECK_EQ(t.demo.total_points(), 13);
    CHECK_EQ(t.destro.total_points(), 31);
    CHECK_EQ(t.total_points(), 51);
    CHECK_EQ(t.aff.suppression, 5);
    CHECK_EQ(t.aff.improved_corruption, 2);
    CHECK_EQ(t.demo.unholy_power, 5);
    CHECK_EQ(t.demo.improved_sayaad, 3);
    CHECK_EQ(t.destro.ruin, 5);
    CHECK_EQ(t.destro.incinerate, 1);

    WarlockSimulator sim;
    apply_spec_preset(sim, *p);
    CHECK(sim.policy.pet == PetChoice::SUCCUBUS);
    CHECK(sim.policy.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_2);
}
