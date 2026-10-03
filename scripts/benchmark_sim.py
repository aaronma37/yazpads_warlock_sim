#!/usr/bin/env python3
"""
Benchmark suite for Warlock Simulation: WebGL2 GPU Shader vs Native C++ CPU Oracle.

Evaluates simulation throughput (fights/second), execution latency, batch scaling,
and multi-spec concurrency across both GPU and multi-threaded CPU environments.

Usage:
  python3 scripts/benchmark_sim.py
  python3 scripts/benchmark_sim.py --scales 1000 5000 10000 50000 100000 500000
  python3 scripts/benchmark_sim.py --multi-spec
  python3 scripts/benchmark_sim.py --gpu-only
  python3 scripts/benchmark_sim.py --browser chrome
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
        # Suppress logging
        handler.log_message = lambda *args: None
        self.httpd = socketserver.TCPServer(("127.0.0.1", port), handler)
        self.port = self.httpd.server_address[1]
        self.thread = threading.Thread(target=self.httpd.serve_forever, daemon=True)
        self.thread.start()

    def stop(self):
        self.httpd.shutdown()
        self.httpd.server_close()


def get_browser_command(choice="chrome", url=""):
    # Check flatpak chrome first (common in SteamOS / Linux desktop)
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

    # Firefox fallback
    firefox = shutil.which("firefox")
    if firefox:
        return [firefox, "--headless", url]

    return None


def run_headless_gpu_benchmark(cases, browser_choice="chrome", timeout=120):
    runner_html_path = Path("threejs_webgl_des/benchmark_runner.html")
    cases_js = json.dumps(cases)

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
    const results = [];

    // Warm-up pass (compile shader and initialize WebGL buffers)
    await runSimulation({{ duration: 180, iterations: 100, seed: 1 }});

    for (const testCase of cases) {{
      const configs = testCase.configs;
      const totalFights = testCase.totalFights;
      
      const t0 = performance.now();
      let simResult;
      if (configs.length === 1) {{
        simResult = await runSimulation(configs[0]);
      }} else {{
        simResult = await runMultiSimulation(configs, {{ iterations: configs[0].iterations }});
      }}
      const elapsedMs = performance.now() - t0;
      const meanDps = (simResult.summary ? simResult.summary.mean : 
                      (simResult.results ? simResult.results.reduce((a, b) => a + (b.summary ? b.summary.mean : 0), 0) / simResult.results.length : 0));

      results.push({{
        name: testCase.name,
        numConfigs: configs.length,
        iterationsPerConfig: configs[0].iterations,
        totalFights: totalFights,
        elapsedMs: elapsedMs,
        throughput: Math.round((totalFights / (elapsedMs / 1000))),
        meanDps: meanDps
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

    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        finished = report_event.wait(timeout=timeout)
        if not finished:
            proc.kill()
            print(f"{RED}GPU benchmark timed out after {timeout}s.{RESET}")
            return None
    finally:
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
        # Interactive stream protocol in cpu_fixture
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
    parser.add_argument("--gpu-only", action="store_true", help="Run only GPU benchmarks")
    parser.add_argument("--cpu-only", action="store_true", help="Run only CPU benchmarks")
    parser.add_argument("--multi-spec", action="store_true", help="Include multi-spec concurrency scaling benchmark")
    parser.add_argument("--preset", type=str, default="ds_af", help="Base spec preset to benchmark (default: ds_af)")
    parser.add_argument("--browser", type=str, default="chrome", choices=["chrome", "firefox", "auto"],
                        help="Browser choice for WebGL simulation (default: chrome)")
    parser.add_argument("--timeout", type=int, default=180, help="Max timeout in seconds for GPU run")
    parser.add_argument("--markdown", type=str, default=None, help="Export benchmark results table to markdown file")
    parser.add_argument("--json", action="store_true", help="Output raw JSON results")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    base_cfg = get_base_preset_config(repo_root, args.preset)

    print(f"\n{BOLD}{CYAN}========================================================================================{RESET}")
    print(f"{BOLD}   Warlock Simulation Performance & Scaling Benchmark{RESET}")
    print(f"{DIM}   Environment: {platform.system()} {platform.machine()} ({os.cpu_count()} CPU cores available){RESET}")
    print(f"{BOLD}{CYAN}========================================================================================{RESET}\n")

    # 1. Single-Spec GPU Scaling Benchmark
    gpu_cases = []
    for it in args.scales:
        cfg = dict(base_cfg)
        cfg["iterations"] = it
        cfg["seed"] = 42
        gpu_cases.append({
            "name": f"Batch_{it}",
            "configs": [cfg],
            "totalFights": it
        })

    # Add Multi-Spec concurrent tests if requested
    if args.multi_spec:
        # Load all available presets
        bin_path = repo_root / "bin" / "warlock_cpu_fixture"
        res = subprocess.run([str(bin_path), "--list-presets"], capture_output=True, text=True)
        all_presets = json.loads(res.stdout.strip())

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
    if not args.cpu_only:
        print(f"{BOLD}[1/2] Running WebGL2 GPU Shader Simulation Benchmark...{RESET}")
        gpu_results = run_headless_gpu_benchmark(gpu_cases, browser_choice=args.browser, timeout=args.timeout)
        if not gpu_results:
            print(f"{RED}GPU benchmark failed to run or timed out.{RESET}")

    # 2. CPU Oracle Scaling Benchmark
    cpu_results = {}
    if not args.gpu_only:
        print(f"\n{BOLD}[2/2] Running Authoritative C++ CPU Oracle Benchmark...{RESET}")
        cpu_scales = [s for s in args.scales if s <= 50000] # Limit CPU to reasonable interactive durations
        
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

    # Display results table
    print(f"\n{BOLD}{CYAN}========================================================================================{RESET}")
    print(f"{BOLD}   Simulation Throughput & Scaling Summary{RESET}")
    print(f"{BOLD}{CYAN}========================================================================================{RESET}\n")

    st_throughput = cpu_results.get("single_thread", {}).get("throughput", 1.0)
    mt_throughput = cpu_results.get("multi_thread", {}).get("throughput", 1.0)

    header = f"{'Batch / Workload':<24} {'Fights':<12} {'GPU Time':<12} {'GPU Throughput':<18} {'Speedup vs 1-CPU':<18} {'Speedup vs Multi-CPU'}"
    print(f"{BOLD}{header}{RESET}")
    print(f"{DIM}{'-'*len(header)}{RESET}")

    if gpu_results:
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
    print(f"{BOLD}Key Insights:{RESET}")
    if gpu_results and len(gpu_results) > 0:
        max_tp = max(r["throughput"] for r in gpu_results)
        print(f"  • Peak GPU Throughput reached: {BOLD}{GREEN}{max_tp:,.0f} simulations / sec{RESET}")
        if mt_throughput > 0:
            print(f"  • Peak GPU Acceleration factor: {BOLD}{CYAN}{(max_tp / mt_throughput):.1f}x faster{RESET} than all {os.cpu_count()} CPU cores combined.")
            print(f"  • Peak GPU Acceleration factor: {BOLD}{CYAN}{(max_tp / st_throughput):.1f}x faster{RESET} than a single CPU core.")
    print()

    if args.markdown:
        md_lines = [
            "# Warlock Simulation Throughput & Scaling Benchmark Report\n",
            f"- **Environment**: `{platform.system()} {platform.machine()}` ({os.cpu_count()} CPU cores)",
            f"- **Date / Time**: `{time.strftime('%Y-%m-%d %H:%M:%S UTC', time.gmtime())}`",
            f"- **Single-Core CPU Baseline**: `{st_throughput:,.0f} fights/s`",
            f"- **Multi-Core CPU ({os.cpu_count()}T) Baseline**: `{mt_throughput:,.0f} fights/s`\n",
            "| Batch / Workload | Total Fights | GPU Latency | GPU Throughput | Speedup vs 1-CPU | Speedup vs Multi-CPU |",
            "|---|---|---|---|---|---|"
        ]
        if gpu_results:
            for r in gpu_results:
                total_fights = r["totalFights"]
                elapsed_ms = r["elapsedMs"]
                throughput = r["throughput"]
                speedup_st = f"{(throughput / st_throughput):.1f}x" if st_throughput > 0 else "—"
                speedup_mt = f"{(throughput / mt_throughput):.1f}x" if mt_throughput > 0 else "—"
                time_str = f"{elapsed_ms:.1f} ms" if elapsed_ms < 1000 else f"{elapsed_ms/1000:.2f} s"
                md_lines.append(f"| `{r['name']}` | {total_fights:,} | {time_str} | **{throughput:,.0f} fights/s** | `{speedup_st}` | `{speedup_mt}` |")
        
        md_content = "\n".join(md_lines) + "\n"
        Path(args.markdown).write_text(md_content)
        print(f"{GREEN}✓ Benchmark report written to {args.markdown}{RESET}\n")


if __name__ == "__main__":
    main()
