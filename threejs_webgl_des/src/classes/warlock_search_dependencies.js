import { getTalentFlagsFromRanks } from '../talents.js';
import { getPresetPetAndSac } from './warlock_preset_options.js';

export const WARLOCK_SEARCH_DEPENDENCIES = Object.freeze({
  resolveTalentFlags: getTalentFlagsFromRanks, resolvePetAndSac: getPresetPetAndSac,
});
