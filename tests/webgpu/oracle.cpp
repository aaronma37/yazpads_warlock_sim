// Independent golden fixtures from the unchanged production CPU simulator.
#include "src/sim/warlock/warlock_sim.hpp"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace warlock;
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Usage: webgpu_oracle output.json\n";return 1;}
    std::ofstream out(argv[1]);if(!out)return 2;
    out<<std::setprecision(12)<<"{\"iterations\":10000,\"fixtures\":[";
    for(int scenario=0;scenario<5;scenario++) {
        WarlockSimulator sim;
        sim.race=Race::HUMAN;sim.use_raw_stats=true;sim.raw_stats=Stats{};
        sim.raw_stats.intellect=200;sim.raw_stats.spirit=0;sim.raw_stats.spell_power=500;
        sim.raw_stats.spell_hit_percent=12;sim.raw_stats.mp5=20;
        sim.raw_stats.spell_haste_percent=scenario==2?15:0;
        sim.buffs=BuffConfig{};sim.buffs.curse_of_elements=false;sim.buffs.curse_of_shadows=false;sim.buffs.sacrifice_imp=false;
        sim.talents=Talents{};sim.talents.destro.bane=5;sim.talents.destro.ruin=5;
        sim.talents.destro.improved_shadow_bolt=(scenario>=3)?5:0;
        sim.talents.aff.improved_corruption=5;sim.talents.aff.improved_life_tap=2;
        sim.talents.aff.nightfall=(scenario==1||scenario==2||scenario==4)?2:0;
        sim.mechanics.projectile_travel_time=false;sim.mechanics.partial_resists_enabled=false;sim.mechanics.spell_piercing_below_zero=false;
        sim.mechanics.isb_has_charges=(scenario==3);
        sim.policy.pet=PetChoice::NONE;sim.policy.use_trinkets_on_cooldown=false;sim.policy.use_custom_apl=true;
        sim.policy.life_tap_threshold_pct=30;sim.policy.custom_rules.clear();
        auto rule=[&](PriorityAction action,bool trance=false){PriorityRule r;r.action=action;r.check_shadow_trance=trance;sim.policy.custom_rules.push_back(r);};
        rule(PriorityAction::LIFE_TAP);rule(PriorityAction::NIGHTFALL_SHADOW_BOLT,true);
        if(scenario==1||scenario==2||scenario==4)rule(PriorityAction::CORRUPTION);
        rule(PriorityAction::SHADOW_BOLT_FILLER);
        sim.fight_duration=scenario==2?300.123:180.123;sim.randomize_duration=false;
        Stats effective=sim.raw_stats;
        const auto base=get_base_attributes_for_race(sim.race);
        sim.buffs.apply_to_stats(effective,base,true,true);
        sim.raw_stats.spell_crit_percent=20-base.base_spell_crit-effective.intellect/60.6-2;
        effective=sim.raw_stats;sim.buffs.apply_to_stats(effective,base,true,true);
        double mean=0,m2=0,bolts=0,dots=0,ticks=0,taps=0,procs=0;
        const auto start=std::chrono::steady_clock::now();
        for(int i=0;i<10000;i++){
            FastRNG rng(42+uint64_t(i)*7919);const auto r=sim.run_single_simulation(rng);
            const double delta=r.dps-mean;mean+=delta/(i+1);m2+=delta*(r.dps-mean);
            bolts+=r.shadow_bolt_casts;dots+=r.spell_stats[size_t(SpellID::CORRUPTION)].casts;
            ticks+=r.spell_stats[size_t(SpellID::CORRUPTION)].hits;taps+=r.life_taps;procs+=r.nightfall_procs;
        }
        const double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        if(scenario)out<<",";
        const char* name = (scenario==0?"bolt_tap":scenario==1?"corruption_nightfall":scenario==2?"haste_nightfall":scenario==3?"isb_charges_bolt":"isb_window_corruption");
        out<<"{\"name\":\""<<name<<"\",\"nativeCpuMs\":"<<elapsed<<",\"config\":{";
        out<<"\"duration\":"<<sim.fight_duration<<",\"castTime\":"<<2.5/(1+sim.raw_stats.spell_haste_percent*.01)<<",\"gcd\":1.5,\"maxMana\":"<<effective.max_mana;
        out<<",\"spellPower\":500,\"hit\":"<<sim.calculate_hit_chance(School::SHADOW)<<",\"crit\":"<<sim.calculate_crit_chance(School::SHADOW,effective);
        out<<",\"boltCrit\":2,\"dotCrit\":1.5,\"tapGain\":"<<(430+effective.spirit)*1.2<<",\"mp5\":20,\"tapThreshold\":0.3,\"corruption\":"<<((scenario==1||scenario==2||scenario==4)?1:0);
        out<<",\"nightfall\":"<<((scenario==1||scenario==2||scenario==4)?0.04:0)<<",\"boltCost\":370,\"corruptionCost\":290,\"boltMin\":246,\"boltMax\":274,\"dotBase\":57,\"dotMultiplier\":1.1";
        out<<",\"isbBonus\":"<<(sim.talents.destro.improved_shadow_bolt*0.04)<<",\"isbCharges\":"<<(sim.talents.destro.improved_shadow_bolt?(sim.mechanics.isb_has_charges?4:0):0);
        out<<",\"isbAllShadow\":"<<(sim.talents.destro.improved_shadow_bolt?((!sim.mechanics.isb_has_charges||sim.mechanics.isb_all_shadow_sources)?1:0):0)<<",\"pad0\":0},\"summary\":{";
        out<<"\"meanDps\":"<<mean<<",\"standardError\":"<<std::sqrt(m2/9999/10000)<<",\"bolts\":"<<bolts/10000<<",\"dots\":"<<dots/10000<<",\"ticks\":"<<ticks/10000<<",\"taps\":"<<taps/10000<<",\"procs\":"<<procs/10000<<"}}";
    }
    out<<"]}\n";
    return 0;
}
