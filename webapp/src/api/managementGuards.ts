import type {
  DiagnosticsCapacity,
  DiagnosticsStack,
  DiagnosticsSubsystem,
  FactoryResetAccepted,
  FullDiagnostics,
  SetDeletion,
  DiscardedObject,
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

function isDiscardedObject(value: unknown): value is DiscardedObject {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["path", "error"]) &&
    typeof value.path === "string" &&
    typeof value.error === "string"
  );
}

export function isStorageHealth(value: unknown): value is StorageHealth {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "verified",
      "webMounted",
      "dataMounted",
      "usedBytes",
      "totalBytes",
      "remainingBytes",
      "setFileMaxBytes",
      "temporariesRemovedAtBoot",
      "discardedObjectCount",
      "discardedObjects",
    ]) &&
    typeof value.verified === "boolean" &&
    typeof value.webMounted === "boolean" &&
    typeof value.dataMounted === "boolean" &&
    isNonNegativeInteger(value.usedBytes) &&
    isNonNegativeInteger(value.totalBytes) &&
    isNonNegativeInteger(value.remainingBytes) &&
    isNonNegativeInteger(value.setFileMaxBytes) &&
    isNonNegativeInteger(value.temporariesRemovedAtBoot) &&
    isNonNegativeInteger(value.discardedObjectCount) &&
    Array.isArray(value.discardedObjects) &&
    value.discardedObjects.every(isDiscardedObject)
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
    isBoundedString(value.executionState, 64) &&
    Array.isArray(value.subsystems) &&
    value.subsystems.length === 9 &&
    value.subsystems.every(isDiagnosticsSubsystem)
  );
}
