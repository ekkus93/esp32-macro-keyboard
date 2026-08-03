export const v2Limits = {
  packageNameMaxBytes: 64,
  macroNameMaxBytes: 64,
  macroSourceMaxBytes: 4096,
  compiledActionsMax: 4096,
  delayDirectiveMaxMs: 10_000,
  keyPressMaxMs: 10_000,
  interKeyMaxMs: 10_000,
  estimatedDurationMaxMs: 300_000,
  executorAbsoluteDeadlineMs: 310_000,
  jsonBodyMaxBytes: 8192,
  blobMaxBytes: 131_072,
  activeSessionsMax: 8,
  sessionIdleLifetimeSeconds: 86_400,
  sessionAbsoluteLifetimeSeconds: 604_800,
  serialConfirmationTimeoutSeconds: 60,
  adminPasswordMinBytes: 12,
  adminPasswordMaxBytes: 128,
  snapshotRetentionTargetMax: 100,
} as const;

export type V2Limits = typeof v2Limits;
