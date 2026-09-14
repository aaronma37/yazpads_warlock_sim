#include "test_framework.hpp"
#include "src/sim/buffs.hpp"
#include "src/sim/stats.hpp"

using namespace warlock;

TEST_CASE(Buffs, ConsumablesSpellPower) {
    BuffConfig buffs;
    // Turn off raid/world buffs to isolate consumables
    buffs.arcane_intellect = false;
    buffs.blessing_of_kings = false;
    buffs.blessing_of_wisdom = false;
    buffs.mark_of_the_wild = false;
    buffs.rallying_cry = false;
    buffs.songflower = false;
    buffs.spirit_of_zandalar = false;
    buffs.warchiefs_blessing = false;
    buffs.sayges_fortune = false;
    buffs.shadow_weaving = false;
    buffs.curse_of_shadows = false;
    buffs.curse_of_elements = false;
    buffs.nightfall_axe = false;
    buffs.sacrifice_imp = false;
    buffs.sacrifice_succubus = false;

    buffs.flask_of_supreme_power = true; // +150 SP
    buffs.greater_arcane_elixir = true;  // +35 SP
    buffs.elixir_of_shadow_power = true; // +40 Shadow SP
    buffs.brilliant_wizard_oil = true;   // +36 SP, +1% crit

    Stats stats;
    BaseAttributes base;
    buffs.apply_to_stats(stats, base, true);

    CHECK_NEAR(stats.spell_power, 150.0 + 35.0 + 36.0, 0.001);
    CHECK_NEAR(stats.shadow_power, 40.0, 0.001);
    CHECK_NEAR(stats.spell_crit_percent, 1.0, 0.001);
}

TEST_CASE(Buffs, WorldBuffCritAndDamage) {
    BuffConfig buffs;
    buffs.flask_of_supreme_power = false;
    buffs.greater_arcane_elixir = false;
    buffs.elixir_of_shadow_power = false;
    buffs.brilliant_wizard_oil = false;
    buffs.arcane_intellect = false;
    buffs.blessing_of_kings = false;
    buffs.blessing_of_wisdom = false;
    buffs.mark_of_the_wild = false;
    buffs.spirit_of_zandalar = false;
    buffs.warchiefs_blessing = false;
    buffs.shadow_weaving = false;
    buffs.curse_of_shadows = false;
    buffs.curse_of_elements = false;
    buffs.nightfall_axe = false;
    buffs.sacrifice_imp = false;
    buffs.sacrifice_succubus = false;

    buffs.rallying_cry = true;  // +10% spell crit
    buffs.songflower = true;    // +5% spell crit, +15 stats
    buffs.sayges_fortune = true; // +10% all damage

    Stats stats;
    BaseAttributes base;
    buffs.apply_to_stats(stats, base, true);

    CHECK_NEAR(stats.spell_crit_percent, 15.0, 0.001);
    CHECK_NEAR(stats.all_damage_multiplier, 1.10, 0.001);
}

TEST_CASE(Buffs, DemonicSacrificeMultipliers) {
    BuffConfig buffs;
    buffs.flask_of_supreme_power = false;
    buffs.greater_arcane_elixir = false;
    buffs.elixir_of_shadow_power = false;
    buffs.brilliant_wizard_oil = false;
    buffs.arcane_intellect = false;
    buffs.blessing_of_kings = false;
    buffs.blessing_of_wisdom = false;
    buffs.mark_of_the_wild = false;
    buffs.spirit_of_zandalar = false;
    buffs.warchiefs_blessing = false;
    buffs.rallying_cry = false;
    buffs.songflower = false;
    buffs.sayges_fortune = false;
    buffs.shadow_weaving = false;
    buffs.curse_of_shadows = false;
    buffs.curse_of_elements = false;
    buffs.nightfall_axe = false;

    // In WoW Forever: Sac Imp -> +15% Shadow
    buffs.sacrifice_imp = true;
    buffs.sacrifice_succubus = false;
    Stats stats_imp;
    BaseAttributes base;
    buffs.apply_to_stats(stats_imp, base, true);
    CHECK_NEAR(stats_imp.shadow_multiplier, 1.15, 0.001);

    // In WoW Forever: Sac Succubus -> +15% Fire
    buffs.sacrifice_imp = false;
    buffs.sacrifice_succubus = true;
    Stats stats_succ;
    buffs.apply_to_stats(stats_succ, base, true);
    CHECK_NEAR(stats_succ.fire_multiplier, 1.15, 0.001);
}

TEST_CASE(Buffs, PersonalShadowWeavingMechanic) {
    BuffConfig buffs;
    buffs.curse_of_shadows = false;
    buffs.curse_of_elements = false;
    buffs.sacrifice_imp = false;
    buffs.sacrifice_succubus = false;
    buffs.shadow_weaving = true;
    BaseAttributes base;

    // When personal_shadow_weaving is true (default in WoW Forever), warlock does not gain the 1.15x shadow multiplier
    Stats stats_personal;
    buffs.apply_to_stats(stats_personal, base, true, true);
    CHECK_NEAR(stats_personal.shadow_multiplier, 1.00, 0.001);

    // When personal_shadow_weaving is false (shared debuff mode), warlock gains the 1.15x shadow multiplier
    Stats stats_shared;
    buffs.apply_to_stats(stats_shared, base, true, false);
    CHECK_NEAR(stats_shared.shadow_multiplier, 1.15, 0.001);
}


