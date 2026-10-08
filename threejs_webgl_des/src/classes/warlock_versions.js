// Conservative source fingerprints: any source change requires an explicit compatibility review.
// Detailed observation layout changed; configuration packing and fast shader are unchanged.
// These IDs describe GPU format/mechanics compatibility, not CPU parity.
export const WARLOCK_VERSIONS = Object.freeze({
  "packingVersion": "warlock-packing-sha256-093699d1a27311b964216abcced6610e4386e48160d2f2d4596ba8f7e0c9d4d0",
  "simulationVersion": "warlock-simulation-sha256-99deec69dd0bdeeaefa3173db34b1a382a3d6eabfc677ebc334b27bc09a2e299"
});

export const WARLOCK_VERSION_SOURCE_HASHES = Object.freeze({
  "src/model.js": "093699d1a27311b964216abcced6610e4386e48160d2f2d4596ba8f7e0c9d4d0",
  "src/kernel.js": "b191a8319465492bb194785b09d336e510379fc5642f8d61380818cc6e047f7e",
  "src/config_builder.js": "a01b78cebc2529c569c9bdaaf11a057dd6493cfdd181bfebf5a42d7257c46f07"
});
