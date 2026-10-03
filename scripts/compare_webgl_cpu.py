#!/usr/bin/env python3
"""
CLI Tool: High-Fidelity WebGL2 DES vs Native C++ Oracle Parity Pipeline
======================================================================
Treats the native C++ Discrete Event Simulator (WarlockSimulator) as the
authoritative ground truth. Compares WebGL2 shader simulation against the CPU
engine across full spec presets, parameter sweeps, and reference scenarios.

Usage examples:
  # 1. Run full spec presets comparison (e.g. 12/31/8 Aff/DP, SM/Ruin, DS/Ruin, Fire Destro)
  python3 scripts/compare_webgl_cpu.py --presets

  # 2. Run spec presets with detailed per-spell and pet damage breakdowns
  python3 scripts/compare_webgl_cpu.py --presets --breakdown

  # 3. Compare a single specific preset
  python3 scripts/compare_webgl_cpu.py --preset aff_dp_brand --breakdown

  # 4. Run standard reference test fixtures
  python3 scripts/compare_webgl_cpu.py --fixtures

  # 5. Inspect side-by-side event trace for a specific case
  python3 scripts/compare_webgl_cpu.py --diff-trace "fire"

  # 6. Compare a custom ad-hoc configuration on the fly
  python3 scripts/compare_webgl_cpu.py --custom --sp 650 --hit 16 --crit 20 --duration 120 --rotation fire

  # 7. Run parameter sweeps (sp, hit, crit, duration)
  python3 scripts/compare_webgl_cpu.py --sweep sp --sweep-min 100 --sweep-max 800 --sweep-step 100

  # 8. Export full markdown report with per-spell breakdowns
  python3 scripts/compare_webgl_cpu.py --presets --markdown parity_report_presets.md
"""

import os
import sys
import time
import json
import socket
import argparse
from concurrent.futures import ThreadPoolExecutor
import subprocess
import threading
import http.server
import socketserver
from pathlib import Path

# Terminal Colors
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
MAGENTA = "\033[95m"
BOLD = "\033[1m"
DIM = "\033[2m"
RESET = "\033[0m"

CORE_SPELL_NAMES = ["Shadow Bolt", "Corruption", "Bane of Agony", "Immolate", "Incinerate", "Searing Pain"]
CPU_SPELL_IDS = {
    1: "Shadow Bolt", 2: "Corruption", 5: "Bane of Agony", 6: "Curse of Doom",
    8: "Immolate", 9: "Searing Pain", 10: "Shadowburn", 11: "Conflagrate",
    12: "Incinerate", 13: "Soul Fire", 14: "Wrack / Drain Hope", 15: "Drain Life",
    16: "Drain Soul", 17: "Siphon Life", 20: "Imp Firebolt (Pet)",
    21: "Succubus Lash of Pain (Pet)", 22: "Succubus Melee (Pet)", 31: "Demonic Brand (Pet Proc)"
}


def find_free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('', 0))
        return s.getsockname()[1]


class ParityReportServer(http.server.SimpleHTTPRequestHandler):
    report_data = None
    report_event = None

    def log_message(self, format, *args):
        pass  # Suppress HTTP request logs in console

    def do_POST(self):
        if self.path == '/api/report':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            ParityReportServer.report_data = json.loads(body)
            self.send_response(200)
            self.send_header('Content-Type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'OK')
            if ParityReportServer.report_event:
                ParityReportServer.report_event.set()
        elif self.path == '/api/error':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            ParityReportServer.report_data = {"error": body, "passed": 0, "total": 0, "results": []}
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b'OK')
            if ParityReportServer.report_event:
                ParityReportServer.report_event.set()
        else:
            self.send_response(404)
            self.end_headers()


def start_server(web_dir, port, event):
    ParityReportServer.report_event = event
    ParityReportServer.report_data = None
    handler = lambda *args, **kwargs: ParityReportServer(*args, directory=str(web_dir), **kwargs)
    httpd = socketserver.TCPServer(("127.0.0.1", port), handler)
    thread = threading.Thread(target=httpd.serve_forever, daemon=True)
    thread.start()
    return httpd


def rebuild_cpp_oracle(repo_root):
    bin_dir = repo_root / "bin"
    bin_dir.mkdir(exist_ok=True)
    fixture_bin = bin_dir / "warlock_cpu_fixture"
    print(f"{CYAN}Compiling native C++ oracle adapter ({fixture_bin})...{RESET}")
    cmd = [
        "g++", "-std=c++20", "-O2", "-ffunction-sections", "-fdata-sections",
        "-I.", "-Isrc/sim", "-Isrc/sim/common", "-Ithird_party/rl-tools/include",
        "threejs_webgl_des/validation/cpu_fixture.cpp",
        "src/sim/warlock/warlock_sim.cpp",
        "src/sim/common/gear.cpp",
        "src/sim/warlock/imitation_training.cpp",
        "-Wl,--gc-sections", "-pthread", "-o", str(fixture_bin)
    ]
    res = subprocess.run(cmd, cwd=str(repo_root), capture_output=True, text=True)
    if res.returncode != 0:
        print(f"{RED}Failed to build C++ oracle binary:{RESET}\n{res.stderr}")
        return False
    print(f"{GREEN}✓ Built native C++ oracle binary successfully.{RESET}")

    gen_script = repo_root / "scripts" / "generate_fixtures.py"
    if gen_script.exists():
        print(f"{CYAN}Regenerating cpu-fixtures.json from C++ simulator...{RESET}")
        res_gen = subprocess.run([sys.executable, str(gen_script)], capture_output=True, text=True)
        if res_gen.returncode != 0:
            print(f"{RED}Failed to regenerate fixtures:{RESET}\n{res_gen.stderr}")
            return False
        print(f"{GREEN}✓ Updated reference fixtures successfully.{RESET}\n")
    return True


