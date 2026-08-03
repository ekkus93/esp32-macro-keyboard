import { v2Limits } from "./limits";
import type {
  LoginRequest,
  NetworkCredentialsRequest,
  PasswordChangeRequest,
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

function all(checks: readonly boolean[]): boolean {
  return checks.every(Boolean);
}

function isRecord(value: unknown): value is Record<string, unknown> {
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    return false;
  }
  return Object.getPrototypeOf(value) === Object.prototype;
}

function hasExactKeys(
  value: Record<string, unknown>,
  keys: readonly string[],
): boolean {
  const actual = Object.keys(value).sort();
  const expected = [...keys].sort();
  return all([
    actual.length === expected.length,
    actual.every((key, index) => key === expected[index]),
  ]);
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
  if (typeof value !== "string") {
    return false;
  }
  const length = byteLength(value);
  return all([length >= minimum, length <= maximum]);
}

function isCanonicalUuidV4(value: unknown): value is string {
  return typeof value === "string" && uuidV4Pattern.test(value);
}

function isBoundedInteger(
  value: unknown,
  minimum: number,
  maximum: number,
): value is number {
  if (typeof value !== "number") {
    return false;
  }
  return all([
    Number.isSafeInteger(value),
    Number.isFinite(value),
    value >= minimum,
    value <= maximum,
  ]);
}

function isNetworkCredentials(
  value: unknown,
): value is NetworkCredentialsRequest {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["passphrase", "ssid"]),
    isStringWithinBytes(value.ssid, 1, 32),
    isStringWithinBytes(value.passphrase, 8, 63),
  ]);
}

function isSetupIdentity(value: Record<string, unknown>): boolean {
  return all([
    typeof value.setupCode === "string",
    typeof value.setupCode === "string" && setupCodePattern.test(value.setupCode),
    isStringWithinBytes(value.deviceName, 1, 32),
  ]);
}

function isSetupCredentials(value: Record<string, unknown>): boolean {
  return all([
    isStringWithinBytes(value.apSsid, 1, 32),
    isStringWithinBytes(value.apPassphrase, 8, 63),
    isStringWithinBytes(
      value.adminPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    ),
  ]);
}

export function isSetupRequest(value: unknown): value is SetupRequest {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "adminPassword",
      "apPassphrase",
      "apSsid",
      "deviceName",
      "requireSerialConfirmation",
      "setupCode",
    ]),
    isSetupIdentity(value),
    isSetupCredentials(value),
    typeof value.requireSerialConfirmation === "boolean",
  ]);
}

export function isLoginRequest(value: unknown): value is LoginRequest {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["adminPassword"]),
    isStringWithinBytes(
      value.adminPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    ),
  ]);
}

function optionalField(
  value: Record<string, unknown>,
  key: string,
  validator: (candidate: unknown) => boolean,
): boolean {
  if (!Object.hasOwn(value, key)) {
    return true;
  }
  return validator(value[key]);
}

function isOptionalDeviceSettings(value: Record<string, unknown>): boolean {
  return all([
    optionalField(value, "deviceName", (candidate) =>
      isStringWithinBytes(candidate, 1, 32),
    ),
    optionalField(
      value,
      "requireSerialConfirmation",
      (candidate) => typeof candidate === "boolean",
    ),
    optionalField(
      value,
      "sendMode",
      (candidate) => candidate === "quick" || candidate === "preview",
    ),
    optionalField(value, "snapshotRetentionTarget", (candidate) =>
      isBoundedInteger(candidate, 0, v2Limits.snapshotRetentionTargetMax),
    ),
  ]);
}

function isOptionalPresentationSettings(
  value: Record<string, unknown>,
): boolean {
  return all([
    optionalField(
      value,
      "showMacroSourcePreviews",
      (candidate) => typeof candidate === "boolean",
    ),
    optionalField(
      value,
      "lastSelectedPackageId",
      (candidate) => candidate === null || isCanonicalUuidV4(candidate),
    ),
  ]);
}

function isOptionalNetworkSettings(value: Record<string, unknown>): boolean {
  return all([
    optionalField(value, "accessPoint", isNetworkCredentials),
    optionalField(
      value,
      "station",
      (candidate) => candidate === null || isNetworkCredentials(candidate),
    ),
  ]);
}

export function isSettingsUpdateRequest(
  value: unknown,
): value is SettingsUpdateRequest {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    Object.keys(value).length > 0,
    hasOnlyAllowedKeys(value, settingsUpdateKeys),
    isOptionalDeviceSettings(value),
    isOptionalPresentationSettings(value),
    isOptionalNetworkSettings(value),
  ]);
}

export function isPasswordChangeRequest(
  value: unknown,
): value is PasswordChangeRequest {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["currentPassword", "newPassword"]),
    isStringWithinBytes(
      value.currentPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    ),
    isStringWithinBytes(
      value.newPassword,
      v2Limits.adminPasswordMinBytes,
      v2Limits.adminPasswordMaxBytes,
    ),
  ]);
}

export function isSendRequest(value: unknown): value is SendRequest {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["interKeyMs", "keyPressMs", "source"]),
    typeof value.source === "string",
    typeof value.source === "string" &&
      byteLength(value.source) <= v2Limits.macroSourceMaxBytes,
    isBoundedInteger(value.keyPressMs, 0, v2Limits.keyPressMaxMs),
    isBoundedInteger(value.interKeyMs, 0, v2Limits.interKeyMaxMs),
  ]);
}
