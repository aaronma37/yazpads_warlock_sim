#!/usr/bin/env python3
"""
WoW: Forever Warlock DPS Surrogate Model Trainer & Lua Exporter
Trains a LightGBM GBDT regressor on simulation dataset and exports to pure Lua 5.1/5.2/5.3 format.
"""

import sys
import os
import argparse
import csv
import json
import time
import numpy as np

try:
    import lightgbm as lgb
    from sklearn.model_selection import train_test_split
    from sklearn.metrics import r2_score, mean_squared_error, mean_absolute_error
except ImportError as e:
    sys.exit(f"Error importing ML libraries: {e}. Run via .venv/bin/python3 scripts/train_surrogate_lgbm.py")

# Feature specification: list of column names used as input features
FEATURE_COLS = [
    "spec_idx",
    "race_id",
    "fight_duration",
    "spell_power",
    "shadow_power",
    "fire_power",
    "spell_hit_percent",
    "spell_crit_percent",
    "spell_haste_percent",
    "intellect",
    "spirit",
    "stamina",
    "max_mana",
    "target_level",
    "target_shadow_res",
    "target_fire_res",
    "curse_of_shadows",
    "curse_of_elements",
    "shadow_weaving",
    "world_buffs",
    "consumables",
    "pet",
    "sac_imp",
    "sac_succubus",
    "maintain_immolate"
]

TARGET_COL = "dps"

def load_data(csv_path):
    print(f"Loading dataset from: {csv_path}")
    if not os.path.exists(csv_path):
        raise FileNotFoundError(f"Dataset file not found: {csv_path}")

    with open(csv_path, "r", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)
        rows = list(reader)

    if not rows:
        raise ValueError("Dataset is empty!")

    print(f"Loaded {len(rows)} samples.")

    non_feature_cols = {
        "build_desc", "dps", "dps_std", "isb_uptime_percent",
        "avg_life_taps", "avg_mana_spent", "avg_mana_gained"
    }
    feature_cols = [c for c in header if c not in non_feature_cols]
    col_to_idx = {c: i for i, c in enumerate(header)}
    target_idx = col_to_idx["dps"]
    feat_indices = [col_to_idx[c] for c in feature_cols]

    print(f"Features ({len(feature_cols)}): {feature_cols[:5]} ... {feature_cols[-5:]}")

    X = []
    y = []
    for r in rows:
        X.append([float(r[i]) for i in feat_indices])
        y.append(float(r[target_idx]))

    return np.array(X, dtype=np.float32), np.array(y, dtype=np.float32), feature_cols

def flatten_tree(node, feature_list, threshold_list, left_list, right_list, value_list):
    """Recursively flattens a LightGBM tree structure into 1-indexed parallel arrays for Lua."""
    current_idx = len(feature_list) + 1  # 1-indexed for Lua

    # Placeholder slots
    feature_list.append(-1)
    threshold_list.append(0.0)
    left_list.append(0)
    right_list.append(0)
    value_list.append(0.0)

    curr_pos = current_idx - 1

    if "leaf_value" in node:
        # Leaf node
        feature_list[curr_pos] = -1
        threshold_list[curr_pos] = 0.0
        left_list[curr_pos] = 0
        right_list[curr_pos] = 0
        value_list[curr_pos] = float(node["leaf_value"])
    else:
        # Split node: LightGBM split_feature is 0-indexed; convert to 1-indexed for Lua!
        feat_idx = int(node["split_feature"]) + 1
        thresh = float(node["threshold"])
        
        feature_list[curr_pos] = feat_idx
        threshold_list[curr_pos] = thresh

        # Left child
        left_child_idx = flatten_tree(node["left_child"], feature_list, threshold_list, left_list, right_list, value_list)
        left_list[curr_pos] = left_child_idx

        # Right child
        right_child_idx = flatten_tree(node["right_child"], feature_list, threshold_list, left_list, right_list, value_list)
        right_list[curr_pos] = right_child_idx

    return current_idx

