import {
  V2ApiError,
  v2DeleteNoContent,
  v2GetBinary,
  v2GetJson,
  v2PostBinary,
} from "./apiClient";
import { isBlobCreatedResponse, isBlobListResponse } from "./apiContracts";
import type { BlobCreatedResponse, BlobListResponse } from "./apiTypes";
import { GzipUnsupportedError } from "./gzip";
import { v2Limits } from "./limits";
import type { Repository, RepositoryValidationIssue } from "./repository";
import {
  decodeRepositorySnapshot,
  encodeRepositorySnapshot,
  RepositoryDecodeError,
} from "./repositoryCodec";
import type { RepositoryWorkingCopyStore } from "./repositoryWorkingCopy";
import { validateRepositoryForUse } from "./repositoryValidation";

/**
 * Manual gzip snapshot client, per SPEC_V2 §9, §10 and TODO_V2 V2-073. Every
 * operation talks to the opaque `/api/v1/blob*` routes only — the firmware
 * has no concept of packages, macros, or a "current" blob (SPEC_V2 §10.2).
 * React chooses which blob to load, and never falls back to storing
 * uncompressed data.
 */

const gzipContentType = "application/gzip";
const blobBasePath = "/api/v1/blob";

function blobPath(id: string): string {
  return `${blobBasePath}/${encodeURIComponent(id)}`;
}

export interface SnapshotListResult {
  blobs: readonly { id: string; sizeBytes: number }[];
  usedBytes: number;
  remainingBytes: number;
}

/** `GET /api/v1/blob` — newest first by numeric ID (enforced by the response guard). */
export async function listSnapshots(): Promise<SnapshotListResult> {
  const response: BlobListResponse = await v2GetJson(
    blobBasePath,
    isBlobListResponse,
  );
  return response;
}

export type SnapshotCommitReconciliation =
  | { state: "matched"; blobId: string }
  | { state: "not_found" }
  | { state: "unavailable" }
  | { state: "ambiguous"; matchingBlobIds: readonly string[] };

interface PendingSnapshotCommit {
  readonly beforeIds: ReadonlySet<string>;
  readonly bytes: Uint8Array;
}

const pendingSnapshotCommits = new WeakMap<
  RepositoryWorkingCopyStore,
  PendingSnapshotCommit
>();

function byteArraysEqual(left: Uint8Array, right: Uint8Array): boolean {
  if (left.byteLength !== right.byteLength) {
    return false;
  }
  for (let index = 0; index < left.byteLength; index += 1) {
    if (left[index] !== right[index]) {
      return false;
    }
  }
  return true;
}

async function reconcilePendingSnapshotCommit(
  pending: PendingSnapshotCommit,
): Promise<SnapshotCommitReconciliation> {
  let current: SnapshotListResult;
  try {
    current = await listSnapshots();
  } catch {
    return { state: "unavailable" };
  }

  const newBlobs = current.blobs.filter(
    (blob) => !pending.beforeIds.has(blob.id),
  );
  if (newBlobs.length === 0) {
    return { state: "not_found" };
  }

  const matchingBlobIds: string[] = [];
  for (const blob of newBlobs) {
    let storedBytes: Uint8Array;
    try {
      storedBytes = await downloadSnapshotBytes(blob.id);
    } catch {
      return { state: "unavailable" };
    }
    if (byteArraysEqual(storedBytes, pending.bytes)) {
      matchingBlobIds.push(blob.id);
    }
  }

  if (matchingBlobIds.length === 0) {
    return { state: "not_found" };
  }
  if (matchingBlobIds.length === 1) {
    return { state: "matched", blobId: matchingBlobIds[0] ?? "" };
  }
  return { state: "ambiguous", matchingBlobIds };
}

function reconciliationMessage(
  reconciliation: SnapshotCommitReconciliation,
): string {
  switch (reconciliation.state) {
    case "matched":
      return `The device activated snapshot ${reconciliation.blobId}, but final durability acknowledgement was lost. No duplicate upload was sent. The working copy remains dirty; load the recovered snapshot to accept it.`;
    case "not_found":
      return "The device reported an uncertain snapshot commit, but reconciliation found no matching new blob. No automatic retry was sent; choose Save again to retry explicitly.";
    case "ambiguous":
      return "The device reported an uncertain snapshot commit and more than one new blob matches the exact uploaded bytes. Saving is blocked until the stored snapshots are reconciled.";
    case "unavailable":
      return "The device reported an uncertain snapshot commit, but canonical blob state could not be reconciled. Saving is blocked until reconciliation succeeds.";
  }
}

export class SnapshotCommitUncertainError extends Error {
  public readonly reconciliation: SnapshotCommitReconciliation;

  public constructor(reconciliation: SnapshotCommitReconciliation) {
    super(reconciliationMessage(reconciliation));
    this.name = "SnapshotCommitUncertainError";
    this.reconciliation = reconciliation;
  }
}

