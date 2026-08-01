import { limits } from "../types/limits";
import type {
  DiagnosticsCapacity,
  DiagnosticsQuarantineSummary,
  DiagnosticsStack,
  DiagnosticsSubsystem,
  FactoryResetAccepted,
  FullDiagnostics,
  QuarantineEntry,
  QuarantineList,
  SetDeletion,
  StorageHealth,
  SubsystemHealthState,
} from "../types/models";
import { isRecord, isUuid } from "./guards";

function hasExactKeys(
  value: Record<string, unknown>,
  expected: readonly string[],
): boolean {
  const actual = Object.keys(value);
  return (
    actual.length === expected.length &&
    expected.every((key) => Object.prototype.hasOwnProperty.call(value, key))
  );
}

function isNonNegativeInteger(value: unknown): value is number {
  return typeof value === "number" && Number.isInteger(value) && value >= 0;
}

function isBoundedString(
  value: unknown,
  maximumBytes: number,
): value is string {
  return (
    typeof value === "string" &&
    new TextEncoder().encode(value).byteLength <= maximumBytes
  );
}

export function isSetDeletion(value: unknown): value is SetDeletion {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["deleted", "id"]) &&
    value.deleted === true &&
    isUuid(value.id)
  );
}

export function isFactoryResetAccepted(
  value: unknown,
): value is FactoryResetAccepted {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["factoryReset", "restartScheduled"]) &&
    value.factoryReset === true &&
    value.restartScheduled === true
  );
}

export function isStorageHealth(value: unknown): value is StorageHealth {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "verified",
      "webMounted",
      "dataMounted",
      "quarantineCount",
      "damagedQuarantineCount",
    ]) &&
    typeof value.verified === "boolean" &&
    typeof value.webMounted === "boolean" &&
    typeof value.dataMounted === "boolean" &&
    isNonNegativeInteger(value.quarantineCount) &&
    isNonNegativeInteger(value.damagedQuarantineCount) &&
    value.damagedQuarantineCount <= value.quarantineCount
  );
}

function isQuarantineEntry(value: unknown): value is QuarantineEntry {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["id", "sourcePath", "evidencePath", "reason"]) &&
    isUuid(value.id) &&
    isBoundedString(value.sourcePath, 512) &&
    isBoundedString(value.evidencePath, 512) &&
    isBoundedString(value.reason, limits.descriptionBytes)
  );
}

export function isQuarantineList(value: unknown): value is QuarantineList {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["damagedCount", "items"]) &&
    isNonNegativeInteger(value.damagedCount) &&
    Array.isArray(value.items) &&
    value.items.every(isQuarantineEntry) &&
    value.damagedCount <= value.items.length &&
    new Set(value.items.map((entry) => entry.id)).size === value.items.length
  );
}

const SUBSYSTEM_HEALTH_STATES: readonly SubsystemHealthState[] = [
  "healthy",
  "degraded",
  "unavailable",
  "recovering",
  "failed",
  "unknown",
];

function isSubsystemHealthState(value: unknown): value is SubsystemHealthState {
  return (
    typeof value === "string" &&
    (SUBSYSTEM_HEALTH_STATES as readonly string[]).includes(value)
  );
}

function isDiagnosticsSubsystem(value: unknown): value is DiagnosticsSubsystem {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["name", "state"]) &&
    isBoundedString(value.name, 64) &&
    isSubsystemHealthState(value.state)
  );
}

function isDiagnosticsCapacity(value: unknown): value is DiagnosticsCapacity {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["ok", "totalBytes", "usedBytes"]) &&
    typeof value.ok === "boolean" &&
    isNonNegativeInteger(value.totalBytes) &&
    isNonNegativeInteger(value.usedBytes) &&
    value.usedBytes <= value.totalBytes
  );
}

function isDiagnosticsQuarantineSummary(
  value: unknown,
): value is DiagnosticsQuarantineSummary {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["ok", "count", "damagedCount"]) &&
    typeof value.ok === "boolean" &&
    isNonNegativeInteger(value.count) &&
    isNonNegativeInteger(value.damagedCount) &&
    value.damagedCount <= value.count
  );
}

function isDiagnosticsStack(value: unknown): value is DiagnosticsStack {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["controlsWords", "executorWords"]) &&
    isNonNegativeInteger(value.controlsWords) &&
    isNonNegativeInteger(value.executorWords)
  );
}

export function isFullDiagnostics(value: unknown): value is FullDiagnostics {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "buildId",
      "firmwareVersion",
      "schemaVersion",
      "resetReason",
      "uptimeMs",
      "freeHeapBytes",
      "minFreeHeapBytes",
      "stack",
      "webfs",
      "userdata",
      "quarantine",
      "executionState",
      "subsystems",
    ]) &&
    isBoundedString(value.buildId, 64) &&
    isBoundedString(value.firmwareVersion, 64) &&
    isNonNegativeInteger(value.schemaVersion) &&
    isBoundedString(value.resetReason, 64) &&
    isNonNegativeInteger(value.uptimeMs) &&
    isNonNegativeInteger(value.freeHeapBytes) &&
    isNonNegativeInteger(value.minFreeHeapBytes) &&
    isDiagnosticsStack(value.stack) &&
    isDiagnosticsCapacity(value.webfs) &&
    isDiagnosticsCapacity(value.userdata) &&
    isDiagnosticsQuarantineSummary(value.quarantine) &&
    isBoundedString(value.executionState, 64) &&
    Array.isArray(value.subsystems) &&
    value.subsystems.length === 9 &&
    value.subsystems.every(isDiagnosticsSubsystem)
  );
}
