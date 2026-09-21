// Self-unregistering service worker to cleanly purge old coi-serviceworker caches
self.addEventListener('install', (e) => {
    self.skipWaiting();
});
self.addEventListener('activate', (e) => {
    self.registration.unregister().then(() => {
        return self.clients.matchAll();
    }).then(clients => {
        // Unregistered successfully
    });
});
