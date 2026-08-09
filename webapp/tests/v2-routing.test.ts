import { describe, expect, test } from "vitest";
import {
  macroEditorTargetFromHash,
  macroPreviewTargetFromHash,
  navigateToAddMacro,
  navigateToEditMacro,
  navigateToMacroPreview,
  navigateToMacros,
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
      "macro-preview",
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
    setHashSilently("/macro-preview?macroId=" + macroId);
    navigateToMacros();
    expect(window.location.hash).toBe("#/macros");
  });

  test("macroPreviewTargetFromHash parses a valid preview target", () => {
    setHashSilently(`/macro-preview?macroId=${macroId}`);
    expect(macroPreviewTargetFromHash()).toEqual({
      kind: "valid",
      macroId,
    });
  });

  test.each([
    "/macros",
    "/macro-preview",
    `/macro-preview?macroId=not-a-uuid`,
    `/macro-preview?macroId=${macroId}&extra=true`,
    `/macro-preview?macroId=${macroId}&macroId=${macroId}`,
  ])("rejects malformed preview route %s", (hash) => {
    setHashSilently(hash);
    expect(macroPreviewTargetFromHash()).toEqual({ kind: "invalid" });
  });

  test("navigateToMacroPreview encodes the macro ID", () => {
    navigateToMacroPreview(macroId);
    expect(window.location.hash).toBe(`#/macro-preview?macroId=${macroId}`);
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
});
