import { v2Limits } from "./limits";

export type MacroAction =
  | { kind: "key"; usage: number; modifiers: number }
  | { kind: "chord"; usage: number; modifiers: number }
  | { kind: "delay"; durationMs: number };

export interface MacroCompileError {
  code: "invalid_argument" | "macro_syntax" | "macro_limit";
  byteOffset: number;
  line: number;
  column: number;
  message: string;
}

export type MacroCompileResult =
  | {
      ok: true;
      actions: MacroAction[];
      estimatedDurationMs: number;
    }
  | { ok: false; error: MacroCompileError };

type MacroCompileFailure = Extract<MacroCompileResult, { ok: false }>;

export interface MacroCompileOptions {
  keyPressMs: number;
  interKeyMs: number;
}

interface KeyStroke {
  usage: number;
  modifiers: number;
}

interface Position {
  index: number;
  byteOffset: number;
  line: number;
  column: number;
}

interface ParsedToken {
  action: MacroAction;
  next: Position;
}

interface CompileState {
  actions: MacroAction[];
  estimatedDurationMs: number;
  position: Position;
}

const encoder = new TextEncoder();
const shift = 0x02;
const startPosition: Position = {
  index: 0,
  byteOffset: 0,
  line: 1,
  column: 1,
};

const namedKeys: Readonly<Record<string, number>> = {
  ENTER: 0x28,
  TAB: 0x2b,
  ESC: 0x29,
  BACKSPACE: 0x2a,
  DELETE: 0x4c,
  INSERT: 0x49,
  HOME: 0x4a,
  END: 0x4d,
  PAGEUP: 0x4b,
  PAGEDOWN: 0x4e,
  UP: 0x52,
  DOWN: 0x51,
  LEFT: 0x50,
  RIGHT: 0x4f,
  SPACE: 0x2c,
  F1: 0x3a,
  F2: 0x3b,
  F3: 0x3c,
  F4: 0x3d,
  F5: 0x3e,
  F6: 0x3f,
  F7: 0x40,
  F8: 0x41,
  F9: 0x42,
  F10: 0x43,
  F11: 0x44,
  F12: 0x45,
};

const modifierBits: Readonly<Record<string, number>> = {
  CTRL: 0x01,
  SHIFT: 0x02,
  ALT: 0x04,
  GUI: 0x08,
};

const punctuation: Readonly<Record<string, KeyStroke>> = {
  " ": { usage: 0x2c, modifiers: 0 },
  "-": { usage: 0x2d, modifiers: 0 },
  _: { usage: 0x2d, modifiers: shift },
  "=": { usage: 0x2e, modifiers: 0 },
  "+": { usage: 0x2e, modifiers: shift },
  "[": { usage: 0x2f, modifiers: 0 },
  "{": { usage: 0x2f, modifiers: shift },
  "]": { usage: 0x30, modifiers: 0 },
  "}": { usage: 0x30, modifiers: shift },
  "\\": { usage: 0x31, modifiers: 0 },
  "|": { usage: 0x31, modifiers: shift },
  ";": { usage: 0x33, modifiers: 0 },
  ":": { usage: 0x33, modifiers: shift },
  "'": { usage: 0x34, modifiers: 0 },
  '"': { usage: 0x34, modifiers: shift },
  "`": { usage: 0x35, modifiers: 0 },
  "~": { usage: 0x35, modifiers: shift },
  ",": { usage: 0x36, modifiers: 0 },
  "<": { usage: 0x36, modifiers: shift },
  ".": { usage: 0x37, modifiers: 0 },
  ">": { usage: 0x37, modifiers: shift },
  "/": { usage: 0x38, modifiers: 0 },
  "?": { usage: 0x38, modifiers: shift },
  "!": { usage: 0x1e, modifiers: shift },
  "@": { usage: 0x1f, modifiers: shift },
  "#": { usage: 0x20, modifiers: shift },
  $: { usage: 0x21, modifiers: shift },
  "%": { usage: 0x22, modifiers: shift },
  "^": { usage: 0x23, modifiers: shift },
  "&": { usage: 0x24, modifiers: shift },
  "*": { usage: 0x25, modifiers: shift },
  "(": { usage: 0x26, modifiers: shift },
  ")": { usage: 0x27, modifiers: shift },
};

function byteLength(value: string): number {
  return encoder.encode(value).byteLength;
}

function validTiming(value: number): boolean {
  return (
    Number.isSafeInteger(value) &&
    Number.isFinite(value) &&
    value >= 0 &&
    value <= v2Limits.keyPressMaxMs
  );
}

