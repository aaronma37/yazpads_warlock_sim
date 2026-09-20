#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

namespace priest {

struct TalentNodeDef {
    const char* id;
    const char* name;
    int row;
    int col;
    int max_points;
    const char* icon;
    const char* req;
    const char* desc[5];
};

struct DisciplineTalents {
    int power_in_light = 0; // max: 5, R1C1
    int wand_specialization = 0; // max: 2, R1C2
    int twin_disciplines = 0; // max: 5, R1C3
    int silent_resolve = 0; // max: 3, R2C1
    int holy_precision = 0; // max: 3, R2C2
    int improved_power_word_shield = 0; // max: 3, R2C3
    int martyrdom = 0; // max: 2, R2C4
    int mental_agility = 0; // max: 3, R3C1
    int inner_focus = 0; // max: 1, R3C2
    int meditation = 0; // max: 3, R3C4
    int improved_inner_fire = 0; // max: 3, R4C1
    int mental_strength = 0; // max: 5, R4C2
    int soul_warding = 0; // max: 1, R4C3
    int improved_mana_burn = 0; // max: 2, R4C4
    int penance = 0; // max: 1, R5C2
    int renewed_hope = 0; // max: 5, R5C3
    int divine_aegis = 0; // max: 3, R6C3
    int power_infusion = 0; // max: 1, R7C2

    int total_points() const {
        return power_in_light + 
               wand_specialization + 
               twin_disciplines + 
               silent_resolve + 
               holy_precision + 
               improved_power_word_shield + 
               martyrdom + 
               mental_agility + 
               inner_focus + 
               meditation + 
               improved_inner_fire + 
               mental_strength + 
               soul_warding + 
               improved_mana_burn + 
               penance + 
               renewed_hope + 
               divine_aegis + 
               power_infusion;
    }

    int& get_points_by_index(size_t idx) {
        switch (idx) {
            case 0: return power_in_light;
            case 1: return wand_specialization;
            case 2: return twin_disciplines;
            case 3: return silent_resolve;
            case 4: return holy_precision;
            case 5: return improved_power_word_shield;
            case 6: return martyrdom;
            case 7: return mental_agility;
            case 8: return inner_focus;
            case 9: return meditation;
            case 10: return improved_inner_fire;
            case 11: return mental_strength;
            case 12: return soul_warding;
            case 13: return improved_mana_burn;
            case 14: return penance;
            case 15: return renewed_hope;
            case 16: return divine_aegis;
            case 17: return power_infusion;
            default: return power_in_light;
        }
    }

    int get_points_by_index(size_t idx) const {
        switch (idx) {
            case 0: return power_in_light;
            case 1: return wand_specialization;
            case 2: return twin_disciplines;
            case 3: return silent_resolve;
            case 4: return holy_precision;
            case 5: return improved_power_word_shield;
            case 6: return martyrdom;
            case 7: return mental_agility;
            case 8: return inner_focus;
            case 9: return meditation;
            case 10: return improved_inner_fire;
            case 11: return mental_strength;
            case 12: return soul_warding;
            case 13: return improved_mana_burn;
            case 14: return penance;
            case 15: return renewed_hope;
            case 16: return divine_aegis;
            case 17: return power_infusion;
            default: return 0;
        }
    }
};

struct HolyTalents {
    int twilight_focus = 0; // max: 3, R1C1
    int improved_renew = 0; // max: 3, R1C2
    int holy_specialization = 0; // max: 5, R1C3
    int spell_warding = 0; // max: 5, R2C2
    int divine_fury = 0; // max: 5, R2C3
    int holy_nova = 0; // max: 1, R3C1
    int blessed_recovery = 0; // max: 3, R3C2
    int inspiration = 0; // max: 3, R3C4
    int holy_reach = 0; // max: 2, R4C1
    int improved_healing = 0; // max: 3, R4C2
    int searing_light = 0; // max: 2, R4C3
    int binding_heal = 0; // max: 1, R4C4
    int litany_of_light = 0; // max: 2, R5C1
    int spirit_of_redemption = 0; // max: 1, R5C2
    int spiritual_guidance = 0; // max: 5, R5C3
    int spiritual_healing = 0; // max: 3, R6C3
    int prayer_of_mending = 0; // max: 1, R7C2

