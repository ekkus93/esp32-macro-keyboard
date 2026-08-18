/**
 * The visual-regression driver (WEBAPP_TAILWIND_TODO_2026-08-18.md T1-1).
 * Runs every scenario in scenarios.mjs at each of its viewports, captures
 * computed style + geometry + `::before` state for every element
 * (capture.mjs), and compares against the checked-in baseline
 * (compare.mjs).
 *
 * Usage:
 *   node tests/browser/visual/run-visual-tests.mjs
 *     Compare the current build against baselines/. Exits nonzero on any
 *     content difference OR if any scenario cannot reach its target state
 *     ("fail loudly" -- T1-1 -- rather than silently skipping it) OR if a
 *     scenario has no baseline at all.
 *
 *   node tests/browser/visual/run-visual-tests.mjs --update-baselines
 *     Regenerate baselines/ from the current build. Always exits 0 (short of
 *     a scenario error). Run this deliberately, review the resulting diff,
 *     and commit it as its own change -- see baselines/README.md.
 *
 *   node tests/browser/visual/run-visual-tests.mjs --baseline-dir <path>
 *     Compare against an arbitrary baseline directory instead of the
 *     checked-in one -- this is the tree-vs-tree mode: point it at a JSON
 *     dump produced by an earlier run of this same script against a
 *     different git worktree, to diff two commits directly rather than each
 *     against the committed baseline.
 *
 *   node tests/browser/visual/run-visual-tests.mjs --grep <substring>
 *     Only run scenarios whose name contains the substring.
 *
 *   node tests/browser/visual/run-visual-tests.mjs --diff-dir <path>
 *     Where to write current-state JSON + PNGs for scenarios that differ
 *     (default: a temp directory, printed on failure). Not committed.
 */
import { mkdir, mkdtemp, readFile, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { dirname, join } from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";
import { chromium } from "playwright";

import { compareCaptures } from "./compare.mjs";
import { SCENARIOS } from "./scenarios.mjs";

const __dirname = dirname(fileURLToPath(import.meta.url));
const DEFAULT_BASELINE_DIR = join(__dirname, "baselines");
/** How many scenario/viewport runs execute concurrently. */
const CONCURRENCY = 4;

function parseArgs(argv) {
  const args = {
    updateBaselines: false,
    baselineDir: DEFAULT_BASELINE_DIR,
    grep: null,
    diffDir: null,
  };
  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    if (arg === "--update-baselines") {
      args.updateBaselines = true;
    } else if (arg === "--baseline-dir") {
      i += 1;
      args.baselineDir = argv[i];
    } else if (arg === "--grep") {
      i += 1;
      args.grep = argv[i];
    } else if (arg === "--diff-dir") {
      i += 1;
      args.diffDir = argv[i];
    } else {
      throw new Error(`Unrecognized argument: ${arg}`);
    }
  }
  return args;
}

function baselineFileName(scenarioName, viewportName) {
  return `${scenarioName}--${viewportName}.json`;
}

/** Strips the PNG buffer -- baselines are JSON-only; see baselines/README.md. */
function toBaselineJson(capture) {
  return JSON.stringify(
    { elements: capture.elements, pseudoElements: capture.pseudoElements },
    null,
    1,
  );
}

async function runPool(items, worker, concurrency) {
  const results = new Array(items.length);
  let next = 0;
  async function runNext() {
    while (next < items.length) {
      const index = next;
      next += 1;
      results[index] = await worker(items[index], index);
    }
  }
  await Promise.all(
    Array.from({ length: Math.min(concurrency, items.length) }, runNext),
  );
  return results;
}

