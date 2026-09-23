#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <string>
#include "talent_graph.hpp"
#include "policy.hpp"
#include "buffs.hpp"
#include "stats.hpp"

namespace warlock {

constexpr size_t SURROGATE_FEATURE_DIM = 93;

struct SurrogateSample {
    std::array<int, TOTAL_TALENT_NODES> talents;
    PetChoice pet;
    bool sac_imp;
    bool sac_succubus;
    RotationChoice rotation;
    bool maintain_immolate;
    CurseChoice curse;
    Race race = Race::UNDEAD;
    double dps = 0.0;
};

class SurrogateModel {
public:
    SurrogateModel(double l2_reg = 1e-2) : l2_reg_(l2_reg) {
        weights_.fill(0.0);
    }

    // Extract standardized features from candidate configuration
    static std::array<double, SURROGATE_FEATURE_DIM> extract_features(
        const std::array<int, TOTAL_TALENT_NODES>& talents,
        PetChoice pet,
        bool sac_imp,
        bool sac_succubus,
        RotationChoice rotation,
        bool maintain_immolate,
        CurseChoice curse,
        Race race = Race::UNDEAD
    ) {
        std::array<double, SURROGATE_FEATURE_DIM> x{};
        size_t idx = 0;

        // 0. Bias term
        x[idx++] = 1.0;

        // 1..52: Normalized talent values
        const auto& graph = TalentGraph::get();
        for (size_t i = 0; i < TOTAL_TALENT_NODES; ++i) {
            x[idx++] = static_cast<double>(talents[i]) / static_cast<double>(graph.node(i).max_points);
        }

        // 53..57: Pet & Sac one-hots
        x[idx++] = (pet == PetChoice::IMP && !sac_imp && !sac_succubus) ? 1.0 : 0.0;
        x[idx++] = (pet == PetChoice::SUCCUBUS && !sac_imp && !sac_succubus) ? 1.0 : 0.0;
        x[idx++] = sac_imp ? 1.0 : 0.0;
        x[idx++] = sac_succubus ? 1.0 : 0.0;
        x[idx++] = (pet == PetChoice::NONE && !sac_imp && !sac_succubus) ? 1.0 : 0.0;

        // 58..66: Rotation category one-hots
        bool is_fire = (rotation == RotationChoice::FIRE_DESTRO ||
                        rotation == RotationChoice::FIRE_DESTRO_NO_CORRUPTION ||
                        rotation == RotationChoice::DP_RUIN_FIRE ||
                        rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_2 ||
                        rotation == RotationChoice::SHADOW_AND_FLAME_FIRE_BANE);
        bool is_deep_aff = (rotation == RotationChoice::DEEP_AFFLICTION ||
                            rotation == RotationChoice::DEEP_AFFLICTION_SB ||
                            rotation == RotationChoice::DEEP_AFFLICTION_SB_NO_SL ||
                            rotation == RotationChoice::AFFLICTION_HYBRID_DOTS);
        bool is_demo = (rotation == RotationChoice::DP_AF_SHADOW ||
                        rotation == RotationChoice::DP_AF_SHADOW_NO_CORRUPTION ||
                        rotation == RotationChoice::DEMONOLOGY_EXECUTE ||
                        rotation == RotationChoice::DP_AF_SHADOW_BRAND);

        x[idx++] = is_fire ? 1.0 : 0.0;
        x[idx++] = is_deep_aff ? 1.0 : 0.0;
        x[idx++] = is_demo ? 1.0 : 0.0;
        x[idx++] = maintain_immolate ? 1.0 : 0.0;
        x[idx++] = (curse == CurseChoice::BANE_OF_AGONY) ? 1.0 : 0.0;
        x[idx++] = (curse == CurseChoice::CURSE_OF_DOOM) ? 1.0 : 0.0;

        // 64..84: Key Non-Linear Synergy Interactions
        // Affliction synergies
        double sm_rank = static_cast<double>(talents[15]) / 5.0; // Shadow Mastery
        double nightfall_rank = static_cast<double>(talents[11]) / 2.0; // Nightfall
        double corr_rank = static_cast<double>(talents[2]) / 5.0; // Imp Corruption
        double wrack_rank = static_cast<double>(talents[16]) / 1.0; // Wrack
        x[idx++] = sm_rank * (!is_fire ? 1.0 : 0.0);
        x[idx++] = nightfall_rank * corr_rank;
        x[idx++] = wrack_rank * (is_deep_aff ? 1.0 : 0.0);

        // Demonology synergies
        double ds_rank = static_cast<double>(talents[17 + 9]) / 1.0; // Demonic Sacrifice
        double md_rank = static_cast<double>(talents[17 + 17]) / 5.0; // Master Demonologist
        double dk_rank = static_cast<double>(talents[17 + 16]) / 3.0; // Demonic Knowledge
        double decimation_rank = static_cast<double>(talents[17 + 11]) / 2.0; // Decimation
        double dp_rank = static_cast<double>(talents[17 + 18]) / 1.0; // Demonic Pact
        x[idx++] = ds_rank * (sac_imp ? 1.0 : 0.0);
        x[idx++] = ds_rank * (sac_succubus ? 1.0 : 0.0);
        x[idx++] = md_rank * (pet != PetChoice::NONE ? 1.0 : 0.0);
        x[idx++] = dk_rank * (pet != PetChoice::NONE ? 1.0 : 0.0);
        x[idx++] = decimation_rank * (is_demo ? 1.0 : 0.0);
        x[idx++] = dp_rank * sac_imp * (pet == PetChoice::SUCCUBUS ? 1.0 : 0.0);

        // Destruction synergies
        double ruin_rank = static_cast<double>(talents[36 + 6]) / 5.0; // Ruin
        double isb_rank = static_cast<double>(talents[36 + 1]) / 5.0; // Improved Shadow Bolt
        double bane_rank = static_cast<double>(talents[36 + 2]) / 5.0; // Bane
        double incinerate_rank = static_cast<double>(talents[36 + 15]) / 1.0; // Incinerate
        double agonizing_flames_rank = static_cast<double>(talents[36 + 9]) / 3.0; // Agonizing Flames
        double conflag_rank = static_cast<double>(talents[36 + 10]) / 1.0; // Conflagrate
        double sf_rank = static_cast<double>(talents[36 + 14]) / 5.0; // Shadow and Flame

        x[idx++] = ruin_rank * (!is_fire ? 1.0 : 0.0);
        x[idx++] = ruin_rank * (is_fire ? 1.0 : 0.0);
        x[idx++] = isb_rank * (!is_fire ? 1.0 : 0.0);
        x[idx++] = bane_rank;
        x[idx++] = incinerate_rank * (is_fire ? 1.0 : 0.0);
        x[idx++] = agonizing_flames_rank;
        x[idx++] = conflag_rank * sf_rank;
        x[idx++] = sf_rank * (sac_imp ? 1.0 : 0.0);

        // Total tree distribution ratios
        int aff_pts = graph.count_tree_points(talents, 0);
        int demo_pts = graph.count_tree_points(talents, 1);
        int destro_pts = graph.count_tree_points(talents, 2);
        x[idx++] = static_cast<double>(aff_pts) / 51.0;
        x[idx++] = static_cast<double>(demo_pts) / 51.0;
        x[idx++] = static_cast<double>(destro_pts) / 51.0;

        // 85..92: Race One-Hots & Racial Synergies
        bool is_undead = (race == Race::UNDEAD);
        bool is_orc = (race == Race::ORC);
        bool is_troll = (race == Race::TROLL);
        bool is_human = (race == Race::HUMAN);
        bool is_gnome = (race == Race::GNOME);

        x[idx++] = is_undead ? 1.0 : 0.0;
        x[idx++] = is_orc ? 1.0 : 0.0;
        x[idx++] = is_troll ? 1.0 : 0.0;
        x[idx++] = is_human ? 1.0 : 0.0;
        x[idx++] = is_gnome ? 1.0 : 0.0;

        x[idx++] = is_orc * (pet != PetChoice::NONE && !sac_imp && !sac_succubus ? 1.0 : 0.0); // Orc Command bonus
        x[idx++] = is_troll * (static_cast<double>(destro_pts) / 51.0); // Troll Berserking
        x[idx++] = is_gnome * (static_cast<double>(aff_pts) / 51.0); // Gnome Expansive Mind

        return x;
    }

