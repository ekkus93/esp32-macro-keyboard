import { createServer } from "node:http";
import { readFile, stat } from "node:fs/promises";
import { extname, normalize } from "node:path";
import process from "node:process";
import { gunzipSync, gzipSync } from "node:zlib";
import { chromium } from "playwright";

const packageId = "11111111-1111-4111-8111-111111111111";
const macroCompleteId = "22222222-2222-4222-8222-222222222222";
const macroConfirmId = "66666666-6666-4666-8666-666666666666";

const repository = {
  format: "esp32-macro-keyboard-repository",
  schemaVersion: 1,
  packages: [
    {
      id: packageId,
      name: "H5 reconciliation bench",
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
          source: "b",
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
  deviceName: "H5 Reconciliation Keyboard",
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

function sendError(response, status, code, message) {
  sendJson(response, status, { error: { code, message } });
}

async function requestBody(request) {
  const chunks = [];
  for await (const chunk of request) {
    chunks.push(chunk);
  }
  const text = Buffer.concat(chunks).toString("utf8");
  return text.length === 0 ? null : JSON.parse(text);
}

async function rawRequestBody(request) {
  const chunks = [];
  for await (const chunk of request) {
    chunks.push(chunk);
  }
  return Buffer.concat(chunks);
}

async function startFixtureServer() {
  const dist = new URL("../../dist/", import.meta.url);
  const state = {
    settings: { ...settings },
    blobs: [{ id: "1", bytes: repositoryGzip }],
    nextBlobId: 2,
    blobPostCount: 0,
    blobListGetCount: 0,
    blobDownloadGetCount: 0,
    uncertainResponseSent: false,
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
          sendJson(response, 200, state.settings);
          return;
        }
        if (method === "PUT" && url.pathname === "/api/v1/settings") {
          const body = await requestBody(request);
          if (body !== null && typeof body === "object") {
            if (Object.hasOwn(body, "lastSelectedPackageId")) {
              state.settings.lastSelectedPackageId = body.lastSelectedPackageId;
            }
          }
          sendJson(response, 200, {
            settings: state.settings,
            restartRequired: false,
            reconnectRequired: false,
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/status") {
          const usedBytes = state.blobs.reduce(
            (sum, blob) => sum + blob.bytes.byteLength,
            0,
          );
          sendJson(response, 200, {
            provisioned: true,
            deviceName: state.settings.deviceName,
            firmwareVersion: "0.2.0",
            buildId: "h5-browser-fixture",
            uptimeMs: 1000,
            usb: { state: "ready" },
            accessPoint: {
              state: "started",
              ssid: state.settings.apSsid,
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
              usedBytes,
              remainingBytes: 131072 - usedBytes,
              blobCount: state.blobs.length,
            },
            send: { present: false, state: null },
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/blob") {
          state.blobListGetCount += 1;
          const usedBytes = state.blobs.reduce(
            (sum, blob) => sum + blob.bytes.byteLength,
            0,
          );
          sendJson(response, 200, {
            blobs: [...state.blobs]
              .sort((left, right) => Number(right.id) - Number(left.id))
              .map((blob) => ({
                id: blob.id,
                sizeBytes: blob.bytes.byteLength,
              })),
            usedBytes,
            remainingBytes: 131072 - usedBytes,
          });
          return;
        }
        if (method === "POST" && url.pathname === "/api/v1/blob") {
          const bytes = await rawRequestBody(request);
          state.blobPostCount += 1;
          const id = String(state.nextBlobId);
          state.nextBlobId += 1;
          state.blobs.push({ id, bytes: Buffer.from(bytes) });

          if (!state.uncertainResponseSent) {
            state.uncertainResponseSent = true;
            sendError(
              response,
              503,
              "commit_uncertain",
              "Blob activation succeeded but durability is uncertain.",
            );
            return;
          }

          sendJson(response, 201, { id, sizeBytes: bytes.byteLength });
          return;
        }

        const blobMatch = /^\/api\/v1\/blob\/([^/]+)$/.exec(url.pathname);
        if (method === "GET" && blobMatch !== null) {
          state.blobDownloadGetCount += 1;
          const id = decodeURIComponent(blobMatch[1]);
          const blob = state.blobs.find((candidate) => candidate.id === id);
          if (blob === undefined) {
            sendError(response, 404, "not_found", "No such blob.");
            return;
          }
          response.writeHead(200, {
            "Content-Type": "application/gzip",
            "Content-Length": blob.bytes.byteLength,
            "Cache-Control": "no-store",
          });
          response.end(blob.bytes);
          return;
        }

        if (method === "GET" && url.pathname === "/api/v1/send") {
          sendError(response, 404, "not_found", "No send since boot.");
          return;
        }

        sendError(
          response,
          404,
          "not_found",
          `${method} ${url.pathname} is not implemented by the H5 fixture`,
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
          code: "h5_browser_fixture_error",
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
    "H5 fixture server did not expose a port.",
  );

  return {
    baseUrl: `http://127.0.0.1:${String(address.port)}`,
    close: () => new Promise((resolve) => server.close(resolve)),
    state,
  };
}

async function waitFor(page, predicate, message, timeoutMs = 12_000) {
  try {
    await page.waitForFunction(predicate, undefined, {
      timeout: timeoutMs,
      polling: 50,
    });
  } catch (error) {
    if (!(error instanceof Error) || error.name !== "TimeoutError") {
      throw error;
    }
    const body = await page.evaluate(() => document.body.innerText);
    throw new Error(`${message}\nCurrent page:\n${body}`);
  }
}

async function clickButton(page, name) {
  await page.getByRole("button", { name, exact: true }).first().click();
}

async function main() {
  const fixture = await startFixtureServer();
  const browser = await chromium.launch({ headless: true });
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
  });
  const page = await context.newPage();

  page.on("dialog", (dialog) => {
    dialog.accept().catch(() => {
      // Best-effort cleanup if the dirty-page beforeunload dialog races close.
    });
  });

  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      () => document.body?.innerText.includes("H5 reconciliation bench") ?? false,
      "H5 fixture did not reach the Macros page.",
    );

    await page.locator('[aria-label="Move Open terminal down"]').press("Enter");
    await waitFor(
      page,
      () => document.body.innerText.includes("Unsaved changes"),
      "Reordering did not dirty the working copy before the uncertain save.",
    );

    const listGetsBeforeFirstSave = fixture.state.blobListGetCount;
    const downloadsBeforeFirstSave = fixture.state.blobDownloadGetCount;
    await clickButton(page, "Save snapshot");
    await waitFor(
      page,
      () =>
        document.body.innerText.includes(
          "No duplicate upload was sent. The working copy remains dirty",
        ),
      "The first uncertain save did not surface the matched reconciliation result.",
    );

    assert(
      fixture.state.blobPostCount === 1,
      `The first Save produced ${String(fixture.state.blobPostCount)} blob POSTs instead of one.`,
    );
    assert(
      fixture.state.blobs.length === 2,
      `The uncertain activation should leave exactly one new canonical blob; found ${String(fixture.state.blobs.length - 1)} new blobs.`,
    );
    assert(
      fixture.state.blobListGetCount >= listGetsBeforeFirstSave + 2,
      "The uncertain Save did not perform both the pre-create list and post-error reconciliation list.",
    );
    assert(
      fixture.state.blobDownloadGetCount >= downloadsBeforeFirstSave + 1,
      "The uncertain Save did not download the newly observed blob for exact-byte reconciliation.",
    );

    const activated = fixture.state.blobs.find((blob) => blob.id === "2");
    assert(activated !== undefined, "The uncertain activation did not retain blob 2.");
    const activatedRepository = JSON.parse(
      gunzipSync(activated.bytes).toString("utf8"),
    );
    assert(
      activatedRepository.packages?.[0]?.macros?.[0]?.name ===
        "Confirm before typing",
      "The retained uncertain blob did not contain the exact reordered working copy.",
    );

    const listGetsBeforeSecondSave = fixture.state.blobListGetCount;
    const downloadsBeforeSecondSave = fixture.state.blobDownloadGetCount;
    await clickButton(page, "Save snapshot");
    await waitFor(
      page,
      () => document.body.innerText.includes("Unsaved changes"),
      "The second Save attempt unexpectedly cleared dirty state.",
    );
    await waitFor(
      page,
      () => document.body.innerText.includes("No duplicate upload was sent."),
      "The second Save attempt did not remain in commit-uncertain reconciliation.",
    );

    const reconciliationDeadline = Date.now() + 5_000;
    while (
      fixture.state.blobListGetCount === listGetsBeforeSecondSave &&
      Date.now() < reconciliationDeadline
    ) {
      await new Promise((resolve) => setTimeout(resolve, 25));
    }

    assert(
      fixture.state.blobListGetCount > listGetsBeforeSecondSave,
      "The second Save did not perform GET-only reconciliation.",
    );
    assert(
      fixture.state.blobDownloadGetCount > downloadsBeforeSecondSave,
      "The second Save did not re-check the candidate blob bytes.",
    );
    assert(
      fixture.state.blobPostCount === 1,
      `A second Save after commit uncertainty issued ${String(fixture.state.blobPostCount)} total blob POSTs; expected exactly one.`,
    );
    assert(
      fixture.state.blobs.length === 2,
      "A second Save after commit uncertainty silently created a duplicate blob.",
    );

    console.log(
      "Real Chrome H5 commit-uncertain reconciliation/no-duplicate-POST workflow passed.",
    );
  } finally {
    await context.close();
    await browser.close();
    await fixture.close();
  }
}

main().catch((error) => {
  console.error(error instanceof Error ? error.stack : error);
  process.exitCode = 1;
});
