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

export type SubsystemHealthState =
  | "healthy"
  | "degraded"
  | "unavailable"
  | "recovering"
  | "failed"
  | "unknown";

export interface ErrorDetail {
  code: string;
  message: string;
  field?: string | undefined;
  byteOffset?: number | undefined;
  line?: number | undefined;
  column?: number | undefined;
}

export interface ErrorEnvelope {
  error: ErrorDetail;
}

export interface SetupStateResponse {
  provisioned: false;
  deviceName: string;
}

export interface SetupRequest {
  setupCode: string;
  deviceName: string;
  apSsid: string;
  apPassphrase: string;
  adminPassword: string;
  requireSerialConfirmation: boolean;
}

export interface LoginRequest {
  adminPassword: string;
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
  activeSessionsMax: number;
  sessionIdleLifetimeSeconds: number;
  sessionAbsoluteLifetimeSeconds: number;
  serialConfirmationTimeoutSeconds: number;
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

export type BlobCreatedResponse = BlobSummary;

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

export interface NetworkCredentialsRequest {
  ssid: string;
  passphrase: string;
}

export interface SettingsUpdateRequest {
  deviceName?: string;
  requireSerialConfirmation?: boolean;
  sendMode?: SendMode;
  snapshotRetentionTarget?: number;
  showMacroSourcePreviews?: boolean;
  lastSelectedPackageId?: string | null;
  accessPoint?: NetworkCredentialsRequest;
  station?: NetworkCredentialsRequest | null;
}

export interface SettingsUpdatedResponse {
  settings: SettingsResponse;
  restartRequired: boolean;
  reconnectRequired: boolean;
}

export interface PasswordChangeRequest {
  currentPassword: string;
  newPassword: string;
}

export interface SendRequest {
  source: string;
  keyPressMs: number;
  interKeyMs: number;
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

export interface ResetSettingsRequest {
  confirmation: "RESET SETTINGS";
}

export interface FactoryResetRequest {
  adminPassword: string;
  confirmation: "FACTORY RESET";
}

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
  subsystems: { name: string; state: SubsystemHealthState }[];
}
