import { APL_ACTION, APL_COND, MAX_APL_RULES } from '../model.js';

// Class-owned policy construction; talent resolution remains at the caller boundary.
export function buildPolicyAPL(rotation, talentFlags) {
  const rot = rotation;
  const tf = talentFlags;

  let shaderRotation = 'shadow';
  const actions = new Set();
  const rules = [];

  const addRule = (action, cond = APL_COND.ALWAYS, param = 0.0, targetSpell = 0) => {
    if (rules.length < MAX_APL_RULES) {
      rules.push({ action, cond, param, targetSpell, enabled: 1 });
    }
  };

  // Rule 0: Life Tap resource safeguard
  addRule(APL_ACTION.LIFE_TAP, APL_COND.MANA_LE, 25.0);
  actions.add('eureka');
  addRule(APL_ACTION.EUREKA);

  if (rot === 'FIRE_DESTRO' || rot === 'INCINERATE_DECIMATION') {
    shaderRotation = 'fire';
    actions.add('corr');
    actions.add('immo');
    if (tf.conflagrate) actions.add('conflag');
    if (tf.incinerate) actions.add('incinerate');
    if (tf.shadowburn) actions.add('shadowburn');
    
    if (tf.decimation) {
      addRule(APL_ACTION.DECIMATION_SEARING_PAIN, APL_COND.DECIMATION_INACTIVE, 35.0, 5);
      addRule(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 35.0, 13);
    }
    if (tf.immolate || tf.conflagrate) addRule(APL_ACTION.IMMOLATE, APL_COND.DOT_REM_LE, 0.0, 3);
    if (tf.conflagrate) addRule(APL_ACTION.CONFLAGRATE, APL_COND.ALWAYS);
    if (tf.shadowburn) addRule(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
    addRule(tf.incinerate ? APL_ACTION.INCINERATE_FILLER : APL_ACTION.SEARING_PAIN_FILLER, APL_COND.ALWAYS);
  } else if (rot === 'DP_AF_FIRE') {
    shaderRotation = 'searing';
    actions.add('corr');
    actions.add('agony');
    actions.add('curse');
    actions.add('immo');
    if (tf.demonicBrand) actions.add('brand');
    if (tf.conflagrate) actions.add('conflag');
    
    addRule(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 57.0, 2);
    addRule(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 2.5, 2);
    addRule(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 2.5, 1);
    addRule(APL_ACTION.IMMOLATE, APL_COND.DOT_REM_LE, 2.5, 3);
    if (tf.demonicBrand) addRule(APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, APL_COND.DEMONIC_BRAND_MISSING, 0.0, 5);
    if (tf.decimation) {
      addRule(APL_ACTION.DECIMATION_SEARING_PAIN, APL_COND.DECIMATION_INACTIVE, 28.0, 5);
      addRule(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 28.0, 13);
    }
    if (tf.conflagrate) addRule(APL_ACTION.CONFLAGRATE, APL_COND.ALWAYS);
    addRule(APL_ACTION.SEARING_PAIN_FILLER, APL_COND.ALWAYS);
  } else if (rot === 'DEEP_AFFLICTION' || rot === 'NIGHTFALL_AFFLICTION') {
    shaderRotation = 'shadow';
    actions.add('corr');
    actions.add('agony');
    actions.add('curse');
    if (tf.siphonLife) actions.add('siphon');
    if (tf.wrack) actions.add('wrack');
    if (tf.shadowburn) actions.add('shadowburn');

    if (tf.nightfall) addRule(APL_ACTION.NIGHTFALL_SHADOW_BOLT, APL_COND.SHADOW_TRANCE);
    if (tf.wrack) addRule(APL_ACTION.DRAIN_HOPE, APL_COND.ALWAYS);
    addRule(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 0.0, 1);
    addRule(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 60.0, 2);
    addRule(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 0.0, 2);
    if (tf.siphonLife) addRule(APL_ACTION.SIPHON_LIFE, APL_COND.DOT_REM_LE, 0.0, 15);
    if (tf.shadowburn) addRule(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
    addRule(APL_ACTION.SHADOW_BOLT_FILLER, APL_COND.ALWAYS);
  } else if (rot === 'SHADOW_AND_FLAME') {
    shaderRotation = 'shadow';
    actions.add('corr');
    actions.add('agony');
    actions.add('curse');
    actions.add('immo');
    if (tf.conflagrate) actions.add('conflag');
    if (tf.shadowburn) actions.add('shadowburn');
    if (tf.demonicBrand) actions.add('brand');

    if (tf.nightfall) addRule(APL_ACTION.NIGHTFALL_SHADOW_BOLT, APL_COND.SHADOW_TRANCE);
    if (tf.demonicBrand) addRule(APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, APL_COND.DEMONIC_BRAND_MISSING, 0.0, 5);
    if (tf.decimation) {
      addRule(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 35.0, 13);
    }
    addRule(APL_ACTION.IMMOLATE, APL_COND.DOT_REM_LE, 0.0, 3);
    if (tf.conflagrate) addRule(APL_ACTION.CONFLAGRATE, APL_COND.ALWAYS);
    addRule(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 0.0, 1);
    addRule(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 60.0, 2);
    addRule(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 0.0, 2);
    if (tf.shadowburn) addRule(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
    addRule(APL_ACTION.SHADOW_BOLT_FILLER, APL_COND.ALWAYS);
  } else {
    // SHADOW_DESTRO / DP_AF_SHADOW
    shaderRotation = 'shadow';
    actions.add('corr');
    actions.add('agony');
    actions.add('curse');
    if (tf.shadowburn) actions.add('shadowburn');
    if (tf.demonicBrand) actions.add('brand');

    if (tf.nightfall) addRule(APL_ACTION.NIGHTFALL_SHADOW_BOLT, APL_COND.SHADOW_TRANCE);
    if (tf.demonicBrand) addRule(APL_ACTION.DEMONIC_BRAND_SEARING_PAIN, APL_COND.DEMONIC_BRAND_MISSING, 0.0, 5);
    if (tf.decimation) {
      addRule(APL_ACTION.DECIMATION_SOUL_FIRE, APL_COND.DECIMATION_ACTIVE, 35.0, 13);
    }
    addRule(APL_ACTION.CORRUPTION, APL_COND.DOT_REM_LE, 0.0, 1);
    addRule(APL_ACTION.CURSE_OF_DOOM, APL_COND.FIGHT_TIME_GE, 60.0, 2);
    addRule(APL_ACTION.CURSE_OF_AGONY, APL_COND.DOT_REM_LE, 0.0, 2);
    if (tf.shadowburn) addRule(APL_ACTION.SHADOWBURN, APL_COND.ALWAYS);
    addRule(APL_ACTION.SHADOW_BOLT_FILLER, APL_COND.ALWAYS);
  }

  return { shaderRotation, actionIds: Array.from(actions), aplRules: rules };
}

