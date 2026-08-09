import { v2GetJson } from "./apiClient";
import { isSettingsResponse } from "./apiContracts";
import type { SettingsResponse } from "./apiTypes";

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