export class SnapshotTooLargeError extends Error {
  public constructor(sizeBytes: number) {
    super(
      `The repository snapshot is ${String(sizeBytes)} bytes, which exceeds the ${String(v2Limits.blobMaxBytes)}-byte maximum accepted blob size.`,
    );
    this.name = "SnapshotTooLargeError";
  }
}

/**
 * Raised when {@link saveWorkingCopyAsSnapshot} finds the working copy fails
 * full repository validation — SPEC_V2 §8.5 "Before React accepts a
 * repository as a working copy or uploads it as a snapshot, it MUST
 * validate..." (TODO_V2 V2-110). In practice every editing path already
 * enforces field-level limits before calling
 * {@link import("./repositoryWorkingCopy").RepositoryWorkingCopyStore.applyContentChange},
 * so this is defense in depth, not the primary gate — but it is the one
 * point that unconditionally guards every byte this application ever
 * uploads.
 */
export class SnapshotValidationError extends Error {
  public readonly issues: readonly RepositoryValidationIssue[];
  public constructor(issues: readonly RepositoryValidationIssue[]) {
    super("The working copy failed repository validation and was not saved.");
    this.name = "SnapshotValidationError";
    this.issues = issues;
  }
}

/**
 * Serializes and gzip-compresses the current working copy and uploads it as
 * a new blob (`POST /api/v1/blob`, per SPEC_V2 §10.3). A pre-create list is
 * recorded so a `503 commit_uncertain` response can be reconciled using only
 * canonical GETs and exact raw-gzip byte comparison. The client never retries
 * that POST automatically. Unavailable or ambiguous reconciliation remains
 * latched per working-copy store and blocks later POSTs until GET reconciliation
 * succeeds. Only an actual `201 Created` clears dirty state automatically.
 */
export async function saveWorkingCopyAsSnapshot(
  store: RepositoryWorkingCopyStore,
): Promise<BlobCreatedResponse> {
  const repository = store.getRepository();
  const validated = validateRepositoryForUse(repository);
  if (!validated.ok) {
    throw new SnapshotValidationError(validated.issues);
  }
  const bytes = await encodeRepositorySnapshot(validated.value);
  if (bytes.byteLength > v2Limits.blobMaxBytes) {
    throw new SnapshotTooLargeError(bytes.byteLength);
  }

  const pending = pendingSnapshotCommits.get(store);
  if (pending !== undefined) {
    const reconciliation = await reconcilePendingSnapshotCommit(pending);
    if (reconciliation.state === "not_found") {
      pendingSnapshotCommits.delete(store);
    }
    throw new SnapshotCommitUncertainError(reconciliation);
  }

  const before = await listSnapshots();
  const pendingCommit: PendingSnapshotCommit = {
    beforeIds: new Set(before.blobs.map((blob) => blob.id)),
    bytes: new Uint8Array(bytes),
  };

  let created: BlobCreatedResponse;
  try {
    created = await v2PostBinary(
      blobBasePath,
      bytes,
      gzipContentType,
      isBlobCreatedResponse,
    );
  } catch (error: unknown) {
    if (error instanceof V2ApiError && error.code === "commit_uncertain") {
      pendingSnapshotCommits.set(store, pendingCommit);
      const reconciliation =
        await reconcilePendingSnapshotCommit(pendingCommit);
      if (reconciliation.state === "not_found") {
        pendingSnapshotCommits.delete(store);
      }
      throw new SnapshotCommitUncertainError(reconciliation);
    }
    throw error;
  }

  pendingSnapshotCommits.delete(store);
  store.markSaved(validated.value);
  return created;
}

export type LoadSnapshotResult =
  | { ok: true; repository: Repository; created: false }
  | { ok: false; reason: "gzip_unsupported" }
  | { ok: false; reason: "unreadable"; message: string }
  | {
      ok: false;
      reason: "invalid";
      issues: readonly RepositoryValidationIssue[];
    };

/**
 * Downloads a stored blob, decompresses it, and validates it completely
 * before ever touching the working copy (TODO_V2 V2-073 "Validate before
 * replacing the working copy"). The working copy is replaced — as a
 * deliberate, non-dirtying replacement — only when validation succeeds; on
 * any failure the store is left untouched and the caller receives a typed
 * reason so the UI can let the user pick another snapshot instead of
 * silently falling back (SPEC_V2 §9).
 */
export async function loadSnapshotIntoWorkingCopy(
  id: string,
  store: RepositoryWorkingCopyStore,
): Promise<LoadSnapshotResult> {
  const bytes = await v2GetBinary(blobPath(id), gzipContentType);
  let parsed: unknown;
  try {
    parsed = await decodeRepositorySnapshot(bytes);
  } catch (error: unknown) {
    if (error instanceof GzipUnsupportedError) {
      return { ok: false, reason: "gzip_unsupported" };
    }
    if (error instanceof RepositoryDecodeError) {
      return { ok: false, reason: "unreadable", message: error.message };
    }
    throw error;
  }

  const validated = validateRepositoryForUse(parsed);
  if (!validated.ok) {
    return { ok: false, reason: "invalid", issues: validated.issues };
  }

  store.replaceWorkingCopy(validated.value);
  pendingSnapshotCommits.delete(store);
  return { ok: true, repository: validated.value, created: false };
}

