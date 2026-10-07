import test from 'node:test';
import assert from 'node:assert/strict';
import { getClassModule } from '../src/classes/registry.js';

function rng(seed) {
  let state = BigInt(seed);
  return { nextU64() {
    state = (state * 6364136223846793005n + 1442695040888963407n) & 0xFFFFFFFFFFFFFFFFn;
    return Number(state >> 32n) >>> 0;
  } };
}

const { graph, definitions, nodeCount, pointBudget } = getClassModule('warlock').search.talents;

test('Warlock talent generation, crossover repair, and mutation preserve legality', () => {
  for (let seed = 1; seed <= 100; seed++) {
    const random = rng(seed);
    const a = graph.generateRandomValid(random);
    const b = graph.generateRandomValid(random);
    assert.ok(graph.isValid(a));
    assert.ok(graph.isValid(b));
    const child = Uint8Array.from(a, (rank, i) => random.nextU64() % 2 ? rank : b[i]);
    graph.repair(child, random);
    assert.ok(graph.isValid(child));
    assert.equal(graph.countTotalPoints(child), pointBudget);
    assert.deepEqual(graph.fromTalentsObject(graph.toTalentsObject(child)), child);
    for (const donor of graph.getValidDonors(child)) {
      const moved = child.slice();
      moved[donor]--;
      for (const receiver of graph.getValidReceivers(moved)) {
        const candidate = moved.slice();
        candidate[receiver]++;
        assert.ok(graph.isValid(candidate), `${seed}: ${donor} -> ${receiver}`);
      }
    }
  }
});

test('Warlock talent repair handles overfilled and unsupported prerequisite ranks', () => {
  const ranks = new Uint8Array(nodeCount).fill(255);
  graph.repair(ranks, rng(42));
  assert.ok(graph.isValid(ranks));
  for (let i = 0; i < nodeCount; i++) assert.ok(ranks[i] <= definitions[i].max);
});

const { pets } = getClassModule('warlock').search;
const forcedModes = {
  ACTIVE_IMP: ['imp', false, false],
  ACTIVE_SUCCUBUS: ['succubus', false, false],
  SAC_IMP: ['none', true, false],
  SAC_SUCCUBUS: ['none', false, true],
  DEMONIC_PACT_IMP_SUCC: ['succubus', true, false],
  DEMONIC_PACT_SUCC_IMP: ['imp', false, true],
  NO_PET: ['none', false, false],
};

test('forced pet modes preserve legal talents, explicit requirements, and sacrifice prerequisites', () => {
  for (const [mode, expected] of Object.entries(forcedModes)) {
    for (let seed = 1; seed <= 100; seed++) {
      const random = rng(seed);
      const ind = { talents: graph.generateRandomValid(random), pet: 'none', sacImp: false, sacSuccubus: false };
      const requiredTalents = [0];
      pets.enforce(ind, { forcedPetMode: mode, requiredTalents }, random);
      assert.deepEqual([ind.pet, ind.sacImp, ind.sacSuccubus], expected);
      assert.ok(graph.isValid(ind.talents), `${mode}, seed ${seed}`);
      assert.equal(ind.talents[0], definitions[0].max);
      assert.deepEqual(requiredTalents, [0]);
      if (mode.startsWith('SAC_')) assert.equal(ind.talents[26], 1);
      if (mode.startsWith('DEMONIC_PACT_')) {
        assert.equal(ind.talents[35], 1);
        assert.equal(ind.talents[32], 1);
        assert.equal(ind.talents[26], 1);
      }
    }
  }
});

test('unforced pet repair respects sacrifice and pact talent availability without consuming RNG', () => {
  for (const forcedPetMode of [undefined, 'ALL']) {
    for (const [ds, dp, pet, sacImp, sacSuccubus] of [
      [0, 0, 'imp', false, false],
      [1, 0, 'none', true, false],
      [1, 1, 'imp', true, false],
    ]) {
      const talents = new Uint8Array(nodeCount);
      talents[26] = ds;
      talents[35] = dp;
      const ind = { talents, pet: 'imp', sacImp: true, sacSuccubus: false };
      pets.enforce(ind, { forcedPetMode }, { nextU64() { assert.fail('unexpected RNG consumption'); } });
      assert.deepEqual([ind.pet, ind.sacImp, ind.sacSuccubus], [pet, sacImp, sacSuccubus]);
    }
  }
});

