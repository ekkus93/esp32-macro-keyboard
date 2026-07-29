import { useState } from "react";
import { errorText } from "../../api/errors";
import { exportSetPackage } from "../../api/packages";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroSet } from "../../types/models";

interface PackageOperationsPageProps {
  activeSet: MacroSet | null;
  initialSection: "import" | "export";
  saveFile?: (filename: string, text: string) => void;
}

interface OperationCardProps {
  action: string;
  description: string;
  explanation: string;
}

function OperationCard({
  action,
  description,
  explanation,
}: OperationCardProps): React.JSX.Element {
  return (
    <article className="validation-card unavailable-operation">
      <h3>{action}</h3>
      <p>{description}</p>
      <button disabled type="button">
        {action}
      </button>
      <p className="field-help">Unavailable: {explanation}</p>
    </article>
  );
}

function downloadSetPackage(filename: string, text: string): void {
  const blob = new Blob([text], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = filename;
  document.body.append(anchor);
  anchor.click();
  anchor.remove();
  URL.revokeObjectURL(url);
}

export function PackageOperationsPage({
  activeSet,
  initialSection,
  saveFile = downloadSetPackage,
}: PackageOperationsPageProps): React.JSX.Element {
  const [exporting, setExporting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);

  const performExport = async (): Promise<void> => {
    if (activeSet === null || exporting) {
      return;
    }
    setExporting(true);
    setError(null);
    setMessage(null);
    try {
      const download = await exportSetPackage(activeSet.id);
      saveFile(`macro-set-${activeSet.id}.json`, download.text);
      setMessage(
        `Exported ${activeSet.name} as ${String(download.byteLength)} bytes.`,
      );
    } catch (exportError: unknown) {
      setError(errorText(exportError));
    } finally {
      setExporting(false);
    }
  };

  return (
    <section aria-labelledby="package-operations-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Transactional data operations</p>
          <h2 id="package-operations-title">Import, export, and recovery</h2>
          <p>
            Package operations validate server-owned data and exclude credentials,
            sessions, and encryption material.
          </p>
        </div>
      </div>

      <div className="boundary-message" role="status">
        Deterministic set export is available. Import, transactional replacement,
        full backup, and restore remain disabled until their Phase 18 transaction
        services are complete.
      </div>

      <ErrorBanner message={error} />
      {message === null ? null : (
        <p className="save-message" role="status" aria-live="polite">
          {message}
        </p>
      )}

      <nav className="section-tabs" aria-label="Package operations">
        <a
          aria-current={initialSection === "import" ? "page" : undefined}
          className={initialSection === "import" ? "active" : ""}
          href="#/import"
        >
          Import and restore
        </a>
        <a
          aria-current={initialSection === "export" ? "page" : undefined}
          className={initialSection === "export" ? "active" : ""}
          href="#/export"
        >
          Export and backup
        </a>
      </nav>

      {initialSection === "import" ? (
        <div className="management-grid">
          <OperationCard
            action="Import as new set"
            description="Validate a complete package, assign a new identity, and create it without changing existing sets."
            explanation="transactional import-as-new is not implemented."
          />
          <OperationCard
            action="Replace selected set"
            description={
              activeSet === null
                ? "Select an active set before choosing a transactional replacement target."
                : `Replace ${activeSet.name} only after staging and validating the entire package.`
            }
            explanation={
              activeSet === null
                ? "there is no active replacement target."
                : "transactional replace and interrupted-operation recovery are not implemented."
            }
          />
          <OperationCard
            action="Restore full backup"
            description="Restore all sets, global macros, procedures, and optional progress as one transaction."
            explanation="all-or-nothing restore and backup secret scanning are not implemented."
          />
        </div>
      ) : (
        <div className="management-grid">
          <article className="validation-card">
            <h3>Export selected set</h3>
            <p>
              {activeSet === null
                ? "Select an active set before exporting a macro-set package."
                : `Export ${activeSet.name} with set-local macros, referenced global macros, procedures, and current progress.`}
            </p>
            <button
              className="primary"
              disabled={activeSet === null || exporting}
              onClick={() => {
                void performExport();
              }}
              type="button"
            >
              {exporting ? "Exporting…" : "Export selected set"}
            </button>
            <p className="field-help">
              The downloaded JSON is generated from one locked repository snapshot,
              validated again before response, and never stored by the frontend.
            </p>
          </article>
          <OperationCard
            action="Create full backup"
            description="Create a deterministic backup of user data while excluding provisioning credentials, sessions, and encryption material."
            explanation="the full-backup service and backup secret scanner are not implemented."
          />
        </div>
      )}
    </section>
  );
}
