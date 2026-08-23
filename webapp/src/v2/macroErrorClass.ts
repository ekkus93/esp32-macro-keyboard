import type { MacroCompileError } from "./macroCompiler";

export type MacroErrorMessageClass =
  | "duration_limit"
  | "unknown_key"
  | "invalid_delay"
  | "lone_carriage_return"
  | "unmatched_opening_brace"
  | "unmatched_closing_brace"
  | "unmatched_opening_bracket"
  | "unmatched_closing_bracket"
  | "group_nesting"
  | "group_duplicate_modifier"
  | "group_duplicate_key"
  | "group_limit_exceeded"
  | "group_empty"
  | "group_delay_not_permitted"
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
    case "invalid delay directive":
    case "delay is outside the allowed range":
      return "invalid_delay";
    case "carriage return must be followed by line feed":
      return "lone_carriage_return";
    case "unmatched opening brace":
      return "unmatched_opening_brace";
    case "unmatched closing brace":
      return "unmatched_closing_brace";
    case "unmatched opening bracket":
      return "unmatched_opening_bracket";
    case "unmatched closing bracket":
      return "unmatched_closing_bracket";
    case "simultaneous-key groups do not nest":
      return "group_nesting";
    case "duplicate modifier in a simultaneous-key group":
      return "group_duplicate_modifier";
    case "duplicate key in a simultaneous-key group":
      return "group_duplicate_key";
    case "simultaneous-key group exceeds the 6-key limit":
      return "group_limit_exceeded";
    case "empty simultaneous-key group":
      return "group_empty";
    case "a delay is not permitted inside a simultaneous-key group":
      return "group_delay_not_permitted";
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
