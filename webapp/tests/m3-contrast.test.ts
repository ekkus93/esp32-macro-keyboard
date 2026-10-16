/// <reference types="node" />
import { readFileSync } from "node:fs";
import { resolve } from "node:path";
import { describe, expect, test } from "vitest";

/**
 * WCAG 2.2 AA contrast for the Material 3 colour roles
 * (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §3.3, §8; TODO UXR-012).
 *
 * The tokens are read straight from src/styles.css -- the one place they are
 * defined -- so this test cannot drift from what ships. Light values come from
 * `@theme`; dark values come from the `prefers-color-scheme: dark` override
 * and are layered over the light set (a role the dark block does not redefine
 * keeps its light value, exactly as in the browser).
 */

// Read from disk rather than imported: Vitest replaces imported CSS (even
// `?raw`) with an empty string, and this test needs the real token text.
// Vitest runs with the webapp directory as its working directory.
const css = readFileSync(resolve(process.cwd(), "src/styles.css"), "utf8");

const DARK_MARKER = "@media (prefers-color-scheme: dark)";

function colorsIn(block: string): Map<string, string> {
  const result = new Map<string, string>();
  for (const match of block.matchAll(
    /--color-([a-z-]+):\s*(#[0-9a-fA-F]{3,6})\s*;/g,
  )) {
    const name = match[1];
    const value = match[2];
    if (name !== undefined && value !== undefined) {
      result.set(name, value);
    }
  }
  return result;
}

function parseHex(hex: string): [number, number, number] {
  const digits =
    hex.length === 4
      ? hex
          .slice(1)
          .split("")
          .map((digit) => digit + digit)
          .join("")
      : hex.slice(1);
  return [
    parseInt(digits.slice(0, 2), 16),
    parseInt(digits.slice(2, 4), 16),
    parseInt(digits.slice(4, 6), 16),
  ];
}

function luminance(hex: string): number {
  const [r, g, b] = parseHex(hex).map((channel) => {
    const value = channel / 255;
    return value <= 0.03928 ? value / 12.92 : ((value + 0.055) / 1.055) ** 2.4;
  }) as [number, number, number];
  return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

function contrast(foreground: string, background: string): number {
  const a = luminance(foreground);
  const b = luminance(background);
  return (Math.max(a, b) + 0.05) / (Math.min(a, b) + 0.05);
}

const darkStart = css.indexOf(DARK_MARKER);
if (darkStart < 0) {
  throw new Error("styles.css has no prefers-color-scheme: dark block.");
}
const light = colorsIn(css.slice(0, darkStart));
const darkOverrides = colorsIn(css.slice(darkStart));
const dark = new Map([...light, ...darkOverrides]);

const SCHEMES = [
  ["light", light],
  ["dark", dark],
] as const;

const SURFACES = [
  "surface",
  "surface-container-lowest",
  "surface-container-low",
  "surface-container",
  "surface-container-high",
  "surface-container-highest",
];

/** [foreground role, background role] -- text, AA 4.5:1. */
const TEXT_PAIRS: [string, string][] = [
  ["on-primary", "primary"],
  ["on-primary-container", "primary-container"],
  ["on-secondary", "secondary"],
  ["on-secondary-container", "secondary-container"],
  ["on-tertiary", "tertiary"],
  ["on-tertiary-container", "tertiary-container"],
  ["on-error", "error"],
  ["on-error-container", "error-container"],
  ["on-success-container", "success-container"],
  ["on-warning-container", "warning-container"],
  ["inverse-on-surface", "inverse-surface"],
  ...SURFACES.flatMap((surface): [string, string][] => [
    ["on-surface", surface],
    ["on-surface-variant", surface],
  ]),
  // Colour used directly as text on the page and card surfaces.
  ["primary", "surface"],
  ["primary", "surface-container"],
  ["error", "surface"],
  ["error", "surface-container"],
  ["success", "surface"],
  ["success", "surface-container"],
];

/** Non-text boundaries and focus indicators -- AA 3:1. */
const NON_TEXT_PAIRS: [string, string][] = [
  ["outline", "surface"],
  ["outline", "surface-container-low"],
  ["outline", "surface-container"],
  ["primary", "surface"],
  ["primary", "surface-container"],
];

describe.each(SCHEMES)("M3 colour roles (%s scheme)", (_scheme, colors) => {
  const lookup = (role: string): string => {
    const value = colors.get(role);
    if (value === undefined) {
      throw new Error(`No --color-${role} defined.`);
    }
    return value;
  };

  test.each(TEXT_PAIRS)("text %s on %s is at least 4.5:1", (fg, bg) => {
    expect(contrast(lookup(fg), lookup(bg))).toBeGreaterThanOrEqual(4.5);
  });

  test.each(NON_TEXT_PAIRS)(
    "boundary or focus colour %s on %s is at least 3:1",
    (fg, bg) => {
      expect(contrast(lookup(fg), lookup(bg))).toBeGreaterThanOrEqual(3);
    },
  );
});

describe("M3 colour token set", () => {
  test("the dark scheme overrides only roles the light scheme defines", () => {
    for (const role of darkOverrides.keys()) {
      expect(light.has(role)).toBe(true);
    }
  });

  test("every role used by a text pair is defined in both schemes", () => {
    const roles = new Set(TEXT_PAIRS.flat());
    for (const role of roles) {
      expect(light.has(role)).toBe(true);
      expect(dark.has(role)).toBe(true);
    }
  });
});
