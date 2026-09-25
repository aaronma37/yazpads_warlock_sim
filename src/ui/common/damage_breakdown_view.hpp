#pragma once
#include "imgui.h"
#include "src/sim/parallel_runner.hpp"
#include "src/sim/spells.hpp"
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>

// 2D Swirling Billowing Smoke Shader Simulation
inline float sample_smoke_density(float x, float y, float time_sec) {
    float u = x * 0.035f;
    float v = y * 0.160f;

    // Drifting vortex domain warping (fluid swirl)
    float warp_x = std::sin(v * 2.2f + time_sec * .15f) * 0.70f;
    float warp_y = std::cos(u * 1.6f - time_sec * .1f) * 0.50f;

    float px = u + warp_x - time_sec * 0.085f;
    float py = v + warp_y;

    // Multi-octave turbulence
    float o1 = std::sin(px * 1.3f + py * 0.9f);
    float o2 = std::sin(px * 2.7f - py * 1.8f + time_sec * 0.8f) * 0.5f;
    float o3 = std::sin(px * 5.2f + py * 3.4f - time_sec * 1.4f) * 0.25f;

    float raw_val = (o1 + o2 + o3) / 1.75f; // [-1.0, 1.0]
    float density = 0.5f + 0.5f * raw_val; // [0.0, 1.0]

    return std::pow(density, 1.5f);
}

inline void draw_shimmer_noise_overlay(ImDrawList* draw_list, ImVec2 f0, ImVec2 f1, float rounding) {
    float fill_w = f1.x - f0.x;
    if (fill_w <= 0.5f) return;

    draw_list->PushClipRect(f0, f1, true);

    float time_sec = static_cast<float>(ImGui::GetTime());
    const float step = 3.5f;

    for (float x = f0.x; x < f1.x; x += step) {
        float x_next = std::min(f1.x, x + step);

        float d_tl = sample_smoke_density(x, f0.y, time_sec);
        float d_tr = sample_smoke_density(x_next, f0.y, time_sec);
        float d_bl = sample_smoke_density(x, f1.y, time_sec);
        float d_br = sample_smoke_density(x_next, f1.y, time_sec);

        // 1. Dark smoke shadow troughs
        ImU32 shadow_tl = IM_COL32(0, 0, 0, static_cast<int>((1.0f - d_tl) * 65.0f));
        ImU32 shadow_tr = IM_COL32(0, 0, 0, static_cast<int>((1.0f - d_tr) * 65.0f));
        ImU32 shadow_bl = IM_COL32(0, 0, 0, static_cast<int>((1.0f - d_bl) * 65.0f));
        ImU32 shadow_br = IM_COL32(0, 0, 0, static_cast<int>((1.0f - d_br) * 65.0f));

        draw_list->AddRectFilledMultiColor(
            ImVec2(x, f0.y),
            ImVec2(x_next, f1.y),
            shadow_tl, shadow_tr, shadow_br, shadow_bl
        );

        // 2. Bright glowing billowing smoke wisps
        ImU32 smoke_tl = IM_COL32(255, 255, 255, static_cast<int>(d_tl * 105.0f));
        ImU32 smoke_tr = IM_COL32(255, 255, 255, static_cast<int>(d_tr * 105.0f));
        ImU32 smoke_bl = IM_COL32(255, 255, 255, static_cast<int>(d_bl * 85.0f));
        ImU32 smoke_br = IM_COL32(255, 255, 255, static_cast<int>(d_br * 85.0f));

        draw_list->AddRectFilledMultiColor(
            ImVec2(x, f0.y),
            ImVec2(x_next, f1.y),
            smoke_tl, smoke_tr, smoke_br, smoke_bl
        );
    }

    // Top specular highlight line
    draw_list->AddLine(ImVec2(f0.x, f0.y + 1.0f), ImVec2(f1.x, f0.y + 1.0f), IM_COL32(255, 255, 255, 80), 1.0f);

    draw_list->PopClipRect();
}

