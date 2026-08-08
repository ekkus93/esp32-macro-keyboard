import {
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

export class SnapshotTooLargeError extends Error {
  public constructor(sizeBytes: number) {
    super(
      `The repository snapshot is ${String(sizeBytes)} bytes, which exceeds the ${String(v2Limits.blobMaxBytes)}-byte maximum accepted blob size.`,
    );
    this.name = "SnapshotTooLargeError";
  }
}

/**
 * Serializes and gzip-compresses the current working copy and uploads it as
 * a new blob (`POST /api/v1/blob`, per SPEC_V2 §10.3). On success (`201`),
 * the working copy's dirty flag is cleared via
 * {@link RepositoryWorkingCopyStore.markSaved}. On any failure — including a
 * pre-flight size check against `v2Limits.blobMaxBytes`, network failure, or
 * a server error such as `413`/`507` — the working copy is left untouched
 * and dirty, per TODO_V2 V2-073 "Keep dirty work after failed save."
 */
export async function saveWorkingCopyAsSnapshot(
  store: RepositoryWorkingCopyStore,
): Promise<BlobCreatedResponse> {
  const repository = store.getRepository();
  const bytes = await encodeRepositorySnapshot(repository);
  if (bytes.byteLength > v2Limits.blobMaxBytes) {
    throw new SnapshotTooLargeError(bytes.byteLength);
  }
  const created: BlobCreatedResponse = await v2PostBinary(
    blobBasePath,
    bytes,
    gzipContentType,
    isBlobCreatedResponse,
  );
  store.markSaved(repository);
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
