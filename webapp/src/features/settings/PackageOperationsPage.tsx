import type { MacroSet } from "../../types/models";

interface PackageOperationsPageProps {
  activeSet: MacroSet | null;
  initialSection: "import" | "export";
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

export function PackageOperationsPage({
  activeSet,
  initialSection,
}: PackageOperationsPageProps): React.JSX.Element {
  return (
    <section aria-labelledby="package-operations-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Transactional data operations</p>
          <h2 id="package-operations-title">Import, export, and recovery</h2>
          <p>
            These controls remain disabled until the device can validate the
            complete package before mutation and exclude every secret.
          </p>
        </div>
      </div>

      <div className="boundary-message" role="status">
        The frontend does not simulate package success. The current firmware
        intentionally returns <code>503 Service Unavailable</code> for these
        Phase 18 boundaries.
      </div>

      <div className="section-tabs" role="tablist" aria-label="Package operations">
        <a
          aria-selected={initialSection === "import"}
          className={initialSection === "import" ? "active" : ""}
          href="#/import"
          role="tab"
        >
          Import and restore
        </a>
        <a
          aria-selected={initialSection === "export"}
          className={initialSection === "export" ? "active" : ""}
          href="#/export"
          role="tab"
        >
          Export and backup
        </a>
      </div>

      {initialSection === "import" ? (
        <div className="management-grid">
          <OperationCard
            action="Import as new set"
            description="Validate a complete package, assign a new identity, and create it without changing existing sets."
            explanation="the Phase 18 bounded package reader and all-object validation service are not implemented."
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
                : "the Phase 18 transactional replace and interrupted-operation recovery service are not implemented."
            }
          />
          <OperationCard
            action="Restore full backup"
            description="Restore all sets, global macros, procedures, and optional progress as one transaction."
            explanation="the Phase 18 all-or-nothing restore and secret-scanning service are not implemented."
          />
        </div>
      ) : (
        <div className="management-grid">
          <OperationCard
            action="Export selected set"
            description={
              activeSet === null
                ? "Select an active set before exporting a macro-set package."
                : `Export ${activeSet.name} with referenced macros and procedures but without credentials or sessions.`
            }
            explanation={
              activeSet === null
                ? "there is no active export target."
                : "the Phase 18 deterministic package writer and secret exclusion scanner are not implemented."
            }
          />
          <OperationCard
            action="Create full backup"
            description="Create a deterministic backup of user data while excluding provisioning credentials, sessions, and encryption material."
            explanation="the Phase 18 full-backup service and secret scanner are not implemented."
          />
        </div>
      )}
    </section>
  );
}
