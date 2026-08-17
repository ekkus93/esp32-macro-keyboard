import { useEffect, useRef, useState } from "react";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import { getDiagnostics as defaultGetDiagnostics } from "../../../v2/diagnosticsClient";
import { buildDiagnosticsExportText } from "../../../v2/diagnosticsExport";
import type { DiagnosticsResponse } from "../../../v2/apiTypes";
import { saveBytesAsFile } from "../../../v2/saveFile";

/**
 * Diagnostics, per SPEC_V2 §13.13/§16.3 and UI_UX_SPEC_V2 §4 screen 15
 * (TODO_V2 V2-122). Renders exactly the fixed `GET /api/v1/diagnostics`
 * schema — firmware/build, uptime, reset reason, memory, USB, Wi-Fi,
 * storage (including invalid/temporary filenames), send state, and
 * subsystem health — and nothing else. There is no package, macro, or
 * credential field in that schema to accidentally render: the response
 * guard (`isDiagnosticsResponse`, `apiGuards.ts`) enforces the exact key set
 * at every level, and `buildDiagnosticsExportText` (`v2/diagnosticsExport.ts`)
 * re-derives Copy/Download's output from the typed fields one at a time
 * rather than passing any raw response through, so Copy/Download can never
 * carry a field this page does not already know about and display.
 */

export interface DiagnosticsPageDependencies {
  getDiagnostics: typeof defaultGetDiagnostics;
  copyToClipboard: (text: string) => Promise<void>;
  saveAsFile: (bytes: Uint8Array, filename: string, mimeType: string) => void;
}

function defaultCopyToClipboard(text: string): Promise<void> {
  return navigator.clipboard.writeText(text);
}

function defaultDependencies(): DiagnosticsPageDependencies {
  return {
    getDiagnostics: defaultGetDiagnostics,
    copyToClipboard: defaultCopyToClipboard,
    saveAsFile: saveBytesAsFile,
  };
}

export interface DiagnosticsPageProps {
  onBack: () => void;
  dependencies?: DiagnosticsPageDependencies;
}

function formatBytes(bytes: number): string {
  return `${String(bytes)} bytes`;
}

