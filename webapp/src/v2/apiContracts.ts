import { v2Limits } from "./limits";

export type UsbState =
  | "uninitialized"
  | "disconnected"
  | "enumerating"
  | "ready"
  | "suspended"
  | "error";

export type SendState =
  | "awaiting_confirmation"
  | "running"
  | "completed"
  | "cancelled"
  | "failed"
  | "timed_out";

export type SendMode = "quick" | "preview";

export interface ErrorDetail {
  code: string;
  message: string;
  field?: string;
  byteOffset?: number;
  line?: number;
  column?: number;
}

export interface ErrorEnvelope {
  error: ErrorDetail;
}

export interface SessionStatus {
  authenticated: true;
  idleExpiresInSeconds: number;
  absoluteExpiresInSeconds: number;
}

export interface ActionAccepted {
  accepted: true;
  connectionWillClose: true;
  reprovisioningRequired: boolean;
}

export interface SetupAccepted extends ActionAccepted {
  restartRequired: true;
}

export interface ResetAccepted extends ActionAccepted {
  repositoryBlobsPreserved: boolean;
}

export interface StatusResponse {
  provisioned: boolean;
  deviceName: string;
  firmwareVersion: string;
  buildId: string;
  uptimeMs: number;
  usb: { state: UsbState };
  accessPoint: { state: string; ssid: string; clientCount: number };
  station: {
    configured: boolean;
    state: string;
    ssid: string | null;
    ipv4: string | null;
  };
  storage: {
    state: string;
    totalBytes: number;
    usedBytes: number;
    remainingBytes: number;
    blobCount: number;
  };
  send: { present: boolean; state: SendState | null };
}

export interface LimitsResponse {
  packageNameMaxBytes: number;
  macroNameMaxBytes: number;
  macroSourceMaxBytes: number;
  compiledActionsMax: number;
  delayDirectiveMaxMs: number;
  keyPressMaxMs: number;
  interKeyMaxMs: number;
  estimatedDurationMaxMs: number;
  executorAbsoluteDeadlineMs: number;
  jsonBodyMaxBytes: number;
  blobMaxBytes: number;
  adminPasswordMinBytes: number;
  adminPasswordMaxBytes: number;
  snapshotRetentionTargetMax: number;
}

export interface BlobSummary {
  id: string;
  sizeBytes: number;
}

export interface BlobListResponse {
  blobs: BlobSummary[];
  usedBytes: number;
  remainingBytes: number;
}

export interface BlobCreatedResponse extends BlobSummary {}

export interface SettingsResponse {
  deviceName: string;
  requireSerialConfirmation: boolean;
  sendMode: SendMode;
  snapshotRetentionTarget: number;
  showMacroSourcePreviews: boolean;
  lastSelectedPackageId: string | null;
  apSsid: string;
  stationConfigured: boolean;
  stationSsid: string | null;
}

export interface SettingsUpdatedResponse {
  settings: SettingsResponse;
  restartRequired: boolean;
  reconnectRequired: boolean;
}

export interface SendAcceptedResponse {
  id: string;
  state: "awaiting_confirmation" | "running";
  actionCount: number;
  estimatedDurationMs: number;
}

export interface SendStatusResponse {
  id: string;
  state: SendState;
  actionIndex: number;
  actionCount: number;
  estimatedDurationMs: number;
  cancellationRequested: boolean;
  error: string;
  releaseError: string;
}

export type SubsystemHealthState =
  | "healthy"
  | "degraded"
  | "unavailable"
  | "recovering"
  | "failed"
  | "unknown";

export interface DiagnosticsResponse {
  firmwareVersion: string;
  buildId: string;
  resetReason: string;
  uptimeMs: number;
  memory: {
    freeHeapBytes: number;
    minimumFreeHeapBytes: number;
    largestFreeBlockBytes: number;
  };
  usb: { state: UsbState };
  wifi: { accessPointState: string; stationState: string };
  storage: {
    state: string;
    webfsTotalBytes: number;
    webfsUsedBytes: number;
    userdataTotalBytes: number;
    userdataUsedBytes: number;
    blobCount: number;
    invalidNames: string[];
    temporaryFiles: string[];
  };
  send: { present: boolean; state: SendState | null };
  subsystems: Array<{ name: string; state: SubsystemHealthState }>;
}

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

