import type {
  ImportResult,
  LoadSnapshotResult,
} from "../../../v2/snapshotClient";

export function loadFailureMessage(
  result: Extract<LoadSnapshotResult, { ok: false }>,
): string {
  switch (result.reason) {
    case "gzip_unsupported":
      return "This browser does not support the compression APIs required to read this snapshot.";
    case "unreadable":
      return result.message;
    case "invalid":
      return "This snapshot does not contain a valid repository.";
  }
}

export function importFailureMessage(
  result: Extract<ImportResult, { ok: false }>,
): string {
  switch (result.reason) {
    case "gzip_unsupported":
      return "This browser does not support the compression APIs required to read this file.";
    case "unreadable":
      return result.message;
    case "invalid":
      return "This file does not contain a valid repository.";
  }
}

export function formatBytes(bytes: number): string {
  return `${String(bytes)} bytes`;
}
