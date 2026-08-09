import { spawn, spawnSync } from "node:child_process";
import { createServer } from "node:http";
import { mkdtemp, readFile, rm, stat } from "node:fs/promises";
import { tmpdir } from "node:os";
import { extname, join, normalize } from "node:path";
import process from "node:process";
import { gzipSync } from "node:zlib";

/*
 * v2 fixture data (TODO_V2 Phase 9 exit gate). The v1 fixture this file used
 * to drive is gone: firmware deleted the v1 package/macro/execution routes
 * in Phase 2, and `main.tsx` now boots `AppV2` (TODO_V2 V2-090), so a real
 * browser exercising the built app only ever sees v2 `/api/v1/*` routes.
 *
 * The repository blob below is real gzip (`node:zlib`), decoded by the
 * browser's own `DecompressionStream("gzip")` — this is the one piece of
 * coverage the Vitest suite cannot fully substitute for, since jsdom has no
 * `DecompressionStream`.
 */
const packageId = "11111111-1111-4111-8111-111111111111";
const macroCompleteId = "22222222-2222-4222-8222-222222222222";
const macroFailId = "33333333-3333-4333-8333-333333333333";
const macroTimeoutId = "44444444-4444-4444-8444-444444444444";
const macroReleaseErrorId = "55555555-5555-4555-8555-555555555555";
const macroConfirmId = "66666666-6666-4666-8666-666666666666";
// A fresh, real-looking UUID v4 per send (not a fixed constant): the client
// (correctly) treats a repeated send ID as an already-observed completion,
// exactly as it must to satisfy TODO_V2 V2-095 "prevent duplicate completion
// callbacks" — a fixture reusing one ID across sends would silently hide
// every send after the first instead of exercising that guard honestly.
let sendCounter = 0;
function nextSendId() {
  sendCounter += 1;
  const suffix = String(sendCounter).padStart(12, "0");
  return `77777777-7777-4777-8777-${suffix}`;
}
const blobId = "1";

const repository = {
  format: "esp32-macro-keyboard-repository",
  schemaVersion: 1,
  packages: [
    {
      id: packageId,
      name: "Lab bench workflow",
      macros: [
        {
          id: macroCompleteId,
          name: "Open terminal",
          source: "a",
          keyPressMs: 8,
          interKeyMs: 15,
        },
        {
          id: macroConfirmId,
          name: "Confirm before typing",
          source: "AWAIT_CONFIRM",
          keyPressMs: 8,
          interKeyMs: 15,
        },
        {
          id: macroFailId,
          name: "Trigger failure",
          source: "FAIL",
          keyPressMs: 8,
          interKeyMs: 15,
        },
        {
          id: macroTimeoutId,
          name: "Trigger timeout",
          source: "TIMEOUT",
          keyPressMs: 8,
          interKeyMs: 15,
        },
        {
          id: macroReleaseErrorId,
          name: "Trigger release error",
          source: "RELEASE_ERROR",
          keyPressMs: 8,
          interKeyMs: 15,
        },
      ],
    },
  ],
};
const repositoryGzip = gzipSync(
  Buffer.from(JSON.stringify(repository), "utf8"),
);

const settings = {
  deviceName: "Bench Macro Keyboard",
  requireSerialConfirmation: false,
  sendMode: "quick",
  snapshotRetentionTarget: 5,
  showMacroSourcePreviews: false,
  lastSelectedPackageId: packageId,
  apSsid: "MacroKeyboard",
  stationConfigured: false,
  stationSsid: null,
};

