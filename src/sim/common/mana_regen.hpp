#pragma once
#include <cmath>
#include <algorithm>
#include "player_class.hpp"
#include "stats.hpp"

namespace sim {

// Configuration for spirit-based and MP5 mana regeneration
struct ManaRegenConfig {
    double mp5 = 0.0;
    double spirit = 0.0;
    double intellect = 0.0;
    double meditation_ratio = 0.0; // Fraction of spirit regen active while inside 5SR (e.g. 0.15 for 15% Meditation)
    PlayerClass player_class = PlayerClass::PRIEST;

    // Custom coefficient override (if 0.0, standard class formula is used)
    double custom_spirit_coeff = 0.0;
};

// Pure calculation functions for Spirit-based mana regeneration
class ManaRegenCalculator {
public:
    // Returns the class-specific spirit coefficient for the Level 60 mana regen formula:
    // tick_spirit_regen = 5 * (0.001 + sqrt(Int) * Spirit * coeff)
    static constexpr double get_spirit_coefficient(PlayerClass c) {
        switch (c) {
            case PlayerClass::PRIEST:
                return 0.009327; // Classic Priest Level 60
            case PlayerClass::WARLOCK:
                return 0.007725; // Classic Warlock Level 60
            default:
                return 0.009327;
        }
    }

    // Calculates base spirit-based mana generated per 2-second tick outside 5SR
    static inline double calculate_spirit_regen_per_tick(double intellect, double spirit, PlayerClass c) {
        if (spirit <= 0.0) return 0.0;
        double coeff = get_spirit_coefficient(c);
        double safe_int = std::max(0.0, intellect);
        // Standard WoW 1.12 mana regen per 2-second tick from Spirit
        return 5.0 * (0.001 + std::sqrt(safe_int) * spirit * coeff);
    }

    // Calculates MP5 contribution per 2-second tick (2s / 5s = 0.40)
    static inline double calculate_mp5_regen_per_tick(double mp5) {
        return mp5 * 0.40;
    }

    // Calculates total mana restored in a 2-second tick given 5SR state
    static inline double calculate_tick_mana(
        double intellect,
        double spirit,
        double mp5,
        bool inside_5sr,
        double meditation_ratio,
        PlayerClass c
    ) {
        double spirit_tick = calculate_spirit_regen_per_tick(intellect, spirit, c);
        double spirit_contrib = inside_5sr ? (spirit_tick * meditation_ratio) : spirit_tick;
        double mp5_contrib = calculate_mp5_regen_per_tick(mp5);
        return spirit_contrib + mp5_contrib;
    }
};

// Stateful Five-Second Rule (5SR) tracker for discrete event simulation
class FiveSecondRuleTracker {
private:
    double fsr_expires_at_ = -1.0;
    double last_mana_spent_time_ = -1.0;

public:
    inline void reset() {
        fsr_expires_at_ = -1.0;
        last_mana_spent_time_ = -1.0;
    }

    // Call whenever a spell successfully spends mana
    inline void on_mana_spent(double current_time) {
        last_mana_spent_time_ = current_time;
        fsr_expires_at_ = current_time + 5.0;
    }

    // Returns true if currently inside the Five-Second Rule window (spirit regen hindered)
    inline bool is_inside_5sr(double current_time) const {
        return current_time < fsr_expires_at_;
    }

    inline double time_remaining_in_5sr(double current_time) const {
        if (current_time >= fsr_expires_at_) return 0.0;
        return fsr_expires_at_ - current_time;
    }

    inline double last_mana_spent_time() const { return last_mana_spent_time_; }
    inline double fsr_expires_at() const { return fsr_expires_at_; }
};

} // namespace sim

namespace warlock {
    using sim::ManaRegenConfig;
    using sim::ManaRegenCalculator;
    using sim::FiveSecondRuleTracker;
}
