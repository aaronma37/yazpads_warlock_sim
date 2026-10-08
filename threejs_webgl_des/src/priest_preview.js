import { PRIEST_RACES, PRIEST_RACIALS, RACE_NAMES, RACE_ICONS } from './priest/racials.js';
import { resolvePriestUIBuild } from './priest/ui_config.js';
import { createPriestRunController } from './priest/run_controller.js';
import { formatPriestPolicy } from './priest/policy.js';
import { createPriestSearchView } from './priest/search_view.js';
import { createSharedConfigurationPanel } from './shared_configuration.js';
// Shared Priest configuration and explicitly scoped class-owned runtime dispatch.
const escapeHTML = value => String(value).replace(/[&<>"']/g, char => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[char]));
const panel = (title, body) => `<section class="wow-panel priest-preview-panel"><h2 class="wow-panel-header">${title}</h2>${body}</section>`;

export function renderPriestTalents(data, preset) {
  return Object.entries(data.trees).map(([name, nodes]) => {
    const points = nodes.reduce((total, node) => total + (preset.ranks[node.key] || 0), 0);
    return `<section class="talent-tree-panel"><div class="tree-header"><span class="tree-title">${escapeHTML(name)}</span><div class="tree-header-right"><span class="tree-points-display">${points} pts</span><button type="button" class="tree-reset-btn" title="Reset ${escapeHTML(name)} Talents" disabled>✕</button></div></div><div class="talent-grid">${nodes.map(node => {
      const rank = preset.ranks[node.key] || 0;
      const description = node.descriptions[String(rank || 1)] || '';
      return `<div class="talent-node ${rank ? 'active' : 'disabled'} ${rank === node.ranks.length ? 'max' : ''}" style="grid-row:${node.row + 1};grid-column:${node.col + 1}" tabindex="0" aria-label="${escapeHTML(node.name)} ${rank}/${node.ranks.length}" title="${escapeHTML(node.name)} (${rank}/${node.ranks.length})&#10;${escapeHTML(description)}"><div class="talent-icon-frame"><img src="./assets/icons/${escapeHTML(node.icon)}" alt="${escapeHTML(node.name)}" loading="lazy"></div><span class="talent-rank-badge">${rank}/${node.ranks.length}</span></div>`;
    }).join('')}</div></section>`;
  }).join('');
}

export function initPriestPreview({ tabs, switchTab, canSwitch, createPanel = createSharedConfigurationPanel, canSimulate = () => true,
  loadSimulation = async () => (await import('./classes/priest_simulation.js')).PRIEST_SIMULATION, document: doc = document, loadData = async () => {
  const response = await fetch(new URL('../data/priest_preview.json', import.meta.url));
  if (!response.ok) throw new Error('Priest preview data could not be loaded.');
  return response.json();
} }) {
  const priest = doc.getElementById('btn-class-priest');
  const warlock = doc.getElementById('btn-class-warlock');
  const status = doc.getElementById('top-sim-status');
  let data, active = false, loading = false, savedTab, snapshot;
  const hidden = [], previews = [];
  let cachedBuildView, selectedPreset, runController, searchView;
  const inputsLocked = [];
  const readInputs = () => {
    const number = (name,fallback) => {
      const value = cachedBuildView.querySelector(`[name="priest-preview-${name}"]`)?.value??fallback;
      if (String(value).trim() === '') throw new Error(`Enter Priest ${name}.`);
      return Number(value);
    };
    const stats = Object.fromEntries(['spellPower','shadowPower','holyPower','hit','crit','intellect','stamina','spirit','mp5'].map(key=>[key,number(key)]));
    return {stats,sim:{duration:number('duration'),iterations:number('iterations')},race:selectedPreset?.race??'HUMAN',trees:data.trees,target:{level:number('targetLevel'),resistance:number('resistance'),distance:number('distance',0),isHumanoid:number('creatureType',0)===1,isBeast:number('creatureType',0)===2,controlImmune:number('controlImmune',1)===1,weaponIsMace:number('weaponIsMace',0)===1,
      incomingAttackInterval:number('incomingAttackInterval',0),incomingAttackType:number('incomingAttackType',1),incomingDamage:number('incomingDamage',0),mana:number('enemyMana',0),enemyHealingPerTick:number('enemyHealingPerTick',0),
      allyAttackInterval:number('allyAttackInterval',0),allyIncomingDamage:number('allyIncomingDamage',0),allyDistance:number('allyDistance',0),allyWeakenedSoul:number('allyWeakenedSoul',0)}};
  };
  const readBuild = () => resolvePriestUIBuild({...readInputs(),preset:selectedPreset});
  const updateSummary = build => {
    const s=build.summary;
    for(const [id,value] of Object.entries({'shadow-sp':s.shadowSP,'fire-sp':s.holySP,'max-mana':s.maxMana,'max-health':s.maxHealth,
      'spell-hit':`${s.hit.toFixed(1)}%`,'spell-crit':`${s.crit.toFixed(1)}%`,mp5:s.mp5,intellect:s.intellect,stamina:s.stamina,
      'spell-pen':0,'shadow-mult':`${s.shadowMultiplier.toFixed(2)}x`})) cachedBuildView.querySelector(`#priest-preview-sum-${id}`).textContent=typeof value==='number'?value.toLocaleString(undefined,{maximumFractionDigits:1}):value;
  };
  const clearResults = () => {
    for(const id of ['mean','range','elapsed'])cachedBuildView.querySelector(`#priest-preview-${id}`).textContent='—';
    for(const id of ['confidence','throughput'])cachedBuildView.querySelector(`#priest-preview-${id}`).textContent='';
    cachedBuildView.querySelector('#priest-preview-breakdown').innerHTML='';
    cachedBuildView.querySelector('#priest-preview-racial-outcome').textContent='';
    cachedBuildView.querySelector('#priest-preview-current-sim-damage-split').innerHTML='';
  };
  const refreshRun = () => {
    const run = cachedBuildView.querySelector('#priest-preview-run');
    try {
      const build=readBuild(); updateSummary(build);
      run.disabled=!canSimulate()||Boolean(searchView?.isRunning()); run.title=canSimulate()?'Shadow and Holy damage cores.':'WebGL2 is unavailable.';
    } catch(error) {run.disabled=true;run.title=error.message;for(const value of cachedBuildView.querySelectorAll('.stats-summary-grid strong'))value.textContent='—';}
  };
  priest.setAttribute('aria-pressed', 'false');
  warlock.setAttribute('aria-pressed', 'true');
  const showTalents = preset => {
    doc.getElementById('priest-preview-trees').innerHTML = renderPriestTalents(data, preset);
    doc.getElementById('priest-preview-preset').value = preset.id;
    const counts = Object.values(data.trees).map(nodes => nodes.reduce((total, node) => total + (preset.ranks[node.key] || 0), 0));
    doc.getElementById('priest-preview-talents-summary-badge').textContent = `${counts.join(' / ')} (${51 - counts.reduce((a, b) => a + b, 0)} remaining)`;
    const changed=selectedPreset!==preset;
    selectedPreset = preset;
    if(cachedBuildView){
      const race=preset.race??'HUMAN';
      cachedBuildView.querySelector('#priest-preview-race').value=race;
      const icon=cachedBuildView.querySelector('#priest-preview-slot-race-icon');
      icon.src=`./assets/icons/${RACE_ICONS[race]}`;icon.alt=RACE_NAMES[race];
      cachedBuildView.querySelector('#priest-preview-slot-race-picker').title=icon.alt;
      cachedBuildView.querySelector('#priest-preview-racials-list-container').innerHTML=PRIEST_RACIALS[race].map(r=>`<a class="racial-icon-badge" href="${escapeHTML(r.source)}" target="_blank" rel="noopener" title="${escapeHTML(r.name+': '+r.description+' '+r.coverage)}"><img src="./assets/icons/${escapeHTML(r.icon)}" alt="${escapeHTML(r.name)}"></a>`).join('');
      cachedBuildView.querySelector('#priest-preview-apl-edit-box').value=formatPriestPolicy({rotation:preset.rotation??'shadow',race:preset.race??'HUMAN',apl:preset.apl});
    }
    if (searchView&&!searchView.isRunning()) {try{searchView.refresh();}catch{}}
    if (runController) {if(changed)clearResults();refreshRun();}
  };
  priest.addEventListener('click', async () => {
    if (active || loading) return;
    if (!canSwitch()) { status.textContent = 'Finish or stop the current run before switching classes.'; return; }
    loading = true;
    try {
      data ||= await loadData();
      if (!canSwitch()) return;
      savedTab = tabs.find(t => doc.getElementById(t.btn).classList.contains('active'))?.btn;
      const title = doc.querySelector('.brand-title'), subtitle = doc.querySelector('.brand-sub');
      snapshot = { title: doc.title, brand: title.textContent, subtitle: subtitle.textContent, status: status.textContent };
      for (const tab of tabs) {
        const pane = doc.getElementById(tab.pane);
        const searchPanel=tab.btn==='btn-constrained-search'&&!searchView?createPanel(pane,{prefix:'priest-search'}):null;
        for (const child of Array.from(pane.children)) { hidden.push([child, child.style.display]); child.style.display = 'none'; }
        const view = doc.createElement('div'); view.className = 'priest-preview'; previews.push(view); pane.appendChild(view);
        if (tab.btn === 'btn-current-build') {
          if (cachedBuildView) {
            view.appendChild(cachedBuildView);
          } else {
            // One configuration structure for both classes; only class content changes.
            const build = createPanel(doc.querySelector('[data-shared-panel="current-configuration"]'), {
              prefix: 'priest-preview',
              rename: { firePower: { name: 'holyPower', label: 'Holy Power' } },
              remove: ['#slot-pet-picker', '#slot-ds-picker', '#popover-race', '#paperdoll-view',
                '#apl-reference-details', '#check-multidot-corruption', '#regret-panel',
                'input[type="hidden"]']
            });
            // Controls are present in their normal positions, with unavailable actions disabled.
            for (const control of build.querySelectorAll('input, select, textarea')) {
              const name = control.name.replace('priest-preview-', '');
              control.disabled = !['spellPower', 'shadowPower', 'holyPower', 'hit', 'crit',
                'intellect', 'stamina', 'spirit', 'mp5', 'iterations', 'duration'].includes(name);
            }
            for (const field of build.querySelectorAll('.stat-field')) {
              if (field.querySelector('[name="priest-preview-racialPolicy"], [name="priest-preview-targetIsBeast"]')) field.remove();
            }
            for (const row of build.querySelectorAll('.buff-item-row')) {
              if (row.querySelector('[name="priest-preview-greaterFirepowerElixir"], [name="priest-preview-curseOfElements"], [name="priest-preview-improvedScorch"]')) row.remove();
            }
            build.querySelector('#priest-preview-direct-stats-view').style.display = '';
            build.querySelector('#priest-preview-btn-mode-direct').classList.add('active');
            build.querySelector('#priest-preview-btn-mode-equipped').classList.remove('active');
            const raceIcon = build.querySelector('#priest-preview-slot-race-icon');
            raceIcon.src = './assets/icons/Achievement_Character_Human_Male.png'; raceIcon.alt = 'Human';
            build.querySelector('#priest-preview-slot-race-picker').title = 'Human';
            build.querySelector('#priest-preview-racials-list-container').innerHTML = `<div class="racial-icon-badge" title="The Human Spirit: Spirit increased by 5%"><img src="./assets/icons/Spell_Holy_MagicalSentry.png" alt="The Human Spirit"></div><div class="racial-icon-badge" title="Perception"><img src="./assets/icons/Spell_Holy_MindVision.png" alt="Perception"></div>`;
            const trees = build.querySelector('#priest-preview-talent-trees-container');
            trees.id = 'priest-preview-trees'; trees.innerHTML = '';
            const preset = build.querySelector('#priest-preview-talent-preset-select');
            preset.id = 'priest-preview-preset'; preset.disabled = false;
            preset.innerHTML = '<option value="">Load Preset…</option>' + data.presets.map(p => `<option value="${p.id}">${escapeHTML(p.label)}</option>`).join('');
            build.querySelector('#priest-preview-detailed-results').disabled = false;
            const badge = build.querySelector('#priest-preview-talents-summary-badge');
            badge.textContent = '14 / 0 / 37 (0 remaining)';
            const apl = build.querySelector('#priest-preview-apl-edit-box');
            apl.value = 'shadow_word_pain\nmind_blast\nshadow_word_death\nmind_flay'; apl.placeholder = ''; apl.readOnly = true;
            // Remove Warlock-specific reference and multi-target policy content.
            const aplPanel = apl.closest('section');
            if (aplPanel) {
              for (const child of Array.from(aplPanel.children)) {
                if (!child.classList.contains('wow-panel-header') && !child.classList.contains('apl-edit-container')) child.remove();
              }
              const caption = aplPanel.querySelector('.wow-panel-header span:last-child');
              if (caption) caption.textContent = '';
            }
            for (const value of build.querySelectorAll('.stats-summary-grid strong')) value.textContent = '—';
            for (const metric of build.querySelectorAll('.stat-metric')) {
              const label = metric.querySelector('span');
              if (label) label.textContent = label.textContent.replace('Fire', 'Holy');
            }
            for (const id of ['mean', 'range', 'elapsed']) build.querySelector(`#priest-preview-${id}`).textContent = '—';
            for (const id of ['confidence', 'throughput']) build.querySelector(`#priest-preview-${id}`).textContent = '';
            build.querySelector('#priest-preview-status').textContent = '';
            build.querySelector('#priest-preview-current-sim-damage-split').innerHTML = '';
            build.querySelector('#priest-preview-breakdown').innerHTML = '';
            const raceSelect=doc.createElement('select');raceSelect.id='priest-preview-race';raceSelect.setAttribute('aria-label','Priest race');raceSelect.innerHTML=PRIEST_RACES.map(race=>`<option value="${race}">${RACE_NAMES[race]}</option>`).join('');
            build.querySelector('#priest-preview-racials-list-container').before(raceSelect);
            raceSelect.addEventListener('change',()=>{showTalents({...selectedPreset,race:raceSelect.value});searchView?.invalidate();});
            const encounter=doc.createElement('div');encounter.className='priest-racial-encounter';
            const field=(name,label,value,step='1')=>`<label class="stat-field"><span>${label}</span><input type="number" name="priest-preview-${name}" value="${value}" min="0" step="${step}"></label>`;
            const select=(name,label,choices)=>`<label class="stat-field"><span>${label}</span><select name="priest-preview-${name}">${choices.map(([value,text])=>`<option value="${value}">${text}</option>`).join('')}</select></label>`;
            encounter.innerHTML=panel('Racial encounter settings',`<p>Intervals of 0 disable incoming attacks. Healing and retaliation racials require these triggers. Race cooldowns are used automatically; all races can cast Devouring Plague.</p><div class="stats-grid">${select('creatureType','Enemy type',[[0,'Other'],[1,'Humanoid'],[2,'Beast']])}${select('controlImmune','Enemy control immunity',[[1,'Immune'],[0,'Can be controlled']])}${select('weaponIsMace','Weapon',[[0,'Other / Staff'],[1,'Mace']])}${field('distance','Enemy distance (yd)',0,'0.1')}${select('incomingAttackType','Incoming attack type',[[1,'Melee'],[2,'Ranged'],[3,'Spell']])}${field('incomingAttackInterval','Incoming attack interval (sec)',0,'0.1')}${field('incomingDamage','Damage per incoming hit',0)}${field('enemyMana','Enemy mana',0)}${field('enemyHealingPerTick','Enemy healing every 2 sec',0)}${field('allyAttackInterval','Ally hit interval (sec)',0,'0.1')}${field('allyIncomingDamage','Damage per ally hit',0)}${field('allyDistance','Ally distance (yd)',0,'0.1')}${field('allyWeakenedSoul','Ally Weakened Soul (sec)',0)}</div><p>One enemy and one 5000-Health ally. Contingency Plan uses its verified base shield/heal; server-side scaling is unavailable. Movement, stealth and friendly-target shielding rotations are outside this encounter.</p><p id="priest-preview-racial-outcome" aria-live="polite"></p>`);
            build.appendChild(encounter);
            cachedBuildView = build;
            view.appendChild(build);
            runController = createPriestRunController({ readBuild, loadSimulation,
              onStatus: message => { build.querySelector('#priest-preview-status').textContent=message;status.textContent=message; },
              onProgress: fraction => { build.querySelector('#priest-preview-progress-bar-fill').style.width=`${fraction*100}%`;build.querySelector('#priest-preview-progress-text').textContent=`${Math.round(fraction*100)}%`; },
              onBusy: busy => {
                priest.disabled=busy;warlock.disabled=busy;
                const run=build.querySelector('#priest-preview-run');run.textContent=busy?'Stop':'Run';run.disabled=false;
                build.querySelector('#priest-preview-progress-container').style.display=busy?'':'none';
                if(busy)for(const input of build.querySelectorAll('input, select')) {inputsLocked.push([input,input.disabled]);input.disabled=true;}
                else {for(const [input,disabled] of inputsLocked.splice(0))input.disabled=disabled;refreshRun();}
              },
              onResult: result => {
                const mean=result.summary.mean,sd=result.summary.sd,n=result.summary.samples;
                build.querySelector('#priest-preview-mean').textContent=mean.toFixed(1);
                build.querySelector('#priest-preview-confidence').textContent=n>1?`± ${(1.96*sd/Math.sqrt(n)).toFixed(1)} DPS (95%)`:'';
                const sorted=result.states.map(s=>s.total/result.config.duration).sort((a,b)=>a-b);
                build.querySelector('#priest-preview-range').textContent=`${sorted[Math.floor((n-1)*0.05)].toFixed(1)}–${sorted[Math.floor((n-1)*0.95)].toFixed(1)}`;
                build.querySelector('#priest-preview-elapsed').textContent=`${(result.timing.elapsedMs/1000).toFixed(2)} s`;
                build.querySelector('#priest-preview-throughput').textContent=`${Math.round(n*1000/result.timing.elapsedMs).toLocaleString()} fights/s`;
                const schoolDamage={Shadow:0,Holy:0,Arcane:0};for(const [index,spell] of result.summary.spells.entries())schoolDamage[index===11?'Arcane':[0,1,2,3,8,10,13,14,15,23].includes(index)?'Shadow':'Holy']+=spell.damage;
                build.querySelector('#priest-preview-current-sim-damage-split').innerHTML=result.summary.total>0?Object.entries(schoolDamage).filter(([,damage])=>damage>0).map(([school,damage])=>`<div class="split-seg ${school.toLowerCase()}" style="width:${100*damage/result.summary.total}%">${school}</div>`).join(''):'';
                const racialResult=result.summary.racials??{};build.querySelector('#priest-preview-racial-outcome').textContent=`Healing: ${(racialResult.healingDone??0).toFixed(1)} · Absorbed: ${(racialResult.absorbed??0).toFixed(1)} · Mana from Dark Sacrifice: ${(racialResult.darkSacrificeMana??0).toFixed(1)}`;
                build.querySelector('#priest-preview-breakdown').innerHTML=result.summary.spells.map(spell=>`<tr><td>${escapeHTML(spell.name)}</td><td title="Healing: ${(spell.healing??0).toFixed(1)}">${spell.damage.toFixed(1)}${spell.healing>0?` (healed ${spell.healing.toFixed(1)})`:""}</td><td>${(spell.damage/result.config.duration).toFixed(1)}</td><td>${spell.casts.toFixed(1)}</td><td>${spell.crits.toFixed(1)}</td><td>${result.summary.total>0?(spell.damage/result.summary.total*100).toFixed(1):'0'}%</td></tr>`).join('');
              }
            });
            build.querySelector('#priest-preview-run').addEventListener('click',()=>{
              if(runController.isRunning())runController.cancel();
              else if(!searchView?.isRunning())runController.run({detailedResults:build.querySelector('#priest-preview-detailed-results').checked});
            });
            build.addEventListener('input',()=>{if(!runController.isRunning()){clearResults();refreshRun();searchView?.invalidate();}});
          }
        } else if(tab.btn==='btn-constrained-search') {
          if(!searchView)searchView=createPriestSearchView({panel:searchPanel,readInputs,loadSimulation,presets:data.presets.filter(p=>p.id!=='priest_search_result'),
            canRun:()=>canSimulate()&&!runController?.isRunning(),
            unavailableReason:()=>!canSimulate()?'WebGL2 is unavailable.':runController?.isRunning()?'Finish or stop the current Priest run before searching.':'',
            onStatus:message=>{status.textContent=message;},
            onBusy:busy=>{
              priest.disabled=busy;warlock.disabled=busy;
              if(busy){for(const input of cachedBuildView.querySelectorAll('input, select, button')){inputsLocked.push([input,input.disabled]);input.disabled=true;}}
              else{for(const [input,disabled] of inputsLocked.splice(0))input.disabled=disabled;refreshRun();}
            },
            onApply:preset=>{
              data.presets=data.presets.filter(p=>p.id!==preset.id);data.presets.push(preset);
              const select=cachedBuildView.querySelector('#priest-preview-preset');
              select.innerHTML='<option value="">Load Preset…</option>'+data.presets.map(p=>`<option value="${escapeHTML(p.id)}">${escapeHTML(p.label)}</option>`).join('');
              showTalents(preset);switchTab('btn-current-build');
            }});
          view.appendChild(searchView.panel);
        } else {
          const label = tab.btn === 'btn-compare-specs' ? 'Compare Priest presets' : tab.btn === 'btn-constrained-search' ? 'Priest build search' : 'Priest APL synthesis';
          view.innerHTML = panel(label, '');
        }
      }
      showTalents(selectedPreset || data.presets[0]);
      doc.getElementById('priest-preview-preset').onchange = event => {
        const preset = data.presets.find(p => p.id === event.target.value);
        if (preset) showTalents(preset);
      };
      active = true; title.textContent = 'PRIEST SIMULATOR'; subtitle.textContent = snapshot.subtitle; doc.title = 'Priest Shadow Simulator';
      status.textContent = 'Priest';
      priest.classList.add('active'); warlock.classList.remove('active'); priest.title = 'Priest (Active)'; warlock.title = 'Switch to Warlock';
      priest.setAttribute('aria-pressed', 'true'); warlock.setAttribute('aria-pressed', 'false');
      switchTab('btn-current-build');
    } catch (error) { status.textContent = error.message; }
    finally { loading = false; }
  });
  warlock.addEventListener('click', () => {
    if (!active || runController?.isRunning() || searchView?.isRunning()) return;
    cachedBuildView.remove();
    searchView?.panel.remove();
    for (const view of previews.splice(0)) view.remove();
    for (const [child, display] of hidden.splice(0)) child.style.display = display;
    doc.title = snapshot.title; doc.querySelector('.brand-title').textContent = snapshot.brand; doc.querySelector('.brand-sub').textContent = snapshot.subtitle; status.textContent = snapshot.status;
    active = false; priest.classList.remove('active'); warlock.classList.add('active'); priest.title = 'Switch to Priest'; warlock.title = 'Warlock (Active)';
    priest.setAttribute('aria-pressed', 'false'); warlock.setAttribute('aria-pressed', 'true'); switchTab(savedTab);
  });
}