// Custom animated progress bar with scrolling smoke shader effect and rounded corners
inline void draw_shimmer_bar(
    ImDrawList* draw_list,
    ImVec2 p0,
    ImVec2 p1,
    float fill_fraction,
    const ImVec4& col,
    const char* text_overlay = nullptr,
    float rounding = 3.0f
) {
    float bar_w = p1.x - p0.x;
    float bar_h = p1.y - p0.y;
    if (bar_w <= 0.0f || bar_h <= 0.0f) return;

    // Dark inset trough background matching frame style
    draw_list->AddRectFilled(p0, p1, IM_COL32(18, 16, 22, 255), rounding);

    float fill_w = bar_w * std::clamp(fill_fraction, 0.0f, 1.0f);
    if (fill_w > 0.5f) {
        ImVec2 f1(p0.x + fill_w, p1.y);
        draw_list->PushClipRect(p0, f1, true);

        // Base color fill
        ImU32 base_u32 = ImGui::ColorConvertFloat4ToU32(col);
        draw_list->AddRectFilled(p0, p1, base_u32, rounding);

        // Procedural scrolling billowing smoke overlay
        draw_shimmer_noise_overlay(draw_list, p0, f1, rounding);

        draw_list->PopClipRect();
    }

    // Border with rounded corners
    draw_list->AddRect(p0, p1, IM_COL32(65, 65, 80, 200), rounding, 0, 1.0f);

    // Text overlay (centered)
    if (text_overlay && text_overlay[0] != '\0') {
        ImVec2 text_sz = ImGui::CalcTextSize(text_overlay);
        ImVec2 text_pos(p0.x + (bar_w - text_sz.x) * 0.5f, p0.y + (bar_h - text_sz.y) * 0.5f);
        draw_list->AddText(ImVec2(text_pos.x + 1.0f, text_pos.y + 1.0f), IM_COL32(0, 0, 0, 220), text_overlay);
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), text_overlay);
    }
}

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
        // Shadow Direct & DoTs - Distinct shades within shadow spectrum
        case SpellID::SHADOW_BOLT:        return ImVec4(0.55f, 0.35f, 0.95f, 1.0f); // Deep Shadow Purple
        case SpellID::CORRUPTION:         return ImVec4(0.40f, 0.65f, 0.95f, 1.0f); // Periwinkle
        case SpellID::CURSE_OF_AGONY:     return ImVec4(0.65f, 0.60f, 0.90f, 1.0f); // Slate Purple
        case SpellID::CURSE_OF_DOOM:      return ImVec4(0.92f, 0.35f, 0.85f, 1.0f); // Pinkish Purple / Magenta
        case SpellID::BANE_OF_HAVOC:      return ImVec4(0.85f, 0.40f, 0.95f, 1.0f); // Bright Neon Purple
        case SpellID::SIPHON_LIFE:        return ImVec4(0.35f, 0.85f, 0.70f, 1.0f); // Jade Violet
        case SpellID::SHADOWBURN:         return ImVec4(0.80f, 0.22f, 0.65f, 1.0f); // Crimson Violet
        case SpellID::DRAIN_HOPE:         return ImVec4(0.45f, 0.45f, 0.92f, 1.0f); // Indigo
        case SpellID::DRAIN_LIFE:         return ImVec4(0.30f, 0.80f, 0.55f, 1.0f); // Emerald Violet
        case SpellID::DRAIN_SOUL:         return ImVec4(0.65f, 0.45f, 0.95f, 1.0f); // Rich Soul Purple
        case SpellID::TOUCH_OF_THE_GRAVE: return ImVec4(0.72f, 0.60f, 0.92f, 1.0f); // Ghostly Lavender

        // Fire Spells - Distinct shades within fire spectrum
        case SpellID::IMMOLATE:           return ImVec4(1.00f, 0.52f, 0.18f, 1.0f); // Fiery Amber
        case SpellID::CONFLAGRATE:        return ImVec4(1.00f, 0.65f, 0.15f, 1.0f); // Bright Flare Orange
        case SpellID::INCINERATE:         return ImVec4(1.00f, 0.40f, 0.10f, 1.0f); // Red-Orange Blaze
        case SpellID::SEARING_PAIN:       return ImVec4(1.00f, 0.58f, 0.22f, 1.0f); // Golden Searing Light
        case SpellID::SOUL_FIRE:          return ImVec4(1.00f, 0.22f, 0.08f, 1.0f); // Deep Crimson Pyro

        // Pet Abilities - Distinct shades within emerald/fel green spectrum
        case SpellID::PET_FIREBOLT:       return ImVec4(0.20f, 0.85f, 0.42f, 1.0f); // Fel Emerald Green
        case SpellID::PET_LASH_OF_PAIN:   return ImVec4(0.32f, 0.90f, 0.60f, 1.0f); // Mint Lash Green
        case SpellID::PET_MELEE:          return ImVec4(0.45f, 0.82f, 0.45f, 1.0f); // Sage Melee Green

        default:                          return ImVec4(0.60f, 0.60f, 0.70f, 1.0f);
    }
}

