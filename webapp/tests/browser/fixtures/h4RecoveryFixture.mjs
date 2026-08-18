import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { normalize } from "node:path";
import { gzipSync } from "node:zlib";

import {
  assert,
  contentType,
  requestBody,
  sendError,
  sendJson,
} from "../lib/http.mjs";

/**
 * The H4 degraded-send-recovery fixture, extracted from
 * `run-h4-recovery-tests.mjs` so `visual/scenarios.mjs`'s
 * `execution-recovery-overlay` scenario can reach the same state without
 * duplicating this server. Behaviourally identical to the fixture
 * `run-h4-recovery-tests.mjs` used inline before this extraction: same
 * package/macro IDs, same three-failed-poll-then-degrade sequence, same
 * routes. `run-h4-recovery-tests.mjs` now imports this instead of defining
 * it -- see that file for the workflow this fixture was built to drive.
 */
export const packageId = "11111111-1111-4111-8111-111111111111";
export const macroId = "22222222-2222-4222-8222-222222222222";
export const sendId = "77777777-7777-4777-8777-000000000001";
export const blobId = "1";

export const repository = {
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
        {
          id: "33333333-3333-4333-8333-333333333333",
          name: "Second macro",
          source: "b",
          keyPressMs: 8,
          interKeyMs: 15,
        },
      ],
    },
  ],
};
export const repositoryGzip = gzipSync(
  Buffer.from(JSON.stringify(repository), "utf8"),
);
export const settings = {
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

export async function startH4RecoveryFixture() {
  const dist = new URL("../../../dist/", import.meta.url);
  const state = {
    send: null,
    sendPostCount: 0,
    failedPollsRemaining: 0,
    failFirstSendGetAfterNextDocument: false,
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
      if (
        rawPath === "/index.html" &&
        state.failFirstSendGetAfterNextDocument
      ) {
        state.failFirstSendGetAfterNextDocument = false;
        state.failedPollsRemaining = 1;
      }
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