const { choices } = getClassModule('warlock').search;

test('locked race and rotation selections preserve other candidate fields and unlocked choices', () => {
  for (const forcedRace of [undefined, '', 'ALL', ...choices.races]) {
    for (const forcedRotation of [undefined, '', 'ALL', ...choices.rotations]) {
      const talents = new Uint8Array(nodeCount);
      const apl = { rules: [] };
      const ind = { talents, apl, race: 'ORC', rotation: 'FIRE_DESTRO', pet: 'imp',
        sacImp: false, sacSuccubus: true, fitness: 123 };
      const expected = { ...ind,
        race: forcedRace && forcedRace !== 'ALL' ? forcedRace : ind.race,
        rotation: forcedRotation && forcedRotation !== 'ALL' ? forcedRotation : ind.rotation };
      choices.enforceLocked(ind, { forcedRace, forcedRotation });
      assert.deepEqual(ind, expected);
      assert.equal(ind.talents, talents);
      assert.equal(ind.apl, apl);
    }
  }
  assert.equal(new Set(choices.races).size, choices.races.length);
  assert.equal(new Set(choices.rotations).size, choices.rotations.length);
  assert.deepEqual(Object.keys(choices.rotationLabels), choices.rotations);
});

const { APL_ACTION, APL_COND, MAX_APL_RULES } = await import('../src/model.js');
const { build: buildPolicyAPL } = getClassModule('warlock').search.policyAPL;

test('policy APL keeps resource protection first and a usable filler last for every rotation', () => {
  for (const rotation of choices.rotations) {
    for (const flags of [{}, { incinerate: true, conflagrate: true, immolate: true, shadowburn: true,
      decimation: true, demonicBrand: true, nightfall: true, siphonLife: true, wrack: true }]) {
      const original = { ...flags };
      const policy = buildPolicyAPL(rotation, flags);
      assert.deepEqual(policy.aplRules[0], { action: APL_ACTION.LIFE_TAP,
        cond: APL_COND.MANA_LE, param: 25, targetSpell: 0, enabled: 1 });
      assert.ok(policy.aplRules.length <= MAX_APL_RULES);
      assert.ok([APL_ACTION.SHADOW_BOLT_FILLER, APL_ACTION.INCINERATE_FILLER,
        APL_ACTION.SEARING_PAIN_FILLER].includes(policy.aplRules.at(-1).action));
      assert.equal(new Set(policy.actionIds).size, policy.actionIds.length);
      assert.deepEqual(flags, original);
    }
  }
});

test('policy APL gates talent actions and preserves execute and affliction priority', () => {
  const fire = buildPolicyAPL('FIRE_DESTRO', {});
  assert.equal(fire.shaderRotation, 'fire');
  assert.equal(fire.aplRules.at(-1).action, APL_ACTION.SEARING_PAIN_FILLER);
  assert.ok(!fire.actionIds.includes('conflag'));
  assert.ok(!fire.aplRules.some(r => r.action === APL_ACTION.DECIMATION_SOUL_FIRE));
  const execute = buildPolicyAPL('INCINERATE_DECIMATION', { decimation: true, incinerate: true });
  assert.equal(execute.aplRules[1].action, APL_ACTION.DECIMATION_SEARING_PAIN);
  assert.equal(execute.aplRules[2].action, APL_ACTION.DECIMATION_SOUL_FIRE);
  assert.equal(execute.aplRules.at(-1).action, APL_ACTION.INCINERATE_FILLER);
  const affliction = buildPolicyAPL('DEEP_AFFLICTION', { nightfall: true, wrack: true, siphonLife: true });
  assert.equal(affliction.aplRules[1].action, APL_ACTION.NIGHTFALL_SHADOW_BOLT);
  assert.equal(affliction.aplRules[2].action, APL_ACTION.DRAIN_HOPE);
  assert.ok(affliction.actionIds.includes('siphon'));
  const pactFire = buildPolicyAPL('DP_AF_FIRE', { demonicBrand: true });
  assert.equal(pactFire.shaderRotation, 'searing');
  assert.equal(pactFire.aplRules[1].param, 57);
  assert.ok(pactFire.aplRules.some(r => r.action === APL_ACTION.DEMONIC_BRAND_SEARING_PAIN));
});

