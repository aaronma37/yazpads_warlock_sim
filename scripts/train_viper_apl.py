#!/usr/bin/env python3
"""
VIPER Policy Extraction & APL Transpiler
Implements Verifiable Reinforcement Learning via Policy Extraction (Bastani et al., NeurIPS/ICLR).

Given an aggregated dataset of (state, oracle_action, sample_weight) generated from
DAgger rollouts with Q-loss weighting:
    w(s) = max_a Q(s, a) - min_a Q(s, a)

This script:
1. Trains a Q-weighted Decision Tree (interpretable policy) on the dataset.
2. Evaluates weighted policy fidelity against the oracle.
3. Transpiles the Decision Tree into:
   a) C++ / if-else branching code for simulation execution.
   b) Formatted Action Priority List (APL) rules.
"""

import sys
import os
import argparse
import pandas as pd
import numpy as np
from sklearn.tree import DecisionTreeClassifier, export_text, _tree

# Action Mapping enum matching C++ PriorityAction
ACTION_NAMES = {
    0: "LIFE_TAP",
    1: "RACIAL_EUREKA",
    2: "RACIAL_BLOOD_FURY",
    3: "RACIAL_BERSERKING",
    4: "AMPLIFY_CURSE",
    5: "CURSE_OF_AGONY",
    6: "CURSE_OF_DOOM",
    7: "BANE_OF_HAVOC",
    8: "NIGHTFALL_SHADOW_BOLT",
    9: "DECIMATION_SEARING_PAIN",
    10: "DECIMATION_SOUL_FIRE",
    11: "DEMONIC_BRAND_SEARING_PAIN",
    12: "CORRUPTION",
    13: "SIPHON_LIFE",
    14: "DRAIN_HOPE",
    15: "IMMOLATE",
    16: "CONFLAGRATE",
    17: "SHADOWBURN",
    18: "INCINERATE_FILLER",
    19: "SEARING_PAIN_FILLER",
    20: "DRAIN_LIFE_FILLER",
    21: "DRAIN_SOUL_FILLER",
    22: "SHADOW_BOLT_FILLER"
}

FEATURE_COLS = [
    "player_mana_pct",
    "player_hp_pct",
    "fight_progress_pct",
    "time_remaining_sec",
    "target_hp_pct",
    "num_targets",
    "target2_has_havoc",
    "nightfall_proc_active",
    "decimation_rem_sec",
    "shadow_and_flame_rem_sec",
    "trinket_rem_sec",
    "racial_rem_sec",
    "eureka_charges",
    "dot_corruption_rem_sec",
    "dot_agony_rem_sec",
    "dot_doom_rem_sec",
    "dot_immolate_rem_sec",
    "dot_siphon_life_rem_sec",
    "dot_wrack_rem_sec",
    "isb_charges_rem",
    "cd_conflagrate_sec",
    "cd_shadowburn_sec",
    "cd_curse_of_doom_sec",
    "cd_racial_sec"
]

def train_viper_tree(csv_path: str, max_depth: int = 5, min_samples_leaf: int = 15):
    print(f"[*] Loading VIPER dataset from: {csv_path}")
    df = pd.read_csv(csv_path)
    
    X = df[FEATURE_COLS].values
    y = df["oracle_action"].values
    weights = df["sample_weight"].values if "sample_weight" in df.columns else np.ones(len(df))
    
    # Normalize weights so sum equals sample count
    weights = weights / np.mean(weights)
    
    print(f"[*] Training Q-weighted DecisionTreeClassifier (max_depth={max_depth}, min_samples_leaf={min_samples_leaf})...")
    clf = DecisionTreeClassifier(
        max_depth=max_depth,
        min_samples_leaf=min_samples_leaf,
        random_state=42
    )
    clf.fit(X, y, sample_weight=weights)
    
    # Evaluate
    preds = clf.predict(X)
    unweighted_acc = np.mean(preds == y)
    weighted_acc = np.sum(weights * (preds == y)) / np.sum(weights)
    
    print(f"[+] Decision Tree Trained Successfully!")
    print(f"    - Leaf Nodes: {clf.get_n_leaves()}")
    print(f"    - Tree Depth: {clf.get_depth()}")
    print(f"    - Unweighted Oracle Accuracy: {unweighted_acc * 100:.2f}%")
    print(f"    - Q-Weighted Fidelity:         {weighted_acc * 100:.2f}%")
    
    return clf, FEATURE_COLS

def transpile_tree_to_cpp(tree: DecisionTreeClassifier, feature_names: list) -> str:
    """Transpiles scikit-learn DecisionTreeClassifier into optimized C++ branch code."""
    tree_ = tree.tree_
    feature_name = [
        feature_names[i] if i != _tree.TREE_UNDEFINED else "undefined!"
        for i in tree_.feature
    ]
    
    lines = []
    lines.append("// Auto-generated VIPER Policy Decision Function")
    lines.append("inline PriorityAction evaluate_viper_policy(const sim::SimObservation& obs) {")
    
    def recurse(node, depth):
        indent = "    " * (depth + 1)
        if tree_.feature[node] != _tree.TREE_UNDEFINED:
            name = feature_name[node]
            threshold = tree_.threshold[node]
            lines.append(f"{indent}if (obs.{name} <= {threshold:.4f}f) {{")
            recurse(tree_.children_left[node], depth + 1)
            lines.append(f"{indent}}} else {{")
            recurse(tree_.children_right[node], depth + 1)
            lines.append(f"{indent}}}")
        else:
            action_idx = int(np.argmax(tree_.value[node]))
            action_name = ACTION_NAMES.get(action_idx, f"UNKNOWN_{action_idx}")
            lines.append(f"{indent}return PriorityAction::{action_name};")

    recurse(0, 0)
    lines.append("}")
    return "\n".join(lines)

def main():
    parser = argparse.ArgumentParser(description="VIPER Decision Tree Policy Trainer & Transpiler")
    parser.add_argument("--data", type=str, default="viper_dataset.csv", help="Path to input dataset CSV")
    parser.add_argument("--max_depth", type=int, default=5, help="Maximum decision tree depth")
    parser.add_argument("--min_leaf", type=int, default=10, help="Minimum samples per leaf")
    parser.add_argument("--out_cpp", type=str, default="src/sim/warlock/viper_policy_generated.hpp", help="Output C++ header path")
    args = parser.parse_args()

    if not os.path.exists(args.data):
        print(f"[!] Dataset file '{args.data}' not found. Generate it first using the sim test harness.")
        sys.exit(1)

    clf, feat_names = train_viper_tree(args.data, max_depth=args.max_depth, min_samples_leaf=args.min_leaf)
    
    cpp_code = transpile_tree_to_cpp(clf, feat_names)
    with open(args.out_cpp, "w") as f:
        f.write("#pragma once\n#include \"policy.hpp\"\n#include \"src/sim/common/sim_state_vector.hpp\"\n\nnamespace warlock {\n\n" + cpp_code + "\n\n} // namespace warlock\n")
    print(f"[+] Transpiled C++ VIPER policy written to: {args.out_cpp}")

if __name__ == "__main__":
    main()
