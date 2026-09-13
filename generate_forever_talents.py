import json, re

def to_snake(name):
    return re.sub(r'[^a-zA-Z0-9]+', '_', name).strip('_').lower()

with open('warlock_forever_talents.json') as f:
    data = json.load(f)

trees = data['trees']

out = """#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

namespace warlock {

struct TalentNodeDef {
    const char* id;
    const char* name;
    int row;
    int col;
    int max_points;
    const char* icon;
    const char* req;
    const char* desc[5];
};

"""

# For each tree: generate struct
for ti, tree in enumerate(trees):
    tname = tree['name']
    struct_name = f"{tname}Talents"
    talents = tree['talents']
    
    out += f"struct {struct_name} {{\n"
    for t in talents:
        var = to_snake(t['name'])
        out += f"    int {var} = 0; // max: {t['max']}, R{t['row']}C{t['col']}\n"
    
    out += "\n    int total_points() const {\n        return "
    out += " + \n               ".join([to_snake(t['name']) for t in talents])
    out += ";\n    }\n"
    
    # get by index
    out += "\n    int& get_points_by_index(size_t idx) {\n        switch (idx) {\n"
    for i, t in enumerate(talents):
        var = to_snake(t['name'])
        out += f"            case {i}: return {var};\n"
    out += f"            default: return {to_snake(talents[0]['name'])};\n"
    out += "        }\n    }\n"
    
    out += f"\n    int get_points_by_index(size_t idx) const {{\n        switch (idx) {{\n"
    for i, t in enumerate(talents):
        var = to_snake(t['name'])
        out += f"            case {i}: return {var};\n"
    out += f"            default: return 0;\n"
    out += "        }\n    }\n"
    
    out += "};\n\n"

def scale_text(text, from_r, to_r, t):
    if t["name"] == "Shadow and Flame":
        dmg = 2 * to_r
        pct = 20 * to_r
        return f"Hitting an enemy with Conflagrate increases all Shadow damage you deal by {dmg}% for 20 sec, and hitting an enemy with Shadowburn increases all Fire damage you deal by {dmg}% for 20 sec. In addition, Conflagrate has a {pct}% chance not to consume Immolate, and Shadowburn has a {pct}% chance to instantly refund a Soul Shard."
    if t["name"] == "Pandemic":
        bonus = 33 * to_r if to_r < 3 else 100
        return f"Increases the critical strike damage bonus of your Corruption, Bane of Agony, Bane of Doom, Drain Soul, Drain Life, Siphon Life, and Drain Hope spells by {bonus}%."
    if t["name"] == "Demonic Knowledge":
        pct = 33 * to_r if to_r < 3 else 100
        return f"Increases your spell damage and healing by up to {pct}% of your level while you have a summoned Demon pet active."
    if t["name"] in ("Fel Concentration", "Intensity"):
        pct = 70 if to_r == 3 else (23 * to_r)
        return text.replace("23%", f"{pct}%")
    if t["name"] == "Soul Siphon":
        rate = 50 if to_r == 3 else (17 * to_r)
        heal = 10 * to_r
        return f"Increases the rate at which your Drain Life and Drain Soul deal damage by {rate}%, but reduces your healing from Drain Life by {heal}%."
    if t["name"] == "Demonic Brand":
        th = 50 if to_r == 3 else (17 * to_r)
        dmin = 13 * to_r
        dmax = 14 * to_r
        return f"Your Searing Pain generates {th}% less threat and brands the target for 10 sec. Your pet's next 2 attacks against the target generate high threat and deal {dmin} to {dmax} Fire or Shadow damage based on the pet."

    fixed = set(t.get("fixed", []))
    idx_only = set(t["scaleIdx"]) if "scaleIdx" in t else None
    
    chance_matches = list(re.finditer(r"(\d+(?:\.\d+)?)%\s+chance", text, re.IGNORECASE))
    chance_overflow = any(float(m.group(1)) * to_r / from_r > 100 for m in chance_matches)
    has_chance = (not chance_overflow) and bool(re.search(r"\d+(?:\.\d+)?%\s+chance", text, re.IGNORECASE))
    
    token_i = [-1]
    pattern = re.compile(r"(\b(?:by an additional|by up to|by|an additional|a|an|up to|below|under|above|within|over|every|next|first|lasts|for|than|per|as if you were)\s+)?(\d+(?:\.\d+)?)(%)?(\s+chance)?", re.IGNORECASE)
    
    def repl(m):
        token_i[0] += 1
        i = token_i[0]
        pre = m.group(1) or ""
        num = m.group(2)
        pct = m.group(3) or ""
        chance = m.group(4) or ""
        tok = num + pct
        
        ok = False
        if idx_only is not None:
            ok = (i in idx_only)
        else:
            if tok in fixed:
                return m.group(0)
            if chance_overflow and chance:
                return m.group(0)
            p = pre.strip().lower()
            if p in ["up to", "below", "under", "above", "within", "over", "every", "next", "first", "lasts", "for", "than", "per", "as if you were"]:
                return m.group(0)
            if has_chance:
                ok = bool(chance)
            else:
                ok = bool(pct) or ("." in num) or p.startswith("by")
        
        if not ok:
            return m.group(0)
            
        v = float(num) * to_r / from_r
        if pct and v > 100:
            v = 100.0
        out_str = str(int(v)) if v.is_integer() else f"{v:.1f}"
        return pre + out_str + pct + chance

    return pattern.sub(repl, text)

