import { useEffect, useState } from "react";
import { errorText } from "../../api/errors";
import {
  getFullDiagnostics,
  getQuarantine,
  getStorageHealth,
} from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import { StatusBadge } from "../../components/StatusBadge";
import type {
  FullDiagnostics,
  QuarantineList,
  StorageHealth,
  SubsystemHealthState,
} from "../../types/models";

function formatBytes(bytes: number): string {
  if (bytes < 1024) {
    return `${String(bytes)} B`;
  }
  const kilobytes = bytes / 1024;
  if (kilobytes < 1024) {
    return `${kilobytes.toFixed(1)} KB`;
  }
  return `${(kilobytes / 1024).toFixed(1)} MB`;
}

function subsystemBadgeState(
  state: SubsystemHealthState,
): "good" | "warning" | "bad" | "neutral" {
  switch (state) {
    case "healthy":
      return "good";
    case "degraded":
    case "recovering":
      return "warning";
    case "unavailable":
    case "failed":
      return "bad";
    default:
      return "neutral";
  }
}

export function DiagnosticsPage(): React.JSX.Element {
  const [health, setHealth] = useState<StorageHealth | null>(null);
  const [quarantine, setQuarantine] = useState<QuarantineList | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);
  const [fullDiagnostics, setFullDiagnostics] =
    useState<FullDiagnostics | null>(null);
  const [fullDiagnosticsLoading, setFullDiagnosticsLoading] = useState(false);
  const [fullDiagnosticsError, setFullDiagnosticsError] = useState<
    string | null
  >(null);

  async function loadFullDiagnostics(): Promise<void> {
    setFullDiagnosticsLoading(true);
    setFullDiagnosticsError(null);
    try {
      setFullDiagnostics(await getFullDiagnostics());
    } catch (loadError: unknown) {
      setFullDiagnosticsError(errorText(loadError));
    } finally {
      setFullDiagnosticsLoading(false);
    }
  }

  useEffect(() => {
    let active = true;
    setLoading(true);
    setError(null);
    void Promise.all([getStorageHealth(), getQuarantine()])
      .then(([loadedHealth, loadedQuarantine]) => {
        if (active) {
          setHealth(loadedHealth);
          setQuarantine(loadedQuarantine);
        }
      })
      .catch((loadError: unknown) => {
        if (active) {
          setError(errorText(loadError));
        }
      })
      .finally(() => {
        if (active) {
          setLoading(false);
        }
      });
    return () => {
      active = false;
    };
  }, [loadVersion]);

  return (
    <section aria-labelledby="diagnostics-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Redacted device state</p>
          <h2 id="diagnostics-title">Storage diagnostics</h2>
          <p>
            Live mount and quarantine records are shown without credentials,
            tokens, or macro source.
          </p>
        </div>
        <button
          disabled={loading}
          onClick={() => {
            setLoadVersion((version) => version + 1);
          }}
          type="button"
        >
          {loading ? "Refreshing…" : "Refresh"}
        </button>
      </div>

      <ErrorBanner message={error} />
      {loading && health === null ? (
        <p aria-busy="true" role="status">
          Loading diagnostics…
        </p>
      ) : null}

      {health === null ? null : (
        <article
          className="validation-card"
          aria-labelledby="storage-health-title"
        >
          <div className="management-title-row">
            <h3 id="storage-health-title">Storage health</h3>
            <StatusBadge
              label={
                health.webMounted && health.dataMounted
                  ? "Required filesystems mounted"
                  : "Filesystem mount problem"
              }
              state={health.webMounted && health.dataMounted ? "good" : "bad"}
            />
          </div>
          <dl className="metadata">
            <dt>Web filesystem</dt>
            <dd>{health.webMounted ? "Mounted" : "Not mounted"}</dd>
            <dt>User data</dt>
            <dd>{health.dataMounted ? "Mounted" : "Not mounted"}</dd>
            <dt>Bounded verification</dt>
            <dd>
              {health.verified
                ? "Verified"
                : "Not run — Phase 19 verification service is unavailable"}
            </dd>
            <dt>Quarantine records</dt>
            <dd>{String(health.quarantineCount)}</dd>
            <dt>Damaged records</dt>
            <dd>{String(health.damagedQuarantineCount)}</dd>
          </dl>
          <button disabled type="button">
            Run full storage verification
          </button>
          <p className="field-help">
            Unavailable until the Phase 19 diagnostics service owns the bounded
            verification operation.
          </p>
        </article>
      )}

      {quarantine === null ? null : (
        <section aria-labelledby="quarantine-title">
          <div className="management-title-row">
            <h3 id="quarantine-title">Quarantine</h3>
            <StatusBadge
              label={
                quarantine.items.length === 0
                  ? "No quarantined records"
                  : `${String(quarantine.items.length)} quarantined records`
              }
              state={quarantine.items.length === 0 ? "good" : "warning"}
            />
          </div>
          {quarantine.items.length === 0 ? (
            <p role="status">No recovery evidence is currently quarantined.</p>
          ) : (
            <ol className="management-list">
              {quarantine.items.map((entry) => (
                <li className="card" key={entry.id}>
                  <div>
                    <h4>{entry.reason || "Unspecified quarantine reason"}</h4>
                    <dl className="metadata">
                      <dt>Record ID</dt>
                      <dd>
                        <code>{entry.id}</code>
                      </dd>
                      <dt>Source</dt>
                      <dd>
                        <code>{entry.sourcePath}</code>
                      </dd>
                      <dt>Evidence</dt>
                      <dd>
                        <code>{entry.evidencePath}</code>
                      </dd>
                    </dl>
                  </div>
                </li>
              ))}
            </ol>
          )}
          {quarantine.damagedCount > 0 ? (
            <p className="error-message" role="alert">
              {String(quarantine.damagedCount)} quarantine record
              {quarantine.damagedCount === 1 ? " is" : "s are"} damaged and
              cannot be treated as complete recovery evidence.
            </p>
          ) : null}
        </section>
      )}

      <article
        className="validation-card"
        aria-labelledby="full-diagnostics-title"
      >
        <div className="management-title-row">
          <h3 id="full-diagnostics-title">Full subsystem diagnostics</h3>
          <button
            disabled={fullDiagnosticsLoading}
            onClick={() => {
              void loadFullDiagnostics();
            }}
            type="button"
          >
            {fullDiagnosticsLoading ? "Loading…" : "Load full diagnostics"}
          </button>
        </div>
        <p>
          Build identity, heap, task stacks, capacities, and subsystem health
          are not fabricated by this screen.
        </p>
        <ErrorBanner message={fullDiagnosticsError} />

        {fullDiagnostics === null ? null : (
          <>
            <dl className="metadata">
              <dt>Build ID</dt>
              <dd>
                <code>{fullDiagnostics.buildId}</code>
              </dd>
              <dt>Firmware version</dt>
              <dd>{fullDiagnostics.firmwareVersion}</dd>
              <dt>Schema version</dt>
              <dd>{String(fullDiagnostics.schemaVersion)}</dd>
              <dt>Reset reason</dt>
              <dd>{fullDiagnostics.resetReason}</dd>
              <dt>Uptime</dt>
              <dd>
                {(fullDiagnostics.uptimeMs / 1000 / 60).toFixed(1)} minutes
              </dd>
              <dt>Free heap</dt>
              <dd>{formatBytes(fullDiagnostics.freeHeapBytes)}</dd>
              <dt>Minimum free heap</dt>
              <dd>{formatBytes(fullDiagnostics.minFreeHeapBytes)}</dd>
              <dt>Controls task stack high-water mark</dt>
              <dd>{String(fullDiagnostics.stack.controlsWords)} words</dd>
              <dt>Executor task stack high-water mark</dt>
              <dd>{String(fullDiagnostics.stack.executorWords)} words</dd>
              <dt>Web filesystem capacity</dt>
              <dd>
                {fullDiagnostics.webfs.ok
                  ? `${formatBytes(fullDiagnostics.webfs.usedBytes)} of ${formatBytes(fullDiagnostics.webfs.totalBytes)} used`
                  : "Unavailable"}
              </dd>
              <dt>User data capacity</dt>
              <dd>
                {fullDiagnostics.userdata.ok
                  ? `${formatBytes(fullDiagnostics.userdata.usedBytes)} of ${formatBytes(fullDiagnostics.userdata.totalBytes)} used`
                  : "Unavailable"}
              </dd>
              <dt>Quarantine</dt>
              <dd>
                {fullDiagnostics.quarantine.ok
                  ? `${String(fullDiagnostics.quarantine.count)} record(s), ${String(fullDiagnostics.quarantine.damagedCount)} damaged`
                  : "Unavailable"}
              </dd>
              <dt>Execution state</dt>
              <dd>{fullDiagnostics.executionState}</dd>
            </dl>

            <h4>Subsystem health</h4>
            <ul className="management-list">
              {fullDiagnostics.subsystems.map((subsystem) => (
                <li className="card" key={subsystem.name}>
                  <StatusBadge
                    label={`${subsystem.name}: ${subsystem.state}`}
                    state={subsystemBadgeState(subsystem.state)}
                  />
                </li>
              ))}
            </ul>
          </>
        )}
      </article>
    </section>
  );
}
