/**
 * Desktop and tablet browser matrix against the real device.
 *
 * Covers the parts of Phase 13 that a phone cannot answer:
 *
 *   * device classification (UI_UX_SPEC_V2 §12.4) — the landscape block must
 *     fire for phone-like displays and must NOT broaden to tablet, laptop, or
 *     desktop landscape. Each case flips exactly one term of the query
 *     `(orientation: landscape) and (pointer: coarse) and (max-height: 600px)`,
 *     so a case that passes for the wrong reason is visible;
 *   * a 320 CSS-pixel viewport (V2-130) — the narrowest supported width;
 *   * focus order (V2-133) — DOM order must equal tab order.
 *
 * Runs real Chromium on this host against the device over the LAN. Nothing
 * joins the device SoftAP.
 *
 * Usage: node tests/hardware/run-v2-desktop-matrix.mjs [--origin http://IP]
 */
import { createRequire } from "node:module";

// tests/hardware/ has no node_modules of its own; Playwright lives in the
// webapp install. Resolve it relative to this file so the script runs from any
// working directory.
const require = createRequire(new URL("../../webapp/package.json", import.meta.url));
const { chromium } = require("playwright");
import fs from "node:fs";

const originArgument = process.argv.indexOf("--origin");
const ORIGIN =
  originArgument >= 0 ? process.argv[originArgument + 1] : "http://192.168.88.108";
const PASSWORD = fs
  .readFileSync(`${process.env.HOME}/.config/esp32-macro-keyboard/hil/admin_password.txt`, "utf8")
  .trim();

const failures = [];
function check(condition, message) {
  console.log(`${condition ? "PASS" : "FAIL"}: ${message}`);
  if (!condition) failures.push(message);
}

/**
 * Acquire a signed-in page, retrying the whole setup once.
 *
 * The retry is for this bench, not the product: continuous Docker veth churn
 * makes Chromium abort navigations and occasionally stretches a load past the
 * waits below. Retrying a whole case is honest here because each attempt starts
 * from a fresh context; a product failure fails both attempts.
 */
async function signedInPageWithRetry(browser, viewport, touch, attempts = 3) {
  let lastError = null;
  for (let attempt = 0; attempt < attempts; attempt += 1) {
    let acquired = null;
    try {
      acquired = await signedInPage(browser, viewport, touch);
      return acquired;
    } catch (error) {
      lastError = error;
      if (acquired?.context) await acquired.context.close().catch(() => {});
    }
  }
  throw lastError;
}

async function signedInPage(browser, viewport, touch) {
  const context = await browser.newContext({
    viewport,
    hasTouch: touch,
    isMobile: touch,
  });
  const page = await context.newPage();
  const errors = [];
  page.on("pageerror", (error) => errors.push(error.message));
  // This bench churns Docker veth interfaces continuously, which Chromium sees
  // as a network change and which aborts an in-flight navigation with
  // ERR_NETWORK_CHANGED. The device and Wi-Fi are healthy; only the navigation
  // needs to be retried.
  let lastError = null;
  for (let attempt = 0; attempt < 4; attempt += 1) {
    try {
      await page.goto(ORIGIN, { waitUntil: "load" });
      lastError = null;
      break;
    } catch (error) {
      lastError = error;
      if (!/ERR_NETWORK_CHANGED|ERR_NETWORK_IO_SUSPENDED/.test(String(error))) throw error;
      await page.waitForTimeout(1500);
    }
  }
  if (lastError) throw lastError;
  await page.waitForTimeout(1200);
  if (await page.getByLabel(/administrator password/i).count()) {
    await page.getByLabel(/administrator password/i).fill(PASSWORD);
    await page.getByRole("button", { name: /^sign in$/i }).click();
    // Wait for the form to actually go away rather than for a fixed delay: under
    // this bench's network churn a fixed wait sometimes sampled the sign-in
    // screen and reported "not blocked" for a reason unrelated to the display.
    // Asserted on rendered text, not on a locator's detachment -- a locator
    // wait that throws (strict-mode or timeout) is easy to swallow silently,
    // which is exactly how this produced confusing alternating failures.
    await page.waitForFunction(
      () => !document.body.innerText.includes("Administrator password"),
      undefined,
      { timeout: 25000 },
    );
  }
  // Wait for a settled surface before sampling. The landscape block wraps the
  // authenticated shell, so sampling while the app is still on a standalone
  // screen (loading, first-run, sign-in) reports "not blocked" for a reason
  // that has nothing to do with device classification.
  await page.waitForFunction(
    () => document.querySelector(".app-shell") || document.querySelector(".landscape-block"),
    undefined,
    { timeout: 20000 },
  );
  await page.waitForTimeout(500);
  return { context, page, errors };
}

