export const limits = {
  packageNameBytes: 64,
  descriptionBytes: 256,
  manufacturerBytes: 64,
  modelBytes: 64,
  boardBytes: 32,
  macroNameBytes: 64,
  macroSourceBytes: 4096,
  compiledActions: 4096,
  delayMs: 10_000,
  durationMs: 300_000,
  macrosPerPackage: 100,
  macroPackages: 50,
  importBytes: 512 * 1024,
} as const;

export type Limits = typeof limits;
