import { ApiError } from "../../api/client";
import { isRecord } from "../../api/guards";
import { limits } from "../../types/limits";
import type {
  Macro,
  MacroParseLocation,
  MacroValidation,
} from "../../types/models";

const encoder = new TextEncoder();

export const macroDirectives = [
  { label: "Insert Enter", value: "{ENTER}" },
  { label: "Insert Tab", value: "{TAB}" },
  { label: "Insert Escape", value: "{ESC}" },
  { label: "Insert Ctrl+L", value: "{CTRL+L}" },
  { label: "Insert Ctrl+Shift+T", value: "{CTRL+SHIFT+T}" },
  { label: "Insert Alt+F4", value: "{ALT+F4}" },
  { label: "Insert 500 ms delay", value: "{DELAY:500}" },
  { label: "Insert literal {", value: "{{" },
  { label: "Insert literal }", value: "}}" },
] as const;

export interface MacroLocalValidation {
  valid: boolean;
  nameBytes: number;
  sourceBytes: number;
  issues: string[];
}

export function utf8ByteLength(value: string): number {
  return encoder.encode(value).byteLength;
}

export function createMacroDraft(setId: string, macroId: string): Macro {
  return {
    schema_version: 1,
    id: macroId,
    revision: 1,
    set_id: setId,
    name: "",
    source: "",
    key_press_ms: 8,
    inter_key_ms: 15,
  };
}

export function macroFingerprint(macro: Macro): string {
  return JSON.stringify([
    macro.id,
    macro.set_id,
    macro.name,
    macro.source,
    macro.key_press_ms,
    macro.inter_key_ms,
  ]);
}

function timingValid(value: number): boolean {
  return (
    Number.isSafeInteger(value) && value >= 0 && value <= limits.durationMs
  );
}

export function validateMacroLocally(macro: Macro): MacroLocalValidation {
  const nameBytes = utf8ByteLength(macro.name);
  const sourceBytes = utf8ByteLength(macro.source);
  const issues: string[] = [];

  if (macro.name.trim().length === 0) {
    issues.push("Name is required.");
  }
  if (nameBytes > limits.macroNameBytes) {
    issues.push(`Name exceeds ${String(limits.macroNameBytes)} UTF-8 bytes.`);
  }
  if (sourceBytes > limits.macroSourceBytes) {
    issues.push(
      `Source exceeds ${String(limits.macroSourceBytes)} UTF-8 bytes.`,
    );
  }
  if (!timingValid(macro.key_press_ms)) {
    issues.push(
      `Key press time must be an integer from 0 to ${String(limits.durationMs)} ms.`,
    );
  }
  if (!timingValid(macro.inter_key_ms)) {
    issues.push(
      `Inter-key time must be an integer from 0 to ${String(limits.durationMs)} ms.`,
    );
  }

  return {
    valid: issues.length === 0,
    nameBytes,
    sourceBytes,
    issues,
  };
}

function isNonNegativeInteger(value: unknown): value is number {
  return typeof value === "number" && Number.isSafeInteger(value) && value >= 0;
}

export function parseMacroLocation(error: unknown): MacroParseLocation | null {
  if (!(error instanceof ApiError) || error.status !== 422) {
    return null;
  }
  const details = error.body.details;
  if (
    !isRecord(details) ||
    Object.keys(details).length !== 3 ||
    !isNonNegativeInteger(details.line) ||
    !isNonNegativeInteger(details.column) ||
    !isNonNegativeInteger(details.byteOffset)
  ) {
    return null;
  }
  return {
    line: details.line,
    column: details.column,
    byteOffset: details.byteOffset,
  };
}

export function validationSummary(validation: MacroValidation): string {
  return `${String(validation.actionCount)} actions · ${String(
    validation.estimatedDurationMs,
  )} ms estimated`;
}

export function byteOffsetToCodeUnit(
  source: string,
  byteOffset: number,
): number {
  if (byteOffset <= 0) {
    return 0;
  }
  let bytes = 0;
  let codeUnit = 0;
  for (const character of source) {
    const nextBytes = bytes + utf8ByteLength(character);
    if (nextBytes > byteOffset) {
      break;
    }
    bytes = nextBytes;
    codeUnit += character.length;
  }
  return codeUnit;
}
