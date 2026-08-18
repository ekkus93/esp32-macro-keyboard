import { describe, expect, test } from "vitest";

import { StandaloneScreen } from "../src/components/StandaloneScreen";
import { render } from "./render";

/**
 * WEBAPP_TAILWIND_TODO_2026-08-18.md T3-3: `StandaloneScreen`'s `*:mx-auto
 * *:w-[min(100%,27rem)]` sizes every direct child without exception
 * (`SPEC` §6.2). A `position: fixed` direct child -- a dialog backdrop, say
 * -- would be given a 27rem width and stop covering the viewport. No call
 * site does this today; this guards against it happening by accident.
 *
 * Checks the `fixed` utility *token*, not the computed `position` property:
 * jsdom applies no stylesheet at all (`SPEC` §10.1), so `getComputedStyle`
 * would report every element's `position` as the browser default
 * ("static") regardless of which Tailwind classes it carries. The class
 * token is the only signal jsdom can see, and it is also the only thing a
 * real call site could actually write.
 */

function directChildrenClassLists(container: HTMLElement): string[] {
  const main = container.querySelector("main");
  if (main === null) {
    throw new Error("Expected a <main> (StandaloneScreen) in the render.");
  }
  return Array.from(main.children).map(
    (child) => child.getAttribute("class") ?? "",
  );
}

function hasFixedToken(classList: string): boolean {
  return classList.split(/\s+/).includes("fixed");
}

describe("StandaloneScreen's *: child rule never reaches a fixed-position child", () => {
  test("an ordinary child is not flagged", async () => {
    const view = await render(
      <StandaloneScreen>
        <h1>Ordinary content</h1>
      </StandaloneScreen>,
    );
    const violations = directChildrenClassLists(view.container).filter(
      hasFixedToken,
    );
    expect(violations).toEqual([]);
    await view.unmount();
  });

  test("the detector has teeth: a fixed direct child is caught", async () => {
    // Deliberately constructed -- no real call site does this today.
    const view = await render(
      <StandaloneScreen>
        <div className="fixed inset-0">synthetic dialog backdrop</div>
      </StandaloneScreen>,
    );
    const violations = directChildrenClassLists(view.container).filter(
      hasFixedToken,
    );
    expect(violations).toEqual(["fixed inset-0"]);
    await view.unmount();
  });

  test("a fixed GRANDCHILD (not a direct child) is not flagged", async () => {
    // *: is Tailwind's child combinator (`> *`), not a descendant selector
    // -- it never reaches this deep, so this must not be treated as a
    // violation of the same kind.
    const view = await render(
      <StandaloneScreen>
        <div>
          <div className="fixed inset-0">nested, not a direct child</div>
        </div>
      </StandaloneScreen>,
    );
    const violations = directChildrenClassLists(view.container).filter(
      hasFixedToken,
    );
    expect(violations).toEqual([]);
    await view.unmount();
  });
});
