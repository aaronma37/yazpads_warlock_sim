#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace sim {

// Standardized continuous/discrete state features for VIPER policy extraction
struct SimObservation {
    // 1. Player Resources & Fight Progress (Normalized [0, 1])
    float player_mana_pct = 1.0f;       // player_mana / max_mana
    float player_hp_pct = 1.0f;         // player_health / max_health
    float fight_progress_pct = 0.0f;    // now / fight_duration
    float time_remaining_sec = 60.0f;   // fight_duration - now
    float target_hp_pct = 1.0f;         // 1.0 - (now / fight_duration)

    // 2. Target Count & Multi-Target Context
    float num_targets = 1.0f;
    float target2_has_havoc = 0.0f;     // 1.0 if target 2 has Bane of Havoc active

    // 3. Player Procs & Active Buff Timers (in seconds remaining, 0 if inactive)
    float nightfall_proc_active = 0.0f;  // 1.0 if Shadow Trance active
    float decimation_rem_sec = 0.0f;     // Decimation buff duration left
    float shadow_and_flame_rem_sec = 0.0f; // Shadow & Flame +10% buff duration left
    float trinket_rem_sec = 0.0f;        // Trinket on-use buff active
    float racial_rem_sec = 0.0f;         // Blood Fury / Berserking / Eureka active
    float eureka_charges = 0.0f;         // Gnome Eureka remaining charges

    // 4. Primary Target Debuffs & DoTs (in seconds remaining, 0 if expired)
    float dot_corruption_rem_sec = 0.0f;
    float dot_agony_rem_sec = 0.0f;
    float dot_doom_rem_sec = 0.0f;
    float dot_immolate_rem_sec = 0.0f;
    float dot_siphon_life_rem_sec = 0.0f;
    float dot_wrack_rem_sec = 0.0f;
    float isb_charges_rem = 0.0f;        // ISB debuff charges on target (0..4)

    // 5. Ability Cooldowns (in seconds until ready, 0.0 if ready)
    float cd_conflagrate_sec = 0.0f;
    float cd_shadowburn_sec = 0.0f;
    float cd_curse_of_doom_sec = 0.0f;
    float cd_amplify_curse_sec = 0.0f;
    float cd_racial_sec = 0.0f;
    float cd_potion_sec = 0.0f;
    float cd_demonic_rune_sec = 0.0f;

    // Feature Count constant
    static constexpr size_t FEATURE_COUNT = 24;

    // Converts observation into a flat float array for ML model ingestion
    std::array<float, FEATURE_COUNT> to_array() const {
        return {
            player_mana_pct,
            player_hp_pct,
            fight_progress_pct,
            time_remaining_sec,
            target_hp_pct,
            num_targets,
            target2_has_havoc,
            nightfall_proc_active,
            decimation_rem_sec,
            shadow_and_flame_rem_sec,
            trinket_rem_sec,
            racial_rem_sec,
            eureka_charges,
            dot_corruption_rem_sec,
            dot_agony_rem_sec,
            dot_doom_rem_sec,
            dot_immolate_rem_sec,
            dot_siphon_life_rem_sec,
            dot_wrack_rem_sec,
            isb_charges_rem,
            cd_conflagrate_sec,
            cd_shadowburn_sec,
            cd_curse_of_doom_sec,
            cd_racial_sec
        };
    }

    static std::vector<std::string> feature_names() {
        return {
            "player_mana_pct",
            "player_hp_pct",
            "fight_progress_pct",
            "time_remaining_sec",
            "target_hp_pct",
            "num_targets",
            "target2_has_havoc",
            "nightfall_proc_active",
            "decimation_rem_sec",
            "shadow_and_flame_rem_sec",
            "trinket_rem_sec",
            "racial_rem_sec",
            "eureka_charges",
            "dot_corruption_rem_sec",
            "dot_agony_rem_sec",
            "dot_doom_rem_sec",
            "dot_immolate_rem_sec",
            "dot_siphon_life_rem_sec",
            "dot_wrack_rem_sec",
            "isb_charges_rem",
            "cd_conflagrate_sec",
            "cd_shadowburn_sec",
            "cd_curse_of_doom_sec",
            "cd_racial_sec"
        };
    }
};

// Represents a single labeled transition logged during a VIPER DAgger iteration
struct VIPERStep {
    SimObservation state;
    uint8_t oracle_action = 0;      // Best action chosen by the Oracle (e.g. PriorityAction / SpellID)
    std::string action_name;        // Human readable action name
    float sample_weight = 1.0f;     // Q-loss weight: w(s) = max_a Q(s,a) - min_a Q(s,a) (or regret gap)
    float q_best = 0.0f;            // max_a Q(s, a)
    float q_second_best = 0.0f;     // second highest Q(s, a)
    std::vector<float> q_values;    // Q(s, a) for all evaluated actions
};

// Dataset container for VIPER training
struct VIPERDataset {
    std::vector<VIPERStep> samples;

    void add_sample(const VIPERStep& sample) {
        samples.push_back(sample);
    }

    size_t size() const { return samples.size(); }
    void clear() { samples.clear(); }

    std::string to_csv() const {
        std::ostringstream ss;
        auto names = SimObservation::feature_names();
        for (size_t i = 0; i < names.size(); ++i) {
            ss << names[i] << ",";
        }
        ss << "oracle_action,action_name,sample_weight,q_best,q_second_best\n";

        for (const auto& s : samples) {
            auto arr = s.state.to_array();
            for (float val : arr) {
                ss << std::fixed << std::setprecision(4) << val << ",";
            }
            ss << static_cast<int>(s.oracle_action) << ","
               << s.action_name << ","
               << std::fixed << std::setprecision(4) << s.sample_weight << ","
               << s.q_best << ","
               << s.q_second_best << "\n";
        }
        return ss.str();
    }
};

} // namespace sim
