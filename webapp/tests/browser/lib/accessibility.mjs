import AxeBuilder from "@axe-core/playwright";

import { assert } from "./http.mjs";

export async function runAccessibilityScan(page, label) {
  const results = await new AxeBuilder({ page })
    .withTags(["wcag2a", "wcag2aa", "best-practice"])
    .analyze();
  const blocking = results.violations.filter(
    (violation) =>
      violation.impact === "serious" || violation.impact === "critical",
  );
  assert(
    blocking.length === 0,
    `axe-core found ${String(blocking.length)} serious/critical accessibility ` +
      `violation(s) on ${label}: ${JSON.stringify(
        blocking.map((violation) => ({
          id: violation.id,
          impact: violation.impact,
          help: violation.help,
          nodes: violation.nodes.map((node) => node.target),
        })),
        null,
        2,
      )}`,
  );
}
