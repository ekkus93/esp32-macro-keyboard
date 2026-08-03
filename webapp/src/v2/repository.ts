const textEncoder = new TextEncoder();

const canonicalUuidV4Pattern =
  /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;

const repositoryKeys = ["format", "packages", "schemaVersion"] as const;
const packageKeys = ["id", "macros", "name"] as const;
const macroKeys = ["id", "interKeyMs", "keyPressMs", "name", "source"] as const;

const nameMaximumBytes = 64;
const sourceMaximumBytes = 4096;
const timingMaximumMs = 10_000;

export interface RepositoryMacroV1 {
  id: string;
  name: string;
  source: string;
  keyPressMs: number;
  interKeyMs: number;
}

export interface RepositoryPackageV1 {
  id: string;
  name: string;
  macros: RepositoryMacroV1[];
}

export interface RepositoryV1 {
  format: "esp32-macro-keyboard-repository";
  schemaVersion: 1;
  packages: RepositoryPackageV1[];
}

export interface RepositoryValidationIssue {
  path: string;
  code: string;
  message: string;
}

export type MacroSourceValidator = (source: string) => string | null;

export type RepositoryValidationResult =
  | { ok: true; value: RepositoryV1 }
  | { ok: false; issues: RepositoryValidationIssue[] };

function utf8ByteLength(value: string): number {
  return textEncoder.encode(value).byteLength;
}

function isPlainRecord(value: unknown): value is Record<string, unknown> {
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    return false;
  }
  return Object.getPrototypeOf(value) === Object.prototype;
}

function hasExactKeys(
  value: Record<string, unknown>,
  expected: readonly string[],
): boolean {
  const actual = Object.keys(value).sort();
  const sortedExpected = [...expected].sort();
  return (
    actual.length === sortedExpected.length &&
    actual.every((key, index) => key === sortedExpected[index])
  );
}

function isDenseArray(value: unknown[]): boolean {
  for (let index = 0; index < value.length; index += 1) {
    if (!Object.prototype.hasOwnProperty.call(value, index)) {
      return false;
    }
  }
  return true;
}

function isCanonicalUuidV4(value: unknown): value is string {
  return typeof value === "string" && canonicalUuidV4Pattern.test(value);
}

function isBoundedInteger(
  value: unknown,
  minimum: number,
  maximum: number,
): value is number {
  return (
    typeof value === "number" &&
    Number.isSafeInteger(value) &&
    Number.isFinite(value) &&
    value >= minimum &&
    value <= maximum
  );
}

function addIssue(
  issues: RepositoryValidationIssue[],
  path: string,
  code: string,
  message: string,
): void {
  issues.push({ path, code, message });
}

function validateName(
  value: unknown,
  path: string,
  issues: RepositoryValidationIssue[],
): value is string {
  if (typeof value !== "string") {
    addIssue(issues, path, "invalid_type", "Name must be a string.");
    return false;
  }
  if (value.trim().length === 0) {
    addIssue(issues, path, "required", "Name must not be empty.");
    return false;
  }
  if (utf8ByteLength(value) > nameMaximumBytes) {
    addIssue(
      issues,
      path,
      "too_long",
      `Name exceeds ${String(nameMaximumBytes)} UTF-8 bytes.`,
    );
    return false;
  }
  return true;
}

function validateMacro(
  value: unknown,
  path: string,
  validateSource: MacroSourceValidator,
  macroIds: Set<string>,
  issues: RepositoryValidationIssue[],
): RepositoryMacroV1 | null {
  if (!isPlainRecord(value)) {
    addIssue(issues, path, "invalid_object", "Macro must be a plain object.");
    return null;
  }
  if (!hasExactKeys(value, macroKeys)) {
    addIssue(
      issues,
      path,
      "invalid_fields",
      "Macro contains missing or unknown fields.",
    );
    return null;
  }

  let valid = true;
  const idPath = `${path}.id`;
  if (!isCanonicalUuidV4(value.id)) {
    addIssue(
      issues,
      idPath,
      "invalid_uuid",
      "Macro ID must be a canonical lowercase UUID v4.",
    );
    valid = false;
  } else if (macroIds.has(value.id)) {
    addIssue(
      issues,
      idPath,
      "duplicate_id",
      "Macro ID must be unique across the repository.",
    );
    valid = false;
  } else {
    macroIds.add(value.id);
  }

  if (!validateName(value.name, `${path}.name`, issues)) {
    valid = false;
  }

  if (typeof value.source !== "string") {
    addIssue(
      issues,
      `${path}.source`,
      "invalid_type",
      "Macro source must be a string.",
    );
    valid = false;
  } else {
    if (utf8ByteLength(value.source) > sourceMaximumBytes) {
      addIssue(
        issues,
        `${path}.source`,
        "too_long",
        `Macro source exceeds ${String(sourceMaximumBytes)} UTF-8 bytes.`,
      );
      valid = false;
    } else {
      const sourceError = validateSource(value.source);
      if (sourceError !== null) {
        addIssue(issues, `${path}.source`, "invalid_macro_source", sourceError);
        valid = false;
      }
    }
  }

  if (!isBoundedInteger(value.keyPressMs, 0, timingMaximumMs)) {
    addIssue(
      issues,
      `${path}.keyPressMs`,
      "invalid_timing",
      `Key press time must be an integer from 0 to ${String(timingMaximumMs)} ms.`,
    );
    valid = false;
  }
  if (!isBoundedInteger(value.interKeyMs, 0, timingMaximumMs)) {
    addIssue(
      issues,
      `${path}.interKeyMs`,
      "invalid_timing",
      `Inter-key time must be an integer from 0 to ${String(timingMaximumMs)} ms.`,
    );
    valid = false;
  }

  if (
    !valid ||
    !isCanonicalUuidV4(value.id) ||
    typeof value.name !== "string" ||
    typeof value.source !== "string" ||
    typeof value.keyPressMs !== "number" ||
    typeof value.interKeyMs !== "number"
  ) {
    return null;
  }

  return {
    id: value.id,
    name: value.name,
    source: value.source,
    keyPressMs: value.keyPressMs,
    interKeyMs: value.interKeyMs,
  };
}

