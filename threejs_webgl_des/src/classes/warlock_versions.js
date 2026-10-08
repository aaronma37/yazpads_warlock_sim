// Conservative source fingerprints: any source change requires an explicit compatibility review.
// These IDs describe GPU format/mechanics compatibility, not CPU parity.
export const WARLOCK_VERSIONS = Object.freeze({
  "packingVersion": "warlock-packing-sha256-7fce326bd5d45dce1b43c252be1f7a3ecef8628154c0ba417846f856ebb2d344",
  "simulationVersion": "warlock-simulation-sha256-2d4b99261adddd3918399402d29f9001d1edd3d27e710f63f90affc15e260f63"
});

export const WARLOCK_VERSION_SOURCE_HASHES = Object.freeze({
  "src/model.js": "7fce326bd5d45dce1b43c252be1f7a3ecef8628154c0ba417846f856ebb2d344",
  "src/kernel.js": "f3d7823db1c14a4e06b88bbcfca5db546b1a9c870a6263b6f362b0d7ae2f449c",
  "src/config_builder.js": "a01b78cebc2529c569c9bdaaf11a057dd6493cfdd181bfebf5a42d7257c46f07"
});
