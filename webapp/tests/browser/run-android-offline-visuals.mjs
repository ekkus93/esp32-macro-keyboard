import net from "node:net";
import process from "node:process";
import { chromium } from "playwright";

import { SCENARIOS } from "./visual/scenarios.mjs";

const cdpUrl = process.env.ANDROID_CDP_URL ?? "http://127.0.0.1:9222";
const fixturePort = Number(process.env.ANDROID_FIXTURE_PORT ?? "39371");
const originalListen = net.Server.prototype.listen;

// Android Chrome CDP does not support isolated BrowserContexts. Run scenarios
// sequentially in its default context and reuse the fixture's fixed host port
// through `adb reverse`.
net.Server.prototype.listen = function patchedListen(...args) {
  if (args[0] === 0 && args[1] === "127.0.0.1") {
    args[0] = fixturePort;
  }
  return originalListen.apply(this, args);
};

const browser = await chromium.connectOverCDP(cdpUrl);
const deviceContext = browser.contexts()[0];
if (deviceContext === undefined) {
  throw new Error(`No Android Chrome browser context at ${cdpUrl}.`);
}

let activePage;
const deviceBrowser = {
  async newContext(options = {}) {
    activePage = await deviceContext.newPage();
    if (options.viewport !== undefined) {
      await activePage.setViewportSize(options.viewport);
    }
    if (options.colorScheme !== undefined) {
      await activePage.emulateMedia({ colorScheme: options.colorScheme });
    }
    activePage.screenshot = async () => Buffer.alloc(0);
    const page = activePage;
    return {
      newPage: async () => page,
      close: async () => {
        if (!page.isClosed()) {
          await page.close();
        }
      },
    };
  },
};

try {
  for (const scenario of SCENARIOS) {
    const viewport = scenario.viewports[0];
    await scenario.run(deviceBrowser, viewport);
    console.log(`PASS ${scenario.name} @ ${viewport.name}`);
  }
  console.log(
    `Android Chrome offline route matrix passed (${SCENARIOS.length} scenarios).`,
  );
} finally {
  if (activePage !== undefined && !activePage.isClosed()) {
    await activePage.close();
  }
  net.Server.prototype.listen = originalListen;
  await browser.close();
}
