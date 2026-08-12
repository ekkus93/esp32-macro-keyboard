/// <reference types="vite/client" />
import ts from "typescript";
import { describe, expect, test } from "vitest";

function referencesBrowserConsole(path: string, source: string): boolean {
  const scriptKind = path.endsWith(".tsx") ? ts.ScriptKind.TSX : ts.ScriptKind.TS;
  const sourceFile = ts.createSourceFile(
    path,
    source,
    ts.ScriptTarget.Latest,
    true,
    scriptKind,
  );
  let found = false;

  function visit(node: ts.Node): void {
    if (found) return;
    if (ts.isIdentifier(node) && node.text === "console") {
      found = true;
      return;
    }
    if (
      ts.isElementAccessExpression(node) &&
      ts.isStringLiteralLike(node.argumentExpression) &&
      node.argumentExpression.text === "console"
    ) {
      found = true;
      return;
    }
    ts.forEachChild(node, visit);
  }

  visit(sourceFile);
  return found;
}

const v2SourceModules = import.meta.glob<string>(
  [
    "../src/AppV2.tsx",
    "../src/v2/**/*.{ts,tsx}",
    "../src/features/**/v2/**/*.{ts,tsx}",
  ],
  {
    eager: true,
    query: "?raw",
    import: "default",
  },
);

describe("v2 browser-console prohibition", () => {
  test("no production V2 source references the browser console", () => {
    const entries = Object.entries(v2SourceModules);
    expect(entries.length).toBeGreaterThan(0);
    const offenders = entries
      .filter(([path, source]) => referencesBrowserConsole(path, source))
      .map(([path]) => path);
    expect(offenders).toEqual([]);
  });

  test.each([
    "console.log(secret)",
    "window.console.error(secret)",
    'globalThis["console"]["warn"](secret)',
    "const logger = console; logger.info(secret)",
  ])(
    "console prohibition cannot be bypassed by alias form: %s",
    (source) => {
      expect(referencesBrowserConsole("fixture.ts", source)).toBe(true);
    },
  );

  test("user-visible text containing the word console is not a logging sink", () => {
    expect(
      referencesBrowserConsole("fixture.ts", 'const text = "Use the serial console.";'),
    ).toBe(false);
  });
});