    int total_points() const {
        return twilight_focus + 
               improved_renew + 
               holy_specialization + 
               spell_warding + 
               divine_fury + 
               holy_nova + 
               blessed_recovery + 
               inspiration + 
               holy_reach + 
               improved_healing + 
               searing_light + 
               binding_heal + 
               litany_of_light + 
               spirit_of_redemption + 
               spiritual_guidance + 
               spiritual_healing + 
               prayer_of_mending;
    }

    int& get_points_by_index(size_t idx) {
        switch (idx) {
            case 0: return twilight_focus;
            case 1: return improved_renew;
            case 2: return holy_specialization;
            case 3: return spell_warding;
            case 4: return divine_fury;
            case 5: return holy_nova;
            case 6: return blessed_recovery;
            case 7: return inspiration;
            case 8: return holy_reach;
            case 9: return improved_healing;
            case 10: return searing_light;
            case 11: return binding_heal;
            case 12: return litany_of_light;
            case 13: return spirit_of_redemption;
            case 14: return spiritual_guidance;
            case 15: return spiritual_healing;
            case 16: return prayer_of_mending;
            default: return twilight_focus;
        }
    }

    int get_points_by_index(size_t idx) const {
        switch (idx) {
            case 0: return twilight_focus;
            case 1: return improved_renew;
            case 2: return holy_specialization;
            case 3: return spell_warding;
            case 4: return divine_fury;
            case 5: return holy_nova;
            case 6: return blessed_recovery;
            case 7: return inspiration;
            case 8: return holy_reach;
            case 9: return improved_healing;
            case 10: return searing_light;
            case 11: return binding_heal;
            case 12: return litany_of_light;
            case 13: return spirit_of_redemption;
            case 14: return spiritual_guidance;
            case 15: return spiritual_healing;
            case 16: return prayer_of_mending;
            default: return 0;
        }
    }
};

struct ShadowTalents {
    int shadow_focus = 0; // max: 5, R1C1
    int blackout = 0; // max: 5, R1C2
    int spirit_tap = 0; // max: 5, R1C3
    int shadow_affinity = 0; // max: 3, R2C1
    int improved_shadow_word_pain = 0; // max: 2, R2C3
    int shadow_reach = 0; // max: 2, R2C4
    int improved_mind_blast = 0; // max: 5, R3C1
    int improved_psychic_scream = 0; // max: 2, R3C2
    int mind_flay = 0; // max: 1, R3C3
    int improved_mind_flay = 0; // max: 2, R3C4
    int improved_fade = 0; // max: 2, R4C1
    int vampiric_embrace = 0; // max: 1, R4C2
    int shadow_weaving = 0; // max: 3, R4C3
    int silence = 0; // max: 1, R5C1
    int devouring_contagion = 0; // max: 2, R5C3
    int early_demise = 0; // max: 2, R6C1
    int darkness = 0; // max: 5, R6C3
    int shadowform = 0; // max: 1, R7C2

    int total_points() const {
        return shadow_focus + 
               blackout + 
               spirit_tap + 
               shadow_affinity + 
               improved_shadow_word_pain + 
               shadow_reach + 
               improved_mind_blast + 
               improved_psychic_scream + 
               mind_flay + 
               improved_mind_flay + 
               improved_fade + 
               vampiric_embrace + 
               shadow_weaving + 
               silence + 
               devouring_contagion + 
               early_demise + 
               darkness + 
               shadowform;
    }

    int& get_points_by_index(size_t idx) {
        switch (idx) {
            case 0: return shadow_focus;
            case 1: return blackout;
            case 2: return spirit_tap;
            case 3: return shadow_affinity;
            case 4: return improved_shadow_word_pain;
            case 5: return shadow_reach;
            case 6: return improved_mind_blast;
            case 7: return improved_psychic_scream;
            case 8: return mind_flay;
            case 9: return improved_mind_flay;
            case 10: return improved_fade;
            case 11: return vampiric_embrace;
            case 12: return shadow_weaving;
            case 13: return silence;
            case 14: return devouring_contagion;
            case 15: return early_demise;
            case 16: return darkness;
            case 17: return shadowform;
            default: return shadow_focus;
        }
    }

