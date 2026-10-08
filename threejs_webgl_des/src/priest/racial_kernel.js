// Racial actions use the same DES in both observation modes.
export function racialGLSL(detailed) {
 const observe=code=>detailed?code:'';
 return `
uint racialReady, racialEnd, eurekaEnd, eurekaCharges, graveReady, chastiseReady;
uint feedbackReady, feedbackEnd, guardEnd, guardCharges, guardReady, weaknessEnd, weaknessBuffEnd, hexEnd;
uint sacrificeReady, sacrificeTicks, prayerReady, graceReady, contingencyReady, contingencyEnd, contingencyTicks, shieldEnd, flashReady, confuseEnd, rootEnd, eluneGraceReady, eluneGraceEnd, eluneGraceMisses, weakenedSoulEnd;
float playerHealth, allyHealth, allyShield, castEureka, flayEureka, penanceEureka;
bool racial(uint race){return c.racialsEnabled!=0u&&c.race==race;}
float haste(){return racial(5u)&&now<racialEnd?1.1:1.0;}
uint hasted(uint ms){return max(1u,uint(round(float(ms)/haste())));}
uint gcd(){return max(1000u,hasted(1500u));}
float racialCrit(){return racial(2u)&&now<racialEnd?0.1:0.0;}
bool eurekaEligible(uint spell){return spell==1u||spell==2u||spell==3u||spell==4u||spell==5u||spell==6u||spell==7u||spell==10u||spell==12u||spell==18u||spell==19u;}
float eurekaDiscount(uint spell,uint at){return eurekaEligible(spell)&&eurekaCharges>0u&&at<eurekaEnd?0.9:1.0;}
float useEureka(uint spell){if(eurekaDiscount(spell,now)<1.0){eurekaCharges--;return 1.1;}return 1.0;}
void heal(uint spell,float amount,bool ally){
 if((ally?allyHealth:playerHealth)<=0.0)return;
 float gain=min(amount,(ally?c.allyMaxHealth:c.maxHealth)-(ally?allyHealth:playerHealth));
 if(ally)allyHealth+=gain;else playerHealth+=gain;
 ${observe("s.healingDone+=gain;if(spell==18u)s.healing18+=gain;else if(spell==19u)s.healing19+=gain;else if(spell==20u)s.healing20+=gain;")}
}
void graveProc(){
 if(!racial(1u)||now<graveReady)return;
 if(random01()<0.1){graveReady=now+uint(round(c.graveProcInterval*1000.0));${observe('s.casts23++;')}damage(23u,c.maxHealth*0.05,false);heal(23u,c.maxHealth*0.05,false);}
}
void offGCDRacial(){
 if(c.racialsEnabled==0u||now<racialReady)return;
 if(c.race==2u){racialReady=now+180000u;racialEnd=now+15000u;${observe('s.elunesLightUses++;')}}
 else if(c.race==4u){racialReady=now+120000u;eurekaEnd=now+15000u;eurekaCharges=3u;${observe('s.eurekaUses++;')}}
 else if(c.race==5u){racialReady=now+180000u;racialEnd=now+10000u;${observe('s.berserkingUses++;')}}
}
bool utilityAffordable(float cost){return s.mana>=cost;}
void utilityPay(float cost){s.mana-=cost;s.manaSpent+=cost;if(cost>0.0)fsrEnd=now+5000u;}
void utilityGCD(uint spell){countCast(spell);enqueue(now+gcd(),3u,0u);}
float racialHealing(uint spell){
 bool crit=random01()<min(1.0,c.holyCritChance+racialCrit());
 float bonus=useEureka(spell);
 float amount=(mix(1318.0,1546.0,random01())+(c.healingPower+(now<spiritTapEnd?c.spiritTapSpellPower:0.0))*0.429)*c.healingMultiplier*c.instantDamageMultiplier*bonus*(crit?1.5:1.0)*(now<infusionEnd?1.2:1.0);
 ${observe('if(crit){if(spell==18u)s.crits18++;else s.crits19++;}if(spell==18u)s.hits18++;else s.hits19++;')}
 return amount;
}
// Utility spells are conditional support actions before the damage APL. They
// never activate in an encounter missing their attack/heal trigger.
bool useUtility(){
 if(c.racialsEnabled==0u)return false;
 bool incoming=c.incomingAttackInterval>0.0&&c.incomingDamage>0.0;
 if(racial(3u)&&c.shadowformEnabled==0u&&now>=prayerReady&&playerHealth<c.maxHealth*0.5){prayerReady=now+600000u;heal(18u,racialHealing(18u),false);utilityGCD(18u);return true;}
 if(racial(0u)&&c.shadowformEnabled==0u&&now>=graceReady&&allyHealth>0.0&&allyHealth<c.allyMaxHealth*0.5&&c.allyDistance<=40.0){graceReady=now+600000u;weakenedSoulEnd=now;heal(19u,racialHealing(19u),true);utilityGCD(19u);return true;}
 if(racial(4u)&&c.shadowformEnabled==0u&&c.allyAttackInterval>0.0&&c.allyIncomingDamage>0.0&&allyHealth>0.0&&c.allyDistance<=30.0&&now>=contingencyReady){contingencyReady=now+600000u;contingencyEnd=now+30000u;utilityGCD(20u);return true;}
 if(racial(1u)&&now>=sacrificeReady&&playerHealth>1600.0&&c.maxMana-s.mana>=1600.0+c.spirit){sacrificeReady=now+600000u;sacrificeTicks=5u;enqueue(now+3000u,6u,17u);utilityGCD(17u);return true;}
 if(racial(0u)&&incoming&&c.incomingAttackType==3u&&enemyMana>0.0&&now>=feedbackReady&&utilityAffordable(230.0*c.instantCostMultiplier*c.costMultiplier)){utilityPay(230.0*c.instantCostMultiplier*c.costMultiplier);feedbackReady=now+180000u;feedbackEnd=now+15000u;utilityGCD(14u);return true;}
 if(racial(1u)&&incoming&&c.incomingAttackType==1u&&now>=weaknessBuffEnd&&utilityAffordable(195.0*c.instantCostMultiplier*c.costMultiplier)){utilityPay(195.0*c.instantCostMultiplier*c.costMultiplier);weaknessBuffEnd=now+600000u;utilityGCD(15u);return true;}
 if(racial(5u)&&incoming&&(guardCharges==0u||now>=guardEnd)&&utilityAffordable(250.0*c.instantCostMultiplier*c.costMultiplier)){utilityPay(250.0*c.instantCostMultiplier*c.costMultiplier);guardEnd=now+600000u;guardCharges=3u;utilityGCD(13u);return true;}
 if(racial(5u)&&(incoming&&c.incomingAttackType==1u||c.enemyHealingPerTick>0.0)&&now>=hexEnd&&c.targetDistance<=30.0*c.shadowRangeMultiplier&&utilityAffordable(240.0*c.instantCostMultiplier*c.costMultiplier)){utilityPay(240.0*c.instantCostMultiplier*c.costMultiplier);if(hit(16u))hexEnd=now+120000u;utilityGCD(16u);return true;}
 if(racial(2u)&&incoming&&c.incomingAttackType!=3u&&now>=eluneGraceReady&&utilityAffordable(c.baseMana*0.03*c.instantCostMultiplier)){utilityPay(c.baseMana*0.03*c.instantCostMultiplier);eluneGraceReady=now+300000u;eluneGraceEnd=now+15000u;eluneGraceMisses=0u;utilityGCD(22u);return true;}
 if(racial(4u)&&incoming&&playerHealth<c.maxHealth*0.5&&c.targetControlImmune==0u&&c.targetDistance<=8.0&&now>=flashReady&&c.durationMs-now>=hasted(500u)&&utilityAffordable(c.baseMana*0.03)){utilityPay(c.baseMana*0.03);flashReady=now+120000u;countCast(21u);enqueue(now+hasted(500u),6u,21u);enqueue(now+hasted(500u),3u,0u);return true;}
 return false;
}
void incomingAttack(bool ally){
 if(ally&&allyHealth<=0.0)return;
 if(now<confuseEnd||(c.incomingAttackType==1u&&c.targetDistance>5.0&&now<rootEnd)){${observe('s.damagePrevented+=ally?c.allyIncomingDamage:c.incomingDamage;')}return;}
 if(!ally){
  float chance=c.incomingHitChance;
  if(c.incomingAttackType!=3u&&now<eluneGraceEnd)chance=max(0.0,chance-0.5);
  if(c.incomingAttackType==1u&&racial(2u))chance=max(0.0,chance-0.01);
  if(chance<1.0&&random01()>=chance){if(c.incomingAttackType!=3u&&now<eluneGraceEnd){eluneGraceMisses++;if(eluneGraceMisses>=3u)eluneGraceEnd=now;}return;}
 }
 float amount=ally?c.allyIncomingDamage:c.incomingDamage;
 if(c.incomingAttackType==1u&&(now<weaknessEnd||now<hexEnd)){float reduction=min(amount,204.0/14.0*c.enemyAttackSpeed);amount-=reduction;${observe('s.damagePrevented+=reduction;')}}
 if(!ally&&c.incomingAttackType!=3u&&c.shadowformEnabled!=0u)amount*=0.85;
 if(ally){
  if(now>=shieldEnd)allyShield=0.0;
  float absorb=min(allyShield,amount);allyShield-=absorb;amount-=absorb;${observe('s.absorbed+=absorb;')}
  allyHealth=max(0.0,allyHealth-amount);
  // The ward reacts AFTER the triggering hit; it cannot resurrect a dead ally.
  if(allyHealth>0.0&&allyHealth<c.allyMaxHealth*0.35&&now<contingencyEnd){contingencyEnd=now;allyShield=926.0;shieldEnd=now+15000u;contingencyTicks=5u;enqueue(now+3000u,6u,20u);${observe('s.hits20++;')}}
 }else{
  playerHealth=max(0.0,playerHealth-amount);${observe('s.damageTaken+=amount;')}
  if(c.incomingAttackType==3u&&now<feedbackEnd&&enemyMana>0.0){float burned=min(105.0,enemyMana);enemyMana-=burned;${observe('s.targetManaBurned+=burned;')}if(hit(14u))damage(14u,burned*c.damageMultiplier*c.instantDamageMultiplier,false);}
  if(c.incomingAttackType==1u&&now<weaknessBuffEnd){weaknessBuffEnd=now;weaknessEnd=now+120000u;if(hit(15u))damage(15u,(56.0+shadowPower()*0.107)*c.damageMultiplier*c.instantDamageMultiplier,false);}
  if(guardCharges>0u&&now<guardEnd&&now>=guardReady){guardReady=now+uint(round(c.shadowguardProcInterval*1000.0));guardCharges--;if(hit(13u))damage(13u,(96.0+shadowPower()*0.267)*c.damageMultiplier*c.instantDamageMultiplier,false);}
 }
}
void racialEvent(uint spell){
 if(spell==17u){float loss=min(320.0,max(0.0,playerHealth-1.0));playerHealth-=loss;float gain=min((320.0+c.spirit*0.2)*(loss/320.0),c.maxMana-s.mana);s.mana+=gain;s.manaGained+=gain;${observe('s.healthSpent+=loss;s.darkSacrificeMana+=gain;')}sacrificeTicks--;if(sacrificeTicks>0u)enqueue(now+3000u,6u,spell);}
 else if(spell==20u){bool crit=random01()<min(1.0,c.holyCritChance+racialCrit());heal(20u,134.0*c.healingMultiplier*(crit?1.5:1.0),true);${observe('if(crit)s.crits20++;')}contingencyTicks--;if(contingencyTicks>0u)enqueue(now+3000u,6u,spell);}
 else if(spell==21u){if(hit(21u)&&c.targetControlImmune==0u)confuseEnd=now+3000u;}
}
void initRacials(){
 racialReady=0u;racialEnd=0u;eurekaEnd=0u;eurekaCharges=0u;graveReady=0u;chastiseReady=0u;feedbackReady=0u;feedbackEnd=0u;guardEnd=0u;guardCharges=0u;guardReady=0u;weaknessEnd=0u;weaknessBuffEnd=0u;hexEnd=0u;sacrificeReady=0u;sacrificeTicks=0u;prayerReady=0u;graceReady=0u;contingencyReady=0u;contingencyEnd=0u;contingencyTicks=0u;shieldEnd=0u;flashReady=0u;confuseEnd=0u;rootEnd=0u;eluneGraceReady=0u;eluneGraceEnd=0u;eluneGraceMisses=0u;weakenedSoulEnd=uint(round(c.allyWeakenedSoul*1000.0));
 playerHealth=c.maxHealth*c.initialHealthRatio;allyHealth=c.allyMaxHealth*c.allyInitialHealthRatio;allyShield=0.0;castEureka=1.0;flayEureka=1.0;penanceEureka=1.0;
 if(c.incomingAttackInterval>0.0)enqueue(uint(round(c.incomingAttackInterval*1000.0)),5u,0u);
 if(c.allyAttackInterval>0.0)enqueue(uint(round(c.allyAttackInterval*1000.0)),5u,1u);
}
`;
}