/**
 * Downloads a stored blob's raw bytes without decompressing, validating, or
 * replacing the working copy — for the user to save to local disk
 * (SPEC_V2 §10.4 "no transform").
 */
export async function downloadSnapshotBytes(id: string): Promise<Uint8Array> {
  return v2GetBinary(blobPath(id), gzipContentType);
}

/** `DELETE /api/v1/blob/{id}` (`204`). Always an explicit user action (SPEC_V2 §10.5). */
export async function deleteSnapshot(id: string): Promise<void> {
  await v2DeleteNoContent(blobPath(id));
}

export type ReplaceSnapshotResult =
  | { ok: true; deletedId: string; created: BlobCreatedResponse }
  | { ok: false; stage: "delete"; deletedId: null; error: unknown }
  | { ok: false; stage: "add"; deletedId: string; error: unknown };

/**
 * The advanced, explicitly non-atomic "replace this snapshot" flow, per
 * SPEC_V2 §10.6 and UI_UX_SPEC_V2 §9.7 (TODO_V2 V2-116): "There is no `PUT`
 * and no atomic blob-replace operation... React performs two independent
 * operations: 1. delete the selected blob; 2. add the replacement as a new
 * blob." Ordinary saves (`saveWorkingCopyAsSnapshot`) never call this —
 * normal snapshot saving only ever adds.
 *
 * If the delete fails, nothing else happens and the original blob is
 * untouched (`stage: "delete"`). If the delete succeeds but the subsequent
 * add fails, the deleted blob is already gone and stays gone — the caller
 * MUST warn about exactly that non-atomic consequence before ever calling
 * this (`stage: "add"`, `deletedId` identifies what was already removed).
 * The working copy itself is never touched by this function; a failed add
 * leaves it exactly as `saveWorkingCopyAsSnapshot` would (dirty, unchanged).
 */
export async function replaceSnapshotWithWorkingCopy(
  id: string,
  store: RepositoryWorkingCopyStore,
): Promise<ReplaceSnapshotResult> {
  try {
    await deleteSnapshot(id);
  } catch (error: unknown) {
    return { ok: false, stage: "delete", deletedId: null, error };
  }
  try {
    const created = await saveWorkingCopyAsSnapshot(store);
    return { ok: true, deletedId: id, created };
  } catch (error: unknown) {
    return { ok: false, stage: "add", deletedId: id, error };
  }
}

export interface ExportedRepository {
  bytes: Uint8Array;
  filename: string;
  mimeType: "application/gzip";
}

/**
 * Exports the given repository (normally the current working copy) as the
 * same gzip representation used for device storage, per SPEC_V2 §8.8. The
 * filename ends with the spec's recommended suffix `.emk-repository.json.gz`;
 * SPEC_V2 does not mandate a base name, so a fixed, descriptive one is used.
 */
export async function exportRepository(
  repository: Repository,
): Promise<ExportedRepository> {
  const bytes = await encodeRepositorySnapshot(repository);
  return {
    bytes,
    filename: "repository.emk-repository.json.gz",
    mimeType: "application/gzip",
  };
}

export type ImportResult =
  | {
      ok: true;
      repository: Repository;
      packageCount: number;
      macroCount: number;
    }
  | { ok: false; reason: "gzip_unsupported" }
  | { ok: false; reason: "unreadable"; message: string }
  | {
      ok: false;
      reason: "invalid";
      issues: readonly RepositoryValidationIssue[];
    };

/**
 * Reads, decompresses, decodes, and validates an imported repository file,
 * per SPEC_V2 §8.8 steps 1-5. Import does NOT automatically upload a
 * snapshot: on success, the caller is expected to show the returned package
 * and macro counts (step 6) and then call
 * {@link RepositoryWorkingCopyStore.applyImport} to replace the working copy
 * and mark it dirty (step 7). This function itself never touches the store,
 * matching {@link loadSnapshotIntoWorkingCopy}'s validate-before-replace
 * contract, and never uploads anything.
 */
export async function importRepository(
  bytes: Uint8Array,
): Promise<ImportResult> {
  let parsed: unknown;
  try {
    parsed = await decodeRepositorySnapshot(bytes);
  } catch (error: unknown) {
    if (error instanceof GzipUnsupportedError) {
      return { ok: false, reason: "gzip_unsupported" };
    }
    if (error instanceof RepositoryDecodeError) {
      return { ok: false, reason: "unreadable", message: error.message };
    }
    throw error;
  }

  const validated = validateRepositoryForUse(parsed);
  if (!validated.ok) {
    return { ok: false, reason: "invalid", issues: validated.issues };
  }

  const macroCount = validated.value.packages.reduce(
    (total, pkg) => total + pkg.macros.length,
    0,
  );
  return {
    ok: true,
    repository: validated.value,
    packageCount: validated.value.packages.length,
    macroCount,
  };
}
