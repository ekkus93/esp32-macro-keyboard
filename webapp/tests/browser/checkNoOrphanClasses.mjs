/**
 * WEBAPP_TAILWIND_TODO_2026-08-18.md T2-3: the direct guard against SPEC
 * §4.1's silent-failure mode -- a class token that generates zero CSS is
 * either a deliberate test hook (SPEC §7, exactly three: `app-shell`,
 * `landscape-block`, `storage-summary`) or a bug (an interpolated class name
 * Tailwind's scanner couldn't see, or a typo). This script tells the two
 * apart and fails loudly on anything in the second category.
 *
 * Needs the *compiled* stylesheet -- whether a token generates CSS is a
 * build-time fact, not something jsdom (T2-1/T2-2's environment) can answer,
 * jsdom has no CSS engine and applies no stylesheet at all. Runs after
 * `npm run build`, driven through a real browser via the same scenario set
 * the visual-regression harness uses (`visual/scenarios.mjs`) -- reusing it
 * here means this check exercises the same ~40 real pages/dialogs/states
 * that harness does, so it sees every class token those states actually
 * render, including the ones assigned through an intermediate `const`
 * (`Card`'s `CARD_CLASS`, `Dialog`'s `DIALOG_PANEL_CLASS`, ...) that a
 * source-text regex over `className="..."` JSX attributes would miss
 * entirely -- confirmed miss: a first draft of this check that scanned
 * `.tsx` source text found zero of those six components' own classes.
 * Only one viewport per scenario is used (the class *list* on an element
 * does not change with viewport width, only which `@media` rules currently
 * apply to it), which is why this is fast despite covering the whole
 * scenario set.
 */
import { readdir, readFile } from "node:fs/promises";
import process from "node:process";
import { chromium } from "playwright";

import { SCENARIOS } from "./visual/scenarios.mjs";

/** SPEC §7: the only class tokens allowed to generate zero CSS. */
const ALLOWED_ZERO_CSS_CLASSES = new Set([
  "app-shell",
  "landscape-block",
  "storage-summary",
]);

/**
 * Tailwind escapes any character a CSS identifier cannot contain literally
 * when it emits the selector for a class name -- the same escaping this
 * project's own migration review relied on throughout (e.g. `.min-\[34rem\]
 * \:items-start`). Mirrors that escaping so a token can be searched for in
 * the compiled CSS by exact selector, not by a loose substring match that
 * could false-positive on one token being a substring of another's name.
 */
