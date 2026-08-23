import { describe, expect, test, vi } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import { V2ApiError } from "../src/v2/apiClient";
import type { Repository } from "../src/v2/repository";
import { createEmptyRepository } from "../src/v2/repositoryValidation";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import {
  resolvePackageDestination,
  runStartup,
  type StartupDependencies,
} from "../src/v2/startup";
import type { SendStatusResponse, SettingsResponse } from "../src/v2/apiTypes";

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
  lastSelectedPackageId: null,
  apSsid: "MacroKeyboard",
  stationConfigured: false,
  stationSsid: null,
};

const sendStatus: SendStatusResponse = {
  id: "1",
  state: "completed",
  actionIndex: 1,
  actionCount: 1,
  estimatedDurationMs: 10,
  cancellationRequested: false,
  error: "",
  releaseError: "",
};

function deps(
  overrides: Partial<StartupDependencies> = {},
): StartupDependencies {
  return {
    isGzipSupported: () => true,
    getSettings: () => Promise.resolve(baseSettings),
    listSnapshots: () =>
      Promise.resolve({ blobs: [], usedBytes: 0, remainingBytes: 100 }),
    loadBlobIntoStore: (_id, store) => {
      store.replaceWorkingCopy(canonical);
      return Promise.resolve({
        ok: true,
        repository: canonical,
        created: false,
      });
    },
    recoverSendState: () => Promise.resolve(null),
    ...overrides,
  };
}

describe("resolvePackageDestination", () => {
  test("resolves lastSelectedPackageId when it identifies a package", () => {
    expect(
      resolvePackageDestination(twoPackageRepository, secondPackageId),
    ).toEqual({
      kind: "resolved",
      packageId: secondPackageId,
      shouldPersist: false,
    });
  });

  test("resolves and marks shouldPersist for a sole package with no matching selection", () => {
    expect(resolvePackageDestination(canonical, null)).toEqual({
      kind: "resolved",
      packageId: canonicalPackageId,
      shouldPersist: true,
    });
  });

  test("shows the chooser for multiple packages with no resolvable selection", () => {
    expect(resolvePackageDestination(twoPackageRepository, null)).toEqual({
      kind: "package-chooser",
    });
  });

  test("shows the chooser when the stored selection no longer exists", () => {
    expect(
      resolvePackageDestination(
        twoPackageRepository,
        "00000000-0000-4000-8000-000000000000",
      ),
    ).toEqual({ kind: "package-chooser" });
  });

  test("asks for the first package when the repository is empty", () => {
    expect(resolvePackageDestination(createEmptyRepository(), null)).toEqual({
      kind: "first-package",
    });
  });
});

