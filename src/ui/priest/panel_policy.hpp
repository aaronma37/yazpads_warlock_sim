#pragma once
#include "imgui.h"
#include "src/sim/priest/policy.hpp"

namespace priest {

inline void render_priest_policy_panel(PolicyConfig& policy) {
    ImGui::BeginChild("PriestPolicyPanel", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Priest Rotational Policy & Priority");
    ImGui::Separator();

    // Rotation choice combo
    int rot_idx = static_cast<int>(policy.rotation);
    const char* rot_names[] = { "Shadow (SW:P -> MB -> MF)", "Smite DPS (Holy Fire -> Smite)", "Holy Fire Weaving" };
    if (ImGui::Combo("Rotation", &rot_idx, rot_names, 3)) {
        policy.rotation = static_cast<RotationChoice>(rot_idx);
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.7f, 1.0f, 1.0f), "Ability Priorities:");
    ImGui::Checkbox("Maintain Shadow Word: Pain", &policy.maintain_swp);
    ImGui::Checkbox("Cast Mind Blast on Cooldown", &policy.cast_mind_blast);
    ImGui::Checkbox("Cast Shadow Word: Death", &policy.cast_sw_death);
    if (policy.cast_sw_death) {
        ImGui::Indent();
        ImGui::Checkbox("Execute Only (<20% Target HP)", &policy.execute_sw_death_only);
        ImGui::Unindent();
    }
    ImGui::Checkbox("Cast Devouring Plague on Cooldown", &policy.cast_devouring_plague);
    ImGui::Checkbox("Clip Mind Flay after Tick 2 for Mind Blast", &policy.clip_mind_flay_for_mb);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.7f, 1.0f, 1.0f), "Major Cooldowns & Consumables:");
    ImGui::Checkbox("Use Inner Focus on Cooldown", &policy.use_inner_focus);
    ImGui::Checkbox("Use Power Infusion on Cooldown", &policy.use_power_infusion);

    float pot_thresh = static_cast<float>(policy.mana_potion_threshold * 100.0);
    if (ImGui::SliderFloat("Mana Potion Threshold (%)", &pot_thresh, 10.0f, 80.0f, "%.0f%%")) {
        policy.mana_potion_threshold = pot_thresh / 100.0;
    }

    float rune_thresh = static_cast<float>(policy.demonic_rune_threshold * 100.0);
    if (ImGui::SliderFloat("Demonic Rune Threshold (%)", &rune_thresh, 10.0f, 80.0f, "%.0f%%")) {
        policy.demonic_rune_threshold = rune_thresh / 100.0;
    }

    ImGui::EndChild();
}

} // namespace priest
