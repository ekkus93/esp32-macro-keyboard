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