describe("runStartup decision table", () => {
  test("unsupported Compression Streams short-circuits before any network call", async () => {
    const getSettings = vi.fn();
    const result = await runStartup(
      deps({ isGzipSupported: () => false, getSettings }),
    );
    expect(result).toEqual({ kind: "gzip-unsupported" });
    expect(getSettings).not.toHaveBeenCalled();
  });

  test("a network failure loading settings reports device-unreachable", async () => {
    const result = await runStartup(
      deps({
        getSettings: () => Promise.reject(new TypeError("Failed to fetch")),
      }),
    );
    expect(result).toEqual({
      kind: "device-unreachable",
      stage: "settings",
      message: "Failed to fetch",
    });
  });

  test("an invalid settings response is reported distinctly from unreachable", async () => {
    const result = await runStartup(
      deps({
        getSettings: () =>
          Promise.reject(
            new V2ApiError(500, { code: "internal_error", message: "Boom." }),
          ),
      }),
    );
    expect(result).toEqual({ kind: "invalid-settings", message: "Boom." });
  });

  test("a network failure listing blobs reports device-unreachable", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () => Promise.reject(new TypeError("net down")),
      }),
    );
    expect(result).toEqual({
      kind: "device-unreachable",
      stage: "blobs",
      message: "net down",
    });
  });

  test("a server error listing blobs reports load-failed", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.reject(
            new V2ApiError(503, {
              code: "subsystem_unavailable",
              message: "Storage unavailable.",
            }),
          ),
      }),
    );
    expect(result).toEqual({
      kind: "load-failed",
      stage: "blobs",
      message: "Storage unavailable.",
    });
  });

  test("no blobs opens Create Your First Repository", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({ blobs: [], usedBytes: 0, remainingBytes: 100 }),
      }),
    );
    expect(result).toEqual({
      kind: "first-repository",
      settings: baseSettings,
      sendRecovery: { kind: "none" },
    });
  });

  test("several valid blobs never trigger a chooser merely because more than one exists", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [
              { id: "5", sizeBytes: 10 },
              { id: "4", sizeBytes: 10 },
              { id: "3", sizeBytes: 10 },
            ],
            usedBytes: 30,
            remainingBytes: 100,
          }),
        loadBlobIntoStore: (id, store) => {
          expect(id).toBe("5"); // newest first, no chooser involved
          store.replaceWorkingCopy(canonical);
          return Promise.resolve({
            ok: true,
            repository: canonical,
            created: false,
          });
        },
      }),
    );
    expect(result.kind).toBe("ready");
  });

  test("a network failure loading the newest blob reports device-unreachable", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "1", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
        loadBlobIntoStore: () => Promise.reject(new TypeError("net down")),
      }),
    );
    expect(result).toEqual({
      kind: "device-unreachable",
      stage: "newest-blob",
      message: "net down",
    });
  });

  test("an unreadable newest blob reports invalid-newest-snapshot without falling back", async () => {
    const loadBlobIntoStore = vi.fn(() =>
      Promise.resolve({
        ok: false as const,
        reason: "unreadable" as const,
        message: "The compressed data could not be decompressed.",
      }),
    );
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [
              { id: "3", sizeBytes: 10 },
              { id: "2", sizeBytes: 10 },
            ],
            usedBytes: 20,
            remainingBytes: 100,
          }),
        loadBlobIntoStore,
      }),
    );
    expect(result).toEqual({
      kind: "invalid-newest-snapshot",
      blobId: "3",
      reason: "unreadable",
      message: "The compressed data could not be decompressed.",
      issues: [],
      otherBlobs: [{ id: "2", sizeBytes: 10 }],
      settings: baseSettings,
    });
    expect(loadBlobIntoStore).toHaveBeenCalledTimes(1);
    expect(loadBlobIntoStore).toHaveBeenCalledWith("3", expect.anything());
  });

  test("an invalid (schema-failing) newest blob reports its structural issues", async () => {
    const issues = [
      { path: "$.packages[0].id", code: "invalid_uuid", message: "Bad ID." },
    ];
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "9", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
        loadBlobIntoStore: () =>
          Promise.resolve({
            ok: false as const,
            reason: "invalid" as const,
            issues,
          }),
      }),
    );
    expect(result).toMatchObject({
      kind: "invalid-newest-snapshot",
      blobId: "9",
      reason: "invalid",
      issues,
      otherBlobs: [],
    });
  });

  test("a gzip-unsupported reason reaching loadBlobIntoStore still reports invalid-newest-snapshot", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "1", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
        loadBlobIntoStore: () =>
          Promise.resolve({
            ok: false as const,
            reason: "gzip_unsupported" as const,
          }),
      }),
    );
    expect(result).toMatchObject({
      kind: "invalid-newest-snapshot",
      reason: "gzip_unsupported",
    });
  });

  test("resolves the sole package and returns recovered send status", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "1", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
        recoverSendState: () => Promise.resolve(sendStatus),
      }),
    );
    expect(result.kind).toBe("ready");
    if (result.kind !== "ready") {
      throw new Error("expected ready");
    }
    expect(result.destination).toEqual({
      kind: "resolved",
      packageId: canonicalPackageId,
      shouldPersist: true,
    });
    expect(result.sendRecovery).toEqual({ kind: "known", send: sendStatus });
    expect(result.store.getRepository()).toEqual(canonical);
    expect(result.store.getIsDirty()).toBe(false);
  });

  test("resolves lastSelectedPackageId over the sole-package default", async () => {
    const result = await runStartup(
      deps({
        getSettings: () =>
          Promise.resolve({
            ...baseSettings,
            lastSelectedPackageId: canonicalPackageId,
          }),
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "1", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
      }),
    );
    expect(result).toMatchObject({
      kind: "ready",
      destination: {
        kind: "resolved",
        packageId: canonicalPackageId,
        shouldPersist: false,
      },
    });
  });

  test("shows the package chooser when several packages exist and none resolve", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "1", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
        loadBlobIntoStore: (_id, store) => {
          store.replaceWorkingCopy(twoPackageRepository);
          return Promise.resolve({
            ok: true,
            repository: twoPackageRepository,
            created: false,
          });
        },
      }),
    );
    expect(result).toMatchObject({
      kind: "ready",
      destination: { kind: "package-chooser" },
    });
  });

  test("an empty but otherwise valid repository asks for the first package", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "1", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
        loadBlobIntoStore: (_id, store) => {
          store.replaceWorkingCopy(createEmptyRepository());
          return Promise.resolve({
            ok: true,
            repository: createEmptyRepository(),
            created: false,
          });
        },
      }),
    );
    expect(result).toMatchObject({
      kind: "ready",
      destination: { kind: "first-package" },
    });
  });

  test("a send-status recovery failure does not block reaching the ready state", async () => {
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "1", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
        recoverSendState: () => Promise.reject(new Error("send poll failed")),
      }),
    );
    expect(result).toMatchObject({
      kind: "ready",
      sendRecovery: { kind: "unavailable", message: "send poll failed" },
    });
  });

  test("loadBlobIntoStore replaces the returned store, not some other instance", async () => {
    const externalStore = createRepositoryWorkingCopyStore(
      createEmptyRepository(),
    );
    const result = await runStartup(
      deps({
        listSnapshots: () =>
          Promise.resolve({
            blobs: [{ id: "1", sizeBytes: 10 }],
            usedBytes: 10,
            remainingBytes: 100,
          }),
        loadBlobIntoStore: (_id, store) => {
          expect(store).not.toBe(externalStore);
          store.replaceWorkingCopy(canonical);
          return Promise.resolve({
            ok: true,
            repository: canonical,
            created: false,
          });
        },
      }),
    );
    expect(result.kind).toBe("ready");
  });
});
