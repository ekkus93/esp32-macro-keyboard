import {
  STANDARD_VIEWPORTS,
  setReviewColorScheme,
} from "./scenarioSupport.mjs";
import { SCENARIOS as macroScenarios } from "./macroScenarios.mjs";
import { SCENARIOS as repositoryScenarios } from "./repositoryScenarios.mjs";
import { SCENARIOS as deviceScenarios } from "./deviceScenarios.mjs";
import { SCENARIOS as startupScenarios } from "./startupScenarios.mjs";

export { STANDARD_VIEWPORTS, setReviewColorScheme };

export const SCENARIOS = [
  ...macroScenarios,
  ...repositoryScenarios,
  ...deviceScenarios,
  ...startupScenarios,
];
