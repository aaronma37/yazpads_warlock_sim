#pragma once
#include "imgui.h"
#include "wow_widgets.hpp"
#include "ui_theme.hpp"

namespace warlock {

inline void render_panel_changelog() {
    BeginWowChild("ChangelogMainPane", ImVec2(0, 0), true);

    ImGui::Spacing();
    ImGui::Indent(10.0f);

    // Main Header
    ImGui::TextColored(wow_colors::Gold, "Project Changelog & Research Investigation");
    ImGui::TextColored(wow_colors::GrayMuted, "Record of engine updates, research audits, and empirical discrepancies from discord research.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Changelog Entry 1: Investigate diffs from discord and engine
    if (WowCollapsingHeader("Investigate diffs from discord and engine", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        ImGui::TextColored(wow_colors::YellowHighlight, "Investigation Summary:");
        ImGui::TextWrapped(
            "Audited all 44 research threads from the #forever-research archive (discord_data/1548410962464481280) "
            "against the simulator codebase (src/sim/warlock/spells.hpp, warlock_sim.cpp, combat_mechanics.hpp). "
            "Below is the complete enumeration of identified discrepancies, empirical testing findings, and engine alignment notes."
        );
        ImGui::Spacing();

        // High Level Table Comparison
        if (ImGui::BeginTable("DiffsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("System / Spell", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Engine Value", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Discord / Forever Beta", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Status / Finding", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("Notes & Impact", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            // Row 1: Drain Soul Coeff
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Drain Soul (Coeff)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "100%% SP (20%% / tick)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "50%% SP (10%% / tick)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::OrangeWarning, "Discrepancy");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Discord pinned coefficient table confirms 50%% total SP coefficient (0.10 per tick across 5 ticks).");

            // Row 2: Wrack Base & Coeff
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Wrack (Base & Coeff)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "212 Base (35.3/t), 100%% SP");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "216 Base (36/t), 85.8%% SP");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::OrangeWarning, "Discrepancy");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Discord pinned table indicates 216 total base damage (36 per tick) and 0.858 total coefficient (0.143 per tick).");

            // Row 3: Drain Life Base
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Drain Life (Base Dmg)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "355 Base (71 / tick)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "255 Base (51 / tick)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::OrangeWarning, "Discrepancy");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Forever client nerfed base damage to 255 total (51 per tick across 5s). SP coefficient matches at 50%% (10%%/tick).");

            // Row 4: Shadowburn Base
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Shadowburn (Base Dmg)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "238 - 268 (Avg 253)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "259 - 289 (Avg 274)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::OrangeWarning, "Discrepancy");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Discord pinned coefficient table records Rank 6 base damage as 259-289.");

            // Row 5: Touch of the Grave (ICD & Ticks)
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Touch of the Grave");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "No ICD, Procs on DoT ticks, +Shadow%%");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "1.0s ICD, Casts only, Flat 5%% HP");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::OrangeWarning, "Discrepancy");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Extensive testing confirmed 1s ICD, no procs on periodic ticks, and flat 5%% Max HP without Shadow multiplier scaling.");

