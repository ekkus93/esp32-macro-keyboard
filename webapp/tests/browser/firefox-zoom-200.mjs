import { execFileSync } from "node:child_process";
import { mkdir } from "node:fs/promises";
import process from "node:process";
import { join } from "node:path";
import { firefox } from "playwright";

import { startApplicationServer } from "./fixtures/applicationServer.mjs";
import { assert } from "./lib/http.mjs";
import { waitFor } from "./lib/page.mjs";
import { SCREENS } from "./workflows/visibleControls.mjs";

const artifactDirectory = process.env.UXR_FIREFOX_ZOOM_OUTPUT ?? "/artifacts";
const zoomMode = process.env.UXR_FIREFOX_ZOOM_MODE ?? "browser";
assert(
  zoomMode === "browser" || zoomMode === "text-only",
  `Unsupported Firefox zoom mode: ${zoomMode}`,
);
const textOnlyZoom = zoomMode === "text-only";
await mkdir(artifactDirectory, { recursive: true });

const application = await startApplicationServer();
const browser = await firefox.launch({
  headless: false,
  firefoxUserPrefs: { "browser.zoom.full": !textOnlyZoom },
});
let context;

try {
  context = await browser.newContext({ viewport: null });
  const page = await context.newPage();
  await page.goto(application.baseUrl);
  await waitFor(
    page,
    () => document.body.innerText.includes("Lab bench workflow"),
    "Macros did not render.",
  );

  const windowId = execFileSync(
    "bash",
    ["-lc", "xdotool search --onlyvisible --class firefox | head -n 1"],
    { encoding: "utf8" },
  ).trim();
  assert(windowId.length > 0, "Could not find the visible Firefox window.");
  execFileSync("xdotool", ["windowfocus", "--sync", windowId]);
  execFileSync("xdotool", ["windowsize", "--sync", windowId, "640", "1236"]);
  await page.waitForFunction(() => innerWidth > 600 && innerWidth <= 650);

  const beforeZoom = await page.evaluate(() => ({
    width: innerWidth,
    height: innerHeight,
    devicePixelRatio,
    headingFontSize: Number.parseFloat(
      getComputedStyle(document.querySelector("header h1")).fontSize,
    ),
    headingHeight: document.querySelector("header h1").getBoundingClientRect()
      .height,
  }));
  let afterZoom = beforeZoom;
  for (let step = 0; step < 12; step += 1) {
    execFileSync("xdotool", ["key", "--clearmodifiers", "ctrl+plus"]);
    await page.waitForTimeout(250);
    afterZoom = await page.evaluate(() => ({
      width: innerWidth,
      height: innerHeight,
      devicePixelRatio,
      headingFontSize: Number.parseFloat(
        getComputedStyle(document.querySelector("header h1")).fontSize,
      ),
      headingHeight: document.querySelector("header h1").getBoundingClientRect()
        .height,
    }));
    const zoomReached = textOnlyZoom
      ? afterZoom.headingHeight >= beforeZoom.headingHeight * 1.95
      : afterZoom.devicePixelRatio >= 1.95;
    if (zoomReached) {
      break;
    }
  }

  console.log(
    JSON.stringify({
      browser: await browser.version(),
      beforeZoom,
      afterZoom,
    }),
  );
  assert(
    textOnlyZoom
      ? afterZoom.headingHeight >= beforeZoom.headingHeight * 1.95 &&
          Math.abs(afterZoom.width - beforeZoom.width) <= 1 &&
          Math.abs(afterZoom.devicePixelRatio - beforeZoom.devicePixelRatio) <=
            0.01
      : afterZoom.devicePixelRatio >= 1.95 &&
          afterZoom.devicePixelRatio <= 2.05 &&
          afterZoom.width >= 300 &&
          afterZoom.width <= 330,
    `Expected actual 200% ${zoomMode} zoom; before=${JSON.stringify(beforeZoom)}, after=${JSON.stringify(afterZoom)}`,
  );

  const assertNoHorizontalOverflow = async (where) => {
    const dimensions = await page.evaluate(() => ({
      width: innerWidth,
      documentWidth: document.documentElement.scrollWidth,
      bodyWidth: document.body.scrollWidth,
    }));
    assert(
      dimensions.documentWidth <= dimensions.width &&
        dimensions.bodyWidth <= dimensions.width,
      `${where} overflows horizontally: ${JSON.stringify(dimensions)}`,
    );
  };

  const assertInsideViewport = async (locator, label, where) => {
    const target = locator.first();
    const box = await target.boundingBox();
    assert(box !== null, `${where}: ${label} is not rendered.`);
    assert(
      box.x >= 0 &&
        box.y >= 0 &&
        box.x + box.width <= afterZoom.width + 1 &&
        box.y + box.height <= afterZoom.height + 1,
      `${where}: ${label} is outside ${afterZoom.width}x${afterZoom.height}: ${JSON.stringify(box)}`,
    );
    const covered = await target.evaluate((element) => {
      const rect = element.getBoundingClientRect();
      const top = document.elementFromPoint(
        rect.x + rect.width / 2,
        rect.y + rect.height / 2,
      );
      return top === null || !(element.contains(top) || top.contains(element));
    });
    assert(!covered, `${where}: ${label} is covered by another element.`);
  };

  for (const screen of SCREENS) {
    await page.evaluate((route) => {
      window.location.hash = `/${route}`;
    }, screen.route);
    await waitFor(
      page,
      new Function(
        `return document.body.innerText.includes(${JSON.stringify(screen.ready)});`,
      ),
      `${screen.route} did not render at 200% zoom.`,
    );

    if (screen.name === "key picker") {
      const opener = page.getByRole("button", {
        name: "Keys and combinations",
        exact: true,
      });
      await opener.focus();
      await page.keyboard.press("Enter");
      await waitFor(
        page,
        () => document.body.innerText.includes("Insert into macro"),
        "Key picker did not open by keyboard at 200% zoom.",
      );
    }

    if (textOnlyZoom && screen.route === "macros") {
      await page.screenshot({
        path: join(artifactDirectory, "macros-text-only-200-percent.png"),
      });
    }

    const where = `${screen.name ?? screen.route} @ Firefox 200% ${zoomMode}, ${afterZoom.width}x${afterZoom.height} CSS px`;
    await assertNoHorizontalOverflow(where);
    await assertInsideViewport(page.locator("header h1"), "page title", where);
    for (const control of screen.controls) {
      if (
        control.minHeight !== undefined &&
        afterZoom.height < control.minHeight
      ) {
        continue;
      }
      if (textOnlyZoom) {
        await control.find(page).first().scrollIntoViewIfNeeded();
      }
      await assertInsideViewport(control.find(page), control.label, where);
    }

    if (screen.route === "macros") {
      await page.screenshot({
        path: join(
          artifactDirectory,
          textOnlyZoom
            ? "macros-text-only-200-percent.png"
            : "macros-200-percent-320css.png",
        ),
      });
    }
    if (screen.route === "macro-editor" && screen.name === undefined) {
      await page.screenshot({
        path: join(
          artifactDirectory,
          textOnlyZoom
            ? "macro-editor-text-only-200-percent.png"
            : "macro-editor-200-percent-320css.png",
        ),
      });
    }
  }

  console.log(
    `Firefox ${SCREENS.length} responsive-control screen states passed at 200% ${zoomMode} zoom (${afterZoom.width}x${afterZoom.height} CSS px), including title/control bounds, occlusion, document overflow, and keyboard-opened key picker.`,
  );
} finally {
  if (context) {
    await context.close();
  }
  await browser.close();
  await application.close();
}