function errorAt(
  position: Position,
  code: MacroCompileError["code"],
  message: string,
): MacroCompileFailure {
  return {
    ok: false,
    error: {
      code,
      byteOffset: position.byteOffset,
      line: position.line,
      column: position.column,
      message,
    },
  };
}

function printableKey(character: string): KeyStroke | null {
  const code = character.charCodeAt(0);
  if (code >= 0x61 && code <= 0x7a) {
    return { usage: 0x04 + code - 0x61, modifiers: 0 };
  }
  if (code >= 0x41 && code <= 0x5a) {
    return { usage: 0x04 + code - 0x41, modifiers: shift };
  }
  if (code >= 0x31 && code <= 0x39) {
    return { usage: 0x1e + code - 0x31, modifiers: 0 };
  }
  if (code === 0x30) {
    return { usage: 0x27, modifiers: 0 };
  }
  return punctuation[character] ?? null;
}

function chordKey(token: string): number | null {
  if (Object.hasOwn(namedKeys, token)) {
    return namedKeys[token] ?? null;
  }
  if (token.length !== 1) {
    return null;
  }
  const code = token.charCodeAt(0);
  if (code >= 0x41 && code <= 0x5a) {
    return 0x04 + code - 0x41;
  }
  if (code >= 0x31 && code <= 0x39) {
    return 0x1e + code - 0x31;
  }
  if (code === 0x30) {
    return 0x27;
  }
  return null;
}

function nextAscii(position: Position, consumed: string): Position {
  return {
    index: position.index + consumed.length,
    byteOffset: position.byteOffset + consumed.length,
    line: position.line,
    column: position.column + consumed.length,
  };
}

function nextLine(position: Position, consumedLength: number): Position {
  return {
    index: position.index + consumedLength,
    byteOffset: position.byteOffset + consumedLength,
    line: position.line + 1,
    column: 1,
  };
}

function parseDirective(
  directive: string,
  start: Position,
): MacroAction | MacroCompileFailure {
  if (directive.startsWith("DELAY:")) {
    const rawValue = directive.slice("DELAY:".length);
    if (!/^[0-9]+$/.test(rawValue)) {
      return errorAt(start, "macro_syntax", "invalid delay directive");
    }
    const durationMs = Number(rawValue);
    if (
      !Number.isSafeInteger(durationMs) ||
      durationMs < 1 ||
      durationMs > v2Limits.delayDirectiveMaxMs
    ) {
      return errorAt(
        start,
        "macro_limit",
        "delay is outside the allowed range",
      );
    }
    return { kind: "delay", durationMs };
  }

  if (Object.hasOwn(namedKeys, directive)) {
    const usage = namedKeys[directive];
    if (usage === undefined) {
      return errorAt(start, "macro_syntax", "unknown key directive");
    }
    return { kind: "key", usage, modifiers: 0 };
  }

  if (directive.includes("+")) {
    const tokens = directive.split("+");
    if (tokens.length < 2 || tokens.some((token) => token.length === 0)) {
      return errorAt(start, "macro_syntax", "invalid chord");
    }
    const ordinary = tokens[tokens.length - 1];
    if (ordinary === undefined || Object.hasOwn(modifierBits, ordinary)) {
      return errorAt(start, "macro_syntax", "invalid chord");
    }
    const usage = chordKey(ordinary);
    if (usage === null) {
      return errorAt(start, "macro_syntax", "invalid chord");
    }

    let modifiers = 0;
    const seen = new Set<string>();
    for (const token of tokens.slice(0, -1)) {
      const bit = modifierBits[token];
      if (bit === undefined || seen.has(token)) {
        return errorAt(start, "macro_syntax", "invalid chord");
      }
      seen.add(token);
      modifiers |= bit;
    }
    if (modifiers === 0) {
      return errorAt(start, "macro_syntax", "invalid chord");
    }
    return { kind: "chord", usage, modifiers };
  }

  return errorAt(start, "macro_syntax", "unknown key directive");
}

function isCompileFailure(
  value: MacroAction | MacroCompileFailure | ParsedToken,
): value is MacroCompileFailure {
  return "ok" in value;
}

function validDirectiveText(directive: string): boolean {
  if (
    directive.length === 0 ||
    directive.includes("{") ||
    /\s/.test(directive)
  ) {
    return false;
  }
  for (let index = 0; index < directive.length; index += 1) {
    const code = directive.charCodeAt(index);
    if (code < 0x21 || code > 0x7e) {
      return false;
    }
  }
  return true;
}

function parseCarriageReturn(
  source: string,
  position: Position,
): ParsedToken | MacroCompileFailure {
  if (source[position.index + 1] !== "\n") {
    return errorAt(
      position,
      "macro_syntax",
      "carriage return must be followed by line feed",
    );
  }
  return {
    action: { kind: "key", usage: 0x28, modifiers: 0 },
    next: nextLine(position, 2),
  };
}

