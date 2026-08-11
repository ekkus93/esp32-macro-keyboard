/// <reference types="vite/client" />
import { describe, expect, test, vi } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import { gzipCompress } from "../src/v2/gzip";
import type { Repository } from "../src/v2/repository";
import { serializeRepository } from "../src/v2/repository";
import { createEmptyRepository } from "../src/v2/repositoryValidation";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import {
  deleteSnapshot,
  exportRepository,
  importRepository,
  listSnapshots,
  loadSnapshotIntoWorkingCopy,
  saveWorkingCopyAsSnapshot,
} from "../src/v2/snapshotClient";
import {
  persistSelectedPackageId,
  resolveSelectedPackage,
} from "../src/v2/packageSelection";
import { recoverSendState, sendMacro } from "../src/v2/sendClient";
import { FirstRunSetupPage } from "../src/features/auth/v2/FirstRunSetupPage";
import { SignInPage } from "../src/features/auth/v2/SignInPage";
import { RepositoryStartupScreen } from "../src/features/startup/v2/RepositoryStartupScreen";
import type { RepositoryStartupReady } from "../src/features/startup/v2/RepositoryStartupScreen";
import { jsonResponse, planFetch, planJsonResponse } from "./fakeFetch";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
  submit as submitForm,
} from "./render";

const canonical = canonicalRepository as Repository;

/**
 * SPEC_V2 §8.6 / TODO_V2 V2-072: repository JSON, IDs, names, source, and
 * compressed bytes MUST NOT be persisted in localStorage, sessionStorage,
 * IndexedDB, Cache Storage, or a service worker. This file gives both the
 * static "build scan" (source never references the forbidden APIs at all)
 * and a runtime behavioral check (exercising the Phase 7 client leaves
 * browser storage untouched).
 *
 * The static scan covers `src/AppV2.tsx`, the complete `src/v2/` data layer,
 * and every production V2 feature directory under `src/features/`. This includes
 * authentication/setup secrets, loaded repository state, macros, snapshots,
 * settings, shell state, and future V2 feature families added under that
 * directory convention.
 */

const forbiddenApiPattern =
  /\blocalStorage\b|\bsessionStorage\b|\bindexedDB\b|\bcaches\.\w|\bserviceWorker\b|\bopenDatabase\b/;
