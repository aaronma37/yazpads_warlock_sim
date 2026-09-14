#pragma once
#include "imgui.h"
#include "asset_manager.hpp"
#include "src/sim/spells.hpp"
#include "src/sim/talents.hpp"
#include <vector>
#include <string>

namespace warlock {

struct SpellBookEntry {
    SpellID id;
    std::string rank_name;
    std::string school_str;
    std::string base_cast_str;
    std::string base_mana_str;
    std::string direct_dmg_str;
    std::string dot_dmg_str;
    std::string coeff_str;
    std::string sim_formula;
    std::string details_explanation;
    std::string icon_name;
};

inline const std::vector<SpellBookEntry>& get_all_spellbook_entries() {
    static const std::vector<SpellBookEntry> entries = {
        {
            SpellID::SHADOW_BOLT,
            "Shadow Bolt (Rank 10)",
            "Shadow",
            "3.0s (2.5s with 5/5 Bane)",
            "380 Mana",
            "482 - 538 Direct Damage",
            "None",
            "85.71% Direct (3.0 / 3.5)",
            "Damage = Roll(482, 538) + (SpellPower + ShadowSpellPower) * (3.0 / 3.5) * Multipliers",
            "Primary single-target shadow filler. Applies Improved Shadow Bolt debuff (+20% shadow damage) when it crits (WoW Forever: permanent aura, classic: 4 charges). Scaled by Shadow Mastery (+10%), Demonic Sacrifice: Succubus (+15%), Darkness (+10%), ISB (+20%), CoS (+10%), Shadow Weaving (+15%).",
            "spell_shadow_shadowbolt"
        },
        {
            SpellID::CORRUPTION,
            "Corruption (Rank 7)",
            "Shadow",
            "2.0s (Instant with 5/5 Imp Corruption)",
            "290 Mana",
            "None (or Initial Hit with Empowered Corruption)",
            "822 Base DoT over 18s (6 ticks of 137.0 every 3s)",
            "100.0% DoT (16.67% per tick)",
            "Tick Damage = (822.0 / 6.0 + (SpellPower + ShadowSpellPower) / 6.0) * Multipliers",
            "Baseline primary Shadow DoT. Can trigger Nightfall (4% chance per tick to grant instant cast Shadow Bolt). In WoW Forever, empowered variants allow DoT ticks to crit with Ruin +100% crit bonus.",
            "spell_shadow_abominationexplosion"
        },
        {
            SpellID::IMMOLATE,
            "Immolate (Rank 8)",
            "Fire",
            "2.0s (1.5s with 5/5 Bane)",
            "380 Mana",
            "258 - 306 Initial Direct Damage",
            "485 Base DoT over 15s (5 ticks of 97.0 every 3s)",
            "20.0% Direct, 65.0% DoT (13.0% per tick)",
            "Direct = Roll(258, 306) + SP * 0.20 | Tick = 97.0 + SP * 0.13",
            "Hybrid fire direct/DoT spell. Prerequisite for Conflagrate consumption. Scaled by Demonic Sacrifice: Imp (+15%), Devastation (+5% crit), Fire and Brimstone (+10%), CoE (+10%), and Molten Core.",
            "spell_fire_immolation"
        },
        {
            SpellID::CURSE_OF_AGONY,
            "Bane of Agony (Rank 6)",
            "Shadow",
            "Instant (1.5s GCD)",
            "215 Mana",
            "None",
            "1044 Base DoT over 24s (12 ticks every 2s, ramping)",
            "100.0% DoT (8.33% per tick average)",
            "Tick Damage = (1044.0 / 12.0 + (SpellPower + ShadowSpellPower) / 12.0) * Multipliers",
            "Bane slot DoT ramping damage. Can be used concurrently alongside target utility curses (CoS / CoE) and benefits from Pandemic crits.",
            "spell_shadow_curseofsargeras"
        },
        {
            SpellID::CURSE_OF_DOOM,
            "Curse of Doom (Rank 1)",
            "Shadow",
            "Instant (1.5s GCD, 60s CD)",
            "300 Mana",
            "None",
            "3200 Base Shadow Damage after 60s",
            "200.0% Coefficient",
            "Damage = (3200.0 + (SpellPower + ShadowSpellPower) * 2.0) * Multipliers",
            "Massive delayed single-hit shadow curse. Best for long uninterrupted encounters (>= 60s).",
            "spell_shadow_auraofdarkness"
        },
        {
            SpellID::CURSE_OF_SHADOWS,
            "Curse of Shadows (Rank 2)",
            "Shadow",
            "Instant (1.5s GCD)",
            "175 Mana",
            "None (Debuff)",
            "None",
            "Binary Spell (No partial resists)",
            "Target Modifier: +10% Shadow & Arcane damage taken, -75 Shadow/Arcane resistance",
            "Raid utility curse maximizing party Shadow Bolt / Affliction and Arcane damage.",
            "spell_shadow_curseofachimonde"
        },
        {
            SpellID::CURSE_OF_ELEMENTS,
            "Curse of Elements (Rank 2)",
            "Shadow",
            "Instant (1.5s GCD)",
            "175 Mana",
            "None (Debuff)",
            "None",
            "Binary Spell (No partial resists)",
            "Target Modifier: +10% Fire & Frost damage taken, -75 Fire/Frost resistance",
            "Raid utility curse maximizing Fire Destro (Incinerate, Conflagrate, Immolate) and Mage fire damage.",
            "spell_shadow_chilltouch"
        },
        {
            SpellID::SHADOWBURN,
            "Shadowburn (Rank 6)",
            "Shadow",
            "Instant (1.5s GCD, 8s CD, 1 Soul Shard)",
            "365 Mana",
            "450 - 502 Direct Damage",
            "None",
            "42.86% Direct (1.5 / 3.5)",
            "Damage = Roll(450, 502) + (SpellPower + ShadowSpellPower) * (1.5 / 3.5) * Multipliers",
            "Instant shadow burst spell. Excellent on-the-move finisher or burst weave.",
            "spell_shadow_scourgebuild"
        },
        {
            SpellID::CONFLAGRATE,
            "Conflagrate (Rank 4)",
            "Fire",
            "Instant (1.5s GCD, 10s CD)",
            "265 Mana",
            "578 - 704 Direct Damage",
            "Consumes active Immolate on target",
            "42.86% Direct (1.5 / 3.5)",
            "Damage = Roll(578, 704) + (SpellPower + FireSpellPower) * (1.5 / 3.5) * Multipliers",
            "31-point Destruction capstone. Consumes Immolate to deal immediate fire burst damage. Reapplied immediately in optimal Fire rotations.",
            "spell_fire_fireball"
        },
        {
            SpellID::INCINERATE,
            "Incinerate (Rank 1)",
            "Fire",
            "2.5s (2.0s with 5/5 Bane)",
            "355 Mana",
            "445 - 515 Direct Damage (+25% bonus vs Immolated target)",
            "None",
            "71.43% Direct (2.5 / 3.5)",
            "Damage = (Roll(445, 515) + SP * (2.5 / 3.5)) * (Immolate_Active ? 1.25 : 1.0) * Multipliers",
            "Primary Fire Destruction filler spell. Deals 25% extra damage if target is afflicted with Immolate.",
            "spell_fire_burnout"
        },
        {
            SpellID::SOUL_FIRE,
            "Soul Fire (Rank 5)",
            "Fire",
            "4.0s (2.0s with Bane/Decimation, 60s CD base / 6s CD Decimation)",
            "335 Mana",
            "715 - 895 Direct Damage",
            "None",
            "100.0% Direct",
            "Damage = Roll(715, 895) + (SpellPower + FireSpellPower) * 1.0 * Multipliers",
            "Massive fire nuke. In Demonology specs with Decimation talent, cast time and CD are drastically reduced below 35% boss HP (Execute Phase).",
            "spell_fire_fireball02"
        },
        {
            SpellID::DRAIN_HOPE,
            "Drain Hope (Rank 1)",
            "Shadow",
            "Channeled 6.0s (1.0s ticks, 20s CD)",
            "240 Mana",
            "None",
            "312 Total Shadow Damage (6 ticks of 52.0 every 1s)",
            "100.0% DoT (16.6% per tick)",
            "Tick Damage = (52.0 + (SpellPower + ShadowSpellPower) * 0.166) * Multipliers",
            "WoW Forever Deep Affliction channeled execute / resource-draining mechanism.",
            "spell_shadow_lifedrain02"
        },
        {
            SpellID::DRAIN_SOUL,
            "Drain Soul (Rank 4)",
            "Shadow",
            "Channeled 15.0s (3.0s ticks)",
            "290 Mana",
            "None",
            "455 Total Shadow Damage (5 ticks of 91.0 every 3s)",
            "100.0% DoT (20.0% per tick)",
            "Tick Damage = (91.0 + (SpellPower + ShadowSpellPower) * 0.20) * Multipliers",
            "Channeled Shadow filler for Deep Affliction. Benefits from Improved Drains (+18%, tripled below 20% HP), Soul Siphon (+50% tick rate), and triggers Nightfall procs.",
            "spell_shadow_soulgem"
        },
        {
            SpellID::SEARING_PAIN,
            "Searing Pain (Rank 6)",
            "Fire",
            "1.5s Cast Time",
            "168 Mana",
            "204 - 240 Direct Damage",
            "None",
            "42.86% Direct (1.5 / 3.5)",
            "Damage = Roll(204, 240) + (SpellPower + FireSpellPower) * (1.5 / 3.5) * Multipliers",
            "Fast 1.5s fire direct damage spell. Features high base threat multiplier (not harmful in DPS sim).",
            "spell_fire_soulburn"
        },
        {
            SpellID::LIFE_TAP,
            "Life Tap (Rank 6)",
            "Shadow",
            "Instant (1.5s GCD)",
            "0 Mana (Cost: 580 Health)",
            "580 Base Mana Restored",
            "None",
            "80.0% Spell Power scaling into Mana returned",
            "Mana Returned = 580 + (SpellPower * 0.80) * (1.0 + 0.10 * Imp_Life_Tap_Points)",
            "Essential resource generation mechanic. Restores mana at the expense of player health.",
            "spell_shadow_burningspirit"
        }
    };
    return entries;
}

inline void render_panel_spellbook() {
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Warlock Spellbook & Base Spell Data");
    ImGui::TextDisabled("Base stats, ranks, spell power scaling coefficients, and discrete event simulation formulas.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    static char search_filter[64] = "";
    ImGui::SetNextItemWidth(300);
    ImGui::InputTextWithHint("##SpellSearch", "Search Spells (e.g. Shadow Bolt, Fire)...", search_filter, sizeof(search_filter));
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        search_filter[0] = '\0';
    }

    ImGui::Spacing();

    const auto& entries = get_all_spellbook_entries();

    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("SpellbookTable", 7, flags, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("Spell / Rank", ImGuiTableColumnFlags_WidthFixed, 220.0f);
        ImGui::TableSetupColumn("School", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Cast Time", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Mana Cost", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Base Damage / Effect", ImGuiTableColumnFlags_WidthFixed, 260.0f);
        ImGui::TableSetupColumn("SP Coefficient", ImGuiTableColumnFlags_WidthFixed, 160.0f);
        ImGui::TableSetupColumn("Simulation Formula & Multipliers", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        std::string query = search_filter;
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        for (const auto& sp : entries) {
            std::string name_lower = sp.rank_name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            std::string school_lower = sp.school_str;
            std::transform(school_lower.begin(), school_lower.end(), school_lower.begin(), ::tolower);

            if (!query.empty() && name_lower.find(query) == std::string::npos && school_lower.find(query) == std::string::npos) {
                continue;
            }

            ImGui::TableNextRow();

            // Col 0: Icon + Name
            ImGui::TableSetColumnIndex(0);
            Texture2D icon = AssetManager::get().get_icon(sp.icon_name);
            ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(20, 20));
            ImGui::SameLine(0, 6);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.70f, 1.0f), "%s", sp.rank_name.c_str());

            // Col 1: School
            ImGui::TableSetColumnIndex(1);
            if (sp.school_str == "Shadow") {
                ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "Shadow");
            } else if (sp.school_str == "Fire") {
                ImGui::TextColored(ImVec4(1.0f, 0.50f, 0.20f, 1.0f), "Fire");
            } else {
                ImGui::TextColored(ImVec4(0.80f, 0.80f, 0.80f, 1.0f), "%s", sp.school_str.c_str());
            }

            // Col 2: Cast Time
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", sp.base_cast_str.c_str());

            // Col 3: Mana Cost
            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.0f, 1.0f), "%s", sp.base_mana_str.c_str());

            // Col 4: Base Damage
            ImGui::TableSetColumnIndex(4);
            if (sp.direct_dmg_str != "None") {
                ImGui::Text("%s", sp.direct_dmg_str.c_str());
            }
            if (sp.dot_dmg_str != "None") {
                ImGui::TextColored(ImVec4(0.50f, 0.90f, 0.50f, 1.0f), "%s", sp.dot_dmg_str.c_str());
            }

            // Col 5: SP Coefficient
            ImGui::TableSetColumnIndex(5);
            ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.40f, 1.0f), "%s", sp.coeff_str.c_str());

            // Col 6: Formula & Notes
            ImGui::TableSetColumnIndex(6);
            ImGui::TextColored(ImVec4(0.40f, 0.90f, 1.0f, 1.0f), "%s", sp.sim_formula.c_str());
            ImGui::TextDisabled("%s", sp.details_explanation.c_str());
        }

        ImGui::EndTable();
    }
}

} // namespace warlock
