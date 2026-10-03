import { spawnSync } from 'node:child_process';
import { writeFileSync, readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { DEFAULTS, validate } from '../src/model.js';

const executable = process.argv[2];
if (!executable) throw new Error('Usage: node tools/generate-fixtures.mjs /path/to/cpu_fixture');
const definitions = [
  ['shadow baseline', {}], ['seed zero', {seed:0}], ['largest seed', {seed:4294967295}],
  ['bolt only', {rotation:'bolt'}], ['charged ISB', {charges:true}],
  ['no ISB', {isb:false}], ['no Nightfall', {nightfall:false}],
  ['hardcast Corruption', {instantCorruption:false}], ['book ranks', {book:true}],
  ['positive resistance', {resistance:75}], ['partial disabled', {resistance:75, partialResists:false}],
  ['negative resistance', {penetration:100}], ['piercing disabled', {penetration:100,piercing:false}],
  ['no travel', {distance:0}], ['overlapping missiles', {distance:120}],
  ['fire', {rotation:'fire',corruption:false,agony:false,immolate:true}],
  ['fire with shadow dots', {rotation:'fire',immolate:true}],
  ['searing', {rotation:'searing',corruption:false,agony:false,immolate:true}],
  ['fire resists', {rotation:'fire',immolate:true,resistance:50}],
  ['low mana', {intellect:0,spirit:0,mp5:0,tapThreshold:0}],
  ['all taps', {tapThreshold:100}], ['no improved tap', {improvedTap:false}],
  ['no ruin', {ruin:false}], ['guaranteed crits', {crit:100}], ['hit cap', {hit:17}],
  ['zero spell power', {spellPower:0}], ['one second cutoff', {duration:1}],
  ['cast cutoff', {duration:3,rotation:'bolt',distance:0}],
  ['regen and end tie', {duration:5}], ['dot and end tie', {duration:24}],
  ['long fight', {duration:1800}], ['no dots', {corruption:false,agony:false}],
  ['demonic sacrifice succubus', {sacSucc:true}],
  ['demonic sacrifice imp fire', {sacImp:true,rotation:'fire',immolate:true}],
  ['master demonologist shadow', {masterDemo:5}],
  ['master demonologist fire', {masterDemo:5,rotation:'fire',immolate:true}],
  ['shadow mastery 5/5', {shadowMultiplier:1.10}],
  ['fire emberstorm 5/5', {rotation:'fire',immolate:true,fireMultiplier:1.10}],
  ['active trinket 175 SP', {trinketSP:175,trinketDuration:20,trinketCD:120}],
  ...[1,7,1337,9001,43,44,45,46].map(seed=>[`shadow seed ${seed}`,{seed}]),
];
const fields = ['seed','duration','rotation','spellPower','intellect','spirit','hit','crit','mp5','distance',
  'resistance','penetration','tapThreshold','book','charges','partialResists','piercing','corruption','agony',
  'immolate','instantCorruption','nightfall','isb','ruin','improvedTap',
  'sacImp','sacSucc','masterDemo','petChoice',
  'trinketSP','trinketDuration','trinketCD','shadowMultiplier','fireMultiplier'];
const cases = definitions.map(([name, changes])=>({name,config:validate({...DEFAULTS,...changes,iterations:1})}));
const input = cases.map(({config})=>fields.map(k=>typeof config[k]==='boolean'?+config[k]:config[k]).join(' ')).join('\n')+'\n';
const run = spawnSync(executable, [], {input,encoding:'utf8',maxBuffer:32*1024*1024});
if (run.status !== 0) throw new Error(run.stderr || String(run.error));
const answers = run.stdout.trim().split('\n').map(JSON.parse);
if (answers.length !== cases.length) throw new Error('CPU fixture count mismatch.');
cases.forEach((c,i)=>c.expected=answers[i]);
const root = new URL('../../', import.meta.url);
const sources = ['src/sim/warlock/warlock_sim.cpp','src/sim/common/des_engine.hpp','src/sim/common/stats.hpp','src/sim/common/buffs.hpp','src/sim/warlock/policy.hpp','src/sim/warlock/talents.hpp'];
const sourceHashes = Object.fromEntries(sources.map(p=>[p,createHash('sha256').update(readFileSync(new URL(p,root))).digest('hex')]));
const result = {schema:1,generatedAt:new Date().toISOString(),sourceHashes,cases};
const path = new URL('../validation/cpu-fixtures.json',import.meta.url);
writeFileSync(path,JSON.stringify(result));
console.log(`Wrote ${cases.length} C++ reference cases to ${fileURLToPath(path)}`);
