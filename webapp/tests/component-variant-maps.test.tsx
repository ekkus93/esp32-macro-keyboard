import { useRef } from "react";
import { describe, expect, test } from "vitest";

import { Card, type CardVariant } from "../src/components/Card";
import { Dialog, type DialogTone } from "../src/components/Dialog";
import { Eyebrow, type EyebrowTone } from "../src/components/Eyebrow";
import { FieldHelp } from "../src/components/FieldHelp";
import { SendStatus } from "../src/components/SendStatus";
import {
  StatusBadge,
  type StatusBadgeState,
} from "../src/components/StatusBadge";
import { findPropertyCollisions } from "./tailwindClassCollisions";
import { render } from "./render";

/**
 * WEBAPP_TAILWIND_TODO_2026-08-18.md T2-1: the six variant maps in
 * WEBAPP_TAILWIND_SPEC_2026-08-18.md §4.2 had no test at all before this
 * file -- a wrong key, or two conflicting utilities layered into one
 * variant, was previously only visible to a hand-run visual diff. Renders
 * every real variant through the component's own public props (not by
 * reaching into the module-private class-string maps: exporting those broke
 * Vite Fast Refresh, `react-refresh/only-export-components` -- see 1b4fda5's
 * sibling investigation) and checks, per variant:
 *
 *  1. The rendered `className` is non-empty.
 *  2. Every variant of a component is textually distinct from every other
 *     variant of that component (a copy-paste key returning the same string
 *     as another would pass "non-empty" and fail this).
 *  3. `findPropertyCollisions` reports nothing -- SPEC §3 rule 4, no two
 *     utilities in the rendered class setting the same CSS property.
 *
 * `findPropertyCollisions` is exactly what caught the real,
 * previously-unnoticed `border`/`border-l-[3px]` defect fixed in 1b4fda5,
 * confirming this check has actual teeth rather than only ever passing.
 */

function classNameOf(container: HTMLElement, selector: string): string {
  const element = container.querySelector(selector);
  if (element === null) {
    throw new Error(
      `Missing element matching "${selector}" in rendered output.`,
    );
  }
  return element.className;
}

function assertVariantsAreCleanAndDistinct(
  componentName: string,
  classNamesByVariant: Record<string, string>,
): void {
  const entries = Object.entries(classNamesByVariant);
  const seen = new Map<string, string>();
  for (const [variant, className] of entries) {
    expect(
      className.trim().length,
      `${componentName}.${variant} rendered no classes`,
    ).toBeGreaterThan(0);

    const collisions = findPropertyCollisions(className);
    expect(
      collisions,
      `${componentName}.${variant} has conflicting utilities: ${collisions.join("; ")}`,
    ).toEqual([]);

    const priorOwner = seen.get(className);
    expect(
      priorOwner,
      `${componentName}.${variant} renders the same class string as ${componentName}.${String(priorOwner)}`,
    ).toBeUndefined();
    seen.set(className, variant);
  }
}

describe("Card variant map", () => {
  test("every variant is non-empty, collision-free, and distinct", async () => {
    const variants: CardVariant[] = ["default", "flush", "danger"];
    const classNamesByVariant: Record<string, string> = {};
    for (const variant of variants) {
      const view = await render(<Card variant={variant}>content</Card>);
      classNamesByVariant[variant] = classNameOf(view.container, "article");
      await view.unmount();
    }
    assertVariantsAreCleanAndDistinct("Card", classNamesByVariant);
  });
});

describe("Dialog tone variant map", () => {
  function DialogHarness({ tone }: { tone: DialogTone }): React.JSX.Element {
    const containerRef = useRef<HTMLDivElement>(null);
    return (
      <Dialog
        aria-labelledby="test-dialog-title"
        containerRef={containerRef}
        heading={<h2 id="test-dialog-title">Title</h2>}
        role="alertdialog"
        tone={tone}
      >
        content
      </Dialog>
    );
  }

  test("every tone is non-empty, collision-free, and distinct", async () => {
    const tones: DialogTone[] = ["panel", "danger"];
    const classNamesByVariant: Record<string, string> = {};
    for (const tone of tones) {
      const view = await render(<DialogHarness tone={tone} />);
      classNamesByVariant[tone] = classNameOf(
        view.container,
        '[role="alertdialog"]',
      );
      await view.unmount();
    }
    assertVariantsAreCleanAndDistinct("Dialog", classNamesByVariant);
  });
});

describe("StatusBadge state variant map", () => {
  test("every state is non-empty, collision-free, and distinct", async () => {
    const states: StatusBadgeState[] = ["good", "warning", "bad", "neutral"];
    const classNamesByVariant: Record<string, string> = {};
    for (const state of states) {
      const view = await render(
        <StatusBadge label="USB ready" state={state} />,
      );
      classNamesByVariant[state] = classNameOf(view.container, "span");
      await view.unmount();
    }
    assertVariantsAreCleanAndDistinct("StatusBadge", classNamesByVariant);
  });
});

describe("FieldHelp exceeded variant map", () => {
  test("every state is non-empty, collision-free, and distinct", async () => {
    const classNamesByVariant: Record<string, string> = {};
    for (const exceeded of [false, true]) {
      const view = await render(
        <FieldHelp exceeded={exceeded}>64 / 64</FieldHelp>,
      );
      classNamesByVariant[exceeded ? "exceeded" : "within"] = classNameOf(
        view.container,
        "span",
      );
      await view.unmount();
    }
    assertVariantsAreCleanAndDistinct("FieldHelp", classNamesByVariant);
  });
});

describe("SendStatus role variant map", () => {
  test("every role is non-empty, collision-free, and distinct", async () => {
    const roles: ("status" | "alert")[] = ["status", "alert"];
    const classNamesByVariant: Record<string, string> = {};
    for (const role of roles) {
      const view = await render(<SendStatus role={role}>Sending…</SendStatus>);
      classNamesByVariant[role] = classNameOf(
        view.container,
        `[role="${role}"]`,
      );
      await view.unmount();
    }
    assertVariantsAreCleanAndDistinct("SendStatus", classNamesByVariant);
  });

  test("the overlay variant appends, and stays collision-free", async () => {
    const view = await render(
      <SendStatus overlay role="status">
        Sending…
      </SendStatus>,
    );
    const className = classNameOf(view.container, '[role="status"]');
    expect(findPropertyCollisions(className)).toEqual([]);
    await view.unmount();
  });
});

describe("Eyebrow tone variant map", () => {
  test("every tone is non-empty, collision-free, and distinct", async () => {
    const tones: EyebrowTone[] = ["default", "dark"];
    const classNamesByVariant: Record<string, string> = {};
    for (const tone of tones) {
      const view = await render(<Eyebrow tone={tone}>Device name</Eyebrow>);
      classNamesByVariant[tone] = classNameOf(view.container, "p");
      await view.unmount();
    }
    assertVariantsAreCleanAndDistinct("Eyebrow", classNamesByVariant);
  });
});
