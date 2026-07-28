# Phase 17 frontend foundation failure

- Transform: success
- Cleanup: success
- Node setup: success
- Frontend dependencies: success
- Format: success
- Frontend validation: failure
- Host dependencies: skipped
- Firmware formatting/tests: skipped
- ESP-IDF install: skipped
- Authoritative gate: skipped

## phase17-foundation-transform.log

```text

Phase 17 backend session foundation applied
Phase 17 documentation evidence applied
Phase 17 authenticated frontend foundation applied (payload 7)
```

## phase17-foundation-cleanup.log

```text

```

## phase17-foundation-frontend-deps.log

```text

npm warn deprecated whatwg-encoding@3.1.1: Use @exodus/bytes instead for a more spec-conformant and faster implementation
npm warn deprecated glob@10.5.0: Old versions of glob are not supported, and contain widely publicized security vulnerabilities, which have been fixed in the current version. Please update. Support for old versions may be purchased (at exorbitant rates) by contacting i@izs.me

added 471 packages, and audited 472 packages in 1m

18 vulnerabilities (16 high, 2 critical)

To address issues that do not require attention, run:
  npm audit fix

To address all issues (including breaking changes), run:
  npm audit fix --force

Run `npm audit` for details.
npm warn allow-scripts 2 packages have install scripts not yet covered by allowScripts:
npm warn allow-scripts   @tailwindcss/oxide@4.1.11 (postinstall: node ./scripts/install.js)
npm warn allow-scripts   esbuild@0.25.12 (postinstall: node install.js)
npm warn allow-scripts
npm warn allow-scripts Run `npm approve-scripts --allow-scripts-pending` to review, or `npm approve-scripts <pkg>` to allow.
```

## phase17-foundation-format.log

```text
10:[90msrc/api/errors.ts[39m 3ms (unchanged)
16:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
51:[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
71:[90msrc/api/errors.ts[39m 3ms (unchanged)
77:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
112:[90mtests/error-banner.test.tsx[39m 3ms (unchanged)


> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 49ms (unchanged)
[90mindex.html[39m 28ms (unchanged)
[90mpackage.json[39m 4ms (unchanged)
[90mREADME.md[39m 28ms (unchanged)
src/api/client.ts 89ms
[90msrc/api/errors.ts[39m 3ms (unchanged)
src/api/guards.ts 27ms
[90msrc/api/README.md[39m 2ms (unchanged)
src/api/routes.ts 13ms
src/App.tsx 27ms
[90msrc/components/AppShell.tsx[39m 9ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 1ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
src/features/auth/LoginPage.tsx 8ms
[90msrc/features/auth/README.md[39m 3ms (unchanged)
src/features/auth/SessionBoundary.tsx 11ms
src/features/auth/SetupPage.tsx 14ms
src/features/execution/ExecutionPage.tsx 9ms
src/features/execution/executionResult.ts 3ms
src/features/execution/ExecutionResultPage.tsx 4ms
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/README.md[39m 3ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 2ms (unchanged)
src/features/sets/SetSelectionPage.tsx 14ms
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 6ms (unchanged)
[90msrc/main.tsx[39m 2ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 1ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 3ms (unchanged)
[90msrc/styles.css[39m 37ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 6ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 1ms (unchanged)
tests/api.test.ts 24ms
tests/app-auth.test.tsx 10ms
tests/app-execution.test.tsx 15ms
tests/app-routing.test.tsx 10ms
tests/app-sets.test.tsx 13ms
[90mtests/app.test.ts[39m 2ms (unchanged)
[90mtests/appFixtures.ts[39m 5ms (unchanged)
[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
[90mtests/fakeFetch.ts[39m 10ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
tests/guards.test.ts 3ms
[90mtests/README.md[39m 4ms (unchanged)
[90mtests/render.tsx[39m 10ms (unchanged)
tests/setup.ts 7ms
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)

> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 37ms (unchanged)
[90mindex.html[39m 22ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 23ms (unchanged)
[90msrc/api/client.ts[39m 72ms (unchanged)
[90msrc/api/errors.ts[39m 3ms (unchanged)
[90msrc/api/guards.ts[39m 28ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 13ms (unchanged)
[90msrc/App.tsx[39m 28ms (unchanged)
[90msrc/components/AppShell.tsx[39m 11ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 2ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 8ms (unchanged)
[90msrc/features/auth/README.md[39m 3ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 12ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 14ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 8ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 2ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 3ms (unchanged)
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/README.md[39m 2ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 2ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 13ms (unchanged)
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 7ms (unchanged)
[90msrc/main.tsx[39m 1ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 1ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 2ms (unchanged)
[90msrc/styles.css[39m 32ms (unchanged)
[90msrc/types/limits.ts[39m 1ms (unchanged)
[90msrc/types/models.ts[39m 5ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 1ms (unchanged)
[90mtests/api.test.ts[39m 22ms (unchanged)
[90mtests/app-auth.test.tsx[39m 10ms (unchanged)
[90mtests/app-execution.test.tsx[39m 14ms (unchanged)
[90mtests/app-routing.test.tsx[39m 9ms (unchanged)
[90mtests/app-sets.test.tsx[39m 14ms (unchanged)
[90mtests/app.test.ts[39m 2ms (unchanged)
[90mtests/appFixtures.ts[39m 5ms (unchanged)
[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
[90mtests/fakeFetch.ts[39m 8ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
[90mtests/guards.test.ts[39m 3ms (unchanged)
[90mtests/README.md[39m 4ms (unchanged)
[90mtests/render.tsx[39m 8ms (unchanged)
[90mtests/setup.ts[39m 5ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)
```

