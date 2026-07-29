import { planJsonResponse } from "./fakeFetch";

export const setId = "11111111-1111-4111-8111-111111111111";
export const macroId = "22222222-2222-4222-8222-222222222222";
export const executionId = "33333333-3333-4333-8333-333333333333";

export const deviceStatus = {
  version: "0.1.0",
  idf: "v5.5.5",
  usbState: "ready",
  wifiState: "started",
  wifiClients: 1,
  executionState: "idle",
} as const;

export const settings = {
  schemaVersion: 1,
  revision: 4,
  requirePhysicalConfirmation: true,
  alwaysSelectSet: true,
  activeSetId: setId,
} as const;

export const macroSet = {
  schema_version: 1,
  id: setId,
  revision: 2,
  name: "Lab Chromebook workflow",
  description: "ChromeOS conversion and Debian installation",
  manufacturer: "Example",
  model: "Model 14",
  board: "board-14",
  keyboard_layout: "en-US",
  sort_order: 0,
} as const;

export const macro = {
  schema_version: 1,
  id: macroId,
  revision: 7,
  scope: "set",
  set_id: setId,
  name: "Open terminal",
  source: "{CTRL+ALT+T}",
  favorite: true,
  key_press_ms: 8,
  inter_key_ms: 15,
} as const;

export function executionStatus(
  state:
    | "idle"
    | "running"
    | "completed"
    | "cancelled"
    | "failed"
    | "timed_out",
  actionIndex = 2,
  actionCount = 5,
): object {
  return {
    executionId,
    setId,
    macroId,
    macroRevision: 7,
    state,
    error: state === "failed" ? "press_failed" : "",
    releaseError: "",
    actionIndex,
    actionCount,
    available: true,
    cancellationRequested: false,
  };
}

export function planNormalUnauthenticatedBootstrap(): void {
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
  planJsonResponse({ ok: true, data: deviceStatus });
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
}

export function planAuthenticatedBootstrap(
  overrides: {
    activeSetId?: string | null;
    sets?: readonly object[];
    usbState?: string;
  } = {},
): void {
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
  planJsonResponse({
    ok: true,
    data: {
      ...deviceStatus,
      usbState: overrides.usbState ?? deviceStatus.usbState,
    },
  });
  planJsonResponse({
    ok: true,
    data: { authenticated: true, csrfToken: "csrf-restored" },
  });
  planJsonResponse({
    ok: true,
    data: {
      ...settings,
      activeSetId:
        overrides.activeSetId === undefined
          ? settings.activeSetId
          : overrides.activeSetId,
    },
  });
  planJsonResponse({
    ok: true,
    data: overrides.sets ?? [macroSet],
  });
}

export function planPostLoginBootstrap(): void {
  planJsonResponse({
    ok: true,
    data: { csrfToken: "csrf-123" },
  });
  planJsonResponse({ ok: true, data: deviceStatus });
  planJsonResponse({ ok: true, data: settings });
  planJsonResponse({ ok: true, data: [macroSet] });
}
