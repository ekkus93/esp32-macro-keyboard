export type UsbState =
  | "uninitialized"
  | "disconnected"
  | "enumerating"
  | "ready"
  | "suspended"
  | "error";

export type ExecutionState =
  | "idle"
  | "running"
  | "completed"
  | "cancelled"
  | "failed"
  | "timed_out";

export interface DeviceStatus {
  version: string;
  idf: string;
  usbState: UsbState;
  wifiState: string;
  wifiClients: number;
  executionState: ExecutionState;
}

export interface SetupState {
  deviceId: string;
  apSsid: string;
  completed: boolean;
  physicalConfirmationRequired: boolean;
}

export interface SessionStatus {
  authenticated: true;
  csrfToken: string;
}

export interface LoginResponse {
  csrfToken: string;
}

export interface Settings {
  schemaVersion: 1;
  revision: number;
  requirePhysicalConfirmation: boolean;
  alwaysSelectSet: boolean;
  activeSetId: string | null;
}

/* A set is a name and an ordered list of macros (SPEC 12.1). Set order lives
   in the index and is the order the API returns them in. */
export interface MacroSet {
  schema_version: 1;
  id: string;
  revision: number;
  name: string;
}

export interface SetDeletion {
  deleted: true;
  id: string;
}

export interface Macro {
  schema_version: 1;
  id: string;
  revision: number;
  /* Every macro belongs to exactly one set (SPEC 7.2). */
  set_id: string;
  name: string;
  source: string;
  key_press_ms: number;
  inter_key_ms: number;
}

export interface MacroValidation {
  valid: true;
  actionCount: number;
  estimatedDurationMs: number;
}

export interface MacroParseLocation {
  line: number;
  column: number;
  byteOffset: number;
}

export interface ExecutionSubmitRequest {
  setId: string;
  macroId: string;
  macroRevision: number;
}

export interface ExecutionAccepted {
  executionId: string;
  actionCount: number;
  estimatedDurationMs: number;
}

export interface ExecutionStatus {
  executionId: string;
  setId: string;
  macroId: string;
  macroRevision: number;
  state: ExecutionState;
  error: string;
  releaseError: string;
  actionIndex: number;
  actionCount: number;
  available: boolean;
  cancellationRequested: boolean;
  /** Monotonic device-uptime milliseconds, not wall-clock time; 0 until set. */
  acceptedMs: number;
  startedMs: number;
  completedMs: number;
  /** Redacted action-type summary only: "key" | "chord" | "delay" | "none". */
  currentAction: string;
}

export interface CancelAccepted {
  cancelRequested: true;
}

export interface RestartAccepted {
  restartScheduled: true;
}

export interface FactoryResetAccepted {
  factoryReset: true;
  restartScheduled: true;
}

export interface StorageHealth {
  verified: boolean;
  webMounted: boolean;
  dataMounted: boolean;
}

export type SubsystemHealthState =
  | "healthy"
  | "degraded"
  | "unavailable"
  | "recovering"
  | "failed"
  | "unknown";

export interface DiagnosticsSubsystem {
  name: string;
  state: SubsystemHealthState;
}

export interface DiagnosticsCapacity {
  ok: boolean;
  totalBytes: number;
  usedBytes: number;
}

export interface DiagnosticsStack {
  controlsWords: number;
  executorWords: number;
}

export interface FullDiagnostics {
  buildId: string;
  firmwareVersion: string;
  schemaVersion: number;
  resetReason: string;
  uptimeMs: number;
  freeHeapBytes: number;
  minFreeHeapBytes: number;
  stack: DiagnosticsStack;
  webfs: DiagnosticsCapacity;
  userdata: DiagnosticsCapacity;
  executionState: string;
  subsystems: DiagnosticsSubsystem[];
}
