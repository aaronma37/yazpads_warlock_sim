#include "test_framework.hpp"
#include "src/sim/spec_presets.hpp"

using namespace warlock;

TEST_CASE(SpecPresets, CountAndUniqueNames) {
    const auto& presets = standard_spec_presets();
    CHECK_EQ(presets.size(), (size_t)19);
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
    CHECK(p->rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_BANE);
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
    CHECK(sim.policy.rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_BANE);
}

TEST_CASE(SpecPresets, IncinSuppDsNoCorruption) {
    const SpecPreset* p = find_spec_preset("fire_supp_ds_no_corr");
    CHECK(p != nullptr);
    CHECK(std::string(p->display_name) == "7/11/33 Incinerate - Suppression + DS (No Corruption)");
    CHECK(p->rotation == RotationChoice::FIRE_DESTRO_NO_CORRUPTION);
    CHECK(p->pet == PetChoice::NONE);
    CHECK(p->sac_succubus == true);
    CHECK(p->sac_imp == false);
    CHECK(p->maintain_immolate == true);

    Talents t = p->make_talents();
    CHECK_EQ(t.aff.total_points(), 7);
    CHECK_EQ(t.demo.total_points(), 11);
    CHECK_EQ(t.destro.total_points(), 33);
    CHECK_EQ(t.total_points(), 51);
    CHECK(t.is_valid());
    CHECK_EQ(t.aff.suppression, 5);
    CHECK_EQ(t.aff.improved_corruption, 0);
    CHECK_EQ(t.aff.improved_life_tap, 2);
    CHECK_EQ(t.destro.cataclysm, 3);
    CHECK_EQ(t.destro.incinerate, 1);
    CHECK_EQ(t.destro.conflagrate, 1);
    CHECK_EQ(t.demo.demonic_sacrifice, 1);

    WarlockSimulator sim;
    apply_spec_preset(sim, *p);
    CHECK(sim.policy.rotation == RotationChoice::FIRE_DESTRO_NO_CORRUPTION);
    CHECK(sim.policy.pet == PetChoice::NONE);
    CHECK(sim.buffs.sacrifice_succubus == true);
    CHECK(sim.buffs.sacrifice_imp == false);
    CHECK(sim.policy.maintain_immolate == true);

    // No-Corruption rotation must not schedule Corruption.
    auto rules = sim.policy.get_priority_rules(sim.talents);
    bool has_corruption = false;
    bool has_incinerate = false;
    bool has_conflagrate = false;
    bool has_immolate = false;
    for (const auto& r : rules) {
        if (r.action == PriorityAction::CORRUPTION) has_corruption = true;
        if (r.action == PriorityAction::INCINERATE_FILLER) has_incinerate = true;
        if (r.action == PriorityAction::CONFLAGRATE) has_conflagrate = true;
        if (r.action == PriorityAction::IMMOLATE) has_immolate = true;
    }
    CHECK(has_corruption == false);
    CHECK(has_incinerate == true);
    CHECK(has_conflagrate == true);
    CHECK(has_immolate == true);
}

TEST_CASE(SpecPresets, DpShadowCorruption) {
    const SpecPreset* p = find_spec_preset("dp_shadow_corr");
    CHECK(p != nullptr);
    CHECK(std::string(p->display_name) == "2/31/18 DP/AF Shadow Corruption");
    CHECK(p->rotation == RotationChoice::DP_AF_SHADOW);
    CHECK(p->pet == PetChoice::SUCCUBUS);
    CHECK(p->sac_succubus == false);
    CHECK(p->sac_imp == true);
    CHECK(p->maintain_immolate == true);

    // Same 2/31/18 split as DP/AF Shadow, with 2 Suppression moved to
    // 2 Improved Corruption.
    Talents t = p->make_talents();
    CHECK_EQ(t.aff.total_points(), 2);
    CHECK_EQ(t.demo.total_points(), 31);
    CHECK_EQ(t.destro.total_points(), 18);
    CHECK_EQ(t.total_points(), 51);
    CHECK(t.is_valid());
    CHECK_EQ(t.aff.suppression, 0);
    CHECK_EQ(t.aff.improved_corruption, 2);
    CHECK_EQ(t.demo.demonic_pact, 1);
    CHECK_EQ(t.demo.demonic_sacrifice, 1);
    CHECK_EQ(t.destro.improved_shadow_bolt, 5);

    const SpecPreset* base = find_spec_preset("dp_shadow");
    CHECK(base != nullptr);
    Talents bt = base->make_talents();
    CHECK_EQ(bt.aff.suppression, 2);
    CHECK_EQ(bt.aff.improved_corruption, 0);
    CHECK_EQ(bt.total_points(), t.total_points());
    CHECK(base->rotation == p->rotation);
    CHECK(base->pet == p->pet);

    WarlockSimulator sim;
    apply_spec_preset(sim, *p);
    CHECK(sim.policy.rotation == RotationChoice::DP_AF_SHADOW);
    CHECK(sim.policy.pet == PetChoice::SUCCUBUS);
    CHECK(sim.buffs.sacrifice_imp == true);

    // Corruption variant must schedule Corruption in the priority list.
    auto rules = sim.policy.get_priority_rules(sim.talents);
    bool has_corruption = false;
    bool has_sb_filler = false;
    for (const auto& r : rules) {
        if (r.action == PriorityAction::CORRUPTION) has_corruption = true;
        if (r.action == PriorityAction::SHADOW_BOLT_FILLER) has_sb_filler = true;
    }
    CHECK(has_corruption == true);
    CHECK(has_sb_filler == true);
}