const browserConsolePattern = /\bconsole\.(?:log|info|warn|error|debug)\s*\(/;

// Reads every scanned source file's raw text through Vite's glob import
// rather than Node's `fs` module: browser app code (this project's
// `tsconfig.app.json` `types`) intentionally excludes Node types, so a
// Node-based file scan cannot type-check here, and a bundler-level glob is
// the idiomatic alternative that stays consistent with that boundary.
const v2SourceModules = import.meta.glob<string>(
  [
    "../src/AppV2.tsx",
    "../src/v2/**/*.{ts,tsx}",
    "../src/features/**/v2/**/*.{ts,tsx}",
  ],
  {
    eager: true,
    query: "?raw",
    import: "default",
  },
);

/**
 * Strips `//` and `/* *\/` comments so the scan only flags actual code
 * usage of a forbidden storage API, not this file's own doc comments (which
 * necessarily name the forbidden APIs in prose) or an unrelated word like
 * "storage" inside a sentence.
 */
function stripComments(source: string): string {
  return source.replace(/\/\*[\s\S]*?\*\//g, "").replace(/\/\/.*$/gm, "");
}

describe("v2 browser-storage prohibition: static scan", () => {
  test("no production V2 source references browser persistence APIs", () => {
    const entries = Object.entries(v2SourceModules);
    expect(entries.length).toBeGreaterThan(0);
    const offenders = entries
      .map(([path, source]) => ({ path, code: stripComments(source) }))
      .filter(({ code }) => forbiddenApiPattern.test(code));
    expect(offenders.map((offender) => offender.path)).toEqual([]);
  });

  test("no production V2 source writes application or secret state to the browser console", () => {
    const offenders = Object.entries(v2SourceModules)
      .map(([path, source]) => ({ path, code: stripComments(source) }))
      .filter(({ code }) => browserConsolePattern.test(code));
    expect(offenders.map((offender) => offender.path)).toEqual([]);
  });
});

describe("v2 browser-storage prohibition: runtime behavior", () => {
  test("exercising the repository working copy, snapshot client, package selection, and send helper never touches browser storage", async () => {
    // Spying on the prototype method (rather than the instance) avoids
    // jsdom's Storage quirk where adding an own property to the instance —
    // which is exactly what `vi.spyOn(window.localStorage, "setItem")`
    // does — is itself observed as a stored key.
    const localStorageSetItem = vi.spyOn(Storage.prototype, "setItem");

    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    store.applyContentChange(canonical);
    store.markSaved(canonical);
    store.discardChanges();
    store.replaceWorkingCopy(canonical);

    planJsonResponse({ blobs: [], usedBytes: 0, remainingBytes: 500_000 });
    await listSnapshots();

    planFetch(() => jsonResponse({ id: "1", sizeBytes: 100 }, 201));
    await saveWorkingCopyAsSnapshot(store);

    const bytes = await gzipCompress(
      new TextEncoder().encode(serializeRepository(canonical)),
    );
    planFetch(
      () =>
        new Response(bytes, {
          status: 200,
          headers: { "Content-Type": "application/gzip" },
        }),
    );
    await loadSnapshotIntoWorkingCopy("1", store);

    planFetch(() => new Response(null, { status: 204 }));
    await deleteSnapshot("1");

    await exportRepository(canonical);
    await importRepository(bytes);

    resolveSelectedPackage(canonical, null);
    planFetch(() =>
      jsonResponse(
        {
          settings: {
            deviceName: "Desk Macro Keyboard",
            requireSerialConfirmation: false,
            sendMode: "quick",
            snapshotRetentionTarget: 5,
            showMacroSourcePreviews: false,
            lastSelectedPackageId: "550e8400-e29b-41d4-a716-446655440000",
            apSsid: "MacroKeyboard",
            stationConfigured: false,
            stationSsid: null,
          },
          restartRequired: false,
          reconnectRequired: false,
        },
        200,
      ),
    );
    await persistSelectedPackageId(
      "550e8400-e29b-41d4-a716-446655440000",
      null,
    );

    planJsonResponse(
      { error: { code: "not_found", message: "No send since boot." } },
      404,
    );
    await recoverSendState();

    planJsonResponse(
      {
        id: "550e8400-e29b-41d4-a716-446655440000",
        state: "running",
        actionCount: 1,
        estimatedDurationMs: 10,
      },
      202,
    );
    const handle = await sendMacro({
      source: "a",
      keyPressMs: 8,
      interKeyMs: 15,
    });
    handle.stop();

    // Covers both localStorage and sessionStorage: jsdom's Storage.prototype
    // is shared by both instances.
    expect(localStorageSetItem).not.toHaveBeenCalled();
    expect(window.localStorage.length).toBe(0);
    expect(window.sessionStorage.length).toBe(0);
  });

  test("exercising the V2-080 setup page and V2-081 sign-in page never touches browser storage", async () => {
    const localStorageSetItem = vi.spyOn(Storage.prototype, "setItem");

    planJsonResponse({
      provisioned: false,
      deviceName: "ESP32 Macro Keyboard",
    });
    const setupView = await render(
      <FirstRunSetupPage
        onSetupComplete={() => {
          /* not exercised in this test */
        }}
      />,
    );
    await flushReact();
    await setInputValue(
      requiredElement("#setup-code", HTMLInputElement),
      "12345678",
    );
    await setInputValue(
      requiredElement("#device-name", HTMLInputElement),
      "Desk Macro Keyboard",
    );
    await setInputValue(
      requiredElement("#ap-ssid", HTMLInputElement),
      "MacroKeyboard",
    );
    await setInputValue(
      requiredElement("#ap-passphrase", HTMLInputElement),
      "example-passphrase",
    );
    await setInputValue(
      requiredElement("#admin-password", HTMLInputElement),
      "example-admin-password",
    );
    await submitForm(requiredElement("form", HTMLFormElement));
    await flushReact();
    planFetch(() =>
      jsonResponse(
        {
          accepted: true,
          restartRequired: true,
          connectionWillClose: true,
          reprovisioningRequired: false,
        },
        202,
      ),
    );
    await click(buttonWithText("Apply setup"));
    await flushReact();
    await setupView.unmount();

    planJsonResponse(
      { error: { code: "unauthorized", message: "Sign in required." } },
      401,
    );
    const signInView = await render(
      <SignInPage
        onAuthenticated={() => {
          /* not exercised in this test */
        }}
      />,
    );
    await flushReact();
    await setInputValue(
      requiredElement("#admin-password", HTMLInputElement),
      "correct horse battery staple",
    );
    planJsonResponse(
      {
        authenticated: true,
        idleExpiresInSeconds: 86_400,
        absoluteExpiresInSeconds: 604_800,
      },
      200,
    );
    await submitForm(requiredElement("form", HTMLFormElement));
    await flushReact();
    await signInView.unmount();

    expect(localStorageSetItem).not.toHaveBeenCalled();
    expect(window.localStorage.length).toBe(0);
    expect(window.sessionStorage.length).toBe(0);
  });

  test("exercising the V2-082/V2-083/V2-084 repository startup screen (Create Your First Repository) never touches browser storage", async () => {
    const localStorageSetItem = vi.spyOn(Storage.prototype, "setItem");

    planJsonResponse({
      deviceName: "Desk Macro Keyboard",
      requireSerialConfirmation: false,
      sendMode: "quick",
      snapshotRetentionTarget: 5,
      showMacroSourcePreviews: false,
      lastSelectedPackageId: null,
      apSsid: "MacroKeyboard",
      stationConfigured: false,
      stationSsid: null,
    });
    planJsonResponse({ blobs: [], usedBytes: 0, remainingBytes: 100 });

    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const startupView = await render(
      <RepositoryStartupScreen existing={null} onReady={onReady} />,
    );
    await flushReact();
    for (
      let attempt = 0;
      attempt < 50 && document.querySelector("#first-package-name") === null;
      attempt += 1
    ) {
      await flushReact();
    }
    await setInputValue(
      requiredElement("#first-package-name", HTMLInputElement),
      "My First Package",
    );
    planFetch(() =>
      jsonResponse(
        {
          settings: {
            deviceName: "Desk Macro Keyboard",
            requireSerialConfirmation: false,
            sendMode: "quick",
            snapshotRetentionTarget: 5,
            showMacroSourcePreviews: false,
            lastSelectedPackageId: "550e8400-e29b-41d4-a716-446655440000",
            apSsid: "MacroKeyboard",
            stationConfigured: false,
            stationSsid: null,
          },
          restartRequired: false,
          reconnectRequired: false,
        },
        200,
      ),
    );
    await submitForm(requiredElement("form", HTMLFormElement));
    for (
      let attempt = 0;
      attempt < 50 && onReady.mock.calls.length === 0;
      attempt += 1
    ) {
      await flushReact();
    }
    expect(onReady).toHaveBeenCalledTimes(1);
    await startupView.unmount();

    expect(localStorageSetItem).not.toHaveBeenCalled();
    expect(window.localStorage.length).toBe(0);
    expect(window.sessionStorage.length).toBe(0);
  });
});
