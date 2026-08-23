import { createServer } from "node:http";
import { readFile, stat } from "node:fs/promises";
import { normalize } from "node:path";

import {
  blobId,
  nextSendId,
  repositoryGzip,
  settings,
  status,
} from "./data.mjs";
import { advanceSend, isTerminal } from "./sendModel.mjs";
import {
  assert,
  contentType,
  rawRequestBody,
  requestBody,
  sendError,
  sendJson,
} from "../lib/http.mjs";

export async function startApplicationServer() {
  // Three levels up, not two: this module lives in tests/browser/fixtures/,
  // one deeper than the driver it was split out of. Resolves to webapp/dist/.
  const dist = new URL("../../../dist/", import.meta.url);
  const state = {
    send: null,
    sendPostCount: 0,
    settingsPutCount: 0,
    // TODO_V2 V2-110/V2-111: a real, mutable multi-blob store — Save/Load/
    // Delete/Import/Export all round-trip through these routes against a
    // real gzip payload the browser itself decompresses
    // (`DecompressionStream`, unavailable to jsdom, so this is the one place
    // that gzip round trip is proven against a real browser).
    blobs: [{ id: blobId, bytes: repositoryGzip }],
    nextBlobId: 2,
    // TODO_V2 V2-120: a mutable copy, not the fixed `settings` fixture
    // above, so the Settings-page browser workflow's device-name edit
    // genuinely round-trips through a subsequent GET.
    settings: { ...settings },
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
          state.settingsPutCount += 1;
          const body = await requestBody(request);
          // A real (if partial) partial-update merge — TODO_V2 V2-120's
          // browser workflow edits the device name and needs the change to
          // actually round-trip through GET afterward, not just echo the
          // fixed fixture back unchanged.
          if (body !== null && typeof body === "object") {
            if (typeof body.deviceName === "string") {
              state.settings.deviceName = body.deviceName;
            }
            if (typeof body.requireSerialConfirmation === "boolean") {
              state.settings.requireSerialConfirmation =
                body.requireSerialConfirmation;
            }
            if (typeof body.sendMode === "string") {
              state.settings.sendMode = body.sendMode;
            }
            if (typeof body.snapshotRetentionTarget === "number") {
              state.settings.snapshotRetentionTarget =
                body.snapshotRetentionTarget;
            }
            if (Object.hasOwn(body, "lastSelectedPackageId")) {
              state.settings.lastSelectedPackageId = body.lastSelectedPackageId;
            }
            if (
              body.accessPoint !== null &&
              typeof body.accessPoint === "object"
            ) {
              state.settings.apSsid = body.accessPoint.ssid;
            }
            if (Object.hasOwn(body, "station")) {
              if (body.station === null) {
                state.settings.stationConfigured = false;
                state.settings.stationSsid = null;
              } else if (typeof body.station === "object") {
                state.settings.stationConfigured = true;
                state.settings.stationSsid = body.station.ssid;
              }
            }
          }
          sendJson(response, 200, {
            settings: state.settings,
            restartRequired: false,
            reconnectRequired: false,
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/diagnostics") {
          sendJson(response, 200, {
            firmwareVersion: "0.2.0",
            buildId: "browser-fixture-abc123",
            resetReason: "power_on",
            uptimeMs: 123456,
            memory: {
              freeHeapBytes: 200000,
              minimumFreeHeapBytes: 180000,
              largestFreeBlockBytes: 120000,
            },
            usb: { state: "ready" },
            wifi: { accessPointState: "running", stationState: "disabled" },
            storage: {
              state: "ready",
              webfsTotalBytes: 1048576,
              webfsUsedBytes: 500000,
              userdataTotalBytes: 131072,
              userdataUsedBytes: state.blobs.reduce(
                (sum, blob) => sum + blob.bytes.byteLength,
                0,
              ),
              blobCount: state.blobs.length,
              invalidNames: [],
              temporaryFiles: [],
            },
            send: { present: false, state: null },
            subsystems: [{ name: "storage", state: "healthy" }],
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
          const usedBytes = state.blobs.reduce(
            (sum, blob) => sum + blob.bytes.byteLength,
            0,
          );
          sendJson(response, 200, {
            blobs: [...state.blobs]
              .sort((a, b) => Number(b.id) - Number(a.id))
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
          if (bytes.byteLength > 131072) {
            sendError(
              response,
              413,
              "payload_too_large",
              "The snapshot exceeds the maximum accepted blob size.",
            );
            return;
          }
          const id = String(state.nextBlobId);
          state.nextBlobId += 1;
          state.blobs.push({ id, bytes });
          sendJson(response, 201, { id, sizeBytes: bytes.byteLength });
          return;
        }
        const blobIdMatch = /^\/api\/v1\/blob\/([^/]+)$/.exec(url.pathname);
        if (blobIdMatch !== null) {
          const id = decodeURIComponent(blobIdMatch[1]);
          const blob = state.blobs.find((candidate) => candidate.id === id);
          if (method === "GET") {
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
          if (method === "DELETE") {
            if (blob === undefined) {
              sendError(response, 404, "not_found", "No such blob.");
              return;
            }
            state.blobs = state.blobs.filter(
              (candidate) => candidate.id !== id,
            );
            response.writeHead(204);
            response.end();
            return;
          }
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