# Node Definitions Arrays
for ti, tree in enumerate(trees):
    tname = tree['name']
    talents = tree['talents']
    var_name = f"FOREVER_{tname.upper()}_NODES"
    out += f"inline const std::array<TalentNodeDef, {len(talents)}> {var_name} = {{{{\n"
    for t in talents:
        var = to_snake(t['name'])
        name = t['name'].replace('"', '\\"')
        icon = t.get('icon', 'temp') + ".png"
        req = f'"{t["req"]}"' if 'req' in t else 'nullptr'
        
        # Build desc array (up to 5)
        descs = t.get('desc', {})
        d_lines = []
        for r in range(1, 6):
            val = ""
            if r <= t['max']:
                if isinstance(descs, list):
                    if r <= len(descs):
                        val = descs[r-1]
                elif isinstance(descs, dict):
                    if str(r) in descs:
                        val = descs[str(r)]
                    elif "1" in descs:
                        val = scale_text(descs["1"], 1, r, t)
            if val:
                val = val.replace('"', '\\"').replace('\n', ' ')
                d_lines.append(f'"{val}"')
            else:
                d_lines.append('nullptr')
        
        d_str = ", ".join(d_lines)
        out += f'    {{"{var}", "{name}", {t["row"]}, {t["col"]}, {t["max"]}, "{icon}", {req}, {{{d_str}}}}},\n'
    out += "}};\n\n"

