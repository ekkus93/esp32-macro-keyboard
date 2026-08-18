import { describe, expect, test } from "vitest";

import { Card } from "../src/components/Card";
import { SendStatus } from "../src/components/SendStatus";
import {
  describeViolation,
  findDescendantVariantNestingViolations,
} from "./descendantVariantNesting";
import { REPRESENTATIVE_PAGES } from "./representativePages";
import { render } from "./render";

/**
 * WEBAPP_TAILWIND_TODO_2026-08-18.md T3-2: `Card`'s `[&_p]:` cannot be moved
 * to its call sites the way `PageHeading`'s `[&_h2]:` was (T3-1) -- it
 * covers far too many paragraphs across the app for that to be a safe or
 * readable change. This is the guard `SPEC` §6.1 / T3-1's own instructions
 * name as the alternative: fail if the nesting `SPEC` §6.1 warns about ever
 * actually happens, across `Card`, `Dialog` and `SendStatus` (every current
 * `[&_p]:` owner) and, for defence in depth, every other descendant-variant
 * scope in the app (`[&_h2]:`, `[&_h3]:`, `[&_button]:`) at once.
 */

describe("no descendant-variant scope nests inside another owner of the same scope", () => {
  for (const page of REPRESENTATIVE_PAGES) {
    test(page.name, async () => {
      const view = await page.render();
      const violations = findDescendantVariantNestingViolations(view.container);
      expect(violations.map(describeViolation)).toEqual([]);
      await view.unmount();
    });
  }

  test("the detector has teeth: a Card nested inside a SendStatus is caught", async () => {
    // Both own `[&_p]:` at (0,1,1) -- exactly SPEC §6.1's hazard, deliberately
    // constructed rather than found, since no real call site nests them today.
    const view = await render(
      <SendStatus role="status">
        <Card>
          <p>doubly-scoped paragraph</p>
        </Card>
      </SendStatus>,
    );
    const violations = findDescendantVariantNestingViolations(view.container);
    expect(violations).toHaveLength(1);
    expect(violations[0]?.tag).toBe("p");
    expect(violations[0]?.scopingAncestors).toHaveLength(2);
    await view.unmount();
  });

  test("a Card next to a SendStatus (not nested) is not a violation", async () => {
    // The sibling case that must NOT be flagged: each scopes its own
    // paragraph independently, and neither element is a descendant of both.
    const view = await render(
      <div>
        <Card>
          <p>card paragraph</p>
        </Card>
        <SendStatus role="status">
          <p>send-status paragraph</p>
        </SendStatus>
      </div>,
    );
    const violations = findDescendantVariantNestingViolations(view.container);
    expect(violations).toEqual([]);
    await view.unmount();
  });
});
