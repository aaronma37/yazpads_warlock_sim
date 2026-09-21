#pragma once
#include <vector>
#include <string>
#include <array>
#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "warlock_sim.hpp"
#include "spec_presets.hpp"

#include "talent_graph.hpp"

namespace warlock {

struct FlatTree {
    std::vector<int16_t> feature;
    std::vector<float> threshold;
    std::vector<int16_t> left;
    std::vector<int16_t> right;
    std::vector<float> value;
};

class SurrogateEvaluator {
private:
    double init_score_ = 0.0;
    std::vector<FlatTree> trees_;
    bool loaded_ = false;

    SurrogateEvaluator() {
        // Automatically attempt to load pre-trained binary model
        if (!load_model("data/surrogate_dps.bin")) {
            load_model("../data/surrogate_dps.bin");
        }
    }

public:
    static SurrogateEvaluator& get() {
        static SurrogateEvaluator instance;
        return instance;
    }

    bool is_loaded() const { return loaded_ && !trees_.empty(); }
    size_t num_trees() const { return trees_.size(); }

    bool load_model(const std::string& bin_path) {
        std::ifstream f(bin_path, std::ios::binary);
        if (!f.is_open()) {
            loaded_ = false;
            return false;
        }

        char magic[4];
        f.read(magic, 4);
        if (std::memcmp(magic, "SURR", 4) != 0) {
            loaded_ = false;
            return false;
        }

        uint32_t version = 0, num_trees = 0;
        double init_score = 0.0;
        f.read(reinterpret_cast<char*>(&version), 4);
        f.read(reinterpret_cast<char*>(&num_trees), 4);
        f.read(reinterpret_cast<char*>(&init_score), 8);

        init_score_ = init_score;
        trees_.resize(num_trees);

        for (uint32_t i = 0; i < num_trees; ++i) {
            uint32_t n = 0;
            f.read(reinterpret_cast<char*>(&n), 4);
            trees_[i].feature.resize(n);
            trees_[i].threshold.resize(n);
            trees_[i].left.resize(n);
            trees_[i].right.resize(n);
            trees_[i].value.resize(n);

            f.read(reinterpret_cast<char*>(trees_[i].feature.data()), n * sizeof(int16_t));
            f.read(reinterpret_cast<char*>(trees_[i].threshold.data()), n * sizeof(float));
            f.read(reinterpret_cast<char*>(trees_[i].left.data()), n * sizeof(int16_t));
            f.read(reinterpret_cast<char*>(trees_[i].right.data()), n * sizeof(int16_t));
            f.read(reinterpret_cast<char*>(trees_[i].value.data()), n * sizeof(float));
        }

        loaded_ = true;
        return true;
    }

    double predict(const double* feat, size_t num_feat) const {
        if (!loaded_ || trees_.empty()) return 0.0;
        double dps = init_score_;
        for (const auto& t : trees_) {
            int node = 0;
            while (t.feature[node] != -1) {
                int f_idx = t.feature[node] - 1; // 1-indexed to 0-indexed
                if (f_idx >= 0 && static_cast<size_t>(f_idx) < num_feat && feat[f_idx] <= t.threshold[node]) {
                    node = t.left[node] - 1;
                } else {
                    node = t.right[node] - 1;
                }
            }
            dps += t.value[node];
        }
        return std::max(0.0, dps);
    }

    double predict(const std::vector<double>& feat) const {
        return predict(feat.data(), feat.size());
    }

    template<size_t N>
    double predict(const std::array<double, N>& feat) const {
        return predict(feat.data(), N);
    }

    double predict_from_sim(const WarlockSimulator& sim, int /*spec_idx*/ = 0) const {
        if (!is_loaded()) return 0.0;

        Stats effective_stats = sim.use_raw_stats ? sim.raw_stats : sim.gear.calculate_stats();

        const auto& graph = TalentGraph::get();
        auto talent_vec = graph.to_vector(sim.talents);
        int aff_pts = sim.talents.aff.total_points();
        int demo_pts = sim.talents.demo.total_points();
        int destro_pts = sim.talents.destro.total_points();

        int demon_setup = 0;
        if (sim.buffs.sacrifice_imp) {
            demon_setup = (sim.policy.pet == PetChoice::SUCCUBUS) ? 5 : 3;
        } else if (sim.buffs.sacrifice_succubus) {
            demon_setup = (sim.policy.pet == PetChoice::IMP) ? 6 : 4;
        } else if (sim.policy.pet == PetChoice::IMP) {
            demon_setup = 1;
        } else if (sim.policy.pet == PetChoice::SUCCUBUS) {
            demon_setup = 2;
        }

        std::vector<double> feat;
        feat.reserve(75);
        feat.push_back(static_cast<double>(aff_pts));
        feat.push_back(static_cast<double>(demo_pts));
        feat.push_back(static_cast<double>(destro_pts));
        for (int p : talent_vec) {
            feat.push_back(static_cast<double>(p));
        }
        feat.push_back(static_cast<double>(sim.race));
        feat.push_back(sim.fight_duration);
        feat.push_back(effective_stats.spell_power);
        feat.push_back(effective_stats.shadow_power);
        feat.push_back(effective_stats.fire_power);
        feat.push_back(effective_stats.spell_hit_percent);
        feat.push_back(effective_stats.total_spell_crit(sim.base_attrs.base_spell_crit));
        feat.push_back(effective_stats.spell_haste_percent);
        feat.push_back(effective_stats.intellect);
        feat.push_back(effective_stats.spirit);
        feat.push_back(effective_stats.stamina);
        feat.push_back(effective_stats.max_mana > 0.0 ? effective_stats.max_mana : (sim.base_attrs.base_mana + effective_stats.intellect * 15.0));
        feat.push_back(static_cast<double>(sim.target_config.level));
        feat.push_back(sim.target_config.base_shadow_resistance);
        feat.push_back(sim.target_config.base_fire_resistance);
        feat.push_back(sim.buffs.curse_of_shadows ? 1.0 : 0.0);
        feat.push_back(sim.buffs.curse_of_elements ? 1.0 : 0.0);
        feat.push_back(static_cast<double>(demon_setup));
        feat.push_back(static_cast<double>(sim.policy.rotation));
        feat.push_back(sim.policy.maintain_immolate ? 1.0 : 0.0);

        return predict(feat);
    }
};

} // namespace warlock
