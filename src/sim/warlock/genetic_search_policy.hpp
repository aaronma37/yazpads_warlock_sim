#pragma once
#include "policy.hpp"
#include "mechanics.hpp"
#include <array>

namespace warlock::genetic_detail {

constexpr size_t rotation_count = static_cast<size_t>(RotationChoice::DP_AF_SHADOW_BRAND) + 1;

// Policy genes are independent of talent archetypes. Talents only restrict legality.
struct SearchPolicy {
    PetChoice pet = PetChoice::NONE;
    bool sac_imp = false;
    bool sac_succubus = false;
    RotationChoice rotation = RotationChoice::SHADOW_DESTRO;
    bool maintain_immolate = true;
    CurseChoice curse = CurseChoice::BANE_OF_AGONY;
    ShadowburnPolicy shadowburn = ShadowburnPolicy::ON_COOLDOWN;
    bool use_decimation_soul_fire = true;
    bool channel_drain_hope = true;

    bool operator==(const SearchPolicy&) const = default;
};

inline void mutate_policy(SearchPolicy& p, const Talents& talents, FastRNG& rng, double rate) {
    if (rng.next_double() < rate) {
        std::array<SearchPolicy, 7> modes{};
        size_t count = 0;
        auto add = [&](PetChoice pet, bool imp, bool succ) {
            modes[count].pet = pet;
            modes[count].sac_imp = imp;
            modes[count++].sac_succubus = succ;
        };
        add(PetChoice::NONE, false, false);
        add(PetChoice::IMP, false, false);
        add(PetChoice::SUCCUBUS, false, false);
        if (talents.demo.demonic_sacrifice || talents.demo.demonic_pact) {
            add(PetChoice::NONE, true, false);
            add(PetChoice::NONE, false, true);
        }
        if (talents.demo.demonic_pact) {
            add(PetChoice::SUCCUBUS, true, false);
            add(PetChoice::IMP, false, true);
        }
        const auto& mode = modes[rng.next_u64() % count];
        p.pet = mode.pet;
        p.sac_imp = mode.sac_imp;
        p.sac_succubus = mode.sac_succubus;
    }
    if (rng.next_double() < rate) p.rotation = static_cast<RotationChoice>(rng.next_u64() % rotation_count);
    if (rng.next_double() < rate) p.maintain_immolate = rng.next_u64() % 2;
    if (rng.next_double() < rate) p.curse = static_cast<CurseChoice>(rng.next_u64() % 3);
    if (rng.next_double() < rate) p.shadowburn = static_cast<ShadowburnPolicy>(rng.next_u64() % 3);
    if (rng.next_double() < rate) p.use_decimation_soul_fire = rng.next_u64() % 2;
    if (rng.next_double() < rate) p.channel_drain_hope = rng.next_u64() % 2;
}

inline SearchPolicy crossover_policy(const SearchPolicy& a, const SearchPolicy& b, FastRNG& rng) {
    // Keep the pet/sacrifice bundle intact; recombine all other genes independently.
    SearchPolicy child = rng.next_u64() % 2 ? a : b;
    child.rotation = rng.next_u64() % 2 ? a.rotation : b.rotation;
    child.maintain_immolate = rng.next_u64() % 2 ? a.maintain_immolate : b.maintain_immolate;
    child.curse = rng.next_u64() % 2 ? a.curse : b.curse;
    child.shadowburn = rng.next_u64() % 2 ? a.shadowburn : b.shadowburn;
    child.use_decimation_soul_fire = rng.next_u64() % 2 ? a.use_decimation_soul_fire : b.use_decimation_soul_fire;
    child.channel_drain_hope = rng.next_u64() % 2 ? a.channel_drain_hope : b.channel_drain_hope;
    return child;
}

} // namespace warlock::genetic_detail