inline void render_damage_breakdown_bars(const BatchSimResult& batch, float bar_width = 180.0f, float label_offset = 130.0f) {
    // 1. Composite 3-Section Horizontal Bar (Shadow, Fire, Pet)
    double shadow_pct = batch.pct_shadow_bolt + batch.pct_corruption + batch.pct_agony +
                        batch.pct_doom + batch.pct_bane_of_havoc + batch.pct_siphon_life +
                        batch.pct_shadowburn + batch.pct_drain_hope + batch.pct_drain_life +
                        batch.pct_drain_soul + batch.pct_touch_of_the_grave;
    double fire_pct = batch.pct_immolate + batch.pct_conflagrate + batch.pct_incinerate +
                      batch.pct_searing_pain + batch.pct_soul_fire;
    double pet_pct = batch.pct_pet;

    double total_pct = shadow_pct + fire_pct + pet_pct;
    double norm_shadow = 0.0, norm_fire = 0.0, norm_pet = 0.0;
    if (total_pct > 0.0) {
        norm_shadow = (shadow_pct / total_pct) * 100.0;
        norm_fire = (fire_pct / total_pct) * 100.0;
        norm_pet = (pet_pct / total_pct) * 100.0;
    }

    float avail_w = ImGui::GetContentRegionAvail().x;
    float full_bar_w = std::max(60.0f, avail_w);
    float bar_h = 15.0f;
    float rounding = 3.0f;
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + full_bar_w, p0.y + bar_h);

    ImGui::InvisibleButton("##CompositeDmgBarWarlock", ImVec2(full_bar_w, bar_h));
    bool is_bar_hovered = ImGui::IsItemHovered();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(p0, p1, IM_COL32(18, 16, 22, 255), rounding);

    float cur_bar_x = p0.x;
    float s_w = (float)(full_bar_w * (norm_shadow * 0.01));
    float f_w = (float)(full_bar_w * (norm_fire * 0.01));
    float p_w = (float)(full_bar_w * (norm_pet * 0.01));

    auto draw_segment = [&](float start_x, float seg_w, ImU32 col) {
        if (seg_w <= 0.5f) return;
        ImVec2 b0(start_x, p0.y);
        ImVec2 b1(std::min(p1.x, start_x + seg_w), p1.y);
        draw_list->PushClipRect(b0, b1, true);
        draw_list->AddRectFilled(p0, p1, col, rounding);
        draw_shimmer_noise_overlay(draw_list, b0, b1, rounding);
        draw_list->PopClipRect();
    };

    // Shadow segment (Purple)
    if (norm_shadow > 0.5) {
        draw_segment(cur_bar_x, s_w, IM_COL32(148, 65, 235, 240));
        cur_bar_x += s_w;
    }

    // Fire segment (Orange)
    if (norm_fire > 0.5) {
        draw_segment(cur_bar_x, f_w, IM_COL32(245, 115, 30, 240));
        cur_bar_x += f_w;
    }

    // Pet segment (Emerald Green)
    if (norm_pet > 0.5) {
        draw_segment(cur_bar_x, p_w, IM_COL32(40, 195, 90, 240));
    }

    draw_list->AddRect(p0, p1, is_bar_hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(65, 65, 80, 200), rounding, 0, 1.0f);

    char shadow_txt[16], fire_txt[16], pet_txt[16];
    std::snprintf(shadow_txt, sizeof(shadow_txt), "%.0f%%", norm_shadow);
    std::snprintf(fire_txt, sizeof(fire_txt), "%.0f%%", norm_fire);
    std::snprintf(pet_txt, sizeof(pet_txt), "%.0f%%", norm_pet);

    float txt_y = p0.y + 1.0f;
    if (s_w >= 26.0f) {
        ImVec2 sz = ImGui::CalcTextSize(shadow_txt);
        float txt_x = p0.x + (s_w - sz.x) * 0.5f;
        draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), shadow_txt);
        draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), shadow_txt);
    }
    if (f_w >= 26.0f) {
        ImVec2 sz = ImGui::CalcTextSize(fire_txt);
        float txt_x = p0.x + s_w + (f_w - sz.x) * 0.5f;
        draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), fire_txt);
        draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), fire_txt);
    }
    if (p_w >= 26.0f) {
        ImVec2 sz = ImGui::CalcTextSize(pet_txt);
        float txt_x = p0.x + s_w + f_w + (p_w - sz.x) * 0.5f;
        draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), pet_txt);
        draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), pet_txt);
    }

    if (is_bar_hovered) {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Damage Share Breakdown:");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "■ Shadow Damage: %.1f%% (%.1f DPS)", shadow_pct, shadow_pct * 0.01 * batch.mean_dps);
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.20f, 1.0f), "■ Fire Damage:   %.1f%% (%.1f DPS)", fire_pct, fire_pct * 0.01 * batch.mean_dps);
        ImGui::TextColored(ImVec4(0.30f, 0.95f, 0.50f, 1.0f), "■ Pet Damage:    %.1f%% (%.1f DPS)", pet_pct, pet_pct * 0.01 * batch.mean_dps);
        ImGui::EndTooltip();
    }

    ImGui::Spacing();

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
        {"Bane of Doom",   batch.pct_doom,               get_spell_breakdown_color(SpellID::CURSE_OF_DOOM),      SpellID::CURSE_OF_DOOM,      false},
        {"Bane of Havoc",  batch.pct_bane_of_havoc,      get_spell_breakdown_color(SpellID::BANE_OF_HAVOC),      SpellID::BANE_OF_HAVOC,      false},
        {"Siphon Life",    batch.pct_siphon_life,        get_spell_breakdown_color(SpellID::SIPHON_LIFE),        SpellID::SIPHON_LIFE,        false},
        {"Soul Fire",      batch.pct_soul_fire,          get_spell_breakdown_color(SpellID::SOUL_FIRE),          SpellID::SOUL_FIRE,          false},
        {"Wrack",          batch.pct_drain_hope,         get_spell_breakdown_color(SpellID::DRAIN_HOPE),         SpellID::DRAIN_HOPE,         false},
        {"Drain Life",     batch.pct_drain_life,         get_spell_breakdown_color(SpellID::DRAIN_LIFE),         SpellID::DRAIN_LIFE,         false},
        {"Drain Soul",     batch.pct_drain_soul,         get_spell_breakdown_color(SpellID::DRAIN_SOUL),         SpellID::DRAIN_SOUL,         false},
        {"Imp (Firebolt)", batch.pct_pet_firebolt,       get_spell_breakdown_color(SpellID::PET_FIREBOLT),       SpellID::PET_FIREBOLT,       true},
        {"Succubus (Lash)",batch.pct_pet_lash_of_pain,   get_spell_breakdown_color(SpellID::PET_LASH_OF_PAIN),   SpellID::PET_LASH_OF_PAIN,   true},
        {"Succubus (Melee)",batch.pct_pet_melee,         get_spell_breakdown_color(SpellID::PET_MELEE),          SpellID::PET_MELEE,          true},
        {"Demonic Brand",  batch.pct_demonic_brand,      ImVec4(0.25f, 0.78f, 0.50f, 1.0f),                     SpellID::NONE,               true},
        {"Touch of Grave", batch.pct_touch_of_the_grave, get_spell_breakdown_color(SpellID::TOUCH_OF_THE_GRAVE), SpellID::TOUCH_OF_THE_GRAVE, false},
    };

    auto draw_dmg_bar = [&](const char* name, double pct, const ImVec4& col, SpellID id) {
        if (pct > 0.05) {
            // White font for ability names
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%-14s:", name);
            ImGui::SameLine(label_offset);

            ImVec2 bar_pos = ImGui::GetCursorScreenPos();
            ImVec2 b_p0 = bar_pos;
            ImVec2 b_p1 = ImVec2(bar_pos.x + bar_width, bar_pos.y + 15.0f);

            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f%% (%.0f)", pct, pct * 0.01 * batch.mean_dps);

            std::string btn_id = "##DmgBarItem_" + std::string(name);
            ImGui::InvisibleButton(btn_id.c_str(), ImVec2(bar_width, 15.0f));
            bool item_hovered = ImGui::IsItemHovered();

            draw_shimmer_bar(ImGui::GetWindowDrawList(), b_p0, b_p1, static_cast<float>(pct / 100.0), col, buf, 3.0f);

            if (item_hovered) {
                ImGui::GetWindowDrawList()->AddRect(b_p0, b_p1, IM_COL32(255, 255, 255, 200), 3.0f, 0, 1.0f);
            }

            if (id != SpellID::NONE && item_hovered) {
                const BatchSpellStats& st = batch.spell_stats[static_cast<size_t>(id)];
                ImGui::BeginTooltip();
                ImGui::TextColored(col, "%s", name);
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
        ImGui::TextColored(ImVec4(0.30f, 0.95f, 0.50f, 1.0f), "%s", pet_summary);
    }
}

} // namespace warlock