## phase17-foundation-frontend.log

```text
77:      JSON.stringify({ expectedRevision: settings.revision }),
97:        error: {
170:    throw new Error("Missing native value setter.");
219:  expectedType: ElementConstructor<T>,
223:    throw new Error(`Missing element: ${selector}`);
225:  if (!(element instanceof expectedType)) {
226:    throw new Error(`Element ${selector} has an unexpected type.`);
236:    throw new Error(`Missing button: ${text}`);
294:    error: state === "failed" ? "press_failed" : "",
295:    releaseError: "",
307:      error: {
318:      error: {
337:      error: {
413:[31m     → expected undefined to be '/api/v1/settings' // Object.is equality[39m
415: [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 44[2mms[22m[39m
423:[31m[1mAssertionError[22m: expected undefined to be '/api/v1/settings' // Object.is equality[39m
442:[31m[1mError[22m: 1 planned fetch call(s) were not consumed.[39m
446:    [90m 95| [39m    [35mthrow[39m [35mnew[39m [33mError[39m(
461:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/app-sets.test.tsx,title=tests/app-sets.test.tsx > server-backed set selection > persists real settings updates,line=138,column=23::AssertionError: expected undefined to be '/api/v1/settings' // Object.is equality%0A%0A- Expected: %0A"/api/v1/settings"%0A%0A+ Received: %0Aundefined%0A%0A ❯ tests/app-sets.test.tsx:138:23%0A%0A
463:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/fakeFetch.ts,title=tests/app-sets.test.tsx > server-backed set selection > persists real settings updates,line=95,column=11::Error: 1 planned fetch call(s) were not consumed.%0A ❯ assertNoPendingFetchPlans tests/fakeFetch.ts:95:11%0A ❯ tests/setup.ts:45:5%0A%0A

--- app-sets.test.tsx lines 1-110 ---
import { describe, expect, test } from "vitest";
import App from "../src/App";
import {
  macroSet,
  planAuthenticatedBootstrap,
  setId,
  settings,
} from "./appFixtures";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setCheckboxChecked,
  setInputValue,
} from "./render";

const secondSet = {
  ...macroSet,
  id: "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
  name: "Workshop desktop workflow",
  description: "Desktop imaging",
  manufacturer: "Other",
  model: "Desktop 2",
  board: "desktop-board",
  sort_order: 1,
};

describe("server-backed set selection", () => {
  test("shows live metadata and filters by search", async () => {
    setHashSilently("/sets");
    planAuthenticatedBootstrap({
      activeSetId: null,
      sets: [macroSet, secondSet],
    });
    const view = await render(<App />);
    await flushReact();

    expect(document.body.textContent).toContain("Lab Chromebook workflow");
    expect(document.body.textContent).toContain("board-14");
    expect(document.body.textContent).toContain("Workshop desktop workflow");

    await setInputValue(
      requiredElement("#set-search", HTMLInputElement),
      "desktop-board",
    );
    expect(document.body.textContent).not.toContain("Lab Chromebook workflow");
    expect(document.body.textContent).toContain("Workshop desktop workflow");
    await view.unmount();
  });

  test("selects a set with the settings revision and updates the header", async () => {
    setHashSilently("/sets");
    planAuthenticatedBootstrap({ activeSetId: null });
    const view = await render(<App />);
    await flushReact();

    planJsonResponse({
      ok: true,
      data: {
        ...settings,
        revision: settings.revision + 1,
        activeSetId: setId,
      },
    });
    await click(buttonWithText("Use this set"));
    await flushReact();

    const call = getFetchCalls()[5];
    expect(call?.url).toBe(`/api/v1/sets/${setId}/select`);
    expect(call?.method).toBe("POST");
    expect(call?.body).toBe(
      JSON.stringify({ expectedRevision: settings.revision }),
    );
    expect(document.querySelector(".app-header")?.textContent).toContain(
      "Lab Chromebook workflow",
    );
    expect(
      window.localStorage.getItem("esp32-macro-keyboard.recent-set-ids"),
    ).toContain(setId);
    await view.unmount();
  });

  test("shows revision conflicts instead of silently accepting selection", async () => {
    setHashSilently("/sets");
    planAuthenticatedBootstrap({ activeSetId: null });
    const view = await render(<App />);
    await flushReact();

    planJsonResponse(
      {
        ok: false,
        error: {
          code: "conflict",
          message: "Settings revision is stale.",
        },
      },
      409,
    );
    await click(buttonWithText("Use this set"));
    await flushReact();

    expect(
      requiredElement("[role='alert']", HTMLElement).textContent,
    ).toContain("conflict: Settings revision is stale.");
    expect(document.querySelector(".app-header")?.textContent).toContain(
      "No active macro set",
--- render.tsx ---
import { act, type ReactNode } from "react";
import { createRoot, type Root } from "react-dom/client";

export interface RenderResult {
  container: HTMLDivElement;
  rerender: (element: ReactNode) => Promise<void>;
  unmount: () => Promise<void>;
}

type ElementConstructor<T extends Element> = abstract new (
  ...arguments_: never[]
) => T;

export async function render(element: ReactNode): Promise<RenderResult> {
  const container = document.createElement("div");
  document.body.append(container);
  const root: Root = createRoot(container);

  const rerender = async (nextElement: ReactNode): Promise<void> => {
    await act(async () => {
      root.render(nextElement);
      await Promise.resolve();
    });
  };

  await rerender(element);
  return {
    container,
    rerender,
    unmount: async (): Promise<void> => {
      await act(async () => {
        root.unmount();
        await Promise.resolve();
      });
      container.remove();
    },
  };
}

export async function flushReact(): Promise<void> {
  await act(async () => {
    await Promise.resolve();
    await Promise.resolve();
    await Promise.resolve();
  });
}

function setNativeValue(
  element: HTMLInputElement | HTMLTextAreaElement,
  value: string,
): void {
  const prototype =
    element instanceof HTMLInputElement
      ? HTMLInputElement.prototype
      : HTMLTextAreaElement.prototype;
  const descriptor = Object.getOwnPropertyDescriptor(prototype, "value");
  if (descriptor?.set === undefined) {
    throw new Error("Missing native value setter.");
  }
  descriptor.set.call(element, value);
}

export async function setInputValue(
  element: HTMLInputElement | HTMLTextAreaElement,
  value: string,
): Promise<void> {
  await act(async () => {
    setNativeValue(element, value);
    element.dispatchEvent(new Event("input", { bubbles: true }));
    element.dispatchEvent(new Event("change", { bubbles: true }));
    await Promise.resolve();
  });
}

export async function setCheckboxChecked(
  element: HTMLInputElement,
  checked: boolean,
): Promise<void> {
  await act(async () => {
    element.checked = checked;
    element.dispatchEvent(new Event("input", { bubbles: true }));
    element.dispatchEvent(new Event("change", { bubbles: true }));
    await Promise.resolve();
  });
}

export async function click(element: HTMLElement): Promise<void> {
  await act(async () => {
    element.dispatchEvent(
      new MouseEvent("click", { bubbles: true, cancelable: true }),
    );
    await Promise.resolve();
  });
}

export async function submit(form: HTMLFormElement): Promise<void> {
  await act(async () => {
    form.dispatchEvent(
      new SubmitEvent("submit", { bubbles: true, cancelable: true }),
    );
    await Promise.resolve();
  });
}

export function requiredElement<T extends Element>(
  selector: string,
  expectedType: ElementConstructor<T>,
): T {
  const element = document.querySelector(selector);
  if (element === null) {
    throw new Error(`Missing element: ${selector}`);
  }
  if (!(element instanceof expectedType)) {
    throw new Error(`Element ${selector} has an unexpected type.`);
  }
  return element;
}

export function buttonWithText(text: string): HTMLButtonElement {
  const button = Array.from(document.querySelectorAll("button")).find(
    (candidate) => candidate.textContent?.trim() === text,
  );
  if (button === undefined) {
    throw new Error(`Missing button: ${text}`);
  }
  return button;
}
--- appFixtures.ts ---
import { planJsonResponse } from "./fakeFetch";

export const setId = "11111111-1111-4111-8111-111111111111";
export const macroId = "22222222-2222-4222-8222-222222222222";
export const executionId = "33333333-3333-4333-8333-333333333333";

export const deviceStatus = {
  version: "0.1.0",
  idf: "v5.5.5",
  usbState: "ready",
  wifiState: "started",
  wifiClients: 1,
  executionState: "idle",
} as const;

export const settings = {
  schemaVersion: 1,
  revision: 4,
  requirePhysicalConfirmation: true,
  alwaysSelectSet: true,
  activeSetId: setId,
} as const;

export const macroSet = {
  schema_version: 1,
  id: setId,
  revision: 2,
  name: "Lab Chromebook workflow",
  description: "ChromeOS conversion and Debian installation",
  manufacturer: "Example",
  model: "Model 14",
  board: "board-14",
  keyboard_layout: "en-US",
  sort_order: 0,
} as const;

export function executionStatus(
  state:
    | "idle"
    | "running"
    | "completed"
    | "cancelled"
    | "failed"
    | "timed_out",
  actionIndex = 2,
  actionCount = 5,
): object {
  return {
    executionId,
    setId,
    macroId,
    macroRevision: 7,
    state,
    error: state === "failed" ? "press_failed" : "",
    releaseError: "",
    actionIndex,
    actionCount,
    available: true,
    cancellationRequested: false,
  };
}

export function planNormalUnauthenticatedBootstrap(): void {
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
  planJsonResponse({ ok: true, data: deviceStatus });
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
}

export function planAuthenticatedBootstrap(
  overrides: {
    activeSetId?: string | null;
    sets?: readonly object[];
    usbState?: string;
  } = {},
): void {
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
  planJsonResponse({
    ok: true,
    data: {
      ...deviceStatus,
      usbState: overrides.usbState ?? deviceStatus.usbState,
    },
  });
  planJsonResponse({
    ok: true,
    data: { authenticated: true, csrfToken: "csrf-restored" },
  });
  planJsonResponse({
    ok: true,
    data: {
      ...settings,
      activeSetId:
        overrides.activeSetId === undefined
          ? settings.activeSetId
          : overrides.activeSetId,
    },
  });
  planJsonResponse({
    ok: true,
    data: overrides.sets ?? [macroSet],
  });
}

export function planPostLoginBootstrap(): void {
  planJsonResponse({
    ok: true,
    data: { csrfToken: "csrf-123" },
  });
  planJsonResponse({ ok: true, data: deviceStatus });
  planJsonResponse({ ok: true, data: settings });
  planJsonResponse({ ok: true, data: [macroSet] });
}

> esp32-macro-keyboard-webapp@0.1.0 typecheck
> tsc -b --pretty false


> esp32-macro-keyboard-webapp@0.1.0 lint
> eslint . --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 stylelint
> stylelint 'src/**/*.css' --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 format:check
> prettier --check .

Checking formatting...
All matched files use Prettier code style!

> esp32-macro-keyboard-webapp@0.1.0 test
> vitest run


[1m[46m RUN [49m[22m [36mv3.2.4 [39m[90m/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp[39m

 [32m✓[39m tests/api.test.ts [2m([22m[2m20 tests[22m[2m)[22m[32m 50[2mms[22m[39m
 [32m✓[39m tests/app-execution.test.tsx [2m([22m[2m11 tests[22m[2m)[22m[32m 145[2mms[22m[39m
 [32m✓[39m tests/app-auth.test.tsx [2m([22m[2m7 tests[22m[2m)[22m[32m 168[2mms[22m[39m
 [31m❯[39m tests/app-sets.test.tsx [2m([22m[2m4 tests[22m[2m | [22m[31m1 failed[39m[2m)[22m[32m 122[2mms[22m[39m
   [32m✓[39m server-backed set selection[2m > [22mshows live metadata and filters by search[32m 59[2mms[22m[39m
   [32m✓[39m server-backed set selection[2m > [22mselects a set with the settings revision and updates the header[32m 24[2mms[22m[39m
   [32m✓[39m server-backed set selection[2m > [22mshows revision conflicts instead of silently accepting selection[32m 19[2mms[22m[39m
[31m   [31m×[31m server-backed set selection[2m > [22mpersists real settings updates[39m[32m 18[2mms[22m[39m
[31m     → expected undefined to be '/api/v1/settings' // Object.is equality[39m
[31m     → 1 planned fetch call(s) were not consumed.[39m
 [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 44[2mms[22m[39m
 [32m✓[39m tests/app-routing.test.tsx [2m([22m[2m21 tests[22m[2m)[22m[32m 191[2mms[22m[39m
 [32m✓[39m tests/guards.test.ts [2m([22m[2m3 tests[22m[2m)[22m[32m 12[2mms[22m[39m
 [32m✓[39m tests/app.test.ts [2m([22m[2m1 test[22m[2m)[22m[32m 9[2mms[22m[39m

[31m⎯⎯⎯⎯⎯⎯⎯[39m[1m[41m Failed Tests 1 [49m[22m[31m⎯⎯⎯⎯⎯⎯⎯[39m

[41m[1m FAIL [22m[49m tests/app-sets.test.tsx[2m > [22mserver-backed set selection[2m > [22mpersists real settings updates
[31m[1mAssertionError[22m: expected undefined to be '/api/v1/settings' // Object.is equality[39m

[32m- Expected:[39m
"/api/v1/settings"

[31m+ Received:[39m
undefined

[36m [2m❯[22m tests/app-sets.test.tsx:[2m138:23[22m[39m
    [90m136| [39m
    [90m137| [39m    [35mconst[39m call [33m=[39m [34mgetFetchCalls[39m()[[34m5[39m][33m;[39m
    [90m138| [39m    [34mexpect[39m(call[33m?.[39murl)[33m.[39m[34mtoBe[39m([32m"/api/v1/settings"[39m)[33m;[39m
    [90m   | [39m                      [31m^[39m
    [90m139| [39m    [34mexpect[39m(call[33m?.[39mmethod)[33m.[39m[34mtoBe[39m([32m"PUT"[39m)[33m;[39m
    [90m140| [39m    [34mexpect[39m(call[33m?.[39mbody)[33m.[39m[34mtoBe[39m(

[31m[2m⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[1/2]⎯[22m[39m

[41m[1m FAIL [22m[49m tests/app-sets.test.tsx[2m > [22mserver-backed set selection[2m > [22mpersists real settings updates
[31m[1mError[22m: 1 planned fetch call(s) were not consumed.[39m
[36m [2m❯[22m assertNoPendingFetchPlans tests/fakeFetch.ts:[2m95:11[22m[39m
    [90m 93| [39m[35mexport[39m [35mfunction[39m [34massertNoPendingFetchPlans[39m()[33m:[39m [35mvoid[39m {
    [90m 94| [39m  [35mif[39m (handlers[33m.[39mlength [33m!==[39m [34m0[39m) {
    [90m 95| [39m    [35mthrow[39m [35mnew[39m [33mError[39m(
    [90m   | [39m          [31m^[39m
    [90m 96| [39m      `${String(handlers.length)} planned fetch call(s) were not consu…
    [90m 97| [39m    )[33m;[39m
[90m [2m❯[22m tests/setup.ts:[2m45:5[22m[39m

[31m[2m⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[2/2]⎯[22m[39m


[2m Test Files [22m [1m[31m1 failed[39m[22m[2m | [22m[1m[32m7 passed[39m[22m[90m (8)[39m
[2m      Tests [22m [1m[31m1 failed[39m[22m[2m | [22m[1m[32m69 passed[39m[22m[90m (70)[39m
[2m   Start at [22m 23:15:09
[2m   Duration [22m 2.57s[2m (transform 271ms, setup 189ms, collect 671ms, tests 742ms, environment 3.46s, prepare 639ms)[22m


::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/app-sets.test.tsx,title=tests/app-sets.test.tsx > server-backed set selection > persists real settings updates,line=138,column=23::AssertionError: expected undefined to be '/api/v1/settings' // Object.is equality%0A%0A- Expected: %0A"/api/v1/settings"%0A%0A+ Received: %0Aundefined%0A%0A ❯ tests/app-sets.test.tsx:138:23%0A%0A

::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/fakeFetch.ts,title=tests/app-sets.test.tsx > server-backed set selection > persists real settings updates,line=95,column=11::Error: 1 planned fetch call(s) were not consumed.%0A ❯ assertNoPendingFetchPlans tests/fakeFetch.ts:95:11%0A ❯ tests/setup.ts:45:5%0A%0A
```