def get_available_presets(repo_root):
    bin_path = repo_root / "bin" / "warlock_cpu_fixture"
    if not bin_path.exists():
        if not rebuild_cpp_oracle(repo_root):
            return []
    res = subprocess.run([str(bin_path), "--list-presets"], capture_output=True, text=True)
    if res.returncode != 0:
        return []
    try:
        return json.loads(res.stdout.strip())
    except Exception:
        return []


def run_cpp_oracle_preset(repo_root, preset_id, seed=42, duration=180.0, iterations=1, workers=16):
    bin_path = repo_root / "bin" / "warlock_cpu_fixture"
    if not bin_path.exists():
        if not rebuild_cpp_oracle(repo_root):
            return None
    def run_seed(offset):
        cmd = [str(bin_path), "--preset", preset_id, str((seed + offset) & 0xFFFFFFFF), str(duration)]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            return None
        try:
            return json.loads(res.stdout.strip())
        except Exception:
            return None
    workers = min(iterations, os.cpu_count() or 1, workers)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        samples = list(pool.map(run_seed, range(iterations)))
    if any(sample is None for sample in samples):
        return None
    if iterations == 1:
        return samples[0]

    # Average scalar outcome fields and per-spell statistics across independent seeds.
    # Traces and final RNG state are single-run diagnostics and have no aggregate meaning.
    averaged = {}
    for key, value in samples[0].items():
        if isinstance(value, (int, float)) and not isinstance(value, bool):
            averaged[key] = sum(sample.get(key, 0) for sample in samples) / iterations
    averaged["spellBreakdown"] = []
    first_spells = samples[0].get("spellBreakdown", [])
    for spell in first_spells:
        spell_id = spell.get("id")
        matching = [next((item for item in sample.get("spellBreakdown", []) if item.get("id") == spell_id), {}) for sample in samples]
        averaged["spellBreakdown"].append({
            key: (sum(item.get(key, 0) for item in matching) / iterations
                  if isinstance(value, (int, float)) and not isinstance(value, bool) else value)
            for key, value in spell.items()
        })
    return averaged


def run_cpp_oracle_custom(repo_root, config):
    bin_path = repo_root / "bin" / "warlock_cpu_fixture"
    if not bin_path.exists():
        if not rebuild_cpp_oracle(repo_root):
            return None

    fields = [
        'seed', 'duration', 'rotation', 'spellPower', 'intellect', 'spirit', 'hit', 'crit', 'mp5', 'distance',
        'resistance', 'penetration', 'tapThreshold', 'book', 'charges', 'partialResists', 'piercing', 'corruption', 'agony',
        'immolate', 'instantCorruption', 'nightfall', 'isb', 'ruin', 'improvedTap',
        'sacImp', 'sacSucc', 'masterDemo', 'petChoice',
        'trinketSP', 'trinketDuration', 'trinketCD', 'shadowMultiplier', 'fireMultiplier'
    ]
    tokens = []
    for k in fields:
        val = config.get(k, 0)
        if isinstance(val, bool):
            tokens.append(str(int(val)))
        else:
            tokens.append(str(val))
    input_str = " ".join(tokens) + "\n"
    res = subprocess.run([str(bin_path)], input=input_str, capture_output=True, text=True)
    if res.returncode != 0:
        return None
    try:
        return json.loads(res.stdout.strip())
    except Exception:
        return None


def run_headless_simulation(port, report_event, custom_cases=None, filter_pattern=None, browser_choice="chrome"):
    runner_html_path = Path("threejs_webgl_des/cli_runner.html")
    filter_js = json.dumps(filter_pattern) if filter_pattern else "null"
    cases_js = json.dumps(custom_cases) if custom_cases is not None else "null"

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
<div id="status">Running WebGL shader simulations...</div>
<script type="module">
import {{ runSimulation }} from './src/engine.js';
import {{ compare }} from './validation/compare.js';

