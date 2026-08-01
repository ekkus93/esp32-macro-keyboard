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

export interface MacroSet {
  schema_version: 1;
  id: string;
  revision: number;
  name: string;
  description: string;
  manufacturer: string;
  model: string;
  board: string;
  keyboard_layout: "en-US";
  sort_order: number;
}

export interface SetDeletion {
  deleted: true;
  id: string;
}

export type MacroScope = "set" | "global";

export interface Macro {
  schema_version: 1;
  id: string;
  revision: number;
  scope: MacroScope;
  set_id?: string;
  name: string;
  source: string;
  favorite: boolean;
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

export type ProcedureStep =
  | {
      id: string;
      type: "macro";
      title: string;
      macro_id: string;
      required: boolean;
      auto_complete_on_success: boolean;
    }
  | {
      id: string;
      type: "instruction" | "checkpoint";
      title: string;
      body: string;
      required: boolean;
    };

export interface ProcedureSummary {
  schema_version: 1;
  id: string;
  revision: number;
  set_id: string;
  name: string;
  description: string;
  step_count: number;
  sort_order: number;
}

export interface Procedure {
  schema_version: 1;
  id: string;
  revision: number;
  set_id: string;
  name: string;
  description: string;
  steps: ProcedureStep[];
  sort_order: number;
}

export interface ProcedureProgress {
  schema_version: 1;
  set_id: string;
  procedure_id: string;
  procedure_revision: number;
  current_step_id: string;
  completed_step_ids: string[];
  skipped_step_ids: string[];
}

export interface ProcedureProgressSnapshot {
  status: "current" | "stale";
  currentProcedureRevision: number;
  progress: ProcedureProgress;
}

export interface ExecutionSourceContext {
  procedureId: string;
  stepId: string;
}

export interface ExecutionSubmitRequest {
  setId: string;
  macroId: string;
  macroRevision: number;
  sourceContext?: ExecutionSourceContext;
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
  quarantineCount: number;
  damagedQuarantineCount: number;
}

export interface QuarantineEntry {
  id: string;
  sourcePath: string;
  evidencePath: string;
  reason: string;
}

export interface QuarantineList {
  damagedCount: number;
  items: QuarantineEntry[];
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

export interface DiagnosticsQuarantineSummary {
  ok: boolean;
  count: number;
  damagedCount: number;
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
  quarantine: DiagnosticsQuarantineSummary;
  executionState: string;
  subsystems: DiagnosticsSubsystem[];
}