const { apl: aplSearch } = getClassModule('warlock').search;

test('synthesis eligibility gates talent actions and retains baseline actions in metadata order', () => {
  const gates = { nightfall: 'nightfall', incinerate: 'incinerate', conflag: 'conflagrate',
    shadowburn: 'shadowburn', siphon: 'siphonLife', wrack: 'wrack', brand: 'demonicBrand',
    decimateSearing: 'decimation', decimateSoulFire: 'decimation' };
  const baseline = aplSearch.actions.filter(a => !gates[a.id]).map(a => a.id);
  assert.ok(baseline.includes('hellfire'));
  assert.deepEqual(aplSearch.availableActions().map(a => a.id), baseline);
  assert.deepEqual(aplSearch.availableActions(null).map(a => a.id), baseline);
  for (const talent of new Set(Object.values(gates))) {
    const flags = { [talent]: true };
    const available = aplSearch.availableActions(flags);
    assert.deepEqual(available.map(a => a.id), aplSearch.actions.filter(a => !gates[a.id] || gates[a.id] === talent).map(a => a.id));
    for (const action of available) assert.equal(action, aplSearch.actions.find(a => a.id === action.id));
  }
});

test('synthesis conditions require proc talents and return independent eligibility lists', () => {
  for (const [action, condition, talent] of [
    ['nightfall', 'SHADOW_TRANCE', 'nightfall'],
    ['brand', 'DEMONIC_BRAND_MISSING', 'demonicBrand'],
    ['decimateSoulFire', 'DECIMATION_ACTIVE', 'decimation'],
    ['decimateSearing', 'DECIMATION_INACTIVE', 'decimation'],
  ]) {
    assert.ok(!aplSearch.conditionsForAction(action).includes(condition));
    assert.ok(aplSearch.conditionsForAction(action, { [talent]: true }).includes(condition));
    const result = aplSearch.conditionsForAction(action, { [talent]: true });
    result.length = 0;
    assert.ok(aplSearch.conditionsForAction(action, { [talent]: true }).includes(condition));
  }
  assert.deepEqual(aplSearch.conditionsForAction('unsupported', null), ['ALWAYS', 'NEVER']);
  for (const action of aplSearch.actions) {
    for (const key of aplSearch.validConditions[action.id]) assert.ok(aplSearch.conditionTypes[key]);
  }
});

test('APL encoding retains strict comparisons, per-clause targets, and NEVER disabling', () => {
  const rule = aplSearch.createTwoConditionRule('agony', 'DOOM_MISSING', 0, 'DOT_REM_LT', 2.34);
  assert.equal(rule.cond1, APL_COND.DOOM_MISSING);
  assert.equal(rule.cond2, APL_COND.DOT_REM_LT);
  assert.equal(rule.param2, 2.3);
  assert.equal(rule.targetSpell1, 2);
  assert.equal(rule.targetSpell2, 2);
  assert.ok(rule.rawCond.includes(' && '));
  const disabled = aplSearch.createTwoConditionRule('corr', 'FIGHT_TIME_GE', 12, 'NEVER');
  assert.equal(disabled.enabled, false);
  assert.equal(disabled.rawCond, 'false');
  assert.equal(aplSearch.encodeIndividual({ rules: [disabled] })[0].enabled, 0);
  const execute = aplSearch.createTwoConditionRule('decimateSoulFire', 'DECIMATION_ACTIVE', 0);
  assert.equal(execute.param1, 35);
  assert.equal(execute.targetSpell1, 13);
  assert.equal(aplSearch.createCondition('conflag', 'DOT_REM_LT', 999).param, 6);
  assert.equal(aplSearch.createCondition('tap', 'MANA_LE', -1).param, 5);
});

test('APL bytecode supports legacy single-condition rules and independent modern clauses', () => {
  const legacy = { action: APL_ACTION.CORRUPTION, cond: APL_COND.DOT_REM_LE,
    param: '2.5', targetSpell: '1', enabled: true };
  const modern = aplSearch.createTwoConditionRule('decimateSearing', 'TARGET_HP_LE', 35, 'DECIMATION_INACTIVE', 35);
  const [old, current] = aplSearch.encodeIndividual({ rules: [legacy, modern] });
  assert.equal(old.cond1, legacy.cond);
  assert.equal(old.param1, 2.5);
  assert.equal(old.targetSpell1, 1);
  assert.equal(old.cond2, APL_COND.ALWAYS);
  assert.equal(old.param2, 0);
  assert.equal(current.targetSpell1, 0);
  assert.equal(current.targetSpell2, 5);
  assert.equal(current.cond, current.cond1);
});