/* --- 1. Device classification -------------------------------------------- */

const CASES = [
  { name: "desktop 1920x1080", viewport: { width: 1920, height: 1080 }, touch: false, blocked: false },
  { name: "desktop 1280x800", viewport: { width: 1280, height: 800 }, touch: false, blocked: false },
  // The discriminating case: short enough to match max-height, but pointer:fine.
  { name: "desktop short 1000x500", viewport: { width: 1000, height: 500 }, touch: false, blocked: false },
  { name: "tablet 1024x768 coarse", viewport: { width: 1024, height: 768 }, touch: true, blocked: false },
  { name: "tablet 1280x800 coarse", viewport: { width: 1280, height: 800 }, touch: true, blocked: false },
  // Controls: all three terms true, so the block must fire.
  { name: "phone 800x400 coarse", viewport: { width: 800, height: 400 }, touch: true, blocked: true },
  { name: "phone 640x360 coarse", viewport: { width: 640, height: 360 }, touch: true, blocked: true },
];

const browser = await chromium.launch({ headless: true });
console.log(`\n[1/3] device classification (UI_UX_SPEC_V2 §12.4) against ${ORIGIN}`);
for (const testCase of CASES) {
  const { context, page, errors } = await signedInPageWithRetry(browser, testCase.viewport, testCase.touch);
  const text = await page.evaluate(() => document.body.innerText);
  const mediaMatches = await page.evaluate(() =>
    window.matchMedia(
      "(orientation: landscape) and (pointer: coarse) and (max-height: 600px)",
    ).matches,
  );
  const blocked = /Rotate your phone/i.test(text);
  check(!/Administrator password/i.test(text),
    `${testCase.name}: reached an authenticated surface (not still signing in)`);
  const horizontal = await page.evaluate(
    () => document.documentElement.scrollWidth > document.documentElement.clientWidth,
  );
  check(
    blocked === testCase.blocked && mediaMatches === testCase.blocked,
    `${testCase.name}: blocked=${blocked} mediaQuery=${mediaMatches} (expected ${testCase.blocked})`,
  );
  check(!horizontal, `${testCase.name}: no horizontal page scroll`);
  check(errors.length === 0, `${testCase.name}: no page errors${errors.length ? ` (${errors[0]})` : ""}`);
  await context.close();
}

/* --- 2. Narrowest supported viewport (V2-130) ---------------------------- */

console.log("\n[2/3] 320 CSS-pixel viewport (V2-130)");
{
  const { context, page, errors } = await signedInPageWithRetry(browser, { width: 320, height: 568 }, true);
  const metrics = await page.evaluate(() => {
    const root = document.documentElement;
    const shell = document.querySelector(".app-shell, .standalone");
    return {
      horizontalScroll: root.scrollWidth > root.clientWidth,
      scrollWidth: root.scrollWidth,
      clientWidth: root.clientWidth,
      shellWidth: shell ? Math.round(shell.getBoundingClientRect().width) : null,
      overflowing: [...document.querySelectorAll("main *")]
        .filter((element) => element.getBoundingClientRect().right > root.clientWidth + 1)
        .slice(0, 5)
        .map((element) => `${element.tagName}.${element.className}`.slice(0, 48)),
    };
  });
  check(!metrics.horizontalScroll,
    `320px: no horizontal scroll (scrollWidth ${metrics.scrollWidth} vs client ${metrics.clientWidth})`);
  check(metrics.overflowing.length === 0,
    `320px: nothing overflows the viewport${metrics.overflowing.length ? ` (${metrics.overflowing.join(", ")})` : ""}`);
  check(errors.length === 0, "320px: no page errors");
  await context.close();
}

