#!/usr/bin/env python3
"""
WebGPU vs CPU DES Comparison Pipeline Script
Runs the native parity pipeline binary, collects outputs, parses JSON,
and produces human-readable summary diagnostics and markdown reports.
"""

import sys
import os
import argparse
import subprocess
import json

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN_PATH = os.path.join(ROOT_DIR, "bin", "webgpu_parity_pipeline")

def main():
    parser = argparse.ArgumentParser(description="WebGPU vs CPU DES Parity Comparison Pipeline")
    parser.add_argument("--all", action="store_true", default=True, help="Run all standard scenarios")
    parser.add_argument("--sweep", choices=["sp", "hit", "crit", "haste", "duration"], help="Run parameter sweep")
    parser.add_argument("--presets", action="store_true", help="Run spec presets")
    parser.add_argument("--scenario", type=str, help="Run individual scenario name")
    parser.add_argument("--iterations", type=int, default=10000, help="Sim iterations per engine (default 10000)")
    parser.add_argument("--tolerance", type=float, default=1.5, help="DPS percent tolerance (default 1.5%%)")
    parser.add_argument("--json", type=str, default="parity_results.json", help="Path to output JSON")
    parser.add_argument("--markdown", type=str, default="parity_report.md", help="Path to output Markdown")
    parser.add_argument("--verbose", "-v", action="store_true", help="Print verbose tables and breakdown")
    args = parser.parse_args()

    if not os.path.isfile(BIN_PATH):
        print(f"[!] Pipeline binary not found at {BIN_PATH}. Building project first...")
        build_res = subprocess.run(["make", "-C", os.path.join(ROOT_DIR, "build"), "webgpu_parity_pipeline"])
        if build_res.returncode != 0:
            print("[X] Build failed.")
            sys.exit(1)

    cmd = [BIN_PATH, "--iterations", str(args.iterations), "--tolerance", str(args.tolerance),
           "--json", args.json, "--markdown", args.markdown]

    if args.verbose:
        cmd.append("--verbose")
    if args.scenario:
        cmd.extend(["--scenario", args.scenario])
    elif args.sweep:
        cmd.extend(["--sweep", args.sweep])
    elif args.presets:
        cmd.append("--presets")
    else:
        cmd.append("--all")

    print(f"[*] Executing parity pipeline: {' '.join(cmd)}")
    res = subprocess.run(cmd)

    if os.path.isfile(args.json):
        with open(args.json, "r") as f:
            data = json.load(f)
        total = len(data)
        passed = sum(1 for item in data if item.get("passed", False))
        failed = total - passed
        print(f"\n[+] Pipeline execution completed: {passed}/{total} Scenarios Passed ({failed} Failed)")
        if failed > 0:
            print("\n[!] Failed Scenarios & Diagnostics:")
            for item in data:
                if not item.get("passed", False):
                    print(f"  - {item['scenario']} (DPS Diff: {item['summary']['dps_pct_diff']:.2f}%)")
                    for d in item.get("diagnostics", []):
                        print(f"      * {d}")

    sys.exit(res.returncode)

if __name__ == "__main__":
    main()
