import { ACTION_NAMES } from './regret.js';
import { showTooltip, hideTooltip } from './tooltips.js';

export const ACTION_ICONS={
 1:'Spell_Shadow_BurningSpirit.png',2:'Spell_Shadow_ShadowBolt.png',3:'Spell_Fire_SoulBurn.png',
 4:'Spell_Fire_Fireball02.png',6:'Spell_Shadow_AbominationExplosion.png',7:'Spell_Shadow_AuraOfDarkness.png',
 8:'Spell_Shadow_CurseOfSargeras.png',9:'Spell_Fire_Immolation.png',10:'Spell_Fire_Fireball.png',
 11:'Spell_Shadow_ScourgeBuild.png',12:'Spell_Fire_Burnout.png',16:'Spell_Shadow_ShadowBolt.png',
 17:'Spell_Shadow_Requiem.png',18:'ability_deathknight_hemorrhagicfever.png',19:'Spell_Fire_Incinerate.png',
};
const number=value=>value.toLocaleString(undefined,{maximumFractionDigits:2});
export const gainLabel=value=>`${value>0?'+':''}${number(value)} DPS`;

function actionIcon(action,row,chosen=false,best=false){
 const name=ACTION_NAMES[action],button=document.createElement('button');
 button.type='button';
 button.className=`regret-action-icon ${chosen?'is-chosen':row.deltaDps>0?'is-gain':row.deltaDps<0?'is-loss':'is-neutral'}${best?' is-best':''}`;
 const subtitle=chosen?'Chosen action':gainLabel(row.deltaDps);
 const desc=chosen?'Baseline':`Gain compared with chosen action: ${gainLabel(row.deltaDps)}`;
 const footer=chosen?'':`Adjusted 95% interval: ${gainLabel(row.lower)} to ${gainLabel(row.upper)}`;
 button.setAttribute('aria-label',`${name} · ${subtitle}`);
 button.setAttribute('data-wow-tooltip-title',name);
 button.setAttribute('data-wow-tooltip-subtitle',subtitle);
 button.setAttribute('data-wow-tooltip',desc);
 button.setAttribute('data-wow-tooltip-footer',footer);
 button.addEventListener('focus',()=>{
  const rect=button.getBoundingClientRect();
  showTooltip({clientX:rect.right,clientY:rect.top},{title:name,subtitle,desc,footer});
 });
 button.addEventListener('blur',hideTooltip);
 const img=document.createElement('img');img.src=`./assets/icons/${ACTION_ICONS[action]}`;img.alt=name;
 button.append(img);return button;
}

export function renderRegretResults(container,result){
 container.replaceChildren();container.hidden=false;
 if(!result.items.length){container.textContent='No clear improvements found.';return;}
 const scroll=document.createElement('div');scroll.className='regret-table-scroll';
 const table=document.createElement('table');table.className='regret-table';
 table.setAttribute('aria-label','Action regret comparisons');
 const head=document.createElement('thead'),headers=document.createElement('tr');
 for(const title of ['Action','Cast at','Alternatives','Max DPS gain']){
  const th=document.createElement('th');th.scope='col';th.textContent=title;headers.append(th);
 }
 head.append(headers);table.append(head);
 const body=document.createElement('tbody');
 for(const item of result.items){
  const tr=document.createElement('tr'),chosen=document.createElement('td'),time=document.createElement('td');
  chosen.append(actionIcon(item.policyAction,{deltaDps:0},true));
  time.className='regret-cast-time';time.textContent=`${number(item.time)}s`;
  const alternatives=document.createElement('td'),icons=document.createElement('div');icons.className='regret-ranked-actions';
  for(const [index,row] of item.alternatives.entries()){
   icons.append(actionIcon(row.action,row,false,index===0));
  }
  alternatives.append(icons);
  const gain=document.createElement('td');gain.className='regret-max-gain';gain.textContent=gainLabel(item.maxDpsGain);
  tr.append(chosen,time,alternatives,gain);body.append(tr);
 }
 table.append(body);scroll.append(table);container.append(scroll);
}
