#!/usr/bin/env python3
"""
Benchmark suite for Warlock Simulation: WebGL2 GPU Shader vs Native C++ CPU Oracle.

Evaluates:
  1. Simulation throughput (fights/second), execution latency, and batch scaling.
  2. Per-spec isolated performance across all 22 preset configurations to identify
     bottlenecks and slow configurations.
  3. Multi-spec batching and SIMD/Warp divergence analysis.
  4. Authoritative C++ CPU Oracle scaling across single and multi-core baselines.

Usage:
  python3 scripts/benchmark_sim.py
  python3 scripts/benchmark_sim.py --isolate-specs
  python3 scripts/benchmark_sim.py --divergence-analysis
  python3 scripts/benchmark_sim.py --scales 1000 5000 10000 50000 100000
  python3 scripts/benchmark_sim.py --multi-spec
  python3 scripts/benchmark_sim.py --fast
  python3 scripts/benchmark_sim.py --markdown benchmark_report.md
"""

import argparse
import http.server
import json
import os
import platform
import shutil
import socketserver
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

# Terminal colors
BOLD = "\033[1m"
DIM = "\033[2m"
CYAN = "\033[36m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
BLUE = "\033[34m"
MAGENTA = "\033[35m"
RED = "\033[31m"
RESET = "\033[0m"


class BenchmarkServer:
    def __init__(self, directory, port=0):
        self.directory = directory
        handler = lambda *args, **kwargs: http.server.SimpleHTTPRequestHandler(
            *args, directory=str(self.directory), **kwargs
        )
        handler.log_message = lambda *args: None
        self.httpd = socketserver.TCPServer(("127.0.0.1", port), handler)
        self.port = self.httpd.server_address[1]
        self.thread = threading.Thread(target=self.httpd.serve_forever, daemon=True)
        self.thread.start()

    def stop(self):
        self.httpd.shutdown()
        self.httpd.server_close()


def get_browser_command(choice="chrome", url=""):
    flatpak = shutil.which("flatpak")
    if flatpak:
        check = subprocess.run([flatpak, "info", "com.google.Chrome"], capture_output=True, text=True)
        if check.returncode == 0:
            return [
                "flatpak", "run", "--share=network", "--device=all", "com.google.Chrome",
                "--headless", "--no-sandbox", "--disable-setuid-sandbox", "--enable-webgl",
                "--use-angle=swiftshader", "--enable-unsafe-swiftshader", url
            ]

    candidates = [
        "google-chrome-stable", "google-chrome", "chromium-browser", "chromium",
        "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
        "/usr/bin/google-chrome-stable", "/usr/bin/google-chrome", "/usr/bin/chromium-browser", "/usr/bin/chromium"
    ]
    for c in candidates:
        path = shutil.which(c)
        if path and os.path.isfile(path) and os.access(path, os.X_OK):
            return [
                path, "--headless", "--no-sandbox", "--disable-setuid-sandbox",
                "--enable-webgl", "--use-angle=swiftshader", "--enable-unsafe-swiftshader", url
            ]

    firefox = shutil.which("firefox")
    if firefox:
        return [firefox, "--headless", url]

    return None


def run_headless_gpu_benchmark(cases, browser_choice="chrome", timeout=180, fast=False):
    runner_html_path = Path("threejs_webgl_des/benchmark_runner.html")
    cases_js = json.dumps(cases)
    detailed_results = json.dumps(not fast)

    runner_html = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<script type="importmap">
{{
  "imports": {{
    "three": "./vendor/three.module.js"
  }}
}}
</script>
</head>
<body>
<div id="status">Running GPU simulation benchmark...</div>
<script type="module">
import {{ runSimulation, runMultiSimulation }} from './src/engine.js';

