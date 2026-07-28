# Phase 17 frontend foundation failure

- Transform: success
- Cleanup: success
- Node setup: success
- Frontend dependencies: success
- Format: success
- Frontend validation: success
- Host dependencies: success
- Firmware formatting/tests: failure
- ESP-IDF install: skipped
- Authoritative gate: skipped

## phase17-foundation-transform.log

```text

Phase 17 backend session foundation applied
Phase 17 documentation evidence applied
Phase 17 authenticated frontend foundation applied (payload 10)
```

## phase17-foundation-cleanup.log

```text

```

## phase17-foundation-frontend-deps.log

```text

npm warn deprecated whatwg-encoding@3.1.1: Use @exodus/bytes instead for a more spec-conformant and faster implementation
npm warn deprecated glob@10.5.0: Old versions of glob are not supported, and contain widely publicized security vulnerabilities, which have been fixed in the current version. Please update. Support for old versions may be purchased (at exorbitant rates) by contacting i@izs.me

added 471 packages, and audited 472 packages in 59s

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
10:[90msrc/api/errors.ts[39m 4ms (unchanged)
16:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
51:[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
71:[90msrc/api/errors.ts[39m 4ms (unchanged)
77:[90msrc/components/ErrorBanner.tsx[39m 3ms (unchanged)
112:[90mtests/error-banner.test.tsx[39m 4ms (unchanged)


> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 57ms (unchanged)
[90mindex.html[39m 29ms (unchanged)
[90mpackage.json[39m 4ms (unchanged)
[90mREADME.md[39m 28ms (unchanged)
src/api/client.ts 102ms
[90msrc/api/errors.ts[39m 4ms (unchanged)
src/api/guards.ts 32ms
[90msrc/api/README.md[39m 2ms (unchanged)
src/api/routes.ts 11ms
src/App.tsx 27ms
[90msrc/components/AppShell.tsx[39m 13ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 2ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
src/features/auth/LoginPage.tsx 8ms
[90msrc/features/auth/README.md[39m 4ms (unchanged)
src/features/auth/SessionBoundary.tsx 15ms
src/features/auth/SetupPage.tsx 16ms
src/features/execution/ExecutionPage.tsx 9ms
src/features/execution/executionResult.ts 3ms
src/features/execution/ExecutionResultPage.tsx 5ms
[90msrc/features/execution/README.md[39m 4ms (unchanged)
[90msrc/features/macros/README.md[39m 4ms (unchanged)
[90msrc/features/procedures/README.md[39m 3ms (unchanged)
[90msrc/features/README.md[39m 2ms (unchanged)
[90msrc/features/sets/README.md[39m 2ms (unchanged)
src/features/sets/SetSelectionPage.tsx 15ms
[90msrc/features/settings/README.md[39m 3ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 9ms (unchanged)
[90msrc/main.tsx[39m 3ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 2ms (unchanged)
[90msrc/pages/README.md[39m 4ms (unchanged)
[90msrc/README.md[39m 2ms (unchanged)
[90msrc/routing.ts[39m 4ms (unchanged)
[90msrc/styles.css[39m 43ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 7ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 2ms (unchanged)
tests/api.test.ts 27ms
tests/app-auth.test.tsx 14ms
tests/app-execution.test.tsx 20ms
tests/app-routing.test.tsx 10ms
tests/app-sets.test.tsx 11ms
[90mtests/app.test.ts[39m 2ms (unchanged)
[90mtests/appFixtures.ts[39m 5ms (unchanged)
[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
[90mtests/fakeFetch.ts[39m 11ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
tests/guards.test.ts 5ms
[90mtests/README.md[39m 5ms (unchanged)
[90mtests/render.tsx[39m 14ms (unchanged)
tests/setup.ts 7ms
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)

> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 42ms (unchanged)
[90mindex.html[39m 25ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 23ms (unchanged)
[90msrc/api/client.ts[39m 84ms (unchanged)
[90msrc/api/errors.ts[39m 4ms (unchanged)
[90msrc/api/guards.ts[39m 35ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 15ms (unchanged)
[90msrc/App.tsx[39m 29ms (unchanged)
[90msrc/components/AppShell.tsx[39m 13ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 3ms (unchanged)
[90msrc/components/README.md[39m 1ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 8ms (unchanged)
[90msrc/features/auth/README.md[39m 4ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 16ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 16ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 9ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 3ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 5ms (unchanged)
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/README.md[39m 3ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 3ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 15ms (unchanged)
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 9ms (unchanged)
[90msrc/main.tsx[39m 3ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 1ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 3ms (unchanged)
[90msrc/styles.css[39m 37ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 6ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 2ms (unchanged)
[90mtests/api.test.ts[39m 27ms (unchanged)
[90mtests/app-auth.test.tsx[39m 13ms (unchanged)
[90mtests/app-execution.test.tsx[39m 25ms (unchanged)
[90mtests/app-routing.test.tsx[39m 10ms (unchanged)
[90mtests/app-sets.test.tsx[39m 10ms (unchanged)
[90mtests/app.test.ts[39m 2ms (unchanged)
[90mtests/appFixtures.ts[39m 5ms (unchanged)
[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
[90mtests/fakeFetch.ts[39m 10ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
[90mtests/guards.test.ts[39m 4ms (unchanged)
[90mtests/README.md[39m 5ms (unchanged)
[90mtests/render.tsx[39m 11ms (unchanged)
[90mtests/setup.ts[39m 7ms (unchanged)
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
409: [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 38[2mms[22m[39m

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
    if (element.checked !== checked) {
      element.click();
    }
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

 [32m✓[39m tests/api.test.ts [2m([22m[2m20 tests[22m[2m)[22m[32m 61[2mms[22m[39m
 [32m✓[39m tests/app-auth.test.tsx [2m([22m[2m7 tests[22m[2m)[22m[32m 189[2mms[22m[39m
 [32m✓[39m tests/app-execution.test.tsx [2m([22m[2m11 tests[22m[2m)[22m[32m 209[2mms[22m[39m
 [32m✓[39m tests/app-sets.test.tsx [2m([22m[2m4 tests[22m[2m)[22m[32m 141[2mms[22m[39m
 [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 38[2mms[22m[39m
 [32m✓[39m tests/app-routing.test.tsx [2m([22m[2m21 tests[22m[2m)[22m[32m 283[2mms[22m[39m
 [32m✓[39m tests/guards.test.ts [2m([22m[2m3 tests[22m[2m)[22m[32m 14[2mms[22m[39m
 [32m✓[39m tests/app.test.ts [2m([22m[2m1 test[22m[2m)[22m[32m 10[2mms[22m[39m

[2m Test Files [22m [1m[32m8 passed[39m[22m[90m (8)[39m
[2m      Tests [22m [1m[32m70 passed[39m[22m[90m (70)[39m
[2m   Start at [22m 23:33:59
[2m   Duration [22m 2.87s[2m (transform 308ms, setup 182ms, collect 749ms, tests 945ms, environment 3.76s, prepare 677ms)[22m


> esp32-macro-keyboard-webapp@0.1.0 build
> tsc -b && vite build

[36mvite v7.0.6 [32mbuilding for production...[36m[39m
transforming...
[32m✓[39m 46 modules transformed.
rendering chunks...
computing gzip size...
[2mdist/[22m[32mindex.html                 [39m[1m[2m  0.46 kB[22m[1m[22m[2m │ gzip:  0.30 kB[22m
[2mdist/[22m[2massets/[22m[35mindex-CHl-eRLJ.css  [39m[1m[2m  6.93 kB[22m[1m[22m[2m │ gzip:  2.28 kB[22m
[2mdist/[22m[2massets/[22m[36mindex-CxVunAmB.js   [39m[1m[2m209.65 kB[22m[1m[22m[2m │ gzip: 65.59 kB[22m
[32m✓ built in 1.03s[39m
```

