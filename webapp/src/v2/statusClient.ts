import { v2GetJson } from "./apiClient";
import { isStatusResponse } from "./apiGuards";
import type { StatusResponse } from "./apiTypes";

/**
 * `GET /api/v1/status` — the authenticated live device status used by the
 * application shell (TODO_V2 V2-090) to show USB state prominently
 * (UI_UX_SPEC_V2 §2.1) and by the Macros page to gate Quick Send on
 * `usb.state === "ready"` (TODO_V2 V2-091).
 */
export async function getStatus(): Promise<StatusResponse> {
  return v2GetJson("/api/v1/status", isStatusResponse);
}
