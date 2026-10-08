import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, existsSync } from 'node:fs';
import { initPriestPreview, renderPriestTalents } from '../src/priest_preview.js';
const data = JSON.parse(readFileSync(new URL('../data/priest_preview.json', import.meta.url)));

test('Priest preview presets have 51 bounded ranks and all 53 icons exist', () => {
  const nodes = Object.values(data.trees).flat();
  assert.equal(nodes.length, 53);
  for (const node of nodes) assert.ok(existsSync(new URL(`../assets/icons/${node.icon}`, import.meta.url)), node.icon);
  for (const preset of data.presets) {
    assert.equal(Object.values(preset.ranks).reduce((a, b) => a + b, 0), 51);
    for (const [key, rank] of Object.entries(preset.ranks)) {
      const node = nodes.find(n => n.key === key);
      assert.ok(node, key); assert.ok(rank <= node.ranks.length, key);
    }
    const html = renderPriestTalents(data, preset);
    assert.equal((html.match(/class="talent-node /g) || []).length, 53);
    assert.ok(!html.includes('undefined'));
  }
});

function harness(options = {}) {
  const elements = new Map();
  const element = id => {
    const classes = new Set();
    const defaults={spellPower:500,shadowPower:0,holyPower:0,hit:12,crit:15,intellect:200,stamina:220,spirit:100,mp5:20,duration:180,iterations:4096,targetLevel:63,resistance:0};
    const name=id.match(/name="priest-preview-([^"]+)"/)?.[1];
    const el = { id, value:name?defaults[name]:undefined, checked:true, children: [], style: { display: '' }, textContent: id, attributes: {}, listeners: {},
      classList: { add: k => classes.add(k), remove: k => classes.delete(k), contains: k => classes.has(k) },
      setAttribute(k, v) { this.attributes[k] = v; }, addEventListener(k, v) { this.listeners[k] = v; },
      closest() { return null; },
      querySelector(selector) { return elements.get(`${this.id}-${selector}`) || element(`${this.id}-${selector}`); },
      querySelectorAll() { return []; },
      before(child) { child.parent=this.parent; },
      appendChild(child) { this.children.push(child); child.parent = this; },
      remove() { if(this.parent)this.parent.children.splice(this.parent.children.indexOf(this), 1); }
    }; elements.set(id, el); return el;
  };
  for (const id of ['btn-class-priest', 'btn-class-warlock', 'top-sim-status', '.brand-title', '.brand-sub', 'priest-preview-trees', 'priest-preview-preset', 'priest-preview-talents-summary-badge']) element(id);
  const tabs = ['current-build','compare-specs','constrained-search','apl-synthesis'].map(id => ({ btn: `btn-${id}`, pane: `tab-${id}` }));
  for (const tab of tabs) { element(tab.btn); element(tab.pane).appendChild(element(`original-${tab.pane}`)); }
  elements.get(tabs[1].btn).classList.add('active');
  const source = element('[data-shared-panel="current-configuration"]');
  const copies = [];
  let busy = false, switched;
  const doc = { title: 'Warlock title', getElementById: id => elements.get(id), querySelector: id => elements.get(id), createElement: () => element(`view-${elements.size}`) };
  initPriestPreview({ createPanel: (source, options) => { copies.push({ source, options }); return element(`shared-${elements.size}`); }, document: doc, tabs, switchTab: id => { switched = id; }, canSwitch: () => !busy, loadData: async () => data, ...options });
  return { elements, tabs, doc, source, copies, click: id => elements.get(id).listeners.click(), busy: value => { busy = value; }, switched: () => switched };
}

test('Class preview blocks switching during runs and restores original form nodes and tab', async () => {
  const h = harness();
  h.busy(true); await h.click('btn-class-priest'); assert.equal(h.doc.title, 'Warlock title');
  h.busy(false);
  let cachedBuild;
  for (let attempt = 0; attempt < 2; attempt++) {
    await h.click('btn-class-priest');
    assert.equal(h.doc.title, 'Priest Shadow Simulator');
    const build = h.elements.get('tab-current-build').children[1].children[0];
    if (cachedBuild) assert.equal(build, cachedBuild, 'Reuse Priest controls to retain edited values');
    cachedBuild = build;
    for (const tab of h.tabs) {
      const pane = h.elements.get(tab.pane);
      assert.equal(pane.children.length, 2); assert.equal(pane.children[0].style.display, 'none');
    }
    assert.equal(h.elements.get('btn-class-priest').attributes['aria-pressed'], 'true');
    await h.click('btn-class-warlock');
    assert.equal(h.doc.title, 'Warlock title'); assert.equal(h.switched(), 'btn-compare-specs');
    for (const tab of h.tabs) {
      const pane = h.elements.get(tab.pane);
      assert.equal(pane.children.length, 1); assert.equal(pane.children[0], h.elements.get(`original-${tab.pane}`)); assert.equal(pane.children[0].style.display, '');
    }
  }
});

test('Shared panel copies reset values, namespace controls, apply school override and gate unsupported fields', async () => {
  const { createSharedConfigurationPanel } = await import('../src/shared_configuration.js');
  const label = { textContent: 'Fire Power', attributes: {}, setAttribute(k,v) { this.attributes[k] = v; }, querySelector() { return null; } };
  const fire = { id: 'in-firePower', name: 'firePower', tagName: 'INPUT', type: 'number', value: '999', defaultValue: '0', closest() { return { querySelector: () => label }; } };
  const haste = { id: 'in-haste', name: 'haste', tagName: 'INPUT', type: 'number', value: '25', defaultValue: '0', closest() { return null; } };
  const run = { id: 'run', type: 'submit', disabled: false };
  const clone = { id: 'stats', querySelectorAll(selector) {
    if (selector === '[id]') return [fire,haste,run];
    if (selector === 'input, select, textarea') return [fire,haste];
    if (selector === 'button') return [run];
    return [];
  } };
  const original = { value: '999', cloneNode: deep => { assert.equal(deep,true); return clone; } };
  const panel = createSharedConfigurationPanel(original, { prefix: 'priest', rename: { firePower: { name: 'holyPower', label: 'Holy Power' } }, unsupported: ['haste'] });
  assert.equal(panel, clone); assert.equal(original.value,'999');
  assert.equal(fire.name,'priest-holyPower'); assert.equal(fire.id,'priest-in-firePower'); assert.equal(fire.value,'0');
  assert.equal(label.textContent,'Holy Power'); assert.equal(label.attributes.for,fire.id);
  assert.equal(haste.disabled,true); assert.equal(run.disabled,true); assert.equal(run.type,'button');
});

// This contract prevents a second hand-written configuration layout from drifting.
test('Priest configuration reuses the entire shared grid once across switches', async () => {
  const h = harness();
  await h.click('btn-class-priest');
  await h.click('btn-class-warlock');
  await h.click('btn-class-priest');
  assert.equal(h.copies.length, 2);
  assert.equal(h.copies[1].options.prefix, 'priest-search');
  assert.equal(h.copies[0].source, h.source);
  assert.deepEqual(h.copies[0].options.rename.firePower, { name: 'holyPower', label: 'Holy Power' });
});

test('Shared Priest Run button dispatches core builds, renders results and gates unfinished presets',async()=> {
  let dispatched=false;
  const h=harness({loadSimulation:async()=>({id:'priest',runSimulation:async(config,options)=>{
    dispatched=true;assert.equal(config.weavingChance,1);assert.equal(options.detailedResults,true);
    return {config,summary:{mean:100,sd:0,samples:2,total:18000,spells:[]},states:[{total:18000},{total:18000}],timing:{elapsedMs:10}};
  }})});
  await h.click('btn-class-priest');
  const build=h.elements.get('tab-current-build').children[1].children[0];
  const run=build.querySelector('#priest-preview-run');
  assert.equal(run.disabled,false);
  await run.listeners.click();
  // The DOM event handler starts an async run without awaiting the event return.
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.equal(dispatched,true);assert.equal(build.querySelector('#priest-preview-mean').textContent,'100.0');
  h.elements.get('priest-preview-preset').onchange({target:{value:'shadow_standard'}});
  assert.equal(run.disabled,false);
  assert.equal(build.querySelector('#priest-preview-mean').textContent,'—');
});

test('Priest constrained search uses namespaced controls and applies a winning allocation to the runnable build',async()=>{
  let batches=0,lastConfig;
  const h=harness({loadSimulation:async()=>({id:'priest',runMultiSimulation:async(configs,options)=>{
    batches++;
    for(const config of configs){assert.equal(config.targetDeaths,0);assert.equal(config.duration,180);assert.equal(config.plagueEnabled,1);}
    return {results:configs.map(config=>({config,summary:{mean:config.spellPower,sd:1},states:Array.from({length:options.iterations},()=>({done:1,total:config.spellPower*config.duration}))}))};
  },runSimulation:async(config)=>{lastConfig=config;return {config,summary:{mean:1,sd:0,samples:1,total:config.duration,spells:[]},states:[{total:config.duration}],timing:{elapsedMs:10}};}})});
  await h.click('btn-class-priest');
  const search=h.elements.get('tab-constrained-search').children[1].children[0];
  const get=id=>search.querySelector(`#priest-search-${id}`);
  for(const [id,value] of Object.entries({'ga-generations':1,'ga-pop-size':4,'ga-screening-sims':1,'ga-final-sims':2,'ga-mutation-rate':.45,'ga-locked-race':'UNDEAD','ga-locked-rotation':'mixed'}))get(id).value=String(value);
  const reqIndex=data.trees.Shadow.findIndex(node=>node.key==='shadowform')+data.trees.Discipline.length+data.trees.Holy.length;
  get('ga-req-talent-1').value=String(reqIndex);
  const result=await get('btn-run-ga').listeners.click();
  assert.equal(get('ga-apl-mode').value,'coevolve');
  assert.ok(result.bestCandidate.apl.length>0);
  assert.equal(batches,3);assert.ok(result.bestCandidate);assert.equal(result.bestCandidate.race,'UNDEAD');assert.equal(result.bestCandidate.rotation,'mixed');
  assert.match(get('ga-leaderboard-body').innerHTML,/mixed|Mixed/);
  assert.match(get('ga-selected-spec-details-panel').innerHTML,/Discipline/);
  assert.match(get('ga-selected-spec-details-panel').innerHTML,/Shadowform 1\/1/);
  assert.match(get('ga-chart-wrapper').innerHTML,/<svg/);
  const apply={getAttribute:()=> '0'};
  search.listeners.click({target:{closest:selector=>selector==='[data-priest-apply]'?apply:null}});
  assert.equal(h.switched(),'btn-current-build');
  const build=h.elements.get('tab-current-build').children[1].children[0];
  assert.equal(h.elements.get('priest-preview-preset').value,'priest_search_result');
  await build.querySelector('#priest-preview-run').listeners.click();await new Promise(resolve=>setTimeout(resolve,0));
  assert.equal(lastConfig.aplEnabled,1);
  assert.equal(lastConfig.rotation,'mixed');assert.equal(lastConfig.plagueEnabled,1);assert.equal(lastConfig.costMultiplier,.5);
  assert.equal(h.elements.get('btn-class-warlock').disabled,false);
  await h.click('btn-class-warlock');await h.click('btn-class-priest');
  assert.equal(h.elements.get('tab-constrained-search').children[1].children[0],search);
  assert.match(get('ga-leaderboard-body').innerHTML,/Mixed/);
});

test('Priest search locks current inputs and class switching, rejects duplicate starts, and Stop works during loading',async()=>{
  let release,loads=0,calls=0;
  const pending=new Promise(resolve=>{release=resolve;});
  const h=harness({loadSimulation:async()=>{loads++;await pending;return {id:'priest',runMultiSimulation:async()=>{calls++;}};}});
  await h.click('btn-class-priest');
  const search=h.elements.get('tab-constrained-search').children[1].children[0],get=id=>search.querySelector(`#priest-search-${id}`);
  for(const [id,value] of Object.entries({'ga-generations':1,'ga-pop-size':4,'ga-screening-sims':1,'ga-final-sims':2,'ga-mutation-rate':.45}))get(id).value=String(value);
  const run=get('btn-run-ga').listeners.click();
  await Promise.resolve();assert.equal(loads,1);assert.equal(h.elements.get('btn-class-warlock').disabled,true);
  await get('btn-run-ga').listeners.click();assert.equal(loads,1);
  await h.click('btn-class-warlock');assert.equal(h.doc.title,'Priest Shadow Simulator');
  get('btn-stop-ga').listeners.click();release();const result=await run;
  assert.equal(result.stopped,true);assert.equal(calls,0);assert.equal(h.elements.get('btn-class-warlock').disabled,false);
  assert.match(h.elements.get('top-sim-status').textContent,/stopped/);
});


test('Priest displayed policies share execution priority and explain conditions', async () => {
  const { POLICY_RULES, formatPriestPolicy } = await import('../src/priest/policy.js');
  for (const rotation of Object.keys(POLICY_RULES)) {
    const text = formatPriestPolicy({rotation,race:'UNDEAD'});
    let cursor=-1;
    for(const rule of POLICY_RULES[rotation]) {
      if(rule.spell===11||rule.spell===12)continue;
      const next=text.indexOf(rule.text,cursor+1);
      assert.ok(next>cursor, rotation+' '+rule.text); cursor=next;
    }
  }
  const holy=formatPriestPolicy({rotation:'holy'});
  assert.ok(holy.indexOf('holy_nova')<holy.indexOf('holy_fire'));
  const shadow=formatPriestPolicy();
  assert.match(shadow,/clip after tick 2/);
  assert.match(shadow,/affordable within 6s/);
  assert.match(shadow,/execute crit bonus at health <= 20%/);
  assert.ok(shadow.includes('devouring_plague'));
});


test('Priest search stays available when current-build talent resolution fails',async()=>{
  const invalidPreset={...data.presets[0],ranks:{...data.presets[0].ranks,shadowform:99}};
  const h=harness({loadData:async()=>({...data,presets:[invalidPreset]})});
  await h.click('btn-class-priest');
  const build=h.elements.get('tab-current-build').children[1].children[0];
  assert.equal(build.querySelector('#priest-preview-run').disabled,true);
  const search=h.elements.get('tab-constrained-search').children[1].children[0];
  assert.equal(search.querySelector('#priest-search-btn-run-ga').disabled,false);
});

test('Priest search explains disabled Run when WebGL2 is unavailable',async()=>{
  const h=harness({canSimulate:()=>false});await h.click('btn-class-priest');
  const search=h.elements.get('tab-constrained-search').children[1].children[0];
  const run=search.querySelector('#priest-search-btn-run-ga');
  assert.equal(run.disabled,true);assert.match(run.title,/WebGL2/);
});

test('Priest race selector exposes all six races and updates racial tooltips',async()=>{
 const h=harness();await h.click('btn-class-priest');
 const selector=[...h.elements.values()].find(el=>el.attributes['aria-label']==='Priest race');
 assert.ok(selector);
 for(const [race,name] of [['HUMAN','Human'],['UNDEAD','Undead'],['NIGHT_ELF','Night Elf'],['DWARF','Dwarf'],['GNOME','Gnome'],['TROLL','Troll']]){
  assert.ok(selector.innerHTML.includes(`value="${race}"`));selector.value=race;selector.listeners.change();
  const build=h.elements.get('tab-current-build').children[1].children[0];
  assert.equal(build.querySelector('#priest-preview-slot-race-icon').alt,name);
  const badges=build.querySelector('#priest-preview-racials-list-container').innerHTML;
  assert.ok(badges.includes('wowhead.com/forever/spell='));assert.ok(badges.includes('title='));
 }
});
