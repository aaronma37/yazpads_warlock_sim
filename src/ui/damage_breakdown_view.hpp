#pragma once
#include "imgui.h"
#include "src/sim/parallel_runner.hpp"
#include "src/sim/spells.hpp"
#include <cstdio>
#include <vector>
#include <algorithm>

namespace warlock {

struct DamageBreakdownEntry {
    const char* name;
    double pct;
    ImVec4 color;
    SpellID id;
    bool is_pet;
};

inline ImVec4 get_spell_breakdown_color(SpellID id) {
    switch (id) {
        // Shadow Direct / DoT
        case SpellID::SHADOW_BOLT:      return ImVec4(0.55f, 0.35f, 0.95f, 1.0f);
        case SpellID::CORRUPTION:       return ImVec4(0.35f, 0.75f, 0.95f, 1.0f);
        case SpellID::CURSE_OF_AGONY:   return ImVec4(0.65f, 0.65f, 0.85f, 1.0f);
        case SpellID::CURSE_OF_DOOM:    return ImVec4(0.95f, 0.65f, 0.20f, 1.0f);
        case SpellID::BANE_OF_HAVOC:    return ImVec4(0.85f, 0.40f, 0.95f, 1.0f);
        case SpellID::SIPHON_LIFE:      return ImVec4(0.40f, 0.90f, 0.60f, 1.0f);
        case SpellID::SHADOWBURN:       return ImVec4(0.75f, 0.25f, 0.85f, 1.0f);
        case SpellID::DRAIN_HOPE:       return ImVec4(0.35f, 0.90f, 0.65f, 1.0f);
        case SpellID::DRAIN_LIFE:       return ImVec4(0.25f, 0.90f, 0.45f, 1.0f);
        case SpellID::DRAIN_SOUL:       return ImVec4(0.55f, 0.45f, 0.95f, 1.0f);
        case SpellID::TOUCH_OF_THE_GRAVE: return ImVec4(0.70f, 0.90f, 0.60f, 1.0f);

        // Fire
        case SpellID::IMMOLATE:         return ImVec4(1.00f, 0.50f, 0.20f, 1.0f);
        case SpellID::CONFLAGRATE:      return ImVec4(1.00f, 0.60f, 0.15f, 1.0f);
        case SpellID::INCINERATE:       return ImVec4(1.00f, 0.40f, 0.10f, 1.0f);
        case SpellID::SEARING_PAIN:     return ImVec4(1.00f, 0.55f, 0.15f, 1.0f);
        case SpellID::SOUL_FIRE:        return ImVec4(1.00f, 0.20f, 0.10f, 1.0f);

        // Pet
        case SpellID::PET_FIREBOLT:     return ImVec4(1.00f, 0.60f, 0.20f, 1.0f);
        case SpellID::PET_LASH_OF_PAIN: return ImVec4(0.70f, 0.30f, 0.90f, 1.0f);
        case SpellID::PET_MELEE:        return ImVec4(0.80f, 0.80f, 0.80f, 1.0f);

        default:                        return ImVec4(0.60f, 0.60f, 0.70f, 1.0f);
    }
}

inline void render_damage_breakdown_bars(const BatchSimResult& batch, float bar_width = 180.0f, float label_offset = 130.0f) {
    // Single shared source of all damage breakdown entries
    const DamageBreakdownEntry all_entries[] = {
        {"Shadow Bolt",    batch.pct_shadow_bolt,        get_spell_breakdown_color(SpellID::SHADOW_BOLT),        SpellID::SHADOW_BOLT,        false},
        {"Incinerate",     batch.pct_incinerate,         get_spell_breakdown_color(SpellID::INCINERATE),         SpellID::INCINERATE,         false},
        {"Searing Pain",   batch.pct_searing_pain,       get_spell_breakdown_color(SpellID::SEARING_PAIN),       SpellID::SEARING_PAIN,       false},
        {"Conflagrate",    batch.pct_conflagrate,        get_spell_breakdown_color(SpellID::CONFLAGRATE),        SpellID::CONFLAGRATE,        false},
        {"Shadowburn",     batch.pct_shadowburn,         get_spell_breakdown_color(SpellID::SHADOWBURN),         SpellID::SHADOWBURN,         false},
        {"Corruption",     batch.pct_corruption,         get_spell_breakdown_color(SpellID::CORRUPTION),         SpellID::CORRUPTION,         false},
        {"Immolate",       batch.pct_immolate,           get_spell_breakdown_color(SpellID::IMMOLATE),           SpellID::IMMOLATE,           false},
        {"Bane of Agony",  batch.pct_agony,              get_spell_breakdown_color(SpellID::CURSE_OF_AGONY),     SpellID::CURSE_OF_AGONY,     false},
        {"Curse of Doom",  batch.pct_doom,               get_spell_breakdown_color(SpellID::CURSE_OF_DOOM),      SpellID::CURSE_OF_DOOM,      false},
        {"Bane of Havoc",  batch.pct_bane_of_havoc,      get_spell_breakdown_color(SpellID::BANE_OF_HAVOC),      SpellID::BANE_OF_HAVOC,      false},
        {"Siphon Life",    batch.pct_siphon_life,        get_spell_breakdown_color(SpellID::SIPHON_LIFE),        SpellID::SIPHON_LIFE,        false},
        {"Soul Fire",      batch.pct_soul_fire,          get_spell_breakdown_color(SpellID::SOUL_FIRE),          SpellID::SOUL_FIRE,          false},
        {"Wrack",          batch.pct_drain_hope,         get_spell_breakdown_color(SpellID::DRAIN_HOPE),         SpellID::DRAIN_HOPE,         false},
        {"Drain Life",     batch.pct_drain_life,         get_spell_breakdown_color(SpellID::DRAIN_LIFE),         SpellID::DRAIN_LIFE,         false},
        {"Drain Soul",     batch.pct_drain_soul,         get_spell_breakdown_color(SpellID::DRAIN_SOUL),         SpellID::DRAIN_SOUL,         false},
        {"Imp (Firebolt)", batch.pct_pet_firebolt,       get_spell_breakdown_color(SpellID::PET_FIREBOLT),       SpellID::PET_FIREBOLT,       true},
        {"Succubus (Lash)",batch.pct_pet_lash_of_pain,   get_spell_breakdown_color(SpellID::PET_LASH_OF_PAIN),   SpellID::PET_LASH_OF_PAIN,   true},
        {"Succubus (Melee)",batch.pct_pet_melee,         get_spell_breakdown_color(SpellID::PET_MELEE),          SpellID::PET_MELEE,          true},
        {"Demonic Brand",  batch.pct_demonic_brand,      ImVec4(0.90f, 0.40f, 0.80f, 1.0f),                     SpellID::NONE,               true},
        {"Touch of Grave", batch.pct_touch_of_the_grave, get_spell_breakdown_color(SpellID::TOUCH_OF_THE_GRAVE), SpellID::TOUCH_OF_THE_GRAVE, false},
    };

    auto draw_dmg_bar = [&](const char* name, double pct, const ImVec4& col, SpellID id) {
        if (pct > 0.05) {
            ImGui::Text("%-14s:", name);
            ImGui::SameLine(label_offset);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f%% (%.0f)", pct, pct * 0.01 * batch.mean_dps);
            ImGui::ProgressBar(static_cast<float>(pct / 100.0), ImVec2(bar_width, 15), buf);
            ImGui::PopStyleColor();

            if (id != SpellID::NONE && ImGui::IsItemHovered()) {
                const BatchSpellStats& st = batch.spell_stats[static_cast<size_t>(id)];
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", name);
                ImGui::Separator();
                ImGui::Text("Avg casts: %.1f | Avg hits: %.1f | Avg hit: %.0f", st.mean_casts, st.mean_hits, spell_avg_hit(st));
                ImGui::Text("Crit: %.1f%% | Miss: %.1f%%", spell_crit_pct(st), spell_miss_pct(st));
                ImGui::EndTooltip();
            }
        }
    };

    for (const auto& entry : all_entries) {
        draw_dmg_bar(entry.name, entry.pct, entry.color, entry.id);
    }

    if (batch.pct_pet > 0.05) {
        char pet_summary[64];
        snprintf(pet_summary, sizeof(pet_summary), "Total Pet: %.1f DPS (%.1f%%)", batch.mean_pet_dps, batch.pct_pet);
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "%s", pet_summary);
    }
}

} // namespace warlock
