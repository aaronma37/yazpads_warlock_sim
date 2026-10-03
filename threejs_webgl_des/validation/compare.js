import { CPU_IDS } from '../src/model.js';

export function nextRandom(state) {
  const mask=(1n<<64n)-1n;
  const word=(BigInt(state.rng3)<<32n)|BigInt(state.rng2);
  const x=(word*5n)&mask;
  const result=(((x<<7n)|(x>>57n))*9n)&mask;
  return [Number(result&0xffffffffn),Number(result>>32n)];
}
export function compare(result, fixture) {
  const got=result.states[0], expected=fixture.expected, failures=[];
  const ignored = new Set(['nextRandom', 'damageTrace', 'spellBreakdown', 'dps', 'impDamage', 'succubusDamage', 'demonicBrandDamage']);
  for (const [key, value] of Object.entries(expected)) {
    if (ignored.has(key)) continue;
    const float = /^(total|mana|spent|gained|petDamage|damage\d)$/.test(key);
    const tolerance = float ? Math.max(.02,Math.abs(value)*.00002) : 0;
    if (!Number.isFinite(got[key]) || Math.abs(got[key]-value)>tolerance) failures.push(`${key}: GPU ${got[key]}, CPU ${value}`);
  }
  const rng=nextRandom(got);
  if (rng.some((word,i)=>word!==expected.nextRandom[i])) failures.push(`Next RNG output differs: ${rng} / ${expected.nextRandom}`);
  // Positive damage events are unambiguous for this slice. Ignore truncated
  // suffixes, but require every recorded event to agree in order and content.
  const trace=result.trace.filter(e=>e.damage>0);
  if (!result.traceTruncated && trace.length!==expected.damageTrace.length) failures.push(`Damage trace length: ${trace.length} / ${expected.damageTrace.length}`);
  for (let i=0;i<trace.length;i++) {
    const a=trace[i], b=expected.damageTrace[i];
    if (!b || Math.abs(a.time-b[0])>1e-6 || CPU_IDS[a.spell]!==b[1] || Math.abs(a.damage-b[2])>Math.max(.01,b[2]*.00002) || (a.flags&1)!==b[3]) {
      failures.push(`First differing damage event ${i}: ${JSON.stringify(a)} / ${JSON.stringify(b)}`); break;
    }
  }
  return {name:fixture.name,pass:failures.length===0,failures};
}