#include "src/sim/priest/parallel_runner.hpp"
#include "src/sim/priest/spells.hpp"

namespace priest {

struct PriestDamageBreakdownEntry {
    const char* name;
    double pct;
    ImVec4 color;
    SpellID id;
};

inline ImVec4 get_priest_spell_breakdown_color(SpellID id) {
    switch (id) {
        // Shadow (Purple spectrum)
        case SpellID::SHADOW_WORD_PAIN:   return ImVec4(0.55f, 0.38f, 0.95f, 1.0f); // Deep Shadow Purple
        case SpellID::MIND_FLAY:          return ImVec4(0.45f, 0.58f, 0.95f, 1.0f); // Indigo Periwinkle
        case SpellID::MIND_BLAST:         return ImVec4(0.85f, 0.30f, 0.90f, 1.0f); // Dark Magenta
        case SpellID::SHADOW_WORD_DEATH:  return ImVec4(0.95f, 0.25f, 0.45f, 1.0f); // Crimson Violet
        case SpellID::DEVOURING_PLAGUE:   return ImVec4(0.40f, 0.80f, 0.60f, 1.0f); // Toxic Violet
        case SpellID::SHADOWGUARD:        return ImVec4(0.70f, 0.50f, 0.88f, 1.0f); // Slate Violet
        case SpellID::TOUCH_OF_THE_GRAVE: return ImVec4(0.72f, 0.60f, 0.92f, 1.0f); // Ghostly Lavender

        // Holy (Gold/Yellow spectrum)
        case SpellID::SMITE:              return ImVec4(1.00f, 0.85f, 0.30f, 1.0f); // Brilliant Holy Gold
        case SpellID::HOLY_FIRE:          return ImVec4(1.00f, 0.65f, 0.20f, 1.0f); // Amber Holy Fire
        case SpellID::PENANCE:            return ImVec4(1.00f, 0.92f, 0.50f, 1.0f); // Radiant Light Gold
        case SpellID::HOLY_NOVA:          return ImVec4(0.95f, 0.80f, 0.40f, 1.0f); // Golden Nova Flare
        case SpellID::CHASTISE:           return ImVec4(0.88f, 0.75f, 0.35f, 1.0f); // Sun Gold

        // Arcane (Celestial Blue)
        case SpellID::STARSHARDS:         return ImVec4(0.35f, 0.75f, 0.95f, 1.0f);

        default:                          return ImVec4(0.60f, 0.60f, 0.70f, 1.0f);
    }
}

inline void render_priest_damage_breakdown_bars(const BatchSimResult& batch, float bar_width = 180.0f, float label_offset = 130.0f) {
    // 1. Composite 3-Section Horizontal Bar (Shadow, Holy, Arcane)
    double shadow_pct = batch.pct_sw_pain + batch.pct_mind_flay + batch.pct_mind_blast +
                        batch.pct_sw_death + batch.pct_devouring_plague + batch.pct_shadowguard +
                        batch.pct_touch_of_the_grave;
    double holy_pct = batch.pct_smite + batch.pct_holy_fire + batch.pct_penance +
                      batch.pct_holy_nova + batch.pct_chastise;
    double arcane_pct = batch.pct_starshards;

    double total_pct = shadow_pct + holy_pct + arcane_pct;
    double norm_shadow = 0.0, norm_holy = 0.0, norm_arcane = 0.0;
    if (total_pct > 0.0) {
        norm_shadow = (shadow_pct / total_pct) * 100.0;
        norm_holy = (holy_pct / total_pct) * 100.0;
        norm_arcane = (arcane_pct / total_pct) * 100.0;
    }

    float avail_w = ImGui::GetContentRegionAvail().x;
    float full_bar_w = std::max(60.0f, avail_w);
    float bar_h = 15.0f;
    float rounding = 3.0f;
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + full_bar_w, p0.y + bar_h);

