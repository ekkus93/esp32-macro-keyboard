import { startApplicationServer } from "../fixtures/applicationServer.mjs";
import { runAccessibilityScan } from "../lib/accessibility.mjs";
import { assert } from "../lib/http.mjs";
import { assertFitsViewport, evaluate, waitFor } from "../lib/page.mjs";

/**
 * UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §10 / TODO UXR-021: at the smallest
 * supported viewports, a screen's title, primary action, essential state and
 * next-step control are visible in the initial viewport -- fully inside it
 * (no scrolling needed) and not covered by fixed chrome such as the app bar,
 * the navigation, or an overlay.
 *
 * `SCREENS` is a table, not a script, so each redesign phase adds the
 * controls its screen promises as it lands (the TODO's phase exit gates), and
 * a regression on an earlier screen fails here. Only controls that are true
 * today belong in it; an aspiration that currently fails is not listed.
 *
 * Each entry is a Playwright locator factory. A control passes when exactly
 * the first match (a) has a bounding box wholly inside the viewport and (b) is
 * the topmost element at its centre point, i.e. nothing is painted over it.
 */
export const VIEWPORTS = [
  { name: "320x568", width: 320, height: 568 },
  { name: "360x800", width: 360, height: 800 },
  { name: "390x844", width: 390, height: 844 },
  // Tablet portrait and landscape, and desktop: the navigation rail layout.
  { name: "768x1024", width: 768, height: 1024 },
  { name: "1024x768", width: 1024, height: 768 },
  { name: "1440x900", width: 1440, height: 900 },
];

const NAV = ["Macros", "Packages", "Snapshots", "Settings"];

/** Controls every authenticated route must show without scrolling. */
const SHELL_CONTROLS = [
  { label: "page title", find: (page) => page.locator("header h1") },
  { label: "USB state", find: (page) => page.getByText(/^USB /) },
  {
    label: "saved state",
    find: (page) => page.getByText(/^(Saved|Unsaved changes)$/),
  },
  ...NAV.map((name) => ({
    label: `navigation: ${name}`,
    find: (page) =>
      page
        .getByRole("navigation", { name: "Primary navigation" })
        .getByRole("button", { name }),
  })),
];

const button = (name) => ({
  label: `button: ${name}`,
  find: (page) => page.getByRole("button", { name, exact: true }),
});

/** route hash -> additional controls that screen promises. */
export const SCREENS = [
  {
    route: "macros",
    ready: "Lab bench workflow",
    controls: [button("Add macro")],
  },
  { route: "packages", ready: "Create package", controls: [] },
  { route: "snapshots", ready: "Snapshots", controls: [] },
  { route: "settings", ready: "Settings", controls: [] },
];

async function assertVisibleInViewport(page, control, where) {
  const locator = control.find(page).first();
  const box = await locator.boundingBox();
  assert(box !== null, `${where}: ${control.label} is not rendered.`);
  const viewport = page.viewportSize();
  assert(
    box.x >= 0 &&
      box.y >= 0 &&
      box.x + box.width <= viewport.width + 0.5 &&
      box.y + box.height <= viewport.height + 0.5,
    `${where}: ${control.label} is not fully inside the ${String(viewport.width)}x${String(viewport.height)} viewport (${JSON.stringify(box)}).`,
  );
  const covered = await locator.evaluate((element) => {
    const rect = element.getBoundingClientRect();
    const top = document.elementFromPoint(
      rect.x + rect.width / 2,
      rect.y + rect.height / 2,
    );
    return top === null || !(element.contains(top) || top.contains(element));
  });
  assert(!covered, `${where}: ${control.label} is covered by another element.`);
}

export async function runVisibleControlsWorkflow(browser) {
  const application = await startApplicationServer();
  try {
    for (const viewport of VIEWPORTS) {
      const context = await browser.newContext({
        viewport: { width: viewport.width, height: viewport.height },
      });
      const page = await context.newPage();
      try {
        await page.goto(application.baseUrl);
        await waitFor(
          page,
          () => document.body.innerText.includes("Lab bench workflow"),
          "The Macros page did not render.",
        );
        for (const screen of SCREENS) {
          await evaluate(
            page,
            (route) => {
              window.location.hash = `/${route}`;
            },
            screen.route,
          );
          await waitFor(
            page,
            new Function(
              `return document.body.innerText.includes(${JSON.stringify(screen.ready)});`,
            ),
            `The ${screen.route} screen did not render.`,
          );
          const where = `${screen.route} @ ${viewport.name}`;
          for (const control of [...SHELL_CONTROLS, ...screen.controls]) {
            await assertVisibleInViewport(page, control, where);
          }
          await assertFitsViewport(page, where);
          // axe at the narrowest phone and the widest desktop: the two
          // layouts (navigation bar and navigation rail) differ most there.
          if (viewport.name === "320x568" || viewport.name === "1440x900") {
            await runAccessibilityScan(page, where);
          }
        }
      } finally {
        await context.close();
      }
    }
  } finally {
    await application.close();
  }
}

/**
 * On a touch phone the bottom navigation steps aside while a text field has
 * focus, so the on-screen keyboard, the field and its action get the height
 * (UXR-021 virtual keyboard rule). A real keyboard cannot be summoned in
 * headless Chrome, so this proves the part that is ours: the navigation is
 * hidden exactly while a field is focused under a coarse pointer at phone
 * width, and returns on blur. `interactive-widget=resizes-content` in
 * index.html is what makes the browser shrink the layout around the keyboard.
 */
export async function runKeyboardNavigationWorkflow(browser) {
  const application = await startApplicationServer();
  try {
    const context = await browser.newContext({
      viewport: { width: 360, height: 800 },
      isMobile: true,
      hasTouch: true,
    });
    const page = await context.newPage();
    try {
      await page.goto(application.baseUrl);
      await waitFor(
        page,
        () => document.body.innerText.includes("Lab bench workflow"),
        "The Macros page did not render.",
      );
      await evaluate(page, () => {
        window.location.hash = "/packages";
      });
      await waitFor(
        page,
        () =>
          document.querySelector("#package-management-create-name") !== null,
        "The Packages page did not render.",
      );
      const nav = page.getByRole("navigation", { name: "Primary navigation" });
      assert(
        await nav.isVisible(),
        "Navigation is hidden before any field has focus.",
      );
      await page.locator("#package-management-create-name").focus();
      assert(
        !(await nav.isVisible()),
        "Navigation stays on screen while a text field has focus on a touch phone.",
      );
      await page.locator("#package-management-create-name").blur();
      assert(
        await nav.isVisible(),
        "Navigation did not return after the field lost focus.",
      );
    } finally {
      await context.close();
    }
  } finally {
    await application.close();
  }
}
