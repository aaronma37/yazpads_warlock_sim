#pragma once
#include <cstdint>

namespace sim {

enum class PlayerClass : uint8_t {
    WARLOCK = 0,
    PRIEST = 1
};

inline const char* player_class_to_string(PlayerClass c) {
    switch (c) {
        case PlayerClass::WARLOCK: return "Warlock";
        case PlayerClass::PRIEST: return "Priest";
        default: return "Unknown";
    }
}

inline const char* player_class_to_icon(PlayerClass c) {
    switch (c) {
        case PlayerClass::WARLOCK: return "Class_Warlock.png";
        case PlayerClass::PRIEST: return "Class_Priest.png";
        default: return "INV_Misc_QuestionMark.png";
    }
}

// Standard WoW Class Hex Colors: Warlock = #9482C9 (148, 130, 201), Priest = #FFFFFF (255, 255, 255)
inline uint32_t player_class_color(PlayerClass c) {
    switch (c) {
        case PlayerClass::WARLOCK: return 0xFFC98294; // ABGR for ImGui / ImU32
        case PlayerClass::PRIEST:  return 0xFFFFFFFF;
        default:                   return 0xFFFFFFFF;
    }
}

} // namespace sim

namespace warlock {
    using sim::PlayerClass;
    using sim::player_class_to_string;
    using sim::player_class_to_icon;
    using sim::player_class_color;
}
