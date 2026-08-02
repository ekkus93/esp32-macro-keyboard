import { spawn, spawnSync } from "node:child_process";
import { createServer } from "node:http";
import { mkdtemp, readFile, rm, stat } from "node:fs/promises";
import { tmpdir } from "node:os";
import { extname, join, normalize } from "node:path";
import process from "node:process";

const setId = "11111111-1111-4111-8111-111111111111";
const secondSetId = "99999999-9999-4999-8999-999999999999";
const macroId = "22222222-2222-4222-8222-222222222222";
const executionId = "33333333-3333-4333-8333-333333333333";

const firstSet = {
  schema_version: 1,
  id: setId,
  revision: 2,
  name: "Lab bench workflow",
};
const secondSet = {
  ...firstSet,
  id: secondSetId,
  revision: 3,
  name: "Second workflow",
};
const macro = {
  schema_version: 1,
  id: macroId,
  revision: 7,
  set_id: setId,
  name: "Open terminal",
  source: "{CTRL+ALT+T}",
  key_press_ms: 8,
  inter_key_ms: 15,
};
const settings = {
  schemaVersion: 1,
  revision: 4,
  requirePhysicalConfirmation: false,
  alwaysSelectSet: true,
  activeSetId: setId,
};
const idleStatus = {
  version: "0.1.0",
  idf: "v5.5.5",
  usbState: "ready",
  wifiState: "started",
  wifiClients: 1,
  executionState: "idle",
};

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function commandPath(candidates) {
  for (const candidate of candidates) {
    const result = spawnSync("which", [candidate], { encoding: "utf8" });
    if (result.status === 0) {
      return result.stdout.trim();
    }
  }
  throw new Error(
    "Chrome or Chromium is required for Phase 17.10 browser validation.",
  );
}

function contentType(path) {
  switch (extname(path)) {
    case ".css":
      return "text/css; charset=utf-8";
    case ".html":
      return "text/html; charset=utf-8";
    case ".js":
      return "text/javascript; charset=utf-8";
    case ".json":
      return "application/json; charset=utf-8";
    case ".svg":
      return "image/svg+xml";
    default:
      return "application/octet-stream";
  }
}

function sendJson(response, status, data) {
  const body = JSON.stringify(data);
  response.writeHead(status, {
    "Content-Type": "application/json; charset=utf-8",
    "Content-Length": Buffer.byteLength(body),
    "Cache-Control": "no-store",
  });
  response.end(body);
}

async function requestBody(request) {
  const chunks = [];
  for await (const chunk of request) {
    chunks.push(chunk);
  }
  const text = Buffer.concat(chunks).toString("utf8");
  return text.length === 0 ? null : JSON.parse(text);
}

