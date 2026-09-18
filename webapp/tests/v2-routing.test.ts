import { describe, expect, test } from "vitest";
import {
  macroEditorTargetFromHash,
  navigateToAddMacro,
  navigateToDiagnostics,
  navigateToEditMacro,
  navigateToMacros,
  navigateToSettings,
  navigateV2,
  routeFromHashV2,
} from "../src/v2/routingV2";
import { setHashSilently } from "./fakeLocation";

const macroId = "550e8400-e29b-41d4-a716-446655440000";

describe("v2 routing", () => {
  test("routeFromHashV2 recognizes every known v2 screen", () => {
    for (const screen of [
      "macros",
      "packages",
      "snapshots",
      "settings",
      "diagnostics",
      "macro-editor",
    ] as const) {
      setHashSilently(`/${screen}`);
      expect(routeFromHashV2()).toBe(screen);
    }
  });

  test("routeFromHashV2 falls back to macros for an unknown or empty route", () => {
    setHashSilently("");
    expect(routeFromHashV2()).toBe("macros");
    setHashSilently("/not-a-real-v2-screen");
    expect(routeFromHashV2()).toBe("macros");
  });

  test("routeFromHashV2 never recognizes a v1 screen name", () => {
    setHashSilently("/execution");
    expect(routeFromHashV2()).toBe("macros");
  });

  test("navigateV2 sets the hash for each screen", () => {
    navigateV2("packages");
    expect(window.location.hash).toBe("#/packages");
    navigateV2("snapshots");
    expect(window.location.hash).toBe("#/snapshots");
    navigateV2("settings");
    expect(window.location.hash).toBe("#/settings");
  });

  test("navigateToMacros always returns to the macros route", () => {
    setHashSilently(`/macro-editor?macroId=${macroId}`);
    navigateToMacros();
    expect(window.location.hash).toBe("#/macros");
  });

  test("macroEditorTargetFromHash treats a bare macro-editor route as create", () => {
    setHashSilently("/macro-editor");
    expect(macroEditorTargetFromHash()).toEqual({ kind: "create" });
  });

  test("macroEditorTargetFromHash parses a valid edit target", () => {
    setHashSilently(`/macro-editor?macroId=${macroId}`);
    expect(macroEditorTargetFromHash()).toEqual({ kind: "edit", macroId });
  });

  test.each([
    "/macros",
    `/macro-editor?macroId=not-a-uuid`,
    `/macro-editor?macroId=${macroId}&extra=true`,
    `/macro-editor?macroId=${macroId}&macroId=${macroId}`,
    "/macro-editor?other=1",
  ])("rejects malformed macro-editor route %s", (hash) => {
    setHashSilently(hash);
    expect(macroEditorTargetFromHash()).toEqual({ kind: "invalid" });
  });

  test("navigateToAddMacro clears any macro ID", () => {
    setHashSilently(`/macro-editor?macroId=${macroId}`);
    navigateToAddMacro();
    expect(window.location.hash).toBe("#/macro-editor");
  });

  test("navigateToEditMacro encodes the macro ID", () => {
    navigateToEditMacro(macroId);
    expect(window.location.hash).toBe(`#/macro-editor?macroId=${macroId}`);
  });

  test("navigateToSettings and navigateToDiagnostics set the expected hash", () => {
    navigateToSettings();
    expect(window.location.hash).toBe("#/settings");
    navigateToDiagnostics();
    expect(window.location.hash).toBe("#/diagnostics");
  });
});
