#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"
#include "src/sim/priest/mechanics.hpp"

namespace priest {

inline void render_priest_mechanics_panel(MechanicsConfig& mechanics) {
    if (warlock::WowCollapsingHeader("Game Mechanics", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);

        // 1. Raid Debuff Slot Limit
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Raid Debuff Slot Limit:");
        bool infinite_debuffs = (mechanics.debuff_limit == 0);
        if (warlock::WowCheckbox("Infinite Debuff Slots (Unlimited - Default)", &infinite_debuffs)) {
            mechanics.debuff_limit = infinite_debuffs ? 0 : 16;
            mechanics.enforce_debuff_slots = !infinite_debuffs;
        }
        if (!infinite_debuffs) {
            ImGui::SameLine();
            ImGui::RadioButton("16 Slots", &mechanics.debuff_limit, 16);
            ImGui::SameLine();
            ImGui::RadioButton("8 Slots", &mechanics.debuff_limit, 8);
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 2. Shadow Weaving
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Shadow Weaving Mechanics:");
        warlock::WowCheckbox("Personal-Only Shadow Weaving (Default: ON in Forever)", &mechanics.shadow_weaving_personal);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Forever): Shadow Weaving only amplifies the caster's own Shadow spells.\nWhen OFF (Classic): Shadow Weaving increases all shadow damage dealt by the raid.");
        }

        float sw_per_stack = static_cast<float>(mechanics.shadow_weaving_per_stack * 100.0);
        if (warlock::WowSliderFloat("Shadow Weaving % Per Stack", &sw_per_stack, 1.0f, 5.0f, "%.1f%% / stack")) {
            mechanics.shadow_weaving_per_stack = sw_per_stack / 100.0;
        }
        ImGui::TextDisabled("  -> Max 5 stacks: +%.1f%% total Shadow damage", sw_per_stack * 5.0f);

        ImGui::Spacing();
        ImGui::Separator();

        // 3. DoT Scaling & Channel Mechanics
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "DoT Scaling & Channel Mechanics:");
        warlock::WowCheckbox("Enable DoT Snapshotting (Classic WoW)", &mechanics.snapshot_dots);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Classic): DoTs snapshot spell power at cast time.\nWhen OFF (Modern / Forever): DoTs dynamically scale on each tick.");
        }
        warlock::WowCheckbox("Allow Mind Flay Tick Clipping", &mechanics.allow_mind_flay_clipping);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON: Allows re-channeling Mind Flay or casting Mind Blast after 2 ticks (2.0s) without GCD delay.");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 4. Resistance & Partial Resists
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Spell Resistance Mechanics:");
        warlock::WowCheckbox("Enable Partial Resists (Classic 4-Roll Table)", &mechanics.partial_resists_enabled);

        ImGui::Spacing();
        ImGui::Separator();

        // 5. 5-Second Rule & Meditation
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "5-Second Rule & Meditation:");
        float med_ratio = static_cast<float>(mechanics.meditation_casting_regen_ratio * 100.0);
        if (warlock::WowSliderFloat("Meditation In-5SR Regen Ratio", &med_ratio, 15.0f, 100.0f, "%.0f%%")) {
            mechanics.meditation_casting_regen_ratio = med_ratio / 100.0;
        }
        ImGui::TextDisabled("  -> Percentage of Spirit mana regeneration continuing while casting (Rank 3: 50%% in Forever)");

        ImGui::Spacing();
        ImGui::Separator();

        // 6. Spell Batching & Missile Physics
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Batching & Missile Travel Time:");
        warlock::WowCheckbox("Simulate Spell Batching Window", &mechanics.spell_batching);
        if (mechanics.spell_batching) {
            float bw = static_cast<float>(mechanics.batch_window_ms);
            if (warlock::WowSliderFloat("Batch Window (ms)", &bw, 50.0f, 400.0f, "%.0f ms")) {
                mechanics.batch_window_ms = bw;
            }
        }

        ImGui::Unindent(8.0f);
    }
}

} // namespace priest
