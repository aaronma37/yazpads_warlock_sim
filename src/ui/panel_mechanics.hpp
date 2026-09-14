#pragma once
#include "imgui.h"
#include "src/sim/mechanics.hpp"

namespace warlock {

inline void render_panel_mechanics(MechanicsConfig& mechanics) {
    if (ImGui::CollapsingHeader("Game Mechanics", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);

        // 1. Raid Debuff Slots
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Raid Debuff Slot Limit:");
        bool infinite_debuffs = (mechanics.debuff_limit == 0);
        if (ImGui::Checkbox("Infinite Debuff Slots (Unlimited - Default)", &infinite_debuffs)) {
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

        // 2. Personal Shadow Weaving
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Shadow Weaving (Shadow Priest):");
        ImGui::Checkbox("Personal-Only Shadow Weaving (Warlocks do not benefit)", &mechanics.personal_shadow_weaving);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Updated): Shadow Weaving only amplifies the Shadow Priest who applied it.\nWhen OFF (Classic): All raid shadow damage gains +15%% from Shadow Weaving.");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 3. DoT Snapshotting
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "DoT Scaling Mechanics:");
        ImGui::Checkbox("Enable DoT Snapshotting (Classic WoW)", &mechanics.snapshot_dots);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Classic): DoTs snapshot spell power at cast time.\nWhen OFF (Modern / Forever): DoTs dynamically scale on each tick.");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 4. Resistance & Partial Resists
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Spell Resistance Mechanics:");
        ImGui::Checkbox("Enable Partial Resists (Classic 4-Roll Table)", &mechanics.partial_resists_enabled);

        ImGui::Spacing();
        ImGui::Separator();

        // 5. Improved Shadow Bolt
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Improved Shadow Bolt (ISB):");
        ImGui::TextDisabled("ISB operates as a 12-second debuff window on critical strikes.");

        ImGui::Spacing();
        ImGui::Separator();

        // 6. Pet Stat Scaling & Mana Management
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Pet Stat Inheritance & Mana Management:");
        ImGui::Checkbox("Enable Pet Spell Power / AP Scaling (Forever)", &mechanics.pet_scaling);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Forever): Summoned pets inherit 57%% of master's Spell Power to their spells and Attack Power.\nWhen OFF (Classic 1.12): Pets deal flat base ability damage.");
        }
        ImGui::Checkbox("Enable Pet Mana Tracking & Spell Costs", &mechanics.pet_mana_management);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON: Imp (1,150 base mana) and Succubus (1,450 base mana) consume mana on casts and regen mana passively (MP5), via raid buffs (Blessing of Wisdom, Judgement of Wisdom), and Demonic Energies talent.\nWhen OFF: Pets have infinite mana.");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 7. Projectile Travel Time
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Missile Physics & Travel Time:");
        ImGui::Checkbox("Simulate Projectile Travel Time", &mechanics.projectile_travel_time);
        if (mechanics.projectile_travel_time) {
            ImGui::SliderFloat("Boss Distance (yd)", (float*)&mechanics.default_boss_distance_yards, 10.0f, 40.0f, "%.0f yd");
        }

        ImGui::Unindent(8.0f);
    }
}

} // namespace warlock
