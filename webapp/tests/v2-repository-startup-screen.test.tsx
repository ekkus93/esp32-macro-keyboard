import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import { RepositoryStartupScreen } from "../src/features/startup/v2/RepositoryStartupScreen";
import type { RepositoryStartupReady } from "../src/features/startup/v2/RepositoryStartupScreen";
import { gzipCompress } from "../src/v2/gzip";
import type { Repository } from "../src/v2/repository";
import { serializeRepository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { SendStatusResponse, SettingsResponse } from "../src/v2/apiTypes";
import { getFetchCalls, jsonResponse, planFetch } from "./fakeFetch";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
  submit,
} from "./render";

const canonical = canonicalRepository as Repository;
const canonicalPackageId = "550e8400-e29b-41d4-a716-446655440000";
const secondPackageId = "6ba7b810-9dad-41d1-80b4-00c04fd430c9";
const twoPackageRepository: Repository = {
  ...canonical,
  packages: [
    ...canonical.packages,
    { id: secondPackageId, name: "Second package", macros: [] },
  ],
};

const baseSettings: SettingsResponse = {
  deviceName: "Desk Macro Keyboard",
  requireSerialConfirmation: false,
  sendMode: "quick",
  snapshotRetentionTarget: 5,
  showMacroSourcePreviews: false,
  lastSelectedPackageId: null,
  apSsid: "MacroKeyboard",
  stationConfigured: false,
  stationSsid: null,
};

async function canonicalGzipBytes(): Promise<Uint8Array> {
  return gzipCompress(new TextEncoder().encode(serializeRepository(canonical)));
}

function planSettingsGet(settings: SettingsResponse = baseSettings): void {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/settings");
    expect(call.method).toBe("GET");
    return jsonResponse(settings);
  });
}

function planBlobList(
  blobs: { id: string; sizeBytes: number }[],
  usedBytes = 0,
  remainingBytes = 100,
): void {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/blob");
    expect(call.method).toBe("GET");
    return jsonResponse({ blobs, usedBytes, remainingBytes });
  });
}

function planBlobGet(id: string, bytes: Uint8Array): void {
  planFetch((call) => {
    expect(call.url).toBe(`/api/v1/blob/${id}`);
    expect(call.method).toBe("GET");
    return new Response(bytes, {
      status: 200,
      headers: { "Content-Type": "application/gzip" },
    });
  });
}

function planNoSend(): void {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/send");
    expect(call.method).toBe("GET");
    return jsonResponse(
      { error: { code: "not_found", message: "No send yet." } },
      404,
    );
  });
}

function planSend(status: SendStatusResponse): void {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/send");
    expect(call.method).toBe("GET");
    return jsonResponse(status);
  });
}

function planSettingsPutAccepted(): { called: () => boolean } {
  let called = false;
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/settings");
    expect(call.method).toBe("PUT");
    called = true;
    return jsonResponse({
      settings: baseSettings,
      restartRequired: false,
      reconnectRequired: false,
    });
  });
  return { called: () => called };
}

function planSettingsPutFailure(error: Error): void {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/settings");
    expect(call.method).toBe("PUT");
    throw error;
  });
}

/**
 * One real event-loop tick (not just drained microtasks) wrapped in `act`.
 * `gzipCompress`/`gzipDecompress` go through Node's zlib, which completes on
 * the libuv threadpool — a real I/O callback, not a microtask — so pure
 * `Promise.resolve()` chaining (as in `flushReact`) never observes it.
 */
async function flushTick(): Promise<void> {
  await act(async () => {
    await new Promise<void>((resolve) => {
      setTimeout(resolve, 0);
    });
  });
}

/**
 * Polls by yielding real event-loop ticks until `predicate` is true, instead
 * of guessing a fixed number of flushes. The load sequence this screen
 * drives chains several real `fetch` + `Response` body reads and gzip
 * operations, whose exact micro/macrotask hop count is not a contract this
 * test suite should hardcode.
 */
async function waitUntil(
  predicate: () => boolean,
  description: string,
): Promise<void> {
  for (let attempt = 0; attempt < 50; attempt += 1) {
    if (predicate()) {
      return;
    }
    await flushTick();
  }
  throw new Error(`Timed out waiting for: ${description}`);
}

async function renderScreen(
  onReady: (ready: RepositoryStartupReady) => void = () => {
    /* no-op default */
  },
): Promise<Awaited<ReturnType<typeof render>>> {
  const view = await render(
    <RepositoryStartupScreen existing={null} onReady={onReady} />,
  );
  await flushReact();
  return view;
}