function hasAllowedKeys(
  value: Record<string, unknown>,
  required: readonly string[],
  allowed: readonly string[],
): boolean {
  const actual = Object.keys(value);
  return (
    required.every((key) => Object.hasOwn(value, key)) &&
    actual.every((key) => allowed.includes(key))
  );
}

function isNonNegativeInteger(value: unknown): value is number {
  return (
    typeof value === "number" &&
    Number.isSafeInteger(value) &&
    Number.isFinite(value) &&
    value >= 0
  );
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
  return Array.isArray(value) && value.every(isString);
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

function isSendSummary(value: unknown): boolean {
  if (
    !isRecord(value) ||
    !hasExactKeys(value, ["present", "state"]) ||
    typeof value.present !== "boolean"
  ) {
    return false;
  }
  if (value.present) {
    return isSendState(value.state);
  }
  return value.state === null;
}

export function isErrorEnvelope(value: unknown): value is ErrorEnvelope {
  if (!isRecord(value) || !hasExactKeys(value, ["error"])) {
    return false;
  }
  const detail = value.error;
  if (
    !isRecord(detail) ||
    !hasAllowedKeys(
      detail,
      ["code", "message"],
      ["code", "message", "field", "byteOffset", "line", "column"],
    ) ||
    !isNonEmptyString(detail.code) ||
    !isNonEmptyString(detail.message) ||
    (Object.hasOwn(detail, "field") && !isNonEmptyString(detail.field))
  ) {
    return false;
  }

  const coordinateKeys = ["byteOffset", "line", "column"] as const;
  const coordinateCount = coordinateKeys.filter((key) =>
    Object.hasOwn(detail, key),
  ).length;
  return (
    coordinateCount === 0 ||
    (coordinateCount === coordinateKeys.length &&
      coordinateKeys.every((key) => isNonNegativeInteger(detail[key])))
  );
}

export function isSessionStatus(value: unknown): value is SessionStatus {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "absoluteExpiresInSeconds",
      "authenticated",
      "idleExpiresInSeconds",
    ]) &&
    value.authenticated === true &&
    value.idleExpiresInSeconds === v2Limits.sessionIdleLifetimeSeconds &&
    value.absoluteExpiresInSeconds ===
      v2Limits.sessionAbsoluteLifetimeSeconds
  );
}

export function isSetupAccepted(value: unknown): value is SetupAccepted {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "accepted",
      "connectionWillClose",
      "reprovisioningRequired",
      "restartRequired",
    ]) &&
    value.accepted === true &&
    value.connectionWillClose === true &&
    value.reprovisioningRequired === false &&
    value.restartRequired === true
  );
}

export function isActionAccepted(value: unknown): value is ActionAccepted {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "accepted",
      "connectionWillClose",
      "reprovisioningRequired",
    ]) &&
    value.accepted === true &&
    value.connectionWillClose === true &&
    typeof value.reprovisioningRequired === "boolean"
  );
}

export function isResetAccepted(value: unknown): value is ResetAccepted {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "accepted",
      "connectionWillClose",
      "repositoryBlobsPreserved",
      "reprovisioningRequired",
    ]) &&
    value.accepted === true &&
    value.connectionWillClose === true &&
    typeof value.reprovisioningRequired === "boolean" &&
    typeof value.repositoryBlobsPreserved === "boolean"
  );
}