def export_lua(model, output_lua_path, metrics, feature_names):
    """Exports the LightGBM model to a clean, fast, standalone Lua file."""
    model_dump = model.dump_model()
    trees = model_dump["tree_info"]
    init_score = model_dump.get("init_score", 0.0)

    print(f"Exporting {len(trees)} trees to Lua: {output_lua_path}...")

    flattened_trees = []
    for t_idx, tree in enumerate(trees):
        feat_arr = []
        thresh_arr = []
        left_arr = []
        right_arr = []
        val_arr = []

        flatten_tree(tree["tree_structure"], feat_arr, thresh_arr, left_arr, right_arr, val_arr)
        flattened_trees.append({
            "feature": feat_arr,
            "threshold": thresh_arr,
            "left": left_arr,
            "right": right_arr,
            "value": val_arr
        })

    # Generate Lua Code
    lua_code = []
    lua_code.append("-- ==========================================================================")
    lua_code.append("-- WoW Forever Warlock DPS Surrogate Model (LightGBM GBDT)")
    lua_code.append(f"-- Generated on: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    lua_code.append(f"-- Trees: {len(flattened_trees)} | Test R²: {metrics['r2']:.4f} | Test MAPE: {metrics['mape']:.2f}% | Test MAE: {metrics['mae']:.2f} DPS")
    lua_code.append("-- Compatible with Lua 5.1, Lua 5.2, Lua 5.3, and LuaJIT (Zero Allocations)")
    lua_code.append("-- ==========================================================================\n")
    lua_code.append("local WarlockDPSModel = {}\n")

    # Feature mapping constants
    lua_code.append("-- Feature Indices (1-indexed for Lua)")
    for i, col in enumerate(feature_names):
        lua_code.append(f"WarlockDPSModel.FEAT_{col.upper()} = {i + 1}")
    lua_code.append(f"WarlockDPSModel.NUM_FEATURES = {len(feature_names)}\n")

    # Accuracy Metrics Table
    lua_code.append("WarlockDPSModel.METRICS = {")
    lua_code.append(f"    trees = {len(flattened_trees)},")
    lua_code.append(f"    r2_score = {metrics['r2']:.5f},")
    lua_code.append(f"    mape_percent = {metrics['mape']:.2f},")
    lua_code.append(f"    mae_dps = {metrics['mae']:.2f},")
    lua_code.append(f"    rmse_dps = {metrics['rmse']:.2f},")
    lua_code.append(f"    generated_at = \"{time.strftime('%Y-%m-%d %H:%M:%S')}\"")
    lua_code.append("}\n")

    lua_code.append(f"WarlockDPSModel.INIT_SCORE = {init_score:.8f}\n")

    # Write trees table
    lua_code.append("local TREES = {")
    for t_idx, ft in enumerate(flattened_trees):
        lua_code.append("    {")
        lua_code.append(f"        feature = {{{','.join(str(x) for x in ft['feature'])}}},")
        lua_code.append(f"        threshold = {{{','.join(f'{x:.6f}' for x in ft['threshold'])}}},")
        lua_code.append(f"        left = {{{','.join(str(x) for x in ft['left'])}}},")
        lua_code.append(f"        right = {{{','.join(str(x) for x in ft['right'])}}},")
        lua_code.append(f"        value = {{{','.join(f'{x:.6f}' for x in ft['value'])}}}")
        lua_code.append("    }" + ("," if t_idx < len(flattened_trees) - 1 else ""))
    lua_code.append("}\n")

    # Inference function
    lua_code.append("""--- Predicts mean DPS from an input feature array (1-indexed, size NUM_FEATURES).
-- @param features table: Array of numerical feature values indexed 1..NUM_FEATURES.
-- @return number: Predicted mean DPS.
function WarlockDPSModel.PredictDPS(features)
    local dps = WarlockDPSModel.INIT_SCORE
    local num_trees = #TREES

    for i = 1, num_trees do
        local tree = TREES[i]
        local feat_arr = tree.feature
        local thresh_arr = tree.threshold
        local left_arr = tree.left
        local right_arr = tree.right
        local val_arr = tree.value

        local node = 1
        while feat_arr[node] ~= -1 do
            local f_idx = feat_arr[node]
            if features[f_idx] <= thresh_arr[node] then
                node = left_arr[node]
            else
                node = right_arr[node]
            end
        end
        dps = dps + val_arr[node]
    end

    if dps < 0.0 then return 0.0 end
    return dps
end

--- Convenience function to evaluate DPS by passing a named table of stats/settings.
-- Any unspecified fields fall back to sensible defaults.
function WarlockDPSModel.PredictFromConfig(cfg)
    local f = {}
    f[1]  = cfg.spec_idx or 0
    f[2]  = cfg.race_id or 0
    f[3]  = cfg.fight_duration or 180.0
    f[4]  = cfg.spell_power or 400.0
    f[5]  = cfg.shadow_power or 0.0
    f[6]  = cfg.fire_power or 0.0
    f[7]  = cfg.spell_hit_percent or 6.0
    f[8]  = cfg.spell_crit_percent or 10.0
    f[9]  = cfg.spell_haste_percent or 0.0
    f[10] = cfg.intellect or 250.0
    f[11] = cfg.spirit or 180.0
    f[12] = cfg.stamina or 280.0
    f[13] = cfg.max_mana or (1393.0 + (f[10] * 15.0))
    f[14] = cfg.target_level or 63
    f[15] = cfg.target_shadow_res or 0.0
    f[16] = cfg.target_fire_res or 0.0
    f[17] = cfg.curse_of_shadows and 1 or 0
    f[18] = cfg.curse_of_elements and 1 or 0
    f[19] = cfg.shadow_weaving and 1 or 0
    f[20] = cfg.world_buffs and 1 or 0
    f[21] = cfg.consumables and 1 or 0
    f[22] = cfg.pet or 0
    f[23] = cfg.sac_imp and 1 or 0
    f[24] = cfg.sac_succubus and 1 or 0
    f[25] = cfg.maintain_immolate and 1 or 0

    return WarlockDPSModel.PredictDPS(f)
end

return WarlockDPSModel
""")

    os.makedirs(os.path.dirname(os.path.abspath(output_lua_path)), exist_ok=True)
    with open(output_lua_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lua_code))

    print(f"Successfully wrote Lua surrogate model to {output_lua_path} ({os.path.getsize(output_lua_path) / 1024:.1f} KB)")

    # Also export compact binary model for C++ engine fast loading
    bin_path = os.path.splitext(output_lua_path)[0] + ".bin"
    import struct
    with open(bin_path, "wb") as bf:
        bf.write(b"SURR")
        bf.write(struct.pack("<IId", 1, len(flattened_trees), init_score))
        for ft in flattened_trees:
            n = len(ft["feature"])
            bf.write(struct.pack("<I", n))
            bf.write(struct.pack(f"<{n}h", *ft["feature"]))
            bf.write(struct.pack(f"<{n}f", *ft["threshold"]))
            bf.write(struct.pack(f"<{n}h", *ft["left"]))
            bf.write(struct.pack(f"<{n}h", *ft["right"]))
            bf.write(struct.pack(f"<{n}f", *ft["value"]))
    print(f"Successfully wrote C++ binary surrogate model to {bin_path} ({os.path.getsize(bin_path) / 1024:.1f} KB)")

    # Also export web JS and JSON if docs directory exists or is targetable
    try:
        from export_model_to_web import export as export_web
        export_web(bin_path=bin_path, out_dir="docs", feature_names=feature_names)
    except Exception as e:
        print(f"Web export note: {e}")
        pass

    return flattened_trees, init_score