out += """struct Talents {
    AfflictionTalents aff;
    DemonologyTalents demo;
    DestructionTalents destro;

    int total_points() const {
        return aff.total_points() + demo.total_points() + destro.total_points();
    }

    bool is_valid() const {
        return total_points() <= 51;
    }

    // =========================================================================
    // WoW Classic Forever Presets
    // =========================================================================

    // 1. Forever Shadow Destro (Sac Imp + Shadow & Flame Conflag Buff) (0/21/30)
    static Talents create_forever_shadow_destro() {
        Talents t;
        // Demonology: 21 points
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.improved_imp = 3;
        t.demo.unholy_power = 5;
        t.demo.master_summoner = 2;
        t.demo.demonic_aegis = 2;
        t.demo.demonic_sacrifice = 1; // Sac Imp -> +15% Shadow!

        // Destruction: 30 points
        t.destro.improved_shadow_bolt = 5; // 20% Shadow vuln for 12s on crit
        t.destro.bane = 5;                 // -0.5s SB cast time
        t.destro.cataclysm = 3;            // -9% mana cost
        t.destro.ruin = 5;                 // +100% crit damage bonus (2.0x total)
        t.destro.shadowburn = 1;           // instant shadow finisher
        t.destro.agonizing_flames = 3;     // +9% all Destruction spell damage!
        t.destro.conflagrate = 1;          // Conflagrate on cooldown
        t.destro.destructive_reach = 2;    // +20% range
        t.destro.shadow_and_flame = 5;     // Conflag gives +10% Shadow for 20s & 100% chance not to consume Immolate!
        return t;
    }

    // 2. Forever Fire Destro (Incinerate + Conflagrate + Aftermath) (0/11/40)
    static Talents create_forever_fire_destro() {
        Talents t;
        // Demonology: 11 points (Sac Succubus -> +15% Fire!)
        t.demo.demonic_embrace = 5;
        t.demo.fel_vitality = 3;
        t.demo.improved_sayaad = 2;
        t.demo.demonic_sacrifice = 1; // Sac Succubus -> +15% Fire!

        // Destruction: 40 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;                 // -0.5s Incinerate (2.0s cast!)
        t.destro.aftermath = 5;            // Immolate initial direct damage +50%!
        t.destro.cataclysm = 3;
        t.destro.ruin = 5;                 // 2.0x crit bonus
        t.destro.shadowburn = 1;           // triggers +10% Fire buff from Shadow & Flame!
        t.destro.agonizing_flames = 3;     // +9% Destruction damage
        t.destro.conflagrate = 1;
        t.destro.fire_and_brimstone = 3;   // +24% Conflagrate crit chance!
        t.destro.shadow_and_flame = 5;     // Conflag never consumes Immolate; Shadowburn buffs Fire by 10%
        t.destro.bane_of_havoc = 1;        // Prerequisite for Incinerate
        t.destro.destructive_reach = 2;
        t.destro.incinerate = 1;           // Fire filler spell (2.0s cast, +25% dmg with Immolate)
        return t;
    }

    // 3. Forever Demonic Pact (Demonic Sacrifice + Active Demon Pet!) (0/41/10)
    static Talents create_forever_demonic_pact() {
        Talents t;
        // Demonology: 41 points (Capstone: Demonic Pact!)
        t.demo.demonic_embrace = 5;
        t.demo.improved_imp = 3;
        t.demo.unholy_power = 5;
        t.demo.fel_vitality = 3;
        t.demo.demonic_aegis = 2;
        t.demo.improved_sayaad = 3;
        t.demo.master_summoner = 2;
        t.demo.demonic_sacrifice = 1;      // Sac Imp for +15% Shadow!
        t.demo.decimation = 2;             // Soul Fire execute below 35% HP
        t.demo.soul_link = 1;              // +3% all damage
        t.demo.demonic_knowledge = 3;      // +60 Spell Power while pet is out!
        t.demo.master_demonologist = 5;    // +10% Shadow damage from Succubus!
        t.demo.fel_domination = 1;
        t.demo.demonic_brand = 3;
        t.demo.demonic_pact = 1;           // KEEP Demonic Sacrifice WHILE SUMMONING SUCCUBUS!

        // Destruction: 10 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;
        return t;
    }

    // 4. Forever Deep Affliction (Drain Hope + Pandemic + Malevolence) (41/0/10)
    static Talents create_forever_deep_affliction() {
        Talents t;
        // Affliction: 41 points
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;             // +5% spell hit, -20% threat
        t.aff.improved_corruption = 5;     // Instant, +10% damage
        t.aff.malediction = 5;             // +5% periodic damage
        t.aff.improved_bane_of_agony = 2;  // +10% Agony damage
        t.aff.pandemic = 3;                // +100% DoT crit damage bonus!
        t.aff.malevolence = 5;             // +5% Shadow spell crit
        t.aff.nightfall = 2;               // 4% Shadow Trance on Corruption ticks
        t.aff.siphon_life = 1;
        t.aff.soul_siphon = 3;             // +50% faster drain ticks
        t.aff.shadow_mastery = 5;          // +5% Shadow damage
        t.aff.drain_hope = 1;              // +10% Shadow DoT amplification!
        t.aff.amplify_curse = 1;
        t.aff.curse_of_exhaustion = 1;

        // Destruction: 10 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;
        return t;
    }

    // 5a. Forever SM/Ruin Full Shadow Mastery (32/0/19 - 5/5 SM)
    static Talents create_forever_sm_ruin_pure() {
        Talents t;
        // Affliction: 32 points
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;
        t.aff.improved_corruption = 5;
        t.aff.malediction = 5;
        t.aff.pandemic = 3;
        t.aff.malevolence = 5;
        t.aff.nightfall = 2;
        t.aff.shadow_mastery = 5; // Full 5/5 Shadow Mastery

        // Destruction: 19 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;
        t.destro.cataclysm = 3;
        t.destro.ruin = 5;
        t.destro.shadowburn = 1;
        return t;
    }

    // 5b. Forever SM/Ruin Hybrid (30/0/21 - 3/5 SM, 2/3 Agonizing Flames)
    static Talents create_forever_sm_ruin() {
        Talents t;
        // Affliction: 30 points
        t.aff.improved_life_tap = 2;
        t.aff.suppression = 5;
        t.aff.improved_corruption = 5;
        t.aff.malediction = 5;
        t.aff.pandemic = 3;
        t.aff.malevolence = 5;
        t.aff.nightfall = 2;
        t.aff.shadow_mastery = 3;

        // Destruction: 21 points
        t.destro.improved_shadow_bolt = 5;
        t.destro.bane = 5;
        t.destro.cataclysm = 3;
        t.destro.ruin = 5;
        t.destro.shadowburn = 1;
        t.destro.agonizing_flames = 2;
        return t;
    }

    // Classic compatibility aliases
    static Talents create_ds_ruin() { return create_forever_shadow_destro(); }
    static Talents create_sm_ruin() { return create_forever_sm_ruin(); }
    static Talents create_fire_destro() { return create_forever_fire_destro(); }
    static Talents create_md_ruin() { return create_forever_demonic_pact(); }
};

} // namespace warlock
"""

with open("src/sim/talents.hpp", "w") as f:
    f.write(out)

print("Generated src/sim/talents.hpp successfully!")