export function isStatusResponse(value: unknown): value is StatusResponse {
  if (
    !isRecord(value) ||
    !hasExactKeys(value, [
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
    ]) ||
    typeof value.provisioned !== "boolean" ||
    !isNonEmptyString(value.deviceName) ||
    !isNonEmptyString(value.firmwareVersion) ||
    !isNonEmptyString(value.buildId) ||
    !isNonNegativeInteger(value.uptimeMs)
  ) {
    return false;
  }

  const usb = value.usb;
  const accessPoint = value.accessPoint;
  const station = value.station;
  const storage = value.storage;
  return (
    isRecord(usb) &&
    hasExactKeys(usb, ["state"]) &&
    isUsbState(usb.state) &&
    isRecord(accessPoint) &&
    hasExactKeys(accessPoint, ["clientCount", "ssid", "state"]) &&
    isNonEmptyString(accessPoint.state) &&
    isNonEmptyString(accessPoint.ssid) &&
    isNonNegativeInteger(accessPoint.clientCount) &&
    isRecord(station) &&
    hasExactKeys(station, ["configured", "ipv4", "ssid", "state"]) &&
    typeof station.configured === "boolean" &&
    isNonEmptyString(station.state) &&
    isNullableString(station.ssid) &&
    isNullableString(station.ipv4) &&
    isRecord(storage) &&
    hasExactKeys(storage, [
      "blobCount",
      "remainingBytes",
      "state",
      "totalBytes",
      "usedBytes",
    ]) &&
    isNonEmptyString(storage.state) &&
    isNonNegativeInteger(storage.totalBytes) &&
    isNonNegativeInteger(storage.usedBytes) &&
    isNonNegativeInteger(storage.remainingBytes) &&
    isNonNegativeInteger(storage.blobCount) &&
    isSendSummary(value.send)
  );
}

export function isLimitsResponse(value: unknown): value is LimitsResponse {
  return (
    isRecord(value) &&
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
    ]) &&
    value.packageNameMaxBytes === v2Limits.packageNameMaxBytes &&
    value.macroNameMaxBytes === v2Limits.macroNameMaxBytes &&
    value.macroSourceMaxBytes === v2Limits.macroSourceMaxBytes &&
    value.compiledActionsMax === v2Limits.compiledActionsMax &&
    value.delayDirectiveMaxMs === v2Limits.delayDirectiveMaxMs &&
    value.keyPressMaxMs === v2Limits.keyPressMaxMs &&
    value.interKeyMaxMs === v2Limits.interKeyMaxMs &&
    value.estimatedDurationMaxMs === v2Limits.estimatedDurationMaxMs &&
    value.executorAbsoluteDeadlineMs ===
      v2Limits.executorAbsoluteDeadlineMs &&
    value.jsonBodyMaxBytes === v2Limits.jsonBodyMaxBytes &&
    value.blobMaxBytes === v2Limits.blobMaxBytes &&
    value.adminPasswordMinBytes === v2Limits.adminPasswordMinBytes &&
    value.adminPasswordMaxBytes === v2Limits.adminPasswordMaxBytes &&
    value.snapshotRetentionTargetMax ===
      v2Limits.snapshotRetentionTargetMax
  );
}

function isBlobSummary(value: unknown): value is BlobSummary {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["id", "sizeBytes"]) &&
    isBlobId(value.id) &&
    isNonNegativeInteger(value.sizeBytes) &&
    value.sizeBytes <= v2Limits.blobMaxBytes
  );
}

export function isBlobListResponse(value: unknown): value is BlobListResponse {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["blobs", "remainingBytes", "usedBytes"]) &&
    Array.isArray(value.blobs) &&
    value.blobs.every(isBlobSummary) &&
    isNonNegativeInteger(value.usedBytes) &&
    isNonNegativeInteger(value.remainingBytes)
  );
}

export function isBlobCreatedResponse(
  value: unknown,
): value is BlobCreatedResponse {
  return isBlobSummary(value);
}

export function isSettingsResponse(value: unknown): value is SettingsResponse {
  return (
    isRecord(value) &&
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
    ]) &&
    isNonEmptyString(value.deviceName) &&
    typeof value.requireSerialConfirmation === "boolean" &&
    (value.sendMode === "quick" || value.sendMode === "preview") &&
    isNonNegativeInteger(value.snapshotRetentionTarget) &&
    value.snapshotRetentionTarget <= v2Limits.snapshotRetentionTargetMax &&
    typeof value.showMacroSourcePreviews === "boolean" &&
    (value.lastSelectedPackageId === null ||
      isUuidV4(value.lastSelectedPackageId)) &&
    isNonEmptyString(value.apSsid) &&
    typeof value.stationConfigured === "boolean" &&
    isNullableString(value.stationSsid) &&
    (value.stationConfigured
      ? value.stationSsid !== null && value.stationSsid.length > 0
      : value.stationSsid === null)
  );
}

