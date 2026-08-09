import { v2GetJson, v2PostNoContent, v2PutJson } from "./apiClient";
import { isSettingsResponse, isSettingsUpdatedResponse } from "./apiContracts";
import type {
  PasswordChangeRequest,
  SettingsResponse,
  SettingsUpdatedResponse,
  SettingsUpdateRequest,
} from "./apiTypes";

/**
 * `GET /api/v1/settings` — sanitized device UI settings, per SPEC_V2 §12 and
 * UI_UX_SPEC_V2 §3.4 step 2 ("loads device UI settings, including
 * `lastSelectedPackageId`"). This is the authenticated settings read used by
 * the repository startup sequence (TODO_V2 V2-082); it never includes
 * secret material (the response guard enforces the sanitized field set).
 */
export async function getSettings(): Promise<SettingsResponse> {
  return v2GetJson("/api/v1/settings", isSettingsResponse);
}

/**
 * `PUT /api/v1/settings` — a strict partial update (SPEC_V2 §13.9): omitted
 * fields preserve their current values, unknown fields are rejected, and an
 * empty update object is rejected. Used by the Settings UI (TODO_V2 V2-120)
 * for device name, serial-confirmation policy, sending behavior, retention
 * target, source-preview preference, and access-point/station network
 * configuration. This is a device-settings write, never a repository edit —
 * it never touches `RepositoryWorkingCopyStore` and so never dirties the
 * repository (SPEC_V2 §11.1 "UI preferences ... do not make the repository
 * dirty").
 */
export async function updateSettings(
  request: SettingsUpdateRequest,
): Promise<SettingsUpdatedResponse> {
  return v2PutJson("/api/v1/settings", request, isSettingsUpdatedResponse);
}

/**
 * `POST /api/v1/settings/change-password` — SPEC_V2 §13.9: a successful
 * change returns `204`, invalidates every session including the current one,
 * and clears the current cookie. The caller is responsible for taking the UI
 * back to Sign In afterward; this function only performs the request.
 */
export async function changePassword(
  request: PasswordChangeRequest,
): Promise<void> {
  await v2PostNoContent("/api/v1/settings/change-password", request);
}