function bodyIncludes(text: string): boolean {
  return document.body.textContent?.includes(text) === true;
}

describe("v2 RepositoryStartupScreen", () => {
  test("restores an existing live working copy without any network request", async () => {
    const store = createRepositoryWorkingCopyStore(canonical);
    const existing: RepositoryStartupReady = {
      store,
      packageId: canonicalPackageId,
      sendRecovery: { kind: "none" },
      loadedBlobId: "1",
    };
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await render(
      <RepositoryStartupScreen existing={existing} onReady={onReady} />,
    );
    await flushReact();

    expect(getFetchCalls()).toHaveLength(0);
    expect(onReady).toHaveBeenCalledTimes(1);
    expect(onReady).toHaveBeenCalledWith(existing);
    await view.unmount();
  });

  test("shows a brief repository-loading state while the sequence runs", async () => {
    planFetch(
      () =>
        new Promise<Response>(() => {
          /* deliberately never resolves */
        }),
    );
    const view = await render(
      <RepositoryStartupScreen
        existing={null}
        onReady={() => {
          /* not exercised in this test */
        }}
      />,
    );
    await flushReact();

    expect(document.body.textContent).toContain("Opening your repository…");
    expect(document.body.textContent).toContain(
      "Loading the newest snapshot from the device.",
    );
    await view.unmount();
  });

  test("shows a compatibility error for unsupported Compression Streams before any network call", async () => {
    vi.stubGlobal("CompressionStream", undefined);
    vi.stubGlobal("DecompressionStream", undefined);
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(
      () => bodyIncludes("Browser not supported"),
      "the compatibility error",
    );

    expect(getFetchCalls()).toHaveLength(0);
    expect(onReady).not.toHaveBeenCalled();
    await view.unmount();
  });

  test("reports device-unreachable distinctly and Retry re-runs the sequence", async () => {
    planFetch(() => {
      throw new TypeError("Failed to fetch");
    });
    const view = await renderScreen();
    await waitUntil(
      () => bodyIncludes("Device unreachable"),
      "the device-unreachable error",
    );
    expect(requiredElement("[role='alert']", HTMLElement).textContent).toBe(
      "Failed to fetch",
    );

    planSettingsGet();
    planBlobList([]);
    planNoSend();
    await click(buttonWithText("Retry"));
    await waitUntil(
      () => bodyIncludes("Create Your First Repository"),
      "Create Your First Repository after retry",
    );
    await view.unmount();
  });

  test("reports an invalid settings response distinctly from device-unreachable", async () => {
    planFetch(() =>
      jsonResponse(
        { error: { code: "internal_error", message: "Settings corrupted." } },
        500,
      ),
    );
    const view = await renderScreen();
    await waitUntil(
      () => bodyIncludes("Couldn't load device settings"),
      "the invalid-settings error",
    );
    expect(requiredElement("[role='alert']", HTMLElement).textContent).toBe(
      "Settings corrupted.",
    );
    await view.unmount();
  });

  test("no blobs opens Create Your First Repository; creating the first package produces a dirty, unsaved working copy", async () => {
    planSettingsGet();
    planBlobList([]);
    planNoSend();
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(
      () => bodyIncludes("Create Your First Repository"),
      "Create Your First Repository",
    );
    expect(document.body.textContent).toContain(
      "exists only in this browser tab",
    );
    expect(document.body.textContent).toContain("Unsaved changes");

    await setInputValue(
      requiredElement("#first-package-name", HTMLInputElement),
      "My First Package",
    );
    const put = planSettingsPutAccepted();
    await submit(requiredElement("form", HTMLFormElement));
    await waitUntil(
      () => onReady.mock.calls.length > 0,
      "onReady after create",
    );

    expect(put.called()).toBe(true);
    expect(onReady).toHaveBeenCalledTimes(1);
    const ready = onReady.mock.calls[0]?.[0];
    expect(ready).toBeDefined();
    if (ready === undefined) {
      throw new Error("unreachable");
    }
    expect(ready.store.getIsDirty()).toBe(true);
    expect(ready.store.getRepository().packages).toHaveLength(1);
    expect(ready.store.getRepository().packages[0]?.name).toBe(
      "My First Package",
    );
    expect(ready.packageId).toBe(ready.store.getRepository().packages[0]?.id);
    await view.unmount();
  });

  test("an empty but otherwise valid loaded repository asks for the first package with different framing", async () => {
    planSettingsGet();
    planBlobList([{ id: "1", sizeBytes: 10 }]);
    const emptyRepository: Repository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [],
    };
    planBlobGet(
      "1",
      await gzipCompress(
        new TextEncoder().encode(serializeRepository(emptyRepository)),
      ),
    );
    const view = await renderScreen();
    await waitUntil(
      () => bodyIncludes("Create your first package"),
      "Create your first package",
    );

    expect(document.body.textContent).not.toContain(
      "Create Your First Repository",
    );
    await view.unmount();
  });

  test("resolves the sole package automatically and persists the new default selection", async () => {
    planSettingsGet({ ...baseSettings, lastSelectedPackageId: null });
    planBlobList([{ id: "7", sizeBytes: 42 }]);
    planBlobGet("7", await canonicalGzipBytes());
    planNoSend();
    const put = planSettingsPutAccepted();
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(() => onReady.mock.calls.length > 0, "onReady");

    expect(put.called()).toBe(true);
    expect(onReady).toHaveBeenCalledTimes(1);
    const ready = onReady.mock.calls[0]?.[0];
    expect(ready?.packageId).toBe(canonicalPackageId);
    expect(ready?.sendRecovery).toEqual({ kind: "none" });
    expect(ready?.store.getRepository()).toEqual(canonical);
    await view.unmount();
  });

  test("resolves lastSelectedPackageId without persisting when it is already current", async () => {
    planSettingsGet({
      ...baseSettings,
      lastSelectedPackageId: canonicalPackageId,
    });
    planBlobList([{ id: "1", sizeBytes: 10 }]);
    planBlobGet("1", await canonicalGzipBytes());
    planNoSend();
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(() => onReady.mock.calls.length > 0, "onReady");

    expect(onReady).toHaveBeenCalledTimes(1);
    expect(onReady.mock.calls[0]?.[0]?.packageId).toBe(canonicalPackageId);
    expect(getFetchCalls()).toHaveLength(4);
    expect(getFetchCalls().some((call) => call.method === "PUT")).toBe(false);
    await view.unmount();
  });

  test("passes a recovered non-terminal send status through to onReady", async () => {
    const sendStatus: SendStatusResponse = {
      id: "6ba7b810-9dad-41d1-80b4-00c04fd430ca",
      state: "running",
      actionIndex: 1,
      actionCount: 3,
      estimatedDurationMs: 500,
      cancellationRequested: false,
      error: "",
      releaseError: "",
    };
    planSettingsGet({
      ...baseSettings,
      lastSelectedPackageId: canonicalPackageId,
    });
    planBlobList([{ id: "1", sizeBytes: 10 }]);
    planBlobGet("1", await canonicalGzipBytes());
    planSend(sendStatus);
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(() => onReady.mock.calls.length > 0, "onReady");

    expect(onReady).toHaveBeenCalledWith({
      store: expect.anything() as unknown,
      packageId: canonicalPackageId,
      sendRecovery: { kind: "known", send: sendStatus },
      loadedBlobId: "1",
    });
    await view.unmount();
  });

  test("preserves an unavailable send-recovery state instead of reporting no send", async () => {
    planSettingsGet({
      ...baseSettings,
      lastSelectedPackageId: canonicalPackageId,
    });
    planBlobList([{ id: "1", sizeBytes: 10 }]);
    planBlobGet("1", await canonicalGzipBytes());
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/send");
      expect(call.method).toBe("GET");
      throw new TypeError("send recovery network failure");
    });
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(() => onReady.mock.calls.length > 0, "onReady");

    expect(onReady.mock.calls[0]?.[0]?.sendRecovery).toEqual({
      kind: "unavailable",
      message: "send recovery network failure",
    });
    await view.unmount();
  });

  test("always loads the newest of several blobs, never showing a chooser merely because more than one exists", async () => {
    planSettingsGet({
      ...baseSettings,
      lastSelectedPackageId: canonicalPackageId,
    });
    planBlobList([
      { id: "9", sizeBytes: 10 },
      { id: "8", sizeBytes: 10 },
      { id: "7", sizeBytes: 10 },
    ]);
    planBlobGet("9", await canonicalGzipBytes());
    planNoSend();
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(() => onReady.mock.calls.length > 0, "onReady");

    expect(getFetchCalls().map((call) => call.url)).toEqual([
      "/api/v1/settings",
      "/api/v1/blob",
      "/api/v1/blob/9",
      "/api/v1/send",
    ]);
    expect(document.body.textContent).not.toContain("Choose a package");
    expect(onReady).toHaveBeenCalledTimes(1);
    await view.unmount();
  });

  test("shows the package chooser for several packages with no resolvable selection, and persists the explicit choice", async () => {
    planSettingsGet({ ...baseSettings, lastSelectedPackageId: null });
    planBlobList([{ id: "2", sizeBytes: 20 }]);
    planBlobGet(
      "2",
      await gzipCompress(
        new TextEncoder().encode(serializeRepository(twoPackageRepository)),
      ),
    );
    planNoSend();
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(
      () => bodyIncludes("Choose a package"),
      "the package chooser",
    );

    const put = planSettingsPutAccepted();
    await click(buttonWithText("Second package (0 macros)"));
    await waitUntil(
      () => onReady.mock.calls.length > 0,
      "onReady after choose",
    );

    expect(put.called()).toBe(true);
    expect(onReady).toHaveBeenCalledTimes(1);
    expect(onReady.mock.calls[0]?.[0]?.packageId).toBe(secondPackageId);
    await view.unmount();
  });

  test("package chooser persistence failure still opens locally and carries a visible-warning handoff", async () => {
    planSettingsGet({ ...baseSettings, lastSelectedPackageId: null });
    planBlobList([{ id: "2", sizeBytes: 20 }]);
    planBlobGet(
      "2",
      await gzipCompress(
        new TextEncoder().encode(serializeRepository(twoPackageRepository)),
      ),
    );
    planNoSend();
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(
      () => bodyIncludes("Choose a package"),
      "the package chooser",
    );

    const failure = new TypeError("settings unavailable");
    planSettingsPutFailure(failure);
    await click(buttonWithText("Second package (0 macros)"));
    await waitUntil(
      () => onReady.mock.calls.length > 0,
      "onReady after failed selection persistence",
    );

    const ready = onReady.mock.calls[0]?.[0];
    expect(ready?.packageId).toBe(secondPackageId);
    expect(ready?.selectionPersistenceFailure).toEqual({
      packageId: secondPackageId,
      previousPackageId: null,
      error: failure,
    });
    await view.unmount();
  });

  test("recovers from an invalid newest snapshot by explicitly loading an alternate, never falling back silently", async () => {
    planSettingsGet({
      ...baseSettings,
      lastSelectedPackageId: canonicalPackageId,
    });
    planBlobList([
      { id: "9", sizeBytes: 5 },
      { id: "8", sizeBytes: 42 },
    ]);
    planBlobGet("9", new Uint8Array([1, 2, 3]));
    const onReady = vi.fn<(ready: RepositoryStartupReady) => void>();
    const view = await renderScreen(onReady);
    await waitUntil(
      () => bodyIncludes("Snapshot recovery"),
      "the snapshot recovery screen",
    );

    expect(document.body.textContent).toContain(
      "Blob 9 could not be opened automatically.",
    );
    expect(onReady).not.toHaveBeenCalled();

    planBlobGet("8", await canonicalGzipBytes());
    planNoSend();
    await click(buttonWithText("Load snapshot 8 (42 bytes)"));
    await waitUntil(
      () => onReady.mock.calls.length > 0,
      "onReady after recovery",
    );

    expect(onReady).toHaveBeenCalledTimes(1);
    expect(onReady.mock.calls[0]?.[0]?.packageId).toBe(canonicalPackageId);
    await view.unmount();
  });

  test("shows no alternates when the only stored blob is the failing one", async () => {
    planSettingsGet();
    planBlobList([{ id: "5", sizeBytes: 5 }]);
    planBlobGet("5", new Uint8Array([9, 9, 9]));
    const view = await renderScreen();
    await waitUntil(
      () => bodyIncludes("No other stored snapshots are available to try."),
      "the no-alternates message",
    );
    await view.unmount();
  });

  test("Start over from snapshot recovery re-runs the full sequence", async () => {
    planSettingsGet();
    planBlobList([{ id: "5", sizeBytes: 5 }]);
    planBlobGet("5", new Uint8Array([9, 9, 9]));
    const view = await renderScreen();
    await waitUntil(
      () => bodyIncludes("Snapshot recovery"),
      "the snapshot recovery screen",
    );

    planSettingsGet();
    planBlobList([]);
    planNoSend();
    await click(buttonWithText("Start over"));
    await waitUntil(
      () => bodyIncludes("Create Your First Repository"),
      "Create Your First Repository after Start over",
    );
    await view.unmount();
  });
});
