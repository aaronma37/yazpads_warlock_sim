const CACHE_NAME = 'warlock-sim-v70';
const ASSETS = [
  './src/detailed_results.js',
  "./assets/icons/Achievement_Character_Human_Male.png",
  "./assets/icons/Achievement_Character_Undead_Male.png",
  "./assets/icons/Inv_Misc_Tournaments_Symbol_Dwarf.png",
  "./assets/icons/Inv_Misc_Tournaments_Symbol_Gnome.png",
  "./assets/icons/Inv_Misc_Tournaments_Symbol_Troll.png",
  "./assets/icons/ability_priest_savinggrace.png",
  "./assets/icons/spell_shadow_ritualofsacrifice.png",
  "./assets/icons/Spell_Holy_MagicalSentry.png",
  "./assets/icons/spell_holy_powerinfusion_shadow.png",
  "./assets/icons/spell_shadow_deadofnight.png",
  "./assets/icons/Spell_Shadow_FingerOfDeath.png",
  "./assets/icons/spell_holy_elunesgrace.png",
  "./assets/icons/Spell_Holy_ElunesGrace.png",
  "./assets/icons/spell_holy_chastise.png",
  "./assets/icons/spell_holy_restoration.png",
  "./assets/icons/Ability_Hunter_SniperShot.png",
  "./assets/icons/INV_Mace_01.png",
  "./assets/icons/ability_paladin_blindinglight2.png",
  "./assets/icons/ability_priest_soulwarding.png",
  "./assets/icons/Spell_Arcane_MindMastery.png",
  "./assets/icons/spell_shadow_fingerofdeath.png",
  "./assets/icons/spell_nature_lightningshield.png",
  "./assets/icons/Racial_Troll_Berserk.png",
  "./assets/icons/Ability_Mount_Raptor.png",
  './src/priest/racials.js',
  './src/priest/racial_kernel.js',
  './assets/icons/spell_arcane_starfire.png',
  './assets/icons/Inv_Misc_Tournaments_Symbol_NightElf.png',
  './src/priest/policy.js',
  './src/priest/search.js',
  './src/priest/search_view.js',
  './src/classes/priest_simulation.js',
  './src/priest/model.js',
  './src/priest/talents.js',
  './src/priest/ui_config.js',
  './src/priest/run_controller.js',
  './src/priest/talent_data.js',
  './src/priest/kernel.js',
  './src/priest/rng.js',
  './src/priest/results.js',
  './src/gpu/compact_des_engine.js',

  './assets/icons/Spell_Shadow_UnsummonBuilding.png',
  './assets/icons/Spell_Shadow_UnholyFrenzy.png',
  './assets/icons/Spell_Shadow_Twilight.png',
  './assets/icons/Spell_Shadow_SoulLeech_2.png',
  './assets/icons/Spell_Shadow_SiphonMana.png',
  './assets/icons/Spell_Shadow_Shadowform.png',
  './assets/icons/Spell_Shadow_ShadowWordPain.png',
  './assets/icons/Spell_Shadow_ShadowWard.png',
  './assets/icons/Spell_Shadow_Requiem.png',
  './assets/icons/Spell_Shadow_PsychicScream.png',
  './assets/icons/Spell_Shadow_ManaBurn.png',
  './assets/icons/Spell_Shadow_ImpPhaseShift.png',
  './assets/icons/Spell_Shadow_GatherShadows.png',
  './assets/icons/Spell_Shadow_DevouringPlague.png',
  './assets/icons/Spell_Shadow_DemonicFortitude.png',
  './assets/icons/Spell_Shadow_ChillTouch.png',
  './assets/icons/Spell_Shadow_BurningSpirit.png',
  './assets/icons/Spell_Shadow_BlackPlague.png',
  './assets/icons/Spell_Nature_Tranquility.png',
  './assets/icons/Spell_Nature_Sleep.png',
  './assets/icons/Spell_Nature_MoonGlow.png',
  './assets/icons/Spell_Nature_ManaRegenTotem.png',
  './assets/icons/Spell_Nature_EnchantArmor.png',
  './assets/icons/Spell_Magic_LesserInvisibilty.png',
  './assets/icons/Spell_Holy_SpiritualGuidence.png',
  './assets/icons/Spell_Holy_SpellWarding.png',
  './assets/icons/Spell_Holy_SearingLightPriest.png',
  './assets/icons/Spell_Holy_SearingLight.png',
  './assets/icons/Spell_Holy_SealOfWrath.png',
  './assets/icons/Spell_Holy_SealOfVengeance.png',
  './assets/icons/Spell_Holy_SealOfSalvation.png',
  './assets/icons/Spell_Holy_Renew.png',
  './assets/icons/Spell_Holy_Purify.png',
  './assets/icons/Spell_Holy_PureOfHeart.png',
  './assets/icons/Spell_Holy_PrayerOfMendingtga.png',
  './assets/icons/Spell_Holy_PowerWordShield.png',
  './assets/icons/Spell_Holy_PowerInfusion.png',
  './assets/icons/Spell_Holy_Penance.png',
  './assets/icons/Spell_Holy_LayOnHands.png',
  './assets/icons/Spell_Holy_InnerFire.png',
  './assets/icons/Spell_Holy_HolyProtection.png',
  './assets/icons/Spell_Holy_HolyNova.png',
  './assets/icons/Spell_Holy_HealingFocus.png',
  './assets/icons/Spell_Holy_Heal02.png',
  './assets/icons/Spell_Holy_DivineIllumination.png',
  './assets/icons/Spell_Holy_DevineAegis.png',
  './assets/icons/Spell_Holy_BlindingHeal.png',
  './assets/icons/Spell_Holy_BlessedRecovery.png',
  './assets/icons/Spell_Frost_WindWalkOn.png',
  './assets/icons/INV_Wand_01.png',
  './assets/icons/INV_Scroll_07.png',
  './assets/icons/INV_Enchant_EssenceEternalLarge.png',
  './assets/icons/Ability_Hibernation.png',
  './',
  './index.html',
  './style.css',
  './src/app.js',
  './src/priest_preview.js',
  './src/shared_configuration.js',
  './data/priest_preview.json',
  './src/contracts/ui_build.js',
  './src/contracts/evaluation.js',
  './src/contracts/native_evaluation.js',
  './src/contracts/build_io.js',
  './src/contracts/saved_build.js',
  './src/contracts/legacy_build.js',
  './src/classes/warlock_presentation.js',
  './src/classes/registry.js',
  './src/classes/capabilities.js',
  './src/classes/runtime.js',
  './src/classes/warlock_search_dependencies.js',
  './src/classes/warlock_preset_options.js',
  './src/search/genetic_optimizer.js',
  './src/search/apl_genetic_optimizer.js',
  './src/classes/warlock_versions.js',
  './src/classes/active_class.js',
  './src/classes/active_simulation.js',
  './src/classes/warlock_simulation.js',
  './src/classes/warlock_search.js',
  './src/classes/warlock_policy_apl.js',
  './src/classes/warlock_apl_search.js',
  './src/classes/warlock_build_search.js',
  './src/classes/warlock_candidate_config.js',
  './src/classes/warlock_search_identity.js',
  './src/classes/warlock_search_presets.js',
  './src/classes/warlock_search_results.js',
  './src/classes/warlock_import.js',
  './src/classes/warlock_equipment.js',
  './src/classes/warlock_import_buffs.js',
  './src/config_builder.js',
  './src/apl.js',
  './src/apl_rules.js',
  './assets/icons/Spell_Fire_Incinerate.png',
  './src/apl_fallback_view.js',
  './src/engine.js',
  './src/regret.js',
  './src/regret_view.js',
  './src/kernel.js',
  './src/model.js',
  './src/talents.js',
  './src/buffs.js',
  './src/presets.js',
  './data/talents.json',
  './data/items.json',
  './data/presets.json',
  './vendor/three.module.js'
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      return cache.addAll(ASSETS).catch((err) => console.warn('Cache prefetch error:', err));
    })
  );
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) => {
      return Promise.all(
        keys.map((key) => {
          if (key !== CACHE_NAME) return caches.delete(key);
        })
      );
    })
  );
  self.clients.claim();
});

self.addEventListener('fetch', (event) => {
  if (event.request.method !== 'GET') return;
  event.respondWith(
    caches.match(event.request).then((cached) => {
      const networked = fetch(event.request).then((response) => {
        if (response.status === 200) {
          const cacheCopy = response.clone();
          caches.open(CACHE_NAME).then((cache) => cache.put(event.request, cacheCopy));
        }
        return response;
      }).catch(() => cached);
      return cached || networked;
    })
  );
});