const status = {
  provisioned: true,
  deviceName: settings.deviceName,
  firmwareVersion: "0.2.0",
  buildId: "browser-fixture",
  uptimeMs: 1000,
  usb: { state: "ready" },
  accessPoint: { state: "started", ssid: settings.apSsid, clientCount: 1 },
  station: { configured: false, state: "idle", ssid: null, ipv4: null },
  storage: {
    state: "healthy",
    totalBytes: 131072,
    usedBytes: repositoryGzip.byteLength,
    remainingBytes: 131072 - repositoryGzip.byteLength,
    blobCount: 1,
  },
  send: { present: false, state: null },
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

function sendJson(response, status_, data) {
  const body = JSON.stringify(data);
  response.writeHead(status_, {
    "Content-Type": "application/json; charset=utf-8",
    "Content-Length": Buffer.byteLength(body),
    "Cache-Control": "no-store",
  });
  response.end(body);
}

function sendError(response, status_, code, message) {
  sendJson(response, status_, { error: { code, message } });
}

async function requestBody(request) {
  const chunks = [];
  for await (const chunk of request) {
    chunks.push(chunk);
  }
  const text = Buffer.concat(chunks).toString("utf8");
  return text.length === 0 ? null : JSON.parse(text);
}

/**
 * Advances the fixture's one active send by one simulated poll, per
 * `source` (each scenario macro uses a distinct, recognizable source so one
 * real macro row exercises one Phase 9 exit-gate terminal state).
 */
function advanceSend(send) {
  send.pollCount += 1;
  if (send.cancellationRequested) {
    return {
      state: "cancelled",
      actionIndex: send.pollCount,
      error: "",
      releaseError: "",
    };
  }
  if (send.source === "AWAIT_CONFIRM" && send.pollCount < 2) {
    return {
      state: "awaiting_confirmation",
      actionIndex: 0,
      error: "",
      releaseError: "",
    };
  }
  if (send.source === "FAIL") {
    return {
      state: "failed",
      actionIndex: send.pollCount,
      error: "simulated_failure",
      releaseError: "",
    };
  }
  if (send.source === "TIMEOUT") {
    return {
      state: "timed_out",
      actionIndex: send.pollCount,
      error: "",
      releaseError: "",
    };
  }
  if (send.pollCount < 2) {
    return {
      state: "running",
      actionIndex: send.pollCount,
      error: "",
      releaseError: "",
    };
  }
  const releaseError =
    send.source === "RELEASE_ERROR" ? "stuck_key_left_ctrl" : "";
  return { state: "completed", actionIndex: 2, error: "", releaseError };
}

async function startApplicationServer() {
  const dist = new URL("../../dist/", import.meta.url);
  const state = {
    send: null,
    sendPostCount: 0,
    settingsPutCount: 0,
  };

  const server = createServer(async (request, response) => {
    try {
      const url = new URL(request.url ?? "/", "http://127.0.0.1");
      const method = request.method ?? "GET";
      if (url.pathname.startsWith("/api/")) {
        if (method === "GET" && url.pathname === "/api/v1/setup") {
          sendError(response, 404, "not_found", "Already provisioned.");
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/auth/session") {
          sendJson(response, 200, {
            authenticated: true,
            idleExpiresInSeconds: 86400,
            absoluteExpiresInSeconds: 604800,
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/settings") {
          sendJson(response, 200, settings);
          return;
        }
        if (method === "PUT" && url.pathname === "/api/v1/settings") {
          state.settingsPutCount += 1;
          await requestBody(request);
          sendJson(response, 200, {
            settings,
            restartRequired: false,
            reconnectRequired: false,
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/status") {
          sendJson(response, 200, {
            ...status,
            send:
              state.send === null
                ? { present: false, state: null }
                : { present: true, state: state.send.lastState ?? "running" },
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/blob") {
          sendJson(response, 200, {
            blobs: [{ id: blobId, sizeBytes: repositoryGzip.byteLength }],
            usedBytes: repositoryGzip.byteLength,
            remainingBytes: 131072 - repositoryGzip.byteLength,
          });
          return;
        }
        if (method === "GET" && url.pathname === `/api/v1/blob/${blobId}`) {
          response.writeHead(200, {
            "Content-Type": "application/gzip",
            "Content-Length": repositoryGzip.byteLength,
            "Cache-Control": "no-store",
          });
          response.end(repositoryGzip);
          return;
        }
        if (method === "POST" && url.pathname === "/api/v1/send") {
          if (state.send !== null && !isTerminal(state.send.lastState)) {
            sendError(
              response,
              409,
              "already_sending",
              "A send is already in progress.",
            );
            return;
          }
          const body = await requestBody(request);
          assert(
            typeof body?.source === "string" &&
              typeof body?.keyPressMs === "number" &&
              typeof body?.interKeyMs === "number",
            "Browser send request did not match the send contract.",
          );
          state.sendPostCount += 1;
          const initialState =
            body.source === "AWAIT_CONFIRM"
              ? "awaiting_confirmation"
              : "running";
          state.send = {
            id: nextSendId(),
            source: body.source,
            pollCount: 0,
            cancellationRequested: false,
            lastState: initialState,
          };
          sendJson(response, 202, {
            id: state.send.id,
            state: initialState,
            actionCount: 2,
            estimatedDurationMs: 100,
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/send") {
          if (state.send === null) {
            sendError(response, 404, "not_found", "No send since boot.");
            return;
          }
          const progress = advanceSend(state.send);
          state.send.lastState = progress.state;
          sendJson(response, 200, {
            id: state.send.id,
            actionCount: 2,
            estimatedDurationMs: 100,
            cancellationRequested: state.send.cancellationRequested,
            ...progress,
          });
          return;
        }
        if (method === "DELETE" && url.pathname === "/api/v1/send") {
          if (state.send === null) {
            sendError(response, 404, "not_found", "No send since boot.");
            return;
          }
          state.send.cancellationRequested = true;
          sendJson(response, 202, { id: state.send.id });
          return;
        }
        sendError(
          response,
          404,
          "not_found",
          `${method} ${url.pathname} is not implemented by the browser fixture`,
        );
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

function isTerminal(state_) {
  return (
    state_ === "completed" ||
    state_ === "cancelled" ||
    state_ === "failed" ||
    state_ === "timed_out"
  );
}

async function devToolsUrl(processHandle) {
  return new Promise((resolve, reject) => {
    let output = "";
    const timeout = setTimeout(() => {
      reject(new Error(`Chrome did not expose DevTools. Output:\n${output}`));
    }, 30_000);
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
      // Unsolicited CDP events (no `id`) include Page.javascriptDialogOpening.
      // TODO_V2 V2-103 registers a real `beforeunload` listener while the
      // working copy is dirty, which fires a native, unstyleable browser
      // dialog on reload/navigation-away. A native dialog blocks the page's
      // JS thread until dismissed; with no auto-responder here, every
      // subsequent Runtime.evaluate() call hangs forever rather than failing
      // fast. Auto-accept any dialog so scenarios that intentionally leave
      // the store dirty (e.g. reorder-then-reload) don't wedge the harness --
      // no current scenario asserts on the dialog's own presence, only on
      // app state before/after it.
      if (message.method === "Page.javascriptDialogOpening") {
        this.send("Page.handleJavaScriptDialog", { accept: true }).catch(() => {
          // Best-effort: if the socket is already closing, ignore it.
        });
        return;
      }
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

function buttonByAriaLabelExpression(label) {
  return `Array.from(document.querySelectorAll('button')).find((element) => element.getAttribute('aria-label') === ${JSON.stringify(label)})`;
}

async function clickExpression(
  cdp,
  expression,
  description,
  timeoutMs = 2_000,
) {
  // Single-shot evaluation raced a genuine render-timing gap: a preceding
  // waitFor() on nearby text content succeeding does not guarantee this
  // button has mounted in the same paint (React can render the text and the
  // button in separate commits). Retry like waitFor() does, rather than
  // asserting the DOM is already settled the instant the text check passes.
  const deadline = Date.now() + timeoutMs;
  let found = false;
  do {
    found = await evaluate(
      cdp,
      `(() => { const button = ${expression}; if (!button) return false; button.click(); return true; })()`,
    );
    if (found) {
      return;
    }
    await new Promise((resolve) => setTimeout(resolve, 50));
  } while (Date.now() < deadline);
  assert(found, `Missing browser button: ${description}`);
}

async function clickButton(cdp, text) {
  await clickExpression(cdp, buttonExpression(text), text);
}

async function clickButtonByAriaLabel(cdp, label) {
  await clickExpression(cdp, buttonByAriaLabelExpression(label), label);
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

/**
 * TODO_V2 Phase 9 exit gate: "Macros page browser tests cover idle, USB
 * unavailable, quick send, confirmation, progress, cancel, complete,
 * failure, timeout, release error, reload, and rapid repeated input."
 *
 * Covered here against the real built app and a real gzip-compressed
 * repository blob: idle, quick send, progress, confirmation, complete,
 * cancel, failure, timeout, release error, and reload recovery.
 *
 * Deliberately NOT covered here (see docs/implementation-v2 report for
 * why): USB unavailable (would need a slow, real 5-second device-status
 * poll cycle to flip mid-test) and rapid repeated input (a same-tick
 * double-dispatch race is not reproducible over a real CDP round trip,
 * whose latency alone exceeds React's synchronous re-render time — the
 * Vitest suite proves the guard deterministically instead).
 */
async function runBrowserWorkflows(cdp, serverState) {
  await waitFor(
    cdp,
    "document.body?.innerText.includes('Lab bench workflow') ?? false",
    "The Macros page did not load the real gzip-decoded repository.",
  );
  await waitFor(
    cdp,
    "document.body.innerText.includes('Open terminal')",
    "The macro list did not render.",
  );
  await assertTouchTargets(cdp);
  await assertResponsiveLayout(cdp);

  // Accessible reordering (V2-091): a real keyboard Enter press on the
  // "Move down" control — not just a mouse click — actually reorders.
  await evaluate(
    cdp,
    `document.querySelector('[aria-label="Move Open terminal down"]').focus()`,
  );
  await dispatchKey(cdp, "Enter");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Moved Open terminal to position 2.')",
    "Keyboard Enter on Move down did not reorder the macro list.",
  );
  await evaluate(
    cdp,
    `document.querySelector('[aria-label="Move Open terminal up"]').focus()`,
  );
  await dispatchKey(cdp, "Enter");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Moved Open terminal to position 1.')",
    "Keyboard Enter on Move up did not reorder the macro list back.",
  );

  // Macro source is hidden by default, with a temporary per-row reveal
  // (V2-092).
  assert(
    await evaluate(cdp, "document.body.innerText.includes('Source hidden')"),
    "Macro source was not hidden by default.",
  );
  assert(
    (await evaluate(cdp, "document.querySelectorAll('code').length")) === 0,
    "A macro source appeared in the DOM before being revealed.",
  );
  await clickButtonByAriaLabel(cdp, "Reveal source for Open terminal");
  const revealedSource = await evaluate(
    cdp,
    "document.querySelector('code')?.textContent ?? null",
  );
  assert(
    revealedSource === "a",
    `Revealing source did not show the macro source: ${String(revealedSource)}`,
  );
  await clickButtonByAriaLabel(cdp, "Hide source for Open terminal");
  assert(
    (await evaluate(cdp, "document.querySelectorAll('code').length")) === 0,
    "Hide source did not remove the revealed source from the DOM.",
  );

  // Quick Send: progress, then a completion acknowledgement that clears
  // itself (V2-093).
  await clickButtonByAriaLabel(cdp, "Send Open terminal");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Sending Open terminal')",
    "Quick Send did not show inline progress.",
  );
  await waitFor(
    cdp,
    "document.body.innerText.includes('Sent Open terminal.')",
    "Quick Send did not reach a completion acknowledgement.",
  );
  await waitFor(
    cdp,
    "!document.body.innerText.includes('Sent Open terminal.')",
    "The completion acknowledgement did not clear itself.",
    8_000,
  );
  assert(
    serverState.sendPostCount === 1,
    `Expected exactly one POST /api/v1/send for the completed send, got ${String(serverState.sendPostCount)}.`,
  );

  // Serial-confirmation waiting state, inline (UI_UX_SPEC_V2 §5.5).
  await clickButtonByAriaLabel(cdp, "Send Confirm before typing");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Waiting for physical confirmation')",
    "The confirmation-wait state was not shown inline.",
  );
  await waitFor(
    cdp,
    "document.body.innerText.includes('Sent Confirm before typing.')",
    "The confirmed send did not complete.",
  );
  await waitFor(
    cdp,
    "!document.body.innerText.includes('Sent Confirm before typing.')",
    "The confirmation completion acknowledgement did not clear itself.",
    8_000,
  );

  // Cancel and release all keys.
  await clickButtonByAriaLabel(cdp, "Send Open terminal");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Sending Open terminal')",
    "Progress did not show before cancelling.",
  );
  await clickButton(cdp, "Cancel and release all keys");
  await waitFor(
    cdp,
    "document.body.innerText.includes('was cancelled.')",
    "Cancellation did not produce a persistent acknowledgement.",
  );
  await clickButton(cdp, "Dismiss");
  await waitFor(
    cdp,
    "!document.body.innerText.includes('was cancelled.')",
    "Dismiss did not clear the cancellation banner.",
  );

  // Failure, with the exact error and a persistent banner until dismissed.
  await clickButtonByAriaLabel(cdp, "Send Trigger failure");
  await waitFor(
    cdp,
    "document.body.innerText.includes('failed: simulated_failure')",
    "The failure state was not shown with its exact error.",
  );
  await clickButton(cdp, "Dismiss");
  await waitFor(
    cdp,
    "!document.body.innerText.includes('failed: simulated_failure')",
    "Dismiss did not clear the failure banner.",
  );

  // Timeout, persistent until dismissed.
  await clickButtonByAriaLabel(cdp, "Send Trigger timeout");
  await waitFor(
    cdp,
    "document.body.innerText.includes('timed out.')",
    "The timeout state was not shown.",
  );
  await clickButton(cdp, "Dismiss");

  // Release error, reported separately from the completion it rode in on.
  await clickButtonByAriaLabel(cdp, "Send Trigger release error");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Key release failed: stuck_key_left_ctrl')",
    "The release error was not reported.",
  );
  const releaseErrorButtons = await evaluate(
    cdp,
    "Array.from(document.querySelectorAll('button')).filter((element) => element.textContent.trim() === 'Dismiss').length",
  );
  assert(
    releaseErrorButtons >= 1,
    "The release-error banner did not offer its own Dismiss control.",
  );
  await clickButton(cdp, "Dismiss");
  // The release error is a separate, independently dismissible banner from
  // the completion acknowledgement it rode in on (UI_UX_SPEC_V2 §5.5); Send
  // controls stay disabled until that acknowledgement itself clears.
  await waitFor(
    cdp,
    "!document.body.innerText.includes('Sent Trigger release error.')",
    "The completion acknowledgement behind the release error did not clear.",
    8_000,
  );

  // Reload recovery (V2-095/UI_UX_SPEC_V2 §5.6): start a send, reload before
  // it finishes, and confirm React resumes tracking from `GET /api/v1/send`
  // instead of losing the in-progress state.
  await clickButtonByAriaLabel(cdp, "Send Open terminal");
  await waitFor(
    cdp,
    "document.body.innerText.includes('Sending Open terminal')",
    "Progress did not show before reload.",
  );
  await cdp.send("Page.reload", { ignoreCache: false });
  await waitFor(
    cdp,
    "document.body?.innerText.includes('Lab bench workflow') ?? false",
    "The app did not reload back to the Macros page.",
    15_000,
  );
  await waitFor(
    cdp,
    "document.body.innerText.includes('Sending…') || document.body.innerText.includes('Sent.')",
    "Send state was not recovered after reload.",
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
    cdp = await connectPage(debuggerUrl, application.baseUrl);
    await runBrowserWorkflows(cdp, application.state);
    console.log("Real Chrome v2 Macros page/Quick Send workflows passed.");
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