async function run() {{
  const statusEl = document.getElementById('status');
  try {{
    const cases = {cases_js};
    const detailedResults = {detailed_results};
    const results = [];

    // Warm-up pass (compile shader and initialize WebGL buffers)
    await runSimulation({{ duration: 180, iterations: 100, seed: 1 }}, {{ detailedResults }});

    for (const testCase of cases) {{
      const configs = testCase.configs;
      const totalFights = testCase.totalFights;
      
      const t0 = performance.now();
      let simResult;
      if (configs.length === 1) {{
        simResult = await runSimulation(configs[0], {{ detailedResults }});
      }} else {{
        simResult = await runMultiSimulation(configs, {{ iterations: configs[0].iterations, detailedResults }});
      }}
      const elapsedMs = performance.now() - t0;
      
      let meanDps = 0, avgEvents = 0, avgTaps = 0, petDmg = 0, maxHeap = 0;
      if (simResult.summary) {{
        meanDps = simResult.summary.mean || 0;
        avgEvents = simResult.summary.events ? (simResult.summary.events / totalFights) : 0;
        avgTaps = simResult.summary.taps || 0;
        petDmg = simResult.summary.petDamage || 0;
        maxHeap = simResult.summary.maxHeap || 0;
      }} else if (simResult.results) {{
        const rLen = simResult.results.length;
        meanDps = simResult.results.reduce((a, b) => a + (b.summary ? b.summary.mean : 0), 0) / rLen;
        avgEvents = simResult.results.reduce((a, b) => a + (b.summary ? (b.summary.events / b.summary.count) : 0), 0) / rLen;
        avgTaps = simResult.results.reduce((a, b) => a + (b.summary ? b.summary.taps : 0), 0) / rLen;
        petDmg = simResult.results.reduce((a, b) => a + (b.summary ? b.summary.petDamage : 0), 0) / rLen;
      }}

      results.push({{
        name: testCase.name,
        numConfigs: configs.length,
        iterationsPerConfig: configs[0].iterations,
        totalFights: totalFights,
        elapsedMs: elapsedMs,
        throughput: Math.round((totalFights / (elapsedMs / 1000))),
        meanDps: meanDps,
        avgEvents: Math.round(avgEvents * 10) / 10,
        avgTaps: Math.round(avgTaps * 10) / 10,
        petDmg: Math.round(petDmg * 10) / 10,
        timing: simResult.timing,
        adapter: simResult.adapter,
        detailedResults,
        maxHeap: maxHeap
      }});
    }}

    await fetch('/benchmark_report', {{
      method: 'POST',
      headers: {{ 'Content-Type': 'application/json' }},
      body: JSON.stringify({{ results }})
    }});
    statusEl.textContent = 'Benchmark complete.';
  }} catch (err) {{
    await fetch('/benchmark_report', {{
      method: 'POST',
      headers: {{ 'Content-Type': 'application/json' }},
      body: JSON.stringify({{ error: err.stack || err.message }})
    }});
    statusEl.textContent = 'Error: ' + err.message;
  }}
}}
run();
</script>
</body>
</html>
"""
    runner_html_path.write_text(runner_html)

    report_data = []
    report_event = threading.Event()

    class BenchmarkReportingHandler(http.server.SimpleHTTPRequestHandler):
        def do_POST(self):
            if self.path == "/benchmark_report":
                length = int(self.headers.get("Content-Length", 0))
                body = self.rfile.read(length)
                try:
                    report_data.append(json.loads(body.decode("utf-8")))
                except Exception as e:
                    report_data.append({"error": f"Failed to parse json: {e}"})
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                self.wfile.write(b'{"status":"ok"}')
                report_event.set()
            else:
                self.send_response(404)
                self.end_headers()

        def do_OPTIONS(self):
            self.send_response(200)
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
            self.send_header("Access-Control-Allow-Headers", "Content-Type")
            self.end_headers()

    class ReusableTCPServer(socketserver.TCPServer):
        allow_reuse_address = True

    server = ReusableTCPServer(("127.0.0.1", 0), BenchmarkReportingHandler)
    assigned_port = server.server_address[1]
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()

    url = f"http://127.0.0.1:{assigned_port}/threejs_webgl_des/benchmark_runner.html"
    cmd = get_browser_command(browser_choice, url)
    if not cmd:
        server.shutdown()
        server.server_close()
        runner_html_path.unlink(missing_ok=True)
        print(f"{RED}Error: Suitable browser not found for GPU benchmark.{RESET}")
        return None

    proc = None
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        finished = report_event.wait(timeout=timeout)
        if not finished:
            proc.kill()
            print(f"{RED}GPU benchmark timed out after {timeout}s.{RESET}")
            return None
    finally:
        if proc is not None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        server.shutdown()
        server.server_close()
        runner_html_path.unlink(missing_ok=True)

    if report_data and "error" in report_data[0]:
        print(f"{RED}Browser benchmark error: {report_data[0]['error']}{RESET}")
        return None

    return report_data[0].get("results", []) if report_data else None


def benchmark_cpu(repo_root, preset_id, iterations, workers=None):
    bin_path = repo_root / "bin" / "warlock_cpu_fixture"
    if not bin_path.exists():
        return None

    num_threads = workers or os.cpu_count() or 1
    t0 = time.perf_counter()

    def run_worker_batch(task):
        seed_start, count = task
        input_lines = []
        for offset in range(count):
            input_lines.append(f"PRESET {preset_id} {(seed_start + offset) & 0xFFFFFFFF} 180.0")
        input_str = "\n".join(input_lines) + "\n"
        proc = subprocess.Popen([str(bin_path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, text=True)
        stdout, _ = proc.communicate(input=input_str)
        return len([line for line in stdout.splitlines() if line.startswith("{")])

    batch_size = max(1, iterations // num_threads)
    tasks = []
    rem = iterations
    cur_seed = 42
    while rem > 0:
        c = min(rem, batch_size)
        tasks.append((cur_seed, c))
        cur_seed += c
        rem -= c

    with ThreadPoolExecutor(max_workers=num_threads) as pool:
        results = list(pool.map(run_worker_batch, tasks))

    elapsed = time.perf_counter() - t0
    completed = sum(results)
    throughput = (completed / elapsed) if elapsed > 0 else 0
    return {
        "completed": completed,
        "elapsedMs": elapsed * 1000.0,
        "throughput": throughput,
        "workers": num_threads
    }


def get_base_preset_config(repo_root, preset_id="ds_af"):
    bin_path = repo_root / "bin" / "warlock_cpu_fixture"
    res = subprocess.run([str(bin_path), "--list-presets"], capture_output=True, text=True)
    presets = json.loads(res.stdout.strip())
    p = next((x for x in presets if x["id"] == preset_id), presets[0])

    return {
        "duration": 180,
        "rotation": p.get("rotation", "shadow"),
        "spellPower": p.get("spellPower", 500),
        "intellect": 200, "spirit": 100, "stamina": 220,
        "hit": p.get("hit", 12),
        "crit": p.get("crit", 15),
        "mp5": 20, "distance": 30, "resistance": 0, "penetration": 0, "tapThreshold": 25,
        "book": False, "charges": False, "partialResists": True, "piercing": True,
        "corruption": p.get("corruption", True),
        "agony": p.get("agony", True),
        "immolate": p.get("immolate", False),
        "conflagrate": p.get("conflagrate", False),
        "shadowburn": p.get("shadowburn", False),
        "curseOfDoom": p.get("curseOfDoom", False),
        "incinerate": p.get("incinerate", False),
        "snfChance": p.get("snfChance", 0.0),
        "snfBonus": p.get("snfBonus", 0.0),
        "dotCrit": p.get("dotCrit", 1.5),
        "fnbCrit": p.get("fnbCrit", 0.0),
        "instantCorruption": p.get("instantCorruption", True),
        "nightfall": p.get("nightfall", True),
        "isb": p.get("isb", True),
        "ruin": p.get("ruin", True),
        "improvedTap": p.get("improvedTap", True),
        "sacImp": p.get("sacImp", False),
        "sacSucc": p.get("sacSucc", False),
        "masterDemo": p.get("masterDemo", 0),
        "petChoice": p.get("pet", "none"),
        "trinketSP": 0, "trinketDuration": 20, "trinketCD": 120,
        "petMult": p.get("petMult", 1.0),
        "corrMultiplier": p.get("corrMultiplier", 0.0),
        "shadowMultiplier": p.get("shadowMultiplier", 1.0),
        "fireMultiplier": p.get("fireMultiplier", 1.0),
        "malevolence": p.get("malevolence", 0.0),
        "afBonus": p.get("afBonus", 0.0),
        "maledictionBonus": p.get("maledictionBonus", 0.0),
        "shadowMasteryBonus": p.get("shadowMasteryBonus", 0.0),
        "improvedCorruptionBonus": p.get("improvedCorruptionBonus", 0.0),
        "siphonLife": p.get("siphonLife", False),
        "drainHope": p.get("drainHope", False),
        "improvedDrainsBonus": p.get("improvedDrainsBonus", 0.0),
        "soulSiphonBonus": p.get("soulSiphonBonus", 0.0),
        "decimation": p.get("decimation", False),
        "baneRank": p.get("baneRank", 0),
        "decimationRank": p.get("decimationRank", 0),
        "decimationSearing": preset_id == "sf_shadow_decimate",
        "demonicBrand": p.get("demonicBrand", False),
        "demonicBrandRank": p.get("demonicBrandRank", 0),
        "brandMult": p.get("brandMult", 1.0),
        "petFireboltMult": p.get("petFireboltMult", p.get("petMult", 1.0)),
        "petMeleeMult": p.get("petMeleeMult", p.get("petMult", 1.0)),
        "petLashMult": p.get("petLashMult", p.get("petMult", 1.0)),
        "demonicEnergies": p.get("demonicEnergies", 0.0),
        "demonicKnowledge": p.get("demonicKnowledge", 0)
    }


def main():
    parser = argparse.ArgumentParser(description="Warlock Simulation GPU/CPU Scaling & Throughput Benchmark")
    parser.add_argument("--scales", nargs="+", type=int, default=[1000, 5000, 10000, 50000, 100000, 250000, 500000, 1000000],
                        help="Simulation batch sizes to evaluate")
    parser.add_argument("--isolate-specs", action="store_true", help="Benchmark and rank every spec preset in isolation")
    parser.add_argument("--divergence-analysis", action="store_true", help="Analyze warp divergence overhead of mixed vs isolated specs")
    parser.add_argument("--spec-iterations", type=int, default=5000, help="Iterations per spec for isolated benchmark (default: 5000)")
    parser.add_argument("--gpu-only", action="store_true", help="Run only GPU benchmarks")
    parser.add_argument("--cpu-only", action="store_true", help="Run only CPU benchmarks")
    parser.add_argument("--multi-spec", action="store_true", help="Include multi-spec concurrency scaling benchmark")
    parser.add_argument("--preset", type=str, default="ds_af", help="Base spec preset to benchmark (default: ds_af)")
    parser.add_argument("--browser", type=str, default="chrome", choices=["chrome", "firefox", "auto"],
                        help="Browser choice for WebGL simulation (default: chrome)")
    parser.add_argument("--timeout", type=int, default=180, help="Max timeout in seconds for GPU run")
    parser.add_argument("--markdown", type=str, default=None, help="Export benchmark results table to markdown file")
    parser.add_argument("--json", action="store_true", help="Output raw JSON results")
    parser.add_argument("--fast", action="store_true", help="Skip detailed spell accounting and use one output stripe for GPU runs")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    base_cfg = get_base_preset_config(repo_root, args.preset)

    print(f"\n{BOLD}{CYAN}========================================================================================{RESET}")
    print(f"{BOLD}   Warlock Simulation Performance & Scaling Benchmark{RESET}")
    print(f"{DIM}   Environment: {platform.system()} {platform.machine()} ({os.cpu_count()} CPU cores available){RESET}")
    print(f"{BOLD}{CYAN}========================================================================================{RESET}\n")

    bin_path = repo_root / "bin" / "warlock_cpu_fixture"
    res = subprocess.run([str(bin_path), "--list-presets"], capture_output=True, text=True)
    all_presets = json.loads(res.stdout.strip()) if res.returncode == 0 else []

    # 1. Spec Isolation Benchmark
    spec_results = []
    if args.isolate_specs or args.divergence_analysis:
        print(f"{BOLD}[Spec Isolation] Benchmarking all {len(all_presets)} preset configurations ({args.spec_iterations:,} fights each)...{RESET}")
        spec_cases = []
        for p in all_presets:
            p_cfg = get_base_preset_config(repo_root, p["id"])
            p_cfg["iterations"] = args.spec_iterations
            p_cfg["seed"] = 42
            spec_cases.append({
                "name": p["id"],
                "configs": [p_cfg],
                "totalFights": args.spec_iterations,
                "preset_meta": p
            })
        spec_results = run_headless_gpu_benchmark(spec_cases, browser_choice=args.browser, timeout=args.timeout, fast=args.fast) or []
        
        if spec_results:
            spec_results.sort(key=lambda r: r["throughput"], reverse=True)
            fastest_tp = spec_results[0]["throughput"] if spec_results else 1

            print(f"\n{BOLD}{CYAN}==================================================================================================================={RESET}")
            print(f"{BOLD}   Isolated Spec Presets Benchmark & Bottleneck Leaderboard ({args.spec_iterations:,} fights each){RESET}")
            print(f"{BOLD}{CYAN}==================================================================================================================={RESET}\n")

            header_spec = f"{'Rank':<5} {'Preset ID':<24} {'Pet':<9} {'Rotation':<8} {'Time':<9} {'Throughput':<16} {'Events/Fight':<13} {'Slowdown':<10} {'Diagnosis'}"
            print(f"{BOLD}{header_spec}{RESET}")
            print(f"{DIM}{'-'*len(header_spec)}{RESET}")

            for rank, r in enumerate(spec_results, 1):
                p_meta = next((p for p in all_presets if p["id"] == r["name"]), {})
                pet = p_meta.get("pet", "none")
                rot = p_meta.get("rotation", "shadow")
                ev = r.get("avgEvents", 0)
                tp = r["throughput"]
                slowdown = f"{((fastest_tp - tp) / fastest_tp * 100):.1f}%" if fastest_tp > tp else "Baseline"
                
                # Diagnosis reasoning
                diag = []
                if ev > 240:
                    diag.append("High Event Density")
                if rot == "searing":
                    diag.append("1.5s Searing Spam")
                if pet == "imp":
                    diag.append("Imp Firebolt Loops")
                elif pet == "succubus":
                    diag.append("Melee + Lash")
                if p_meta.get("demonicBrand"):
                    diag.append("Brand Weave")
                if p_meta.get("decimation"):
                    diag.append("Decimation Weave")
                if not diag:
                    diag.append("Standard Low-Divergence")

                diag_str = ", ".join(diag)
                color = GREEN if rank <= 5 else (YELLOW if rank <= 15 else RED)
                print(f"{rank:<5} {r['name']:<24} {pet:<9} {rot:<8} {r['elapsedMs']:<9.1f} {color}{BOLD}{tp:<16,d}{RESET} {ev:<13.1f} {slowdown:<10} {DIM}{diag_str}{RESET}")

            print(f"{DIM}{'-'*len(header_spec)}{RESET}\n")

    # 2. Divergence Analysis
    if args.divergence_analysis and spec_results:
        print(f"{BOLD}[Divergence Analysis] Comparing Isolated vs Unified Multi-Spec Batching...{RESET}")
        multi_configs = []
        for p in all_presets:
            p_cfg = get_base_preset_config(repo_root, p["id"])
            p_cfg["iterations"] = args.spec_iterations
            p_cfg["seed"] = 42
            multi_configs.append(p_cfg)

        div_cases = [{
            "name": f"MultiSpec_Unified_{len(all_presets)}x{args.spec_iterations}",
            "configs": multi_configs,
            "totalFights": len(multi_configs) * args.spec_iterations
        }]
        div_results = run_headless_gpu_benchmark(div_cases, browser_choice=args.browser, timeout=args.timeout, fast=args.fast)
        if div_results:
            unified_res = div_results[0]
            sum_isolated_ms = sum(r["elapsedMs"] for r in spec_results)
            unified_ms = unified_res["elapsedMs"]
            divergence_penalty_pct = ((unified_ms - sum_isolated_ms) / sum_isolated_ms * 100.0) if sum_isolated_ms > 0 else 0

            print(f"\n{BOLD}{CYAN}----------------------------------------------------------------------------------------{RESET}")
            print(f"{BOLD}   Warp / SIMD Divergence Impact Breakdown{RESET}")
            print(f"{BOLD}{CYAN}----------------------------------------------------------------------------------------{RESET}")
            print(f"  • Total Fights Executed:                  {BOLD}{unified_res['totalFights']:,}{RESET} ({len(all_presets)} specs × {args.spec_iterations:,} fights)")
            print(f"  • Homogeneous Isolated Batches Sum:      {BOLD}{sum_isolated_ms:.1f} ms{RESET} (Avg {(sum_isolated_ms / len(spec_results)):.1f} ms/spec)")
            print(f"  • Unified Heterogeneous Batch Execution:  {BOLD}{unified_ms:.1f} ms{RESET}")
            if divergence_penalty_pct > 0:
                print(f"  • Warp Divergence Execution Penalty:      {BOLD}{RED}+{divergence_penalty_pct:.1f}% longer{RESET} when mixing slow & fast specs in same warp.")
            else:
                print(f"  • Warp Divergence Execution Penalty:      {BOLD}{GREEN}{divergence_penalty_pct:.1f}% (GPU occupancy fully saturated){RESET}")
            print(f"  • Unified Batch GPU Throughput:           {BOLD}{GREEN}{unified_res['throughput']:,} fights/sec{RESET}\n")

    # 3. Standard Single-Spec GPU Scaling Benchmark
    gpu_cases = []
    if not (args.isolate_specs and not args.scales):
        for it in args.scales:
            cfg = dict(base_cfg)
            cfg["iterations"] = it
            cfg["seed"] = 42
            gpu_cases.append({
                "name": f"Batch_{it}",
                "configs": [cfg],
                "totalFights": it
            })

    # Add Multi-Spec concurrent scaling tests if requested
    if args.multi_spec and all_presets:
        for multi_it in [1000, 5000, 10000]:
            multi_configs = []
            for p in all_presets:
                p_cfg = get_base_preset_config(repo_root, p["id"])
                p_cfg["iterations"] = multi_it
                p_cfg["seed"] = 42
                multi_configs.append(p_cfg)
            gpu_cases.append({
                "name": f"MultiSpec_22x{multi_it}",
                "configs": multi_configs,
                "totalFights": len(multi_configs) * multi_it
            })

    gpu_results = None
    if not args.cpu_only and gpu_cases:
        print(f"{BOLD}[Scaling] Running WebGL2 GPU Batch Scaling Benchmark...{RESET}")
        gpu_results = run_headless_gpu_benchmark(gpu_cases, browser_choice=args.browser, timeout=args.timeout, fast=args.fast)
        if not gpu_results:
            print(f"{RED}GPU benchmark failed to run or timed out.{RESET}")

    # 4. CPU Oracle Scaling Benchmark
    cpu_results = {}
    if not args.gpu_only and not args.isolate_specs:
        print(f"\n{BOLD}[CPU Oracle] Running Authoritative C++ CPU Oracle Benchmark...{RESET}")
        
        # Single thread baseline
        print(f"  • Benchmarking Single-Threaded CPU (1 core)...")
        st_res = benchmark_cpu(repo_root, args.preset, iterations=10000, workers=1)
        cpu_results["single_thread"] = st_res
        if st_res:
            print(f"    ↳ Single-Thread Throughput: {st_res['throughput']:,.0f} fights/s ({st_res['elapsedMs']:.1f}ms for {st_res['completed']:,} fights)")

        # Multi-threaded baseline
        num_cores = os.cpu_count() or 1
        print(f"  • Benchmarking Multi-Threaded CPU ({num_cores} cores)...")
        mt_res = benchmark_cpu(repo_root, args.preset, iterations=25000, workers=num_cores)
        cpu_results["multi_thread"] = mt_res
        if mt_res:
            print(f"    ↳ Multi-Thread ({num_cores}T) Throughput: {mt_res['throughput']:,.0f} fights/s ({mt_res['elapsedMs']:.1f}ms for {mt_res['completed']:,} fights)")

    # Display scaling summary
    if gpu_results:
        print(f"\n{BOLD}{CYAN}========================================================================================{RESET}")
        print(f"{BOLD}   Simulation Throughput & Scaling Summary ({args.preset}){RESET}")
        print(f"{BOLD}{CYAN}========================================================================================{RESET}\n")

        st_throughput = cpu_results.get("single_thread", {}).get("throughput", 0.0)
        mt_throughput = cpu_results.get("multi_thread", {}).get("throughput", 0.0)

        header = f"{'Batch / Workload':<24} {'Fights':<12} {'GPU Time':<12} {'GPU Throughput':<18} {'Speedup vs 1-CPU':<18} {'Speedup vs Multi-CPU'}"
        print(f"{BOLD}{header}{RESET}")
        print(f"{DIM}{'-'*len(header)}{RESET}")

        for r in gpu_results:
            total_fights = r["totalFights"]
            elapsed_ms = r["elapsedMs"]
            throughput = r["throughput"]
            speedup_st = f"{(throughput / st_throughput):.1f}x" if st_throughput > 0 else "—"
            speedup_mt = f"{(throughput / mt_throughput):.1f}x" if mt_throughput > 0 else "—"
            
            fights_str = f"{total_fights:,}"
            time_str = f"{elapsed_ms:.1f} ms" if elapsed_ms < 1000 else f"{elapsed_ms/1000:.2f} s"
            tp_str = f"{throughput:,.0f} fights/s"
            print(f"{r['name']:<24} {fights_str:<12} {time_str:<12} {GREEN}{BOLD}{tp_str:<18}{RESET} {CYAN}{speedup_st:<18}{RESET} {YELLOW}{speedup_mt}{RESET}")

        print(f"\n{DIM}{'-'*len(header)}{RESET}")
        max_tp = max(r["throughput"] for r in gpu_results)
        print(f"  • Peak GPU Throughput reached: {BOLD}{GREEN}{max_tp:,.0f} simulations / sec{RESET}\n")

    if args.json:
        print(json.dumps({"gpu": gpu_results, "specs": spec_results, "cpu": cpu_results}, indent=2))

    if args.markdown:
        md_lines = [
            "# Warlock Simulation Throughput & Spec Isolation Benchmark Report\n",
            f"- **Environment**: `{platform.system()} {platform.machine()}` ({os.cpu_count()} CPU cores)",
            f"- **Date / Time**: `{time.strftime('%Y-%m-%d %H:%M:%S UTC', time.gmtime())}`\n"
        ]
        
        if spec_results:
            md_lines.append("## Isolated Spec Presets Performance & Bottleneck Analysis\n")
            md_lines.append("| Rank | Preset ID | Pet | Rotation | Time (ms) | GPU Throughput | Mean Events | Slowdown vs Best | Bottleneck Drivers |")
            md_lines.append("|---|---|---|---|---|---|---|---|---|")
            fastest_tp = spec_results[0]["throughput"] if spec_results else 1
            for rank, r in enumerate(spec_results, 1):
                p_meta = next((p for p in all_presets if p["id"] == r["name"]), {})
                pet = p_meta.get("pet", "none")
                rot = p_meta.get("rotation", "shadow")
                ev = r.get("avgEvents", 0)
                tp = r["throughput"]
                slowdown = f"{((fastest_tp - tp) / fastest_tp * 100):.1f}%" if fastest_tp > tp else "Baseline"
                diag = []
                if ev > 240: diag.append("High Event Count")
                if rot == "searing": diag.append("1.5s Searing Spam")
                if pet == "imp": diag.append("Imp Firebolt Loops")
                elif pet == "succubus": diag.append("Melee + Lash")
                if p_meta.get("demonicBrand"): diag.append("Brand Weave")
                if p_meta.get("decimation"): diag.append("Decimation Weave")
                if not diag: diag.append("Low Divergence")
                md_lines.append(f"| {rank} | `{r['name']}` | {pet} | {rot} | {r['elapsedMs']:.1f} | **{tp:,} f/s** | {ev:.1f} | `{slowdown}` | {', '.join(diag)} |")
            md_lines.append("\n")

        if gpu_results:
            md_lines.append("## Scaling Throughput Summary\n")
            md_lines.append("| Batch / Workload | Total Fights | GPU Latency | GPU Throughput |")
            md_lines.append("|---|---|---|---|")
            for r in gpu_results:
                time_str = f"{r['elapsedMs']:.1f} ms" if r['elapsedMs'] < 1000 else f"{r['elapsedMs']/1000:.2f} s"
                md_lines.append(f"| `{r['name']}` | {r['totalFights']:,} | {time_str} | **{r['throughput']:,} fights/s** |")
            md_lines.append("\n")

        Path(args.markdown).write_text("\n".join(md_lines))
        print(f"{GREEN}✓ Benchmark report written to {args.markdown}{RESET}\n")


if __name__ == "__main__":
    main()
