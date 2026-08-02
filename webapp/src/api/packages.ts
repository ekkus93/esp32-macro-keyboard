import { apiRawJsonRequest, apiRequest } from "./client";
import { isMacroSet, isRecord } from "./guards";
import type { MacroSet } from "../types/models";

interface PackageDocumentBase {
  schema_version: 1;
  sets: unknown[];
  macros: unknown[];
  procedures: unknown[];
  progress: unknown[];
}

export interface SetPackageDocument extends PackageDocumentBase {
  package_type: "set";
}

export interface BackupPackageDocument extends PackageDocumentBase {
  package_type: "backup";
}

export interface PackageDownload {
  text: string;
  byteLength: number;
}

export interface RestoreResult {
  restored: true;
  reloadRequired: true;
}

const packageKeys = [
  "schema_version",
  "package_type",
  "sets",
  "macros",
  "procedures",
  "progress",
] as const;

function hasExactPackageShape(value: unknown): value is PackageDocumentBase & {
  package_type: unknown;
} {
  if (!isRecord(value)) {
    return false;
  }
  const keys = Object.keys(value);
  return (
    keys.length === packageKeys.length &&
    packageKeys.every((key) => keys.includes(key)) &&
    value.schema_version === 1 &&
    Array.isArray(value.sets) &&
    Array.isArray(value.macros) &&
    Array.isArray(value.procedures) &&
    Array.isArray(value.progress)
  );
}

export function isSetPackageDocument(
  value: unknown,
): value is SetPackageDocument {
  return (
    hasExactPackageShape(value) &&
    value.package_type === "set" &&
    value.sets.length === 1
  );
}

export function isBackupPackageDocument(
  value: unknown,
): value is BackupPackageDocument {
  return hasExactPackageShape(value) && value.package_type === "backup";
}

function isRestoreResult(value: unknown): value is RestoreResult {
  return (
    isRecord(value) &&
    Object.keys(value).length === 2 &&
    value.restored === true &&
    value.reloadRequired === true
  );
}

export async function exportSetPackage(
  setId: string,
): Promise<PackageDownload> {
  const response = await apiRawJsonRequest(
    `/api/v1/sets/${encodeURIComponent(setId)}/export`,
    isSetPackageDocument,
    { timeoutMs: 30_000 },
  );
  return {
    text: response.text,
    byteLength: response.byteLength,
  };
}

export async function exportBackupPackage(): Promise<PackageDownload> {
  const response = await apiRawJsonRequest(
    "/api/v1/backup",
    isBackupPackageDocument,
    { timeoutMs: 60_000 },
  );
  return {
    text: response.text,
    byteLength: response.byteLength,
  };
}

export async function replaceSetPackage(
  targetSetId: string,
  expectedRevision: number,
  packageDocument: SetPackageDocument,
): Promise<MacroSet> {
  return apiRequest(
    "/api/v1/sets/import",
    {
      method: "POST",
      body: JSON.stringify({
        targetSetId,
        expectedRevision,
        package: packageDocument,
      }),
    },
    isMacroSet,
    { timeoutMs: 30_000 },
  );
}

export async function importSetAsNewPackage(
  newSetId: string,
  packageDocument: SetPackageDocument,
): Promise<MacroSet> {
  return apiRequest(
    "/api/v1/sets/import-new",
    {
      method: "POST",
      body: JSON.stringify({
        newSetId,
        package: packageDocument,
      }),
    },
    isMacroSet,
    { timeoutMs: 30_000 },
  );
}

export async function restoreBackupPackage(
  packageDocument: BackupPackageDocument,
): Promise<RestoreResult> {
  return apiRequest(
    "/api/v1/restore",
    {
      method: "POST",
      body: JSON.stringify(packageDocument),
    },
    isRestoreResult,
    { timeoutMs: 60_000 },
  );
}
