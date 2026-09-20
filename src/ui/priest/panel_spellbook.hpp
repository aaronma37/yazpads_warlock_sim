#pragma once
#include "imgui.h"
#include "src/sim/priest/spells.hpp"

namespace priest {

inline void render_priest_spellbook_panel() {
    ImGui::BeginChild("PriestSpellbookPanel", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Priest Spellbook & Base Coefficients");
    ImGui::Separator();

    auto render_spell_entry = [](const char* name, const sim::SpellDefinition& def, const char* note) {
        ImGui::TextColored(ImVec4(0.85f, 0.75f, 1.0f, 1.0f), "%s", name);
        ImGui::BulletText("School: %s | Cast Time: %.1fs | Mana Cost: %.0f | Cooldown: %.1fs",
            sim::school_to_string(def.school), def.base_cast_time, def.mana_cost, def.cooldown);
        if (def.is_dot || def.is_channeled) {
            ImGui::BulletText("Periodic: %.0f base dmg/tick | Tick Interval: %.1fs | %d Ticks | SP Coeff: %.1f%%/tick",
                def.dot_base_dmg_per_tick, def.dot_tick_interval, def.num_ticks, def.dot_coeff_per_tick * 100.0);
        } else {
            ImGui::BulletText("Direct: %.0f - %.0f base dmg | SP Coeff: %.1f%%",
                def.min_dmg, def.max_dmg, def.direct_coefficient * 100.0);
        }
        if (note && note[0] != '\0') {
            ImGui::TextDisabled("  Note: %s", note);
        }
        ImGui::Spacing();
    };

    render_spell_entry("Shadow Word: Pain (Rank 8)", SpellBook::shadow_word_pain_rank8(), "Base 18s duration (6 ticks), extended to 24s (8 ticks) by Imp SW:P");
    render_spell_entry("Mind Flay (Rank 6)", SpellBook::mind_flay_rank6(), "3-second channeled spell with 1-second tick intervals");
    render_spell_entry("Mind Blast (Rank 9)", SpellBook::mind_blast_rank9(), "Direct shadow nuke with 8.0s CD, reduced to 5.5s by Imp Mind Blast");
    render_spell_entry("Shadow Word: Death (Rank 1)", SpellBook::shadow_word_death_rank1(), "Instant finisher on 12s CD, +30% crit below 20% HP with Early Demise");
    render_spell_entry("Devouring Plague (Rank 6)", SpellBook::devouring_plague_rank6(), "24s disease DoT, mana cost reduced by 50% with Devouring Contagion");
    render_spell_entry("Smite (Rank 8)", SpellBook::smite_rank8(), "Direct Holy damage spell with 2.5s base cast time (2.0s with Divine Fury)");
    render_spell_entry("Holy Fire (Rank 8)", SpellBook::holy_fire_rank8(), "Direct Holy damage + 10-second burning DoT");

    ImGui::EndChild();
}

} // namespace priest
