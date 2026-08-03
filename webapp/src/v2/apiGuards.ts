import { v2Limits } from "./limits";
import type {
  ActionAccepted,
  BlobCreatedResponse,
  BlobListResponse,
  BlobSummary,
  DiagnosticsResponse,
  ErrorEnvelope,
  FactoryResetRequest,
  LimitsResponse,
  ResetAccepted,
  ResetSettingsRequest,
  SendAcceptedResponse,
  SendState,
  SendStatusResponse,
  SessionStatus,
  SettingsResponse,
  SettingsUpdatedResponse,
  SetupAccepted,
  SetupStateResponse,
  StatusResponse,
  SubsystemHealthState,
  UsbState,
} from "./apiTypes";

const uuidV4Pattern =
  /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
const blobIdPattern = /^[1-9][0-9]*$/;

const usbStates = new Set<UsbState>([
  "uninitialized",
  "disconnected",
  "enumerating",
  "ready",
  "suspended",
  "error",
]);
const sendStates = new Set<SendState>([
  "awaiting_confirmation",
  "running",
  "completed",
  "cancelled",
  "failed",
  "timed_out",
]);
const subsystemHealthStates = new Set<SubsystemHealthState>([
  "healthy",
  "degraded",
  "unavailable",
  "recovering",
  "failed",
  "unknown",
]);

function all(checks: readonly boolean[]): boolean {
  return checks.every(Boolean);
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return all([
    typeof value === "object",
    value !== null,
    !Array.isArray(value),
    value !== null && Object.getPrototypeOf(value) === Object.prototype,
  ]);
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

function hasAllowedKeys(
  value: Record<string, unknown>,
  required: readonly string[],
  allowed: readonly string[],
): boolean {
  const actual = Object.keys(value);
  return all([
    required.every((key) => Object.hasOwn(value, key)),
    actual.every((key) => allowed.includes(key)),
  ]);
}

function isDenseArray(value: unknown[]): boolean {
  return value.every((_, index) => Object.hasOwn(value, index));
}

function isNonNegativeInteger(value: unknown): value is number {
  return all([
    typeof value === "number",
    typeof value === "number" && Number.isSafeInteger(value),
    typeof value === "number" && Number.isFinite(value),
    typeof value === "number" && value >= 0,
  ]);
}

function isString(value: unknown): value is string {
  return typeof value === "string";
}

function isNonEmptyString(value: unknown): value is string {
  return typeof value === "string" && value.length > 0;
}

function isNullableString(value: unknown): value is string | null {
  return value === null || typeof value === "string";
}

function isStringArray(value: unknown): value is string[] {
  if (!Array.isArray(value)) {
    return false;
  }
  return all([isDenseArray(value), value.every(isString)]);
}

function isUsbState(value: unknown): value is UsbState {
  return typeof value === "string" && usbStates.has(value as UsbState);
}

function isSendState(value: unknown): value is SendState {
  return typeof value === "string" && sendStates.has(value as SendState);
}

function isSubsystemHealthState(value: unknown): value is SubsystemHealthState {
  return (
    typeof value === "string" &&
    subsystemHealthStates.has(value as SubsystemHealthState)
  );
}

function isUuidV4(value: unknown): value is string {
  return typeof value === "string" && uuidV4Pattern.test(value);
}

function isBlobId(value: unknown): value is string {
  return typeof value === "string" && blobIdPattern.test(value);
}

function isOptionalNonEmptyString(
  value: Record<string, unknown>,
  key: string,
): boolean {
  if (!Object.hasOwn(value, key)) {
    return true;
  }
  return isNonEmptyString(value[key]);
}

function hasValidParserCoordinates(value: Record<string, unknown>): boolean {
  const keys = ["byteOffset", "line", "column"] as const;
  const presentCount = keys.filter((key) => Object.hasOwn(value, key)).length;
  if (presentCount === 0) {
    return true;
  }
  return all([
    presentCount === keys.length,
    keys.every((key) => isNonNegativeInteger(value[key])),
  ]);
}

function isSendSummary(value: unknown): boolean {
  if (!isRecord(value) || !hasExactKeys(value, ["present", "state"])) {
    return false;
  }
  if (typeof value.present !== "boolean") {
    return false;
  }
  return value.present ? isSendState(value.state) : value.state === null;
}

export function isErrorEnvelope(value: unknown): value is ErrorEnvelope {
  if (!isRecord(value) || !hasExactKeys(value, ["error"])) {
    return false;
  }
  const detail = value.error;
  if (!isRecord(detail)) {
    return false;
  }
  return all([
    hasAllowedKeys(
      detail,
      ["code", "message"],
      ["code", "message", "field", "byteOffset", "line", "column"],
    ),
    isNonEmptyString(detail.code),
    isNonEmptyString(detail.message),
    isOptionalNonEmptyString(detail, "field"),
    hasValidParserCoordinates(detail),
  ]);
}

export function isSetupStateResponse(
  value: unknown,
): value is SetupStateResponse {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["deviceName", "provisioned"]),
    value.provisioned === false,
    isNonEmptyString(value.deviceName),
  ]);
}

