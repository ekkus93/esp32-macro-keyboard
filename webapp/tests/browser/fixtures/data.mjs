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
export const packageId = "11111111-1111-4111-8111-111111111111";
export const macroCompleteId = "22222222-2222-4222-8222-222222222222";
export const macroFailId = "33333333-3333-4333-8333-333333333333";
export const macroTimeoutId = "44444444-4444-4444-8444-444444444444";
export const macroReleaseErrorId = "55555555-5555-4555-8555-555555555555";
export const macroConfirmId = "66666666-6666-4666-8666-666666666666";
// A fresh, real-looking UUID v4 per send (not a fixed constant): the client
// (correctly) treats a repeated send ID as an already-observed completion,
// exactly as it must to satisfy TODO_V2 V2-095 "prevent duplicate completion
// callbacks" — a fixture reusing one ID across sends would silently hide
// every send after the first instead of exercising that guard honestly.
let sendCounter = 0;
export function nextSendId() {
  sendCounter += 1;
  const suffix = String(sendCounter).padStart(12, "0");
  return `77777777-7777-4777-8777-${suffix}`;
}
export const blobId = "1";

export const repository = {
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
export const repositoryGzip = gzipSync(
  Buffer.from(JSON.stringify(repository), "utf8"),
);

export const settings = {
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

export const status = {
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
