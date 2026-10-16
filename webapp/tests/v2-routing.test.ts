import { describe, expect, test } from "vitest";
import {
  macroEditorTargetFromHash,
  navigateToSnapshotsFiles,
  snapshotsViewFromHash,
  navigateToAddMacro,
  navigateToDiagnostics,
  navigateToEditMacro,
  navigateToMacros,
  navigateToSettings,
  navigateV2,
  navigateToSettingsGroup,
  navigateToSettingsSection,
  routeFromHashV2,
  settingsGroupFromHash,
  settingsSectionFromHash,
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

  test("settings groups round-trip through the settings route and reject unknown groups", () => {
    for (const group of [
      "device",
      "network",
      "security",
      "usb",
      "reset",
    ] as const) {
      navigateToSettingsGroup(group);
      expect(routeFromHashV2()).toBe("settings");
      expect(settingsGroupFromHash()).toBe(group);
    }
    setHashSilently("/settings?group=unknown");
    expect(settingsGroupFromHash()).toBeNull();
    setHashSilently("/macros?group=usb");
    expect(settingsGroupFromHash()).toBeNull();
  });

  test("network settings sections round-trip and are ignored on other routes or groups", () => {
    for (const section of [
      "access-point",
      "station",
      "password",
      "session",
      "restart",
      "reset-settings",
      "factory-reset",
    ] as const) {
      navigateToSettingsSection(section);
      expect(routeFromHashV2()).toBe("settings");
      expect(settingsGroupFromHash()).toBe(
        section === "access-point" || section === "station"
          ? "network"
          : section === "password" || section === "session"
            ? "security"
            : "reset",
      );
      expect(settingsSectionFromHash()).toBe(section);
    }
    setHashSilently("/settings?group=network&section=unknown");
    expect(settingsSectionFromHash()).toBeNull();
    setHashSilently("/settings?group=device&section=station");
    expect(settingsSectionFromHash()).toBeNull();
  });

  test("snapshotsViewFromHash reads only view=files on the snapshots route", () => {
    setHashSilently("/snapshots");
    expect(snapshotsViewFromHash()).toBe("list");
    setHashSilently("/snapshots?view=files");
    expect(snapshotsViewFromHash()).toBe("files");
    // Anything else, or the same query on another route, is the ordinary list.
    setHashSilently("/snapshots?view=other");
    expect(snapshotsViewFromHash()).toBe("list");
    setHashSilently("/snapshots?view=");
    expect(snapshotsViewFromHash()).toBe("list");
    setHashSilently("/settings?view=files");
    expect(snapshotsViewFromHash()).toBe("list");
  });

  test("navigateToSnapshotsFiles keeps the snapshots route and sets the files view", () => {
    navigateToSnapshotsFiles();
    expect(window.location.hash).toBe("#/snapshots?view=files");
    expect(routeFromHashV2()).toBe("snapshots");
    expect(snapshotsViewFromHash()).toBe("files");
  });
});
