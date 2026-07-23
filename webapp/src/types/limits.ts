export const limits = {
  macroNameBytes: 64,
  macroSourceBytes: 4096,
  compiledActions: 4096,
  delayMs: 10_000,
  durationMs: 300_000,
  macrosPerSet: 100,
  proceduresPerSet: 50,
  stepsPerProcedure: 200,
  macroSets: 50,
  importBytes: 512 * 1024,
} as const;

export type Limits = typeof limits;
