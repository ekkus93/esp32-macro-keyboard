import type { DiagnosticsResponse } from "./apiTypes";

/**
 * Rebuilds Diagnostics' copy/download text field-by-field from the typed
 * response rather than `JSON.stringify`-ing whatever object the fetch layer
 * handed back — the explicit field list here is what makes "filtered"
 * (TODO_V2 V2-122 "provide copy/download only after filtering sensitive
 * content") a real, testable property instead of a claim. `isDiagnosticsResponse`
 * (`apiGuards.ts`) already enforces the exact key set at every level of the
 * response before it reaches React state, so this is defense in depth, not
 * the only guard.
 */
export function buildDiagnosticsExportText(
  diagnostics: DiagnosticsResponse,
): string {
  const payload = {
    firmwareVersion: diagnostics.firmwareVersion,
    buildId: diagnostics.buildId,
    resetReason: diagnostics.resetReason,
    uptimeMs: diagnostics.uptimeMs,
    memory: {
      freeHeapBytes: diagnostics.memory.freeHeapBytes,
      minimumFreeHeapBytes: diagnostics.memory.minimumFreeHeapBytes,
      largestFreeBlockBytes: diagnostics.memory.largestFreeBlockBytes,
    },
    usb: { state: diagnostics.usb.state },
    wifi: {
      accessPointState: diagnostics.wifi.accessPointState,
      stationState: diagnostics.wifi.stationState,
    },
    storage: {
      state: diagnostics.storage.state,
      webfsTotalBytes: diagnostics.storage.webfsTotalBytes,
      webfsUsedBytes: diagnostics.storage.webfsUsedBytes,
      userdataTotalBytes: diagnostics.storage.userdataTotalBytes,
      userdataUsedBytes: diagnostics.storage.userdataUsedBytes,
      blobCount: diagnostics.storage.blobCount,
      invalidNames: [...diagnostics.storage.invalidNames],
      temporaryFiles: [...diagnostics.storage.temporaryFiles],
    },
    send: { present: diagnostics.send.present, state: diagnostics.send.state },
    subsystems: diagnostics.subsystems.map((subsystem) => ({
      name: subsystem.name,
      state: subsystem.state,
    })),
  };
  return JSON.stringify(payload, null, 2);
}
