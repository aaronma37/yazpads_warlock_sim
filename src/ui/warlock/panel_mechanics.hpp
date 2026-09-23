#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"
#include "src/sim/mechanics.hpp"

namespace warlock {

inline void render_panel_mechanics(MechanicsConfig& mechanics) {
    if (WowCollapsingHeader("Game Mechanics", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent(8.0f);

        // 1. Raid Debuff Slots
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Raid Debuff Slot Limit:");
        bool infinite_debuffs = (mechanics.debuff_limit == 0);
        if (WowCheckbox("Infinite Debuff Slots (Unlimited - Default)", &infinite_debuffs)) {
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
        WowCheckbox("Personal-Only Shadow Weaving (Warlocks do not benefit)", &mechanics.personal_shadow_weaving);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Updated): Shadow Weaving only amplifies the Shadow Priest who applied it.\nWhen OFF (Classic): All raid shadow damage gains +15%% from Shadow Weaving.");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 3. DoT Snapshotting
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "DoT Scaling Mechanics:");
        WowCheckbox("Enable DoT Snapshotting (Classic WoW)", &mechanics.snapshot_dots);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Classic): DoTs snapshot spell power at cast time.\nWhen OFF (Modern / Forever): DoTs dynamically scale on each tick.");
        }
        WowCheckbox("Instant Cast DoT Wrack (Non-Channeled)", &mechanics.instant_drain_hope);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON: Wrack is cast as an instant 6-second DoT (1.5s GCD), allowing filler casts during its duration.\nWhen OFF (Default): Wrack is a 6-second channeled spell that locks your casting during the channel.");
        }
        bool corr_120 = (mechanics.corruption_sp_coefficient >= 1.19);
        if (WowCheckbox("Corruption 1.2 SP Coefficient (120%)", &corr_120)) {
            mechanics.corruption_sp_coefficient = corr_120 ? 1.2 : 1.0;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", "When ON: Corruption gains 120% (1.2x) total master Spell Power over its 18-second duration (20% SP per tick).\nWhen OFF (Default): Corruption gains standard 100% (1.0x) total master Spell Power (16.67% SP per tick).");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 4. Resistance & Partial Resists
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Spell Resistance Mechanics:");
        WowCheckbox("Enable Partial Resists (Classic 4-Roll Table)", &mechanics.partial_resists_enabled);

        ImGui::Spacing();
        ImGui::Separator();

        // 5. Improved Shadow Bolt
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Improved Shadow Bolt (ISB):");
        ImGui::TextDisabled("ISB operates as a 12-second debuff window on critical strikes.");

        ImGui::Spacing();
        ImGui::Separator();

        // 6. Pet Stat Scaling & Mana Management
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Pet Stat Inheritance & Mana Management:");
        WowCheckbox("Enable Pet Spell Power / AP Scaling (Forever)", &mechanics.pet_scaling);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Forever): Summoned pets inherit master's Spell Power to their spells and Attack Power.\nWhen OFF (Classic 1.12): Pets deal flat base ability damage.");
        }
        if (mechanics.pet_scaling) {
            ImGui::Indent(12.0f);
            float sp_pct = static_cast<float>(mechanics.pet_sp_ratio * 100.0);
            if (WowSliderFloat("Pet SP Scaling (%)", &sp_pct, 0.0f, 100.0f, "%.1f %%")) {
                mechanics.pet_sp_ratio = static_cast<double>(sp_pct) / 100.0;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Percentage of master's Spell Power inherited by demon spells (e.g. Imp Firebolt, Succubus Lash of Pain). Default: 10.0%% (10 SP = 1 Pet SP)");
            }
            float ap_pct = static_cast<float>(mechanics.pet_ap_ratio * 100.0);
            if (WowSliderFloat("Pet AP Scaling (%)", &ap_pct, 0.0f, 100.0f, "%.1f %%")) {
                mechanics.pet_ap_ratio = static_cast<double>(ap_pct) / 100.0;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Percentage of master's Spell Power converted to demon Attack Power for physical melee attacks (e.g. Succubus melee). Default: 16.7%% (6 SP = 1 Pet AP)");
            }
            ImGui::Unindent(12.0f);
        }
        WowCheckbox("Enable Pet Mana Tracking & Spell Costs", &mechanics.pet_mana_management);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON: Imp (1,150 base mana) and Succubus (1,450 base mana) consume mana on casts and regen mana passively (MP5), via raid buffs (Blessing of Wisdom, Judgement of Wisdom), and Demonic Energies talent.\nWhen OFF: Pets have infinite mana.");
        }
        WowCheckbox("Modern Imp Firebolt (44 Base + 15% Pet SP, 2.0s cast)", &mechanics.imp_firebolt_modern_scaling);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When ON (Default - Modern/Custom): Firebolt deals 44 base fire damage + 15%% pet SP inheritance (2.0/3.5 coefficient) and a 2.0s cast time.\nWhen OFF (Classic 1.12): Firebolt deals 85-98 flat base damage with 15%% pet SP inheritance (1.5/3.5 coefficient) and a 1.5s cast time.");
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 7. Projectile Travel Time
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.95f, 1.0f), "Missile Physics & Travel Time:");
        WowCheckbox("Simulate Projectile Travel Time", &mechanics.projectile_travel_time);
        if (mechanics.projectile_travel_time) {
            WowSliderFloat("Boss Distance (yd)", (float*)&mechanics.default_boss_distance_yards, 10.0f, 40.0f, "%.0f yd");
        }

        ImGui::Unindent(8.0f);
    }
}

} // namespace warlock