    int get_points_by_index(size_t idx) const {
        switch (idx) {
            case 0: return shadow_focus;
            case 1: return blackout;
            case 2: return spirit_tap;
            case 3: return shadow_affinity;
            case 4: return improved_shadow_word_pain;
            case 5: return shadow_reach;
            case 6: return improved_mind_blast;
            case 7: return improved_psychic_scream;
            case 8: return mind_flay;
            case 9: return improved_mind_flay;
            case 10: return improved_fade;
            case 11: return vampiric_embrace;
            case 12: return shadow_weaving;
            case 13: return silence;
            case 14: return devouring_contagion;
            case 15: return early_demise;
            case 16: return darkness;
            case 17: return shadowform;
            default: return 0;
        }
    }
};

struct Talents {
    DisciplineTalents disc;
    HolyTalents holy;
    ShadowTalents shadow;

    int total_points() const {
        return disc.total_points() + holy.total_points() + shadow.total_points();
    }

    // Canonical builds
    static Talents create_forever_shadow() {
        Talents t;
        // Standard Shadow (14/0/37) - 51 points
        t.disc.twin_disciplines = 5;
        t.disc.silent_resolve = 2;
        t.disc.improved_power_word_shield = 3;
        t.disc.inner_focus = 1;
        t.disc.meditation = 3;

        t.shadow.shadow_focus = 5;
        t.shadow.spirit_tap = 5;
        t.shadow.improved_shadow_word_pain = 2;
        t.shadow.shadow_reach = 2;
        t.shadow.improved_mind_blast = 5;
        t.shadow.mind_flay = 1;
        t.shadow.improved_mind_flay = 2;
        t.shadow.vampiric_embrace = 1;
        t.shadow.shadow_weaving = 3;
        t.shadow.silence = 1;
        t.shadow.devouring_contagion = 2;
        t.shadow.early_demise = 2;
        t.shadow.darkness = 5;
        t.shadow.shadowform = 1;
        return t;
    }

