import { v2Limits } from "./limits";
import type {
  FactoryResetRequest,
  LoginRequest,
  NetworkCredentialsRequest,
  PasswordChangeRequest,
  ResetSettingsRequest,
  SendRequest,
  SettingsUpdateRequest,
  SetupRequest,
} from "./apiTypes";

const encoder = new TextEncoder();
const uuidV4Pattern =
  /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const setupCodePattern = /^[0-9]{8}$/;

const settingsUpdateKeys = [
  "accessPoint",
  "deviceName",
  "lastSelectedPackageId",
  "requireSerialConfirmation",
  "sendMode",
  "showMacroSourcePreviews",
  "snapshotRetentionTarget",
  "station",
] as const;

function isRecord(value: unknown): value is Record<string, unknown> {
  return (
    typeof value === "object" &&
    value !== null &&
    !Array.isArray(value) &&
    Object.getPrototypeOf(value) === Object.prototype
  );
}

function hasExactKeys(
  value: Record<string, unknown>,
  keys: readonly string[],
): boolean {
  const actual = Object.keys(value).sort();
  const expected = [...keys].sort();
  return (
    actual.length === expected.length &&
    actual.every((key, index) => key === expected[index])
  );
}

function hasOnlyAllowedKeys(
  value: Record<string, unknown>,
  keys: readonly string[],
): boolean {
  return Object.keys(value).every((key) => keys.includes(key));
}

function byteLength(value: string): number {
  return encoder.encode(value).byteLength;
}

function isStringWithinBytes(
  value: unknown,
  minimum: number,
  maximum: number,
): value is string {
  return (
    typeof value === "string" &&
    byteLength(value) >= minimum &&
    byteLength(value) <= maximum
  );
}

function isCanonicalUuidV4(value: unknown): value is string {
  return typeof value === "string" && uuidV4Pattern.test(value);
}

function isBoundedInteger(
  value: unknown,
  minimum: number,
  maximum: number,
): value is number {
  return (
    typeof value === "number" &&
    Number.isSafeInteger(value) &&
    Number.isFinite(value) &&
    value >= minimum &&
    value <= maximum
  );
}

function isNetworkCredentials(
  value: unknown,
): value is NetworkCredentialsRequest {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["passphrase", "ssid"]) &&
    isStringWithinBytes(value.ssid, 1, 32) &&
    isStringWithinBytes(value.passphrase, 8, 63)
  );
}

export function isSetupRequest(value: unknown): value is SetupRequest {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "adminPassword",
      "apPassphrase",
      "apSsid",
      "deviceName",
      "requireSerialConfirmation",
      "setupCode",
    ]) &&
    typeof value.setupCode === "string" &&
    setupCodePattern.test(value.setupCode) &&
    isStringWithinBytes(value.deviceName, 1, 32) &&
    isStringWithinBytes(value.apSsid, 1, 32) &&
    isStringWithinBytes(value.apPassphrase, 8, 63) &&
    isStringWithinBytes(
      value.adminPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    ) &&
    typeof value.requireSerialConfirmation === "boolean"
  );
}

export function isLoginRequest(value: unknown): value is LoginRequest {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["adminPassword"]) &&
    isStringWithinBytes(
      value.adminPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    )
  );
}

export function isSettingsUpdateRequest(
  value: unknown,
): value is SettingsUpdateRequest {
  if (
    !isRecord(value) ||
    Object.keys(value).length === 0 ||
    !hasOnlyAllowedKeys(value, settingsUpdateKeys)
  ) {
    return false;
  }

  return (
    (!Object.hasOwn(value, "deviceName") ||
      isStringWithinBytes(value.deviceName, 1, 32)) &&
    (!Object.hasOwn(value, "requireSerialConfirmation") ||
      typeof value.requireSerialConfirmation === "boolean") &&
    (!Object.hasOwn(value, "sendMode") ||
      value.sendMode === "quick" ||
      value.sendMode === "preview") &&
    (!Object.hasOwn(value, "snapshotRetentionTarget") ||
      isBoundedInteger(
        value.snapshotRetentionTarget,
        0,
        v2Limits.snapshotRetentionTargetMax,
      )) &&
    (!Object.hasOwn(value, "showMacroSourcePreviews") ||
      typeof value.showMacroSourcePreviews === "boolean") &&
    (!Object.hasOwn(value, "lastSelectedPackageId") ||
      value.lastSelectedPackageId === null ||
      isCanonicalUuidV4(value.lastSelectedPackageId)) &&
    (!Object.hasOwn(value, "accessPoint") ||
      isNetworkCredentials(value.accessPoint)) &&
    (!Object.hasOwn(value, "station") ||
      value.station === null ||
      isNetworkCredentials(value.station))
  );
}

export function isPasswordChangeRequest(
  value: unknown,
): value is PasswordChangeRequest {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["currentPassword", "newPassword"]) &&
    isStringWithinBytes(
      value.currentPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    ) &&
    isStringWithinBytes(
      value.newPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    )
  );
}

export function isSendRequest(value: unknown): value is SendRequest {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["interKeyMs", "keyPressMs", "source"]) &&
    typeof value.source === "string" &&
    byteLength(value.source) <= v2Limits.macroSourceMaxBytes &&
    isBoundedInteger(value.keyPressMs, 0, v2Limits.keyPressMaxMs) &&
    isBoundedInteger(value.interKeyMs, 0, v2Limits.interKeyMaxMs)
  );
}

export function isResetSettingsRequest(
  value: unknown,
): value is ResetSettingsRequest {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["confirmation"]) &&
    value.confirmation === "RESET SETTINGS"
  );
}

export function isFactoryResetRequest(
  value: unknown,
): value is FactoryResetRequest {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["adminPassword", "confirmation"]) &&
    isStringWithinBytes(
      value.adminPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    ) &&
    value.confirmation === "FACTORY RESET"
  );
}