export function isSessionStatus(value: unknown): value is SessionStatus {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "absoluteExpiresInSeconds",
      "authenticated",
      "idleExpiresInSeconds",
    ]),
    value.authenticated === true,
    value.idleExpiresInSeconds === v2Limits.sessionIdleLifetimeSeconds,
    value.absoluteExpiresInSeconds ===
      v2Limits.sessionAbsoluteLifetimeSeconds,
  ]);
}

export function isSetupAccepted(value: unknown): value is SetupAccepted {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "accepted",
      "connectionWillClose",
      "reprovisioningRequired",
      "restartRequired",
    ]),
    value.accepted === true,
    value.connectionWillClose === true,
    value.reprovisioningRequired === false,
    value.restartRequired === true,
  ]);
}

export function isActionAccepted(value: unknown): value is ActionAccepted {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "accepted",
      "connectionWillClose",
      "reprovisioningRequired",
    ]),
    value.accepted === true,
    value.connectionWillClose === true,
    typeof value.reprovisioningRequired === "boolean",
  ]);
}

function isResetAcceptedShape(value: unknown): value is ResetAccepted {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "accepted",
      "connectionWillClose",
      "repositoryBlobsPreserved",
      "reprovisioningRequired",
    ]),
    value.accepted === true,
    value.connectionWillClose === true,
    typeof value.reprovisioningRequired === "boolean",
    typeof value.repositoryBlobsPreserved === "boolean",
  ]);
}

export function isResetSettingsAccepted(value: unknown): value is ResetAccepted {
  if (!isResetAcceptedShape(value)) {
    return false;
  }
  return all([
    value.reprovisioningRequired === false,
    value.repositoryBlobsPreserved === true,
  ]);
}

export function isFactoryResetAccepted(value: unknown): value is ResetAccepted {
  if (!isResetAcceptedShape(value)) {
    return false;
  }
  return all([
    value.reprovisioningRequired === true,
    value.repositoryBlobsPreserved === false,
  ]);
}

export function isResetSettingsRequest(
  value: unknown,
): value is ResetSettingsRequest {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["confirmation"]),
    value.confirmation === "RESET SETTINGS",
  ]);
}

export function isFactoryResetRequest(
  value: unknown,
): value is FactoryResetRequest {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["adminPassword", "confirmation"]),
    isNonEmptyString(value.adminPassword),
    value.confirmation === "FACTORY RESET",
  ]);
}

function isStatusIdentity(value: Record<string, unknown>): boolean {
  return all([
    typeof value.provisioned === "boolean",
    isNonEmptyString(value.deviceName),
    isNonEmptyString(value.firmwareVersion),
    isNonEmptyString(value.buildId),
    isNonNegativeInteger(value.uptimeMs),
  ]);
}

function isUsbStatus(value: unknown): boolean {
  if (!isRecord(value)) {
    return false;
  }
  return all([hasExactKeys(value, ["state"]), isUsbState(value.state)]);
}

function isAccessPointStatus(value: unknown): boolean {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["clientCount", "ssid", "state"]),
    isNonEmptyString(value.state),
    isNonEmptyString(value.ssid),
    isNonNegativeInteger(value.clientCount),
  ]);
}

function isStationStatus(value: unknown): boolean {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["configured", "ipv4", "ssid", "state"]),
    typeof value.configured === "boolean",
    isNonEmptyString(value.state),
    isNullableString(value.ssid),
    isNullableString(value.ipv4),
  ]);
}

function isStorageStatus(value: unknown): boolean {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "blobCount",
      "remainingBytes",
      "state",
      "totalBytes",
      "usedBytes",
    ]),
    isNonEmptyString(value.state),
    isNonNegativeInteger(value.totalBytes),
    isNonNegativeInteger(value.usedBytes),
    isNonNegativeInteger(value.remainingBytes),
    isNonNegativeInteger(value.blobCount),
  ]);
}

export function isStatusResponse(value: unknown): value is StatusResponse {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "accessPoint",
      "buildId",
      "deviceName",
      "firmwareVersion",
      "provisioned",
      "send",
      "station",
      "storage",
      "uptimeMs",
      "usb",
    ]),
    isStatusIdentity(value),
    isUsbStatus(value.usb),
    isAccessPointStatus(value.accessPoint),
    isStationStatus(value.station),
    isStorageStatus(value.storage),
    isSendSummary(value.send),
  ]);
}

