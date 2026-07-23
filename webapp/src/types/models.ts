export type UsbState =
  | "uninitialized"
  | "disconnected"
  | "enumerating"
  | "ready"
  | "suspended"
  | "error";

export type MacroScope = "set" | "global";

export interface MacroSet {
  schemaVersion: 1;
  id: string;
  revision: number;
  name: string;
  description: string;
  manufacturer: string;
  model: string;
  board: string;
  keyboardLayout: "en-US";
  sortOrder: number;
}

export interface Macro {
  schemaVersion: 1;
  id: string;
  revision: number;
  scope: MacroScope;
  setId?: string;
  name: string;
  source: string;
  favorite: boolean;
  keyPressMs: number;
  interKeyMs: number;
}

export type ProcedureStep =
  | {
      id: string;
      type: "macro";
      title: string;
      macroId: string;
      required: boolean;
      autoCompleteOnSuccess: boolean;
    }
  | {
      id: string;
      type: "instruction" | "checkpoint";
      title: string;
      body: string;
      required: boolean;
    };

export interface Procedure {
  schemaVersion: 1;
  id: string;
  revision: number;
  setId: string;
  name: string;
  description: string;
  steps: ProcedureStep[];
  sortOrder: number;
}

export interface ExecutionStatus {
  state: "idle" | "running" | "completed" | "cancelled" | "failed";
  error: string;
  releaseError: string;
  actionIndex: number;
  actionCount: number;
}
