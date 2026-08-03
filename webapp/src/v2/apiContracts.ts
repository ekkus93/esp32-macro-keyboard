import { isFactoryResetRequest as isFactoryResetRequestShape } from "./apiGuards";
import type { FactoryResetRequest } from "./apiTypes";
import { v2Limits } from "./limits";

const encoder = new TextEncoder();

export {
  isActionAccepted,
  isBlobCreatedResponse,
  isBlobListResponse,
  isDiagnosticsResponse,
  isErrorEnvelope,
  isFactoryResetAccepted,
  isLimitsResponse,
  isResetSettingsAccepted,
  isResetSettingsRequest,
  isSendAcceptedResponse,
  isSendStatusResponse,
  isSessionStatus,
  isSettingsResponse,
  isSettingsUpdatedResponse,
  isSetupAccepted,
  isSetupStateResponse,
  isStatusResponse,
} from "./apiGuards";

export {
  isLoginRequest,
  isPasswordChangeRequest,
  isSendRequest,
  isSettingsUpdateRequest,
  isSetupRequest,
} from "./apiRequestGuards";

export function isFactoryResetRequest(
  value: unknown,
): value is FactoryResetRequest {
  if (!isFactoryResetRequestShape(value)) {
    return false;
  }
  const passwordBytes = encoder.encode(value.adminPassword).byteLength;
  return (
    passwordBytes >= v2Limits.adminPasswordMinBytes &&
    passwordBytes <= v2Limits.adminPasswordMaxBytes
  );
}

export type {
  ActionAccepted,
  BlobCreatedResponse,
  BlobListResponse,
  BlobSummary,
  DiagnosticsResponse,
  ErrorDetail,
  ErrorEnvelope,
  FactoryResetRequest,
  LimitsResponse,
  LoginRequest,
  NetworkCredentialsRequest,
  PasswordChangeRequest,
  ResetAccepted,
  ResetSettingsRequest,
  SendAcceptedResponse,
  SendMode,
  SendRequest,
  SendState,
  SendStatusResponse,
  SessionStatus,
  SettingsResponse,
  SettingsUpdatedResponse,
  SettingsUpdateRequest,
  SetupAccepted,
  SetupRequest,
  SetupStateResponse,
  StatusResponse,
  SubsystemHealthState,
  UsbState,
} from "./apiTypes";
