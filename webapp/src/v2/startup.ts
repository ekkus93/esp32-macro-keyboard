import { V2ApiError } from "./apiClient";
import { resolveSelectedPackage } from "./packageSelection";
import { createEmptyRepository } from "./repositoryValidation";
import type { Repository, RepositoryValidationIssue } from "./repository";
import type {
  BlobSummary,
  SendStatusResponse,
  SettingsResponse,
} from "./apiTypes";
import type { LoadSnapshotResult } from "./snapshotClient";
import { createRepositoryWorkingCopyStore } from "./repositoryWorkingCopy";
import type { RepositoryWorkingCopyStore } from "./repositoryWorkingCopy";

/**
 * The authenticated startup state machine, per SPEC_V2 §14.1 and
 * UI_UX_SPEC_V2 §3.4/§3.6 (TODO_V2 V2-082/V2-083/V2-084).
 *
 * This module is the pure, dependency-injected decision core: it performs
 * the exact ordered load sequence from UI_UX_SPEC_V2 §3.4 steps 2-9 and
 * returns one typed, exhaustive outcome. It never touches the DOM or
 * `fetch` directly — a caller supplies {@link StartupDependencies}, which in
 * production wraps the real v2 API clients and in tests can simulate any
 * combination of provisioning/session/blob/package state without a network.
 *
 * What this module deliberately does NOT decide:
 *  - whether to run at all (UI_UX_SPEC_V2 §3.3's "tab still holds a live
 *    working copy" case skips this module entirely — that is a composition
 *    concern for the caller, since only the caller knows whether a store
 *    already exists);
 *  - anything about session/provisioning state (V2-080/V2-081's concern);
 *  - screen rendering (`RepositoryStartupScreen` owns that).
 */

/** The three ways a loaded (or freshly created) repository resolves a package. */
export type StartupPackageDestination =
  | { kind: "first-package" }
  | { kind: "package-chooser" }
  | { kind: "resolved"; packageId: string; shouldPersist: boolean };

/**
 * Resolves the package destination for an already-validated repository, per
 * UI_UX_SPEC_V2 §3.4's normal destination rules and §3.6's exact 3-step
 * algorithm (reused unmodified from V2-074 — see {@link resolveSelectedPackage}).
 * An empty repository (no packages at all) always means "ask for the first
 * package name," which is checked before package-selection resolution since
 * {@link resolveSelectedPackage} does not itself special-case zero packages.
 */
export function resolvePackageDestination(
  repository: Repository,
  lastSelectedPackageId: string | null,
): StartupPackageDestination {
  if (repository.packages.length === 0) {
    return { kind: "first-package" };
  }
  const resolution = resolveSelectedPackage(repository, lastSelectedPackageId);
  return resolution.kind === "chooser"
    ? { kind: "package-chooser" }
    : {
        kind: "resolved",
        packageId: resolution.packageId,
        shouldPersist: resolution.shouldPersist,
      };
}

export interface StartupDependencies {
  /** SPEC_V2 §9 "React MUST feature-detect both APIs during startup." */
  isGzipSupported: () => boolean;
  /** UI_UX_SPEC_V2 §3.4 step 2. */
  getSettings: () => Promise<SettingsResponse>;
  /** UI_UX_SPEC_V2 §3.4 step 3. */
  listSnapshots: () => Promise<{
    blobs: readonly BlobSummary[];
    usedBytes: number;
    remainingBytes: number;
  }>;
  /**
   * UI_UX_SPEC_V2 §3.4 steps 4-7: downloads, decompresses, and validates the
   * given blob ID, replacing `store`'s working copy only on success (the
   * exact contract of {@link import("./snapshotClient").loadSnapshotIntoWorkingCopy}).
   */
  loadBlobIntoStore: (
    id: string,
    store: RepositoryWorkingCopyStore,
  ) => Promise<LoadSnapshotResult>;
  /** UI_UX_SPEC_V2 §3.4 step 8. */
  recoverSendState: () => Promise<SendStatusResponse | null>;
}

export type StartupLoadStage = "settings" | "blobs" | "newest-blob";