export function isSettingsUpdatedResponse(
  value: unknown,
): value is SettingsUpdatedResponse {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "reconnectRequired",
      "restartRequired",
      "settings",
    ]) &&
    isSettingsResponse(value.settings) &&
    typeof value.restartRequired === "boolean" &&
    typeof value.reconnectRequired === "boolean"
  );
}

export function isSendAcceptedResponse(
  value: unknown,
): value is SendAcceptedResponse {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "actionCount",
      "estimatedDurationMs",
      "id",
      "state",
    ]) &&
    isUuidV4(value.id) &&
    (value.state === "awaiting_confirmation" || value.state === "running") &&
    isNonNegativeInteger(value.actionCount) &&
    value.actionCount <= v2Limits.compiledActionsMax &&
    isNonNegativeInteger(value.estimatedDurationMs) &&
    value.estimatedDurationMs <= v2Limits.estimatedDurationMaxMs
  );
}

export function isSendStatusResponse(
  value: unknown,
): value is SendStatusResponse {
  return (
    isRecord(value) &&
    hasExactKeys(value, [
      "actionCount",
      "actionIndex",
      "cancellationRequested",
      "error",
      "estimatedDurationMs",
      "id",
      "releaseError",
      "state",
    ]) &&
    isUuidV4(value.id) &&
    isSendState(value.state) &&
    isNonNegativeInteger(value.actionIndex) &&
    isNonNegativeInteger(value.actionCount) &&
    value.actionCount <= v2Limits.compiledActionsMax &&
    value.actionIndex <= value.actionCount &&
    isNonNegativeInteger(value.estimatedDurationMs) &&
    value.estimatedDurationMs <= v2Limits.estimatedDurationMaxMs &&
    typeof value.cancellationRequested === "boolean" &&
    isString(value.error) &&
    isString(value.releaseError)
  );
}

function isSubsystem(value: unknown): boolean {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["name", "state"]) &&
    isNonEmptyString(value.name) &&
    isSubsystemHealthState(value.state)
  );
}

export function isDiagnosticsResponse(
  value: unknown,
): value is DiagnosticsResponse {
  if (
    !isRecord(value) ||
    !hasExactKeys(value, [
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
    ]) ||
    !isNonEmptyString(value.firmwareVersion) ||
    !isNonEmptyString(value.buildId) ||
    !isNonEmptyString(value.resetReason) ||
    !isNonNegativeInteger(value.uptimeMs)
  ) {
    return false;
  }

  const memory = value.memory;
  const usb = value.usb;
  const wifi = value.wifi;
  const storage = value.storage;
  return (
    isRecord(memory) &&
    hasExactKeys(memory, [
      "freeHeapBytes",
      "largestFreeBlockBytes",
      "minimumFreeHeapBytes",
    ]) &&
    isNonNegativeInteger(memory.freeHeapBytes) &&
    isNonNegativeInteger(memory.minimumFreeHeapBytes) &&
    isNonNegativeInteger(memory.largestFreeBlockBytes) &&
    isRecord(usb) &&
    hasExactKeys(usb, ["state"]) &&
    isUsbState(usb.state) &&
    isRecord(wifi) &&
    hasExactKeys(wifi, ["accessPointState", "stationState"]) &&
    isNonEmptyString(wifi.accessPointState) &&
    isNonEmptyString(wifi.stationState) &&
    isRecord(storage) &&
    hasExactKeys(storage, [
      "blobCount",
      "invalidNames",
      "state",
      "temporaryFiles",
      "userdataTotalBytes",
      "userdataUsedBytes",
      "webfsTotalBytes",
      "webfsUsedBytes",
    ]) &&
    isNonEmptyString(storage.state) &&
    isNonNegativeInteger(storage.webfsTotalBytes) &&
    isNonNegativeInteger(storage.webfsUsedBytes) &&
    isNonNegativeInteger(storage.userdataTotalBytes) &&
    isNonNegativeInteger(storage.userdataUsedBytes) &&
    isNonNegativeInteger(storage.blobCount) &&
    isStringArray(storage.invalidNames) &&
    isStringArray(storage.temporaryFiles) &&
    isSendSummary(value.send) &&
    Array.isArray(value.subsystems) &&
    value.subsystems.every(isSubsystem)
  );
}