## phase17-foundation-host-deps.log

```text

Get:1 file:/etc/apt/apt-mirrors.txt Mirrorlist [144 B]
Hit:2 http://azure.archive.ubuntu.com/ubuntu noble InRelease
Hit:6 https://packages.microsoft.com/repos/azure-cli noble InRelease
Get:7 https://packages.microsoft.com/ubuntu/24.04/prod noble InRelease [3600 B]
Get:3 http://azure.archive.ubuntu.com/ubuntu noble-updates InRelease [126 kB]
Get:4 http://azure.archive.ubuntu.com/ubuntu noble-backports InRelease [126 kB]
Get:5 http://azure.archive.ubuntu.com/ubuntu noble-security InRelease [126 kB]
Get:8 https://dl.google.com/linux/chrome-stable/deb stable InRelease [2548 B]
Get:9 https://packages.microsoft.com/ubuntu/24.04/prod noble/main arm64 Packages [217 kB]
Get:10 https://packages.microsoft.com/ubuntu/24.04/prod noble/main amd64 Packages [252 kB]
Get:11 http://azure.archive.ubuntu.com/ubuntu noble-updates/main amd64 Packages [1153 kB]
Get:12 http://azure.archive.ubuntu.com/ubuntu noble-updates/main Translation-en [278 kB]
Get:13 http://azure.archive.ubuntu.com/ubuntu noble-updates/main amd64 Components [180 kB]
Get:14 http://azure.archive.ubuntu.com/ubuntu noble-updates/universe amd64 Packages [1678 kB]
Get:15 http://azure.archive.ubuntu.com/ubuntu noble-updates/universe Translation-en [334 kB]
Get:16 http://azure.archive.ubuntu.com/ubuntu noble-updates/universe amd64 Components [389 kB]
Get:17 http://azure.archive.ubuntu.com/ubuntu noble-updates/restricted amd64 Packages [1367 kB]
Get:18 http://azure.archive.ubuntu.com/ubuntu noble-updates/restricted Translation-en [308 kB]
Get:19 http://azure.archive.ubuntu.com/ubuntu noble-updates/multiverse amd64 Packages [45.4 kB]
Get:20 http://azure.archive.ubuntu.com/ubuntu noble-updates/multiverse Translation-en [12.3 kB]
Get:21 http://azure.archive.ubuntu.com/ubuntu noble-updates/multiverse amd64 Components [940 B]
Get:22 http://azure.archive.ubuntu.com/ubuntu noble-backports/main amd64 Components [5760 B]
Get:23 http://azure.archive.ubuntu.com/ubuntu noble-backports/universe amd64 Packages [32.5 kB]
Get:24 http://azure.archive.ubuntu.com/ubuntu noble-backports/universe amd64 Components [12.6 kB]
Get:25 http://azure.archive.ubuntu.com/ubuntu noble-security/main amd64 Packages [898 kB]
Get:26 http://azure.archive.ubuntu.com/ubuntu noble-security/main Translation-en [198 kB]
Get:27 http://azure.archive.ubuntu.com/ubuntu noble-security/main amd64 Components [46.3 kB]
Get:28 http://azure.archive.ubuntu.com/ubuntu noble-security/universe amd64 Packages [1199 kB]
Get:29 http://azure.archive.ubuntu.com/ubuntu noble-security/universe Translation-en [239 kB]
Get:30 http://azure.archive.ubuntu.com/ubuntu noble-security/universe amd64 Components [76.2 kB]
Get:31 http://azure.archive.ubuntu.com/ubuntu noble-security/restricted amd64 Packages [1273 kB]
Get:32 http://azure.archive.ubuntu.com/ubuntu noble-security/restricted Translation-en [290 kB]
Get:33 http://azure.archive.ubuntu.com/ubuntu noble-security/multiverse amd64 Packages [40.3 kB]
Get:34 http://azure.archive.ubuntu.com/ubuntu noble-security/multiverse Translation-en [10.6 kB]
Get:35 https://dl.google.com/linux/chrome-stable/deb stable/main amd64 Packages [1424 B]
Fetched 10.9 MB in 1s (8151 kB/s)
Reading package lists...
Reading package lists...
Building dependency tree...
Reading state information...
jq is already the newest version (1.7.1-3ubuntu0.24.04.2).
python3-venv is already the newest version (3.12.3-0ubuntu2.1).
shellcheck is already the newest version (0.9.0-1).
The following additional packages will be installed:
  cmake-data libcjson1 libjsoncpp25 librhash0
Suggested packages:
  cmake-doc cmake-format elpa-cmake-mode ninja-build
The following NEW packages will be installed:
  build-essential clang-format clang-tidy cmake cmake-data libcjson-dev
  libcjson1 libjsoncpp25 librhash0
0 upgraded, 9 newly installed, 0 to remove and 73 not upgraded.
Need to get 13.6 MB of archives.
After this operation, 49.3 MB of additional disk space will be used.
Get:1 file:/etc/apt/apt-mirrors.txt Mirrorlist [144 B]
Get:2 http://azure.archive.ubuntu.com/ubuntu noble/main amd64 build-essential amd64 12.10ubuntu1 [4928 B]
Get:3 http://azure.archive.ubuntu.com/ubuntu noble/universe amd64 clang-format amd64 1:18.0-59~exp2 [5970 B]
Get:4 http://azure.archive.ubuntu.com/ubuntu noble/universe amd64 clang-tidy amd64 1:18.0-59~exp2 [5724 B]
Get:5 http://azure.archive.ubuntu.com/ubuntu noble/main amd64 libjsoncpp25 amd64 1.9.5-6build1 [82.8 kB]
Get:6 http://azure.archive.ubuntu.com/ubuntu noble/main amd64 librhash0 amd64 1.4.3-3build1 [129 kB]
Get:7 http://azure.archive.ubuntu.com/ubuntu noble/main amd64 cmake-data all 3.28.3-1build7 [2155 kB]
Get:8 http://azure.archive.ubuntu.com/ubuntu noble/main amd64 cmake amd64 3.28.3-1build7 [11.2 MB]
Get:9 http://azure.archive.ubuntu.com/ubuntu noble/universe amd64 libcjson1 amd64 1.7.17-1 [24.8 kB]
Get:10 http://azure.archive.ubuntu.com/ubuntu noble/universe amd64 libcjson-dev amd64 1.7.17-1 [20.8 kB]
Fetched 13.6 MB in 0s (68.5 MB/s)
Selecting previously unselected package build-essential.
(Reading database ... (Reading database ... 5%(Reading database ... 10%(Reading database ... 15%(Reading database ... 20%(Reading database ... 25%(Reading database ... 30%(Reading database ... 35%(Reading database ... 40%(Reading database ... 45%(Reading database ... 50%(Reading database ... 55%(Reading database ... 60%(Reading database ... 65%(Reading database ... 70%(Reading database ... 75%(Reading database ... 80%(Reading database ... 85%(Reading database ... 90%(Reading database ... 95%(Reading database ... 100%(Reading database ... 202954 files and directories currently installed.)
Preparing to unpack .../0-build-essential_12.10ubuntu1_amd64.deb ...
Unpacking build-essential (12.10ubuntu1) ...
Selecting previously unselected package clang-format:amd64.
Preparing to unpack .../1-clang-format_1%3a18.0-59~exp2_amd64.deb ...
Unpacking clang-format:amd64 (1:18.0-59~exp2) ...
Selecting previously unselected package clang-tidy.
Preparing to unpack .../2-clang-tidy_1%3a18.0-59~exp2_amd64.deb ...
Unpacking clang-tidy (1:18.0-59~exp2) ...
Selecting previously unselected package libjsoncpp25:amd64.
Preparing to unpack .../3-libjsoncpp25_1.9.5-6build1_amd64.deb ...
Unpacking libjsoncpp25:amd64 (1.9.5-6build1) ...
Selecting previously unselected package librhash0:amd64.
Preparing to unpack .../4-librhash0_1.4.3-3build1_amd64.deb ...
Unpacking librhash0:amd64 (1.4.3-3build1) ...
Selecting previously unselected package cmake-data.
Preparing to unpack .../5-cmake-data_3.28.3-1build7_all.deb ...
Unpacking cmake-data (3.28.3-1build7) ...
Selecting previously unselected package cmake.
Preparing to unpack .../6-cmake_3.28.3-1build7_amd64.deb ...
Unpacking cmake (3.28.3-1build7) ...
Selecting previously unselected package libcjson1:amd64.
Preparing to unpack .../7-libcjson1_1.7.17-1_amd64.deb ...
Unpacking libcjson1:amd64 (1.7.17-1) ...
Selecting previously unselected package libcjson-dev:amd64.
Preparing to unpack .../8-libcjson-dev_1.7.17-1_amd64.deb ...
Unpacking libcjson-dev:amd64 (1.7.17-1) ...
Setting up clang-format:amd64 (1:18.0-59~exp2) ...
Setting up clang-tidy (1:18.0-59~exp2) ...
Setting up libcjson1:amd64 (1.7.17-1) ...
Setting up libjsoncpp25:amd64 (1.9.5-6build1) ...
Setting up librhash0:amd64 (1.4.3-3build1) ...
Setting up build-essential (12.10ubuntu1) ...
Setting up cmake-data (3.28.3-1build7) ...
Setting up libcjson-dev:amd64 (1.7.17-1) ...
Setting up cmake (3.28.3-1build7) ...
Processing triggers for man-db (2.12.0-4build2) ...
Not building database; man-db/auto-update is not 'true'.
Processing triggers for libc-bin (2.39-0ubuntu8.7) ...

Running kernel seems to be up-to-date.

No services need to be restarted.

No containers need to be restarted.

No user sessions are running outdated binaries.

No VM guests are running outdated hypervisor (qemu) binaries on this host.
Collecting cmakelang==0.6.13
  Downloading cmakelang-0.6.13-py3-none-any.whl.metadata (23 kB)
Collecting yamllint==1.38.0
  Downloading yamllint-1.38.0-py3-none-any.whl.metadata (4.2 kB)
Requirement already satisfied: six>=1.13.0 in /usr/lib/python3/dist-packages (from cmakelang==0.6.13) (1.16.0)
Collecting pathspec>=1.0.0 (from yamllint==1.38.0)
  Downloading pathspec-1.1.1-py3-none-any.whl.metadata (14 kB)
Requirement already satisfied: pyyaml in /usr/lib/python3/dist-packages (from yamllint==1.38.0) (6.0.1)
Downloading cmakelang-0.6.13-py3-none-any.whl (159 kB)
   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 159.8/159.8 kB 18.5 MB/s eta 0:00:00
Downloading yamllint-1.38.0-py3-none-any.whl (68 kB)
   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 68.9/68.9 kB 27.0 MB/s eta 0:00:00
Downloading pathspec-1.1.1-py3-none-any.whl (57 kB)
   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 57.3/57.3 kB 18.9 MB/s eta 0:00:00
Installing collected packages: pathspec, cmakelang, yamllint
Successfully installed cmakelang-0.6.13 pathspec-1.1.1 yamllint-1.38.0
go: downloading mvdan.cc/sh/v3 v3.11.0
go: downloading github.com/google/renameio/v2 v2.0.0
go: downloading github.com/rogpeppe/go-internal v1.14.1
go: downloading golang.org/x/term v0.29.0
go: downloading mvdan.cc/editorconfig v0.3.0
go: downloading golang.org/x/sys v0.30.0
```

