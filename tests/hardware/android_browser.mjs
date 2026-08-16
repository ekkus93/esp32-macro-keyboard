/**
 * Drive real Chrome on a real Android phone against the real device (V2-155).
 *
 * The phone is the *client* under test here; the ESP32 is just the server it
 * talks to. Both sit on the same LAN, so nothing joins the device SoftAP and
 * this host's Wi-Fi is never touched.
 *
 * Connection path: `adb forward` exposes Chrome's DevTools socket on
 * 127.0.0.1:9222, and Playwright attaches over CDP. Chrome must already be
 * running with a page open on the device.
 *
 * Two things about driving a phone this way are not obvious, and both cost real
 * debugging time:
 *
 *   * Playwright's own `click()` does not land reliably. At devicePixelRatio 4
 *     the computed point misses and the log reports a different intercepting
 *     element on each retry. `tap()` below instead issues a real
 *     `adb shell input tap`, which also proves the control is genuinely
 *     tappable rather than merely present in the DOM.
 *
 *   * `adb forward` lapses between runs, so {@link attach} re-establishes it
 *     every time rather than failing with ECONNREFUSED.
 */
import { execFileSync } from "node:child_process";
import { createRequire } from "node:module";

// tests/hardware/ has no node_modules of its own; Playwright lives in the
// webapp install. Resolve it relative to this file so the script runs from any
// working directory.
const require = createRequire(new URL("../../webapp/package.json", import.meta.url));
const { chromium } = require("playwright");

export const DEFAULT_SERIAL = process.env.HIL_ANDROID_SERIAL ?? "LGH87250967ab9";
export const DEFAULT_ORIGIN = process.env.HIL_DEVICE_ORIGIN ?? "http://192.168.88.108";

export function adb(serial, ...args) {
  return execFileSync("adb", ["-s", serial, ...args], { encoding: "utf8" });
}

function ensureForward(serial) {
  try {
    adb(serial, "forward", "--remove-all");
  } catch {
    /* nothing to remove */
  }
  adb(serial, "forward", "tcp:9222", "localabstract:chrome_devtools_remote");
}

/** Attach to the page showing `origin`, re-establishing the CDP forward first. */
export async function attach(serial = DEFAULT_SERIAL, origin = DEFAULT_ORIGIN) {
  ensureForward(serial);
  const browser = await chromium.connectOverCDP("http://127.0.0.1:9222");
  const context = browser.contexts()[0];
  const host = new URL(origin).host;
  const page = context.pages().find((p) => p.url().includes(host)) ?? context.pages()[0];
  const errors = [];
  page.on("pageerror", (error) => errors.push(error.message));
  return { browser, page, errors, serial };
}

export async function bodyText(page, limit = 900) {
  return (await page.evaluate(() => document.body.innerText)).slice(0, limit);
}

/**
 * Map CSS coordinates to device pixels.
 *
 * Web content starts below Chrome's toolbar, so a CSS point must be scaled by
 * devicePixelRatio and shifted down by that band. The band is read from the
 * accessibility tree rather than assumed. Calibrating on content *height* does
 * not work: Chrome does not shrink `innerHeight` when the IME opens, so the
 * visible region and the layout viewport disagree while a keyboard is up.
 */
export async function contentOffset(page, serial = DEFAULT_SERIAL) {
  const dpr = await page.evaluate(() => window.devicePixelRatio);
  adb(serial, "shell", "uiautomator", "dump", "/sdcard/ui-calib.xml");
  const xml = adb(serial, "shell", "cat", "/sdcard/ui-calib.xml");
  let band = 0;
  for (const match of xml.matchAll(/<node[^>]*>/g)) {
    const tag = match[0];
    if (!/resource-id="[^"]*(toolbar|url_bar)[^"]*"/.test(tag)) continue;
    const bounds = /bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"/.exec(tag);
    if (bounds) band = Math.max(band, Number(bounds[4]));
  }
  if (band === 0) throw new Error("could not locate Chrome's toolbar to calibrate taps");
  return { dpr, band };
}

/**
 * A real finger tap at the centre of `locator`.
 *
 * The element is centred first, not merely scrolled "into view": the sticky
 * bottom navigation overlays the lower ~70 CSS px, so a technically-visible
 * control can still sit underneath it and the tap would hit the nav instead.
 */
export async function tap(page, locator, offset, serial = DEFAULT_SERIAL) {
  await locator.scrollIntoViewIfNeeded();
  await locator.evaluate((element) => {
    element.scrollIntoView({ block: "center", inline: "center" });
  });
  await page.waitForTimeout(350);
  const box = await locator.boundingBox();
  if (!box) throw new Error("element has no bounding box");
  const x = Math.round((box.x + box.width / 2) * offset.dpr);
  const y = Math.round(offset.band + (box.y + box.height / 2) * offset.dpr);
  adb(serial, "shell", "input", "tap", String(x), String(y));
  return { x, y, box };
}

/**
 * Close the IME, but only when it is actually open.
 *
 * BACK closes the keyboard when one is showing; with no keyboard up it is an
 * ordinary back press, which dismisses whatever panel is open. Sending it
 * unconditionally silently cancelled a delete-confirmation panel mid-test.
 */
export function hideKeyboard(serial = DEFAULT_SERIAL) {
  try {
    const shown = adb(serial, "shell", "dumpsys", "input_method")
      .split("\n")
      .some((line) => /mInputShown=true/.test(line));
    if (shown) adb(serial, "shell", "input", "keyevent", "4");
  } catch {
    /* best effort */
  }
}

/** Rotate the display. `landscape` locks user_rotation to 90 degrees. */
export function setOrientation(landscape, serial = DEFAULT_SERIAL) {
  adb(serial, "shell", "settings", "put", "system", "accelerometer_rotation", "0");
  adb(serial, "shell", "settings", "put", "system", "user_rotation", landscape ? "1" : "0");
}

/** Open `origin` in Chrome, restarting the browser so the load is clean. */
export function openDevicePage(serial = DEFAULT_SERIAL, origin = DEFAULT_ORIGIN) {
  adb(serial, "shell", "am", "force-stop", "com.android.chrome");
  adb(serial, "shell", "am", "start", "-a", "android.intent.action.VIEW", "-d", origin,
      "com.android.chrome");
}
