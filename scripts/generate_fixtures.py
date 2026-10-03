#!/usr/bin/env python3
import subprocess
import json
import hashlib
from pathlib import Path

DEFAULTS = {
    "duration": 180,
    "iterations": 1,
    "seed": 42,
    "rotation": "shadow",
    "spellPower": 500,
    "shadowPower": 0,
    "firePower": 0,
    "intellect": 200,
    "stamina": 220,
    "spirit": 100,
    "hit": 12,
    "crit": 15,
    "mp5": 20,
    "distance": 30,
    "resistance": 0,
    "penetration": 0,
    "tapThreshold": 25,
    "book": False,
    "charges": False,
    "partialResists": True,
    "piercing": True,
    "corruption": True,
    "agony": True,
    "immolate": False,
    "instantCorruption": True,
    "nightfall": True,
    "isb": True,
    "ruin": True,
    "improvedTap": True,
    "sacImp": False,
    "sacSucc": False,
    "masterDemo": 0,
    "petChoice": "none",
    "trinketSP": 0,
    "trinketDuration": 20,
    "trinketCD": 120,
    "shadowMultiplier": 1.0,
    "fireMultiplier": 1.0,
    "baneRank": 5,
}

definitions = [
    ('shadow baseline', {}),
    ('seed zero', {'seed': 0}),
    ('largest seed', {'seed': 4294967295}),
    ('bolt only', {'rotation': 'bolt'}),
    ('charged ISB', {'charges': True}),
    ('no ISB', {'isb': False}),
    ('no Nightfall', {'nightfall': False}),
    ('hardcast Corruption', {'instantCorruption': False}),
    ('book ranks', {'book': True}),
    ('positive resistance', {'resistance': 75}),
    ('partial disabled', {'resistance': 75, 'partialResists': False}),
    ('negative resistance', {'penetration': 100}),
    ('piercing disabled', {'penetration': 100, 'piercing': False}),
    ('no travel', {'distance': 0}),
    ('overlapping missiles', {'distance': 120}),
    ('fire', {'rotation': 'fire', 'corruption': False, 'agony': False, 'immolate': True}),
    ('fire with shadow dots', {'rotation': 'fire', 'immolate': True}),
    ('searing', {'rotation': 'searing', 'corruption': False, 'agony': False, 'immolate': True}),
    ('fire resists', {'rotation': 'fire', 'immolate': True, 'resistance': 50}),
    ('low mana', {'intellect': 0, 'spirit': 0, 'mp5': 0, 'tapThreshold': 0}),
    ('all taps', {'tapThreshold': 100}),
    ('no improved tap', {'improvedTap': False}),
    ('no ruin', {'ruin': False}),
    ('guaranteed crits', {'crit': 100}),
    ('hit cap', {'hit': 17}),
    ('zero spell power', {'spellPower': 0}),
    ('one second cutoff', {'duration': 1}),
    ('cast cutoff', {'duration': 3, 'rotation': 'bolt', 'distance': 0}),
    ('regen and end tie', {'duration': 5}),
    ('dot and end tie', {'duration': 24}),
    ('long fight', {'duration': 1800}),
    ('no dots', {'corruption': False, 'agony': False}),
    ('demonic sacrifice succubus', {'sacSucc': True}),
    ('demonic sacrifice imp fire', {'sacImp': True, 'rotation': 'fire', 'immolate': True}),
    ('master demonologist shadow', {'masterDemo': 5}),
    ('master demonologist fire', {'masterDemo': 5, 'rotation': 'fire', 'immolate': True}),
    ('shadow mastery 5/5', {'shadowMultiplier': 1.10}),
    ('fire emberstorm 5/5', {'rotation': 'fire', 'immolate': True, 'fireMultiplier': 1.10}),
    ('active trinket 175 SP', {'trinketSP': 175, 'trinketDuration': 20, 'trinketCD': 120}),
] + [(f'shadow seed {s}', {'seed': s}) for s in [1, 7, 1337, 9001, 43, 44, 45, 46]]

fields = [
    'seed', 'duration', 'rotation', 'spellPower', 'intellect', 'spirit', 'hit', 'crit', 'mp5', 'distance',
    'resistance', 'penetration', 'tapThreshold', 'book', 'charges', 'partialResists', 'piercing', 'corruption', 'agony',
    'immolate', 'instantCorruption', 'nightfall', 'isb', 'ruin', 'improvedTap',
    'sacImp', 'sacSucc', 'masterDemo', 'petChoice',
    'trinketSP', 'trinketDuration', 'trinketCD', 'shadowMultiplier', 'fireMultiplier'
]

def main():
    repo_root = Path(__file__).resolve().parent.parent
    bin_path = repo_root / "bin" / "warlock_cpu_fixture"
    if not bin_path.exists():
        raise RuntimeError(f"Executable not found at {bin_path}")

    cases = []
    lines = []
    for name, changes in definitions:
        cfg = {**DEFAULTS, **changes}
        cases.append({"name": name, "config": cfg})
        tokens = []
        for k in fields:
            val = cfg[k]
            if isinstance(val, bool):
                tokens.append(str(int(val)))
            else:
                tokens.append(str(val))
        lines.append(" ".join(tokens))

    input_str = "\n".join(lines) + "\n"
    res = subprocess.run([str(bin_path)], input=input_str, capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Fixture execution failed: {res.stderr}")

    answers = [json.loads(line) for line in res.stdout.strip().split("\n") if line.strip()]
    if len(answers) != len(cases):
        raise RuntimeError(f"Fixture count mismatch: got {len(answers)}, expected {len(cases)}")

    for i, c in enumerate(cases):
        c["expected"] = answers[i]

    sources = [
        'src/sim/warlock/warlock_sim.cpp',
        'src/sim/common/des_engine.hpp',
        'src/sim/common/stats.hpp',
        'src/sim/common/buffs.hpp',
        'src/sim/warlock/policy.hpp',
        'src/sim/warlock/talents.hpp'
    ]
    source_hashes = {}
    for s in sources:
        p = repo_root / s
        if p.exists():
            source_hashes[s] = hashlib.sha256(p.read_bytes()).hexdigest()

    result = {
        "schema": 1,
        "generatedAt": subprocess.check_output(["date", "-u", "+%Y-%m-%dT%H:%M:%SZ"]).decode().strip(),
        "sourceHashes": source_hashes,
        "cases": cases
    }

    out_file = repo_root / "threejs_webgl_des" / "validation" / "cpu-fixtures.json"
    out_file.write_text(json.dumps(result, indent=2))
    print(f"Successfully generated {len(cases)} C++ reference cases -> {out_file}")

if __name__ == "__main__":
    main()
