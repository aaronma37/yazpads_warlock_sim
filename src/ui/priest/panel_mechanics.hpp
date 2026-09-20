#pragma once
#include "imgui.h"
#include "src/sim/priest/mechanics.hpp"

namespace priest {

inline void render_priest_mechanics_panel(MechanicsConfig& mechanics) {
    ImGui::BeginChild("PriestMechanicsPanel", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Priest Mechanics Configuration");
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Priest-Specific Mechanics:");
    ImGui::Checkbox("Shadowform Active (+10% Shadow Dmg, -50% Mana, 2.0x Crit)", &mechanics.shadowform_enabled);
    ImGui::Checkbox("Personal Shadow Weaving Only", &mechanics.shadow_weaving_personal);
    ImGui::Checkbox("Allow Mind Flay Tick Clipping", &mechanics.allow_mind_flay_clipping);

    float sw_per_stack = static_cast<float>(mechanics.shadow_weaving_per_stack * 100.0);
    if (ImGui::SliderFloat("Shadow Weaving % Per Stack", &sw_per_stack, 1.0f, 5.0f, "%.1f%%")) {
        mechanics.shadow_weaving_per_stack = sw_per_stack / 100.0;
    }

    float med_ratio = static_cast<float>(mechanics.meditation_casting_regen_ratio * 100.0);
    if (ImGui::SliderFloat("Meditation 3/3 Casting Regen (%)", &med_ratio, 15.0f, 100.0f, "%.0f%%")) {
        mechanics.meditation_casting_regen_ratio = med_ratio / 100.0;
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "Universal Simulation Mechanics:");
    ImGui::Checkbox("DoT Snapshotting (Classic WoW vs WoW Forever)", &mechanics.snapshot_dots);
    ImGui::Checkbox("Partial Resists (4-roll resist table)", &mechanics.partial_resists_enabled);
    ImGui::Checkbox("Spell Batching Window", &mechanics.spell_batching);
    if (mechanics.spell_batching) {
        float bw = static_cast<float>(mechanics.batch_window_ms);
        if (ImGui::SliderFloat("Batch Window (ms)", &bw, 50.0f, 500.0f, "%.0f ms")) {
            mechanics.batch_window_ms = bw;
        }
    }

    ImGui::EndChild();
}

} // namespace priest