async function main() {
  const args = parseArgs(process.argv.slice(2));

  const jobs = [];
  for (const scenario of SCENARIOS) {
    if (args.grep !== null && !scenario.name.includes(args.grep)) {
      continue;
    }
    for (const viewport of scenario.viewports) {
      jobs.push({ scenario, viewport });
    }
  }
  if (jobs.length === 0) {
    throw new Error(
      args.grep === null
        ? "No scenarios registered."
        : `No scenario name contains "${args.grep}".`,
    );
  }

  console.log(
    `Running ${String(jobs.length)} scenario/viewport captures (concurrency ${String(CONCURRENCY)})...`,
  );

  const browser = await chromium.launch({ headless: true });
  const failures = [];
  const diffs = [];
  let updated = 0;

  try {
    await runPool(
      jobs,
      async ({ scenario, viewport }) => {
        const label = `${scenario.name} @ ${viewport.name}`;
        let capture;
        try {
          capture = await scenario.run(browser, viewport);
        } catch (error) {
          // Fail loudly: a scenario that could not reach its target state is
          // a failure of the whole run, not a silently-omitted data point.
          failures.push(
            `${label}: scenario setup failed -- ${error instanceof Error ? error.message : String(error)}`,
          );
          return;
        }

        const baselinePath = join(
          args.baselineDir,
          baselineFileName(scenario.name, viewport.name),
        );

        if (args.updateBaselines) {
          await mkdir(args.baselineDir, { recursive: true });
          await writeFile(baselinePath, toBaselineJson(capture));
          updated += 1;
          console.log(`  wrote  ${label}`);
          return;
        }

        let baselineRaw;
        try {
          baselineRaw = await readFile(baselinePath, "utf8");
        } catch (error) {
          if (
            error instanceof Error &&
            "code" in error &&
            error.code === "ENOENT"
          ) {
            failures.push(
              `${label}: no baseline at ${baselinePath} -- run with --update-baselines to create it`,
            );
            return;
          }
          throw error;
        }
        const baseline = JSON.parse(baselineRaw);
        const scenarioDiffs = compareCaptures(baseline, capture, label);
        if (scenarioDiffs.length > 0) {
          diffs.push({ label, scenarioDiffs, capture });
          console.log(
            `  DIFF   ${label} (${String(scenarioDiffs.length)} differences)`,
          );
        } else {
          console.log(`  ok     ${label}`);
        }
      },
      CONCURRENCY,
    );
  } finally {
    await browser.close();
  }

  if (args.updateBaselines) {
    console.log(
      `\nWrote ${String(updated)} baseline files to ${args.baselineDir}.`,
    );
    if (failures.length > 0) {
      console.log(
        "\nScenarios that failed to reach their state (NOT updated):",
      );
      for (const failure of failures) {
        console.log(`  ${failure}`);
      }
      process.exitCode = 1;
    }
    return;
  }

  if (diffs.length > 0) {
    const diffDir =
      args.diffDir ?? (await mkdtemp(join(tmpdir(), "visual-diff-")));
    await mkdir(diffDir, { recursive: true });
    for (const { label, scenarioDiffs, capture } of diffs) {
      const safeName = label.replace(/[^\w-]+/g, "_");
      await writeFile(
        join(diffDir, `${safeName}.json`),
        JSON.stringify({ diffs: scenarioDiffs, capture }, null, 1),
      );
      if (capture.png !== null) {
        await writeFile(join(diffDir, `${safeName}.png`), capture.png);
      }
    }
    console.log(
      `\n${String(diffs.length)} scenario(s) differ from baseline:\n`,
    );
    for (const { label, scenarioDiffs } of diffs) {
      console.log(`${label}:`);
      for (const line of scenarioDiffs.slice(0, 20)) {
        console.log(`  ${line}`);
      }
      if (scenarioDiffs.length > 20) {
        console.log(`  ... and ${String(scenarioDiffs.length - 20)} more`);
      }
    }
    console.log(`\nCurrent-state captures written to: ${diffDir}`);
    console.log(
      "If this is a deliberate visual change, review it, then run with " +
        "--update-baselines and commit the result as its own change.",
    );
  }

  if (failures.length > 0) {
    console.log(`\n${String(failures.length)} scenario(s) failed to run:\n`);
    for (const failure of failures) {
      console.log(`  ${failure}`);
    }
  }

  if (diffs.length > 0 || failures.length > 0) {
    process.exitCode = 1;
  } else {
    console.log(
      `\nAll ${String(jobs.length)} scenario/viewport captures match baseline.`,
    );
  }
}

await main();