TEST_CASE(SpecPresets, IncinDsDecimateNoCorruption) {
    const SpecPreset* p = find_spec_preset("incin_ds_decimate");
    CHECK(p != nullptr);
    CHECK(std::string(p->display_name) == "3/17/31 Incinerate - DS + Decimate");
    CHECK(p->rotation == RotationChoice::FIRE_DESTRO_NO_CORRUPTION);
    CHECK(p->pet == PetChoice::NONE);
    CHECK(p->sac_succubus == true);
    CHECK(p->sac_imp == false);
    CHECK(p->maintain_immolate == true);

    Talents t = p->make_talents();
    CHECK_EQ(t.aff.total_points(), 3);
    CHECK_EQ(t.demo.total_points(), 17);
    CHECK_EQ(t.destro.total_points(), 31);
    CHECK_EQ(t.total_points(), 51);
    CHECK(t.is_valid());
    CHECK_EQ(t.aff.improved_corruption, 0);
    CHECK_EQ(t.destro.incinerate, 1);
    CHECK_EQ(t.destro.conflagrate, 1);
    CHECK_EQ(t.demo.decimation, 2);
    CHECK_EQ(t.demo.demonic_sacrifice, 1);

    WarlockSimulator sim;
    apply_spec_preset(sim, *p);
    CHECK(sim.policy.rotation == RotationChoice::FIRE_DESTRO_NO_CORRUPTION);

    // No-Corruption rotation must not schedule Corruption.
    auto rules = sim.policy.get_priority_rules(sim.talents);
    bool has_corruption = false;
    bool has_incinerate = false;
    bool has_conflagrate = false;
    bool has_immolate = false;
    for (const auto& r : rules) {
        if (r.action == PriorityAction::CORRUPTION) has_corruption = true;
        if (r.action == PriorityAction::INCINERATE_FILLER) has_incinerate = true;
        if (r.action == PriorityAction::CONFLAGRATE) has_conflagrate = true;
        if (r.action == PriorityAction::IMMOLATE) has_immolate = true;
    }
    CHECK(has_corruption == false);
    CHECK(has_incinerate == true);
    CHECK(has_conflagrate == true);
    CHECK(has_immolate == true);
}

TEST_CASE(SpecPresets, DeepAfflictionImp) {
    const SpecPreset* p = find_spec_preset("deep_aff_imp");
    CHECK(p != nullptr);
    CHECK(std::string(p->display_name) == "35/6/10 Deep Affliction Imp");
    CHECK(p->rotation == RotationChoice::DEEP_AFFLICTION_SB);
    CHECK(p->pet == PetChoice::IMP);
    CHECK(p->sac_succubus == false);
    CHECK(p->sac_imp == false);
    CHECK(p->maintain_immolate == true);

    Talents t = p->make_talents();
    CHECK_EQ(t.aff.total_points(), 35);
    CHECK_EQ(t.demo.total_points(), 6);
    CHECK_EQ(t.destro.total_points(), 10);
    CHECK_EQ(t.total_points(), 51);
    CHECK(t.is_valid());
    CHECK_EQ(t.aff.suppression, 5);
    CHECK_EQ(t.aff.improved_corruption, 5);
    CHECK_EQ(t.aff.nightfall, 2);
    CHECK_EQ(t.aff.drain_hope, 1);
    CHECK_EQ(t.demo.improved_imp, 3);
    CHECK_EQ(t.demo.unholy_power, 2);
    CHECK_EQ(t.demo.demonic_energies, 1);
    CHECK_EQ(t.destro.improved_shadow_bolt, 5);
    CHECK_EQ(t.destro.bane, 5);

    WarlockSimulator sim;
    apply_spec_preset(sim, *p);
    CHECK(sim.policy.rotation == RotationChoice::DEEP_AFFLICTION_SB);
    CHECK(sim.policy.pet == PetChoice::IMP);
    CHECK(sim.buffs.sacrifice_imp == false);
}

TEST_CASE(SpecPresets, AffIncinerate) {
    const SpecPreset* p = find_spec_preset("aff_incinerate");
    CHECK(p != nullptr);
    CHECK(std::string(p->display_name) == "13/7/31 Aff Incinerate");
    CHECK(p->rotation == RotationChoice::FIRE_DESTRO);
    CHECK(p->pet == PetChoice::SUCCUBUS);
    CHECK(p->sac_succubus == false);
    CHECK(p->sac_imp == false);
    CHECK(p->maintain_immolate == true);

    Talents t = p->make_talents();
    CHECK_EQ(t.aff.total_points(), 13);
    CHECK_EQ(t.demo.total_points(), 7);
    CHECK_EQ(t.destro.total_points(), 31);
    CHECK_EQ(t.total_points(), 51);
    CHECK(t.is_valid());
    CHECK_EQ(t.aff.suppression, 5);
    CHECK_EQ(t.aff.improved_corruption, 2);
    CHECK_EQ(t.aff.amplify_curse, 1);
    CHECK_EQ(t.demo.unholy_power, 4);
    CHECK_EQ(t.destro.bane, 5);
    CHECK_EQ(t.destro.cataclysm, 3);
    CHECK_EQ(t.destro.aftermath, 3);
    CHECK_EQ(t.destro.ruin, 5);
    CHECK_EQ(t.destro.incinerate, 1);

    WarlockSimulator sim;
    apply_spec_preset(sim, *p);
    CHECK(sim.policy.rotation == RotationChoice::FIRE_DESTRO);
    CHECK(sim.policy.pet == PetChoice::SUCCUBUS);
    CHECK(sim.buffs.sacrifice_succubus == false);
    CHECK(sim.buffs.sacrifice_imp == false);
}