function escapeForCssSelector(token) {
  // Hyphen and underscore are valid, unescaped in Tailwind's own compiled
  // selectors (verified against real output: `.gap-4`, `.mb-[0.3rem]`,
  // `1fr_auto` inside an arbitrary value) -- every other CSS-special
  // character it escapes with a backslash.
  return token.replace(
    /[:.[\]()/%,#!@*+<>=~'"&|{}^$?\s]/g,
    (char) => `\\${char}`,
  );
}

async function collectRenderedClassTokens(browser) {
  const tokens = new Set();
  for (const scenario of SCENARIOS) {
    const viewport = scenario.viewports[0];
    if (viewport === undefined) {
      throw new Error(`Scenario "${scenario.name}" has no viewports.`);
    }
    let capture;
    try {
      capture = await scenario.run(browser, viewport);
    } catch (error) {
      throw new Error(
        `Scenario "${scenario.name}" failed to reach its state: ` +
          `${error instanceof Error ? error.message : String(error)}`,
      );
    }
    for (const element of capture.elements) {
      for (const token of element.cls.split(/\s+/).filter(Boolean)) {
        tokens.add(token);
      }
    }
  }
  return tokens;
}

async function readCompiledCss() {
  const assetsDir = new URL("../../dist/assets/", import.meta.url);
  const files = await readdir(assetsDir);
  const cssFiles = files.filter((name) => name.endsWith(".css"));
  if (cssFiles.length === 0) {
    throw new Error(
      "No dist/assets/*.css found -- run `npm run build` before this script.",
    );
  }
  const contents = await Promise.all(
    cssFiles.map((name) => readFile(new URL(name, assetsDir), "utf8")),
  );
  return contents.join("\n");
}

/**
 * Plain substring search, not a regex match against the (already
 * backslash-escaped) selector: the escaped selector string itself contains
 * characters (`[`, `(`, `?`, ...) that are ALSO regex metacharacters, and
 * escaping it a second time for regex use produced a broken pattern during
 * development (an early version of this function used
 * `new RegExp(selector + lookahead)` and silently matched nothing). A
 * boundary check on the character immediately following each substring hit
 * gets the same "not a coincidental prefix of a longer class name" safety
 * (e.g. `.gap-4` must not match inside a hypothetical `.gap-40`) without
 * ever building a second regex out of untrusted-shape text.
 */
function classHasCssRule(css, token) {
  const selector = `.${escapeForCssSelector(token)}`;
  let searchFrom = 0;
  for (;;) {
    const index = css.indexOf(selector, searchFrom);
    if (index === -1) {
      return false;
    }
    const nextChar = css[index + selector.length];
    if (nextChar === undefined || !/[a-zA-Z0-9_-]/.test(nextChar)) {
      return true;
    }
    searchFrom = index + 1;
  }
}

async function main() {
  const browser = await chromium.launch({ headless: true });
  let tokens;
  try {
    tokens = await collectRenderedClassTokens(browser);
  } finally {
    await browser.close();
  }
  console.log(
    `Collected ${String(tokens.size)} distinct class tokens across ${String(SCENARIOS.length)} scenarios.`,
  );

  const css = await readCompiledCss();

  const unexpectedlyOrphaned = [];
  const unexpectedlyStyled = [];
  for (const token of tokens) {
    const hasCss = classHasCssRule(css, token);
    const isAllowedZeroCss = ALLOWED_ZERO_CSS_CLASSES.has(token);
    if (!hasCss && !isAllowedZeroCss) {
      unexpectedlyOrphaned.push(token);
    }
    if (hasCss && isAllowedZeroCss) {
      unexpectedlyStyled.push(token);
    }
  }

  // Every allowlisted hook must actually have been seen -- if a scenario
  // set changes and stops rendering one, the allowlist is stale and should
  // shrink, not sit there unverified.
  const unseenAllowlisted = [...ALLOWED_ZERO_CSS_CLASSES].filter(
    (token) => !tokens.has(token),
  );

  let failed = false;
  if (unexpectedlyOrphaned.length > 0) {
    failed = true;
    console.log(
      `\n${String(unexpectedlyOrphaned.length)} class token(s) generate NO CSS and are not SPEC §7 hooks:`,
    );
    for (const token of unexpectedlyOrphaned.sort()) {
      console.log(`  ${token}`);
    }
    console.log(
      "\nThis is SPEC §4.1's failure mode: an interpolated or misspelled " +
        "class name that Tailwind's scanner cannot see, rendering unstyled " +
        "with no build error. If this is a deliberate new test hook, add it " +
        "to ALLOWED_ZERO_CSS_CLASSES here and to SPEC §7's table.",
    );
  }
  if (unexpectedlyStyled.length > 0) {
    failed = true;
    console.log(
      `\n${String(unexpectedlyStyled.length)} SPEC §7 hook(s) unexpectedly DO generate CSS:`,
    );
    for (const token of unexpectedlyStyled.sort()) {
      console.log(`  ${token}`);
    }
    console.log(
      "\nA hook is supposed to be bare -- carry no rule of its own. If this " +
        "is deliberate, it is no longer a bare hook and should be removed " +
        "from ALLOWED_ZERO_CSS_CLASSES and from SPEC §7's table.",
    );
  }
  if (unseenAllowlisted.length > 0) {
    failed = true;
    console.log(
      `\n${String(unseenAllowlisted.length)} allowlisted hook(s) were never rendered by any scenario:`,
    );
    for (const token of unseenAllowlisted.sort()) {
      console.log(`  ${token}`);
    }
    console.log(
      "\nAn unverified allowlist entry is not evidence of anything -- " +
        "either a scenario should render it, or it should be removed.",
    );
  }

  if (failed) {
    process.exitCode = 1;
    return;
  }
  console.log(
    `\nEvery zero-CSS token is an allowlisted SPEC §7 hook, and every hook was verified zero-CSS.`,
  );
}

await main();