    static Talents create_forever_smite() {
        Talents t;
        // Smite / Holy DPS (14/37/0) - 51 points
        t.disc.power_in_light = 5;
        t.disc.twin_disciplines = 5;
        t.disc.inner_focus = 1;
        t.disc.meditation = 3;

        t.holy.twilight_focus = 3;
        t.holy.holy_specialization = 5;
        t.holy.divine_fury = 5;
        t.holy.holy_reach = 2;
        t.holy.searing_light = 2;
        t.holy.spiritual_guidance = 5;
        t.holy.holy_nova = 1;
        t.holy.improved_renew = 3;
        t.holy.improved_healing = 3;
        t.holy.spiritual_healing = 3;
        t.holy.prayer_of_mending = 1;
        t.holy.spirit_of_redemption = 1;
        t.holy.litany_of_light = 2;
        t.holy.blessed_recovery = 1;
        return t;
    }
};

inline const std::array<TalentNodeDef, 18>& get_disc_nodes() {
    static const std::array<TalentNodeDef, 18> nodes = {
        TalentNodeDef{"power_in_light", "Power in Light", 1, 1, 5, "spell_holy_searinglight", nullptr, {"Your Smite and Penance spells deal 2% increased damage to targets afflicted with your Holy Fire.", "Your Smite and Penance spells deal 4% increased damage to targets afflicted with your Holy Fire.", "Your Smite and Penance spells deal 6% increased damage to targets afflicted with your Holy Fire.", "Your Smite and Penance spells deal 8% increased damage to targets afflicted with your Holy Fire.", "Your Smite and Penance spells deal 10% increased damage to targets afflicted with your Holy Fire."}},
        TalentNodeDef{"wand_specialization", "Wand Specialization", 1, 2, 2, "inv_wand_01", nullptr, {"Increases your damage with Wands by 13%.", "Increases your damage with Wands by 25%.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"twin_disciplines", "Twin Disciplines", 1, 3, 5, "spell_holy_sealofvengeance", nullptr, {"Increases the damage and healing of your instant cast spells by 1%.", "Increases the damage and healing of your instant cast spells by 2%.", "Increases the damage and healing of your instant cast spells by 3%.", "Increases the damage and healing of your instant cast spells by 4%.", "Increases the damage and healing of your instant cast spells by 5%."}},
        TalentNodeDef{"silent_resolve", "Silent Resolve", 2, 1, 3, "spell_nature_manaregentotem", nullptr, {"Reduces the threat generated by your Holy spells by 10% and reduces the duration of Stun, Fear, and Silence effects inflicted on you by 5%.", "Reduces the threat generated by your Holy spells by 20% and reduces the duration of Stun, Fear, and Silence effects inflicted on you by 10%.", "Reduces the threat generated by your Holy spells by 30% and reduces the duration of Stun, Fear, and Silence effects inflicted on you by 15%.", nullptr, nullptr}},
        TalentNodeDef{"holy_precision", "Holy Precision", 2, 2, 3, "spell_holy_divineillumination", nullptr, {"Improves your chance to hit with Holy spells by 6%.", "Improves your chance to hit with Holy spells by 12%.", "Improves your chance to hit with Holy spells by 18%.", nullptr, nullptr}},
        TalentNodeDef{"improved_power_word_shield", "Improved Power Word: Shield", 2, 3, 3, "spell_holy_powerwordshield", nullptr, {"Increases the damage absorbed by your Power Word: Shield by 7%.", "Increases the damage absorbed by your Power Word: Shield by 14%.", "Increases the damage absorbed by your Power Word: Shield by 20%.", nullptr, nullptr}},
        TalentNodeDef{"martyrdom", "Martyrdom", 2, 4, 2, "spell_nature_tranquility", nullptr, {"Gives you a 50% chance to gain Focused Casting for 6 sec after being the victim of a melee or ranged critical strike. The Focused Casting effect prevents you from losing casting time when taking damage and increases your resistance to Interrupt effects by 20%.", "Gives you a 100% chance to gain Focused Casting for 6 sec after being the victim of a melee or ranged critical strike. The Focused Casting effect prevents you from losing casting time when taking damage and increases your resistance to Interrupt effects by 20%.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"mental_agility", "Mental Agility", 3, 1, 3, "ability_hibernation", nullptr, {"Reduces the mana cost of your Smite, Holy Fire, and instant cast spells by 3%.", "Reduces the mana cost of your Smite, Holy Fire, and instant cast spells by 7%.", "Reduces the mana cost of your Smite, Holy Fire, and instant cast spells by 10%.", nullptr, nullptr}},
        TalentNodeDef{"inner_focus", "Inner Focus", 3, 2, 1, "spell_frost_windwalkon", nullptr, {"When activated, reduces the Mana cost of your next spell by 100% and increases its critical effect chance by 25% if it is capable of a critical effect.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"meditation", "Meditation", 3, 4, 3, "spell_nature_sleep", nullptr, {"Allows 17% of your Mana regeneration to continue while casting.", "Allows 33% of your Mana regeneration to continue while casting.", "Allows 50% of your Mana regeneration to continue while casting.", nullptr, nullptr}},
        TalentNodeDef{"improved_inner_fire", "Improved Inner Fire", 4, 1, 3, "spell_holy_innerfire", nullptr, {"Increases the Armor bonus of your Inner Fire spell by 15% and increases its total charges by 4.", "Increases the Armor bonus of your Inner Fire spell by 30% and increases its total charges by 8.", "Increases the Armor bonus of your Inner Fire spell by 45% and increases its total charges by 12.", nullptr, nullptr}},
        TalentNodeDef{"mental_strength", "Mental Strength", 4, 2, 5, "spell_nature_enchantarmor", nullptr, {"Increases your total Intellect by 3%.", "Increases your total Intellect by 6%.", "Increases your total Intellect by 9%.", "Increases your total Intellect by 12%.", "Increases your total Intellect by 15%."}},
        TalentNodeDef{"soul_warding", "Soul Warding", 4, 3, 1, "spell_holy_pureofheart", "Improved Power Word: Shield", {"Reduces the cooldown on your Power Word: Shield spell by 4 sec and reduces its mana cost by 15%.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"improved_mana_burn", "Improved Mana Burn", 4, 4, 2, "spell_shadow_manaburn", nullptr, {"Reduces the casting time of your Mana Burn spell by 0.5 sec.", "Reduces the casting time of your Mana Burn spell by 1.0 sec.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"penance", "Penance", 5, 2, 1, "spell_holy_penance", nullptr, {"Launches a volley of holy light at the target, causing (28.5% of Spell Power) Holy damage to an enemy, or (28.5% of Spell Power) healing to an ally, instantly and every 1 sec for 2 sec.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"renewed_hope", "Renewed Hope", 5, 3, 5, "spell_holy_holyprotection", "Soul Warding", {"Your heals from Flash Heal, Binding Heal, Lesser Heal, Heal, Greater Heal, and Penance gain 2% increased critical strike chance when cast on targets with Weakened Soul, and reduce the remaining duration of Weakened Soul on their target by 1 sec.", "Your heals from Flash Heal, Binding Heal, Lesser Heal, Heal, Greater Heal, and Penance gain 4% increased critical strike chance when cast on targets with Weakened Soul, and reduce the remaining duration of Weakened Soul on their target by 2 sec.", "Your heals from Flash Heal, Binding Heal, Lesser Heal, Heal, Greater Heal, and Penance gain 6% increased critical strike chance when cast on targets with Weakened Soul, and reduce the remaining duration of Weakened Soul on their target by 3 sec.", "Your heals from Flash Heal, Binding Heal, Lesser Heal, Heal, Greater Heal, and Penance gain 8% increased critical strike chance when cast on targets with Weakened Soul, and reduce the remaining duration of Weakened Soul on their target by 4 sec.", "Your heals from Flash Heal, Binding Heal, Lesser Heal, Heal, Greater Heal, and Penance gain 10% increased critical strike chance when cast on targets with Weakened Soul, and reduce the remaining duration of Weakened Soul on their target by 5 sec."}},
        TalentNodeDef{"divine_aegis", "Divine Aegis", 6, 3, 3, "spell_holy_devineaegis", nullptr, {"Your critical heals create a protective shield on the target, absorbing 5% of the amount healed. Lasts 12 sec.", "Your critical heals create a protective shield on the target, absorbing 10% of the amount healed. Lasts 12 sec.", "Your critical heals create a protective shield on the target, absorbing 15% of the amount healed. Lasts 12 sec.", nullptr, nullptr}},
        TalentNodeDef{"power_infusion", "Power Infusion", 7, 2, 1, "spell_holy_powerinfusion", "Penance", {"Infuses the target with power, increasing their spell damage and healing done by 20% for 15 sec.", nullptr, nullptr, nullptr, nullptr}},
    };
    return nodes;
}

inline const std::array<TalentNodeDef, 17>& get_holy_nodes() {
    static const std::array<TalentNodeDef, 17> nodes = {
        TalentNodeDef{"twilight_focus", "Twilight Focus", 1, 1, 3, "spell_holy_healingfocus", nullptr, {"Gives you a 23% chance to avoid interruption caused by damage while casting any spell.", "Gives you a 47% chance to avoid interruption caused by damage while casting any spell.", "Gives you a 70% chance to avoid interruption caused by damage while casting any spell.", nullptr, nullptr}},
        TalentNodeDef{"improved_renew", "Improved Renew", 1, 2, 3, "spell_holy_renew", nullptr, {"Increases the amount healed by your Renew spell by 5%.", "Increases the amount healed by your Renew spell by 10%.", "Increases the amount healed by your Renew spell by 15%.", nullptr, nullptr}},
        TalentNodeDef{"holy_specialization", "Holy Specialization", 1, 3, 5, "spell_holy_sealofsalvation", nullptr, {"Increases the critical effect chance of your Holy spells by 1%.", "Increases the critical effect chance of your Holy spells by 2%.", "Increases the critical effect chance of your Holy spells by 3%.", "Increases the critical effect chance of your Holy spells by 4%.", "Increases the critical effect chance of your Holy spells by 5%."}},
        TalentNodeDef{"spell_warding", "Spell Warding", 2, 2, 5, "spell_holy_spellwarding", nullptr, {"Reduces all spell damage taken by 2%.", "Reduces all spell damage taken by 4%.", "Reduces all spell damage taken by 6%.", "Reduces all spell damage taken by 8%.", "Reduces all spell damage taken by 10%."}},
        TalentNodeDef{"divine_fury", "Divine Fury", 2, 3, 5, "spell_holy_sealofwrath", nullptr, {"Reduces the casting time of your Smite, Holy Fire, Heal, and Greater Heal spells by 0.1 sec.", "Reduces the casting time of your Smite, Holy Fire, Heal, and Greater Heal spells by 0.2 sec.", "Reduces the casting time of your Smite, Holy Fire, Heal, and Greater Heal spells by 0.3 sec.", "Reduces the casting time of your Smite, Holy Fire, Heal, and Greater Heal spells by 0.4 sec.", "Reduces the casting time of your Smite, Holy Fire, Heal, and Greater Heal spells by 0.5 sec."}},
        TalentNodeDef{"holy_nova", "Holy Nova", 3, 1, 1, "spell_holy_holynova", nullptr, {"Causes an explosion of holy light around the caster, causing (10.7% of Spell Power) Holy damage to all enemy targets within 10 yards and healing all party members within 10 yards for (10.7% of Spell Power). These effects cause no threat.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"blessed_recovery", "Blessed Recovery", 3, 2, 3, "spell_holy_blessedrecovery", nullptr, {"After being struck by a melee or ranged critical hit, or suffering more than 30% of your maximum Health from a single attack, heal 8% of the damage taken over 6 sec. Refreshing this effect carries over any remaining healing.", "After being struck by a melee or ranged critical hit, or suffering more than 30% of your maximum Health from a single attack, heal 17% of the damage taken over 6 sec. Refreshing this effect carries over any remaining healing.", "After being struck by a melee or ranged critical hit, or suffering more than 30% of your maximum Health from a single attack, heal 25% of the damage taken over 6 sec. Refreshing this effect carries over any remaining healing.", nullptr, nullptr}},
        TalentNodeDef{"inspiration", "Inspiration", 3, 4, 3, "spell_holy_layonhands", nullptr, {"Your non-periodic critical heals increase your target's Armor by 8% for 15 sec.", "Your non-periodic critical heals increase your target's Armor by 17% for 15 sec.", "Your non-periodic critical heals increase your target's Armor by 25% for 15 sec.", nullptr, nullptr}},
        TalentNodeDef{"holy_reach", "Holy Reach", 4, 1, 2, "spell_holy_purify", nullptr, {"Increases the range of your Smite and Holy Fire spells and the radius of your Prayer of Healing and Holy Nova spells by 10%.", "Increases the range of your Smite and Holy Fire spells and the radius of your Prayer of Healing and Holy Nova spells by 20%.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"improved_healing", "Improved Healing", 4, 2, 3, "spell_holy_heal02", nullptr, {"Reduces the Mana cost of your Lesser Heal, Heal, Greater Heal, Penance, and Prayer of Mending spells by 5%.", "Reduces the Mana cost of your Lesser Heal, Heal, Greater Heal, Penance, and Prayer of Mending spells by 10%.", "Reduces the Mana cost of your Lesser Heal, Heal, Greater Heal, Penance, and Prayer of Mending spells by 15%.", nullptr, nullptr}},
        TalentNodeDef{"searing_light", "Searing Light", 4, 3, 2, "spell_holy_searinglightpriest", "Divine Fury", {"Increases your Holy damage done by 2%, and gives a 5% chance each time your Holy Fire spell deals periodic damage for your next Holy Nova to cost no Mana.", "Increases your Holy damage done by 5%, and gives a 10% chance each time your Holy Fire spell deals periodic damage for your next Holy Nova to cost no Mana.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"binding_heal", "Binding Heal", 4, 4, 1, "spell_holy_blindingheal", nullptr, {"Heals a friendly target and the caster for (42.9% of Spell Power). Low threat.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"litany_of_light", "Litany of Light", 5, 1, 2, "inv_scroll_07", nullptr, {"When you cast a healing spell, gain Mana equal to 5% of the base cost of the spell if your previous heal was a different spell.", "When you cast a healing spell, gain Mana equal to 10% of the base cost of the spell if your previous heal was a different spell.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"spirit_of_redemption", "Spirit of Redemption", 5, 2, 1, "inv_enchant_essenceeternallarge", nullptr, {"Upon death, the priest becomes the Spirit of Redemption for 15 sec.  The Spirit of Redemption cannot move, attack, be attacked or targeted by any spells or effects.  While in this form the priest can cast any healing spell free of cost.  When the effect ends, the priest dies.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"spiritual_guidance", "Spiritual Guidance", 5, 3, 5, "spell_holy_spiritualguidence", nullptr, {"Increases your spell healing by up to 5% of your total Spirit and your spell damage by up to 1% of your total Spirit.", "Increases your spell healing by up to 10% of your total Spirit and your spell damage by up to 3% of your total Spirit.", "Increases your spell healing by up to 15% of your total Spirit and your spell damage by up to 5% of your total Spirit.", "Increases your spell healing by up to 20% of your total Spirit and your spell damage by up to 6% of your total Spirit.", "Increases your spell healing by up to 25% of your total Spirit and your spell damage by up to 8% of your total Spirit."}},
        TalentNodeDef{"spiritual_healing", "Spiritual Healing", 6, 3, 3, "spell_nature_moonglow", nullptr, {"Increases the amount healed by your spells by 3%.", "Increases the amount healed by your spells by 7%.", "Increases the amount healed by your spells by 10%.", nullptr, nullptr}},
        TalentNodeDef{"prayer_of_mending", "Prayer of Mending", 7, 2, 1, "spell_holy_prayerofmendingtga", "Spirit of Redemption", {"Places a spell on the target that heals them for [(172 + (Healing * 0.42899999)) * (1 * 1)] the next time they take damage or receive non-periodic healing. When the heal occurs, Prayer of Mending jumps to a party or raid member within 20 yards. Jumps up to 5 times and lasts 30 sec after each jump. This spell can only be placed on one target at a time per caster.", nullptr, nullptr, nullptr, nullptr}},
    };
    return nodes;
}

inline const std::array<TalentNodeDef, 18>& get_shadow_nodes() {
    static const std::array<TalentNodeDef, 18> nodes = {
        TalentNodeDef{"shadow_focus", "Shadow Focus", 1, 1, 5, "spell_shadow_burningspirit", nullptr, {"Improves your chance to hit with Shadow spells by 1%.", "Improves your chance to hit with Shadow spells by 2%.", "Improves your chance to hit with Shadow spells by 3%.", "Improves your chance to hit with Shadow spells by 4%.", "Improves your chance to hit with Shadow spells by 5%."}},
        TalentNodeDef{"blackout", "Blackout", 1, 2, 5, "spell_shadow_gathershadows", nullptr, {"Gives your Shadow damage spells a 2% chance to stun the target for 3 sec.", "Gives your Shadow damage spells a 4% chance to stun the target for 3 sec.", "Gives your Shadow damage spells a 6% chance to stun the target for 3 sec.", "Gives your Shadow damage spells a 8% chance to stun the target for 3 sec.", "Gives your Shadow damage spells a 10% chance to stun the target for 3 sec."}},
        TalentNodeDef{"spirit_tap", "Spirit Tap", 1, 3, 5, "spell_shadow_requiem", nullptr, {"Gives you a 20% chance to gain a 100% bonus to your Spirit for 15 sec after killing a non-trivial target. For the duration, your Mana will regenerate at a 50% of normal rate while casting.", "Gives you a 40% chance to gain a 100% bonus to your Spirit for 15 sec after killing a non-trivial target. For the duration, your Mana will regenerate at a 50% of normal rate while casting.", "Gives you a 60% chance to gain a 100% bonus to your Spirit for 15 sec after killing a non-trivial target. For the duration, your Mana will regenerate at a 50% of normal rate while casting.", "Gives you a 80% chance to gain a 100% bonus to your Spirit for 15 sec after killing a non-trivial target. For the duration, your Mana will regenerate at a 50% of normal rate while casting.", "Gives you a 100% chance to gain a 100% bonus to your Spirit for 15 sec after killing a non-trivial target. For the duration, your Mana will regenerate at a 50% of normal rate while casting."}},
        TalentNodeDef{"shadow_affinity", "Shadow Affinity", 2, 1, 3, "spell_shadow_shadowward", nullptr, {"Reduces the threat generated by your Shadow spells by 10%.", "Reduces the threat generated by your Shadow spells by 20%.", "Reduces the threat generated by your Shadow spells by 30%.", nullptr, nullptr}},
        TalentNodeDef{"improved_shadow_word_pain", "Improved Shadow Word: Pain", 2, 3, 2, "spell_shadow_shadowwordpain", nullptr, {"Increases the duration of your Shadow Word: Pain spell by 3 sec.", "Increases the duration of your Shadow Word: Pain spell by 6 sec.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"shadow_reach", "Shadow Reach", 2, 4, 2, "spell_shadow_chilltouch", nullptr, {"Increases the range of your offensive Shadow spells by 10%.", "Increases the range of your offensive Shadow spells by 20%.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"improved_mind_blast", "Improved Mind Blast", 3, 1, 5, "spell_shadow_unholyfrenzy", nullptr, {"Reduces the cooldown of your Mind Blast spell by 0.5 sec.", "Reduces the cooldown of your Mind Blast spell by 1 sec.", "Reduces the cooldown of your Mind Blast spell by 1.5 sec.", "Reduces the cooldown of your Mind Blast spell by 2 sec.", "Reduces the cooldown of your Mind Blast spell by 2.5 sec."}},
        TalentNodeDef{"improved_psychic_scream", "Improved Psychic Scream", 3, 2, 2, "spell_shadow_psychicscream", "Blackout", {"Reduces the cooldown of your Psychic Scream spell by 2 sec.", "Reduces the cooldown of your Psychic Scream spell by 4 sec.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"mind_flay", "Mind Flay", 3, 3, 1, "spell_shadow_siphonmana", nullptr, {"Assault the target's mind with Shadow energy, causing (50.1% of Spell Power) Shadow damage over 3 sec  and slowing their movement speed by 50%.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"improved_mind_flay", "Improved Mind Flay", 3, 4, 2, "spell_shadow_soulleech_2", "Mind Flay", {"Your Mind Flay now deals 10% more damage, gains 5 yards increased range, but slows the target's movement speed by 35%.", "Your Mind Flay now deals 20% more damage, gains 10 yards increased range, but slows the target's movement speed by 20%.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"improved_fade", "Improved Fade", 4, 1, 2, "spell_magic_lesserinvisibilty", nullptr, {"Decreases the cooldown of your Fade ability by 3 sec.", "Decreases the cooldown of your Fade ability by 6 sec.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"vampiric_embrace", "Vampiric Embrace", 4, 2, 1, "spell_shadow_unsummonbuilding", nullptr, {"Afflicts your target with Shadow energy that causes all party members to be healed for 20% of any Shadow spell damage you deal for 30 sec. Vampiric Embrace also grants a chance for your Spirit Tap talent to trigger when enemies afflicted by it die.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"shadow_weaving", "Shadow Weaving", 4, 3, 3, "spell_shadow_blackplague", nullptr, {"Your Shadow damage spells have a 33% chance to increase the Shadow damage you deal by 2% for 15 sec, stacking up to 5 times.", "Your Shadow damage spells have a 67% chance to increase the Shadow damage you deal by 2% for 15 sec, stacking up to 5 times.", "Your Shadow damage spells have a 100% chance to increase the Shadow damage you deal by 2% for 15 sec, stacking up to 5 times.", nullptr, nullptr}},
        TalentNodeDef{"silence", "Silence", 5, 1, 1, "spell_shadow_impphaseshift", nullptr, {"Silences the target, preventing them from casting spells for 5 sec and interrupting their spellcasts for 3 sec.", nullptr, nullptr, nullptr, nullptr}},
        TalentNodeDef{"devouring_contagion", "Devouring Contagion", 5, 3, 2, "spell_shadow_devouringplague", nullptr, {"Reduces the mana cost of your Devouring Plague by 25%.\\n\\nTargets that die while Devouring Plague it is active spreads it, jumping to a nearby enemy within 5 yards for the remaining duration.", "Reduces the mana cost of your Devouring Plague by 50%.\\n\\nTargets that die while Devouring Plague it is active spreads it, jumping to a nearby enemy within 10 yards for the remaining duration.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"early_demise", "Early Demise", 6, 1, 2, "spell_shadow_demonicfortitude", nullptr, {"Increases Shadow Word: Death's critical strike chance on targets at or below 20% health by 15%.", "Increases Shadow Word: Death's critical strike chance on targets at or below 20% health by 30%.", nullptr, nullptr, nullptr}},
        TalentNodeDef{"darkness", "Darkness", 6, 3, 5, "spell_shadow_twilight", nullptr, {"Increases your Shadow damage done by 2%.", "Increases your Shadow damage done by 4%.", "Increases your Shadow damage done by 6%.", "Increases your Shadow damage done by 8%.", "Increases your Shadow damage done by 10%."}},
        TalentNodeDef{"shadowform", "Shadowform", 7, 2, 1, "spell_shadow_shadowform", "Vampiric Embrace", {"Assume Shadowform, increasing your Shadow damage by 10%, reducing the Mana cost of all Shadow spells by 50%, increasing the critical strike damage bonus of your Shadow spells by 100%, and reducing Physical damage taken by you by 15%. However, you may not cast healing spells while in this form.", nullptr, nullptr, nullptr, nullptr}},
    };
    return nodes;
}

} // namespace priest
