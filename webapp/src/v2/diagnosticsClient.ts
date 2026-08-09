import { v2GetJson } from "./apiClient";
import { isDiagnosticsResponse } from "./apiContracts";
import type { DiagnosticsResponse } from "./apiTypes";

/**
 * `GET /api/v1/diagnostics` — the fixed operational-health schema (SPEC_V2
 * §13.13, TODO_V2 V2-122). `isDiagnosticsResponse` enforces the exact key set
 * at every level (`hasExactKeys`), so a response carrying anything outside
 * firmware/build, uptime, reset reason, memory, USB, Wi-Fi, storage, send
 * state, and subsystem health — in particular any package, macro, or
 * credential data — is rejected before it ever reaches React state.
 */
export async function getDiagnostics(): Promise<DiagnosticsResponse> {
  return v2GetJson("/api/v1/diagnostics", isDiagnosticsResponse);
}
