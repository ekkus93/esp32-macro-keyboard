import { useEffect, useState } from "react";
import { errorText } from "../../api/errors";
import { getQuarantine, getStorageHealth } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import { StatusBadge } from "../../components/StatusBadge";
import type { QuarantineList, StorageHealth } from "../../types/models";

export function DiagnosticsPage(): React.JSX.Element {
  const [health, setHealth] = useState<StorageHealth | null>(null);
  const [quarantine, setQuarantine] = useState<QuarantineList | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);

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
        <article className="validation-card" aria-labelledby="storage-health-title">
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
                      <dd><code>{entry.id}</code></dd>
                      <dt>Source</dt>
                      <dd><code>{entry.sourcePath}</code></dd>
                      <dt>Evidence</dt>
                      <dd><code>{entry.evidencePath}</code></dd>
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

      <article className="validation-card">
        <h3>Full subsystem diagnostics</h3>
        <p>
          Build identity, heap, task stacks, capacities, and subsystem health are
          not fabricated by this screen.
        </p>
        <button disabled type="button">
          Load full diagnostics
        </button>
        <p className="field-help">
          Unavailable until the Phase 19 redacted diagnostics aggregation route
          exists.
        </p>
      </article>
    </section>
  );
}
