import { limits } from "../types/limits";
import type {
  FactoryResetAccepted,
  QuarantineEntry,
  QuarantineList,
  SetDeletion,
  StorageHealth,
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