function validatePackage(
  value: unknown,
  index: number,
  validateSource: MacroSourceValidator,
  packageIds: Set<string>,
  macroIds: Set<string>,
  issues: RepositoryValidationIssue[],
): RepositoryPackageV1 | null {
  const path = `$.packages[${String(index)}]`;
  if (!isPlainRecord(value)) {
    addIssue(issues, path, "invalid_object", "Package must be a plain object.");
    return null;
  }
  if (!hasExactKeys(value, packageKeys)) {
    addIssue(
      issues,
      path,
      "invalid_fields",
      "Package contains missing or unknown fields.",
    );
    return null;
  }

  let valid = true;
  if (!isCanonicalUuidV4(value.id)) {
    addIssue(
      issues,
      `${path}.id`,
      "invalid_uuid",
      "Package ID must be a canonical lowercase UUID v4.",
    );
    valid = false;
  } else if (packageIds.has(value.id)) {
    addIssue(
      issues,
      `${path}.id`,
      "duplicate_id",
      "Package ID must be unique within the repository.",
    );
    valid = false;
  } else {
    packageIds.add(value.id);
  }

  if (!validateName(value.name, `${path}.name`, issues)) {
    valid = false;
  }

  const macros: RepositoryMacroV1[] = [];
  if (!Array.isArray(value.macros) || !isDenseArray(value.macros)) {
    addIssue(
      issues,
      `${path}.macros`,
      "invalid_array",
      "Macros must be a dense array.",
    );
    valid = false;
  } else {
    value.macros.forEach((macro, macroIndex) => {
      const validated = validateMacro(
        macro,
        `${path}.macros[${String(macroIndex)}]`,
        validateSource,
        macroIds,
        issues,
      );
      if (validated === null) {
        valid = false;
      } else {
        macros.push(validated);
      }
    });
  }

  if (
    !valid ||
    !isCanonicalUuidV4(value.id) ||
    typeof value.name !== "string"
  ) {
    return null;
  }

  return { id: value.id, name: value.name, macros };
}

export function validateRepositoryV1(
  value: unknown,
  validateSource: MacroSourceValidator,
): RepositoryValidationResult {
  const issues: RepositoryValidationIssue[] = [];
  if (!isPlainRecord(value)) {
    addIssue(
      issues,
      "$",
      "invalid_object",
      "Repository must be a plain object.",
    );
    return { ok: false, issues };
  }
  if (!hasExactKeys(value, repositoryKeys)) {
    addIssue(
      issues,
      "$",
      "invalid_fields",
      "Repository contains missing or unknown fields.",
    );
    return { ok: false, issues };
  }
  if (value.format !== "esp32-macro-keyboard-repository") {
    addIssue(
      issues,
      "$.format",
      "invalid_format",
      "Repository format is not supported.",
    );
  }
  if (value.schemaVersion !== 1) {
    addIssue(
      issues,
      "$.schemaVersion",
      "unsupported_schema",
      "Repository schema version is not supported.",
    );
  }

  const packages: RepositoryPackageV1[] = [];
  if (!Array.isArray(value.packages) || !isDenseArray(value.packages)) {
    addIssue(
      issues,
      "$.packages",
      "invalid_array",
      "Packages must be a dense array.",
    );
  } else {
    const packageIds = new Set<string>();
    const macroIds = new Set<string>();
    value.packages.forEach((item, index) => {
      const validated = validatePackage(
        item,
        index,
        validateSource,
        packageIds,
        macroIds,
        issues,
      );
      if (validated !== null) {
        packages.push(validated);
      }
    });
  }

  if (
    issues.length > 0 ||
    value.format !== "esp32-macro-keyboard-repository" ||
    value.schemaVersion !== 1
  ) {
    return { ok: false, issues };
  }

  return {
    ok: true,
    value: {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages,
    },
  };
}

export function serializeRepositoryV1(repository: RepositoryV1): string {
  return JSON.stringify({
    format: repository.format,
    schemaVersion: repository.schemaVersion,
    packages: repository.packages.map((pkg) => ({
      id: pkg.id,
      name: pkg.name,
      macros: pkg.macros.map((macro) => ({
        id: macro.id,
        name: macro.name,
        source: macro.source,
        keyPressMs: macro.keyPressMs,
        interKeyMs: macro.interKeyMs,
      })),
    })),
  });
}
