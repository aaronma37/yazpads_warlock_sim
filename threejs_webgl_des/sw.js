const CACHE_NAME = 'warlock-sim-v44';
const ASSETS = [
  './',
  './index.html',
  './style.css',
  './src/app.js',
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