            // Row 6: Spell Hit Cap
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Spell Hit Cap");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "16%% Cap (1%% hard miss floor)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "17%% Cap (0%% true miss floor)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::OrangeWarning, "Discrepancy");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Forever client character sheet and empirical testing confirm 17%% hit reaches 0%% miss chance on level 63 boss targets.");

            // Row 7: Wand Scaling
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Wand SP Scaling");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "No SP Scaling");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "SP * (Speed / 5.0)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::OrangeWarning, "Discrepancy");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Wands scale from generic Spell Power at ~20%% SP per second of attack speed (Speed / 5.0 to Speed / 5.25).");

            // Row 8: DoT Dynamic Recalculation (No Snapshotting)
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "DoT Dynamic Scaling");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Dynamic (snapshot_dots=false)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Dynamic (No snapshotting)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Aligned");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Engine correctly implements dynamic per-tick SP recalculation by default (matching Forever Beta findings where buffs dynamically affect active DoTs).");

            // Row 9: Drain Soul Cast Bug
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Drain Soul Double Cast");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "Channel breaks on cast");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Channels while casting SB");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GrayMuted, "Will Not Fix");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Beta client bug: Drain Soul continues ticking while casting a secondary spell until that cast finishes. Marked Will Not Fix as it is an unintended client bug.");

            // Row 10: Spell Piercing Below 0
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Spell Piercing");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::RedDebuff, "Resistance floor at 0");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Reduces below 0 (+Dmg)");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::OrangeWarning, "Discrepancy");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Spell Piercing (Penetration) lowers creature resistance below 0, granting +0.55%% - 0.60%% damage per piercing point on non-bosses.");

            // Row 11: Ruin on Immolate Ticks
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Ruin on Immolate DoT");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Destruction Spells 200%%");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Applies to Immolate DoT Crits");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Aligned");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Ruin talent applies to all Destruction spell damage, including periodic Immolate crits.");

            // Row 12: Life Tap Scaling
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::White, "Life Tap Scaling");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "100%% Spirit Scaling");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "100%% Spirit Scaling");
            ImGui::TableNextColumn(); ImGui::TextColored(wow_colors::GreenBuff, "Aligned");
            ImGui::TableNextColumn(); ImGui::TextWrapped("Engine correctly implements 1.0 * Spirit scaling (0%% SP) matching Forever Beta mechanics.");

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Detailed Section 1: Spell Coefficients & Base Damage
        if (WowCollapsingHeader("1. Spell Coefficients & Base Damage Breakdown", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Drain Soul: Discord pinned table states total coefficient is 0.50 (0.10/tick). Engine uses 0.20/tick (1.0 total).");
            ImGui::BulletText("Wrack: Discord pinned table states 216 total base dmg (36/tick) and 0.858 total coefficient (0.143/tick). Engine uses 212 base (35.33/tick) and 1.0 total coefficient (0.1667/tick).");
            ImGui::BulletText("Drain Life: Discord pinned table states 255 base dmg (51/tick) with 0.50 total coefficient (0.10/tick). Engine uses 355 base (71/tick).");
            ImGui::BulletText("Shadowburn: Discord pinned table states base damage 259-289 (avg 274). Engine uses 238-268 (avg 253).");
            ImGui::BulletText("Bane of Doom: Confirmed 1742 base dmg with 4.0 (400%%) SP coefficient across 60s (Engine already aligned).");
            ImGui::BulletText("Bane of Agony: Confirmed 552 base dmg (46/tick across 12 ticks) with 1.6 (160%%) SP coefficient (Engine already aligned).");
            ImGui::BulletText("Corruption: Confirmed 438 base dmg (73/tick across 6 ticks) with 1.2 (120%%) SP coefficient (Engine already aligned).");
            ImGui::Spacing();
        }

        // Detailed Section 2: Touch of the Grave
        if (WowCollapsingHeader("2. Touch of the Grave (Undead Racial)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Proc Chance: 10%% for Casters (Warlock/Mage/Priest), 5%% for Melee/Physical.");
            ImGui::BulletText("Internal Cooldown (ICD): Confirmed 1.0 second ICD.");
            ImGui::BulletText("Trigger Criteria: Procs strictly on direct spell casts and initial DoT/Curse applications (including non-damaging spells like CoW/CoE). Does NOT proc on DoT periodic ticks.");
            ImGui::BulletText("Damage & Multipliers: Deals flat 5%% of caster's Max Health. Does not scale with Spell Power, nor is it increased by Shadow damage multipliers or ISB on target.");
            ImGui::BulletText("Engine Comparison: Current engine procs on every DoT tick, lacks a 1s ICD check, and multiplies by Shadow damage multipliers.");
            ImGui::Spacing();
        }

        // Detailed Section 3: Combat Mechanics & Hit Floor
        if (WowCollapsingHeader("3. Combat Mechanics, Spell Hit & Resistances", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Spell Hit Cap: Forever character sheet shows 17%% spell hit to reach 0%% miss on +3 level raid targets (no 1%% hard miss floor as in Classic).");
            ImGui::BulletText("DoT Miss Checks: Hit roll is evaluated exclusively on spell cast / application; DoT ticks cannot miss once successfully applied.");
            ImGui::BulletText("Spell Piercing: Can reduce target resistance below 0, granting +0.55%% - 0.60%% damage per point on non-boss targets.");
            ImGui::Spacing();
        }

        // Detailed Section 4: Wand & Channeled Quirks
        if (WowCollapsingHeader("4. Wand Mechanics & Beta Quirks", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Wand Scaling: Verified wands scale with Spell Power based on attack speed: SP Coeff = Wand_Speed / 5.0 (or Speed / 5.25), utilizing generic Spell Power.");
            ImGui::BulletText("DoT Dynamic Scaling: Confirmed that DoT ticks recalculate damage dynamically with active buffs (e.g. Eureka, trinket procs) rather than snapshotting on cast. Engine is already aligned with this (mechanics.snapshot_dots = false by default).");
            ImGui::BulletText("Drain Soul Bug: Drain Soul continues channeling and ticking while casting secondary spells (e.g. Shadow Bolt) until the second cast finishes.");
            ImGui::Spacing();
        }
    }

    ImGui::Unindent(10.0f);
    EndWowChild();
}

} // namespace warlock
