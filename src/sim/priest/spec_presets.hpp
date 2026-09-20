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
        {"shadow_standard", "13/0/38 Shadow (Meditation + Inner Focus)", "Shadow 13/0/38",
            &Talents::create_forever_shadow, RotationChoice::SHADOW_PRIEST, true},
        {"smite_dps", "14/37/0 Smite / Holy DPS", "Smite 14/37/0",
            &Talents::create_forever_smite, RotationChoice::SMITE_PRIEST, false}
    };
    return presets;
}

} // namespace priest