/* --- 3. Single-column phone layout (V2-130) ------------------------------ */

console.log("\n[3/4] single-column phone layout (V2-130)");
{
  const { context, page, errors } = await signedInPageWithRetry(browser, { width: 360, height: 640 }, true);
  // Single column means no two block-level siblings share a horizontal band.
  const sideBySide = await page.evaluate(() => {
    const main = document.querySelector("main");
    if (!main) return ["no <main>"];
    const kids = [...main.children].map((element) => element.getBoundingClientRect());
    const overlaps = [];
    for (let i = 0; i < kids.length; i += 1) {
      for (let j = i + 1; j < kids.length; j += 1) {
        const a = kids[i];
        const b = kids[j];
        const verticalOverlap = Math.min(a.bottom, b.bottom) - Math.max(a.top, b.top);
        const horizontallyApart = a.right <= b.left + 1 || b.right <= a.left + 1;
        if (verticalOverlap > 4 && horizontallyApart) overlaps.push(`${i}/${j}`);
      }
    }
    return overlaps;
  });
  check(sideBySide.length === 0,
    `360px: main children stack in one column${sideBySide.length ? ` (side-by-side: ${sideBySide.join(", ")})` : ""}`);
  check(errors.length === 0, "360px: no page errors");
  await context.close();
}

/* --- 4. Focus order (V2-133) --------------------------------------------- */

console.log("\n[4/4] focus order equals DOM order (V2-133)");
{
  const { context, page, errors } = await signedInPageWithRetry(browser, { width: 1280, height: 800 }, false);
  // Exercise a control-rich surface: the macro editor carries the directive,
  // chord and delay toolbars, so a sparse page cannot make this pass trivially.
  const addMacro = page.getByRole("button", { name: /^add macro$/i });
  if (await addMacro.count()) {
    await addMacro.click();
    await page.waitForTimeout(1500);
  }
  const domOrder = await page.evaluate(() =>
    [...document.querySelectorAll("a[href], button, input, select, textarea, [tabindex]")]
      .filter((element) => {
        const style = getComputedStyle(element);
        return (
          !element.hasAttribute("disabled") &&
          element.getAttribute("tabindex") !== "-1" &&
          style.display !== "none" &&
          style.visibility !== "hidden" &&
          element.getBoundingClientRect().width > 0
        );
      })
      .map((element, index) => {
        element.dataset.focusProbe = String(index);
        return index;
      }),
  );
  const tabOrder = [];
  await page.evaluate(() => document.body.focus());
  for (let index = 0; index < domOrder.length; index += 1) {
    await page.keyboard.press("Tab");
    const probe = await page.evaluate(() => document.activeElement?.dataset?.focusProbe ?? null);
    if (probe === null) break;
    tabOrder.push(Number(probe));
  }
  const monotonic = tabOrder.every((value, index) => index === 0 || value > tabOrder[index - 1]);
  check(tabOrder.length > 0, `focus order: reached ${tabOrder.length} of ${domOrder.length} focusable elements`);
  check(monotonic, "focus order: tab sequence follows DOM order without jumping backwards");
  const positiveTabIndex = await page.evaluate(
    () => [...document.querySelectorAll("[tabindex]")].filter((e) => Number(e.getAttribute("tabindex")) > 0).length,
  );
  check(positiveTabIndex === 0, `focus order: no positive tabindex (found ${positiveTabIndex})`);
  check(errors.length === 0, "focus order: no page errors");
  await context.close();
}

await browser.close();

console.log("");
if (failures.length) {
  console.log(`desktop matrix: FAIL (${failures.length})`);
  for (const failure of failures) console.log(`  - ${failure}`);
  process.exit(1);
}
console.log("desktop matrix: PASS");
