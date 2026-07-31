import { useState } from "react";
import type { ChangeEvent } from "react";
import { errorText } from "../../api/errors";
import { isRecord } from "../../api/guards";
import {
  exportSetPackage,
  isSetPackageDocument,
  replaceSetPackage,
} from "../../api/packages";
import type { SetPackageDocument } from "../../api/packages";
import { AccessibleDialog } from "../../components/AccessibleDialog";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroSet } from "../../types/models";

const SET_PACKAGE_MAX_BYTES = 512 * 1024;

interface PackageOperationsPageProps {
  activeSet: MacroSet | null;
  initialSection: "import" | "export";
  saveFile?: (filename: string, text: string) => void;
  onSetReplaced?: (replacement: MacroSet) => void;
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

function packagedSetId(packageDocument: SetPackageDocument): string | null {
  const set = packageDocument.sets[0];
  return isRecord(set) && typeof set.id === "string" ? set.id : null;
}

export function PackageOperationsPage({
  activeSet,
  initialSection,
  saveFile = downloadSetPackage,
  onSetReplaced = () => undefined,
}: PackageOperationsPageProps): React.JSX.Element {
  const [exporting, setExporting] = useState(false);
  const [replacing, setReplacing] = useState(false);
  const [replacementPackage, setReplacementPackage] =
    useState<SetPackageDocument | null>(null);
  const [replacementFilename, setReplacementFilename] = useState<string | null>(
    null,
  );
  const [confirmationOpen, setConfirmationOpen] = useState(false);
  const [confirmation, setConfirmation] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);

  const confirmationPhrase =
    activeSet === null ? "REPLACE" : `REPLACE ${activeSet.name}`;

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

  const selectReplacement = async (
    event: ChangeEvent<HTMLInputElement>,
  ): Promise<void> => {
    setError(null);
    setMessage(null);
    setReplacementPackage(null);
    setReplacementFilename(null);
    const file = event.target.files?.[0];
    if (file === undefined) {
      return;
    }
    if (activeSet === null) {
      setError("Select an active set before choosing a replacement package.");
      return;
    }
    if (file.size === 0 || file.size > SET_PACKAGE_MAX_BYTES) {
      setError("The replacement package must be between 1 byte and 512 KiB.");
      return;
    }
    try {
      const parsed: unknown = JSON.parse(await file.text());
      if (!isSetPackageDocument(parsed)) {
        throw new Error("The file is not a supported macro-set package.");
      }
      if (packagedSetId(parsed) !== activeSet.id) {
        throw new Error(
          "The package set ID does not match the selected replacement target.",
        );
      }
      setReplacementPackage(parsed);
      setReplacementFilename(file.name);
      setMessage(
        `Validated ${file.name}. Review the replacement before continuing.`,
      );
    } catch (selectionError: unknown) {
      setError(errorText(selectionError));
    }
  };

  const performReplacement = async (): Promise<void> => {
    if (
      activeSet === null ||
      replacementPackage === null ||
      confirmation !== confirmationPhrase ||
      replacing
    ) {
      return;
    }
    setReplacing(true);
    setError(null);
    setMessage("Press the confirmation button on the device.");
    try {
      const committed = await replaceSetPackage(
        activeSet.id,
        activeSet.revision,
        replacementPackage,
      );
      onSetReplaced(committed);
      setConfirmationOpen(false);
      setConfirmation("");
      setReplacementPackage(null);
      setReplacementFilename(null);
      setMessage(
        `Replaced ${activeSet.name} with revision ${String(committed.revision)}.`,
      );
    } catch (replacementError: unknown) {
      setError(errorText(replacementError));
      setMessage(null);
    } finally {
      setReplacing(false);
    }
  };

  return (
    <section aria-labelledby="package-operations-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Transactional data operations</p>
          <h2 id="package-operations-title">Import, export, and recovery</h2>
          <p>
            Package operations validate server-owned data and exclude
            credentials, sessions, and encryption material.
          </p>
        </div>
      </div>

      <div className="boundary-message" role="status">
        Deterministic set export and transactional replacement are available.
        Import-as-new, full backup, and full restore remain disabled until their
        later Phase 18 services are complete.
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
          <article className="validation-card">
            <h3>Replace selected set</h3>
            <p>
              {activeSet === null
                ? "Select an active set before choosing a transactional replacement target."
                : `Stage, validate, and atomically replace ${activeSet.name}, including local macros, procedures, ordering, and progress.`}
            </p>
            <label htmlFor="replacement-package">Replacement package</label>
            <input
              accept="application/json,.json"
              disabled={activeSet === null || replacing}
              id="replacement-package"
              onChange={(event) => {
                void selectReplacement(event);
              }}
              type="file"
            />
            <button
              className="danger"
              disabled={
                activeSet === null || replacementPackage === null || replacing
              }
              onClick={() => {
                setConfirmation("");
                setConfirmationOpen(true);
              }}
              type="button"
            >
              Replace selected set
            </button>
            <p className="field-help">
              {replacementFilename === null
                ? "Referenced global macros must already exist on the device with identical content."
                : `Ready to review ${replacementFilename}.`}
            </p>
          </article>
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
              The downloaded JSON is generated from one locked repository
              snapshot, validated again before response, and never stored by the
              frontend.
            </p>
          </article>
          <OperationCard
            action="Create full backup"
            description="Create a deterministic backup of user data while excluding provisioning credentials, sessions, and encryption material."
            explanation="the full-backup service and backup secret scanner are not implemented."
          />
        </div>
      )}

      <AccessibleDialog
        description={
          activeSet === null
            ? "No replacement target is selected."
            : `This replaces ${activeSet.name} and its set-owned data. Interrupted activation is recovered from the durable transaction manifest.`
        }
        onClose={() => {
          if (!replacing) {
            setConfirmationOpen(false);
            setConfirmation("");
          }
        }}
        open={confirmationOpen}
        title="Confirm transactional replacement"
      >
        <label htmlFor="replacement-confirmation">
          Type <strong>{confirmationPhrase}</strong> to continue.
        </label>
        <input
          autoComplete="off"
          id="replacement-confirmation"
          onChange={(event) => {
            setConfirmation(event.target.value);
          }}
          value={confirmation}
        />
        <div className="dialog-actions">
          <button
            disabled={replacing}
            onClick={() => {
              setConfirmationOpen(false);
              setConfirmation("");
            }}
            type="button"
          >
            Cancel
          </button>
          <button
            className="danger"
            disabled={confirmation !== confirmationPhrase || replacing}
            onClick={() => {
              void performReplacement();
            }}
            type="button"
          >
            {replacing ? "Replacing…" : "Confirm replacement"}
          </button>
        </div>
      </AccessibleDialog>
    </section>
  );
}
