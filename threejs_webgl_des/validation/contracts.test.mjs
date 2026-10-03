import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { validate, packConfig, CONFIG } from '../src/model.js';

test('rejects unsupported mechanics and values instead of silently approximating',()=>{
 for(const input of [{haste:10},{pet:'imp'},{duration:1800.5},{seed:-1},{seed:2**32},{iterations:0},
  {spellPower:NaN},{duration:Infinity},{distance:31},{rotation:'unknown'},{nightfall:'false'}]){
  assert.throws(()=>validate(input));
 }
});
test('inputs match CPU raw-stat conventions and preserve integer clock words',()=>{
 const words=packConfig({intellect:0,spirit:0,duration:1800,distance:120});
 const floats=new Float32Array(words.buffer),keys=Object.keys(CONFIG);
 assert.equal(words[keys.indexOf('end')],1800000000);
 assert.equal(words[keys.indexOf('travel')],5000000);
 assert.equal(floats[keys.indexOf('maxMana')],3433);
 assert.ok(Math.abs(floats[keys.indexOf('tapGain')]-692.4)<.001);
});
test('checked-in fixtures were generated against the current CPU sources',()=>{
 const fixture=JSON.parse(readFileSync(new URL('./cpu-fixtures.json',import.meta.url)));
 for(const [path,hash] of Object.entries(fixture.sourceHashes)){
  const actual=createHash('sha256').update(readFileSync(new URL(`../../${path}`,import.meta.url))).digest('hex');
  assert.equal(actual,hash,`${path} changed; regenerate the reference fixtures.`);
 }
 assert.ok(fixture.cases.length>=36);
});