    ImGui::InvisibleButton("##CompositeDmgBarPriest", ImVec2(full_bar_w, bar_h));
    bool is_bar_hovered = ImGui::IsItemHovered();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(p0, p1, IM_COL32(18, 16, 22, 255), rounding);

    float cur_bar_x = p0.x;
    float s_w = (float)(full_bar_w * (norm_shadow * 0.01));
    float h_w = (float)(full_bar_w * (norm_holy * 0.01));
    float a_w = (float)(full_bar_w * (norm_arcane * 0.01));

    auto draw_segment = [&](float start_x, float seg_w, ImU32 col) {
        if (seg_w <= 0.5f) return;
        ImVec2 b0(start_x, p0.y);
        ImVec2 b1(std::min(p1.x, start_x + seg_w), p1.y);
        draw_list->PushClipRect(b0, b1, true);
        draw_list->AddRectFilled(p0, p1, col, rounding);
        draw_shimmer_noise_overlay(draw_list, b0, b1, rounding);
        draw_list->PopClipRect();
    };

    // Shadow segment (Purple)
    if (norm_shadow > 0.5) {
        draw_segment(cur_bar_x, s_w, IM_COL32(148, 65, 235, 240));
        cur_bar_x += s_w;
    }

    // Holy segment (Gold)
    if (norm_holy > 0.5) {
        draw_segment(cur_bar_x, h_w, IM_COL32(235, 195, 45, 240));
        cur_bar_x += h_w;
    }

