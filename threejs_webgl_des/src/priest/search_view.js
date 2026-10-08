import { formatPriestPolicy } from './policy.js';
import { TALENT_DEFINITIONS, RACES, ROTATIONS, ROTATION_LABELS, TREE_NAMES, candidateToPreset, runPriestConstrainedSearch } from './search.js';
const escapeHTML=value=>String(value).replace(/[&<>"']/g,char=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[char]));
const options=(values,label=value=>value)=>values.map(value=>`<option value="${escapeHTML(value)}">${escapeHTML(label(value))}</option>`).join('');

export function createPriestSearchView({panel,readInputs,loadSimulation,presets,onApply,onBusy=()=>{},onStatus=()=>{},canRun=()=>true,unavailableReason=()=>'',runSearch=runPriestConstrainedSearch}){
  const get=id=>panel.querySelector(`#priest-search-${id}`);
  panel.classList.remove('tab-pane');panel.hidden=false;panel.style.display='';
  for(const control of panel.querySelectorAll('input, select, button'))control.disabled=false;
  for(const button of panel.querySelectorAll('button'))button.title='';
  for(const id of ['ga-locked-pet-ds','ga-init-explore','ga-min-explore'])get(id)?.closest('div')?.remove();
  get('ga-advanced-tuning-box').querySelector('span:last-child')?.remove();
  for(const node of panel.querySelectorAll('.specs-base-stats-text span'))node.textContent=node.textContent.replace('Fire SP','Holy SP').replace('Active Gear Stats','Current Direct Stats');
  panel.querySelector('thead tr').innerHTML='<th>Rank</th><th>Spec (Disc / Holy / Shadow)</th><th>Race</th><th>Policy</th><th>Mean DPS</th><th>95% CI</th><th>Action</th>';
  const talentOptions='<option value="-1">Any Talent</option>'+TALENT_DEFINITIONS.map((node,index)=>`<option value="${index}">${escapeHTML(node.treeName)}: ${escapeHTML(node.name)} (${node.max}/${node.max})</option>`).join('');
  for(const n of [1,2,3]){get(`ga-req-talent-${n}`).innerHTML=talentOptions;get(`ga-req-talent-${n}`).value='-1';}
  get('ga-locked-race').innerHTML=options(['ALL',...RACES],value=>value==='ALL'?'Any supported race':value);get('ga-locked-race').value='ALL';
  get('ga-locked-rotation').innerHTML=options(['ALL',...ROTATIONS],value=>value==='ALL'?'Any policy':ROTATION_LABELS[value]);get('ga-locked-rotation').value='ALL';
  get('ga-apl-mode').value='coevolve';
  get('ga-apl-lock-conditions').checked=true;
  get('btn-run-ga').disabled=!canRun();
  get('btn-run-ga').title=canRun()?'':unavailableReason();
  get('btn-stop-ga').style.display='none';
  let controller=null,candidates=[],history=[],selectedId=null,invalidated=false;
  const status=text=>{get('ga-selected-spec-details-panel').setAttribute('aria-live','polite');onStatus(text);};
  const numeric=id=>{const value=get(id).value;if(String(value).trim()==='')throw new Error('Enter Priest search settings.');return Number(value);};
  const readConfig=()=>({aplMode:get('ga-apl-mode').value,lockConditions:get('ga-apl-lock-conditions').checked,generations:numeric('ga-generations'),populationSize:numeric('ga-pop-size'),screeningSims:numeric('ga-screening-sims'),finalSims:numeric('ga-final-sims'),mutationRate:numeric('ga-mutation-rate'),seedPresets:get('ga-seed-presets').checked,optimizeRace:get('ga-optimize-race').checked,forcedRace:get('ga-locked-race').value,forcedRotation:get('ga-locked-rotation').value,
    requiredTalents:[1,2,3].map(n=>Number(get(`ga-req-talent-${n}`).value)).filter(index=>index>=0),presetsList:presets});
  const graph=()=>{
    const wrapper=get('ga-chart-wrapper');
    if(!history.length){wrapper.innerHTML='<div class="ga-chart-placeholder">Run search to compare discovered specs.</div>';return;}
    const min=Math.min(...history.map(p=>p.avgDps)),max=Math.max(...history.map(p=>p.bestDps)),span=Math.max(1,max-min);
    const x=i=>40+i*520/Math.max(1,history.length-1),y=dps=>240-(dps-min)*210/span;
    wrapper.innerHTML=`<svg viewBox="0 0 600 280" role="img" aria-label="Priest search best and population mean DPS by generation" style="width:100%;height:100%"><text x="40" y="20" fill="#94a3b8">${max.toFixed(1)} DPS</text><text x="40" y="265" fill="#94a3b8">${min.toFixed(1)} DPS · Generation 0–${history.at(-1).gen}</text>${[['bestDps','#4ade80'],['avgDps','#38bdf8']].map(([key,color])=>`<polyline fill="none" stroke="${color}" stroke-width="2" points="${history.map((p,i)=>`${x(i)},${y(p[key])}`).join(' ')}"/>${history.map((p,i)=>`<circle cx="${x(i)}" cy="${y(p[key])}" r="3" fill="${color}"><title>Generation ${p.gen}: ${p[key].toFixed(1)} DPS</title></circle>`).join('')}`).join('')}</svg>`;
  };
  const details=()=>{
    const candidate=candidates.find(c=>c.id===selectedId)??candidates[0];
    if(!candidate){get('ga-selected-spec-details-panel').innerHTML='<div class="empty-state">No candidate selected.</div>';return;}
    selectedId=candidate.id;
    const trees=TREE_NAMES.map((name,index)=>`<div><strong>${name}: ${candidate.points[index]}</strong><ul>${TALENT_DEFINITIONS.filter(node=>node.tree===index&&candidate.talents[node.treeKey][node.key]).map(node=>`<li>${escapeHTML(node.name)} ${candidate.talents[node.treeKey][node.key]}/${node.max}</li>`).join('')}</ul></div>`).join('');
    get('ga-selected-spec-details-panel').innerHTML=`<div class="wow-panel-header"><span>${escapeHTML(candidate.name)} · ${escapeHTML(candidate.race)} · ${escapeHTML(candidate.category)}</span><button type="button" class="wow-button" data-priest-apply="${candidates.indexOf(candidate)}" ${controller?'disabled':''}>Apply to Current Build</button></div><p>${candidate.mean_dps.toFixed(1)} DPS ± ${candidate.ci95.toFixed(1)} (95% CI) · ${candidate.min_dps.toFixed(1)}–${candidate.max_dps.toFixed(1)} DPS (5th–95th percentile)</p><pre>${escapeHTML(formatPriestPolicy(candidate))}</pre><div class="priest-search-talent-summary">${trees}</div>`;
  };
  const leaderboard=()=>{
    const leader=candidates[0]?.mean_dps??0,percentage=get('ga-chk-pct-leader').checked,showSD=get('ga-chk-show-sd').checked;
    get('ga-leaderboard-body').innerHTML=candidates.length?candidates.map((c,i)=>`<tr data-priest-select="${i}" class="${c.id===selectedId?'selected-row active':''}"><td>#${c.rank}</td><td>${escapeHTML(c.name)}</td><td>${escapeHTML(c.race)}</td><td>${escapeHTML(c.category)}</td><td>${percentage&&leader>0?`${(100*(c.mean_dps/leader-1)).toFixed(2)}%`:c.mean_dps.toFixed(1)}${showSD?` ± ${c.std_dev.toFixed(1)} SD`:''}</td><td>± ${c.ci95.toFixed(1)}</td><td><button type="button" class="wow-button wow-btn-small" data-priest-apply="${i}" ${controller?'disabled':''}>Apply</button></td></tr>`).join(''):'<tr><td colspan="7">Ready to search. Set constraints and run optimization.</td></tr>';
    details();
  };
  const render=state=>{
    candidates=state.candidates??state.elites??[];history=state.evolutionHistory??[];
    get('ga-stat-unique-configs').textContent=(state.uniqueConfigsCount??0).toLocaleString();get('ga-stat-total-fights').textContent=(state.totalSimulations??0).toLocaleString();
    get('ga-stat-best-dps').textContent=`${(candidates[0]?.mean_dps??0).toFixed(1)} DPS`;get('ga-stat-generations').textContent=`Gen ${history.at(-1)?.gen??0}`;
    get('ga-stat-gain').textContent=history.length?`${((candidates[0]?.mean_dps??0)-history[0].avgDps).toFixed(1)} DPS vs Gen 0`:'';
    leaderboard();graph();
  };
  const busy=value=>{
    onBusy(value);for(const control of panel.querySelectorAll('input, select'))control.disabled=value;
    get('btn-run-ga').style.display=value?'none':'';get('btn-stop-ga').style.display=value?'':'none';
    get('btn-run-ga').disabled=value||!canRun();
    get('btn-run-ga').title=value?'Search is running.':canRun()?'':unavailableReason();
  };
  panel.addEventListener('click',event=>{
    const apply=event.target.closest('[data-priest-apply]');
    if(apply&&!controller){const candidate=candidates[Number(apply.getAttribute('data-priest-apply'))];if(candidate)onApply(candidateToPreset(candidate));return;}
    const select=event.target.closest('[data-priest-select]');if(select){selectedId=candidates[Number(select.getAttribute('data-priest-select'))]?.id;leaderboard();}
  });
  for(const id of ['ga-chk-pct-leader','ga-chk-show-sd'])get(id).addEventListener('change',leaderboard);
  get('ga-advanced-tuning-chk').addEventListener('change',()=>{get('ga-advanced-tuning-box').style.display=get('ga-advanced-tuning-chk').checked?'':'none';});
  get('btn-stop-ga').addEventListener('click',()=>controller?.abort());
  const run=async()=>{
    if(controller||!canRun())return;
    controller=new AbortController();const signal=controller.signal;
    try{
      const base=readInputs(),config=readConfig();invalidated=false;selectedId=null;render({});busy(true);status('Loading Priest search…');
      const simulation=await loadSimulation();
      const result=await runSearch(base,config,{simulation,signal,onProgress:p=>status(`${p.phase} (${p.completed}/${p.total})`),onGeneration:state=>{render(state);status(state.status);}});
      render(result);status(result.stopped?'Search stopped. Best discovered specs preserved.':`Search complete: ${result.uniqueConfigsCount.toLocaleString()} unique builds, ${result.totalSimulations.toLocaleString()} fights.`);
      return result;
    }catch(error){status(`Search failed: ${error.message}`);}
    finally{controller=null;busy(false);leaderboard();}
  };
  get('btn-run-ga').addEventListener('click',run);
  const refresh=()=>{
    if(controller)return;
    let base;
    try{base=readInputs();}catch(error){get('btn-run-ga').disabled=true;get('btn-run-ga').title=error.message;return;}
    const {stats,sim}=base;
    for(const [id,value] of Object.entries({'ga-base-shadow-sp':stats.spellPower+stats.shadowPower,'ga-base-fire-sp':stats.spellPower+stats.holyPower,'ga-base-hit':`${stats.hit}%`,'ga-base-crit':`${stats.crit}%`,'ga-base-fight':`${sim.duration}s`}))get(id).textContent=value;
    get('btn-run-ga').disabled=!canRun();
    get('btn-run-ga').title=canRun()?'':unavailableReason();
  };
  const invalidate=()=>{if(controller)return;invalidated=true;selectedId=null;render({});refresh();};
  for(const id of ['ga-req-talent-1','ga-req-talent-2','ga-req-talent-3','ga-locked-race','ga-locked-rotation','ga-optimize-race'])get(id).addEventListener('change',invalidate);
  render({});
  return {panel,run,isRunning:()=>controller!==null,cancel:()=>controller?.abort(),refresh,invalidate,get invalidated(){return invalidated;}};
}