export type StartupResult =
  | { kind: "gzip-unsupported" }
  | { kind: "device-unreachable"; stage: StartupLoadStage; message: string }
  | { kind: "invalid-settings"; message: string }
  | { kind: "load-failed"; stage: StartupLoadStage; message: string }
  | { kind: "first-repository"; settings: SettingsResponse }
  | {
      kind: "invalid-newest-snapshot";
      blobId: string;
      reason: "unreadable" | "invalid" | "gzip_unsupported";
      message: string;
      issues: readonly RepositoryValidationIssue[];
      otherBlobs: readonly BlobSummary[];
      settings: SettingsResponse;
    }
  | {
      kind: "ready";
      store: RepositoryWorkingCopyStore;
      settings: SettingsResponse;
      send: SendStatusResponse | null;
      destination: StartupPackageDestination;
      /**
       * The blob ID the working copy was just loaded from (TODO_V2 V2-111
       * "loaded snapshot indicator"). Always populated here: reaching `kind:
       * "ready"` requires a successful {@link StartupDependencies.loadBlobIntoStore}
       * call, which only ever happens against `newest.id`.
       */
      blobId: string;
    };

function invalidSnapshotMessage(result: LoadSnapshotResult): string {
  if (result.ok) {
    throw new Error("invalidSnapshotMessage called with a successful result.");
  }
  switch (result.reason) {
    case "gzip_unsupported":
      return "This browser does not support the compression APIs required to read the newest snapshot.";
    case "unreadable":
      return result.message;
    case "invalid":
      return "The newest snapshot does not contain a valid repository.";
  }
}

/**
 * Runs the ordered UI_UX_SPEC_V2 §3.4 load sequence and returns exactly one
 * outcome. Never throws: every dependency failure is caught and mapped to a
 * typed result so a caller can render every case exhaustively without a
 * try/catch of its own (TODO_V2 V2-084 "preserve recoverable working state
 * and provide precise next actions").
 */
export async function runStartup(
  deps: StartupDependencies,
): Promise<StartupResult> {
  if (!deps.isGzipSupported()) {
    return { kind: "gzip-unsupported" };
  }

  let settings: SettingsResponse;
  try {
    settings = await deps.getSettings();
  } catch (error: unknown) {
    if (error instanceof V2ApiError) {
      return { kind: "invalid-settings", message: error.message };
    }
    return {
      kind: "device-unreachable",
      stage: "settings",
      message: error instanceof Error ? error.message : "Device unreachable.",
    };
  }

  let listed: {
    blobs: readonly BlobSummary[];
    usedBytes: number;
    remainingBytes: number;
  };
  try {
    listed = await deps.listSnapshots();
  } catch (error: unknown) {
    if (error instanceof V2ApiError) {
      return { kind: "load-failed", stage: "blobs", message: error.message };
    }
    return {
      kind: "device-unreachable",
      stage: "blobs",
      message: error instanceof Error ? error.message : "Device unreachable.",
    };
  }

  if (listed.blobs.length === 0) {
    return { kind: "first-repository", settings };
  }

  // §10.2: the blob list is sorted by numeric ID, newest first.
  const newest = listed.blobs[0];
  if (newest === undefined) {
    return { kind: "first-repository", settings };
  }

  const store = createRepositoryWorkingCopyStore(createEmptyRepository());
  let loaded: LoadSnapshotResult;
  try {
    loaded = await deps.loadBlobIntoStore(newest.id, store);
  } catch (error: unknown) {
    if (error instanceof V2ApiError) {
      return {
        kind: "load-failed",
        stage: "newest-blob",
        message: error.message,
      };
    }
    return {
      kind: "device-unreachable",
      stage: "newest-blob",
      message: error instanceof Error ? error.message : "Device unreachable.",
    };
  }

  if (!loaded.ok) {
    return {
      kind: "invalid-newest-snapshot",
      blobId: newest.id,
      reason: loaded.reason,
      message: invalidSnapshotMessage(loaded),
      issues: loaded.reason === "invalid" ? loaded.issues : [],
      otherBlobs: listed.blobs.filter((blob) => blob.id !== newest.id),
      settings,
    };
  }

  // Best-effort: a failure recovering send status must not block reaching
  // the resolved package's Macros page (UI_UX_SPEC_V2 §3.4 step 8 is
  // supplementary to the destination decision made from steps 1-7/9).
  let send: SendStatusResponse | null = null;
  try {
    send = await deps.recoverSendState();
  } catch {
    send = null;
  }

  return {
    kind: "ready",
    store,
    settings,
    send,
    destination: resolvePackageDestination(
      loaded.repository,
      settings.lastSelectedPackageId,
    ),
    blobId: newest.id,
  };
}
