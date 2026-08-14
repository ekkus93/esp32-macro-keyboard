import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { extname, normalize } from "node:path";
import { gzipSync } from "node:zlib";
import { chromium } from "playwright";

const packageId = "11111111-1111-4111-8111-111111111111";
const macroId = "22222222-2222-4222-8222-222222222222";
const sendId = "77777777-7777-4777-8777-000000000001";
const blobId = "1";

const repository = {
  format: "esp32-macro-keyboard-repository",
  schemaVersion: 1,
  packages: [
    {
      id: packageId,
      name: "H4 recovery package",
      macros: [
        {
          id: macroId,
          name: "Recovery macro",
          source: "a",
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
  deviceName: "H4 Browser Fixture",
  requireSerialConfirmation: false,
  sendMode: "quick",
  snapshotRetentionTarget: 5,
  showMacroSourcePreviews: false,
  lastSelectedPackageId: packageId,
  apSsid: "MacroKeyboard",
  stationConfigured: false,
  stationSsid: null,
};

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function sendJson(response, status, value) {
  const body = JSON.stringify(value);
  response.writeHead(status, {
    "Content-Type": "application/json; charset=utf-8",
    "Content-Length": Buffer.byteLength(body),
    "Cache-Control": "no-store",
  });
  response.end(body);
}

function sendError(response, status, code, message) {
  sendJson(response, status, { error: { code, message } });
}

function contentType(path) {
  switch (extname(path)) {
    case ".css":
      return "text/css; charset=utf-8";
    case ".html":
      return "text/html; charset=utf-8";
    case ".js":
      return "text/javascript; charset=utf-8";
    default:
      return "application/octet-stream";
  }
}

async function requestBody(request) {
  const chunks = [];
  for await (const chunk of request) {
    chunks.push(chunk);
  }
  const text = Buffer.concat(chunks).toString("utf8");
  return text.length === 0 ? null : JSON.parse(text);
}

async function startFixture() {
  const dist = new URL("../../dist/", import.meta.url);
  const state = {
    send: null,
    sendPostCount: 0,
    failedPollsRemaining: 0,
    completed: false,
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
        if (method === "GET" && url.pathname === "/api/v1/status") {
          sendJson(response, 200, {
            provisioned: true,
            deviceName: settings.deviceName,
            firmwareVersion: "0.2.0",
            buildId: "h4-browser-fixture",
            uptimeMs: 1000,
            usb: { state: "ready" },
            accessPoint: {
              state: "started",
              ssid: settings.apSsid,
              clientCount: 1,
            },
            station: {
              configured: false,
              state: "idle",
              ssid: null,
              ipv4: null,
            },
            storage: {
              state: "healthy",
              totalBytes: 131072,
              usedBytes: repositoryGzip.byteLength,
              remainingBytes: 131072 - repositoryGzip.byteLength,
              blobCount: 1,
            },
            send:
              state.send === null
                ? { present: false, state: null }
                : {
                    present: true,
                    state: state.completed ? "completed" : "running",
                  },
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
          const body = await requestBody(request);
          assert(
            body?.source === "a",
            "H4 browser fixture got unexpected send source.",
          );
          state.sendPostCount += 1;
          state.send = { id: sendId };
          state.failedPollsRemaining = 3;
          sendJson(response, 202, {
            id: sendId,
            state: "running",
            actionCount: 1,
            estimatedDurationMs: 100,
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/send") {
          if (state.send === null) {
            sendError(response, 404, "not_found", "No send since boot.");
            return;
          }
          if (state.failedPollsRemaining > 0) {
            state.failedPollsRemaining -= 1;
            sendError(
              response,
              503,
              "temporarily_unavailable",
              "Send status is temporarily unavailable.",
            );
            return;
          }
          const terminal = state.completed;
          state.completed = true;
          sendJson(response, 200, {
            id: sendId,
            state: terminal ? "completed" : "running",
            actionIndex: terminal ? 1 : 0,
            actionCount: 1,
            estimatedDurationMs: 100,
            cancellationRequested: false,
            error: "",
            releaseError: "",
          });
          return;
        }
        if (method === "DELETE" && url.pathname === "/api/v1/send") {
          state.completed = true;
          sendJson(response, 202, { id: sendId });
          return;
        }
        sendError(response, 404, "not_found", "Fixture route not found.");
        return;
      }

      const rawPath = url.pathname === "/" ? "/index.html" : url.pathname;
      const safePath = normalize(rawPath).replace(/^([.][.][/\\])+/, "");
      const file = new URL(`.${safePath}`, dist);
      try {
        const bytes = await readFile(file);
        response.writeHead(200, {
          "Content-Type": contentType(file.pathname),
          "Content-Length": bytes.byteLength,
        });
        response.end(bytes);
      } catch {
        const bytes = await readFile(new URL("index.html", dist));
        response.writeHead(200, {
          "Content-Type": "text/html; charset=utf-8",
          "Content-Length": bytes.byteLength,
        });
        response.end(bytes);
      }
    } catch (error) {
      sendError(
        response,
        500,
        "fixture_error",
        error instanceof Error ? error.message : String(error),
      );
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
    state,
    close: () => new Promise((resolve) => server.close(resolve)),
  };
}

async function waitFor(page, predicate, message, timeout = 12_000) {
  await page.waitForFunction(predicate, undefined, { timeout }).catch(() => {
    throw new Error(message);
  });
}

const fixture = await startFixture();
const browser = await chromium.launch({ headless: true });
const context = await browser.newContext({
  viewport: { width: 390, height: 844 },
});
const page = await context.newPage();
try {
  await page.goto(fixture.baseUrl);
  await waitFor(
    page,
    () => document.body?.innerText.includes("H4 recovery package") ?? false,
    "H4 fixture did not reach the Macros page.",
  );

  await page.getByRole("button", { name: "Send Recovery macro" }).click();
  await waitFor(
    page,
    () =>
      document.body?.innerText.includes("Execution state unavailable") ?? false,
    "Persistent status failures did not expose degraded execution state.",
  );
  assert(
    fixture.state.sendPostCount === 1,
    "The initial send must POST exactly once.",
  );
  const recovery = page.getByLabel("Execution recovery");
  assert(
    await recovery
      .getByRole("button", { name: "Cancel and release all keys" })
      .isVisible(),
    "Cancel was not visible while execution status was unavailable.",
  );

  await recovery
    .getByRole("button", { name: "Retry execution status" })
    .click();
  await waitFor(
    page,
    () =>
      !(
        document.body?.innerText.includes("Execution state unavailable") ?? true
      ),
    "Successful status reconciliation did not clear the degraded warning.",
  );
  assert(
    fixture.state.sendPostCount === 1,
    "Execution-status Retry must not POST the macro again.",
  );

  console.log("Real Chrome H4 degraded-send recovery workflow passed.");
} finally {
  await context.close();
  await browser.close();
  await fixture.close();
}
