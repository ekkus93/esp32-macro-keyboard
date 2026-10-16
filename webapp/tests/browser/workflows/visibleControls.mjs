import { startApplicationServer } from "../fixtures/applicationServer.mjs";
import { startStartupFixtureServer } from "../fixtures/startupFixtureServer.mjs";
import { blobId, repositoryGzip } from "../fixtures/data.mjs";
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
const STATUS_CONTROLS = [
  { label: "page title", find: (page) => page.locator("header h1") },
  { label: "USB state", find: (page) => page.getByText(/^USB /) },
  {
    label: "saved state",
    find: (page) => page.getByText(/^(Saved|Unsaved changes)$/),
  },
];

/** Primary navigation: the bottom bar on phones, the rail from 600px up. */
const NAVIGATION_CONTROLS = NAV.map((name) => ({
  label: `navigation: ${name}`,
  find: (page) =>
    page
      .getByRole("navigation", { name: "Primary navigation" })
      .getByRole("button", { name }),
}));

/** Below this width the bottom bar replaces the rail. */
const RAIL_MIN_WIDTH = 600;

const button = (name) => ({
  label: `button: ${name}`,
  find: (page) => page.getByRole("button", { name, exact: true }),
});

const labelled = (name) => ({
  label: `control labelled "${name}"`,
  find: (page) => page.getByLabel(name, { exact: true }),
});

/**
 * route hash -> additional controls that screen promises. A control may carry
 * `minHeight`: it is only asserted where the viewport is at least that tall.
 */
