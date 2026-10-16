/**
 * Reproducible screenshot set for the Material 3 Expressive redesign
 * (UI_UX_REDESIGN_TODO_2026-10-15.md UXR-000 / UXR-072).
 *
 * Reuses the visual-regression scenarios (scenarios.mjs) but overrides their
 * viewports, so the same app states can be photographed at the redesign's
 * review sizes. PNGs only -- nothing here is a regression baseline.
 *
 * Usage:
 *   node tests/browser/visual/screenshot-set.mjs --out <dir>
 *        [--scenarios a,b,c] [--viewports 320x568,1440x900]
 *
 * Output: <dir>/<scenario>--<width>x<height>.png
 */
import { mkdir, writeFile } from "node:fs/promises";
import { join } from "node:path";
import process from "node:process";
import { chromium } from "playwright";

import { SCENARIOS } from "./scenarios.mjs";

/** The review sizes named by UXR-000 and UXR-072. */
export const REVIEW_VIEWPORTS = [
  "320x568",
  "360x800",
  "390x844",
  "768x1024",
  "1024x768",
  "1440x900",
];

/** One scenario per screen/state the TODO asks to have captured. */
export const REVIEW_SCENARIOS = [
  "first-run-setup",
  "first-run-review",
  "signin-error",
  "startup-no-blobs",
  "startup-package-chooser",
  "startup-settings-retry",
  "startup-empty-package",
  "startup-snapshot-recovery",
  "macros",
  "macros-overflow-menu",
  "macros-dirty",
  "macro-editor",
  "packages",
  "snapshots",
  "snapshots-advanced",
  "settings",
  "diagnostics",
  "send-in-flight",
  "send-failure",
  "execution-recovery-overlay",
  "dialog-restart",
  "unsaved-changes-prompt",
];

function parseArgs(argv) {
  const args = {
    out: null,
    scenarios: REVIEW_SCENARIOS,
    viewports: REVIEW_VIEWPORTS,
  };
  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    if (arg === "--out") {
      i += 1;
      args.out = argv[i];
    } else if (arg === "--scenarios") {
      i += 1;
      args.scenarios = argv[i].split(",");
    } else if (arg === "--viewports") {
      i += 1;
      args.viewports = argv[i].split(",");
    } else {
      throw new Error(`Unrecognized argument: ${arg}`);
    }
  }
  if (args.out === null) {
    throw new Error("--out <dir> is required.");
  }
  return args;
}

function parseViewport(text) {
  const match = /^(\d+)x(\d+)$/.exec(text);
  if (match === null) {
    throw new Error(`Bad viewport "${text}"; expected WIDTHxHEIGHT.`);
  }
  return { name: text, width: Number(match[1]), height: Number(match[2]) };
}

async function main() {
  const args = parseArgs(process.argv.slice(2));
  await mkdir(args.out, { recursive: true });
  const viewports = args.viewports.map(parseViewport);
  const byName = new Map(SCENARIOS.map((s) => [s.name, s]));
  for (const name of args.scenarios) {
    if (!byName.has(name)) {
      throw new Error(`Unknown scenario "${name}".`);
    }
  }

  const browser = await chromium.launch();
  let written = 0;
  try {
    for (const name of args.scenarios) {
      const scenario = byName.get(name);
      for (const viewport of viewports) {
        // A scenario pinned to one orientation (landscape overlay) still
        // runs; the harness only needs a viewport object.
        const capture = await scenario.run(browser, viewport);
        if (capture.png === null || capture.png === undefined) {
          throw new Error(`${name} @ ${viewport.name} produced no screenshot.`);
        }
        await writeFile(
          join(args.out, `${name}--${viewport.name}.png`),
          capture.png,
        );
        written += 1;
      }
    }
  } finally {
    await browser.close();
  }
  process.stdout.write(`Wrote ${String(written)} screenshots to ${args.out}\n`);
}

main().catch((error) => {
  process.stderr.write(
    `${error instanceof Error ? error.stack : String(error)}\n`,
  );
  process.exitCode = 1;
});