test('handcrafted APL rules retain cooldown, channel-duration, and resource requirements', () => {
  const conflag = aplSearch.handcraftedRule('conflag');
  assert.equal(conflag.cond1, APL_COND.DOT_REM_LT);
  assert.equal(conflag.param1, 6);
  assert.equal(conflag.targetSpell1, 3);
  assert.equal(aplSearch.handcraftedRule('hellfire').param1, 15);
  assert.equal(aplSearch.handcraftedRule('tap').param1, 20);
  assert.equal(aplSearch.handcraftedRule('curse').cond2, APL_COND.DOOM_MISSING);
  for (const action of aplSearch.actions) {
    const rule = aplSearch.handcraftedRule(action.id);
    assert.equal(rule.id, action.id);
    assert.equal(rule.action, action.action);
    assert.equal(rule.enabled, true);
  }
});

test('APL initialization creates each eligible action once and preserves default filler order', () => {
  const random = { nextInt(min, max) { return min; }, nextDouble() { return 0.25; },
    nextFloat(min, max) { return (min + max) / 2; } };
  for (const flags of [{}, { incinerate: true, decimation: true, nightfall: true }]) {
    const actions = aplSearch.availableActions(flags);
    const defaults = aplSearch.createDefault(actions, flags);
    assert.equal(defaults.rules[0].id, 'tap');
    assert.deepEqual(defaults.rules.slice(-3).map(r => r.id), flags.incinerate ?
      ['incinerate', 'searing', 'bolt'] : ['hellfire', 'bolt', 'searing']);
    for (const locked of [true, false]) {
      const ind = aplSearch.createRandom(random, actions, flags, locked);
      assert.deepEqual(ind.rules.map(r => r.id).sort(), actions.map(a => a.id).sort());
      assert.equal(ind.fitness, 0);
      assert.equal(ind.batch, null);
      if (locked) for (const rule of ind.rules) assert.deepEqual(rule, aplSearch.handcraftedRule(rule.id));
      else for (const rule of ind.rules) {
        const valid = aplSearch.conditionsForAction(rule.id, flags);
        assert.ok(valid.includes(rule.condKey1));
        assert.ok(valid.includes(rule.condKey2));
      }
    }
  }
});

test('APL action repair removes duplicates and unavailable actions while retaining first-rule order', () => {
  const actions = aplSearch.availableActions({});
  const first = aplSearch.createRule('corr', 'NEVER');
  const ind = { rules: [first, aplSearch.handcraftedRule('incinerate'),
    aplSearch.handcraftedRule('corr'), aplSearch.handcraftedRule('tap')], fitness: 123, batch: { id: 7 } };
  const noRandom = { nextInt() { assert.fail('locked repair consumes no RNG'); } };
  aplSearch.repair(ind, noRandom, actions, {}, true);
  assert.deepEqual(ind.rules.slice(0, 2).map(r => r.id), ['corr', 'tap']);
  assert.deepEqual(ind.rules.map(r => r.id).sort(), actions.map(a => a.id).sort());
  assert.equal(ind.rules[0].enabled, true);
  assert.equal(ind.fitness, 123);
  assert.equal(ind.batch.id, 7);
  const unlocked = { rules: [first, first] };
  aplSearch.repair(unlocked, noRandom, [actions.find(a => a.id === 'corr')], {}, false);
  assert.equal(unlocked.rules.length, 1);
  assert.equal(unlocked.rules[0], first);
  assert.equal(unlocked.rules[0].enabled, false);
  aplSearch.repair(unlocked, noRandom, [], {}, true);
  assert.deepEqual(unlocked.rules, []);
});

function aplRng(seed) {
  const random = rng(seed);
  const nextDouble = () => random.nextU64() / 4294967296;
  return { nextDouble, nextInt(min, max) { return min + Math.floor(nextDouble() * (max - min + 1)); },
    nextFloat(min, max) { return min + nextDouble() * (max - min); },
    nextGaussian(mean, stdev) { return mean + stdev * Math.sqrt(-2 * Math.log(Math.max(1e-9, nextDouble()))) * Math.cos(2 * Math.PI * nextDouble()); } };
}

