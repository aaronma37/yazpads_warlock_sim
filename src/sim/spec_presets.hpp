#pragma once
// Single source of truth for the standard talent spec presets.
//
// Previously the spec display names (e.g. "5/11/35 DS/AF DS-Imp") were
// hardcoded in at least three places — Optimizer::optimize_talents in
// src/sim/optimizer.cpp, the "Build Presets" menu in src/ui/ui_app.hpp,
// and the preset buttons in src/ui/panel_talents.hpp — so renaming a spec
// in one place left stale names everywhere else. Edit display_name here
// and every consumer (optimizer leaderboard, menus, talent panel) follows.
#include <string>
#include <vector>
#include "warlock_sim.hpp"

namespace warlock {

struct SpecPreset {
    const char* id;            // Stable key, never shown in UI (e.g. "ds_af")
    const char* display_name;  // Full name used in optimizer, menus, tables
    const char* short_label;   // Compact label for the talent panel buttons
    Talents (*make_talents)(); // Talent factory, e.g. &Talents::create_forever_ds_af
    RotationChoice rotation;
    PetChoice pet;
    bool sac_succubus;
    bool sac_imp;
    bool maintain_immolate;
};

// Canonical registry of the 17 standard specs. Order here determines the
// order of the talent panel buttons, the Build Presets menu, and (before
// DPS sorting) the optimizer candidates.
inline const std::vector<SpecPreset>& standard_spec_presets() {
    static const std::vector<SpecPreset> presets = {
        {"ds_af", "5/11/35 DS/AF DS-Imp", "DS/AF",
            &Talents::create_forever_ds_af, RotationChoice::SHADOW_DESTRO, PetChoice::NONE, false, true, true},
        {"fire_supp_ds", "9/11/31 Incinerate - Suppression + DS", "Fire Destro+Suppression",
            &Talents::create_forever_fire_destro, RotationChoice::FIRE_DESTRO, PetChoice::NONE, true, false, true},
        {"fire_supp_ds_no_corr", "7/11/33 Incinerate - Suppression + DS (No Corruption)", "Fire DS No Corr",
            &Talents::create_forever_ds_incinerate_no_corruption, RotationChoice::FIRE_DESTRO_NO_CORRUPTION, PetChoice::NONE, true, false, true},
        {"incin_ds_decimate", "3/17/31 Incinerate - DS + Decimate", "Shadow and Flame Fire DS-Succ",
            &Talents::create_forever_shadow_and_flame_fire_ds_succ, RotationChoice::FIRE_DESTRO_NO_CORRUPTION, PetChoice::NONE, true, false, true},
        {"ds_searing", "5/11/35 DS/Searing Pain DS-Succ", "DS/Searing Pain",
            &Talents::create_forever_ds_searing_pain, RotationChoice::FIRE_DESTRO, PetChoice::NONE, true, false, true},
        {"dp_shadow", "2/31/18 DP/AF Shadow", "DP/AF Shadow",
            &Talents::create_forever_dp_af_shadow, RotationChoice::DP_AF_SHADOW, PetChoice::SUCCUBUS, false, true, true},
        {"dp_shadow_corr", "2/31/18 DP/AF Shadow Corruption", "DP/AF Corruption",
            &Talents::create_forever_dp_af_shadow_corruption, RotationChoice::DP_AF_SHADOW, PetChoice::SUCCUBUS, false, true, true},
        {"aff_dp", "12/31/8 Aff/DP", "Aff/DP",
            &Talents::create_forever_aff_dp, RotationChoice::DP_AF_SHADOW, PetChoice::SUCCUBUS, false, true, false},
        {"dp_fire", "0/31/20 DP/AF Fire", "DP/AF Fire",
            &Talents::create_forever_dp_af_fire, RotationChoice::DP_RUIN_FIRE, PetChoice::IMP, true, false, true},
        {"deep_aff", "40/11/0 Deep Affliction DS-Imp", "Deep Affliction",
            &Talents::create_forever_deep_affliction, RotationChoice::DEEP_AFFLICTION_SB, PetChoice::NONE, false, true, true},
        {"deep_aff_imp", "35/6/10 Deep Affliction Imp", "Deep Affliction Imp",
            &Talents::create_forever_deep_affliction_imp, RotationChoice::DEEP_AFFLICTION_SB, PetChoice::IMP, false, false, true},
        {"sm_af", "32/0/19 SM/AF", "SM/AF",
            &Talents::create_forever_sm_af, RotationChoice::SM_RUIN, PetChoice::IMP, false, false, true},
        {"incin_decimate_imp", "1/17/33 Incinerate - Decimate + Imp", "Shadow and Flame Fire",
            &Talents::create_forever_shadow_and_flame, RotationChoice::FIRE_DESTRO, PetChoice::IMP, false, false, true},
        {"incin_supp_imp", "10/10/31 Incinerate - Suppression + Imp", "Shadow and Flame Fire 2",
            &Talents::create_forever_shadow_and_flame_fire_2, RotationChoice::SHADOW_AND_FLAME_FIRE_BANE, PetChoice::IMP, false, false, true},
        {"incin_supp_succ", "7/13/31 Incinerate - Suppression + Succubus", "Shadow and Flame Fire Succ",
            &Talents::create_forever_shadow_and_flame_fire_2_succubus, RotationChoice::SHADOW_AND_FLAME_FIRE_BANE, PetChoice::SUCCUBUS, false, false, true},
        {"sf_shadow_decimate", "2/17/32 Shadow and Flame Shadow - Decimate", "Shadow and Flame Shadow",
            &Talents::create_forever_shadow_and_flame_shadow, RotationChoice::SHADOW_DESTRO, PetChoice::IMP, false, false, true},
        {"sf_shadow", "8/13/30 Shadow and Flame Shadow", "Shadow and Flame Shadow 2",
            &Talents::create_forever_shadow_and_flame_shadow_2, RotationChoice::SHADOW_DESTRO_2, PetChoice::SUCCUBUS, false, false, true},
        {"nf_ds_ruin", "19/11/21 NF/DS/Ruin DS-Imp", "NF/DS/Ruin",
            &Talents::create_forever_nf_ds_ruin, RotationChoice::SHADOW_DESTRO, PetChoice::NONE, false, true, true},
        {"nf_af", "23/10/18 NF/AF", "NF/AF",
            &Talents::create_forever_nf_af, RotationChoice::SM_RUIN, PetChoice::IMP, false, false, true},
        {"aff_incinerate", "13/7/31 Aff Incinerate", "Aff Incinerate",
            &Talents::create_forever_aff_incinerate, RotationChoice::FIRE_DESTRO, PetChoice::SUCCUBUS, false, false, true},
    };
    return presets;
}

inline const SpecPreset* find_spec_preset(const std::string& id) {
    for (const auto& p : standard_spec_presets()) {
        if (id == p.id) return &p;
    }
    return nullptr;
}

// Applies a preset's full configuration (talents, rotation, pet, sacrifices,
// Immolate policy) to a simulator, mirroring what the UI preset buttons do.
inline void apply_spec_preset(WarlockSimulator& sim, const SpecPreset& p) {
    sim.talents = p.make_talents();
    sim.policy.rotation = p.rotation;
    sim.policy.pet = p.pet;
    sim.policy.maintain_immolate = p.maintain_immolate;
    sim.buffs.sacrifice_succubus = p.sac_succubus;
    sim.buffs.sacrifice_imp = p.sac_imp;
}

} // namespace warlock
