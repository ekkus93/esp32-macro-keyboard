import type { MacroCompileError } from "./macroCompiler";

export type MacroErrorMessageClass =
  | "duration_limit"
  | "unknown_key"
  | "invalid_chord"
  | "invalid_delay"
  | "lone_carriage_return"
  | "unmatched_opening_brace"
  | "unmatched_closing_brace"
  | "unsupported_character"
  | "invalid_directive"
  | "action_limit"
  | "source_limit"
  | "invalid_timing";

export function macroErrorMessageClass(
  error: MacroCompileError,
): MacroErrorMessageClass {
  switch (error.message) {
    case "estimated duration limit exceeded":
      return "duration_limit";
    case "unknown key directive":
      return "unknown_key";
    case "invalid chord":
      return "invalid_chord";
    case "invalid delay directive":
    case "delay is outside the allowed range":
      return "invalid_delay";
    case "carriage return must be followed by line feed":
      return "lone_carriage_return";
    case "unmatched opening brace":
      return "unmatched_opening_brace";
    case "unmatched closing brace":
      return "unmatched_closing_brace";
    case "source contains unsupported character":
      return "unsupported_character";
    case "invalid directive":
      return "invalid_directive";
    case "action limit exceeded":
      return "action_limit";
    case "macro source exceeds the byte limit":
      return "source_limit";
    case "invalid macro timing":
      return "invalid_timing";
    default:
      throw new Error(`Unclassified macro compiler error: ${error.message}`);
  }
}