test('APL crossover and mutation preserve eligible permutations, parent data, and locked rules', () => {
  for (const locked of [true, false]) {
    for (let seed = 1; seed <= 100; seed++) {
      const random = aplRng(seed);
      const flags = { nightfall: seed % 2, decimation: seed % 3, incinerate: seed % 5,
        conflagrate: seed % 7, demonicBrand: seed % 11 };
      const actions = aplSearch.availableActions(flags);
      const p1 = aplSearch.createRandom(random, actions, flags, locked);
      const p2 = aplSearch.createRandom(random, actions, flags, locked);
      const saved = JSON.stringify([p1, p2]);
      const child = aplSearch.crossover(p1, p2, random, actions, flags, locked);
      for (let i = 0; i < 10; i++) {
        aplSearch.mutate(child, random, { lockConditions: locked, condMutRate: 1, jitterRate: 1, toggleNeverRate: 1 }, flags);
        assert.deepEqual(child.rules.map(r => r.id).sort(), actions.map(a => a.id).sort());
        for (const rule of child.rules) {
          assert.ok(!p1.rules.includes(rule));
          assert.ok(!p2.rules.includes(rule));
          if (locked) assert.deepEqual(rule, aplSearch.handcraftedRule(rule.id));
          else {
            const valid = aplSearch.conditionsForAction(rule.id, flags);
            assert.ok(valid.includes(rule.condKey1));
            assert.ok(valid.includes(rule.condKey2));
          }
        }
      }
      assert.equal(JSON.stringify([p1, p2]), saved);
      assert.equal(child.fitness, 0);
      assert.equal(child.batch, null);
    }
  }
});

test('small APL populations cross over without aliasing and mutation skips fewer than two rules', () => {
  for (const size of [0, 1, 2]) {
    const actions = aplSearch.actions.slice(0, size);
    const parent = aplSearch.createDefault(actions);
    const child = aplSearch.crossover(parent, parent, aplRng(42), actions);
    assert.deepEqual(child, parent);
    assert.notEqual(child.rules, parent.rules);
    for (let i = 0; i < size; i++) assert.notEqual(child.rules[i], parent.rules[i]);
    if (size < 2) aplSearch.mutate(child, { nextDouble() { assert.fail('unexpected RNG consumption'); } });
  }
});

const { getTalentFlagsFromRanks } = await import('../src/talents.js');
const buildSearch = getClassModule('warlock').search.createBuildSearch(getTalentFlagsFromRanks);

test('build search preserves forced selections and parents across static and evolved APLs', () => {
  for (const aplMode of ['static', 'evolve']) for (const lockConditions of [true, false]) {
    for (let seed = 1; seed <= 30; seed++) {
      const random = { ...aplRng(seed), ...rng(seed) };
      const config = { aplMode, lockConditions, forcedPetMode: 'DEMONIC_PACT_SUCC_IMP',
        forcedRace: 'GNOME', forcedRotation: 'FIRE_DESTRO', requiredTalents: [0] };
      const p1 = buildSearch.createRandomIndividual(random, config);
      const p2 = buildSearch.createRandomIndividual(random, config);
      assert.ok(graph.isValid(p1.talents));
      assert.ok(graph.isValid(p2.talents));
      const saved = JSON.stringify([p1, p2]);
      const child = buildSearch.crossoverIndividuals(p1, p2, random, config);
      assert.ok(graph.isValid(child.talents));
      buildSearch.mutateIndividual(child, random, config);
      assert.equal(JSON.stringify([p1, p2]), saved);
      assert.equal(child.race, 'GNOME');
      assert.equal(child.rotation, 'FIRE_DESTRO');
      assert.equal(child.pet, 'imp');
      assert.equal(child.sacSuccubus, true);
      assert.equal(child.sacImp, false);
      assert.equal(child.talents[0], definitions[0].max);
      assert.equal(child.talents[35], 1);
      assert.notEqual(child.talents, p1.talents);
      if (aplMode === 'static') assert.equal(child.apl, null);
      else {
        const flags = getTalentFlagsFromRanks(graph.toTalentsObject(child.talents));
        assert.deepEqual(child.apl.rules.map(r => r.id).sort(), aplSearch.availableActions(flags).map(a => a.id).sort());
      }
    }
  }
});