function parseOpenBrace(
  source: string,
  position: Position,
): ParsedToken | MacroCompileFailure {
  if (source[position.index + 1] === "{") {
    return {
      action: { kind: "key", usage: 0x2f, modifiers: shift },
      next: nextAscii(position, "{{"),
    };
  }

  const closing = source.indexOf("}", position.index + 1);
  if (closing < 0) {
    return errorAt(position, "macro_syntax", "unmatched opening brace");
  }
  const directive = source.slice(position.index + 1, closing);
  if (!validDirectiveText(directive)) {
    return errorAt(position, "macro_syntax", "invalid directive");
  }

  const action = parseDirective(directive, position);
  if (isCompileFailure(action)) {
    return action;
  }
  return {
    action,
    next: nextAscii(position, source.slice(position.index, closing + 1)),
  };
}

function parseCloseBrace(
  source: string,
  position: Position,
): ParsedToken | MacroCompileFailure {
  if (source[position.index + 1] !== "}") {
    return errorAt(position, "macro_syntax", "unmatched closing brace");
  }
  return {
    action: { kind: "key", usage: 0x30, modifiers: shift },
    next: nextAscii(position, "}}"),
  };
}

function parsePrintable(
  character: string,
  position: Position,
): ParsedToken | MacroCompileFailure {
  const codePoint = character.codePointAt(0);
  const key = printableKey(character);
  if (
    codePoint === undefined ||
    codePoint < 0x20 ||
    codePoint > 0x7e ||
    key === null
  ) {
    return errorAt(
      position,
      "macro_syntax",
      "source contains unsupported character",
    );
  }
  return {
    action: { kind: "key", ...key },
    next: nextAscii(position, character),
  };
}

function parseNextToken(
  source: string,
  position: Position,
): ParsedToken | MacroCompileFailure {
  const character = source[position.index];
  if (character === undefined) {
    return errorAt(position, "macro_syntax", "invalid source position");
  }

  switch (character) {
    case "\r":
      return parseCarriageReturn(source, position);
    case "\n":
      return {
        action: { kind: "key", usage: 0x28, modifiers: 0 },
        next: nextLine(position, 1),
      };
    case "\t":
      return {
        action: { kind: "key", usage: 0x2b, modifiers: 0 },
        next: nextAscii(position, character),
      };
    case "{":
      return parseOpenBrace(source, position);
    case "}":
      return parseCloseBrace(source, position);
    default:
      return parsePrintable(character, position);
  }
}

function appendAction(
  state: CompileState,
  action: MacroAction,
  actionPosition: Position,
  options: MacroCompileOptions,
): MacroCompileFailure | null {
  if (state.actions.length >= v2Limits.compiledActionsMax) {
    return errorAt(actionPosition, "macro_limit", "action limit exceeded");
  }
  const addedDuration =
    action.kind === "delay"
      ? action.durationMs
      : options.keyPressMs + options.interKeyMs;
  if (
    state.estimatedDurationMs + addedDuration >
    v2Limits.estimatedDurationMaxMs
  ) {
    return errorAt(
      actionPosition,
      "macro_limit",
      "estimated duration limit exceeded",
    );
  }
  state.actions.push(action);
  state.estimatedDurationMs += addedDuration;
  return null;
}

function validateCompileInput(
  source: string,
  options: MacroCompileOptions,
): MacroCompileFailure | null {
  if (!validTiming(options.keyPressMs) || !validTiming(options.interKeyMs)) {
    return errorAt(startPosition, "invalid_argument", "invalid macro timing");
  }
  if (byteLength(source) > v2Limits.macroSourceMaxBytes) {
    return errorAt(
      startPosition,
      "macro_limit",
      "macro source exceeds the byte limit",
    );
  }
  return null;
}

export function compileMacro(
  source: string,
  options: MacroCompileOptions,
): MacroCompileResult {
  const inputFailure = validateCompileInput(source, options);
  if (inputFailure !== null) {
    return inputFailure;
  }

  const state: CompileState = {
    actions: [],
    estimatedDurationMs: 0,
    position: startPosition,
  };
  while (state.position.index < source.length) {
    const parsed = parseNextToken(source, state.position);
    if (isCompileFailure(parsed)) {
      return parsed;
    }
    const appendFailure = appendAction(
      state,
      parsed.action,
      state.position,
      options,
    );
    if (appendFailure !== null) {
      return appendFailure;
    }
    state.position = parsed.next;
  }

  return {
    ok: true,
    actions: state.actions,
    estimatedDurationMs: state.estimatedDurationMs,
  };
}
