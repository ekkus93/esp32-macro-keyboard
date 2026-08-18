import { createServer } from "node:http";
import { readFile, stat } from "node:fs/promises";
import { normalize } from "node:path";

import { blobId, nextSendId, repositoryGzip, settings } from "./data.mjs";
import { advanceSend, isTerminal } from "./sendModel.mjs";
import {
  assert,
  contentType,
  rawRequestBody,
  requestBody,
  sendError,
  sendJson,
} from "../lib/http.mjs";

export async function startStartupFixtureServer(options = {}) {
  const {
    provisioned = true,
    authenticated = true,
    blobs: initialBlobs = [{ id: blobId, bytes: repositoryGzip }],
    usbState: initialUsbState = "ready",
  } = options;

  // Three levels up, not two: this module lives in tests/browser/fixtures/,
  // one deeper than the driver it was split out of. Resolves to webapp/dist/.
  const dist = new URL("../../../dist/", import.meta.url);
  // A disposable, fixture-only credential -- never a real device secret;
  // only this in-memory Node server ever checks it.
  const adminPassword = "bench-fixture-admin-pw-1";

  const state = {
    provisioned,
    authenticated,
    adminPassword,
    settings: { ...settings },
    blobs: initialBlobs.map((blob) => ({ ...blob })),
    nextBlobId:
      initialBlobs.reduce((max, blob) => Math.max(max, Number(blob.id)), 0) + 1,
    usbState: initialUsbState,
    send: null,
    sendPostCount: 0,
    // Every /api/ request in order, as "METHOD /path" -- lets a scenario
    // assert the exact startup fetch sequence re-ran (e.g. after a reload),
    // not just that the resulting page text looks right.
    requestLog: [],
  };

  const server = createServer(async (request, response) => {
    try {
      const url = new URL(request.url ?? "/", "http://127.0.0.1");
      const method = request.method ?? "GET";
      if (url.pathname.startsWith("/api/")) {
        state.requestLog.push(`${method} ${url.pathname}`);

        if (method === "GET" && url.pathname === "/api/v1/setup") {
          if (state.provisioned) {
            sendError(response, 404, "not_found", "Already provisioned.");
          } else {
            sendJson(response, 200, {
              provisioned: false,
              deviceName: "Unconfigured Bench Keyboard",
            });
          }
          return;
        }
        if (method === "POST" && url.pathname === "/api/v1/setup") {
          const body = await requestBody(request);
          assert(
            body !== null &&
              typeof body.setupCode === "string" &&
              typeof body.deviceName === "string" &&
              typeof body.apSsid === "string" &&
              typeof body.apPassphrase === "string" &&
              typeof body.adminPassword === "string" &&
              typeof body.requireSerialConfirmation === "boolean",
            "Browser setup request did not match the setup contract.",
          );
          state.provisioned = true;
          state.settings.deviceName = body.deviceName;
          state.settings.apSsid = body.apSsid;
          state.settings.requireSerialConfirmation =
            body.requireSerialConfirmation;
          state.adminPassword = body.adminPassword;
          sendJson(response, 200, {
            accepted: true,
            connectionWillClose: true,
            reprovisioningRequired: false,
            restartRequired: true,
          });
          return;
        }
        if (method === "GET" && url.pathname === "/api/v1/auth/session") {
          if (state.authenticated) {
            sendJson(response, 200, {
              authenticated: true,
              idleExpiresInSeconds: 86400,
              absoluteExpiresInSeconds: 604800,
            });
          } else {
            sendError(response, 401, "unauthorized", "No active session.");
          }
          return;
        }
        if (method === "POST" && url.pathname === "/api/v1/auth/login") {
          const body = await requestBody(request);
          if (body?.adminPassword === state.adminPassword) {
            state.authenticated = true;
            sendJson(response, 200, {
              authenticated: true,
              idleExpiresInSeconds: 86400,
              absoluteExpiresInSeconds: 604800,
            });
          } else {
            sendError(
              response,
              401,
              "invalid_credentials",
              "Incorrect administrator password.",
            );
          }
          return;
        }

        // Every route below requires the fixture's own session flag --
        // mirrors the real device rejecting authenticated routes once a
        // session ends (TODO_V2 Phase 8 "expired session").
        if (!state.authenticated) {
          sendError(response, 401, "unauthorized", "Session expired.");
          return;
        }

        if (method === "POST" && url.pathname === "/api/v1/device/restart") {
          // Two simulated-down polls before the ordinary auth gate takes
          // over -- see the "waiting" case in /api/v1/status above.
          state.rebootPollsRemaining = 2;
          sendJson(response, 202, {
            accepted: true,
            connectionWillClose: true,
            reprovisioningRequired: false,
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
            if (typeof body.deviceName === "string") {
              state.settings.deviceName = body.deviceName;
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
          // Simulates the device actually being down after `POST
          // /api/v1/device/restart` -- useDeviceReconnect.ts treats a
          // transient 503 the same as a dropped connection ("still down,
          // keep polling"), which is the closest an HTTP fixture can get to
          // a real TCP-level outage. Once the simulated downtime ends,
          // dropping `state.authenticated` lets the ordinary auth gate
          // above answer the next poll with 401 -- the real device's RAM-
          // only sessions do not survive a reboot -- which is what drives
          // DeviceReconnectScreen from "waiting" to "needs-reauth" without
          // this route needing its own special-cased response.
          if (
            state.rebootPollsRemaining !== undefined &&
            state.rebootPollsRemaining > 0
          ) {
            state.rebootPollsRemaining -= 1;
            if (state.rebootPollsRemaining === 0) {
              state.authenticated = false;
            }
            sendError(
              response,
              503,
              "temporarily_unavailable",
              "Device is restarting.",
            );
            return;
          }
          const usedBytes = state.blobs.reduce(
            (sum, blob) => sum + blob.bytes.byteLength,
            0,
          );
          sendJson(response, 200, {
            provisioned: true,
            deviceName: state.settings.deviceName,
            firmwareVersion: "0.2.0",
            buildId: "browser-startup-fixture",
            uptimeMs: 1000,
            usb: { state: state.usbState },
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
          const id = String(state.nextBlobId);
          state.nextBlobId += 1;
          state.blobs.push({ id, bytes });
          sendJson(response, 201, { id, sizeBytes: bytes.byteLength });
          return;
        }
        const blobIdMatch = /^\/api\/v1\/blob\/([^/]+)$/.exec(url.pathname);
        if (blobIdMatch !== null && method === "GET") {
          const id = decodeURIComponent(blobIdMatch[1]);
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
          state.sendPostCount += 1;
          const initialState =
            body?.source === "AWAIT_CONFIRM"
              ? "awaiting_confirmation"
              : "running";
          state.send = {
            id: nextSendId(),
            source: body?.source,
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

        sendError(
          response,
          404,
          "not_found",
          `${method} ${url.pathname} is not implemented by the startup fixture`,
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

/**
 * Runs `fn` in the browser context via Playwright's own `page.evaluate()`.
 * `fn` must be a real function (not a string): Playwright serializes its
 * source and executes it in the page, so it can only close over values
 * passed explicitly through `arg`, never over this module's variables.
 */
