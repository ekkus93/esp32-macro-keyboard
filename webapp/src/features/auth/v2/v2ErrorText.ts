import { V2ApiError } from "../../../v2/apiClient";

/**
 * Renders any error thrown by the v2 API client as user-facing text.
 *
 * For a `V2ApiError` this relays exactly the server's own `code`/`message` —
 * it never adds guesses (e.g. "wrong password", "N attempts remaining") on
 * top of what the device chose to disclose. That is what keeps rate-limit
 * and lockout responses from leaking account state: the device's error
 * message is the only source of truth for what the user sees.
 */
export function v2ErrorText(error: unknown): string {
  if (error instanceof V2ApiError) {
    return `${error.code}: ${error.message}`;
  }
  if (error instanceof Error) {
    return error.message;
  }
  return "An unknown error occurred.";
}
