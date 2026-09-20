#pragma once
#include "imgui.h"
#include "src/ui/asset_manager.hpp"
#include <string>
#include <vector>
#include <algorithm>

namespace priest {

struct SpellBookEntry {
    std::string name;
    std::string rank;
    std::string school;
    std::string cast_time;
    std::string mana_cost;
    std::string effect;
    std::string sp_coeff;
    std::string formula_or_note;
    std::string icon_name;
};

inline const std::vector<SpellBookEntry>& get_all_priest_spellbook_entries() {
    static const std::vector<SpellBookEntry> entries = {
        { "Power Word: Fortitude", "Rank 6", "Discipline", "Instant", "1695 Mana", "Power infuses the target, increasing their Stamina by 70 for 1 hr.", "—", "70 instead of 54, 1 hr instead of 30 min", "spell_holy_wordfortitude" },
        { "Power Word: Shield", "Rank 10", "Discipline", "Instant (4 sec cooldown)", "500 Mana", "Draws on the soul of the party member to shield them, absorbing 928 damage. Lasts 30 sec. While the shield holds, spellcasting will not be interrupted by damage. Once shielded, the target cannot be shielded again for 15 sec.", "10.0%", "928 instead of 942", "spell_holy_powerwordshield" },
        { "Confounding Flash", "—", "Discipline", "0.5 sec cast (2 min cooldown)", "3% of base mana", "Confuses up to 5 enemies within 8 yds for 3 sec. Any damage taken will break the effect.", "—", "Confuses up to 5 enemies within 8 yds for 3 sec. Any damage taken will break the effect.", "ability_paladin_blindinglight2" },
        { "Starshards", "Rank 7", "Arcane", "Channeled (30 sec cooldown)", "350 Mana", "Rains starshards down on the enemy target's head, causing 1800 Arcane damage over 6 sec.", "16.7% instead of 15.4%", "Cooldown 30 sec instead of none; 1800 instead of 936", "spell_arcane_starfire" },
        { "Inner Fire", "Rank 6", "Discipline", "Instant", "315 Mana", "A burst of Holy energy fills the caster, increasing armor by 1395. Each melee or ranged damage hit against the priest will remove one charge. Lasts 10 min or until 20 charges are used.", "—", "A burst of Holy energy fills the caster, increasing armor by 1395. Each melee or ranged damage hit against the priest wi", "spell_holy_innerfire" },
        { "Dispel Magic", "Rank 2", "Discipline", "Instant", "18% of base mana", "Dispels magic on the target, removing 2 harmful spells from a friend or 2 beneficial spells from an enemy.", "—", "Dispels magic on the target, removing 2 harmful spells from a friend or 2 beneficial spells from an enemy.", "spell_holy_dispelmagic" },
        { "Contingency Plan", "Rank 5", "Discipline", "Instant (10 min cooldown)", "", "Place a Holy ward on an ally for 30 sec. The next time this ally takes damage dropping their Health below 35%, they will gain a shield absorbing ? damage and begin healing for ? Health over 15 sec. A target may be affected by only one Contingency Plan.", "—", "Place a Holy ward on an ally for 30 sec. The next time this ally takes damage dropping their Health below 35%, they will", "ability_priest_soulwarding" },
        { "Elune's Grace", "—", "Discipline", "Instant (5 min cooldown)", "3% of base mana", "Reduces the chance you'll be hit by melee and ranged attacks by 50% for 15 sec or until you are missed 3 times.", "—", "Costs 3% of base mana instead of 60 Mana; Tooltip rewritten", "spell_holy_elunesgrace" },
        { "Feedback", "Rank 5", "Discipline", "Instant (3 min cooldown)", "230 Mana", "The priest becomes surrounded with anti-magic energy. Any successful spell cast against the priest will burn 105 of the attacker's Mana, causing 1 Shadow damage for each point of Mana burned. Lasts 15 sec.", "—", "Costs 230 Mana instead of 580 Mana", "spell_shadow_ritualofsacrifice" },
        { "Shackle Undead", "Rank 3", "Discipline", "1.5 sec cast", "150 Mana", "Shackles the target undead enemy for up to 50 sec. The shackled unit is unable to move, attack or cast spells. Any damage caused will release the target. Only one target can be shackled at a time.", "—", "Shackles the target undead enemy for up to 50 sec. The shackled unit is unable to move, attack or cast spells. Any damag", "spell_nature_slow" },
        { "Mana Burn", "Rank 5", "Discipline", "3 sec cast", "270 Mana", "Drains 738 to 780 mana from a target. For each mana drained in this way, the target takes 0.5 Shadow damage.", "—", "Drains 738 to 780 mana from a target. For each mana drained in this way, the target takes 0.5 Shadow damage.", "spell_shadow_manaburn" },
        { "Divine Spirit", "Rank 4", "Discipline", "Instant", "970 Mana", "Holy power infuses the target, increasing their Spirit by 40 for 1 hr.", "—", "1 hr instead of 30 min", "spell_holy_divinespirit" },
        { "Penance", "Rank 4", "Discipline", "Channeled (12 sec cooldown)", "355 Mana", "Launches a volley of holy light at the target, causing 131 Holy damage to an enemy, or 673 healing to an ally, instantly and every 1 sec for 2 sec.", "75.0%", "Launches a volley of holy light at the target, causing 131 Holy damage to an enemy, or 673 healing to an ally, instantly", "spell_holy_penance" },
        { "Levitate", "—", "Discipline", "Instant", "100 Mana", "Allows the caster to levitate, floating a few feet above the ground. While levitating, you will fall at a reduced speed and travel over water-like surfaces. Mounting or any damage taken will cancel the effect. Lasts 2 min.", "—", "Same numbers, reworded", "spell_holy_layonhands" },
        { "Prayer of Fortitude", "Rank 2", "Discipline", "Instant", "3400 Mana", "Power infuses all party and raid members, increasing their Stamina by 70 for 1 hr.", "—", "Tooltip rewritten", "spell_holy_prayeroffortitude" },
        { "Prayer of Spirit", "—", "Discipline", "Instant", "1940 Mana", "Power infuses all party and raid members, increasing their Spirit by 40 for 1 hr.", "—", "Tooltip rewritten", "spell_holy_prayerofspirit" },
        { "Lesser Heal", "Rank 3", "Holy", "2.5 sec cast", "75 Mana", "Heal your target for 130 to 152.", "71.4% instead of 44.6%", "130 to 152 instead of 135 to 157; Spell power bonus 71.4% instead of 44.6%", "spell_holy_lesserheal" },
        { "Smite", "Rank 8", "Holy", "2.5 sec cast", "280 Mana", "Smite an enemy for 160 to 180 Holy damage.", "71.4% instead of 55.4%", "160 to 180 instead of 371 to 415", "spell_holy_holysmite" },
        { "Renew", "Rank 10", "Holy", "Instant", "410 Mana", "Heals the target of 830 damage over 15 sec.", "20% instead of 15.5%", "830 instead of 970", "spell_holy_renew" },
        { "Desperate Prayer", "Rank 7", "Holy", "Instant (10 min cooldown)", "", "Instantly heals the caster for 1269 to 1497.", "42.9% instead of 39.6%", "1269 to 1497 instead of 1324 to 1562", "spell_holy_restoration" },
        { "Divine Grace", "Rank 7", "Holy", "Instant (10 min cooldown)", "", "Instantly heals a friendly target below 50% Health for 1269 to 1497 and removes Weakened Soul from that target. Cannot be cast on self.", "—", "Instantly heals a friendly target below 50% Health for 1269 to 1497 and removes Weakened Soul from that target. Cannot b", "ability_priest_savinggrace" },
        { "Resurrection", "Rank 5", "Holy", "10 sec cast", "75% of base mana", "Brings a dead player back to life with 750 health and 1000 mana. Cannot be cast when in combat.", "—", "Brings a dead player back to life with 750 health and 1000 mana. Cannot be cast when in combat.", "spell_holy_resurrection" },
        { "Cure Disease", "—", "Holy", "Instant", "15% of base mana", "Removes 1 disease from the friendly target.", "—", "Removes 1 disease from the friendly target.", "spell_holy_nullifydisease" },
        { "Heal", "Rank 4", "Holy", "3 sec cast", "305 Mana", "Heal your target for 611 to 691.", "85.7% instead of 72.9%", "611 to 691 instead of 712 to 804", "spell_holy_heal02" },
        { "Chastise", "Rank 5", "Holy", "Instant (2 min cooldown)", "225 Mana", "Chastise the target, causing 272 to 306 Holy damage and Immobilizing them for up to 2 sec. Only works against Humanoids. This spell causes very low threat", "—", "Chastise the target, causing 272 to 306 Holy damage and Immobilizing them for up to 2 sec. Only works against Humanoids.", "spell_holy_chastise" },
        { "Fear Ward", "—", "Holy", "Instant (3 min cooldown)", "100 Mana", "Wards the friendly target against Fear. The next Fear effect used against the target will fail, using up the ward. Lasts 3 min.", "—", "Cooldown 3 min instead of 30 sec; 3 min instead of 10 min", "spell_holy_excorcism" },
        { "Flash Heal", "Rank 7", "Holy", "1.5 sec cast", "380 Mana", "Heals a friendly target for 757 to 893.", "85.7%", "757 to 893 instead of 812 to 958", "spell_holy_flashheal" },
        { "Holy Fire", "Rank 8", "Holy", "3.5 sec cast", "255 Mana", "Consumes the enemy in holy flames that cause 184 to 232 Holy damage and an additional 75 Holy damage over 10 sec.", "75% + 25%", "184 to 232 instead of 355 to 449, 75 instead of 145", "spell_holy_searinglight" },
        { "Holy Nova", "Rank 6", "Holy", "Instant", "750 Mana", "Causes an explosion of holy light around the caster, causing 174 to 200 Holy damage to all enemy targets within 10 yards and healing all party members within 10 yards for 288 to 334. These effects cause no threat.  Each time Holy Fire deals damage, you have a 5% chance for your next Holy Nova to cost no Mana.", "—", "Numbers and wording changed", "spell_holy_holynova" },
        { "Binding Heal", "Rank 6", "Holy", "1.5 sec cast", "380 Mana", "Heals a friendly target and the caster for 757 to 893. Low threat.", "—", "Heals a friendly target and the caster for 757 to 893. Low threat.", "spell_holy_blindingheal" },
        { "Prayer of Healing", "Rank 5", "Holy", "3 sec cast", "1070 Mana", "A powerful prayer that heals the target and their party for 631 to 667. Party members must be within 40 yards of target.", "300.0% (total)", "Range 40 yd instead of none; Tooltip rewritten", "spell_holy_prayerofhealing02" },
        { "Abolish Disease", "—", "Holy", "Instant", "15% of base mana", "Attempts to cure 1 disease effect on the target, and 1 more disease effect every 5 seconds for 20 sec.", "—", "Attempts to cure 1 disease effect on the target, and 1 more disease effect every 5 seconds for 20 sec.", "spell_nature_nullifydisease" },
        { "Greater Heal", "Rank 5", "Holy", "3 sec cast", "710 Mana", "A slow casting spell that heals a single target for 1853 to 2067.", "85.7%", "1853 to 2067 instead of 1966 to 2194", "spell_holy_greaterheal" },
        { "Lightwell", "Rank 3", "Holy", "1.5 sec cast (10 min cooldown)", "365 Mana", "Creates a holy Lightwell near the priest. Members of your raid or party can click the Lightwell to restore 1600 health over 10 sec. Being attacked cancels the effect. Lightwell lasts for 3 min or 5 charges.", "—", "Creates a holy Lightwell near the priest. Members of your raid or party can click the Lightwell to restore 1600 health o", "spell_holy_summonlightwell" },
        { "Lightwell Renew", "Rank 3", "Holy", "Instant", "", "", "—", "", "spell_holy_summonlightwell" },
        { "Prayer of Mending", "Rank 3", "Holy", "Instant (10 sec cooldown)", "390 Mana", "Places a spell on the target that heals them for 413 the next time they take damage or receive non-periodic healing. When the heal occurs, Prayer of Mending jumps to a party or raid member within 20 yards. Jumps up to 5 times and lasts 30 sec after each jump. This spell can only be placed on one target at a time per caster.", "—", "Places a spell on the target that heals them for 413 the next time they take damage or receive non-periodic healing. Whe", "spell_holy_prayerofmendingtga" },
        { "Shadow Word: Pain", "Rank 8", "Shadow", "Instant", "470 Mana", "A word of darkness that causes 762 Shadow damage over 18 sec.", "20% instead of 16.7%", "762 instead of 852; Spell power bonus 20% instead of 16.7%", "spell_shadow_shadowwordpain" },
        { "Fade", "Rank 6", "Shadow", "Instant (30 sec cooldown)", "275 Mana", "Fade out, discouraging enemies from attacking you for 10 sec. More effective than Fade (rank 5).", "—", "Fade out, discouraging enemies from attacking you for 10 sec. More effective than Fade (rank 5).", "spell_magic_lesserinvisibilty" },
        { "Hex of Weakness", "Rank 6", "Shadow", "Instant", "240 Mana", "Weakens the target enemy, reducing melee attack power by 204 and reducing the effectiveness of any healing by 20%. Lasts 2 min", "—", "204 instead of 20, reworded", "spell_shadow_fingerofdeath" },
        { "Mind Blast", "Rank 9", "Shadow", "1.5 sec cast (8 sec cooldown)", "350 Mana", "Blasts the target for 472 to 498 Shadow damage, but causes a high amount of threat.", "42.9% instead of 36.4%", "472 to 498 instead of 503 to 531", "spell_shadow_unholyfrenzy" },
        { "Touch of Weakness", "Rank 6", "Shadow", "Instant", "195 Mana", "The next melee attack on the caster will cause 56 Shadow damage and reduce the attacker's melee attack power by the attacker by 204 for 2 min.", "—", "56 instead of 64, 204 instead of 20, reworded", "spell_shadow_deadofnight" },
        { "Psychic Scream", "Rank 4", "Shadow", "Instant (30 sec cooldown)", "210 Mana", "The caster lets out a psychic scream, causing 5 enemies within 8 yards to flee for 8 sec. Damage caused may interrupt the effect.", "—", "The caster lets out a psychic scream, causing 5 enemies within 8 yards to flee for 8 sec. Damage caused may interrupt th", "spell_shadow_psychicscream" },
        { "Dark Sacrifice", "Rank 5", "Shadow", "Instant (10 min cooldown)", "", "Cannibalize 1600 of your own Health over 15 sec to gain 1600 Mana.", "—", "Cannibalize 1600 of your own Health over 15 sec to gain 1600 Mana.", "spell_holy_powerinfusion_shadow" },
        { "Devouring Plague", "Rank 6", "Shadow", "Instant (1 min cooldown)", "985 Mana", "Afflicts the target with a disease that causes 848 Shadow damage over 24 sec. Damage caused by the Devouring Plague heals the caster.", "80.0%", "Cooldown 1 min instead of 3 min; 848 instead of 904", "spell_shadow_devouringplague" },
        { "Mind Flay", "Rank 6", "Shadow", "Channeled", "205 Mana", "Assault the target's mind with Shadow energy, causing 390 Shadow damage over 3 sec and slowing their movement speed by 50%.", "16.7% instead of 15%", "390 instead of 426; Spell power bonus 16.7% instead of 15%", "spell_shadow_siphonmana" },
        { "Mind Soothe", "Rank 3", "Shadow", "Instant", "90 Mana", "Soothes the target, reducing the range at which it will attack you by 10 yards. Only affects Humanoid targets level 70 or lower. Lasts 15 sec.", "—", "Soothes the target, reducing the range at which it will attack you by 10 yards. Only affects Humanoid targets level 70 o", "spell_holy_mindsooth" },
        { "Shadowguard", "Rank 6", "Shadow", "Instant", "250 Mana", "The caster is surrounded by shadows. When a spell, melee or ranged attack hits the caster, the attacker will be struck for 96 Shadow damage. Attackers can only be damaged once every few seconds. This damage causes no threat. 3 charges. Lasts 10 min.", "80.0%", "96 instead of 116", "spell_nature_lightningshield" },
        { "Mind Vision", "Rank 2", "Shadow", "Channeled", "150 Mana", "Allows the caster to see through the target's eyes for 1 min. Will not work if the target is in another instance or on another continent.", "—", "Allows the caster to see through the target's eyes for 1 min. Will not work if the target is in another instance or on a", "spell_holy_mindvision" },
        { "Mind Control", "Rank 3", "Shadow", "Channeled", "750 Mana", "Controls a humanoid mind up to level 62, but increases the time between attacks by 25%. Lasts up to 1 min.", "—", "Controls a humanoid mind up to level 62, but increases the time between attacks by 25%. Lasts up to 1 min.", "spell_shadow_shadowworddominate" },
        { "Shadow Protection", "Rank 3", "Shadow", "Instant", "650 Mana", "Increases the target's resistance to Shadow spells by 60 for 10 min.", "—", "Increases the target's resistance to Shadow spells by 60 for 10 min.", "spell_shadow_antishadow" },
        { "Shadow Word: Death", "Rank 4", "Shadow", "Instant (15 sec cooldown)", "340 Mana", "A word of dark binding that inflicts 434 to 462 Shadow damage to the target. If your target is not killed by Shadow Word: Death, you take backlash damage equal to 10% of your maximum health.", "42.9%", "A word of dark binding that inflicts 434 to 462 Shadow damage to the target. If your target is not killed by Shadow Word", "spell_shadow_demonicfortitude" },
        { "Prayer of Shadow Protection", "—", "Shadow", "Instant", "1300 Mana", "Power infuses all party and raid members, increasing their Shadow resistance by 60 for 20 min.", "—", "Tooltip rewritten", "spell_holy_prayerofshadowprotection" },
    };
    return entries;
}

inline void render_priest_spellbook_panel() {
    static char search_filter[128] = "";
    static int selected_school_filter = 0; // 0 = All, 1 = Shadow, 2 = Holy, 3 = Discipline, 4 = Arcane

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Priest Spellbook & Base Coefficients (WoW Forever Beta Client)");
    ImGui::TextDisabled("Extracted directly from client data (52 spells across Discipline, Holy, and Shadow Magic)");
    ImGui::Spacing();

    // Search and Filters
    ImGui::SetNextItemWidth(260);
    ImGui::InputTextWithHint("##PriestSpellSearch", "Search spells by name or school...", search_filter, sizeof(search_filter));
    ImGui::SameLine();

    const char* filter_labels[] = { "All (52)", "Shadow (16)", "Holy (20)", "Discipline (15)", "Arcane (1)" };
    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine();
        if (selected_school_filter == i) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.25f, 0.50f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.20f, 0.8f));
        }
        if (ImGui::Button(filter_labels[i])) {
            selected_school_filter = i;
        }
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const auto& entries = get_all_priest_spellbook_entries();

    if (ImGui::BeginTable("PriestSpellbookTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("Spell / Ability", ImGuiTableColumnFlags_WidthFixed, 220.0f);
        ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("School", ImGuiTableColumnFlags_WidthFixed, 85.0f);
        ImGui::TableSetupColumn("Cast Time & CD", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Mana Cost", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Base Damage / Effect", ImGuiTableColumnFlags_WidthFixed, 280.0f);
        ImGui::TableSetupColumn("SP Coeff", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("WoW Forever Diffs & Notes", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        std::string query = search_filter;
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        for (const auto& sp : entries) {
            std::string name_lower = sp.name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            std::string school_lower = sp.school;
            std::transform(school_lower.begin(), school_lower.end(), school_lower.begin(), ::tolower);

            // Filter check
            if (!query.empty() && name_lower.find(query) == std::string::npos && school_lower.find(query) == std::string::npos) {
                continue;
            }
            if (selected_school_filter == 1 && sp.school != "Shadow") continue;
            if (selected_school_filter == 2 && sp.school != "Holy") continue;
            if (selected_school_filter == 3 && sp.school != "Discipline") continue;
            if (selected_school_filter == 4 && sp.school != "Arcane") continue;

            ImGui::TableNextRow();

            // Col 0: Icon + Name
            ImGui::TableSetColumnIndex(0);
            const Texture2D& icon = warlock::AssetManager::get().get_icon(sp.icon_name);
            if (icon.id > 0) {
                ImGui::Image((ImTextureID)(uintptr_t)icon.id, ImVec2(22, 22));
                ImGui::SameLine(0, 6);
            }
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.70f, 1.0f), "%s", sp.name.c_str());

            // Col 1: Rank
            ImGui::TableSetColumnIndex(1);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.80f, 1.0f), "%s", sp.rank.c_str());

            // Col 2: School
            ImGui::TableSetColumnIndex(2);
            ImGui::AlignTextToFramePadding();
            if (sp.school == "Shadow") {
                ImGui::TextColored(ImVec4(0.70f, 0.40f, 1.0f, 1.0f), "Shadow");
            } else if (sp.school == "Holy") {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "Holy");
            } else if (sp.school == "Arcane") {
                ImGui::TextColored(ImVec4(0.40f, 0.80f, 1.0f, 1.0f), "Arcane");
            } else {
                ImGui::TextColored(ImVec4(0.80f, 0.85f, 1.0f, 1.0f), "%s", sp.school.c_str());
            }

            // Col 3: Cast Time & CD
            ImGui::TableSetColumnIndex(3);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", sp.cast_time.c_str());

            // Col 4: Mana Cost
            ImGui::TableSetColumnIndex(4);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.0f, 1.0f), "%s", sp.mana_cost.c_str());

            // Col 5: Base Damage / Effect
            ImGui::TableSetColumnIndex(5);
            ImGui::AlignTextToFramePadding();
            ImGui::TextWrapped("%s", sp.effect.c_str());

            // Col 6: SP Coefficient
            ImGui::TableSetColumnIndex(6);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.40f, 1.0f), "%s", sp.sp_coeff.c_str());

            // Col 7: Forever Diffs & Notes
            ImGui::TableSetColumnIndex(7);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(0.40f, 0.90f, 0.60f, 1.0f), "%s", sp.formula_or_note.c_str());
        }

        ImGui::EndTable();
    }
}

} // namespace priest