def verify_lua_equivalence(X_test, y_test, flattened_trees, init_score, lgb_preds):
    """Verifies that the flattened tree representation exactly reproduces LightGBM predictions."""
    print("Verifying mathematical equivalence between LightGBM and Lua-tree traversal...")
    errors = []
    for row_idx in range(len(X_test)):
        row = X_test[row_idx]
        dps = init_score
        for tree in flattened_trees:
            node = 1
            feat_arr = tree["feature"]
            thresh_arr = tree["threshold"]
            left_arr = tree["left"]
            right_arr = tree["right"]
            val_arr = tree["value"]

            while feat_arr[node - 1] != -1:
                f_idx = feat_arr[node - 1]  # 1-indexed
                val = row[f_idx - 1]
                if val <= thresh_arr[node - 1]:
                    node = left_arr[node - 1]
                else:
                    node = right_arr[node - 1]
            dps += val_arr[node - 1]

        diff = abs(dps - lgb_preds[row_idx])
        errors.append(diff)

    max_err = max(errors)
    mean_err = np.mean(errors)
    print(f"  Verification check: Max diff = {max_err:.8f}, Mean diff = {mean_err:.8f}")
    assert max_err < 1e-4, f"Mismatch in tree flattening logic! Max diff = {max_err}"
    print("  [PASS] Lua tree representation is mathematically identical to LightGBM!")