const candidateConfig = getClassModule('warlock').search.createCandidateConfig(getTalentFlagsFromRanks);

test('build conversion preserves race normalization and leaves candidates and base stats unchanged', () => {
  const ind = { talents: new Uint8Array(nodeCount), race: 'GNOME', rotation: 'SHADOW_DESTRO',
    pet: 'none', sacImp: false, sacSuccubus: false, apl: null };
  const base = { race: 'HUMAN', intellect: 300, spirit: 210 };
  const saved = JSON.stringify([ind, base]);
  const result = candidateConfig.buildToConfig(ind, base);
  assert.equal(result.intellect, 315);
  assert.equal(result.spirit, 200);
  assert.equal(result.rotation, 'shadow');
  assert.equal(result.tapThreshold, 25);
  assert.equal(JSON.stringify([ind, base]), saved);
});

test('candidate conversion preserves distinct build and APL channel fallback behavior', () => {
  const talents = new Uint8Array(nodeCount);
  const talentFlags = getTalentFlagsFromRanks(graph.toTalentsObject(talents));
  const base = { rotation: 'fire', talentFlags, intellect: 300, spirit: 200 };
  for (const [id, expected] of [['incinerate', 'fire'], ['searing', 'searing'], ['bolt', 'shadow']]) {
    const apl = { rules: [aplSearch.createRule('searing', 'NEVER'), aplSearch.handcraftedRule(id)] };
    const build = { talents, race: 'ORC', rotation: 'SHADOW_DESTRO', pet: 'none', apl };
    assert.equal(candidateConfig.buildToConfig(build, base).rotation, expected);
    assert.equal(candidateConfig.aplToConfig(apl, base).rotation, expected);
  }
  const apl = { rules: [aplSearch.handcraftedRule('hellfire')] };
  const build = { talents, race: 'ORC', rotation: 'SHADOW_DESTRO', pet: 'none', apl };
  assert.equal(candidateConfig.buildToConfig(build, base).rotation, 'shadow');
  assert.equal(candidateConfig.aplToConfig(apl, base).rotation, 'fire');
});

const { identity } = getClassModule('warlock').search;

test('search identity distinguishes candidate genes and active APL priority', () => {
  const candidate = { talents: new Uint8Array(nodeCount), race: 'ORC', rotation: 'SHADOW_DESTRO',
    pet: 'none', sacImp: false, sacSuccubus: false, apl: null };
  assert.equal(identity.formatBuildName(candidate), '0/0/0');
  const original = identity.getIndUniqueKey(candidate);
  for (const edit of [{ race: 'GNOME' }, { rotation: 'FIRE_DESTRO' }, { pet: 'imp' },
    { sacImp: true }, { sacSuccubus: true }, { talents: Uint8Array.from(candidate.talents, (_, i) => i === 0 ? 1 : 0) }]) {
    assert.notEqual(identity.getIndUniqueKey({ ...candidate, ...edit }), original);
  }
  const early = { rules: ['tap', 'corr', 'bolt'].map(id => aplSearch.handcraftedRule(id)) };
  const late = { rules: early.rules.slice().reverse() };
  assert.notEqual(identity.getAPLUniqueKey(early), identity.getAPLUniqueKey(late));
  assert.equal(identity.getAPLMapElitesKey(early), 'tap_corr_bolt_early');
  assert.equal(identity.getAPLMapElitesKey(late), 'bolt_corr_bolt_late');
  assert.notEqual(identity.getIndUniqueKey({ ...candidate, apl: early }), original);
});

test('diversity keys ignore disabled fillers and retain empty-APL fallbacks', () => {
  const apl = { rules: [aplSearch.createRule('incinerate', 'NEVER'), aplSearch.handcraftedRule('searing')] };
  const candidate = { talents: new Uint8Array(nodeCount), pet: 'none', rotation: 'FIRE_DESTRO', apl };
  assert.equal(identity.getAPLMapElitesKey(apl), 'searing_none_searing_late');
  assert.equal(identity.getMapElitesKey(candidate).split('_').at(-1), '3');
  assert.equal(identity.getAPLMapElitesKey({ rules: [] }), 'none_none_bolt_late');
  assert.equal(identity.getMapElitesKey({ ...candidate, apl: { rules: [] } }).split('_').at(-1), '0');
});