    void add_sample(const SurrogateSample& s) {
        samples_.push_back(s);
    }

    size_t sample_count() const {
        return samples_.size();
    }

    // Solve regularized linear regression (X^T X + lambda I) w = X^T y
    bool train() {
        if (samples_.empty()) return false;

        constexpr size_t D = SURROGATE_FEATURE_DIM;
        std::vector<std::vector<double>> A(D, std::vector<double>(D, 0.0));
        std::vector<double> b(D, 0.0);

        for (const auto& s : samples_) {
            auto x = extract_features(s.talents, s.pet, s.sac_imp, s.sac_succubus, s.rotation, s.maintain_immolate, s.curse, s.race);
            for (size_t i = 0; i < D; ++i) {
                for (size_t j = 0; j < D; ++j) {
                    A[i][j] += x[i] * x[j];
                }
                b[i] += x[i] * s.dps;
            }
        }

        // L2 Regularization (ridge penalty on weights, no penalty on bias term at index 0)
        for (size_t i = 0; i < D; ++i) {
            A[i][i] += (i == 0 ? 1e-4 : l2_reg_ * static_cast<double>(samples_.size()));
        }

        // Gaussian elimination with partial pivoting
        for (size_t col = 0; col < D; ++col) {
            size_t max_row = col;
            double max_val = std::abs(A[col][col]);
            for (size_t row = col + 1; row < D; ++row) {
                if (std::abs(A[row][col]) > max_val) {
                    max_val = std::abs(A[row][col]);
                    max_row = row;
                }
            }

            if (max_val < 1e-9) continue;

            if (max_row != col) {
                std::swap(A[col], A[max_row]);
                std::swap(b[col], b[max_row]);
            }

            for (size_t row = col + 1; row < D; ++row) {
                double factor = A[row][col] / A[col][col];
                for (size_t c = col; c < D; ++c) {
                    A[row][c] -= factor * A[col][c];
                }
                b[row] -= factor * b[col];
            }
        }

        // Back-substitution
        for (int row = static_cast<int>(D) - 1; row >= 0; --row) {
            double sum = b[row];
            for (size_t col = row + 1; col < D; ++col) {
                sum -= A[row][col] * weights_[col];
            }
            if (std::abs(A[row][row]) > 1e-9) {
                weights_[row] = sum / A[row][row];
            } else {
                weights_[row] = 0.0;
            }
        }

        is_trained_ = true;
        return true;
    }

    // Predict DPS for a candidate
    double predict(
        const std::array<int, TOTAL_TALENT_NODES>& talents,
        PetChoice pet,
        bool sac_imp,
        bool sac_succubus,
        RotationChoice rotation,
        bool maintain_immolate,
        CurseChoice curse,
        Race race = Race::UNDEAD
    ) const {
        if (!is_trained_) return 0.0;
        auto x = extract_features(talents, pet, sac_imp, sac_succubus, rotation, maintain_immolate, curse, race);
        double pred = 0.0;
        for (size_t i = 0; i < SURROGATE_FEATURE_DIM; ++i) {
            pred += weights_[i] * x[i];
        }
        return std::max(0.0, pred);
    }

    // Estimated marginal DPS value for each talent node (points 1..52)
    double get_talent_weight(size_t global_idx) const {
        if (!is_trained_ || global_idx >= TOTAL_TALENT_NODES) return 0.0;
        return weights_[1 + global_idx];
    }

    bool is_trained() const {
        return is_trained_;
    }

private:
    double l2_reg_;
    bool is_trained_ = false;
    std::array<double, SURROGATE_FEATURE_DIM> weights_;
    std::vector<SurrogateSample> samples_;
};

} // namespace warlock
