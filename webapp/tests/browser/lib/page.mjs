import { assert } from "./http.mjs";

export async function evaluate(page, fn, arg) {
  return page.evaluate(fn, arg);
}

/**
 * Polls `fn` in the browser context via Playwright's own
 * `page.waitForFunction()` until it returns a truthy value, surfacing the
 * page's current text/hash on timeout for a debuggable failure — the same
 * diagnostic contract the CDP-based harness this replaces used to provide.
 */
export async function waitFor(page, fn, message, timeoutMs = 12_000) {
  try {
    await page.waitForFunction(fn, undefined, {
      timeout: timeoutMs,
      polling: 50,
    });
  } catch (error) {
    if (!(error instanceof Error) || error.name !== "TimeoutError") {
      throw error;
    }
    const text = await evaluate(page, () => document.body.innerText);
    const hash = await evaluate(page, () => window.location.hash);
    throw new Error(
      `${message}\nCurrent hash: ${String(hash)}\nCurrent page:\n${String(text)}`,
    );
  }
}

/**
 * Clicks the first button whose exact, trimmed accessible name matches
 * `text` — `page.getByRole()` is Playwright's own accessible-name locator,
 * replacing the hand-rolled `querySelectorAll('button').find(...)` scan the
 * CDP-based harness used. `.first()` preserves that scan's "first match in
 * document order wins" semantics instead of Playwright's default strict
 * mode, which would throw if more than one button shares a name.
 */
export async function clickButton(page, text) {
  await page.getByRole("button", { name: text, exact: true }).first().click();
}

/**
 * Clicks the first element whose `aria-label` exactly matches `label`.
 * `page.getByLabel()` matches the `aria-label` attribute directly (not just
 * `<label>`-associated form controls), which is exactly what this project's
 * icon-only buttons use for their accessible name.
 */
export async function clickButtonByAriaLabel(page, label) {
  await page.getByLabel(label, { exact: true }).first().click();
}

export async function assertTouchTargets(page) {
  const failures = await evaluate(page, () =>
    Array.from(
      document.querySelectorAll(
        "button:not([disabled]), a[href], input:not([disabled]), textarea:not([disabled]), select:not([disabled])",
      ),
    )
      .map((element) => {
        const target = element.matches('input[type="checkbox"]')
          ? element.closest("label")
          : element;
        const rect = target.getBoundingClientRect();
        return {
          label:
            element.getAttribute("aria-label") ||
            element.textContent.trim() ||
            element.id ||
            element.tagName,
          width: rect.width,
          height: rect.height,
        };
      })
      .filter((item) => item.width < 44 || item.height < 44),
  );
  assert(
    Array.isArray(failures) && failures.length === 0,
    `Touch targets below 44x44 CSS pixels: ${JSON.stringify(failures)}`,
  );
}

/*
 * SPEC 24.5 item: responsive mobile layout
 * SPEC 9: "The application MUST be mobile-first and usable from a desktop
 * browser", and section 24.5 requires tests to cover responsive mobile layout.
 *
 * This cannot be asserted in the vitest suite: jsdom has no box model, applies
 * no media queries, and reports every element as zero-sized, so the only thing
 * a unit test could check there is that a class name is present -- the markup,
 * not the requirement. Real Chrome with device metrics overridden is the first
 * place the requirement becomes observable.
 *
 * The failure being guarded against is the ordinary one: a fixed pixel width, a
 * long unbroken string, or a table that does not wrap, any of which pushes
 * content off the side of a 360 CSS-pixel screen. On a phone that means content
 * the user cannot reach, because the device's whole point is being operated
 * from one.
 */
export const MOBILE_VIEWPORT = {
  width: 360,
  height: 640,
  deviceScaleFactor: 2,
  mobile: true,
};
export const DESKTOP_VIEWPORT = {
  width: 1280,
  height: 800,
  deviceScaleFactor: 1,
  mobile: false,
};

export async function overflowingElements(page) {
  return evaluate(page, () => {
    const limit = document.documentElement.clientWidth;
    return Array.from(document.querySelectorAll("body *"))
      .filter((element) => {
        const style = window.getComputedStyle(element);
        if (style.display === "none" || style.visibility === "hidden") {
          return false;
        }
        const rect = element.getBoundingClientRect();
        if (rect.width === 0 && rect.height === 0) {
          return false;
        }
        return rect.right > limit + 1;
      })
      .slice(0, 5)
      .map((element) => ({
        tag: element.tagName,
        id: element.id,
        classes: typeof element.className === "string" ? element.className : "",
        right: Math.round(element.getBoundingClientRect().right),
        limit,
      }));
  });
}

