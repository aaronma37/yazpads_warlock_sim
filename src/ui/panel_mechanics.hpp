#pragma once
#include "imgui.h"
#include "src/sim/mechanics.hpp"

namespace warlock {

inline void render_panel_mechanics(MechanicsConfig& mechanics) {
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "Modular Game Rules Engine:");
    ImGui::TextWrapped("Customize core mechanics to simulate pure 1.12 Classic WoW or experiment with new game versions & alternate rulesets.");
    ImGui::Separator();

    // 1. Snapshotting toggle
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "DoT Snapshotting Mechanics:");
    ImGui::Checkbox("Enable DoT Snapshotting (Classic WoW)", &mechanics.snapshot_dots);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When ON (Classic): DoTs snapshot spell power and %% damage modifiers at cast time.\nWhen OFF (Modern): DoTs dynamically recalculate damage on every single tick.");
    }
    if (mechanics.snapshot_dots) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "  -> Snapshotting active: Spell power buffs persist for full DoT duration.");
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.4f, 1.0f), "  -> Dynamic ticks active: Ticks scale dynamically with temporary buffs.");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Spell Batching
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Network & Spell Batching:");
    ImGui::Checkbox("Simulate Spell Batching", &mechanics.spell_batching);
    if (mechanics.spell_batching) {
        ImGui::SliderFloat("Batch Window (ms)", (float*)&mechanics.batch_window_ms, 10.0f, 400.0f, "%.0f ms");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Raid Debuff Slots
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Raid Debuff Slot Limit:");
    ImGui::RadioButton("16 Debuff Slots (Patch 1.12)", &mechanics.debuff_limit, 16);
    ImGui::SameLine();
    ImGui::RadioButton("8 Slots (Early Vanilla)", &mechanics.debuff_limit, 8);
    ImGui::SameLine();
    ImGui::RadioButton("Unlimited Slots (Modern)", &mechanics.debuff_limit, 0);

    ImGui::Spacing();
    ImGui::Separator();

    // 4. Resistance & Partial Resists
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Spell Resistance Mechanics:");
    ImGui::Checkbox("Enable Partial Resists (Classic 4-Roll Table)", &mechanics.partial_resists_enabled);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When ON: Boss 24 innate resistance yields 0%%, 25%%, 50%%, 75%% partial resist rolls.\nWhen OFF: Pure binary hit/miss.");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 5. Improved Shadow Bolt consumption
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Improved Shadow Bolt (ISB) Charges:");
    ImGui::Checkbox("All Shadow Damage Consumes ISB (Wands, SW:P, etc.)", &mechanics.isb_all_shadow_sources);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When ON: Any shadow damage from any source consumes an ISB stack.\nWhen OFF: Only primary Warlock direct shadow damage spells consume stacks.");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 6. Projectile Travel Time
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Missile Physics & Travel Time:");
    ImGui::Checkbox("Simulate Projectile Travel Time", &mechanics.projectile_travel_time);
    if (mechanics.projectile_travel_time) {
        ImGui::SliderFloat("Boss Distance (yards)", (float*)&mechanics.default_boss_distance_yards, 10.0f, 40.0f, "%.0f yd");
        ImGui::SliderFloat("Missile Speed (yd/s)", (float*)&mechanics.projectile_speed_yards_per_sec, 15.0f, 40.0f, "%.1f yd/s");
    }
}

} // namespace warlock
