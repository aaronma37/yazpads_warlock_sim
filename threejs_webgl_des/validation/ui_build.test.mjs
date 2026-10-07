import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
import { prepareUIBuildImport } from '../src/contracts/ui_build.js';
import { exportLogicalBuild, exportResolvedBuild } from '../src/contracts/build_io.js';
import { WARLOCK_IMPORT_BUFF_KEYS } from '../src/classes/warlock_import.js';
const source=readFileSync(new URL('../src/app.js',import.meta.url),'utf8');
function harness() {
 const calls=[];
 const nodes=new Map();
 const fields=new Map();
 const context={ prepareUIBuildImport, getEquipmentDatabase:()=>({slots:[],items:[]}), WARLOCK_IMPORT_BUFF_KEYS,
  controller:null, importedConfig:null, importedLogicalBuild:null, currentResult:null, setStatus:message=>calls.push(['status',message]),
  $:id=>{if(!nodes.has(id))nodes.set(id,{value:'original'});return nodes.get(id);},
  form:{elements:{namedItem:key=>{if(!fields.has(key))fields.set(key,{type:'checkbox',checked:true,value:'original'});return fields.get(key);}} } };
 for(const name of ['setRace','setPet','setDS','setRotation','setGearMode','setEquippedGear','applyTalentsObject','applySynthesizedAPL','updateCombatStatsSummary'])context[name]=arg=>calls.push([name,arg]);
 const fn=source.slice(source.indexOf('export async function applyImportedBuild'),source.indexOf('function setupBuildExportImportModal')).replace('export ','');
 vm.createContext(context);vm.runInContext(fn+'; this.apply = applyImportedBuild;',context);
 return {context,calls,nodes,fields};
}
test('actual app import rejects invalid builds before any setter, form, or status mutation',async()=>{
 for(const payload of [{race:'priest',stats:{}},{aplText:'Frostbolt'}, {stats:{intellect:-1}}]) {
  const h=harness();await assert.rejects(h.context.apply(payload));
  assert.deepEqual(h.calls,[]);assert.equal(h.nodes.size,0);assert.equal(h.fields.size,0);
  assert.equal(h.context.importedConfig,null);
 }
});
test('actual app applies staged editable builds and clears stale buff selections',async()=>{
 const saved=await exportLogicalBuild({race:'ORC',stats:{intellect:300},aplText:'Shadow Bolt',buffs:{arcaneIntellect:true}});
 const h=harness();await h.context.apply(JSON.parse(JSON.stringify(saved)));
 assert.ok(h.calls.some(([name,value])=>name==='setRace'&&value==='ORC'));
 assert.ok(h.calls.some(([name,value])=>name==='applySynthesizedAPL'&&value[0].id==='bolt'));
 assert.equal(h.fields.get('arcaneIntellect').checked,true);
 assert.equal(h.fields.get('blessingOfKings').checked,false);
 assert.equal(h.context.importedConfig.intellect,331);
 assert.equal(h.context.importedLogicalBuild.kind,'logical-build');
});
test('actual app accepts legacy and versioned resolved imports without touching editable fields',async()=>{
 const saved=await exportResolvedBuild({race:'GNOME',stats:{intellect:200},aplText:'Shadow Bolt'});
 for(const payload of [saved,{resolvedConfig:saved.resolvedConfig}]) {
  const h=harness();await h.context.apply(payload);
  assert.equal(h.context.importedConfig.intellect,210);
  assert.equal(h.calls.length,1);assert.equal(h.calls[0][0],'status');
  assert.equal(h.nodes.size,0);assert.equal(h.fields.size,0);
 }
});
test('policy imports retain exact simulation configs through resolved mode',async()=>{
 const saved=await exportLogicalBuild({stats:{}});
 const prepared=await prepareUIBuildImport(saved);
 assert.equal(prepared.kind,'resolved-config');
 assert.ok(prepared.config.aplRules.length);
});
