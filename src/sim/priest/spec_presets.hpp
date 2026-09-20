#pragma once
#include <string>
#include <vector>
#include "talents.hpp"
#include "policy.hpp"

namespace priest {

struct SpecPreset {
    const char* id;
    const char* display_name;
    const char* short_label;
    Talents (*make_talents)();
    RotationChoice rotation;
    bool shadowform;
};

inline const std::vector<SpecPreset>& standard_spec_presets() {
    static const std::vector<SpecPreset> presets = {
        {"shadow_standard", "14/0/37 Shadow (Meditation + Inner Focus)", "Shadow 14/0/37",
            &Talents::create_forever_shadow, RotationChoice::SHADOW_PRIEST, true},
        {"deep_shadow", "10/0/41 Deep Shadow (Early Demise + Shadowform)", "Deep Shadow 10/0/41",
            &Talents::create_forever_deep_shadow, RotationChoice::SHADOW_PRIEST, true},
        {"smite_dps", "14/37/0 Smite / Holy DPS (Searing Light + Nova)", "Smite 14/37/0",
            &Talents::create_forever_smite, RotationChoice::SMITE_PRIEST, false},
        {"pi_smite", "21/30/0 Power Infusion Smite DPS", "PI Smite 21/30/0",
            &Talents::create_forever_pi_smite, RotationChoice::SMITE_PRIEST, false}
    };
    return presets;
}

} // namespace priest
