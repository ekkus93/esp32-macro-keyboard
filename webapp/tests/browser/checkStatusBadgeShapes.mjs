/**
 * WEBAPP_TAILWIND_TODO_2026-08-18.md T2-5: `UI_UX_SPEC_V2` §14 ("Color is
 * never the only indicator of USB, dirty, validation, or send state")
 * requires `StatusBadge`'s four `::before` dots to differ in *shape*, not
 * only in colour -- SPEC §9's own account of the four treatments (filled
 * disc with a halo / hollow ring / square / smaller hollow dot). This is the
 * executable version of that requirement: geometry only, colour set aside
 * entirely, so a future change that makes two states look different only by
 * hue would fail this even though a human skimming the four colours might
 * not notice.
 *
 * Needs real Chrome, not jsdom: `::before` computed style is exactly the
 * thing WEBAPP_TAILWIND_SPEC_2026-08-18.md §10.1 says jsdom cannot produce
 * (no CSS engine, no box model). Reuses the visual harness's own
 * `usb-badge-*` scenarios (`visual/scenarios.mjs`) rather than duplicating a
 * fixture drive -- those four already put the app through all four
 * `StatusBadge` states for real.
 */
import process from "node:process";
import { chromium } from "playwright";

import { SCENARIOS } from "./visual/scenarios.mjs";

/**
 * The shape-only property set: width, height, all four border-radius
 * corners, all four border widths, and box-shadow (the "good" state's halo
 * is a shadow, not a border -- its presence/absence is part of the shape,
 * not the colour). Deliberately excludes every colour property
 * (`background-color`, `color`, `border-*-color`) captured alongside these
 * in props.mjs's PSEUDO_CAPTURE_PROPS -- that is the whole point of this
 * check.
 */
const SHAPE_PROPS = [
  "width",
  "height",
  "border-top-left-radius",
  "border-top-right-radius",
  "border-bottom-left-radius",
  "border-bottom-right-radius",
  "border-top-width",
  "border-right-width",
  "border-bottom-width",
  "border-left-width",
  "box-shadow",
];

const USB_BADGE_SCENARIO_NAMES = [
  "usb-badge-ready",
  "usb-badge-suspended",
  "usb-badge-error",
  "usb-badge-disconnected",
];

function shapeSignature(pseudoElement) {
  return SHAPE_PROPS.map(
    (prop) => `${prop}=${pseudoElement.values[prop]}`,
  ).join(";");
}

async function main() {
  const scenarios = USB_BADGE_SCENARIO_NAMES.map((name) => {
    const scenario = SCENARIOS.find((candidate) => candidate.name === name);
    if (scenario === undefined) {
      throw new Error(
        `Expected scenario "${name}" in visual/scenarios.mjs -- was it renamed?`,
      );
    }
    return scenario;
  });

  const browser = await chromium.launch({ headless: true });
  const shapesByState = new Map();
  try {
    for (const scenario of scenarios) {
      const viewport = scenario.viewports[0];
      const capture = await scenario.run(browser, viewport);
      // The shell header renders TWO StatusBadges: the USB one first
      // (AppShellV2.tsx), then the "Saved"/"Unsaved changes" one -- which is
      // StatusBadge's neutral state too, so it cannot be picked out by
      // shape or class alone when the USB badge is also neutral
      // (usb-badge-disconnected). DOM order is stable and matches JSX
      // order, so the first `<span>` `::before` is reliably the USB badge;
      // asserted explicitly (>= 2, not exactly 1) rather than silently
      // indexing into whatever came back.
      const badgeBefores = capture.pseudoElements.filter(
        (element) => element.tag === "SPAN",
      );
      if (badgeBefores.length < 2) {
        throw new Error(
          `Expected at least two StatusBadge ::before elements (USB, then Saved/Unsaved) on "${scenario.name}", found ${String(badgeBefores.length)}.`,
        );
      }
      shapesByState.set(scenario.name, shapeSignature(badgeBefores[0]));
    }
  } finally {
    await browser.close();
  }

  console.log("Shape signatures (colour excluded):");
  for (const [name, signature] of shapesByState) {
    console.log(`  ${name}: ${signature}`);
  }

  const seen = new Map();
  const collisions = [];
  for (const [name, signature] of shapesByState) {
    const priorOwner = seen.get(signature);
    if (priorOwner !== undefined) {
      collisions.push(`${name} has the identical shape to ${priorOwner}`);
    }
    seen.set(signature, name);
  }

  if (collisions.length > 0) {
    console.log(
      `\n${String(collisions.length)} StatusBadge state(s) are not geometrically distinct:`,
    );
    for (const collision of collisions) {
      console.log(`  ${collision}`);
    }
    console.log(
      "\nUI_UX_SPEC_V2 §14 requires colour to never be the only indicator -- " +
        "two states with the same shape rely on colour alone to be told apart.",
    );
    process.exitCode = 1;
    return;
  }

  console.log(
    `\nAll ${String(shapesByState.size)} StatusBadge states are geometrically distinct, colour aside.`,
  );
}

await main();