export const SCREENS = [
  {
    route: "macros",
    ready: "Lab bench workflow",
    controls: [
      button("Add macro"),
      button("Change package"),
      // The first row's everyday controls, at every size.
      labelled("Send Open terminal"),
      labelled("Edit Open terminal"),
      labelled("More actions for Open terminal"),
      // UXR-040: on a normal phone at least two rows are visible.
      { ...labelled("Send Confirm before typing"), minHeight: 800 },
    ],
  },
  {
    // UXR-042: the editor is a focused task screen. On a phone its bottom
    // bar gives way to a Back control; Cancel, Save, the fields and the key
    // picker entry point must all be on screen at once.
    route: "macro-editor",
    ready: "Macro source",
    focused: true,
    controls: [
      labelled("Back to macros"),
      button("Cancel"),
      button("Save changes"),
      {
        label: "Name field",
        find: (page) => page.locator("#macro-editor-name"),
      },
      {
        label: "Macro source field",
        find: (page) => page.locator("#macro-editor-source"),
      },
      button("Keys and combinations"),
    ],
  },
  {
    route: "macro-editor",
    ready: "Macro source",
    focused: true,
    name: "key picker",
    drive: async (page) => {
      await page
        .getByRole("button", { name: "Keys and combinations", exact: true })
        .click();
    },
    controls: [
      {
        label: "picker title",
        find: (page) => page.getByText("Insert into macro"),
      },
      button("Done"),
      labelled("Insert Enter key"),
      labelled("Insert Tab key"),
    ],
  },
  {
    route: "packages",
    ready: "New package",
    controls: [
      button("New package"),
      labelled("Open Lab bench workflow"),
      labelled("More actions for Lab bench workflow"),
    ],
  },
  {
    route: "snapshots",
    ready: "Import or export",
    controls: [
      button("Save current snapshot"),
      button("Import or export"),
      labelled("Load snapshot 1"),
      labelled("More actions for snapshot 1"),
    ],
  },
  {
    // UXR-051/052: Import and export is its own focused screen.
    route: "snapshots?view=files",
    name: "import and export",
    ready: "Export working copy",
    focused: true,
    controls: [
      labelled("Back to snapshots"),
      button("Export working copy"),
      button("Import repository…"),
    ],
  },
  {
    route: "settings",
    ready: "USB identity and confirmation",
    controls: [
      {
        label: "settings group: Device identity",
        find: (page) => page.getByRole("button", { name: /^Device identity/ }),
      },
      {
        label: "settings group: Network",
        find: (page) => page.getByRole("button", { name: /^Network/ }),
      },
      {
        label: "settings group: Security",
        find: (page) => page.getByRole("button", { name: /^Security/ }),
      },
      {
        label: "settings group: USB identity and confirmation",
        find: (page) =>
          page.getByRole("button", { name: /^USB identity and confirmation/ }),
      },
      {
        label: "settings group: Restart and reset",
        find: (page) =>
          page.getByRole("button", { name: /^Restart and reset/ }),
      },
      {
        label: "settings group: Diagnostics",
        find: (page) => page.getByRole("button", { name: /^Diagnostics/ }),
      },
    ],
  },
  {
    route: "settings?group=device",
    name: "device identity settings",
    ready: "Device name",
    focused: true,
    controls: [
      labelled("Device name"),
      labelled("Advisory snapshot retention target"),
      button("Save"),
    ],
  },
  {
    route: "settings?group=network",
    name: "network settings",
    ready: "Access point",
    focused: true,
    controls: [button("Access point"), button("Station network")],
  },
  {
    route: "settings?group=network&section=access-point",
    name: "access-point settings",
    ready: "Access-point network name (SSID)",
    focused: true,
    controls: [
      labelled("Back to network settings"),
      labelled("Access-point network name (SSID)"),
      labelled("New access-point passphrase"),
      button("Update access point"),
    ],
  },
  {
    route: "settings?group=network&section=station",
    name: "station network settings",
    ready: "Network name (SSID)",
    focused: true,
    controls: [
      labelled("Back to network settings"),
      labelled("Network name (SSID)"),
      labelled("Passphrase"),
      button("Connect"),
    ],
  },
  {
    route: "settings?group=security",
    name: "security settings",
    ready: "Change password",
    focused: true,
    controls: [button("Change password"), button("Sign out")],
  },
  {
    route: "settings?group=security&section=password",
    name: "change password settings",
    ready: "Current password",
    focused: true,
    controls: [
      labelled("Back to security settings"),
      labelled("Current password"),
      labelled("New password"),
      labelled("Confirm new password"),
      button("Change password"),
    ],
  },
  {
    route: "settings?group=security&section=session",
    name: "sign-out settings",
    ready: "Sign out",
    focused: true,
    controls: [labelled("Back to security settings"), button("Sign out")],
  },
  {
    route: "settings?group=usb",
    name: "USB identity and confirmation",
    ready: "USB keyboard identity",
    focused: true,
    controls: [
      labelled("Require physical confirmation before typing"),
      button("Save"),
    ],
  },
  {
    route: "settings?group=reset",
    name: "restart and reset settings",
    ready: "Factory reset",
    focused: true,
    controls: [
      button("Restart"),
      button("Reset settings"),
      button("Factory reset"),
    ],
  },
  {
    route: "settings?group=reset&section=restart",
    name: "restart settings",
    ready: "The device restarts and its Wi-Fi drops briefly.",
    focused: true,
    controls: [labelled("Back to restart and reset"), button("Restart")],
  },
  {
    route: "settings?group=reset&section=reset-settings",
    name: "reset settings",
    ready: "Restores default settings and removes the station network.",
    focused: true,
    controls: [labelled("Back to restart and reset"), button("Reset settings")],
  },
  {
    route: "settings?group=reset&section=factory-reset",
    name: "factory reset settings",
    ready: "Erases all settings, the password and every snapshot",
    focused: true,
    controls: [labelled("Back to restart and reset"), button("Factory reset")],
  },
  {
    route: "diagnostics",
    name: "diagnostics",
    ready: "Download diagnostics",
    focused: true,
    controls: [button("Download diagnostics"), button("Copy diagnostics")],
  },
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
          if (screen.drive !== undefined) {
            await screen.drive(page);
          }
          const where = `${screen.name ?? screen.route} @ ${viewport.name}`;
          const showsBottomBar =
            screen.focused !== true || viewport.width >= RAIL_MIN_WIDTH;
          for (const control of [
            ...STATUS_CONTROLS,
            ...(showsBottomBar ? NAVIGATION_CONTROLS : []),
            ...screen.controls,
          ]) {
            if (
              control.minHeight !== undefined &&
              viewport.height < control.minHeight
            ) {
              continue;
            }
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
 * width, and returns on blur.
 *
 * Deliberately NOT done: `interactive-widget=resizes-content` in index.html.
 * It would shrink the layout viewport when the keyboard opens, and the
 * landscape-phone rule (`useLandscapePhoneBlock`) is a media query on that
 * viewport -- on a 320px-wide phone a ~260px keyboard-open height reads as
 * "landscape" and would replace the app with "Rotate your phone" mid-typing.
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
        () => document.body.innerText.includes("New package"),
        "The Packages page did not render.",
      );
      const nav = page.getByRole("navigation", { name: "Primary navigation" });
      assert(
        await nav.isVisible(),
        "Navigation is hidden before any field has focus.",
      );
      // The New package dialog moves focus into its name field on open.
      await page
        .getByRole("button", { name: "New package", exact: true })
        .click();
      await page.waitForSelector("#package-management-create-name");
      assert(
        await page
          .locator("#package-management-create-name")
          .evaluate((field) => document.activeElement === field),
        "The dialog did not focus its name field.",
      );
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

/**
 * UXR-021: opening a phone's virtual keyboard reduces the usable height.
 * Playwright cannot open Android's Gboard, so focus the real macro source
 * field, then shrink the mobile viewport to the space remaining above a
 * typical keyboard. This covers the web layout behavior: the focused field
 * and Save/Cancel stay reachable, and bottom navigation steps aside.
 */
export async function runMacroEditorKeyboardViewportWorkflow(browser) {
  const application = await startApplicationServer();
  try {
    const context = await browser.newContext({
      viewport: { width: 390, height: 844 },
      isMobile: true,
      hasTouch: true,
    });
    const page = await context.newPage();
    try {
      await page.goto(application.baseUrl);
      await waitFor(
        page,
        () => document.body.innerText.includes("Lab bench workflow"),
        "The Macros page did not render for the keyboard viewport check.",
      );
      await page.getByRole("button", { name: "Edit Open terminal" }).click();
      const source = page.locator("#macro-editor-source");
      await source.click();
      await page.setViewportSize({ width: 390, height: 450 });

      const navigation = page.getByRole("navigation", {
        name: "Primary navigation",
      });
      assert(
        !(await navigation.isVisible()),
        "Phone navigation did not step aside while the macro source field had focus.",
      );
      const where = "Macro editor with virtual-keyboard viewport @ 390x450";
      await assertVisibleInViewport(
        page,
        { label: "Macro source field", find: () => source },
        where,
      );
      for (const name of ["Cancel", "Save changes"]) {
        await assertVisibleInViewport(
          page,
          {
            label: `button: ${name}`,
            find: (currentPage) =>
              currentPage.getByRole("button", { name, exact: true }),
          },
          where,
        );
      }
      await assertFitsViewport(page, where);
    } finally {
      await context.close();
    }
  } finally {
    await application.close();
  }
}

const id = (selector) => ({
  label: `field ${selector}`,
  find: (page) => page.locator(selector),
});
const text = (pattern) => ({
  label: `text ${String(pattern)}`,
  find: (page) => page.getByText(pattern),
});

/**
 * Pre-authentication and startup screens (UXR-030/031): no navigation bar, so
 * the rule is simply that each step's fields, state and next action are in
 * the first viewport. `drive` walks the screen to the state under test.
 */
export const STARTUP_SCREENS = [
  {
    name: "setup step 1",
    options: { provisioned: false },
    marker: "Set up your keyboard",
    controls: [
      text("Step 1 of 4: Device"),
      id("#setup-code"),
      id("#device-name"),
      button("Continue"),
    ],
  },
  {
    name: "setup step 2",
    options: { provisioned: false },
    marker: "Set up your keyboard",
    drive: async (page) => {
      await page.locator("#setup-code").fill("12345678");
      await page.getByRole("button", { name: "Continue", exact: true }).click();
    },
    controls: [
      text("Step 2 of 4: Wi-Fi"),
      id("#ap-ssid"),
      id("#ap-passphrase"),
      button("Back"),
      button("Continue"),
    ],
  },
  {
    name: "setup step 3",
    options: { provisioned: false },
    marker: "Set up your keyboard",
    drive: async (page) => {
      await page.locator("#setup-code").fill("12345678");
      await page.getByRole("button", { name: "Continue", exact: true }).click();
      await page.locator("#ap-ssid").fill("MacroKeyboard");
      await page.locator("#ap-passphrase").fill("bench-ap-passphrase-1");
      await page.getByRole("button", { name: "Continue", exact: true }).click();
    },
    controls: [
      text("Step 3 of 4: Access"),
      id("#admin-password"),
      button("Back"),
      button("Review setup"),
    ],
  },
  {
    name: "setup review",
    options: { provisioned: false },
    marker: "Set up your keyboard",
    drive: async (page) => {
      await page.locator("#setup-code").fill("12345678");
      await page.getByRole("button", { name: "Continue", exact: true }).click();
      await page.locator("#ap-ssid").fill("MacroKeyboard");
      await page.locator("#ap-passphrase").fill("bench-ap-passphrase-1");
      await page.getByRole("button", { name: "Continue", exact: true }).click();
      await page.locator("#admin-password").fill("bench-fixture-admin-pw-1");
      await page
        .getByRole("button", { name: "Review setup", exact: true })
        .click();
    },
    controls: [
      text("Step 4 of 4: Review"),
      button("Back"),
      button("Apply setup"),
    ],
  },
  {
    name: "sign in",
    options: { authenticated: false },
    marker: "Sign in",
    controls: [id("#admin-password"), button("Sign in")],
  },
  {
    name: "first repository",
    options: { blobs: [] },
    marker: "Create Your First Repository",
    controls: [id("#first-package-name"), button("Create package")],
  },
  {
    name: "snapshot recovery",
    options: {
      blobs: [
        { id: blobId, bytes: repositoryGzip },
        { id: "2", bytes: Buffer.from([0, 1, 2, 3, 4]) },
      ],
    },
    marker: "Snapshot recovery",
    controls: [
      button("Start over"),
      {
        label: "first snapshot candidate",
        find: (page) => page.locator('[aria-label^="Load snapshot"]'),
      },
    ],
  },
];

export async function runStartupVisibleControlsWorkflow(browser) {
  for (const viewport of VIEWPORTS) {
    for (const screen of STARTUP_SCREENS) {
      const fixture = await startStartupFixtureServer(screen.options);
      const context = await browser.newContext({
        viewport: { width: viewport.width, height: viewport.height },
      });
      const page = await context.newPage();
      try {
        await page.goto(fixture.baseUrl);
        await waitFor(
          page,
          new Function(
            `return document.body.innerText.includes(${JSON.stringify(screen.marker)});`,
          ),
          `The ${screen.name} screen did not render.`,
        );
        if (screen.drive !== undefined) {
          await screen.drive(page);
        }
        const where = `${screen.name} @ ${viewport.name}`;
        for (const control of screen.controls) {
          await assertVisibleInViewport(page, control, where);
        }
        await assertFitsViewport(page, where);
        if (viewport.name === "320x568" || viewport.name === "1440x900") {
          await runAccessibilityScan(page, where);
        }
      } finally {
        await context.close();
        await fixture.close();
      }
    }
  }
}
