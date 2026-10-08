import { racialGLSL } from './racial_kernel.js';
import { CONFIG, STATE, FAST_STATE, CONFIG_WORDS, HEAP_CAPACITY, COMPACT_STRIPES, ROTATIONS, SPELLS } from './model.js';
import { POLICY_RULES, DOT_MIN_TICKS } from './policy.js';
import { RNG_GLSL } from './rng.js';
export const VERTEX = `precision highp float;
in vec3 position;
void main(){gl_Position=vec4(position,1.0);}`;
const structure = (name, schema) => `struct ${name}{${Object.entries(schema).map(([key, type]) => `${type === 'f32' ? 'float' : 'uint'} ${key};`).join('\n')}};`;

// Starshards rank 7: https://foreverdb.net/spell/19305
// Instant Arcane DoT: 350 mana, 30 yd, 30s cooldown, six 1s ticks.
// The listed 0.167 coefficient applies to EACH 300-damage tick, not the whole DoT.
// The fast variant removes only observation code; DES and RNG stay identical.
export function buildFragment(detailed = true) {
  const schema = detailed ? STATE : FAST_STATE;
  const counter = (name, amount) => detailed ? `switch(spell){${SPELLS.map((_,i)=>i).map(i => `case ${i}u:s.${name}${i}${amount};break;`).join('')}}` : '';
  const priorities=Object.entries(POLICY_RULES).map(([rotation,rules])=>`case ${ROTATIONS[rotation]}u:switch(spell){${[...new Set(rules.map(r=>r.spell))].map(spell=>`case ${spell}u:return ${rules.findIndex(r=>r.spell===spell)}u;`).join('')}default:return 99u;}`).join('');
  const selections=Object.entries(POLICY_RULES).map(([rotation,rules])=>{
    return `case ${ROTATIONS[rotation]}u:${rules.map(r=>`if(eligible(${r.spell}u,at)&&${((rotation==='holy'||rotation==='mixed')&&r.spell===7)?'freeNova':r.extra}&&affordable(${r.spell}u,at))return ${r.spell}u;`).join('')}break;`;
  }).join('');
  return `precision highp float;
precision highp int;
precision highp usampler2D;
layout(location=0) out uvec4 report0;
layout(location=1) out uvec4 report1;
layout(location=2) out uvec4 report2;
layout(location=3) out uvec4 report3;
uniform uint configWords[${CONFIG_WORDS}];
uniform highp usampler2D configTex;
uniform uint numConfigs, fightsPerConfig, seed, offset, count, gridWidth, eventBudget;
${structure('FightConfig', CONFIG)}
${structure('FightState', schema)}
struct Event{uint at; uint kind; uint spell; uint order; uint generation; uint channel;};
FightConfig c; FightState s; Event queue[${HEAP_CAPACITY}];
uvec2 rng[4]; uint size, serial, now, painEnd, blastReady, deathReady, painTicks, weavingStacks, weavingEnd, fsrEnd, focusReady, holyFireEnd, holyFireTicks, penanceReady, infusionEnd, infusionReady, plagueEnd, plagueTicks, plagueReady, generation, plagueGeneration, deaths, spiritTapEnd, flayToken, flaySecondTick, starshardsReady, starshardsTicks;
bool castFocused; float penanceCritBonus, enemyMana; bool freeNova;
${RNG_GLSL}
bool racial(uint race);
float shadowPower();
bool hit(uint spell);
void countCast(uint spell);
void damage(uint spell,float amount,bool crit);
void enqueue(uint at,uint kind,uint spell);
${racialGLSL(detailed)}
uint priority(uint kind){return kind==4u?0u:kind+1u;}
bool beforeEvent(Event a,Event b){return a.at<b.at||(a.at==b.at&&(priority(a.kind)<priority(b.kind)||(a.kind==b.kind&&a.order<b.order)));}
void enqueueToken(uint at,uint kind,uint spell,uint token){
 if(size>=${HEAP_CAPACITY}u){s.done=2u;return;}
 Event e=Event(at,kind,spell,serial++,generation,token);uint i=size++;s.highWater=max(s.highWater,size);
 for(int guard=0;guard<6;guard++){if(i==0u)break;uint p=(i-1u)/2u;if(!beforeEvent(e,queue[p]))break;queue[i]=queue[p];i=p;}queue[i]=e;
}
void enqueue(uint at,uint kind,uint spell){enqueueToken(at,kind,spell,0u);}
Event dequeue(){
 Event e=queue[0];size--;
 if(size>0u){queue[0]=queue[size];uint i=0u;
 for(int guard=0;guard<6;guard++){uint l=i*2u+1u,r=l+1u,k=i;if(l<size&&beforeEvent(queue[l],queue[k]))k=l;if(r<size&&beforeEvent(queue[r],queue[k]))k=r;if(k==i)break;Event t=queue[i];queue[i]=queue[k];queue[k]=t;i=k;}}
 return e;
}
void damage(uint spell,float amount,bool crit);
void graveProc();
float racialCrit();
float haste();
uint hasted(uint ms);
uint gcd();
float eurekaDiscount(uint spell,uint at);
float useEureka(uint spell);
void heal(uint spell,float amount,bool ally);
void countCast(uint spell){${counter('casts', '++')}if(spell<=12u)graveProc();}
void countMiss(uint spell){${counter('misses', '++')}}
bool isShadow(uint spell){return spell<4u||spell==8u||spell==10u||spell==13u||spell==14u||spell==15u||spell==16u;}
float shadowPower(){return c.spellPower+(now<spiritTapEnd?c.spiritTapSpellPower:0.0);}
float holyPower(){return c.holySpellPower+(now<spiritTapEnd?c.spiritTapSpellPower:0.0);}
bool inRange(uint spell){
 float range=spell==12u?20.0:spell==1u?(20.0+c.flayRangeBonus)*c.shadowRangeMultiplier:
  spell==7u?10.0*c.holyRangeMultiplier:
  (spell==4u||spell==5u)?30.0*c.holyRangeMultiplier:isShadow(spell)?30.0*c.shadowRangeMultiplier:30.0;
 return c.targetDistance<=range;
}
void damage(uint spell,float amount,bool crit){
 if(amount>0.0)confuseEnd=now;
 if((racial(3u)||racial(5u))&&c.targetIsBeast!=0u&&spell!=23u)amount*=1.05;
 if(isShadow(spell)){
 if(now>=weavingEnd)weavingStacks=0u;
 amount*=1.0+float(weavingStacks)*0.02;
 if(amount>0.0&&c.weavingChance>0.0){if(random01()<c.weavingChance){weavingStacks=min(5u,weavingStacks+1u);weavingEnd=now+15000u;}}
 }
 if(spell!=9u&&spell!=23u&&now<infusionEnd)amount*=1.2;
 ${detailed?'s.threat+=spell==7u||spell==13u?0.0:spell==12u?0.1*amount*c.holyThreatMultiplier:amount*(isShadow(spell)?c.shadowThreatMultiplier:spell==9u?1.0:spell==11u?c.arcaneThreatMultiplier:c.holyThreatMultiplier);':''}
 s.total+=amount;if(spell==8u)heal(8u,amount,false);${counter('damage', '+=amount')}${counter('hits', '++')}if(crit){${counter('crits', '++')}}}
bool hit(uint spell){bool success=random01()<(spell==9u?c.wandHitChance:spell==11u?c.arcaneHitChance:isShadow(spell)?c.hitChance:c.holyHitChance);if(!success)countMiss(spell);return success;}
void directDamage(uint spell,float critBonus){
 if(!hit(spell))return;bool crit=random01()<min(1.0,c.critChance+racialCrit()+(racial(3u)&&c.weaponIsMace!=0u?0.01:0.0)+critBonus+(spell==3u&&float(now)>=float(c.durationMs)*0.8?c.deathExecuteCritBonus:0.0));
 float base=spell==2u?mix(472.0,498.0,random01()):mix(434.0,462.0,random01());
 damage(spell,(base+shadowPower()*(1.5/3.5))*c.damageMultiplier*(spell==3u?c.instantDamageMultiplier:1.0)*(crit?c.critMultiplier:1.0)*castEureka,crit);
}
void procFreeNova(bool periodic){
 if(c.holyNovaEnabled==0u)return;
 float chance=0.05+(periodic?c.searingNovaChance:0.0);
 if(random01()<chance)freeNova=true;
}
bool holyDamage(uint spell,float critBonus){
 if(!hit(spell))return false;
 bool crit=random01()<min(1.0,c.holyCritChance+racialCrit()+(racial(3u)&&c.weaponIsMace!=0u?0.01:0.0)+critBonus);
 float base=spell==12u?mix(272.0,306.0,random01()):spell==4u?mix(160.0,180.0,random01()):spell==5u?mix(184.0,232.0,random01()):spell==6u?131.0:mix(174.0,200.0,random01());
 float coeff=spell==12u?0.143:spell==4u?2.5/3.5:spell==5u?0.75:spell==6u?0.285:0.107;
 float bonus=(spell==4u||spell==6u)&&now<holyFireEnd?1.0+c.powerInLightBonus:1.0;
 damage(spell,(base+holyPower()*coeff)*c.holyDamageMultiplier*bonus*(spell==7u||spell==12u?c.instantDamageMultiplier:1.0)*(crit?1.5:1.0)*(spell==6u?penanceEureka:castEureka),crit);if(spell==5u)procFreeNova(false);return true;
}
bool focusAvailable(){return c.innerFocusEnabled!=0u&&now>=focusReady;}
void payHoly(float base,bool focused){float cost=focused?0.0:base*c.holyBaseCostMultiplier*(castEureka>1.0?0.9:1.0);s.mana-=cost;s.manaSpent+=cost;if(cost>0.0)fsrEnd=now+5000u;}
void pay(float base,bool focused){float cost=focused?0.0:base*c.costMultiplier*(castEureka>1.0?0.9:1.0);s.mana-=cost;s.manaSpent+=cost;if(cost>0.0)fsrEnd=now+5000u;}
// Event kinds: regen=0, periodic damage=1, cast impact=2, ready=3, credited target death/replacement=4.
// Replacements resolve before regen/damage and carry a generation to discard old-target impacts.
// Same-time damage resolves before the next decision; fight-end damage is inclusive.
bool compositePolicy(){return c.rotation==0u||c.rotation==5u||c.rotation==14u;}
uint castTime(uint spell){
 if(spell==2u)return hasted(1500u);
 if(spell==4u||spell==5u)return hasted(uint(round((spell==4u?2500.0:3500.0)-c.holyCastReduction*1000.0)));
 if(spell==10u)return hasted(uint(round(3000.0-c.manaBurnCastReduction*1000.0)));
 return 0u;
}
float baseSpellCost(uint spell){
 if(spell==7u&&freeNova)return 0.0;
 if(spell==0u)return 470.0*c.instantCostMultiplier*c.costMultiplier;
 if(spell==1u)return 205.0*c.costMultiplier;
 if(spell==2u)return 350.0*c.costMultiplier;
 if(spell==3u)return 340.0*c.instantCostMultiplier*c.costMultiplier;
 if(spell==4u)return 280.0*c.holyCostMultiplier*c.holyBaseCostMultiplier;
 if(spell==5u)return 255.0*c.holyCostMultiplier*c.holyBaseCostMultiplier;
 if(spell==6u)return 355.0*c.penanceCostMultiplier*c.holyBaseCostMultiplier;
 if(spell==7u)return 750.0*c.instantCostMultiplier*c.holyBaseCostMultiplier;
 if(spell==8u)return 985.0*c.plagueCostMultiplier*c.instantCostMultiplier*c.costMultiplier;
 if(spell==12u)return 225.0*c.holyBaseCostMultiplier*c.instantCostMultiplier;
 if(spell==11u)return 350.0*c.arcaneCostMultiplier*c.instantCostMultiplier;
 if(spell==10u)return 270.0*c.costMultiplier;
 return 0.0;
}
float spellCost(uint spell){return baseSpellCost(spell)*eurekaDiscount(spell,now);}
uint readyAt(uint spell){
 if(spell==12u)return chastiseReady;
 if(spell==0u)return painEnd;
 if(spell==2u)return blastReady;
 if(spell==3u)return deathReady;
 if(spell==5u)return holyFireEnd;
 if(spell==6u)return penanceReady;
 if(spell==11u)return starshardsReady;
 if(spell==8u)return plagueReady;
 return now;
}
bool focusedAt(uint at){return c.innerFocusEnabled!=0u&&at>=focusReady;}
bool eligible(uint spell,uint at){
 if(at>=c.durationMs||!inRange(spell)||at<readyAt(spell))return false;
 uint left=c.durationMs-at;
 if(spell==12u&&(!racial(3u)||c.targetIsHumanoid==0u))return false;
 if(spell==11u&&(c.starshardsEnabled==0u||left<(compositePolicy()?6000u:1000u)))return false;
 if(castTime(spell)>left)return false;
 if(spell==0u&&left<(compositePolicy()?${DOT_MIN_TICKS.pain}u:1u)*3000u)return false;
 if(spell==8u&&(c.plagueEnabled==0u||left<(compositePolicy()?${DOT_MIN_TICKS.plague}u:1u)*3000u))return false;
 if(spell==5u&&(compositePolicy()||c.rotation==13u)&&left<castTime(5u)+${DOT_MIN_TICKS.holyFire}u*2000u)return false;
 if(spell==1u&&(c.mindFlayEnabled==0u||left<hasted(1000u)))return false;
 if(spell==6u&&c.penanceEnabled==0u)return false;
 if(spell==7u&&c.holyNovaEnabled==0u)return false;
 if(spell==10u&&enemyMana<=0.0)return false;
 return true;
}
float regenAt(uint at,uint fsr){
 float ratio=at<fsr?c.castingRegenRatio:1.0;
 if(at<spiritTapEnd&&at<fsr)ratio=max(ratio,0.5);
 return c.manaPerTick+c.spiritManaPerTick*(at<spiritTapEnd?2.0:1.0)*ratio;
}
// Project only known regen ticks over the six-second pooling horizon. At equal
// timestamps regen precedes cast payment, just as in the actual event queue.
float projectedMana(uint at,float spend,uint spendAt){
 float mana=s.mana;uint fsr=fsrEnd;bool paid=spend==0.0;
 uint tick=(now/2000u+1u)*2000u;
 for(int guard=0;guard<4;guard++){
  if(tick>at)break;
  if(!paid&&spendAt<tick){mana-=spend;fsr=spendAt+5000u;paid=true;}
  mana=min(c.maxMana,mana+regenAt(tick,fsr));tick+=2000u;
 }
 if(!paid&&spendAt<=at)mana-=spend;
 return mana;
}
bool affordable(uint spell,uint at){return (spell!=9u&&focusedAt(at))||projectedMana(at,0.0,now)>=spellCost(spell);}
uint aplWord(uint i){switch(i){${Array.from({length:11},(_,i)=>`case ${i}u:return c.apl${i};`).join('')}default:return 0u;}}
bool aplAllows(uint word,uint at){return (word&(1u<<18u))!=0u && 100.0*float(c.durationMs-at)/float(c.durationMs)<=float((word>>4u)&127u) && 100.0*projectedMana(at,0.0,now)/c.maxMana>=float((word>>11u)&127u);}
uint priorityRank(uint spell){if(c.aplEnabled!=0u){for(uint i=0u;i<11u;i++){uint word=aplWord(i);if((word&15u)==spell+1u&&aplAllows(word,now))return i;}return 99u;}switch(c.rotation){${priorities}default:return 99u;}}
uint selectSpell(uint at){if(c.aplEnabled!=0u){for(uint i=0u;i<11u;i++){uint word=aplWord(i),action=word&15u;if(action==0u)continue;uint spell=action-1u;if(aplAllows(word,at)&&eligible(spell,at)&&((spell!=7u)||freeNova)&&affordable(spell,at))return spell;}return 255u;}switch(c.rotation){${selections}default:break;}return 255u;}
uint affordableAt(uint spell,uint first,uint last){
 uint at=first;
 for(int guard=0;guard<5;guard++){
  if(at>last||!eligible(spell,at))break;
  if(affordable(spell,at))return at;
  uint next=(at/2000u+1u)*2000u;
  if(c.innerFocusEnabled!=0u&&focusReady>at)next=min(next,focusReady);
  at=next;
 }
 return c.durationMs+1u;
}
// Pooling should preserve damage per mana, not force an expensive nuke while
// an efficient filler is available. Estimates use current resolved talent
// multipliers and only ticks/impacts that can land before the encounter ends.
float expectedDamage(uint spell,uint at){
 float sp=c.spellPower+(at<spiritTapEnd?c.spiritTapSpellPower:0.0);
 float hp=c.holySpellPower+(at<spiritTapEnd?c.spiritTapSpellPower:0.0);
 float shadow=c.damageMultiplier*(at<weavingEnd?1.0+float(weavingStacks)*0.02:1.0);
 float holy=c.holyDamageMultiplier;
 uint left=c.durationMs-at;
 if(spell==0u)return float(min(c.swpTicks,left/3000u))*(127.0+sp*0.20)*shadow*c.instantDamageMultiplier*c.hitChance;
 if(spell==1u)return float(min(3u,left/1000u))*(130.0+sp*0.1667)*shadow*c.flayDamageMultiplier*c.hitChance;
 if(spell==8u)return float(min(8u,left/3000u))*(106.0+sp*0.10)*shadow*c.instantDamageMultiplier*c.hitChance;
 float crit=c.critChance;
 if(spell==3u&&float(at)>=float(c.durationMs)*0.8)crit=min(1.0,crit+c.deathExecuteCritBonus);
 if(spell==2u||spell==3u)return ((spell==2u?485.0:448.0)+sp*(1.5/3.5))*shadow*(spell==3u?c.instantDamageMultiplier:1.0)*(1.0+crit*(c.critMultiplier-1.0))*c.hitChance;
 if(spell==11u)return float(min(6u,left/1000u))*(300.0+(c.arcaneSpellPower+(at<spiritTapEnd?c.spiritTapSpellPower:0.0))*0.167)*c.arcaneDamageMultiplier*c.instantDamageMultiplier*c.arcaneHitChance;
 float holyCrit=1.0+min(1.0,c.holyCritChance+racialCrit())*0.5;
 if(spell==12u)return (289.0+hp*0.143)*holy*holyCrit*c.instantDamageMultiplier*c.holyHitChance;
 if(spell==4u)return (170.0+hp*(2.5/3.5))*holy*holyCrit*c.holyHitChance;
 if(spell==6u)return float(1u+min(2u,left/1000u))*(131.0+hp*0.285)*holy*holyCrit*c.holyHitChance;
 if(spell==5u){
  uint after=left-castTime(5u);
  float direct=(208.0+hp*0.75)*holyCrit;
  float periodic=float(min(5u,after/2000u))*(15.0+hp*0.05);
  float bonus=c.powerInLightBonus*min(10.0,float(after)/1000.0)*(170.0+hp*(2.5/3.5))*holyCrit/(float(castTime(4u))/1000.0);
  return (direct+periodic+bonus)*holy*c.holyHitChance;
 }
 return 0.0;
}
uint efficientFiller(uint at){
 if((c.rotation==0u||c.rotation==14u)&&eligible(1u,at))return 1u;
 if((c.rotation==5u||c.rotation==14u)&&eligible(4u,at))return 4u;
 return 255u;
}
bool shouldPool(uint selected,out uint wake){
 if(c.manaPoolingEnabled==0u||!compositePolicy())return false;
 float spend=selected==255u?0.0:spellCost(selected);
 if(selected!=255u&&(spend==0.0||focusAvailable()))return false;
 uint selectedRank=priorityRank(selected);
 uint last=min(c.durationMs,now+6000u);
 // Resolve the reserve in policy order, including cooldowns/DoTs due within
 // one channel. Do not hold mana for an unaffordable spell indefinitely.
 uint wanted=255u,bestRank=99u;
 for(uint spell=0u;spell<=12u;spell++){
  if(spell==1u||spell==4u||spell==7u)continue;
  uint rank=priorityRank(spell),first=max(now,readyAt(spell));
  if(rank>=selectedRank||rank>=bestRank||first>now+3000u)continue;
  uint at=affordableAt(spell,first,last);
  if(at>last||at<=now)continue;
  uint filler=efficientFiller(at);
  if(filler!=255u&&spellCost(filler)>0.0&&expectedDamage(spell,at)/max(0.0001,spellCost(spell))<expectedDamage(filler,at)/spellCost(filler))continue;
  if(selected==255u||(!focusedAt(at)&&projectedMana(at,spend,now+castTime(selected))<spellCost(spell))){wanted=spell;bestRank=rank;wake=at;}
 }
 return wanted!=255u;
}
void decide(){
 if(now>=c.durationMs||playerHealth<=0.0)return;
 offGCDRacial();
 if(c.powerInfusionEnabled!=0u&&now>=infusionReady){infusionEnd=now+15000u;infusionReady=now+180000u;${detailed?'s.powerInfusionUses++;':''}}
 if(useUtility())return;
 uint spell=selectSpell(now),poolWake=0u;
 if(shouldPool(spell,poolWake)){
  ${detailed?'s.manaPoolWaits++;s.manaPoolTimeMs+=float(poolWake-now);':''}
  enqueue(poolWake,3u,0u);return;
 }
 if(spell==255u){
  uint wake=(now/2000u+1u)*2000u;
  for(uint candidate=0u;candidate<=12u;candidate++){
   if(priorityRank(candidate)==99u)continue;
   uint ready=readyAt(candidate);if(ready>now)wake=min(wake,ready);
  }
  if(c.powerInfusionEnabled!=0u&&infusionReady>now)wake=min(wake,infusionReady);
  if(c.innerFocusEnabled!=0u&&focusReady>now)wake=min(wake,focusReady);
  enqueue(wake,3u,0u);return;
 }
 // Activate off the GCD immediately before the next chosen spell, on cooldown.
 // Consumed once at spell start, including misses and non-critical periodic spells.
 if(spell==9u){countCast(9u);if(hit(9u))damage(9u,c.wandDamage*c.wandDamageMultiplier,false);enqueue(now+hasted(c.wandIntervalMs),3u,0u);return;}
 castEureka=useEureka(spell);
 bool focused=focusAvailable();
 if(focused){focusReady=now+180000u;${detailed?'s.innerFocusUses++;':''}}
 if(spell==12u){countCast(spell);payHoly(225.0*c.instantCostMultiplier,focused);chastiseReady=now+120000u;if(holyDamage(spell,focused?0.25:0.0)&&c.targetControlImmune==0u)rootEnd=now+2000u;enqueue(now+gcd(),3u,0u);return;}
 if(spell==11u){countCast(spell);float cost=focused?0.0:spellCost(spell);s.mana-=cost;s.manaSpent+=cost;if(cost>0.0)fsrEnd=now+5000u;starshardsReady=now+30000u;if(hit(spell)){starshardsTicks=6u;enqueue(now+1000u,1u,spell);}enqueue(now+gcd(),3u,0u);return;}
 if(spell==10u){castFocused=focused;uint castTime=castTime(spell);enqueue(now+castTime,2u,spell);enqueue(now+castTime,3u,0u);return;}
 if(spell==8u){countCast(8u);pay(985.0*c.plagueCostMultiplier*c.instantCostMultiplier,focused);plagueReady=now+60000u;if(hit(8u)){plagueEnd=now+24000u;plagueTicks=8u;plagueGeneration=generation;enqueue(now+3000u,1u,8u);}enqueue(now+gcd(),3u,0u);return;}
 if(spell==4u||spell==5u){castFocused=focused;uint castTime=castTime(spell);enqueue(now+castTime,2u,spell);enqueue(now+castTime,3u,0u);return;}
 if(spell==6u){countCast(spell);payHoly(355.0*c.penanceCostMultiplier,focused);penanceReady=now+12000u;penanceEureka=castEureka;penanceCritBonus=focused?0.25:0.0;holyDamage(spell,penanceCritBonus);enqueue(now+hasted(1000u),1u,spell);enqueue(now+hasted(2000u),1u,spell);enqueue(now+max(gcd(),hasted(2000u)),3u,0u);return;}
 if(spell==7u){countCast(spell);payHoly(750.0*c.instantCostMultiplier,focused||freeNova);if(freeNova){${detailed?'s.freeNovaUses++;':''}freeNova=false;}holyDamage(spell,focused?0.25:0.0);enqueue(now+gcd(),3u,0u);return;}
 if(spell==2u){castFocused=focused;blastReady=now+c.mindBlastCooldownMs;enqueue(now+castTime(2u),2u,spell);enqueue(now+max(gcd(),castTime(2u)),3u,0u);return;}
 countCast(spell);
 if(spell==0u){pay(470.0*c.instantCostMultiplier,focused);if(hit(spell)){painEnd=now+c.swpTicks*3000u;painTicks=c.swpTicks;enqueue(now+3000u,1u,spell);}enqueue(now+gcd(),3u,0u);}
 else if(spell==1u){pay(205.0,focused);if(hit(spell)){flayEureka=castEureka;flayToken++;flaySecondTick=now+hasted(2000u);enqueueToken(now+hasted(1000u),1u,spell,flayToken);enqueueToken(now+hasted(2000u),1u,spell,flayToken);enqueueToken(now+hasted(3000u),1u,spell,flayToken);enqueueToken(now+max(gcd(),hasted(3000u)),3u,0u,flayToken);}else enqueue(now+gcd(),3u,0u);}
 else{pay(340.0*c.instantCostMultiplier,focused);deathReady=now+15000u;directDamage(spell,focused?0.25:0.0);enqueue(now+gcd(),3u,0u);}
}
uint getConfigWord(uint configIndex,uint index){if(numConfigs<=1u)return configWords[index];uvec4 texel=texelFetch(configTex,ivec2(int(index/4u),int(configIndex)),0);return texel[int(index%4u)];}
void simulate(uint lane){
 uint candidate=numConfigs>1u?lane/fightsPerConfig:0u;
 ${Object.entries(CONFIG).map(([key,type],i)=>`c.${key}=${type==='f32'?`uintBitsToFloat(getConfigWord(candidate,${i}u))`:`getConfigWord(candidate,${i}u)`};`).join('\n')}
 ${Object.entries(schema).map(([key,type])=>`s.${key}=${type==='f32'?'0.0':'0u'};`).join('\n')}
 size=0u;serial=0u;now=0u;painEnd=0u;painTicks=0u;blastReady=0u;deathReady=0u;weavingStacks=0u;weavingEnd=0u;fsrEnd=0u;focusReady=0u;castFocused=false;holyFireEnd=0u;holyFireTicks=0u;penanceReady=0u;penanceCritBonus=0.0;infusionEnd=0u;infusionReady=0u;plagueEnd=0u;plagueTicks=0u;plagueReady=0u;generation=0u;plagueGeneration=0u;deaths=0u;spiritTapEnd=0u;flayToken=0u;flaySecondTick=0u;starshardsReady=0u;starshardsTicks=0u;freeNova=false;enemyMana=c.targetMana;s.mana=c.maxMana;
 initRacials();
 seedRandom(numConfigs>1u?lane%fightsPerConfig:lane);enqueue(0u,3u,0u);enqueue(2000u,0u,0u);if(c.targetDeaths>0u)enqueue(c.targetDeathIntervalMs,4u,0u);
 for(uint budget=0u;budget<eventBudget;budget++){
  if(s.done!=0u)break;
  if(size==0u||queue[0].at>c.durationMs){s.done=1u;break;}
  Event e=dequeue();now=e.at;s.events++;
  if(e.channel!=0u&&e.channel!=flayToken)continue;
  if(e.kind==0u){float regen=regenAt(now,fsrEnd);float gain=min(regen,c.maxMana-s.mana);s.mana+=gain;s.manaGained+=gain;enqueue(now+2000u,0u,0u);${detailed?'if(now<hexEnd)s.enemyHealingPrevented+=c.enemyHealingPerTick*0.2;':''}}
  else if(e.kind==4u){
   generation++;deaths++;weaknessEnd=0u;hexEnd=0u;confuseEnd=0u;rootEnd=0u;painEnd=0u;painTicks=0u;holyFireEnd=0u;holyFireTicks=0u;weavingStacks=0u;weavingEnd=0u;enemyMana=c.targetMana;
   if(plagueTicks>0u&&now<plagueEnd&&c.plagueSpreadRadius>0.0&&c.nextTargetDistance<=c.plagueSpreadRadius){plagueGeneration=generation;${detailed?'s.plagueSpreads++;':''}}else{plagueTicks=0u;plagueEnd=0u;}
   if(c.spiritTapChance>0.0&&random01()<c.spiritTapChance){spiritTapEnd=now+15000u;${detailed?'s.spiritTapProcs++;':''}}
   if(deaths<c.targetDeaths)enqueue(now+c.targetDeathIntervalMs,4u,0u);
  }
  else if(e.kind==5u){incomingAttack(e.spell==1u);uint interval=uint(round((e.spell==1u?c.allyAttackInterval:c.incomingAttackInterval)*1000.0));enqueue(now+interval,5u,e.spell);}
  else if(e.kind==6u)racialEvent(e.spell);
  else if(e.kind==1u){
   if(e.spell==8u){if(plagueTicks>0u&&plagueGeneration==generation&&now<=plagueEnd){damage(8u,(106.0+shadowPower()*0.10)*c.damageMultiplier*c.instantDamageMultiplier,false);plagueTicks--;if(plagueTicks>0u)enqueue(now+3000u,1u,8u);}continue;}
   if(e.generation!=generation)continue;
   if(e.spell==11u){damage(11u,(300.0+(c.arcaneSpellPower+(now<spiritTapEnd?c.spiritTapSpellPower:0.0))*0.167)*c.arcaneDamageMultiplier*c.instantDamageMultiplier,false);starshardsTicks--;if(starshardsTicks>0u)enqueue(now+1000u,1u,11u);}
   else if(e.spell==0u){damage(0u,(127.0+shadowPower()*0.20)*c.damageMultiplier*c.instantDamageMultiplier,false);painTicks--;if(painTicks>0u)enqueue(now+3000u,1u,0u);}else if(e.spell==5u){damage(5u,(15.0+holyPower()*0.05)*c.holyDamageMultiplier,false);procFreeNova(true);holyFireTicks--;if(holyFireTicks>0u)enqueue(now+2000u,1u,5u);}
   else if(e.spell==6u)holyDamage(6u,penanceCritBonus);
   else {
    damage(1u,(130.0+shadowPower()*0.1667)*c.damageMultiplier*c.flayDamageMultiplier*flayEureka,false);
    // Only reconsider after tick two. Token invalidation discards the third
    // tick AND its old ready event, preventing duplicate overlapping casts.
    if(c.clipMindFlayEnabled!=0u&&compositePolicy()&&now==flaySecondTick){
     uint next=selectSpell(now);
     if(next!=255u&&priorityRank(next)<priorityRank(1u)){
      flayToken++;${detailed?'s.flayClips++;':''}enqueue(now,3u,0u);
     }
    }
   }}
  else if(e.kind==2u){
   if(e.spell==10u){pay(270.0,castFocused);countCast(10u);if(e.generation==generation&&hit(10u)){float burned=min(enemyMana,mix(738.0,780.0,random01()));enemyMana-=burned;${detailed?'s.targetManaBurned+=burned;':''}damage(10u,burned*0.5*c.damageMultiplier,false);}else if(e.generation!=generation)countMiss(10u);}
   else if(e.spell==2u){pay(350.0,castFocused);countCast(2u);if(e.generation==generation)directDamage(2u,castFocused?0.25:0.0);else countMiss(2u);}
   else{payHoly((e.spell==4u?280.0:255.0)*c.holyCostMultiplier,castFocused);countCast(e.spell);bool landed=false;if(e.generation==generation)landed=holyDamage(e.spell,castFocused?0.25:0.0);else countMiss(e.spell);
    if(e.spell==5u&&landed){holyFireEnd=now+10000u;holyFireTicks=5u;enqueue(now+2000u,1u,5u);}}
   castFocused=false;}
  else decide();
  if(playerHealth<=0.0){${detailed?'s.deathTimeMs=now;':''}s.done=1u;break;}
 }
 ${detailed?'s.playerHealth=playerHealth;s.allyHealth=allyHealth;s.allyWeakenedSoulRemainingMs=weakenedSoulEnd>now?weakenedSoulEnd-now:0u;':''}
 if(s.done==0u)s.done=(size==0u||queue[0].at>c.durationMs)?1u:3u;
}
uint stateWord(uint index){switch(index){${Object.entries(schema).map(([key,type],i)=>`case ${i}u:return ${type==='f32'?`floatBitsToUint(s.${key})`:`s.${key}`};`).join('\n')}default:return 0u;}}
uvec4 report(uint index){return uvec4(stateWord(index),stateWord(index+1u),stateWord(index+2u),stateWord(index+3u));}
void main(){
 uint stripes=${detailed?COMPACT_STRIPES:1}u;
 uint x=uint(gl_FragCoord.x),y=uint(gl_FragCoord.y);
 uint lane=(y/stripes)*gridWidth+x;
 if(lane>=count){report0=uvec4(0u);report1=uvec4(0u);report2=uvec4(0u);report3=uvec4(0u);return;}
 simulate(offset+lane);uint word=(y%stripes)*16u;
 report0=report(word);report1=report(word+4u);report2=report(word+8u);report3=report(word+12u);
}`;
}
export const FRAGMENT = buildFragment(true);
export const FAST_FRAGMENT = buildFragment(false);
