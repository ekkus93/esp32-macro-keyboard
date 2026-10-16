import { V2ApiError } from "../../../v2/apiClient";

/**
 * Renders any error thrown by the v2 API client as user-facing text.
 *
 * Keep useful server messages, but never expose machine error codes. Known
 * committed partial failures get recovery guidance without guessing about
 * authentication state or adding details the device did not disclose.
 */
export function v2ErrorText(error: unknown): string {
  if (error instanceof V2ApiError) {
    if (error.code === "storage_full") {
      return "Device storage is full. Delete an old snapshot, then try again.";
    }
    if (error.code === "auth_state_incomplete") {
      return "Your password changed, but not all active sessions could be cleared. Sign in with the new password.";
    }
    if (error.code === "reset_settings_incomplete") {
      return "Settings were reset, but the device did not restart automatically. Go back and choose Restart.";
    }
    return error.message;
  }
  if (error instanceof DOMException && error.name === "AbortError") {
    return "The request timed out. Check the device connection and try again.";
  }
  if (
    error instanceof TypeError &&
    /^(failed to fetch|load failed|network)/i.test(error.message)
  ) {
    return "Could not reach the device. Check your Wi-Fi connection and try again.";
  }
  if (error instanceof Error) {
    return error.message;
  }
  return "Something went wrong. Please try again.";
}