export function DiagnosticsPage({
  onBack,
  dependencies,
}: DiagnosticsPageProps): React.JSX.Element {
  const deps = dependencies ?? defaultDependencies();
  const depsRef = useRef(deps);
  depsRef.current = deps;

  const [state, setState] = useState<
    | { kind: "loading" }
    | { kind: "error"; message: string }
    | { kind: "loaded"; diagnostics: DiagnosticsResponse }
  >({ kind: "loading" });
  const [copyState, setCopyState] = useState<
    "idle" | "copying" | "copied" | "error"
  >("idle");
  const [copyError, setCopyError] = useState<string | null>(null);

  const load = (): void => {
    setState({ kind: "loading" });
    depsRef.current
      .getDiagnostics()
      .then((diagnostics) => {
        setState({ kind: "loaded", diagnostics });
      })
      .catch((error: unknown) => {
        setState({ kind: "error", message: v2ErrorText(error) });
      });
  };

  useEffect(load, []);

  const copy = async (diagnostics: DiagnosticsResponse): Promise<void> => {
    setCopyState("copying");
    setCopyError(null);
    try {
      await depsRef.current.copyToClipboard(
        buildDiagnosticsExportText(diagnostics),
      );
      setCopyState("copied");
    } catch (error: unknown) {
      setCopyState("error");
      setCopyError(v2ErrorText(error));
    }
  };

  const download = (diagnostics: DiagnosticsResponse): void => {
    const text = buildDiagnosticsExportText(diagnostics);
    depsRef.current.saveAsFile(
      new TextEncoder().encode(text),
      "diagnostics.json",
      "application/json",
    );
  };

  return (
    <section aria-labelledby="diagnostics-title">
      <div className="page-heading">
        <h2 id="diagnostics-title">Diagnostics</h2>
        <div className="header-actions">
          <button onClick={onBack} type="button">
            Back to Settings
          </button>
        </div>
      </div>

      {state.kind === "loading" ? (
        <p role="status">Loading diagnostics…</p>
      ) : null}
      {state.kind === "error" ? (
        <div>
          <ErrorBanner message={state.message} />
          <button onClick={load} type="button">
            Retry
          </button>
        </div>
      ) : null}

      {state.kind === "loaded" ? (
        <>
          <div className="header-actions">
            <button
              disabled={copyState === "copying"}
              onClick={() => {
                void copy(state.diagnostics);
              }}
              type="button"
            >
              {copyState === "copying"
                ? "Copying…"
                : copyState === "copied"
                  ? "Copied"
                  : "Copy diagnostics"}
            </button>
            <button
              onClick={() => {
                download(state.diagnostics);
              }}
              type="button"
            >
              Download diagnostics
            </button>
          </div>
          {copyState === "error" ? <ErrorBanner message={copyError} /> : null}

          <article
            className="card"
            aria-labelledby="diagnostics-firmware-title"
          >
            <h3 id="diagnostics-firmware-title">Firmware</h3>
            <p>Version {state.diagnostics.firmwareVersion}</p>
            <p>Build {state.diagnostics.buildId}</p>
            <p>Reset reason: {state.diagnostics.resetReason}</p>
            <p>Uptime: {String(state.diagnostics.uptimeMs)} ms</p>
          </article>

          <article className="card" aria-labelledby="diagnostics-memory-title">
            <h3 id="diagnostics-memory-title">Memory</h3>
            <p>
              Free heap: {formatBytes(state.diagnostics.memory.freeHeapBytes)}
            </p>
            <p>
              Minimum free heap since boot:{" "}
              {formatBytes(state.diagnostics.memory.minimumFreeHeapBytes)}
            </p>
            <p>
              Largest free block:{" "}
              {formatBytes(state.diagnostics.memory.largestFreeBlockBytes)}
            </p>
          </article>

          <article className="card" aria-labelledby="diagnostics-usb-title">
            <h3 id="diagnostics-usb-title">USB</h3>
            <p>State: {state.diagnostics.usb.state}</p>
          </article>

          <article className="card" aria-labelledby="diagnostics-wifi-title">
            <h3 id="diagnostics-wifi-title">Wi-Fi</h3>
            <p>Access point: {state.diagnostics.wifi.accessPointState}</p>
            <p>Station: {state.diagnostics.wifi.stationState}</p>
          </article>

          <article className="card" aria-labelledby="diagnostics-storage-title">
            <h3 id="diagnostics-storage-title">Storage</h3>
            <p>State: {state.diagnostics.storage.state}</p>
            <p>
              Web assets:{" "}
              {formatBytes(state.diagnostics.storage.webfsUsedBytes)} used of{" "}
              {formatBytes(state.diagnostics.storage.webfsTotalBytes)}
            </p>
            <p>
              User data:{" "}
              {formatBytes(state.diagnostics.storage.userdataUsedBytes)} used of{" "}
              {formatBytes(state.diagnostics.storage.userdataTotalBytes)}
            </p>
            <p>Stored blobs: {String(state.diagnostics.storage.blobCount)}</p>
            {state.diagnostics.storage.invalidNames.length > 0 ? (
              <p role="alert">
                Invalid filenames:{" "}
                {state.diagnostics.storage.invalidNames.join(", ")}
              </p>
            ) : (
              <p>No invalid filenames.</p>
            )}
            {state.diagnostics.storage.temporaryFiles.length > 0 ? (
              <p role="alert">
                Interrupted-add temporary files:{" "}
                {state.diagnostics.storage.temporaryFiles.join(", ")}
              </p>
            ) : (
              <p>No leftover temporary files.</p>
            )}
          </article>

          <article className="card" aria-labelledby="diagnostics-send-title">
            <h3 id="diagnostics-send-title">Send state</h3>
            {state.diagnostics.send.present ? (
              <p>
                Current/most recent send state: {state.diagnostics.send.state}
              </p>
            ) : (
              <p>No send has run since boot.</p>
            )}
          </article>

          <article className="card" aria-labelledby="diagnostics-health-title">
            <h3 id="diagnostics-health-title">Subsystem health</h3>
            {state.diagnostics.subsystems.length === 0 ? (
              <p>No subsystem health entries were reported.</p>
            ) : (
              <ul className="my-4 grid list-none gap-3 p-0">
                {state.diagnostics.subsystems.map((subsystem) => (
                  <li key={subsystem.name}>
                    {subsystem.name}: {subsystem.state}
                  </li>
                ))}
              </ul>
            )}
          </article>
        </>
      ) : null}
    </section>
  );
}
