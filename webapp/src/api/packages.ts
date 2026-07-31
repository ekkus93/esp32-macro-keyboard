import { apiRawJsonRequest, apiRequest } from "./client";
import { isMacroSet, isRecord } from "./guards";
import type { MacroSet } from "../types/models";

export interface SetPackageDocument {
  schema_version: 1;
  package_type: "set";
  sets: unknown[];
  macros: unknown[];
  global_macros: unknown[];
  procedures: unknown[];
  progress: unknown[];
}

export interface SetPackageDownload {
  text: string;
  byteLength: number;
}

const packageKeys = [
  "schema_version",
  "package_type",
  "sets",
  "macros",
  "global_macros",
  "procedures",
  "progress",
] as const;

export function isSetPackageDocument(
  value: unknown,
): value is SetPackageDocument {
  if (!isRecord(value)) {
    return false;
  }
  const keys = Object.keys(value);
  return (
    keys.length === packageKeys.length &&
    packageKeys.every((key) => keys.includes(key)) &&
    value.schema_version === 1 &&
    value.package_type === "set" &&
    Array.isArray(value.sets) &&
    value.sets.length === 1 &&
    Array.isArray(value.macros) &&
    Array.isArray(value.global_macros) &&
    Array.isArray(value.procedures) &&
    Array.isArray(value.progress)
  );
}

export async function exportSetPackage(
  setId: string,
): Promise<SetPackageDownload> {
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
