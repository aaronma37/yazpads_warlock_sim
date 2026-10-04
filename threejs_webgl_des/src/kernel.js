import { CONFIG, STATE, STATE_WORDS, HEAP_CAPACITY, APL_HEADER_OFFSET, APL_PARAM_OFFSET, MAX_APL_RULES } from './model.js';
const type=t=>t==='u32'?'uint':'float';
const structure=(name,schema)=>`struct ${name} { ${Object.entries(schema).map(([k,t])=>`${type(t)} ${k};`).join('\n')} };`;
const word=(name,t)=>t==='f32'?`floatBitsToUint(s.${name})`:`s.${name}`;
export const VERTEX=`precision highp float;
in vec3 position;
void main(){ gl_Position=vec4(position,1.0); }`;

// Complete Table-Driven APL Bytecode VM Discrete Event Simulation per fragment.
export const FRAGMENT = `
precision highp float;
precision highp int;
precision highp usampler2D;
layout(location=0) out uvec4 report0;
layout(location=1) out uvec4 report1;
layout(location=2) out uvec4 report2;
layout(location=3) out uvec4 report3;
uniform uint configWords[${Object.keys(CONFIG).length}];
uniform highp usampler2D configTex;
uniform uint numConfigs;
uniform uint fightsPerConfig;
uniform uint seed;
uniform uint offset;
uniform uint count;
uniform uint gridWidth;
uniform uint mode;
uniform uint eventBudget;
${structure('FightConfig',CONFIG)}
${structure('FightState',STATE)}
struct Event { uint at; uint kind; uint spell; uint generation; };
FightConfig c;
FightState s;
Event queue[${HEAP_CAPACITY}];
uvec2 rng[4];
float eventDamage;
uint eventFlags;
Event lastEvent;
uint currentCfgIdx;

#define APL_HEADER_OFFSET ${APL_HEADER_OFFSET}u
#define APL_PARAM_OFFSET ${APL_PARAM_OFFSET}u
#define MAX_APL_RULES ${MAX_APL_RULES}u

uint getConfigWord(uint cfgIdx, uint wordIdx) {
  if (numConfigs <= 1u) return configWords[wordIdx];
  int texelX = int(wordIdx / 4u);
  uint comp = wordIdx % 4u;
  uvec4 val = texelFetch(configTex, ivec2(texelX, int(cfgIdx)), 0);
  if (comp == 0u) return val.r;
  if (comp == 1u) return val.g;
  if (comp == 2u) return val.b;
  return val.a;
}

uvec2 add64(uvec2 a,uvec2 b){uint low=a.x+b.x;return uvec2(low,a.y+b.y+uint(low<a.x));}
uint highMultiply(uint a,uint b){
 uint a0=a&65535u,a1=a>>16u,b0=b&65535u,b1=b>>16u;
 uint w0=a0*b0,t=a1*b0+(w0>>16u),w1=(t&65535u)+a0*b1;
 return a1*b1+(t>>16u)+(w1>>16u);
}
uvec2 mul64(uvec2 a,uvec2 b){return uvec2(a.x*b.x,highMultiply(a.x,b.x)+a.x*b.y+a.y*b.x);}
uvec2 shr64(uvec2 a,uint n){if(n>=32u)return uvec2(a.y>>(n-32u),0u);return uvec2((a.x>>n)|(a.y<<(32u-n)),a.y>>n);}
uvec2 rot64(uvec2 a,uint n){uvec2 v=a;uint k=n;if(n>=32u){v=a.yx;k-=32u;}return (v<<k)|(v.yx>>(32u-k));}
void seedRandom(uint index){
 uvec2 x=add64(uvec2(seed,0u),uvec2(index,0u));
 if(all(equal(x,uvec2(0u))))x=uvec2(0xbeefcafeu,0x0000deadu);
 for(int i=0;i<4;i++){
  x=add64(x,uvec2(0x7f4a7c15u,0x9e3779b9u));
  uvec2 z=mul64(x^shr64(x,30u),uvec2(0x1ce4e5b9u,0xbf58476du));
  z=mul64(z^shr64(z,27u),uvec2(0x133111ebu,0x94d049bbu));rng[i]=z^shr64(z,31u);
 }
}
float random01(){
 uvec2 value=mul64(rot64(mul64(rng[1],uvec2(5u,0u)),7u),uvec2(9u,0u));
 uvec2 t=uvec2(rng[1].x<<17u,(rng[1].y<<17u)|(rng[1].x>>15u));
 rng[2]^=rng[0];rng[3]^=rng[1];rng[1]^=rng[2];rng[0]^=rng[3];rng[2]^=t;rng[3]=rot64(rng[3],45u);
 s.rngCalls++;return float(value.y>>8u)*(1.0/16777216.0);
}
void enqueue(uint at,uint kind,uint spell,uint generation){
 if(s.size>=${HEAP_CAPACITY}u){s.done=2u;return;}
 uint i=s.size++;s.highWater=max(s.highWater,s.size);Event e=Event(at,kind,spell,generation);
 for(int guard=0;guard<${HEAP_CAPACITY};guard++){
  if(i==0u)break;uint parent=(i-1u)/2u;if(e.at>=queue[parent].at)break;queue[i]=queue[parent];i=parent;
 }
 queue[i]=e;
}
Event dequeue(){
 Event e=queue[0];s.size--;
 if(s.size>0u){queue[0]=queue[s.size];uint i=0u;
  for(int guard=0;guard<${HEAP_CAPACITY};guard++){
   uint left=2u*i+1u,right=left+1u,smallest=i;
   if(left<s.size){if(queue[left].at<queue[smallest].at)smallest=left;}
   if(right<s.size){if(queue[right].at<queue[smallest].at)smallest=right;}
   if(smallest==i)break;Event swap=queue[i];queue[i]=queue[smallest];queue[smallest]=swap;i=smallest;
  }
 }return e;
}
void countCast(uint spell){switch(spell){${Array.from({length:6},(_,i)=>`case ${i}u:s.casts${i}++;break;`).join('')}}}
void countMiss(uint spell){eventFlags|=2u;switch(spell){${Array.from({length:6},(_,i)=>`case ${i}u:s.misses${i}++;break;`).join('')}}}
void damage(uint spell,float amount,bool crit){
 s.total+=amount;eventDamage+=amount;if(crit)eventFlags|=1u;
 switch(spell){${Array.from({length:6},(_,i)=>`case ${i}u:s.damage${i}+=amount;s.hits${i}++;s.crits${i}+=uint(crit);break;`).join('')}}
}
float isbMultiplier(){
 if(s.now>=s.isbEnd)return 1.0;
 s.isbConsumed++;return 1.0+c.isb;
}
float resistanceMultiplier(){
 float resistance=c.resistance-c.penetration;
 if(resistance<0.0&&c.piercing!=0u)return 1.0-resistance*0.00575;
 if(resistance<=0.0||c.partial==0u)return 1.0;
 float roll=random01();if(roll<0.76)return 1.0;if(roll<0.95)return 0.75;if(roll<0.99)return 0.5;return 0.25;
}
float currentShadowMult(){return c.shadowMult*(s.now<s.snfShadowEnd?(1.0+c.snfBonus):1.0);}
float currentFireMult(){return c.fireMult*(s.now<s.snfFireEnd?(1.0+c.snfBonus):1.0);}
float currentPower(){return c.power+(s.now<s.trinketEnd?c.trinketSP:0.0);}
float cost(uint spell){switch(spell){
 case 0u:return c.boltCost;case 1u:return c.corrCost;case 2u:return 215.0;
 case 3u:return c.immCost;case 4u:return 325.0;case 13u:return 335.0;default:return 168.0;}}
void spend(uint spell){float amount=spell==4u?355.0:cost(spell);s.mana-=amount;s.spent+=amount;countCast(spell==13u?3u:spell);}
void applyDot(uint spell){
 if(random01()>=c.hit){countMiss(spell);return;}
 if(spell==1u){s.corrTicks=6u;s.corrGen++;s.corrEnd=s.now+18000000u;enqueue(s.now+3000000u,3u,spell,s.corrGen);}
 if(spell==2u){s.agonyTicks=12u;s.agonyGen++;s.agonyEnd=s.now+24000000u;enqueue(s.now+2000000u,3u,spell,s.agonyGen);}
}
void immolateImpact(){
 if(random01()>=c.hit){countMiss(3u);return;}
 bool crit=random01()<c.fireCrit;
 float amount=(c.immDirect+0.20*currentPower())*(crit?c.directCrit:1.0)*currentFireMult()*(1.0+c.afBonus+c.aftermathBonus);
 amount*=resistanceMultiplier();damage(3u,amount,crit);
 s.immTicks=5u;s.immGen++;s.immEnd=s.now+15000000u;enqueue(s.now+3000000u,3u,3u,s.immGen);
}
void soulFireImpact(){
 if(random01()>=c.hit){countMiss(3u);return;}
 float roll=random01(),p=currentPower();
 float amount=(383.0+roll*96.0+1.0*p)*currentFireMult()*(1.0+c.afBonus);
 bool crit=random01()<c.fireCrit;
 if(crit)amount*=c.directCrit;
 amount*=resistanceMultiplier();damage(3u,amount,crit);
}
void directImpact(uint spell){
 if(random01()>=c.hit){countMiss(spell);return;}
 float roll=random01(),amount,p=currentPower();
 bool crit=false;
 if(spell==0u){
  amount=(c.boltMin+roll*(c.boltMax-c.boltMin)+(3.0/3.5)*p)*currentShadowMult()*(1.0+c.shadowMasteryBonus+c.afBonus);
  float fightProg=float(s.now)/max(1.0,float(c.end));
  if((1.0-fightProg)<=0.35&&c.decimation!=0u)amount*=(1.0+0.03*float(c.decimationRank));
  amount*=isbMultiplier();
  crit=random01()<c.shadowCrit;
  if(crit){amount*=c.directCrit;if(c.isb>0.0){s.isbEnd=s.now+12000000u;s.isbProcs++;}}
 }
 else if(spell==4u){
  amount=(201.0+roll*32.0+(2.5/3.5)*p)*(s.immTicks>0u?1.25:1.0)*currentFireMult()*(1.0+c.afBonus);
  crit=random01()<c.fireCrit;
  if(crit)amount*=c.directCrit;
 }
 else if(spell==5u){
  amount=(108.0+roll*19.0+(1.5/3.5)*p);
  float fightProg=float(s.now)/max(1.0,float(c.end));
  if((1.0-fightProg)<=0.35&&c.decimation!=0u)amount*=(1.0+0.03*float(c.decimationRank));
  amount*=currentFireMult()*(1.0+c.afBonus);
  crit=random01()<(c.fireCrit+c.afBonus);
  if(crit)amount*=c.directCrit;
 }
 else if(spell==13u){
  soulFireImpact();
  return;
 }
 else{amount=0.0;}
 amount*=resistanceMultiplier();damage(spell,amount,crit);
}
void tick(Event e){
 float amount,p=currentPower();uint remaining=0u,interval=3000000u;
 if(e.spell==1u){
  if(e.generation!=s.corrGen||s.corrTicks==0u)return;
  s.corrTicks--;remaining=s.corrTicks;
  amount=(c.corrBase+p*1.2/6.0)*c.corrMultiplier*(s.now<s.snfShadowEnd?(1.0+c.snfBonus):1.0)*(s.now<s.drainHopeEnd?1.10:1.0);
  bool crit=random01()<c.shadowCrit;if(crit)amount*=c.dotCrit;
  amount*=isbMultiplier();damage(1u,amount,crit);
  if(c.nightfall>0.0){if(random01()<c.nightfall){s.trance=1u;s.tranceEnd=s.now+10000000u;s.procs++;enqueue(s.tranceEnd,8u,0u,0u);}}
  if(remaining>0u)enqueue(s.now+3000000u,3u,1u,e.generation);
  return;
 }else if(e.spell==2u){
  if(e.generation!=s.agonyGen||s.agonyTicks==0u)return;
  s.agonyTicks--;remaining=s.agonyTicks;interval=2000000u;
  uint index=12u-remaining;float ramp=index<=4u?0.5:(index<=8u?1.0:1.5);
  amount=(46.0+p*1.596/12.0)*ramp*currentShadowMult()*(1.0+c.shadowMasteryBonus+c.maledictionBonus)*(s.now<s.drainHopeEnd?1.10:1.0);
  bool crit=random01()<c.shadowCrit;if(crit)amount*=c.dotCrit;
  amount*=isbMultiplier();damage(2u,amount,crit);
  if(remaining>0u)enqueue(s.now+interval,3u,2u,e.generation);
  return;
 }else if(e.spell==3u){
  if(e.generation!=s.immGen||s.immTicks==0u)return;
  s.immTicks--;remaining=s.immTicks;amount=(c.immTick+0.13*p)*currentFireMult()*(1.0+c.afBonus+c.maledictionBonus);
  bool crit=random01()<c.fireCrit;if(crit)amount*=c.directCrit;
  damage(3u,amount,crit);
  if(remaining>0u)enqueue(s.now+3000000u,3u,3u,e.generation);
  return;
 }else if(e.spell==15u){
  if(e.generation!=s.siphonGen||s.siphonTicks==0u)return;
  s.siphonTicks--;remaining=s.siphonTicks;
  amount=(41.0+0.05*p)*currentShadowMult()*(1.0+c.shadowMasteryBonus+c.maledictionBonus)*(s.now<s.drainHopeEnd?1.10:1.0);
  bool crit=random01()<c.shadowCrit;if(crit)amount*=c.dotCrit;
  amount*=isbMultiplier();damage(1u,amount,crit);
  if(remaining>0u)enqueue(s.now+3000000u,3u,15u,e.generation);
  return;
 }else if(e.spell==16u){
  uint affCount=(s.corrTicks>0u?1u:0u)+(s.agonyTicks>0u?1u:0u)+(s.siphonTicks>0u?1u:0u);
  float affBonus=c.shadowMasteryBonus+c.maledictionBonus+c.improvedDrainsBonus+c.soulSiphonBonus*float(min(3u,affCount));
  amount=(36.0+0.143*p)*currentShadowMult()*(1.0+affBonus);
  bool crit=random01()<c.shadowCrit;if(crit)amount*=c.dotCrit;
  amount*=isbMultiplier();amount*=resistanceMultiplier();
  damage(1u,amount,crit);
  if(c.nightfall>0.0){if(random01()<c.nightfall){s.trance=1u;s.tranceEnd=s.now+10000000u;s.procs++;enqueue(s.tranceEnd,8u,0u,0u);}}
  return;
 }else if(e.spell==20u){
  if(e.generation!=s.agonyGen||s.doomActive==0u)return;
  s.doomActive=0u;amount=(1742.0+4.0*p)*currentShadowMult()*(1.0+c.shadowMasteryBonus+c.maledictionBonus)*(s.now<s.drainHopeEnd?1.10:1.0);
  bool crit=random01()<c.shadowCrit;if(crit)amount*=c.dotCrit;
  amount*=isbMultiplier();amount*=resistanceMultiplier();
  damage(2u,amount,crit);return;
 }else{return;}
}
void gcd(){s.ready=s.now+1500000u;enqueue(s.ready,5u,0u,0u);}
void tap(){s.mana=min(c.maxMana,s.mana+c.tapGain);s.gained+=c.tapGain;s.taps++;if(c.petChoice!=0u&&c.demonicEnergies>0.0)s.petMana=min(1500.0,s.petMana+c.tapGain*0.5*c.demonicEnergies);gcd();}
void beginCast(uint spell){
 if(spell==2u||(spell==1u&&c.corrCast==0u)){spend(spell);applyDot(spell);gcd();return;}
 uint duration=2500000u;
 if(spell==1u)duration=c.corrCast;
 else if(spell==3u)duration=uint(max(1.0,2.0-0.1*float(c.baneRank))*1000000.0);
 else if(spell==5u)duration=1500000u;
 else if(spell==4u)duration=uint(max(1.0,2.5-0.1*float(c.baneRank))*1000000.0);
 else if(spell==13u){
  float castTime=6.0-0.4*float(c.baneRank);
  if(s.decimationEnd>s.now)castTime*=1.0-0.20*float(c.decimationRank);
  duration=uint(max(0.5,castTime)*1000000.0);
 }
 else if(spell==0u)duration=uint(max(1.0,3.0-0.1*float(c.baneRank))*1000000.0);
 s.casting=1u;enqueue(s.now+duration,1u,spell,0u);gcd();
}
void checkTrinket(){
 if(c.trinketSP>0.0&&s.now>=s.trinketReady){
  s.trinketEnd=s.now+c.trinketDuration;
  s.trinketReady=s.now+c.trinketCD;
  enqueue(s.trinketEnd,11u,0u,0u);
 }
}
void castConflagrate(){
 s.conflagReady=s.now+10000000u;s.mana-=265.0;s.spent+=265.0;countCast(3u);
 if(random01()<c.hit){
  float roll=random01(),p=currentPower();
  float amount=(306.0+roll*68.0+(1.5/3.5)*p)*currentFireMult()*(1.0+c.afBonus);
  bool crit=random01()<(c.fireCrit+c.fnbCrit);
  if(crit)amount*=c.directCrit;
  amount*=resistanceMultiplier();
  damage(3u,amount,crit);
  if(c.snfBonus>0.0||c.snfChance>0.0){s.snfShadowEnd=s.now+20000000u;}
 }else{countMiss(3u);}
 if(c.snfChance>0.0){if(random01()>=c.snfChance){s.immTicks=0u;s.immGen++;}}else{s.immTicks=0u;s.immGen++;}
 gcd();
}
void castShadowburn(){
 s.shadowburnReady=s.now+15000000u;s.mana-=365.0;s.spent+=365.0;countCast(0u);
 if(random01()<c.hit){
  float roll=random01(),p=currentPower();
  float amount=(259.0+roll*30.0+(1.5/3.5)*p)*currentShadowMult()*(1.0+c.shadowMasteryBonus+c.afBonus);
  amount*=isbMultiplier();
  bool crit=random01()<c.shadowCrit;
  if(crit)amount*=c.directCrit;
  amount*=resistanceMultiplier();
  damage(0u,amount,crit);
  if(c.snfBonus>0.0||c.snfChance>0.0){s.snfFireEnd=s.now+20000000u;}
 }else{countMiss(0u);}
 gcd();
}
void castDoom(){
 s.mana-=300.0;s.spent+=300.0;countCast(2u);
 if(random01()<c.hit){
  s.doomActive=1u;s.agonyGen++;enqueue(s.now+60000000u,3u,20u,s.agonyGen);
 }else{countMiss(2u);}
 gcd();
}
void decide(){
 if(s.casting!=0u||s.now<s.ready)return;
 checkTrinket();

 float fightProgress=float(s.now)/max(1.0,float(c.end));
 float targetHpPct=max(0.0,(1.0-fightProgress)*100.0);
 float playerManaPct=(s.mana/c.maxMana)*100.0;

 for(uint r=0u;r<MAX_APL_RULES;r++){
  uint header=getConfigWord(currentCfgIdx,APL_HEADER_OFFSET+r);
  if(header==0u)break;

  uint enabled=(header>>24u)&0xFFu;
  if(enabled==0u)continue;

  uint action=header&0xFFu;
  uint cond=(header>>8u)&0xFFu;
  uint targetSpell=(header>>16u)&0xFFu;
  float param=uintBitsToFloat(getConfigWord(currentCfgIdx,APL_PARAM_OFFSET+r));

  bool condPass=false;
  if(cond==0u){ // ALWAYS
   condPass=true;
  }else if(cond==1u){ // MANA_LE
   condPass=(playerManaPct<=param);
  }else if(cond==2u){ // MANA_GE
   condPass=(playerManaPct>=param);
  }else if(cond==3u){ // TARGET_HP_LE
   condPass=(targetHpPct<=param);
  }else if(cond==4u){ // TARGET_HP_GE
   condPass=(targetHpPct>=param);
  }else if(cond==5u||cond==23u){ // DOT_REM_LE / DOT_REM_LT
   float remSec=0.0;
   if(targetSpell==1u)remSec=(s.corrTicks>0u&&s.corrEnd>s.now)?float(s.corrEnd-s.now)*0.000001:0.0;
   else if(targetSpell==2u)remSec=(s.agonyTicks>0u&&s.agonyEnd>s.now)?float(s.agonyEnd-s.now)*0.000001:0.0;
   else if(targetSpell==22u)remSec=(s.doomActive!=0u?60.0:0.0);
   else if(targetSpell==3u)remSec=(s.immTicks>0u&&s.immEnd>s.now)?float(s.immEnd-s.now)*0.000001:0.0;
   else if(targetSpell==15u)remSec=(s.siphonTicks>0u&&s.siphonEnd>s.now)?float(s.siphonEnd-s.now)*0.000001:0.0;
   condPass=(cond==23u?remSec<param:remSec<=param);
  }else if(cond==6u){ // FIGHT_TIME_GE
   condPass=(c.end>s.now)&&(float(c.end-s.now)>param*1000000.0);
  }else if(cond==7u){ // FIGHT_TIME_LE
   condPass=(c.end>=s.now)&&(float(c.end-s.now)<=param*1000000.0);
  }else if(cond==8u){ // SHADOW_TRANCE
   condPass=(s.trance!=0u);
  }else if(cond==9u){ // DECIMATION_ACTIVE
   condPass=(c.decimation!=0u)&&(targetHpPct<=param)&&(s.decimationEnd>s.now);
  }else if(cond==10u){ // DECIMATION_INACTIVE
   condPass=(c.decimation!=0u)&&(targetHpPct<=param)&&(s.decimationEnd<=s.now);
  }else if(cond==11u){ // DEMONIC_BRAND_MISSING
   condPass=(s.brandCharges==0u||s.brandEnd<=s.now);
  }else if(cond==12u){ // DOOM_MISSING
   condPass=(s.doomActive==0u);
  }else if(cond==13u){ // ISB_ACTIVE
   condPass=(s.now<s.isbEnd);
  }else if(cond==14u){ // FIGHT_TIME_GE and named DoT missing
   bool missing=(targetSpell==1u?s.corrTicks==0u:targetSpell==2u?s.agonyTicks==0u:targetSpell==22u?s.doomActive==0u:targetSpell==3u?s.immTicks==0u:targetSpell==15u?s.siphonTicks==0u:false);
   condPass=(c.end>=s.now&&float(c.end-s.now)>=param*1000000.0&&missing);
  }else if(cond==15u){ // Nightfall talent enabled and named DoT missing
   bool missing=(targetSpell==1u?s.corrTicks==0u:targetSpell==2u?s.agonyTicks==0u:targetSpell==22u?s.doomActive==0u:targetSpell==3u?s.immTicks==0u:targetSpell==15u?s.siphonTicks==0u:false);
   condPass=(c.nightfall>0.0&&missing);
  }else if(cond==16u){ // Fight time threshold and both Doom/Agony missing
   condPass=(c.end>=s.now&&float(c.end-s.now)>=param*1000000.0&&s.doomActive==0u&&s.agonyTicks==0u);
  }else if(cond==17u){condPass=(playerManaPct<param);
  }else if(cond==18u){condPass=(playerManaPct>param);
  }else if(cond==19u){condPass=(targetHpPct<param);
  }else if(cond==20u){condPass=(targetHpPct>param);
  }else if(cond==21u){condPass=(c.end>s.now&&float(c.end-s.now)>param*1000000.0);
  }else if(cond==22u){condPass=(c.end>=s.now&&float(c.end-s.now)<param*1000000.0);
  }else{
   condPass=true;
  }

  if(!condPass)continue;

  // Execute Action
  if(action==1u){ // LIFE_TAP
   tap();return;
  }else if(action==2u){ // NIGHTFALL_SHADOW_BOLT
   if(s.trance!=0u){
    if(s.mana>=cost(0u)){
     s.trance=0u;spend(0u);enqueue(s.now+c.travel,2u,0u,0u);gcd();return;
    }
   }
  }else if(action==3u||action==5u||action==13u){ // SEARING_PAIN (Decimation / Brand / Filler)
   if(s.mana>=cost(5u)){beginCast(5u);return;}
  }else if(action==4u){ // DECIMATION_SOUL_FIRE
   if(s.now>=s.soulFireReady&&s.mana>=335.0){beginCast(13u);return;}
  }else if(action==6u){ // CORRUPTION
   if(c.corr!=0u){
    if(s.mana>=cost(1u)){beginCast(1u);return;}
   }
  }else if(action==7u){ // CURSE_OF_DOOM
   if(c.curseOfDoom!=0u&&s.doomActive==0u&&s.agonyTicks==0u){
    if(s.mana>=300.0){castDoom();return;}
   }
  }else if(action==8u){ // CURSE_OF_AGONY
   if(c.agony!=0u&&s.doomActive==0u){
    if(s.mana>=cost(2u)){beginCast(2u);return;}
   }
  }else if(action==9u){ // IMMOLATE
   if(c.immolate!=0u){
    if(s.mana>=cost(3u)){beginCast(3u);return;}
   }
  }else if(action==10u){ // CONFLAGRATE
   if(c.conflagrate!=0u&&s.immTicks>0u&&s.now>=s.conflagReady){
    if(s.mana>=265.0){castConflagrate();return;}
   }
  }else if(action==17u){ // SIPHON_LIFE
   if(c.siphonLife!=0u){
    if(s.mana>=365.0){
     s.mana-=365.0;s.spent+=365.0;countCast(1u);
     if(random01()<c.hit){s.siphonTicks=10u;s.siphonGen++;s.siphonEnd=s.now+30000000u;enqueue(s.now+3000000u,3u,15u,s.siphonGen);}else{countMiss(1u);}
     gcd();return;
    }
   }
  }else if(action==18u){ // DRAIN_HOPE (Wrack)
   if(c.drainHope!=0u&&s.now>=s.drainHopeReady){
    if(s.mana>=240.0){
     s.mana-=240.0;s.spent+=240.0;countCast(1u);
     s.drainHopeEnd=s.now+6000000u;s.drainHopeReady=s.now+6000000u;
     for(uint i=1u;i<=6u;i++)enqueue(s.now+i*1000000u,3u,16u,0u);
     s.ready=s.now+6000000u;enqueue(s.ready,5u,0u,0u);return;
    }
   }
  }else if(action==11u){ // SHADOWBURN
   if(c.shadowburn!=0u&&s.now>=s.shadowburnReady){
    if(s.mana>=365.0){castShadowburn();return;}
   }
  }else if(action==12u){ // INCINERATE_FILLER
   if(c.incinerate!=0u){if(s.mana>=cost(4u)){beginCast(4u);return;}else{tap();return;}}
  }else if(action==16u||action==14u||action==15u){ // SHADOW_BOLT_FILLER
   if(s.mana>=cost(0u)){beginCast(0u);return;}else{tap();return;}
  }
 }

 // Fallback
 if(s.mana>=cost(c.filler)){beginCast(c.filler);return;}
 tap();
}
void advance(){
 if(s.size==0u){s.done=3u;return;}
 Event e=dequeue();lastEvent=e;
 if(e.at<s.now){s.done=4u;return;}
 s.now=e.at;s.events++;eventDamage=0.0;eventFlags=0u;
 switch(e.kind){
 case 13u:s.done=1u;break;
 case 1u:
  s.casting=0u;spend(e.spell);
  if(e.spell==1u)applyDot(1u);
  else if(e.spell==3u)immolateImpact();
  else if(e.spell==13u){float sfCD=60.0*(1.0-0.45*float(c.decimationRank));s.soulFireReady=s.now+uint(sfCD*1000000.0);enqueue(s.now+c.travel,2u,13u,0u);}
  else enqueue(s.now+c.travel,2u,e.spell,0u);
  if(c.decimation!=0u&&(e.spell==0u||e.spell==5u)){
   float fightProg=float(s.now)/max(1.0,float(c.end));
   if((1.0-fightProg)<=0.35)s.decimationEnd=s.now+10000000u;
  }
  if(e.spell==5u&&c.demonicBrand!=0u){s.brandCharges=c.demonicBrandRank*2u;s.brandEnd=s.now+10000000u;}
  decide();break;
 case 2u:directImpact(e.spell);break;
 case 3u:tick(e);break;
 case 5u:decide();break;
 case 6u:
  if(e.spell==100u){ // Imp Firebolt
   uint delay=2000000u;
   if(s.petMana>=115.0){
    s.petCasts++;
    s.petSpellCasts++;
    s.petMana-=115.0;
    if(random01()<c.hit){
     s.petSpellHits++;
     float dmg=(44.0+(2.0/3.5)*c.petSP)*c.petFireboltMult;
     bool crit=random01()<c.fireCrit;if(crit){dmg*=1.5;s.petSpellCrits++;}dmg*=resistanceMultiplier();
     if(c.demonicBrand!=0u&&s.brandCharges>0u&&s.now<s.brandEnd){
      s.brandCharges--;
      float brandDmg=(65.0+random01()*3.0+0.078*currentPower())*c.brandMult;
      brandDmg*=resistanceMultiplier();dmg+=brandDmg;s.petBrandDamage+=brandDmg;
     }
     s.total+=dmg;eventDamage+=dmg;s.petDamage+=dmg;s.petSpellDamage+=dmg;if(crit)eventFlags|=1u;
    }else{
     s.petSpellMisses++;
    }
   }else{
    delay=1000000u;
   }
   if(s.now+delay<c.end)enqueue(s.now+delay,6u,100u,0u);
  }else if(e.spell==200u){ // Succubus Melee
   s.petCasts++;
   s.petMeleeCasts++;
   float roll=random01()*100.0;
   if(roll>=14.5){
    s.petMeleeHits++;
    float dmg=(101.0+(c.petAP/14.0)*2.0)*c.petMeleeMult;
    bool glance=(roll<54.5);
    bool crit=(!glance&&roll<(54.5+max(0.0,c.crit*100.0+2.72)));
    if(glance)dmg*=0.65;
    if(crit){dmg*=2.0;s.petMeleeCrits++;}
    if(c.demonicBrand!=0u&&s.brandCharges>0u&&s.now<s.brandEnd){
     s.brandCharges--;
      float brandDmg=(65.0+random01()*3.0+0.078*currentPower())*c.brandMult;
      brandDmg*=resistanceMultiplier();dmg+=brandDmg;s.petBrandDamage+=brandDmg;
    }
    s.total+=dmg;eventDamage+=dmg;s.petDamage+=dmg;s.petMeleeDamage+=dmg;if(crit)eventFlags|=1u;
   }else{
    s.petMeleeMisses++;
   }
   if(s.now+2000000u<c.end)enqueue(s.now+2000000u,6u,200u,0u);
  }else if(e.spell==201u){ // Succubus Lash of Pain
   uint delay=12000000u;
   if(s.petMana>=160.0){
    s.petCasts++;
    s.petSpellCasts++;
    s.petMana-=160.0;
    if(random01()<c.hit){
     s.petSpellHits++;
     float dmg=(50.0+(1.5/3.5)*c.petSP)*c.petLashMult;
     bool crit=random01()<c.shadowCrit;if(crit){dmg*=1.5;s.petSpellCrits++;}dmg*=resistanceMultiplier();
     if(c.demonicBrand!=0u&&s.brandCharges>0u&&s.now<s.brandEnd){
      s.brandCharges--;
      float brandDmg=(65.0+random01()*3.0+0.078*currentPower())*c.brandMult;
      brandDmg*=resistanceMultiplier();dmg+=brandDmg;s.petBrandDamage+=brandDmg;
     }
     s.total+=dmg;eventDamage+=dmg;s.petDamage+=dmg;s.petSpellDamage+=dmg;if(crit)eventFlags|=1u;
    }else{
     s.petSpellMisses++;
    }
   }else{
    delay=1500000u;
   }
   if(s.now+delay<c.end)enqueue(s.now+delay,6u,201u,0u);
  }
  break;
 case 8u:if(s.now>=s.tranceEnd)s.trance=0u;break;
 case 10u:s.mana=min(c.maxMana,s.mana+c.mp5);s.gained+=c.mp5;if(c.petChoice!=0u)s.petMana=min(1500.0,s.petMana+45.0);enqueue(s.now+5000000u,10u,0u,0u);break;
 case 11u:break;
 case 12u:s.trinketEnd=s.now+c.trinketDuration;enqueue(s.trinketEnd,11u,0u,0u);if(s.now+c.trinketCD<=c.end)enqueue(s.now+c.trinketCD,12u,0u,0u);break;
 default:s.done=5u;break;
 }
}
uint stateWord(uint index){switch(index){${Object.entries(STATE).map(([k,t],i)=>`case ${i}u:return ${word(k,t)};`).join('\n')}default:return 0u;}}
uint compactWord(uint index){switch(index){
 ${['total','mana','spent','gained'].map((k,i)=>`case ${i}u:return floatBitsToUint(s.${k});`).join('\n')}
 ${['done','now','events','highWater','taps','procs','isbProcs','isbConsumed'].map((k,i)=>`case ${i+4}u:return s.${k};`).join('\n')}
 ${Array.from({length:6},(_,i)=>`case ${12+i*3}u:return floatBitsToUint(s.damage${i});case ${13+i*3}u:return s.casts${i}|(s.hits${i}<<16u);case ${14+i*3}u:return s.crits${i}|(s.misses${i}<<16u);`).join('\n')}
 case 30u:return floatBitsToUint(s.petMeleeDamage);
 case 31u:return s.petMeleeCasts|(s.petMeleeHits<<16u);
 case 32u:return s.petMeleeCrits|(s.petMeleeMisses<<16u);
 case 33u:return floatBitsToUint(s.petSpellDamage);
 case 34u:return s.petSpellCasts|(s.petSpellHits<<16u);
 case 35u:return s.petSpellCrits|(s.petSpellMisses<<16u);
 case 36u:return floatBitsToUint(s.petBrandDamage);
 case 37u:return floatBitsToUint(s.petDamage);
 case 38u:return s.petCasts;
 case 39u:return s.rngCalls;
 default:return 0u;}}
uint outputWord(uint index){return mode==1u?stateWord(index):compactWord(index);}
uvec4 outputFour(uint index){return uvec4(outputWord(index),outputWord(index+1u),outputWord(index+2u),outputWord(index+3u));}
void main(){
 uint x=uint(gl_FragCoord.x),y=uint(gl_FragCoord.y);
 uint lane,stripe;
 if(mode==0u){
  uint simX=x,simY=y/3u;
  stripe=y%3u;
  lane=simY*gridWidth+simX;
 }else if(mode==1u){
  lane=0u;stripe=y;
 }else{
  lane=x;stripe=0u;
 }
 report0=uvec4(0u);report1=uvec4(0u);report2=uvec4(0u);report3=uvec4(0u);
 if(lane>=count&&mode==0u)return;
 uint globalLane=offset+lane;
 currentCfgIdx=0u;
 uint fightInCfg=globalLane;
 if(numConfigs>1u&&fightsPerConfig>0u){
  currentCfgIdx=min(globalLane/fightsPerConfig,numConfigs-1u);
  fightInCfg=globalLane%fightsPerConfig;
 }
 ${Object.entries(CONFIG).map(([k,t],i)=>`c.${k}=${t==='f32'?'uintBitsToFloat':''}(getConfigWord(currentCfgIdx,${i}u));`).join('\n')}
 ${Object.entries(STATE).map(([k,t])=>`s.${k}=${t==='f32'?'0.0':'0u'};`).join('\n')}
 s.initialized=1u;s.mana=c.maxMana;eventDamage=0.0;eventFlags=0u;lastEvent=Event(0u,0u,0u,0u);
 seedRandom(mode==2u?0u:(numConfigs>1u?fightInCfg:globalLane));
 enqueue(c.end,13u,0u,0u);enqueue(5000000u,10u,0u,0u);
 if(c.trinketSP>0.0){s.trinketEnd=c.trinketDuration;s.trinketReady=c.trinketCD;enqueue(s.trinketEnd,11u,0u,0u);}
 if(c.petChoice==1u){s.petMana=1150.0;enqueue(300000u,6u,100u,0u);}
 else if(c.petChoice==2u){s.petMana=1450.0;enqueue(1000000u,6u,200u,0u);enqueue(500000u,6u,201u,0u);}
 decide();
 for(uint step=0u;step<eventBudget;step++){
  if(s.done!=0u)break;advance();if(mode==2u&&s.events==lane+1u)break;
 }
 if(mode!=2u&&s.done==0u)s.done=6u;
 ${Array.from({length:6},(_,i)=>`if(max(max(s.casts${i},s.hits${i}),max(s.crits${i},s.misses${i}))>65535u)s.done=7u;`).join('\n')}
 if(max(max(s.petMeleeCasts,s.petMeleeHits),max(s.petMeleeCrits,s.petMeleeMisses))>65535u)s.done=7u;
 if(max(max(s.petSpellCasts,s.petSpellHits),max(s.petSpellCrits,s.petSpellMisses))>65535u)s.done=7u;
 ${Array.from({length:4},(_,i)=>`s.rng${i*2}=rng[${i}].x;s.rng${i*2+1}=rng[${i}].y;`).join('\n')}
 if(mode==2u){
  report0=uvec4(s.now,lastEvent.kind,lastEvent.spell,floatBitsToUint(eventDamage));
  report1=uvec4(floatBitsToUint(s.mana),eventFlags,floatBitsToUint(s.total),s.rngCalls);
 }else{uint base=stripe*16u;report0=outputFour(base);report1=outputFour(base+4u);report2=outputFour(base+8u);report3=outputFour(base+12u);}
}
`;