test('preset seeding preserves inference priority, explicit options, locks, and input data', () => {
  const make = getClassModule('warlock').search.createPresetCandidate({
    resolveTalentFlags: getTalentFlagsFromRanks, resolvePetAndSac: () => ({ pet: 'succubus', sac: 'imp' }),
    enforceConstraints: buildSearch.enforceConstraints,
  });
  for (const [name, expected] of [['searing incin brand', 'DP_AF_FIRE'], ['incin brand', 'FIRE_DESTRO'],
    ['brand', 'DP_AF_SHADOW'], ['', 'SHADOW_DESTRO']]) {
    const p = { name, race: 'orc', pet: 'imp', sac: 'succubus', talents: {} };
    const saved = JSON.stringify(p);
    const ind = make(p, { aplMode: 'static' }, { ...aplRng(42), ...rng(42) });
    assert.equal(ind.rotation, expected);
    assert.equal(ind.race, 'ORC');
    assert.equal(ind.pet, 'imp');
    assert.equal(ind.apl, null);
    assert.equal(JSON.stringify(p), saved);
  }
  const ind = make({ name: 'brand' }, { forcedRace: 'GNOME', forcedRotation: 'FIRE_DESTRO',
    forcedPetMode: 'SAC_IMP', aplMode: 'evolve', lockConditions: false }, { ...aplRng(55), ...rng(55) });
  assert.equal(ind.race, 'GNOME');
  assert.equal(ind.rotation, 'FIRE_DESTRO');
  assert.equal(ind.pet, 'none');
  assert.equal(ind.sacImp, true);
  assert.equal(ind.talents[26], 1);
  assert.ok(graph.isValid(ind.talents));
  assert.equal(ind.fitness, 0);
  assert.equal(ind.batch, null);
  const flags = getTalentFlagsFromRanks(graph.toTalentsObject(ind.talents));
  assert.deepEqual(ind.apl.rules.map(r => r.id).sort(), aplSearch.availableActions(flags).map(a => a.id).sort());
});

const { results: searchResults } = getClassModule('warlock').search;

test('Warlock build results preserve classification precedence and damage accounting', () => {
  for (const [tree, category] of [[0, 'Affliction Peak'], [1, 'Demonology Peak'], [2, 'Destruction Peak']]) {
    const talents = new Uint8Array(nodeCount);
    let points = 0;
    for (let i = 0; i < nodeCount && points < 25; i++) if (definitions[i].tree === tree) {
      talents[i] = Math.min(definitions[i].max, 25 - points);
      points += talents[i];
    }
    const ind = { talents, fitness: 123, batch: { summary: { mean: 100, shadowDamage: 100,
      fireDamage: 50, petDamage: 50 }, states: [1] } };
    const result = searchResults.createCandidateResult(ind, 2);
    assert.equal(result.category, category);
    assert.equal(result.rank, 2);
    assert.equal(result.mean_dps, 100);
    assert.equal(result.shadow_pct, 50);
    assert.equal(result.fire_pct, 25);
    assert.equal(result.pet_pct, 25);
    assert.equal(result.individual, ind);
    assert.equal(result.summary, ind.batch.summary);
    assert.equal(result.states, ind.batch.states);
  }
  assert.equal(searchResults.createCandidateResult({ talents: new Uint8Array(nodeCount) }).category, 'Hybrid Peak');
});

test('APL result naming excludes disabled rules and preserves result references', () => {
  const ind = { rules: [aplSearch.createRule('incinerate', 'NEVER'),
    aplSearch.handcraftedRule('hellfire'), aplSearch.handcraftedRule('corr')], fitness: 123, batch: null };
  assert.equal(searchResults.formatAPLName(ind), 'Hellfire (Corr/Hellfire · 2 Active Rules)');
  const result = searchResults.createCandidateAPLResult(ind);
  assert.equal(result.rules, ind.rules);
  assert.equal(result.activeRulesCount, 2);
  assert.equal(result.meanDps, 123);
  assert.equal(result.aplRules[0].enabled, 0);
  assert.equal(searchResults.formatAPLName({ rules: [] }), 'Shadow Bolt (Direct · 0 Active Rules)');
});
