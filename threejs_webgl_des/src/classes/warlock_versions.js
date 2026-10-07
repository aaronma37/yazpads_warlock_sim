// Conservative source fingerprints: any source change requires an explicit compatibility review.
// These IDs describe GPU format/mechanics compatibility, not CPU parity.
export const WARLOCK_VERSIONS = Object.freeze({
  "packingVersion": "warlock-packing-sha256-920ef0de3dd27d11d76fab52308cbd9d010cf9e21f27088bc8e3a80e0dd45588",
  "simulationVersion": "warlock-simulation-sha256-745ce44e669574b5e9ee67937207757fb305a6256645bb5e084c10d8a9c6a1d9"
});

export const WARLOCK_VERSION_SOURCE_HASHES = Object.freeze({
  "src/model.js": "920ef0de3dd27d11d76fab52308cbd9d010cf9e21f27088bc8e3a80e0dd45588",
  "src/kernel.js": "41276d1c0996bf3dee46a525c82fb23b9c92bc731e1c6c446e78e578172ab17f",
  "src/config_builder.js": "a01b78cebc2529c569c9bdaaf11a057dd6493cfdd181bfebf5a42d7257c46f07"
});
