import type {
  CancelAccepted,
  DeviceStatus,
  ExecutionState,
  ExecutionStatus,
  LoginResponse,
  MacroSet,
  RestartAccepted,
  SessionStatus,
  Settings,
  SetupState,
  UsbState,
} from "../types/models";

export type Validator<T> = (value: unknown) => value is T;

export function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

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

function isPositiveInteger(value: unknown): value is number {
  return isNonNegativeInteger(value) && value > 0;
}

const uuidPattern =
  /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

export function isUuid(value: unknown): value is string {
  return typeof value === "string" && uuidPattern.test(value);
}

const usbStates = new Set<UsbState>([
  "uninitialized",
  "disconnected",
  "enumerating",
  "ready",
  "suspended",
  "error",
]);

export function isUsbState(value: unknown): value is UsbState {
  return typeof value === "string" && usbStates.has(value as UsbState);
}

const executionStates = new Set<ExecutionState>([
  "idle",
  "running",
  "completed",
  "cancelled",
  "failed",
  "timed_out",
]);

export function isExecutionState(value: unknown): value is ExecutionState {
  return (
    typeof value === "string" && executionStates.has(value as ExecutionState)
  );
}

export function isDeviceStatus(value: unknown): value is DeviceStatus {
  if (
    !isRecord(value) ||
    !hasExactKeys(value, [
      "version",
      "idf",
      "usbState",
      "wifiState",
      "wifiClients",
      "executionState",
    ])
  ) {
    return false;
  }
  return (
    typeof value.version === "string" &&
    typeof value.idf === "string" &&
    isUsbState(value.usbState) &&
    typeof value.wifiState === "string" &&
    isNonNegativeInteger(value.wifiClients) &&
    isExecutionState(value.executionState)
  );
}

export function isSetupState(value: unknown): value is SetupState {
  if (
    !isRecord(value) ||
    !hasExactKeys(value, [
      "deviceId",
      "apSsid",
      "completed",
      "physicalConfirmationRequired",
    ])
  ) {
    return false;
  }
  return (
    typeof value.deviceId === "string" &&
    value.deviceId.length > 0 &&
    typeof value.apSsid === "string" &&
    value.apSsid.length > 0 &&
    typeof value.completed === "boolean" &&
    typeof value.physicalConfirmationRequired === "boolean"
  );
}

export function isSessionStatus(value: unknown): value is SessionStatus {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["authenticated", "csrfToken"]) &&
    value.authenticated === true &&
    typeof value.csrfToken === "string" &&
    value.csrfToken.length > 0
  );
}

export function isLoginResponse(value: unknown): value is LoginResponse {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["csrfToken"]) &&
    typeof value.csrfToken === "string" &&
    value.csrfToken.length > 0
  );
}

export function isSettings(value: unknown): value is Settings {
  if (
    !isRecord(value) ||
    !hasExactKeys(value, [
      "schemaVersion",
      "revision",
      "requirePhysicalConfirmation",
      "alwaysSelectSet",
      "activeSetId",
    ])
  ) {
    return false;
  }
  return (
    value.schemaVersion === 1 &&
    isPositiveInteger(value.revision) &&
    typeof value.requirePhysicalConfirmation === "boolean" &&
    typeof value.alwaysSelectSet === "boolean" &&
    (value.activeSetId === null || isUuid(value.activeSetId))
  );
}

export function isMacroSet(value: unknown): value is MacroSet {
  if (
    !isRecord(value) ||
    !hasExactKeys(value, [
      "schema_version",
      "id",
      "revision",
      "name",
      "description",
      "manufacturer",
      "model",
      "board",
      "keyboard_layout",
      "sort_order",
    ])
  ) {
    return false;
  }
  return (
    value.schema_version === 1 &&
    isUuid(value.id) &&
    isPositiveInteger(value.revision) &&
    typeof value.name === "string" &&
    value.name.length > 0 &&
    typeof value.description === "string" &&
    typeof value.manufacturer === "string" &&
    typeof value.model === "string" &&
    typeof value.board === "string" &&
    value.keyboard_layout === "en-US" &&
    isNonNegativeInteger(value.sort_order)
  );
}

export function isMacroSetList(value: unknown): value is MacroSet[] {
  return Array.isArray(value) && value.every(isMacroSet);
}

export function isExecutionStatus(value: unknown): value is ExecutionStatus {
  if (
    !isRecord(value) ||
    !hasExactKeys(value, [
      "executionId",
      "setId",
      "macroId",
      "macroRevision",
      "state",
      "error",
      "releaseError",
      "actionIndex",
      "actionCount",
      "available",
      "cancellationRequested",
    ])
  ) {
    return false;
  }
  return (
    typeof value.executionId === "string" &&
    typeof value.setId === "string" &&
    typeof value.macroId === "string" &&
    isNonNegativeInteger(value.macroRevision) &&
    isExecutionState(value.state) &&
    typeof value.error === "string" &&
    typeof value.releaseError === "string" &&
    isNonNegativeInteger(value.actionIndex) &&
    isNonNegativeInteger(value.actionCount) &&
    typeof value.available === "boolean" &&
    typeof value.cancellationRequested === "boolean"
  );
}

export function isCancelAccepted(value: unknown): value is CancelAccepted {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["cancelRequested"]) &&
    value.cancelRequested === true
  );
}

export function isRestartAccepted(value: unknown): value is RestartAccepted {
  return (
    isRecord(value) &&
    hasExactKeys(value, ["restartScheduled"]) &&
    value.restartScheduled === true
  );
}

export function isEmptyRecord(value: unknown): value is Record<string, never> {
  return isRecord(value) && Object.keys(value).length === 0;
}
