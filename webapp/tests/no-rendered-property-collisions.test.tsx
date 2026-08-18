import { describe, expect, test } from "vitest";

import { REPRESENTATIVE_PAGES } from "./representativePages";
import { findPropertyCollisions } from "./tailwindClassCollisions";
import type { RenderResult } from "./render";

/**
 * WEBAPP_TAILWIND_TODO_2026-08-18.md T2-2: the SPEC §3 rule-4 invariant
 * ("no element carries two utilities that set the same CSS property"),
 * generalized from component-variant-maps.test.tsx's six known-risky maps
 * (T2-1) to every element on a real, populated page -- an executable check
 * standing in for the one-off Python/regex scan the migration review ran by
 * hand. jsdom is enough: this only reads `className` strings off rendered
 * elements, never anything jsdom cannot compute (layout, actual CSS).
 *
 * The four pages (`representativePages.tsx`, shared with T3-2's
 * `no-descendant-variant-nesting.test.tsx`) are structurally distinct:
 * `MacrosPage` (a list of `Card` rows, `StatusBadge`, `SendStatus`),
 * `DiagnosticsPage` (many `Card`s, dense `dl`/`dt`/`dd` grids),
 * `SettingsPage` (`Card`s, `FormActions`, `PageHeading`, `HeaderActions`),
 * `PackageManagementPage` (`Card` rows, a create form).
 */

function assertNoRenderedPropertyCollisions(view: RenderResult): void {
  const elements = view.container.querySelectorAll<HTMLElement>("*");
  const failures: string[] = [];
  for (const element of elements) {
    if (element.className === "" || typeof element.className !== "string") {
      continue;
    }
    const collisions = findPropertyCollisions(element.className);
    if (collisions.length > 0) {
      failures.push(
        `<${element.tagName} class="${element.className}">: ${collisions.join("; ")}`,
      );
    }
  }
  expect(failures, failures.join("\n")).toEqual([]);
}

describe("no rendered page has a same-property class collision", () => {
  for (const page of REPRESENTATIVE_PAGES) {
    test(page.name, async () => {
      const view = await page.render();
      assertNoRenderedPropertyCollisions(view);
      await view.unmount();
    });
  }
});