## phase17-foundation-firmware.log

```text
28:[  4%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
62:[ 13%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
65:[ 13%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
72:[ 17%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
77:[ 17%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
79:[ 17%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
80:[ 18%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
95:[ 17%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
103:[ 23%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
131:/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c:70:38: error: null character(s) preserved in literal [-Werror]
140:[ 26%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
150:[ 39%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
184:[ 53%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
197:[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
203:[ 63%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
213:[ 70%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
214:cc1: all warnings being treated as errors
218:gmake[2]: *** [CMakeFiles/auth_tests.dir/build.make:121: CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c.o] Error 1
220:[ 73%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
233:[ 37%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
236:[ 37%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
251:[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
256:[ 37%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
257:[ 37%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
266:[ 37%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
279:[ 37%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
313:[ 84%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
324:[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
347:gmake[1]: *** [CMakeFiles/Makefile2:402: CMakeFiles/auth_tests.dir/all] Error 2
415:gmake: *** [Makefile:101: all] Error 2

-- The C compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "1.8.1")
-- Checking for module 'libcjson'
--   Found libcjson, version 1.7.17
-- Configuring done (0.8s)
-- Generating done (0.1s)
-- Build files have been written to: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build
[  1%] Building C object CMakeFiles/test_support.dir/support/test_memory.c.o
[  1%] Building C object CMakeFiles/test_support.dir/support/test_temp_dir.c.o
[  1%] Building C object CMakeFiles/test_support.dir/support/test_assert.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_freertos.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_call_log.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_clock.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_random.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_usb_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_gpio_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_wifi_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_http_backend.c.o
[  4%] Linking C static library libtest_support.a
[  4%] Built target test_support
[  4%] Building C object CMakeFiles/app_operation_result_tests.dir/test_app_operation_result.c.o
[  4%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[  4%] Building C object CMakeFiles/app_operation_result_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[  5%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[  5%] Building C object CMakeFiles/macro_model_tests.dir/test_macro_model.c.o
[  5%] Building C object CMakeFiles/macro_parser_tests.dir/test_macro_parser.c.o
[  5%] Building C object CMakeFiles/macro_executor_tests.dir/test_macro_executor.c.o
[  5%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[  6%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  6%] Building C object CMakeFiles/test_support_tests.dir/test_support.c.o
[  6%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_common.c.o
[  7%] Building C object CMakeFiles/macro_executor_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor_engine.c.o
[  7%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[  5%] Building C object CMakeFiles/macro_model_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  7%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_rate_limit.c.o
[  7%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_password.c.o
[  7%] Building C object CMakeFiles/auth_tests.dir/test_auth.c.o
[  7%] Building C object CMakeFiles/web_security_tests.dir/test_web_security.c.o
[  8%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[  8%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[  8%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[  9%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c.o
[  7%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[  9%] Building C object CMakeFiles/device_controls_tests.dir/test_device_controls.c.o
[ 10%] Building C object CMakeFiles/app_core_tests.dir/test_app_core.c.o
[ 10%] Building C object CMakeFiles/web_server_adapter_tests.dir/test_web_server_adapter.c.o
[ 11%] Building C object CMakeFiles/device_controls_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/device_controls/device_controls_logic.c.o
[ 11%] Building C object CMakeFiles/wifi_ap_tests.dir/test_wifi_ap.c.o
[ 12%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 10%] Building C object CMakeFiles/usb_keyboard_tests.dir/test_usb_keyboard.c.o
[ 12%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core_sequence.c.o
[ 12%] Building C object CMakeFiles/wifi_ap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap_state.c.o
[ 12%] Building C object CMakeFiles/usb_keyboard_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard_state.c.o
[ 12%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_json.c.o
[ 13%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c.o
[ 13%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[  9%] Building C object CMakeFiles/provisioning_tests.dir/test_provisioning.c.o
[ 13%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 13%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 13%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c.o
[ 13%] Building C object CMakeFiles/storage_mount_tests.dir/test_storage_mount.c.o
[ 13%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[ 14%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[ 16%] Building C object CMakeFiles/storage_atomic_tests.dir/test_storage_atomic.c.o
[ 17%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[ 17%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 17%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_common.c.o
[ 15%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_body_auth.c.o
[ 17%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 17%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_static_stream.c.o
[ 17%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 17%] Building C object CMakeFiles/provisioning_settings_tests.dir/test_provisioning_settings.c.o
[ 17%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 18%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 18%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 19%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 20%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 21%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_lifecycle.c.o
[ 22%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 22%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 22%] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[ 17%] Building C object CMakeFiles/storage_repository_lock_tests.dir/test_storage_repository_lock.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 23%] Building C object CMakeFiles/web_execution_submit_tests.dir/test_web_execution_submit.c.o
[ 23%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 24%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_route_policy.c.o
[ 25%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 17%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 18%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 26%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 21%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 26%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 26%] Building C object CMakeFiles/web_api_json_tests.dir/test_web_api_json.c.o
[ 23%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 27%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 28%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 29%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 29%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 30%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 30%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 30%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 30%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c: In function ‘clear_csrf_output’:
[ 31%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/test_web_execution_route_policy.c.o
[ 31%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 31%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 31%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 31%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 33%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 33%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
[ 33%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 34%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 34%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 34%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 35%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 35%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_admin_boundary.c.o
[ 35%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 35%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 35%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 36%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 36%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c:70:38: error: null character(s) preserved in literal [-Werror]
   70 |         validation->csrf_output[0] = ' ';
      |                                      ^
[ 26%] Building C object CMakeFiles/storage_parent_sync_tests.dir/test_storage_parent_sync.c.o
[ 26%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 26%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/test_storage_atomic_recovery.c.o
[ 30%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 26%] Building C object CMakeFiles/storage_repository_io_tests.dir/test_storage_repository_io.c.o
[ 26%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 26%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/test_web_api_admin_boundary.c.o
[ 37%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 37%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 37%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 37%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 38%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 38%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 39%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 39%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 36%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 39%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 39%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 40%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 40%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 41%] Building C object CMakeFiles/web_api_response_tests.dir/test_web_api_response.c.o
[ 44%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 45%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 45%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 46%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 47%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 47%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 47%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 47%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 47%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 47%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 47%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 47%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 48%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 49%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 50%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 43%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 50%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 42%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 50%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 50%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 50%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 45%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 50%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 51%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 52%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 52%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 52%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 53%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 53%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 53%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 54%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 54%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 56%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 57%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 57%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 52%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 58%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 58%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 59%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 62%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c.o
[ 62%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 62%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 63%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 57%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o
[ 64%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 64%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 64%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_handler_common.c.o
[ 66%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 67%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 68%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 69%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_dispatch.c.o
[ 70%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
cc1: all warnings being treated as errors
[ 70%] Linking C executable app_operation_result_tests
[ 71%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 72%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
gmake[2]: *** [CMakeFiles/auth_tests.dir/build.make:121: CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c.o] Error 1
gmake[2]: *** Waiting for unfinished jobs....
[ 73%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 74%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 75%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 76%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 77%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 78%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 79%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 80%] Building C object CMakeFiles/storage_macro_repository_tests.dir/test_storage_macros.c.o
[ 82%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 83%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 37%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 37%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 81%] Linking C executable macro_model_tests
[ 37%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 37%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 37%] Building C object CMakeFiles/web_api_dispatch_tests.dir/test_web_api_dispatch.c.o
[ 37%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 37%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 37%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 37%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 37%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 37%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 37%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 37%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_submit.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 37%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 37%] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
[ 37%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 37%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 37%] Building C object CMakeFiles/web_setup_tests.dir/test_web_setup.c.o
[ 37%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 37%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 37%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 37%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 37%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 37%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
[ 37%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 37%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 37%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 37%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 37%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 37%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_request_policy.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 37%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 37%] Building C object CMakeFiles/web_request_policy_tests.dir/test_web_request_policy.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/test_storage_atomic_validators.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 37%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 37%] Building C object CMakeFiles/storage_quarantine_tests.dir/test_storage_quarantine.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 37%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 37%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 37%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 37%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 84%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 85%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 86%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 86%] Linking C executable storage_repository_lock_tests
[ 87%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 84%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 84%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o
[ 84%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 84%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 84%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/test_web_api_repository_handlers.c.o
[ 84%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 84%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 84%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 84%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 84%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 87%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 88%] Linking C executable web_security_tests
[ 88%] Built target app_operation_result_tests
[ 89%] Linking C executable macro_parser_tests
[ 88%] Linking C executable storage_mount_tests
[ 89%] Linking C executable web_server_adapter_tests
[ 89%] Linking C executable storage_repository_io_tests
[ 89%] Linking C executable device_controls_tests
[ 89%] Built target storage_repository_lock_tests
[ 89%] Linking C executable usb_keyboard_tests
[ 89%] Built target web_security_tests
[ 89%] Linking C executable storage_atomic_tests
gmake[1]: *** [CMakeFiles/Makefile2:402: CMakeFiles/auth_tests.dir/all] Error 2
gmake[1]: *** Waiting for unfinished jobs....
[ 90%] Linking C executable wifi_ap_tests
[ 89%] Linking C executable provisioning_tests
[ 90%] Built target storage_mount_tests
[ 90%] Linking C executable test_support_tests
[ 90%] Linking C executable macro_executor_tests
[ 90%] Linking C executable storage_transaction_tests
[ 91%] Linking C executable web_execution_submit_tests
[ 91%] Built target device_controls_tests
[ 91%] Linking C executable web_api_response_tests
[ 92%] Linking C executable provisioning_bootstrap_tests
[ 92%] Linking C executable web_api_dispatch_tests
[ 93%] Linking C executable web_api_core_tests
[ 92%] Built target web_server_adapter_tests
[ 93%] Linking C executable web_execution_route_policy_tests
[ 93%] Built target storage_repository_io_tests
[ 93%] Built target usb_keyboard_tests
[ 93%] Built target macro_model_tests
[ 93%] Built target storage_atomic_tests
[ 93%] Built target wifi_ap_tests
[ 93%] Linking C executable storage_procedure_repository_tests
[ 94%] Linking C executable web_setup_tests
[ 94%] Linking C executable web_setup_json_tests
[ 94%] Linking C executable storage_parent_sync_tests
[ 94%] Built target web_execution_submit_tests
[ 94%] Built target web_api_response_tests
[ 95%] Linking C executable web_api_json_tests
[ 95%] Linking C executable web_request_policy_tests
[ 94%] Built target test_support_tests
[ 95%] Built target provisioning_tests
[ 95%] Built target macro_parser_tests
[ 96%] Linking C executable storage_object_json_tests
[ 96%] Built target web_api_dispatch_tests
[ 97%] Built target web_execution_route_policy_tests
[ 96%] Built target macro_executor_tests
[ 97%] Linking C executable provisioning_settings_tests
[ 97%] Built target provisioning_bootstrap_tests
[ 97%] Built target web_api_core_tests
[ 97%] Linking C executable web_api_admin_boundary_tests
[ 97%] Built target storage_transaction_tests
[ 97%] Linking C executable storage_progress_repository_tests
[ 97%] Built target web_api_admin_boundary_tests
[ 98%] Linking C executable app_core_tests
[ 98%] Linking C executable storage_active_set_delete_tests
[ 98%] Built target web_setup_json_tests
[ 98%] Built target storage_procedure_repository_tests
[ 99%] Linking C executable storage_macro_repository_tests
[ 99%] Built target web_setup_tests
[ 99%] Built target storage_parent_sync_tests
[ 99%] Built target web_api_json_tests
[ 99%] Built target web_request_policy_tests
[ 99%] Built target storage_object_json_tests
[ 99%] Linking C executable storage_repository_tests
[ 99%] Built target provisioning_settings_tests
[ 99%] Built target storage_active_set_delete_tests
[ 99%] Built target app_core_tests
[ 99%] Linking C executable storage_atomic_recovery_tests
[ 99%] Built target storage_macro_repository_tests
[ 99%] Linking C executable storage_atomic_validators_tests
[ 99%] Linking C executable web_api_repository_handlers_tests
[ 99%] Built target storage_progress_repository_tests
[100%] Linking C executable storage_quarantine_tests
[100%] Built target storage_repository_tests
[100%] Built target storage_atomic_recovery_tests
[100%] Built target storage_atomic_validators_tests
[100%] Built target web_api_repository_handlers_tests
[100%] Built target storage_quarantine_tests
gmake: *** [Makefile:101: all] Error 2
```