export function isLimitsResponse(value: unknown): value is LimitsResponse {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "adminPasswordMaxBytes",
      "adminPasswordMinBytes",
      "blobMaxBytes",
      "compiledActionsMax",
      "delayDirectiveMaxMs",
      "estimatedDurationMaxMs",
      "executorAbsoluteDeadlineMs",
      "interKeyMaxMs",
      "jsonBodyMaxBytes",
      "keyPressMaxMs",
      "macroNameMaxBytes",
      "macroSourceMaxBytes",
      "packageNameMaxBytes",
      "snapshotRetentionTargetMax",
    ]),
    value.packageNameMaxBytes === v2Limits.packageNameMaxBytes,
    value.macroNameMaxBytes === v2Limits.macroNameMaxBytes,
    value.macroSourceMaxBytes === v2Limits.macroSourceMaxBytes,
    value.compiledActionsMax === v2Limits.compiledActionsMax,
    value.delayDirectiveMaxMs === v2Limits.delayDirectiveMaxMs,
    value.keyPressMaxMs === v2Limits.keyPressMaxMs,
    value.interKeyMaxMs === v2Limits.interKeyMaxMs,
    value.estimatedDurationMaxMs === v2Limits.estimatedDurationMaxMs,
    value.executorAbsoluteDeadlineMs ===
      v2Limits.executorAbsoluteDeadlineMs,
    value.jsonBodyMaxBytes === v2Limits.jsonBodyMaxBytes,
    value.blobMaxBytes === v2Limits.blobMaxBytes,
    value.adminPasswordMinBytes === v2Limits.adminPasswordMinBytes,
    value.adminPasswordMaxBytes === v2Limits.adminPasswordMaxBytes,
    value.snapshotRetentionTargetMax ===
      v2Limits.snapshotRetentionTargetMax,
  ]);
}

function isBlobSummary(value: unknown): value is BlobSummary {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["id", "sizeBytes"]),
    isBlobId(value.id),
    isNonNegativeInteger(value.sizeBytes),
    typeof value.sizeBytes === "number" &&
      value.sizeBytes <= v2Limits.blobMaxBytes,
  ]);
}

function isStrictlyDescendingBlobIds(blobs: BlobSummary[]): boolean {
  return blobs.slice(1).every((blob, index) => {
    const previous = BigInt(blobs[index]!.id);
    return previous > BigInt(blob.id);
  });
}

export function isBlobListResponse(value: unknown): value is BlobListResponse {
  if (!isRecord(value) || !Array.isArray(value.blobs)) {
    return false;
  }
  const blobs = value.blobs;
  return all([
    hasExactKeys(value, ["blobs", "remainingBytes", "usedBytes"]),
    isDenseArray(blobs),
    blobs.every(isBlobSummary),
    blobs.every(isBlobSummary) && isStrictlyDescendingBlobIds(blobs),
    isNonNegativeInteger(value.usedBytes),
    isNonNegativeInteger(value.remainingBytes),
  ]);
}

export function isBlobCreatedResponse(
  value: unknown,
): value is BlobCreatedResponse {
  if (!isBlobSummary(value)) {
    return false;
  }
  return true;
}

function isSettingsIdentity(value: Record<string, unknown>): boolean {
  return all([
    isNonEmptyString(value.deviceName),
    typeof value.requireSerialConfirmation === "boolean",
    value.sendMode === "quick" || value.sendMode === "preview",
    isNonNegativeInteger(value.snapshotRetentionTarget),
    typeof value.snapshotRetentionTarget === "number" &&
      value.snapshotRetentionTarget <= v2Limits.snapshotRetentionTargetMax,
    typeof value.showMacroSourcePreviews === "boolean",
  ]);
}

function isSettingsSelection(value: Record<string, unknown>): boolean {
  return all([
    value.lastSelectedPackageId === null ||
      isUuidV4(value.lastSelectedPackageId),
    isNonEmptyString(value.apSsid),
  ]);
}

function isSettingsStation(value: Record<string, unknown>): boolean {
  if (typeof value.stationConfigured !== "boolean") {
    return false;
  }
  if (!isNullableString(value.stationSsid)) {
    return false;
  }
  if (value.stationConfigured) {
    return value.stationSsid !== null && value.stationSsid.length > 0;
  }
  return value.stationSsid === null;
}

export function isSettingsResponse(value: unknown): value is SettingsResponse {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "apSsid",
      "deviceName",
      "lastSelectedPackageId",
      "requireSerialConfirmation",
      "sendMode",
      "showMacroSourcePreviews",
      "snapshotRetentionTarget",
      "stationConfigured",
      "stationSsid",
    ]),
    isSettingsIdentity(value),
    isSettingsSelection(value),
    isSettingsStation(value),
  ]);
}