export async function contentWidth(page) {
  return evaluate(page, () =>
    Math.round(
      document.querySelector("#main-content").getBoundingClientRect().width,
    ),
  );
}

export async function assertFitsViewport(page, label) {
  const scroll = await evaluate(page, () => ({
    scrollWidth: document.documentElement.scrollWidth,
    clientWidth: document.documentElement.clientWidth,
  }));
  assert(
    scroll.scrollWidth <= scroll.clientWidth + 1,
    `${label}: the page scrolls horizontally (${String(scroll.scrollWidth)} > ${String(scroll.clientWidth)}).`,
  );
  const overflowing = await overflowingElements(page);
  assert(
    Array.isArray(overflowing) && overflowing.length === 0,
    `${label}: elements extend past the right edge: ${JSON.stringify(overflowing)}`,
  );
}

/**
 * Toggles real device-metrics emulation mid-test (mobile, then desktop, then
 * cleared) via a Playwright `CDPSession` — Playwright's own supported
 * escape hatch (`page.context().newCDPSession()`) for the one thing its
 * high-level API has no equivalent for: changing `deviceScaleFactor`/
 * `mobile` on an already-open page rather than only at context-creation
 * time. This is a documented Playwright API, not the hand-rolled raw
 * WebSocket JSON-RPC transport it replaces.
 */
export async function assertResponsiveLayout(page) {
  const cdpSession = await page.context().newCDPSession(page);
  try {
    await cdpSession.send(
      "Emulation.setDeviceMetricsOverride",
      MOBILE_VIEWPORT,
    );
    await assertFitsViewport(page, "Mobile 360x640");
    /* Touch targets are re-checked here rather than trusted from the default
       window size: a narrower viewport is where controls get squeezed. */
    await assertTouchTargets(page);
    const mobileWidth = await contentWidth(page);
    assert(
      mobileWidth > 0 && mobileWidth <= MOBILE_VIEWPORT.width,
      `Mobile content width ${String(mobileWidth)} does not fit a ${String(MOBILE_VIEWPORT.width)}px viewport.`,
    );

    await cdpSession.send(
      "Emulation.setDeviceMetricsOverride",
      DESKTOP_VIEWPORT,
    );
    await assertFitsViewport(page, "Desktop 1280x800");
    const desktopWidth = await contentWidth(page);
    /* "Mobile-first and usable from a desktop browser" is two requirements. A
       layout locked to the phone width satisfies the first and fails the
       second, and a horizontal-scroll check alone would never notice. */
    assert(
      desktopWidth > mobileWidth,
      `The layout does not adapt: content is ${String(desktopWidth)}px at 1280px wide and ${String(mobileWidth)}px at 360px.`,
    );

    await cdpSession.send("Emulation.clearDeviceMetricsOverride");
  } finally {
    await cdpSession.detach();
  }
}

/**
 * TODO_V2 Phase 9 exit gate: "Macros page browser tests cover idle, USB
 * unavailable, quick send, confirmation, progress, cancel, complete,
 * failure, timeout, release error, reload, and rapid repeated input."
 *
 * Covered here against the real built app and a real gzip-compressed
 * repository blob: idle, quick send, progress, confirmation, complete,
 * cancel, failure, timeout, release error, reload, and rapid repeated
 * input (below, right after the Quick Send scenario). USB unavailable is
 * covered separately by `runUsbUnavailableWorkflow`, against its own
 * fixture server started with a non-`ready` USB state from first load,
 * since this function's shared fixture is fixed at `usb.state: "ready"`
 * for every other scenario here.
 *
 * An earlier version of this comment said rapid repeated input could not be
 * reproduced over a real browser round trip. That turned out to be wrong:
 * `MacrosPage.tsx`'s double-send guard (`startingRef`, V2-095) is a plain
 * ref set synchronously at the very top of `startSend`, before any
 * `await` -- it does not depend on React re-rendering the button's
 * `disabled` attribute or on any network round trip completing. Playwright's
 * own `locator.click()` genuinely can't reproduce a same-tick double
 * dispatch (each call is a real, separately-awaited CDP round trip), but
 * `page.evaluate()` runs its function body synchronously on the browser's
 * own JS thread, so calling `.click()` on the same button twice inside one
 * `evaluate()` invokes React's `onClick` handler twice in the same task --
 * exactly the race the guard is built to withstand.
 */
