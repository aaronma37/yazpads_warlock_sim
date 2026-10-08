// Conservative source fingerprints: any source change requires an explicit compatibility review.
// These IDs describe GPU format/mechanics compatibility, not CPU parity.
export const WARLOCK_VERSIONS = Object.freeze({
  "packingVersion": "warlock-packing-sha256-c0ec9327a5986d3c353f026ef208e19b6641d3dcf6a9650ea1d782af0df7292b",
  "simulationVersion": "warlock-simulation-sha256-c59fcfdef5b712f64de95c04e7229c469b66c6ddfb1ce9e2e4dcff79b5fe7028"
});

export const WARLOCK_VERSION_SOURCE_HASHES = Object.freeze({
  "src/model.js": "c0ec9327a5986d3c353f026ef208e19b6641d3dcf6a9650ea1d782af0df7292b",
  "src/kernel.js": "57a91b82ab0bb66df14a9ca5cdfc7c978d40db141557ccc683d917dd1c399f0f",
  "src/config_builder.js": "a01b78cebc2529c569c9bdaaf11a057dd6493cfdd181bfebf5a42d7257c46f07"
});