async function startApplicationServer() {
  const dist = new URL("../../dist/", import.meta.url);
  const state = {
    sets: [firstSet, secondSet],
    lastOrder: null,
    settingsReads: 0,
    executionAccepted: false,
    executionPolls: 0,
  };

  const server = createServer(async (request, response) => {
    try {
      const url = new URL(request.url ?? "/", "http://127.0.0.1");
      const method = request.method ?? "GET";
      if (url.pathname.startsWith("/api/")) {
        if (method === "GET" && url.pathname === "/api/v1/setup-state") {
          sendJson(response, 401, {
            ok: false,
            error: { code: "auth_required", message: "normal mode" },
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/status") {
          sendJson(response, 200, {
            ok: true,
            data: {
              ...idleStatus,
              executionState: state.executionAccepted ? "running" : "idle",
            },
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/auth/session") {
          sendJson(response, 200, {
            ok: true,
            data: { authenticated: true },
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/settings") {
          state.settingsReads += 1;
          sendJson(response, 200, { ok: true, data: settings });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/sets") {
          sendJson(response, 200, { ok: true, data: state.sets });
          return;
        }
        if (method === "PUT" && url.pathname === "/api/v1/sets/order") {
          const body = await requestBody(request);
          assert(
            body !== null &&
              Array.isArray(body.ids) &&
              body.ids.length === state.sets.length,
            "Browser reorder request did not contain the complete ID order.",
          );
          state.lastOrder = [...body.ids];
          state.sets = body.ids.map((id) => {
            const found = state.sets.find((set) => set.id === id);
            assert(
              found !== undefined,
              `Unknown reordered set ID: ${String(id)}`,
            );
            return found;
          });
          sendJson(response, 200, { ok: true, data: state.sets });
          return;
        }
        if (
          method === "GET" &&
          url.pathname === `/api/v1/sets/${setId}/macros`
        ) {
          sendJson(response, 200, { ok: true, data: [macro] });
          return;
        }
        if (
          method === "GET" &&
          url.pathname === `/api/v1/sets/${setId}/macros/${macroId}`
        ) {
          sendJson(response, 200, { ok: true, data: macro });
          return;
        }
        if (
          method === "POST" &&
          url.pathname === `/api/v1/sets/${setId}/macros/${macroId}/validate`
        ) {
          sendJson(response, 200, {
            ok: true,
            data: { valid: true, actionCount: 1, estimatedDurationMs: 31 },
          });
          return;
        }
        if (method === "POST" && url.pathname === "/api/v1/executions") {
          const body = await requestBody(request);
          assert(
            body?.setId === setId &&
              body?.macroId === macroId &&
              body?.macroRevision === macro.revision &&
              body?.sourceContext === undefined,
            "Browser execution request did not match the standalone typed contract.",
          );
          state.executionAccepted = true;
          state.executionPolls = 0;
          sendJson(response, 202, {
            ok: true,
            data: { executionId, actionCount: 1, estimatedDurationMs: 31 },
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/executions/current") {
          state.executionPolls += 1;
          const completed = state.executionPolls >= 2;
          sendJson(response, 200, {
            ok: true,
            data: {
              executionId,
              setId,
              macroId,
              macroRevision: macro.revision,
              state: completed ? "completed" : "running",
              error: "",
              releaseError: "",
              actionIndex: completed ? 1 : 0,
              actionCount: 1,
              available: completed,
              cancellationRequested: false,
              acceptedMs: 1000,
              startedMs: 1010,
              completedMs: completed ? 1200 : 0,
              currentAction: completed ? "none" : "key",
            },
          });
          if (completed) {
            state.executionAccepted = false;
          }
          return;
        }
        sendJson(response, 404, {
          ok: false,
          error: {
            code: "not_found",
            message: `${method} ${url.pathname} is not implemented by browser fixture`,
          },
        });
        return;
      }

      const requested =
        url.pathname === "/" ? "index.html" : url.pathname.slice(1);
      const safe = normalize(requested).replace(/^(\.\.[/\\])+/, "");
      let file = new URL(safe, dist);
      try {
        const information = await stat(file);
        if (!information.isFile()) {
          file = new URL("index.html", dist);
        }
      } catch {
        file = new URL("index.html", dist);
      }
      const bytes = await readFile(file);
      response.writeHead(200, {
        "Content-Type": contentType(file.pathname),
        "Content-Length": bytes.byteLength,
        "Cache-Control": "no-store",
      });
      response.end(bytes);
    } catch (error) {
      sendJson(response, 500, {
        ok: false,
        error: {
          code: "browser_fixture_error",
          message: error instanceof Error ? error.message : String(error),
        },
      });
    }
  });

  await new Promise((resolve, reject) => {
    server.once("error", reject);
    server.listen(0, "127.0.0.1", resolve);
  });
  const address = server.address();
  assert(
    address !== null && typeof address !== "string",
    "Missing server port.",
  );
  return {
    baseUrl: `http://127.0.0.1:${String(address.port)}`,
    close: () => new Promise((resolve) => server.close(resolve)),
    state,
  };
}

async function devToolsUrl(processHandle) {
  return new Promise((resolve, reject) => {
    let output = "";
    const timeout = setTimeout(() => {
      reject(new Error(`Chrome did not expose DevTools. Output:\n${output}`));
    }, 15_000);
    const receive = (chunk) => {
      output += chunk.toString("utf8");
      const match = output.match(/DevTools listening on (ws:\/\/[^\s]+)/);
      if (match?.[1] !== undefined) {
        clearTimeout(timeout);
        resolve(match[1]);
      }
    };
    processHandle.stderr.on("data", receive);
    processHandle.stdout.on("data", receive);
    processHandle.once("exit", (code) => {
      clearTimeout(timeout);
      reject(
        new Error(
          `Chrome exited before DevTools startup with code ${String(code)}.`,
        ),
      );
    });
  });
}

class Cdp {
  constructor(socket) {
    this.socket = socket;
    this.nextId = 1;
    this.pending = new Map();
    socket.addEventListener("message", (event) => {
      const message = JSON.parse(String(event.data));
      if (message.id === undefined) {
        return;
      }
      const pending = this.pending.get(message.id);
      if (pending === undefined) {
        return;
      }
      this.pending.delete(message.id);
      if (message.error !== undefined) {
        pending.reject(new Error(JSON.stringify(message.error)));
      } else {
        pending.resolve(message.result);
      }
    });
  }

  send(method, params = {}) {
    const id = this.nextId;
    this.nextId += 1;
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.socket.send(JSON.stringify({ id, method, params }));
    });
  }

  close() {
    this.socket.close();
  }
}

async function connectPage(browserWebSocketUrl, applicationUrl) {
  const browserUrl = new URL(browserWebSocketUrl);
  const target = await fetch(
    `http://127.0.0.1:${browserUrl.port}/json/new?${encodeURIComponent(applicationUrl)}`,
    { method: "PUT" },
  ).then((response) => response.json());
  assert(
    typeof target.webSocketDebuggerUrl === "string",
    "Chrome did not return a page debugger URL.",
  );
  const socket = new WebSocket(target.webSocketDebuggerUrl);
  await new Promise((resolve, reject) => {
    socket.addEventListener("open", resolve, { once: true });
    socket.addEventListener("error", reject, { once: true });
  });
  const cdp = new Cdp(socket);
  await cdp.send("Runtime.enable");
  await cdp.send("Page.enable");
  await cdp.send("Network.enable");
  return cdp;
}

async function evaluate(cdp, expression) {
  const result = await cdp.send("Runtime.evaluate", {
    expression,
    awaitPromise: true,
    returnByValue: true,
    userGesture: true,
  });
  if (result.exceptionDetails !== undefined) {
    throw new Error(
      result.exceptionDetails.exception?.description ??
        result.exceptionDetails.text ??
        "Browser evaluation failed.",
    );
  }
  return result.result.value;
}

async function waitFor(cdp, expression, message, timeoutMs = 12_000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    if (await evaluate(cdp, expression)) {
      return;
    }
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  const text = await evaluate(cdp, "document.body.innerText");
  throw new Error(`${message}\nCurrent page:\n${String(text)}`);
}

function buttonExpression(text) {
  return `Array.from(document.querySelectorAll('button')).find((element) => element.textContent.trim() === ${JSON.stringify(text)})`;
}

async function clickButton(cdp, text, focus = false) {
  const expression = buttonExpression(text);
  const found = await evaluate(
    cdp,
    `(() => { const button = ${expression}; if (!button) return false; ${
      focus ? "button.focus();" : ""
    } button.click(); return true; })()`,
  );
  assert(found, `Missing browser button: ${text}`);
}

async function dispatchKey(cdp, key, modifiers = 0) {
  const keyCode =
    key === "Tab" ? 9 : key === "Escape" ? 27 : key === "Enter" ? 13 : 0;
  await cdp.send("Input.dispatchKeyEvent", {
    type: key === "Enter" ? "keyDown" : "rawKeyDown",
    key,
    code: key,
    windowsVirtualKeyCode: keyCode,
    nativeVirtualKeyCode: keyCode,
    modifiers,
    text: key === "Enter" ? "\r" : undefined,
    unmodifiedText: key === "Enter" ? "\r" : undefined,
  });
  await cdp.send("Input.dispatchKeyEvent", {
    type: "keyUp",
    key,
    code: key,
    windowsVirtualKeyCode: keyCode,
    nativeVirtualKeyCode: keyCode,
    modifiers,
  });
}

async function assertTouchTargets(cdp) {
  const failures = await evaluate(
    cdp,
    `(() => Array.from(document.querySelectorAll('button:not([disabled]), a[href], input:not([disabled]), textarea:not([disabled]), select:not([disabled])')).map((element) => {
      const target = element.matches('input[type="checkbox"]') ? element.closest('label') : element;
      const rect = target.getBoundingClientRect();
      return { label: element.getAttribute('aria-label') || element.textContent.trim() || element.id || element.tagName, width: rect.width, height: rect.height };
    }).filter((item) => item.width < 44 || item.height < 44))()`,
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
const MOBILE_VIEWPORT = {
  width: 360,
  height: 640,
  deviceScaleFactor: 2,
  mobile: true,
};
const DESKTOP_VIEWPORT = {
  width: 1280,
  height: 800,
  deviceScaleFactor: 1,
  mobile: false,
};

async function overflowingElements(cdp) {
  return evaluate(
    cdp,
    `(() => {
      const limit = document.documentElement.clientWidth;
      return Array.from(document.querySelectorAll('body *'))
        .filter((element) => {
          const style = window.getComputedStyle(element);
          if (style.display === 'none' || style.visibility === 'hidden') return false;
          const rect = element.getBoundingClientRect();
          if (rect.width === 0 && rect.height === 0) return false;
          return rect.right > limit + 1;
        })
        .slice(0, 5)
        .map((element) => ({
          tag: element.tagName,
          id: element.id,
          classes: typeof element.className === 'string' ? element.className : '',
          right: Math.round(element.getBoundingClientRect().right),
          limit,
        }));
    })()`,
  );
}

async function contentWidth(cdp) {
  return evaluate(
    cdp,
    "Math.round(document.querySelector('#main-content').getBoundingClientRect().width)",
  );
}

async function assertFitsViewport(cdp, label) {
  const scroll = await evaluate(
    cdp,
    "({ scrollWidth: document.documentElement.scrollWidth, clientWidth: document.documentElement.clientWidth })",
  );
  assert(
    scroll.scrollWidth <= scroll.clientWidth + 1,
    `${label}: the page scrolls horizontally (${String(scroll.scrollWidth)} > ${String(scroll.clientWidth)}).`,
  );
  const overflowing = await overflowingElements(cdp);
  assert(
    Array.isArray(overflowing) && overflowing.length === 0,
    `${label}: elements extend past the right edge: ${JSON.stringify(overflowing)}`,
  );
}

async function assertResponsiveLayout(cdp) {
  await cdp.send("Emulation.setDeviceMetricsOverride", MOBILE_VIEWPORT);
  await assertFitsViewport(cdp, "Mobile 360x640");
  /* Touch targets are re-checked here rather than trusted from the default
     window size: a narrower viewport is where controls get squeezed. */
  await assertTouchTargets(cdp);
  const mobileWidth = await contentWidth(cdp);
  assert(
    mobileWidth > 0 && mobileWidth <= MOBILE_VIEWPORT.width,
    `Mobile content width ${String(mobileWidth)} does not fit a ${String(MOBILE_VIEWPORT.width)}px viewport.`,
  );

  await cdp.send("Emulation.setDeviceMetricsOverride", DESKTOP_VIEWPORT);
  await assertFitsViewport(cdp, "Desktop 1280x800");
  const desktopWidth = await contentWidth(cdp);
  /* "Mobile-first and usable from a desktop browser" is two requirements. A
     layout locked to the phone width satisfies the first and fails the second,
     and a horizontal-scroll check alone would never notice. */
  assert(
    desktopWidth > mobileWidth,
    `The layout does not adapt: content is ${String(desktopWidth)}px at 1280px wide and ${String(mobileWidth)}px at 360px.`,
  );

  await cdp.send("Emulation.clearDeviceMetricsOverride");
}

async function runBrowserWorkflows(cdp, serverState) {
  await waitFor(
    cdp,
    "document.body?.innerText.includes('Choose a macro set') ?? false",
    "Authenticated set selection did not load.",
  );
  await assertTouchTargets(cdp);
  await assertResponsiveLayout(cdp);

  await evaluate(cdp, "document.querySelector('#main-content').focus()");
  await dispatchKey(cdp, "Tab");
  const keyboardFocus = await evaluate(
    cdp,
    "document.activeElement.textContent.trim()",
  );
  assert(
    keyboardFocus === "Manage sets",
    `Keyboard navigation did not reach Manage sets: ${String(keyboardFocus)}`,
  );
  await dispatchKey(cdp, "Enter");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Manage macro sets')",
    "Set management did not load.",
  );

  const colorOnlyStatuses = await evaluate(
    cdp,
    "Array.from(document.querySelectorAll('.status-badge')).filter((element) => element.textContent.trim().length === 0).length",
  );
  assert(
    colorOnlyStatuses === 0,
    "A status badge relied on color without visible text.",
  );

  await clickButton(cdp, "Create set", true);
  await waitFor(
    cdp,
    "document.querySelector('[role=dialog]') !== null",
    "Create-set dialog did not open.",
  );
  const initialFocus = await evaluate(
    cdp,
    "document.activeElement.getAttribute('aria-label') || document.activeElement.textContent.trim()",
  );
  assert(
    initialFocus === "Close Create macro set",
    `Unexpected initial dialog focus: ${String(initialFocus)}`,
  );
  await dispatchKey(cdp, "Tab", 8);
  const wrappedFocus = await evaluate(
    cdp,
    "({inside: document.querySelector('[role=dialog]').contains(document.activeElement), text: document.activeElement.textContent.trim()})",
  );
  assert(
    wrappedFocus.inside === true && wrappedFocus.text === "Cancel",
    `Dialog focus did not wrap to Cancel: ${JSON.stringify(wrappedFocus)}`,
  );
  await dispatchKey(cdp, "Escape");
  await waitFor(
    cdp,
    "document.querySelector('[role=dialog]') === null",
    "Escape did not close the dialog.",
  );
  const restoredFocus = await evaluate(
    cdp,
    "document.activeElement.textContent.trim()",
  );
  assert(
    restoredFocus === "Create set",
    "Dialog focus was not restored to its opener.",
  );

  const reordered = await evaluate(
    cdp,
    `(() => { const button = document.querySelector(${JSON.stringify(
      `[aria-label="Move ${firstSet.name} down"]`,
    )}); if (!button) return false; button.click(); return true; })()`,
  );
  assert(reordered, "Accessible Move down control was missing.");
  await waitFor(
    cdp,
    `document.body.innerText.includes(${JSON.stringify(
      `Moved ${firstSet.name} to position 2.`,
    )})`,
    "Set reorder did not complete.",
  );
  assert(
    JSON.stringify(serverState.lastOrder) ===
      JSON.stringify([secondSetId, setId]),
    `Unexpected reordered IDs: ${JSON.stringify(serverState.lastOrder)}`,
  );

  await evaluate(cdp, "window.dispatchEvent(new Event('offline'))");
  await waitFor(
    cdp,
    "document.querySelector('.connectivity-offline[role=status]') !== null && document.body.innerText.includes('Offline.')",
    "Offline state was not announced.",
  );
  const readsBeforeReconnect = serverState.settingsReads;
  await evaluate(cdp, "window.dispatchEvent(new Event('online'))");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Connection restored.')",
    "Reconnect state was not announced.",
  );
  const reconnectDeadline = Date.now() + 5_000;
  while (
    serverState.settingsReads <= readsBeforeReconnect &&
    Date.now() < reconnectDeadline
  ) {
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  assert(
    serverState.settingsReads > readsBeforeReconnect,
    "Reconnect did not trigger a live settings refresh.",
  );

  await clickButton(cdp, "Macros");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Open terminal')",
    "Macro library did not load.",
  );
  await clickButton(cdp, "Send");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Confirm send') && document.body.innerText.includes('{CTRL+ALT+T}')",
    "Execution preview did not load persisted macro data.",
  );
  await assertTouchTargets(cdp);
  await clickButton(cdp, "Send now");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Macro completed')",
    "The complete execution workflow did not reach a terminal result.",
    15_000,
  );
  assert(
    serverState.executionPolls >= 2,
    "Execution page did not poll through running to terminal state.",
  );
}

async function stopChrome(processHandle) {
  if (processHandle.exitCode !== null) {
    return;
  }
  await new Promise((resolve) => {
    const timeout = setTimeout(() => {
      processHandle.kill("SIGKILL");
    }, 5_000);
    processHandle.once("exit", () => {
      clearTimeout(timeout);
      resolve();
    });
    processHandle.kill("SIGTERM");
  });
}

async function main() {
  const chrome = commandPath([
    "google-chrome",
    "google-chrome-stable",
    "chromium",
    "chromium-browser",
  ]);
  const userDataDirectory = await mkdtemp(
    join(tmpdir(), "esp32-macro-browser-"),
  );
  const application = await startApplicationServer();
  const chromeProcess = spawn(
    chrome,
    [
      "--headless=new",
      "--disable-gpu",
      "--no-sandbox",
      "--disable-dev-shm-usage",
      "--remote-debugging-port=0",
      `--user-data-dir=${userDataDirectory}`,
      "--window-size=390,844",
      "about:blank",
    ],
    { stdio: ["ignore", "pipe", "pipe"] },
  );
  let cdp;
  try {
    const debuggerUrl = await devToolsUrl(chromeProcess);
    cdp = await connectPage(debuggerUrl, `${application.baseUrl}/#/sets`);
    await runBrowserWorkflows(cdp, application.state);
    console.log("Real Chrome Phase 17.10 workflows passed.");
  } finally {
    cdp?.close();
    await stopChrome(chromeProcess);
    await application.close();
    await rm(userDataDirectory, {
      recursive: true,
      force: true,
      maxRetries: 5,
      retryDelay: 100,
    });
  }
}

main().catch((error) => {
  console.error(error instanceof Error ? error.stack : error);
  process.exitCode = 1;
});
