import { describe, expect, test } from "vitest";
import { v2ErrorText } from "../src/features/auth/v2/v2ErrorText";
import { V2ApiError } from "../src/v2/apiClient";

describe("v2ErrorText", () => {
  test("keeps the server's actionable message without exposing its internal code", () => {
    expect(
      v2ErrorText(
        new V2ApiError(403, {
          code: "auth_failed",
          message: "incorrect current password",
        }),
      ),
    ).toBe("incorrect current password");
  });

  test("explains a committed password change and the required recovery action", () => {
    expect(
      v2ErrorText(
        new V2ApiError(409, {
          code: "auth_state_incomplete",
          message:
            "password changed; session invalidation incomplete; sign in with the new password",
        }),
      ),
    ).toBe(
      "Your password changed, but not all active sessions could be cleared. Sign in with the new password.",
    );
  });

  test("explains an incomplete settings reset and the next action", () => {
    expect(
      v2ErrorText(
        new V2ApiError(409, {
          code: "reset_settings_incomplete",
          message:
            "settings reset; automatic restart incomplete; restart the device",
        }),
      ),
    ).toBe(
      "Settings were reset, but the device did not restart automatically. Go back and choose Restart.",
    );
  });

  test("explains full device storage and how to make room", () => {
    expect(
      v2ErrorText(
        new V2ApiError(507, {
          code: "storage_full",
          message: "insufficient blob storage",
        }),
      ),
    ).toBe("Device storage is full. Delete an old snapshot, then try again.");
  });

  test("translates fetch and timeout failures into connection guidance", () => {
    expect(v2ErrorText(new TypeError("Failed to fetch"))).toBe(
      "Could not reach the device. Check your Wi-Fi connection and try again.",
    );
    expect(v2ErrorText(new DOMException("Aborted", "AbortError"))).toBe(
      "The request timed out. Check the device connection and try again.",
    );
  });
});