def main():
    parser = argparse.ArgumentParser(description="Train LightGBM surrogate model and export to Lua")
    parser.add_argument("--input", default="data/surrogate_dataset.csv", help="Path to input CSV dataset")
    parser.add_argument("--output-lua", default="data/surrogate_dps.lua", help="Path to output Lua file")
    parser.add_argument("--num-trees", type=int, default=80, help="Number of boosting trees (default: 80)")
    parser.add_argument("--max-depth", type=int, default=6, help="Maximum tree depth (default: 6)")
    parser.add_argument("--learning-rate", type=float, default=0.08, help="Learning rate (default: 0.08)")
    args = parser.parse_args()

    X, y, feature_cols = load_data(args.input)

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    print(f"Training set: {len(X_train)} samples | Test set: {len(X_test)} samples")

    train_data = lgb.Dataset(X_train, label=y_train, feature_name=feature_cols)
    test_data = lgb.Dataset(X_test, label=y_test, feature_name=feature_cols, reference=train_data)

    params = {
        "objective": "regression",
        "metric": ["rmse", "mae"],
        "boosting_type": "gbdt",
        "num_leaves": 31,
        "max_depth": args.max_depth,
        "learning_rate": args.learning_rate,
        "feature_fraction": 0.9,
        "verbose": -1,
        "n_jobs": -1,
        "random_state": 42
    }

    print(f"Training LightGBM model ({args.num_trees} trees, depth {args.max_depth}, lr {args.learning_rate})...")
    start_t = time.time()
    model = lgb.train(
        params,
        train_data,
        num_boost_round=args.num_trees,
        valid_sets=[test_data]
    )
    train_time = time.time() - start_t
    print(f"Training finished in {train_time:.2f}s")

    # Evaluation
    preds_test = model.predict(X_test)
    r2 = r2_score(y_test, preds_test)
    rmse = np.sqrt(mean_squared_error(y_test, preds_test))
    mae = mean_absolute_error(y_test, preds_test)
    mape = np.mean(np.abs((y_test - preds_test) / np.maximum(y_test, 1e-3))) * 100.0

    print("\n" + "=" * 55)
    print("           SURROGATE MODEL EVALUATION RESULTS         ")
    print("=" * 55)
    print(f"  R² Score (Accuracy):        {r2:.5f} (1.0 = Perfect)")
    print(f"  Mean Absolute Pct Error:    {mape:.2f}%")
    print(f"  Mean Absolute Error (MAE):  {mae:.2f} DPS")
    print(f"  Root Mean Squared (RMSE):   {rmse:.2f} DPS")
    print("=" * 55)

    # Feature Importance
    importances = model.feature_importance(importance_type="gain")
    feat_imp = sorted(zip(feature_cols, importances), key=lambda x: x[1], reverse=True)
    print("\nTop 15 Most Important Features:")
    for f_name, imp in feat_imp[:15]:
        print(f"  {f_name:24s}: {imp:12.1f}")

    metrics = {"r2": r2, "rmse": rmse, "mae": mae, "mape": mape}
    flattened_trees, init_score = export_lua(model, args.output_lua, metrics, feature_cols)

    verify_lua_equivalence(X_test, y_test, flattened_trees, init_score, preds_test)
    print("\nPipeline completed successfully!")

if __name__ == "__main__":
    main()
