import { v2PostJson, v2PostNoContent } from "./apiClient";
import {
  isActionAccepted,
  isFactoryResetAccepted,
  isFactoryResetRequest,
  isResetSettingsAccepted,
  isResetSettingsRequest,
} from "./apiContracts";
import type { ActionAccepted, ResetAccepted } from "./apiTypes";

/**
 * The v2 device-action routes (SPEC_V2 §13.12, TODO_V2 V2-121): restart,
 * reset settings, and factory reset, plus `POST /api/v1/auth/logout` (Sign
 * Out) — the fifth of V2-103's unsaved-change warning trigger points this
 * module's Settings UI caller finally wires up alongside the other two.
 *
 * Every one of these ends the current browser session (a restart or reboot
 * clears firmware's RAM-only sessions; sign-out and a password change
 * invalidate sessions explicitly). None of them touch the in-memory
 * repository working copy directly — the caller (`SettingsPage`/`AppV2`)
 * owns what happens to it afterward.
 */

export async function restartDevice(): Promise<ActionAccepted> {
  return v2PostJson("/api/v1/device/restart", undefined, isActionAccepted);
}

export async function resetDeviceSettings(): Promise<ResetAccepted> {
  const request = { confirmation: "RESET SETTINGS" as const };
  if (!isResetSettingsRequest(request)) {
    throw new Error("Invalid reset-settings request.");
  }
  return v2PostJson(
    "/api/v1/device/reset-settings",
    request,
    isResetSettingsAccepted,
  );
}

export async function factoryResetDevice(
  adminPassword: string,
): Promise<ResetAccepted> {
  const request = {
    adminPassword,
    confirmation: "FACTORY RESET" as const,
  };
  if (!isFactoryResetRequest(request)) {
    throw new Error("Invalid factory-reset request.");
  }
  return v2PostJson(
    "/api/v1/device/factory-reset",
    request,
    isFactoryResetAccepted,
  );
}

/**
 * `POST /api/v1/auth/logout` — Sign Out. A `401` here (the session was
 * already gone before the click landed) is still a legitimate signal to
 * fall back to Sign In, so this deliberately does not suppress the default
 * `notifyOnUnauthorized` behavior the way a route expecting one on the
 * *happy* path might.
 */
export async function signOut(): Promise<void> {
  await v2PostNoContent("/api/v1/auth/logout", undefined);
}