async function run() {{
  const statusEl = document.getElementById('status');
  try {{
    let cases = {cases_js};
    const batchConfigs = {str(custom_cases is not None and len(custom_cases) > 1).lower()};
    if (!cases) {{
      const res = await fetch('./validation/cpu-fixtures.json');
      if (!res.ok) throw new Error('Could not load cpu-fixtures.json');
      const data = await res.json();
      cases = data.cases;
    }}
    const filterPat = {filter_js};
    if (filterPat) {{
      cases = cases.filter(c => c.name.toLowerCase().includes(filterPat.toLowerCase()));
    }}

    const results = [];
    let batchedRuns = null;
    if (batchConfigs && cases.length > 1) {{
      statusEl.textContent = `Running ${{cases.length}} configurations in one GPU batch...`;
      const batch = await runSimulation(cases.map(fixture => fixture.config));
      batchedRuns = batch.results;
    }}
    const t0 = performance.now();

    for (let i = 0; i < cases.length; i++) {{
      const fixture = cases[i];
      try {{
        const simRes = batchedRuns ? batchedRuns[i] : await runSimulation(fixture.config);
        const states = simRes.states || [];
        const st = {{}};
        if (states.length > 0) {{
          for (const key of Object.keys(states[0])) {{
            const values = states.map(s => s[key]);
            st[key] = values.every(v => typeof v === 'number')
              ? values.reduce((sum, value) => sum + value, 0) / values.length
              : values[0];
          }}
        }}
        const exp = fixture.expected || null;
        
        let pass = true;
        const failures = [];

        if (exp && fixture.config.iterations > 1) {{
          const dps = simRes.summary ? simRes.summary.mean : (st.total || 0) / (fixture.config.duration || 180);
          const expectedDps = exp.dps || exp.total / (fixture.config.duration || 180);
          pass = expectedDps > 0 && Math.abs((dps - expectedDps) / expectedDps) <= 0.015;
          if (!pass) failures.push(`Mean DPS differs by ${{((dps - expectedDps) / expectedDps * 100).toFixed(2)}}%`);
        }} else if (exp) {{
          const check = compare(simRes, fixture);
          pass = check.pass;
          if (check.failures && check.failures.length > 0) {{
            failures.push(...check.failures);
          }}
        }}

        results.push({{
          index: i + 1,
          name: fixture.name,
          pass: pass,
          failures: failures,
          meanDps: simRes.summary ? simRes.summary.mean : (st.total || 0) / (fixture.config.duration || 180),
          expectedDps: exp ? (exp.dps || exp.total / (fixture.config.duration || 180)) : null,
          totalDmg: st.total,
          expectedTotalDmg: exp ? exp.total : null,
          manaSpent: st.spent,
          expectedManaSpent: exp ? exp.spent : null,
          manaGained: st.gained,
          expectedManaGained: exp ? exp.gained : null,
          finalMana: st.mana,
          expectedFinalMana: exp ? exp.mana : null,
          lifeTaps: st.taps,
          expectedLifeTaps: exp ? exp.taps : null,
          nightfallProcs: st.procs,
          expectedNightfallProcs: exp ? exp.procs : null,
          isbProcs: st.isbProcs,
          isbConsumed: st.isbConsumed,
          damageBreakdown: [st.damage0, st.damage1, st.damage2, st.damage3, st.damage4, st.damage5],
          castsBreakdown: [st.casts0, st.casts1, st.casts2, st.casts3, st.casts4, st.casts5],
          hitsBreakdown: [st.hits0, st.hits1, st.hits2, st.hits3, st.hits4, st.hits5],
          critsBreakdown: [st.crits0, st.crits1, st.crits2, st.crits3, st.crits4, st.crits5],
          missesBreakdown: [st.misses0, st.misses1, st.misses2, st.misses3, st.misses4, st.misses5],
          expectedDamageBreakdown: exp ? [exp.damage0, exp.damage1, exp.damage2, exp.damage3, exp.damage4, exp.damage5] : null,
          expectedCastsBreakdown: exp ? [exp.casts0, exp.casts1, exp.casts2, exp.casts3, exp.casts4, exp.casts5] : null,
          expectedHitsBreakdown: exp ? [exp.hits0, exp.hits1, exp.hits2, exp.hits3, exp.hits4, exp.hits5] : null,
          expectedCritsBreakdown: exp ? [exp.crits0, exp.crits1, exp.crits2, exp.crits3, exp.crits4, exp.crits5] : null,
          expectedMissesBreakdown: exp ? [exp.misses0, exp.misses1, exp.misses2, exp.misses3, exp.misses4, exp.misses5] : null,
          cpuSpellBreakdown: exp ? exp.spellBreakdown : null,
          petDamage: st.petDamage || 0,
          petBrandDamage: st.petBrandDamage || 0,
          expectedPetDamage: exp ? (exp.petDamage || 0) : null,
          expectedPetBrandDamage: exp ? (exp.demonicBrandDamage || 0) : null,
          petCasts: st.petCasts || 0,
          expectedPetCasts: exp && exp.spellBreakdown
            ? exp.spellBreakdown.filter(s => [20, 21, 22].includes(s.id)).reduce((n, s) => n + (s.casts || 0), 0)
            : null,
          trace: simRes.trace,
          expectedTrace: exp ? exp.damageTrace : null,
          elapsedMs: simRes.timing ? simRes.timing.elapsedMs : 0
        }});
      }} catch (err) {{
        results.push({{
          index: i + 1,
          name: fixture.name,
          pass: false,
          failures: [err.message]
        }});
      }}
    }}
    const totalElapsed = performance.now() - t0;
    const finalReport = {{
      passed: results.filter(r => r.pass).length,
      total: results.length,
      totalElapsedMs: totalElapsed,
      results
    }};
    await fetch('/api/report', {{
      method: 'POST',
      headers: {{ 'Content-Type': 'application/json' }},
      body: JSON.stringify(finalReport)
    }});
    statusEl.textContent = 'DONE';
  }} catch (e) {{
    await fetch('/api/error', {{
      method: 'POST',
      body: e.stack || e.message
    }});
  }}
}}
run();
</script>
</body>
</html>
"""
    runner_html_path.write_text(runner_html)
    url = f"http://127.0.0.1:{port}/cli_runner.html"

    cmd = [
        "flatpak", "run", "--share=network", "--device=all", "com.google.Chrome",
        "--headless",
        "--no-sandbox",
        "--disable-setuid-sandbox",
        "--enable-webgl",
        "--use-angle=swiftshader",
        "--enable-unsafe-swiftshader",
        url
    ]

    proc = None
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if report_event.wait(timeout=35.0):
            return ParityReportServer.report_data
        else:
            return None
    finally:
        if proc:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
        if runner_html_path.exists():
            runner_html_path.unlink(missing_ok=True)


def print_comprehensive_spell_breakdown(result, indent="  "):
    cpu_spells = result.get('cpuSpellBreakdown') or []
    gpu_dmg = result.get('damageBreakdown', [])
    gpu_casts = result.get('castsBreakdown', [])
    gpu_hits = result.get('hitsBreakdown', [])
    gpu_crits = result.get('critsBreakdown', [])
    gpu_misses = result.get('missesBreakdown', [])

    total_cpu_dmg = result.get('expectedTotalDmg') or 1.0
    total_gpu_dmg = result.get('totalDmg') or 1.0

    print(f"{indent}{BOLD}{'Spell / Source':<26} {'GPU Dmg':<10} {'CPU Dmg':<10} {'GPU DPS':<9} {'CPU DPS':<9} {'Share(G/C)':<12} {'Casts(G/C)':<11} {'Status'}{RESET}")
    print(f"{indent}{DIM}{'-'*98}{RESET}")

    # Map core spells from GPU
    core_map = {1: 0, 2: 1, 5: 2, 8: 3, 12: 4, 9: 5}

    for sp in cpu_spells:
        sp_id = sp.get('id', 0)
        # GPU only exposes a pet damage total, so compare it with the CPU's
        # matching total (which includes Demonic Brand) below rather than
        # allocating it proportionally across unrelated pet attacks.
        if sp_id in (20, 21, 22, 31):
            continue
        sp_name = sp.get('name', 'Unknown')
        c_dmg = sp.get('damage', 0.0)
        c_dps = sp.get('dps', 0.0)
        c_casts = sp.get('casts', 0)
        c_share = (c_dmg / total_cpu_dmg * 100.0) if total_cpu_dmg > 0 else 0.0

        g_dmg = 0.0
        g_dps = 0.0
        g_casts = 0
        g_share = 0.0

        if sp_id in (5, 6):
            # Curse channel (Agony + Doom)
            total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (5, 6))
            g_total = gpu_dmg[2] if len(gpu_dmg) > 2 and gpu_dmg[2] is not None else 0.0
            g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
            g_dps = g_dmg / 180.0
            g_casts = round(c_casts / max(1, sum(s.get('casts', 0) for s in cpu_spells if s.get('id') in (5, 6))) * (gpu_casts[2] if len(gpu_casts) > 2 and gpu_casts[2] is not None else 0))
            g_share = (g_dmg / total_gpu_dmg * 100.0) if total_gpu_dmg > 0 else 0.0
        elif sp_id in (8, 11, 13):
            # Fire DoT & Burst channel (Immolate + Conflagrate + Soul Fire)
            total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (8, 11, 13))
            g_total = gpu_dmg[3] if len(gpu_dmg) > 3 and gpu_dmg[3] is not None else 0.0
            g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
            g_dps = g_dmg / 180.0
            g_casts = round(c_casts / max(1, sum(s.get('casts', 0) for s in cpu_spells if s.get('id') in (8, 11, 13))) * (gpu_casts[3] if len(gpu_casts) > 3 and gpu_casts[3] is not None else 0))
            g_share = (g_dmg / total_gpu_dmg * 100.0) if total_gpu_dmg > 0 else 0.0
        elif sp_id in (1, 10):
            # Shadow direct channel (Shadow Bolt + Shadowburn)
            total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (1, 10))
            g_total = gpu_dmg[0] if len(gpu_dmg) > 0 and gpu_dmg[0] is not None else 0.0
            g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
            g_dps = g_dmg / 180.0
            g_casts = round(c_casts / max(1, sum(s.get('casts', 0) for s in cpu_spells if s.get('id') in (1, 10))) * (gpu_casts[0] if len(gpu_casts) > 0 and gpu_casts[0] is not None else 0))
            g_share = (g_dmg / total_gpu_dmg * 100.0) if total_gpu_dmg > 0 else 0.0
        elif sp_id in (2, 14, 15, 16, 17):
            # Affliction DoTs & channels (Corruption, Wrack, Drain Life, Drain Soul, Siphon Life)
            total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (2, 14, 15, 16, 17))
            g_total = gpu_dmg[1] if len(gpu_dmg) > 1 and gpu_dmg[1] is not None else 0.0
            g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
            g_dps = g_dmg / 180.0
            g_casts = round(c_casts / max(1, sum(s.get('casts', 0) for s in cpu_spells if s.get('id') in (2, 14, 15, 16, 17))) * (gpu_casts[1] if len(gpu_casts) > 1 and gpu_casts[1] is not None else 0))
            g_share = (g_dmg / total_gpu_dmg * 100.0) if total_gpu_dmg > 0 else 0.0
        elif sp_id in core_map:
            idx = core_map[sp_id]
            g_dmg = gpu_dmg[idx] if idx < len(gpu_dmg) and gpu_dmg[idx] is not None else 0.0
            g_dps = g_dmg / 180.0
            g_casts = gpu_casts[idx] if idx < len(gpu_casts) and gpu_casts[idx] is not None else 0
            g_share = (g_dmg / total_gpu_dmg * 100.0) if total_gpu_dmg > 0 else 0.0
        delta_dps = g_dps - c_dps
        pct_diff = (delta_dps / c_dps * 100.0) if c_dps > 0.01 else 0.0

        if g_dmg == 0.0 and c_dmg > 0.0:
            status = f"{RED}MISSING (-100%){RESET}"
        elif abs(pct_diff) <= 1.5:
            status = f"{GREEN}PASS ({pct_diff:+.1f}%){RESET}"
        else:
            status = f"{YELLOW}DIFF ({pct_diff:+.1f}%){RESET}"

        share_str = f"{g_share:.1f}%/{c_share:.1f}%"
        casts_str = f"{g_casts}/{c_casts}"
        print(f"{indent}{sp_name:<26} {g_dmg:<10.1f} {c_dmg:<10.1f} {g_dps:<9.1f} {c_dps:<9.1f} {share_str:<12} {casts_str:<11} {status}")

    gpu_pet = result.get('petDamage', 0.0)
    gpu_brand = result.get('petBrandDamage', 0.0)
    cpu_pet = result.get('expectedPetDamage')
    cpu_brand = result.get('expectedPetBrandDamage')
    if cpu_pet is not None and (gpu_pet > 0.0 or cpu_pet > 0.0):
        cpu_brand = cpu_brand or 0.0
        pet_rows = [
            ("Pet Attacks", gpu_pet - gpu_brand, cpu_pet - cpu_brand,
             result.get('petCasts', 0), result.get('expectedPetCasts')),
            ("Demonic Brand (Pet)", gpu_brand, cpu_brand, None, None),
        ]
        for pet_name, gpu_dmg, cpu_dmg, gpu_casts, cpu_casts in pet_rows:
            if gpu_dmg <= 0.0 and cpu_dmg <= 0.0:
                continue
            gpu_dps = gpu_dmg / 180.0
            cpu_dps = cpu_dmg / 180.0
            pet_pct = ((gpu_dps - cpu_dps) / cpu_dps * 100.0) if cpu_dps > 0.01 else 0.0
            pet_status = f"{GREEN}PASS ({pet_pct:+.1f}%){RESET}" if abs(pet_pct) <= 1.5 else f"{YELLOW}DIFF ({pet_pct:+.1f}%){RESET}"
            gpu_share = gpu_dmg / total_gpu_dmg * 100.0 if total_gpu_dmg > 0 else 0.0
            cpu_share = cpu_dmg / total_cpu_dmg * 100.0 if total_cpu_dmg > 0 else 0.0
            casts_str = f"{gpu_casts}/{cpu_casts:.2f}" if cpu_casts is not None else "—"
            print(f"{indent}{pet_name:<26} {gpu_dmg:<10.1f} {cpu_dmg:<10.1f} {gpu_dps:<9.1f} {cpu_dps:<9.1f} {gpu_share:.1f}%/{cpu_share:.1f}%   {casts_str:<11} {pet_status}")

    print(f"{indent}{DIM}{'-'*98}{RESET}")


def run_presets_comparison(repo_root, port, event, breakdown=False, target_preset=None, markdown_path=None, browser_choice="chrome", iterations=1000, seed=42):
    if iterations < 1 or iterations > 100000:
        raise ValueError("Preset iterations must be between 1 and 100000.")
    presets = get_available_presets(repo_root)
    if not presets:
        print(f"{RED}No presets found from native C++ oracle.{RESET}")
        return None

    if target_preset:
        presets = [p for p in presets if p["id"] == target_preset or target_preset.lower() in p["name"].lower()]
        if not presets:
            print(f"{RED}Unknown preset '{target_preset}'. Use --presets to see all available presets.{RESET}")
            return None

    print(f"\n{BOLD}{CYAN}========================================================================================{RESET}")
    print(f"{BOLD}  WebGL2 GPU vs Authoritative C++ CPU Oracle: Standard Spec Presets Comparison{RESET}")
    print(f"{DIM}  {iterations} independent fights per preset; seeds {seed}–{(seed + iterations - 1) & 0xFFFFFFFF}{RESET}")
    print(f"{BOLD}{CYAN}========================================================================================{RESET}\n")

    # Run distinct CPU preset configs concurrently, with a small per-config seed pool.
    config_workers = min(len(presets), 4)
    with ThreadPoolExecutor(max_workers=config_workers) as pool:
        expected_results = list(pool.map(
            lambda p: run_cpp_oracle_preset(repo_root, p["id"], seed=seed, duration=180.0,
                                            iterations=iterations, workers=4),
            presets,
        ))

    cases = []
    for p, exp in zip(presets, expected_results):
        pid = p["id"]
        
        cfg = {
            "duration": 180, "iterations": iterations, "seed": seed,
            # The CPU's DP_RUIN_FIRE preset uses build_dp_fire_searing_rules,
            # which has Searing Pain as its primary filler. The fixture summary
            # classifies the preset as generic "fire", so preserve the actual
            # policy when constructing the WebGL config.
            "rotation": "searing" if pid == "dp_fire" else p.get("rotation", "shadow"),
            "spellPower": p.get("spellPower", 500),
            "intellect": 200, "spirit": 100, "stamina": 220,
            "hit": p.get("hit", 12),
            "crit": p.get("crit", 15),
            "mp5": 20,
            "distance": 30, "resistance": 0, "penetration": 0, "tapThreshold": 25,
            "book": False, "charges": False, "partialResists": True, "piercing": True,
            "corruption": p.get("corruption", True),
            "agony": p.get("agony", True),
            "immolate": p.get("immolate", False),
            "conflagrate": p.get("conflagrate", False),
            "shadowburn": p.get("shadowburn", False),
            "curseOfDoom": p.get("curseOfDoom", False),
            "snfChance": p.get("snfChance", 0.0),
            "snfBonus": p.get("snfBonus", 0.0),
            "dotCrit": p.get("dotCrit", 1.5),
            "fnbCrit": p.get("fnbCrit", 0.0),
            "instantCorruption": p.get("instantCorruption", True),
            "nightfall": p.get("nightfall", True),
            "nightfallChance": p.get("nightfallChance", 0.0),
            "isb": p.get("isb", True),
            "isbBonus": p.get("isbBonus", 0.0),
            "ruin": p.get("ruin", True),
            "ruinRank": p.get("ruinRank", 0),
            "improvedTap": p.get("improvedTap", True),
            "tapBonus": p.get("tapBonus", 0.0),
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
            # This Shadow and Flame preset's CPU policy includes the Searing
            # Pain Decimation trigger even though its main filler is Shadow Bolt.
            "decimationSearing": pid == "sf_shadow_decimate",
            "demonicBrand": p.get("demonicBrand", False),
            "demonicBrandRank": p.get("demonicBrandRank", 0),
            "brandMult": p.get("brandMult", 1.0),
            "petFireboltMult": p.get("petFireboltMult", p.get("petMult", 1.0)),
            "petMeleeMult": p.get("petMeleeMult", p.get("petMult", 1.0)),
            "petLashMult": p.get("petLashMult", p.get("petMult", 1.0)),
            # Missing talent metadata means no mana-transfer talent. Assuming
            # one rank overfeeds pets in older oracle fixture binaries.
            "demonicEnergies": p.get("demonicEnergies", 0.0),
            "demonicKnowledge": p.get("demonicKnowledge", 0)
        }
        cases.append({
            "name": f"{p['name']}",
            "config": cfg,
            "expected": exp
        })

    report = run_headless_simulation(port, event, custom_cases=cases, browser_choice=browser_choice)
    if not report:
        print(f"{RED}Failed to execute WebGL simulation.{RESET}")
        return None

    results = report.get("results", [])
    for r in results:
        g_dps = r.get('meanDps', 0.0)
        c_dps = r.get('expectedDps', 0.0)
        delta_dps = g_dps - c_dps
        pct_diff = (delta_dps / c_dps * 100.0) if c_dps > 0 else 0.0
        r['preset_pass'] = abs(pct_diff) <= 1.5

    passed_count = sum(1 for r in results if r.get("preset_pass", False))
    total_count = len(results)

    # Render Preset Comparison Table
    print(f"{BOLD}{'#':<3} {'Spec Preset Name':<42} {'Status':<12} {'WebGL DPS':<12} {'CPU DPS':<12} {'DPS Delta':<14} {'Pet DPS':<10}{RESET}")
    print(f"{DIM}{'-'*108}{RESET}")

    for r in results:
        is_pass = r.get('preset_pass', False)
        status_str = f"{GREEN}PASS{RESET}" if is_pass else f"{RED}FAIL{RESET}"
        g_dps = r.get('meanDps', 0.0)
        c_dps = r.get('expectedDps', 0.0)
        delta_dps = g_dps - c_dps
        pct_diff = (delta_dps / c_dps * 100.0) if c_dps > 0 else 0.0
        delta_str = f"{delta_dps:+.1f} ({pct_diff:+.1f}%)"
        pet_dps = f"{(r.get('petDamage', 0.0) / 180.0):.1f}"

        print(f"{r['index']:<3} {r['name'][:40]:<42} {status_str:<21} {g_dps:<12.1f} {c_dps:<12.1f} {delta_str:<14} {pet_dps:<10}")

        if breakdown or not is_pass:
            print_comprehensive_spell_breakdown(r, indent="    ")
            if r.get("failures"):
                for f in r["failures"][:2]:
                    print(f"    {RED}↳ {f}{RESET}")
            print()

    print(f"{DIM}{'-'*108}{RESET}\n")
    if markdown_path:
        lines = [
            "# WebGL2 vs Authoritative C++ CPU Oracle: Spec Presets Parity Report\n",
            f"**Generated:** {time.strftime('%Y-%m-%d %H:%M:%S UTC', time.gmtime())}  ",
            f"**Total Presets Tested:** {total_count}  ",
            f"**Passed Presets (<= 1.5% tolerance):** {passed_count} / {total_count}\n",
            "## Summary Table\n",
            "| # | Spec Preset Name | Status | WebGL DPS | Native CPU DPS | DPS Delta | Pet DPS | Diagnostics |",
            "|---|------------------|--------|-----------|----------------|-----------|---------|-------------|"
        ]
        for r in results:
            status_badge = "✅ PASS" if r["pass"] else "❌ FAIL"
            g_dps = r.get('meanDps', 0.0)
            c_dps = r.get('expectedDps', 0.0)
            delta_dps = g_dps - c_dps
            pct_diff = (delta_dps / c_dps * 100.0) if c_dps > 0 else 0.0
            pet_dps = (r.get('petDamage', 0.0) / 180.0)
            diag = "<br>".join(r.get("failures", [])) if r.get("failures") else "Within tolerance"
            lines.append(f"| {r['index']} | {r['name']} | {status_badge} | {g_dps:.1f} | {c_dps:.1f} | {delta_dps:+.1f} ({pct_diff:+.1f}%) | {pet_dps:.1f} | {diag} |")

        lines.append("\n## Detailed Per-Spell & Pet Breakdowns\n")
        for r in results:
            lines.append(f"### {r['index']}. {r['name']} ({'PASS' if r['pass'] else 'FAIL'})\n")
            lines.append("| Spell / Source | WebGL Dmg | CPU Dmg | WebGL DPS | CPU DPS | DPS Delta | Casts (G/C) | Status |")
            lines.append("|----------------|-----------|---------|-----------|---------|-----------|-------------|--------|")
            cpu_spells = r.get('cpuSpellBreakdown') or []
            gpu_dmg = r.get('damageBreakdown', [])
            gpu_casts = r.get('castsBreakdown', [])
            total_gpu_dmg = r.get('totalDmg') or 1.0
            core_map = {1: 0, 2: 1, 5: 2, 8: 3, 12: 4, 9: 5}
            for sp in cpu_spells:
                sp_id = sp.get('id', 0)
                sp_name = sp.get('name', 'Unknown')
                c_dmg = sp.get('damage', 0.0)
                c_dps = sp.get('dps', 0.0)
                c_casts = sp.get('casts', 0)
                g_dmg = 0.0
                g_dps = 0.0
                g_casts = 0
                if sp_id in (5, 6):
                    total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (5, 6))
                    g_total = gpu_dmg[2] if len(gpu_dmg) > 2 and gpu_dmg[2] is not None else 0.0
                    g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
                    g_dps = g_dmg / 180.0
                    g_casts = round(c_casts / max(1, sum(s.get('casts', 0) for s in cpu_spells if s.get('id') in (5, 6))) * (gpu_casts[2] if len(gpu_casts) > 2 and gpu_casts[2] is not None else 0))
                elif sp_id in (8, 11, 13):
                    total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (8, 11, 13))
                    g_total = gpu_dmg[3] if len(gpu_dmg) > 3 and gpu_dmg[3] is not None else 0.0
                    g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
                    g_dps = g_dmg / 180.0
                    g_casts = round(c_casts / max(1, sum(s.get('casts', 0) for s in cpu_spells if s.get('id') in (8, 11, 13))) * (gpu_casts[3] if len(gpu_casts) > 3 and gpu_casts[3] is not None else 0))
                elif sp_id in (1, 10):
                    total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (1, 10))
                    g_total = gpu_dmg[0] if len(gpu_dmg) > 0 and gpu_dmg[0] is not None else 0.0
                    g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
                    g_dps = g_dmg / 180.0
                    g_casts = round(c_casts / max(1, sum(s.get('casts', 0) for s in cpu_spells if s.get('id') in (1, 10))) * (gpu_casts[0] if len(gpu_casts) > 0 and gpu_casts[0] is not None else 0))
                elif sp_id in (2, 14, 15, 16, 17):
                    total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (2, 14, 15, 16, 17))
                    g_total = gpu_dmg[1] if len(gpu_dmg) > 1 and gpu_dmg[1] is not None else 0.0
                    g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
                    g_dps = g_dmg / 180.0
                    g_casts = round(c_casts / max(1, sum(s.get('casts', 0) for s in cpu_spells if s.get('id') in (2, 14, 15, 16, 17))) * (gpu_casts[1] if len(gpu_casts) > 1 and gpu_casts[1] is not None else 0))
                elif sp_id in core_map:
                    idx = core_map[sp_id]
                    g_dmg = gpu_dmg[idx] if idx < len(gpu_dmg) and gpu_dmg[idx] is not None else 0.0
                    g_dps = g_dmg / 180.0
                    g_casts = gpu_casts[idx] if idx < len(gpu_casts) and gpu_casts[idx] is not None else 0
                elif sp_id in (20, 21, 22):
                    total_c = sum(s.get('damage', 0.0) for s in cpu_spells if s.get('id') in (20, 21, 22))
                    g_total = r.get('petDamage', 0.0)
                    g_dmg = (c_dmg / total_c * g_total) if total_c > 0 else g_total
                    g_dps = g_dmg / 180.0
                delta_dps = g_dps - c_dps
                pct = (delta_dps / c_dps * 100.0) if c_dps > 0.01 else 0.0
                st = "MISSING (-100%)" if (g_dmg == 0.0 and c_dmg > 0.0) else ("PASS" if abs(pct) <= 1.5 else f"DIFF ({pct:+.1f}%)")
                lines.append(f"| {sp_name} | {g_dmg:.1f} | {c_dmg:.1f} | {g_dps:.1f} | {c_dps:.1f} | {delta_dps:+.1f} ({pct:+.1f}%) | {g_casts} / {c_casts} | {st} |")
            lines.append("")

        md_p = Path(markdown_path)
        md_p.parent.mkdir(parents=True, exist_ok=True)
        md_p.write_text("\n".join(lines) + "\n")
        print(f"{GREEN}✓ Spec presets parity markdown report written to {markdown_path}{RESET}")

    return report


def main():
    parser = argparse.ArgumentParser(
        description="High-Fidelity WebGL2 DES vs Native C++ Oracle Parity CLI Pipeline"
    )
    parser.add_argument("--browser", choices=["chrome", "firefox"], default="chrome", help="Browser engine to use")
    parser.add_argument("--presets", action="store_true", help="Run full standard spec presets comparison")
    parser.add_argument("--preset", type=str, default=None, help="Run single spec preset by ID or name (e.g. aff_dp_brand, ds_af)")
    parser.add_argument("--fixtures", action="store_true", help="Run 47-case synthetic reference fixtures suite")
    parser.add_argument("--breakdown", "-b", "--detailed", "-d", action="store_true", help="Print per-spell casts, hits, crits, misses, and damage breakdowns")
    parser.add_argument("--filter", type=str, default=None, help="Filter fixture cases matching substring pattern")
    parser.add_argument("--diff-trace", type=str, default=None, help="Display ordered event trace diff for a fixture case")
    parser.add_argument("--rebuild-oracle", action="store_true", help="Recompile native C++ oracle binary and regenerate fixtures")
    parser.add_argument("--json", action="store_true", help="Output raw JSON report for CI/tooling")
    parser.add_argument("--markdown", type=str, default=None, help="Path to write Markdown parity report")

    # Custom Sim flags
    parser.add_argument("--custom", action="store_true", help="Run a custom single-fight sim comparison")
    parser.add_argument("--sp", type=float, default=500, help="Custom spell power (default 500)")
    parser.add_argument("--hit", type=float, default=12, help="Custom spell hit percent (default 12)")
    parser.add_argument("--crit", type=float, default=15, help="Custom spell crit percent (default 15)")
    parser.add_argument("--duration", type=int, default=180, help="Custom fight duration in seconds (default 180)")
    parser.add_argument("--rotation", choices=["shadow", "fire", "searing", "bolt"], default="shadow", help="Rotation profile")
    parser.add_argument("--seed", type=int, default=42, help="RNG seed (default 42)")
    parser.add_argument("--iterations", type=int, default=1000, help="Independent fights per preset comparison (default 1000)")
    parser.add_argument("--distance", type=int, choices=[0, 30, 60, 120], default=30, help="Boss distance yards")

    # Sweep flags
    parser.add_argument("--sweep", choices=["sp", "hit", "crit", "duration"], help="Run parameter sweep parity")
    parser.add_argument("--sweep-min", type=float, default=0, help="Sweep start value")
    parser.add_argument("--sweep-max", type=float, default=500, help="Sweep end value")
    parser.add_argument("--sweep-step", type=float, default=100, help="Sweep step size")

    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    web_dir = repo_root / "threejs_webgl_des"

    if args.rebuild_oracle:
        if not rebuild_cpp_oracle(repo_root):
            sys.exit(1)

    port = find_free_port()
    event = threading.Event()
    server = start_server(web_dir, port, event)

    try:
        if args.presets or args.preset:
            report = run_presets_comparison(repo_root, port, event, breakdown=args.breakdown, target_preset=args.preset, markdown_path=args.markdown, browser_choice=args.browser, iterations=args.iterations, seed=args.seed)
            return

        if args.custom:
            custom_cfg = {
                "duration": args.duration, "iterations": 1, "seed": args.seed, "rotation": args.rotation,
                "spellPower": args.sp, "intellect": 200, "spirit": 100, "hit": args.hit, "crit": args.crit, "mp5": 20,
                "distance": args.distance, "resistance": 0, "penetration": 0, "tapThreshold": 25,
                "book": False, "charges": False, "partialResists": True, "piercing": True,
                "corruption": args.rotation != "bolt", "agony": args.rotation != "bolt", "immolate": args.rotation == "fire",
                "instantCorruption": True, "nightfall": True, "isb": True, "ruin": True, "improvedTap": True,
                "sacImp": False, "sacSucc": False, "masterDemo": 0, "petChoice": "none",
                "trinketSP": 0, "trinketDuration": 20, "trinketCD": 120,
                "shadowMultiplier": 1.0, "fireMultiplier": 1.0
            }
            exp = run_cpp_oracle_custom(repo_root, custom_cfg)
            custom_cases = [{
                "name": f"custom_{args.rotation}_sp{args.sp}_hit{args.hit}_crit{args.crit}",
                "config": custom_cfg,
                "expected": exp
            }]
            report = run_headless_simulation(port, event, custom_cases=custom_cases, browser_choice=args.browser)
            if report and report.get("results"):
                res = report["results"][0]
                print(f"\n{BOLD}{CYAN}=== Ad-Hoc Simulation Results ==={RESET}\n")
                print(f"WebGL DPS: {res.get('meanDps', 0.0):.2f} | CPU DPS: {res.get('expectedDps', 0.0):.2f}")
                print_comprehensive_spell_breakdown(res, indent="")
                sys.exit(0 if res.get("pass") else 1)
            else:
                print(f"{RED}Failed to execute custom simulation.{RESET}")
                sys.exit(1)

        # Default or --fixtures: Run standard fixture suite
        print(f"\n{BOLD}{CYAN}============================================================================{RESET}")
        print(f"{BOLD}  WebGL2 DES vs Native C++ Oracle Reference Fixtures Suite{RESET}")
        print(f"{BOLD}{CYAN}============================================================================{RESET}\n")

        report = run_headless_simulation(port, event, filter_pattern=args.filter or args.diff_trace, browser_choice=args.browser)
        if not report or "error" in report:
            err_msg = report.get("error", "Unknown execution error") if report else "No response received"
            print(f"{RED}✗ Simulation execution failed: {err_msg}{RESET}")
            sys.exit(1)

        results = report.get("results", [])
        passed_count = report.get("passed", 0)
        total_count = report.get("total", 0)

        print(f"{BOLD}{'#':<4} {'Fixture Name':<42} {'Status':<10} {'GPU Dmg':<14} {'CPU Dmg':<14} {'GPU Time':<10}{RESET}")
        print(f"{DIM}{'-'*96}{RESET}")

        for r in results:
            status_str = f"{GREEN}PASS{RESET}" if r["pass"] else f"{RED}FAIL{RESET}"
            gpu_dmg = f"{r.get('totalDmg', 0.0):.1f}" if "totalDmg" in r and r['totalDmg'] is not None else "-"
            cpu_dmg = f"{r.get('expectedTotalDmg', 0.0):.1f}" if "expectedTotalDmg" in r and r['expectedTotalDmg'] is not None else "-"
            gpu_time = f"{r.get('elapsedMs', 0.0):.1f}ms" if "elapsedMs" in r else "-"
            print(f"{r['index']:<4} {r['name'][:40]:<42} {status_str:<19} {gpu_dmg:<14} {cpu_dmg:<14} {gpu_time:<10}")

            if not r["pass"] and r.get("failures"):
                for f in r["failures"]:
                    print(f"     {RED}↳ {f}{RESET}")

            if args.breakdown:
                print_comprehensive_spell_breakdown(r, indent="     ")

        print(f"{DIM}{'-'*96}{RESET}\n")
        print(f"{GREEN if passed_count == total_count else RED}{BOLD}{passed_count} / {total_count} reference cases passed.{RESET}\n")

    finally:
        server.shutdown()


if __name__ == "__main__":
    main()
