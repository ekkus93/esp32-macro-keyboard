import { apiRawJsonRequest, apiRequest } from "./client";
import { isMacroPackage, isRecord } from "./guards";
import type { MacroPackage } from "../types/models";

interface PackageDocumentBase {
  schema_version: 1;
  packages: unknown[];
  macros: unknown[];
}

export interface PackageDocument extends PackageDocumentBase {
  package_type: "package";
}

export interface BackupPackageDocument extends PackageDocumentBase {
  package_type: "backup";
}

export interface PackageDownload {
  text: string;
  byteLength: number;
}

/* Restore is not atomic across packages (SPEC 13.5), so a success response still
   enumerates which packages landed. A run that failed any of them is not a 200 and
   arrives as an API error instead, with the same per-package detail attached. */
export interface RestorePackageOutcome {
  packageId: string;
  restored: boolean;
  error?: string;
}

export interface RestoreResult {
  restored: true;
  reloadRequired: true;
  setsRestored: number;
  setsFailed: number;
  packages: RestorePackageOutcome[];
}

const packageKeys = [
  "schema_version",
  "package_type",
  "packages",
  "macros",
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
    Array.isArray(value.packages) &&
    Array.isArray(value.macros)
  );
}

export function isPackageDocument(value: unknown): value is PackageDocument {
  return (
    hasExactPackageShape(value) &&
    value.package_type === "package" &&
    value.packages.length === 1
  );
}

export function isBackupPackageDocument(
  value: unknown,
): value is BackupPackageDocument {
  return hasExactPackageShape(value) && value.package_type === "backup";
}

function isRestorePackageOutcome(
  value: unknown,
): value is RestorePackageOutcome {
  if (!isRecord(value) || typeof value.packageId !== "string") {
    return false;
  }
  if (typeof value.restored !== "boolean") {
    return false;
  }
  const keys = Object.keys(value);
  return value.restored
    ? keys.length === 2
    : keys.length === 3 && typeof value.error === "string";
}

function isRestoreResult(value: unknown): value is RestoreResult {
  return (
    isRecord(value) &&
    Object.keys(value).length === 5 &&
    value.restored === true &&
    value.reloadRequired === true &&
    typeof value.setsRestored === "number" &&
    value.setsFailed === 0 &&
    Array.isArray(value.packages) &&
    value.packages.every(isRestorePackageOutcome)
  );
}

export async function exportPackage(
  packageId: string,
): Promise<PackageDownload> {
  const response = await apiRawJsonRequest(
    `/api/v1/package/${encodeURIComponent(packageId)}/export`,
    isPackageDocument,
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

export async function replacePackage(
  targetPackageId: string,
  expectedRevision: number,
  packageDocument: PackageDocument,
): Promise<MacroPackage> {
  return apiRequest(
    "/api/v1/package/import",
    {
      method: "POST",
      body: JSON.stringify({
        targetPackageId,
        expectedRevision,
        package: packageDocument,
      }),
    },
    isMacroPackage,
    { timeoutMs: 30_000 },
  );
}

export async function importPackageAsNewPackage(
  newPackageId: string,
  packageDocument: PackageDocument,
): Promise<MacroPackage> {
  return apiRequest(
    "/api/v1/package/import-new",
    {
      method: "POST",
      body: JSON.stringify({
        newPackageId,
        package: packageDocument,
      }),
    },
    isMacroPackage,
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