export function isSettingsUpdatedResponse(
  value: unknown,
): value is SettingsUpdatedResponse {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "reconnectRequired",
      "restartRequired",
      "settings",
    ]),
    isSettingsResponse(value.settings),
    typeof value.restartRequired === "boolean",
    typeof value.reconnectRequired === "boolean",
  ]);
}

function isSendIdentity(value: Record<string, unknown>): boolean {
  return all([
    isUuidV4(value.id),
    value.state === "awaiting_confirmation" || value.state === "running",
  ]);
}

function isSendBounds(value: Record<string, unknown>): boolean {
  return all([
    isNonNegativeInteger(value.actionCount),
    typeof value.actionCount === "number" &&
      value.actionCount <= v2Limits.compiledActionsMax,
    isNonNegativeInteger(value.estimatedDurationMs),
    typeof value.estimatedDurationMs === "number" &&
      value.estimatedDurationMs <= v2Limits.estimatedDurationMaxMs,
  ]);
}

export function isSendAcceptedResponse(
  value: unknown,
): value is SendAcceptedResponse {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "actionCount",
      "estimatedDurationMs",
      "id",
      "state",
    ]),
    isSendIdentity(value),
    isSendBounds(value),
  ]);
}

function isSendProgress(value: Record<string, unknown>): boolean {
  if (!isNonNegativeInteger(value.actionIndex)) {
    return false;
  }
  if (!isNonNegativeInteger(value.actionCount)) {
    return false;
  }
  return all([
    value.actionCount <= v2Limits.compiledActionsMax,
    value.actionIndex <= value.actionCount,
  ]);
}

function isSendResultText(value: Record<string, unknown>): boolean {
  return all([
    typeof value.cancellationRequested === "boolean",
    isString(value.error),
    isString(value.releaseError),
  ]);
}

export function isSendStatusResponse(
  value: unknown,
): value is SendStatusResponse {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "actionCount",
      "actionIndex",
      "cancellationRequested",
      "error",
      "estimatedDurationMs",
      "id",
      "releaseError",
      "state",
    ]),
    isUuidV4(value.id),
    isSendState(value.state),
    isSendProgress(value),
    isSendBounds(value),
    isSendResultText(value),
  ]);
}

function isSubsystem(value: unknown): boolean {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["name", "state"]),
    isNonEmptyString(value.name),
    isSubsystemHealthState(value.state),
  ]);
}

function isDiagnosticsIdentity(value: Record<string, unknown>): boolean {
  return all([
    isNonEmptyString(value.firmwareVersion),
    isNonEmptyString(value.buildId),
    isNonEmptyString(value.resetReason),
    isNonNegativeInteger(value.uptimeMs),
  ]);
}

function isDiagnosticsMemory(value: unknown): boolean {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "freeHeapBytes",
      "largestFreeBlockBytes",
      "minimumFreeHeapBytes",
    ]),
    isNonNegativeInteger(value.freeHeapBytes),
    isNonNegativeInteger(value.minimumFreeHeapBytes),
    isNonNegativeInteger(value.largestFreeBlockBytes),
  ]);
}

function isDiagnosticsWifi(value: unknown): boolean {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["accessPointState", "stationState"]),
    isNonEmptyString(value.accessPointState),
    isNonEmptyString(value.stationState),
  ]);
}

function isDiagnosticsStorage(value: unknown): boolean {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "blobCount",
      "invalidNames",
      "state",
      "temporaryFiles",
      "userdataTotalBytes",
      "userdataUsedBytes",
      "webfsTotalBytes",
      "webfsUsedBytes",
    ]),
    isNonEmptyString(value.state),
    isNonNegativeInteger(value.webfsTotalBytes),
    isNonNegativeInteger(value.webfsUsedBytes),
    isNonNegativeInteger(value.userdataTotalBytes),
    isNonNegativeInteger(value.userdataUsedBytes),
    isNonNegativeInteger(value.blobCount),
    isStringArray(value.invalidNames),
    isStringArray(value.temporaryFiles),
  ]);
}

function isSubsystemArray(value: unknown): boolean {
  if (!Array.isArray(value)) {
    return false;
  }
  return all([isDenseArray(value), value.every(isSubsystem)]);
}

export function isDiagnosticsResponse(
  value: unknown,
): value is DiagnosticsResponse {
  if (!isRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "buildId",
      "firmwareVersion",
      "memory",
      "resetReason",
      "send",
      "storage",
      "subsystems",
      "uptimeMs",
      "usb",
      "wifi",
    ]),
    isDiagnosticsIdentity(value),
    isDiagnosticsMemory(value.memory),
    isUsbStatus(value.usb),
    isDiagnosticsWifi(value.wifi),
    isDiagnosticsStorage(value.storage),
    isSendSummary(value.send),
    isSubsystemArray(value.subsystems),
  ]);
}