    // Arcane segment (Light Blue)
    if (norm_arcane > 0.5) {
        draw_segment(cur_bar_x, a_w, IM_COL32(65, 175, 245, 240));
    }

    draw_list->AddRect(p0, p1, is_bar_hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(65, 65, 80, 200), rounding, 0, 1.0f);

    char shadow_txt[16], holy_txt[16], arcane_txt[16];
    std::snprintf(shadow_txt, sizeof(shadow_txt), "%.0f%%", norm_shadow);
    std::snprintf(holy_txt, sizeof(holy_txt), "%.0f%%", norm_holy);
    std::snprintf(arcane_txt, sizeof(arcane_txt), "%.0f%%", norm_arcane);

    float txt_y = p0.y + 1.0f;
    if (s_w >= 26.0f) {
        ImVec2 sz = ImGui::CalcTextSize(shadow_txt);
        float txt_x = p0.x + (s_w - sz.x) * 0.5f;
        draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), shadow_txt);
        draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), shadow_txt);
    }
    if (h_w >= 26.0f) {
        ImVec2 sz = ImGui::CalcTextSize(holy_txt);
        float txt_x = p0.x + s_w + (h_w - sz.x) * 0.5f;
        draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), holy_txt);
        draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), holy_txt);
    }
    if (a_w >= 26.0f) {
        ImVec2 sz = ImGui::CalcTextSize(arcane_txt);
        float txt_x = p0.x + s_w + h_w + (a_w - sz.x) * 0.5f;
        draw_list->AddText(ImVec2(txt_x + 1, txt_y + 1), IM_COL32(0, 0, 0, 220), arcane_txt);
        draw_list->AddText(ImVec2(txt_x, txt_y), IM_COL32(255, 255, 255, 255), arcane_txt);
    }

    if (is_bar_hovered) {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Damage Share Breakdown:");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "■ Shadow Damage: %.1f%% (%.1f DPS)", shadow_pct, shadow_pct * 0.01 * batch.mean_dps);
        ImGui::TextColored(ImVec4(1.00f, 0.85f, 0.30f, 1.0f), "■ Holy Damage:   %.1f%% (%.1f DPS)", holy_pct, holy_pct * 0.01 * batch.mean_dps);
        if (arcane_pct > 0.05) {
            ImGui::TextColored(ImVec4(0.35f, 0.75f, 0.95f, 1.0f), "■ Arcane Damage: %.1f%% (%.1f DPS)", arcane_pct, arcane_pct * 0.01 * batch.mean_dps);
        }
        ImGui::EndTooltip();
    }

    ImGui::Spacing();

    const PriestDamageBreakdownEntry all_entries[] = {
        {"SW: Pain",          batch.pct_sw_pain,          get_priest_spell_breakdown_color(SpellID::SHADOW_WORD_PAIN),   SpellID::SHADOW_WORD_PAIN},
        {"Mind Flay",         batch.pct_mind_flay,        get_priest_spell_breakdown_color(SpellID::MIND_FLAY),          SpellID::MIND_FLAY},
        {"Mind Blast",        batch.pct_mind_blast,       get_priest_spell_breakdown_color(SpellID::MIND_BLAST),         SpellID::MIND_BLAST},
        {"SW: Death",         batch.pct_sw_death,         get_priest_spell_breakdown_color(SpellID::SHADOW_WORD_DEATH),  SpellID::SHADOW_WORD_DEATH},
        {"Devouring Plague",  batch.pct_devouring_plague, get_priest_spell_breakdown_color(SpellID::DEVOURING_PLAGUE),   SpellID::DEVOURING_PLAGUE},
        {"Smite",             batch.pct_smite,            get_priest_spell_breakdown_color(SpellID::SMITE),              SpellID::SMITE},
        {"Holy Fire",         batch.pct_holy_fire,        get_priest_spell_breakdown_color(SpellID::HOLY_FIRE),          SpellID::HOLY_FIRE},
        {"Penance",           batch.pct_penance,          get_priest_spell_breakdown_color(SpellID::PENANCE),            SpellID::PENANCE},
        {"Holy Nova",         batch.pct_holy_nova,        get_priest_spell_breakdown_color(SpellID::HOLY_NOVA),          SpellID::HOLY_NOVA},
        {"Starshards",        batch.pct_starshards,       get_priest_spell_breakdown_color(SpellID::STARSHARDS),         SpellID::STARSHARDS},
        {"Chastise",          batch.pct_chastise,         get_priest_spell_breakdown_color(SpellID::CHASTISE),           SpellID::CHASTISE},
        {"Shadowguard",       batch.pct_shadowguard,      get_priest_spell_breakdown_color(SpellID::SHADOWGUARD),        SpellID::SHADOWGUARD},
        {"Touch of Grave",    batch.pct_touch_of_the_grave, get_priest_spell_breakdown_color(SpellID::TOUCH_OF_THE_GRAVE), SpellID::TOUCH_OF_THE_GRAVE},
    };

    auto draw_dmg_bar = [&](const char* name, double pct, const ImVec4& col, SpellID id) {
        if (pct > 0.05) {
            // White font for ability names
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%-14s:", name);
            ImGui::SameLine(label_offset);

            ImVec2 bar_pos = ImGui::GetCursorScreenPos();
            ImVec2 b_p0 = bar_pos;
            ImVec2 b_p1 = ImVec2(bar_pos.x + bar_width, bar_pos.y + 15.0f);

            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f%% (%.0f)", pct, pct * 0.01 * batch.mean_dps);

            std::string btn_id = "##PriestDmgBarItem_" + std::string(name);
            ImGui::InvisibleButton(btn_id.c_str(), ImVec2(bar_width, 15.0f));
            bool item_hovered = ImGui::IsItemHovered();

            draw_shimmer_bar(ImGui::GetWindowDrawList(), b_p0, b_p1, static_cast<float>(pct / 100.0), col, buf, 3.0f);

            if (item_hovered) {
                ImGui::GetWindowDrawList()->AddRect(b_p0, b_p1, IM_COL32(255, 255, 255, 200), 3.0f, 0, 1.0f);
            }

            if (id != SpellID::NONE && item_hovered) {
                const auto& st = batch.spell_stats[static_cast<size_t>(id)];
                ImGui::BeginTooltip();
                ImGui::TextColored(col, "%s", name);
                ImGui::Separator();
                ImGui::Text("Avg casts: %.1f | Avg hits: %.1f | Avg hit: %.0f", st.mean_casts, st.mean_hits, sim::spell_avg_hit(st));
                ImGui::Text("Crit: %.1f%% | Miss: %.1f%%", sim::spell_crit_pct(st), sim::spell_miss_pct(st));
                ImGui::EndTooltip();
            }
        }
    };

    for (const auto& entry : all_entries) {
        draw_dmg_bar(entry.name, entry.pct, entry.color, entry.id);
    }
}

} // namespace priest
