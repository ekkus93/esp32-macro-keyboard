import { useState } from "react";
import type { ChangeEvent } from "react";
import { errorText } from "../../api/errors";
import { isRecord } from "../../api/guards";
import {
  exportBackupPackage,
  exportPackage,
  importPackageAsNewPackage,
  isBackupPackageDocument,
  isPackageDocument,
  replacePackage,
  restoreBackupPackage,
} from "../../api/packages";
import type {
  BackupPackageDocument,
  PackageDocument,
} from "../../api/packages";
import { AccessibleDialog } from "../../components/AccessibleDialog";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroPackage } from "../../types/models";

const PACKAGE_MAX_BYTES = 512 * 1024;
const RESTORE_CONFIRMATION_PHRASE = "RESTORE FULL BACKUP";

interface PackageOperationsPageProps {
  activePackage: MacroPackage | null;
  initialSection: "import" | "export";
  saveFile?: (filename: string, text: string) => void;
  onPackageImported?: (created: MacroPackage) => void;
  onPackageReplaced?: (replacement: MacroPackage) => void;
  onBackupRestored?: () => void;
}

function downloadPackage(filename: string, text: string): void {
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

function reloadAfterRestore(): void {
  window.location.reload();
}

function documentPackageId(packageDocument: PackageDocument): string | null {
  const pkg = packageDocument.packages[0];
  return isRecord(pkg) && typeof pkg.id === "string" ? pkg.id : null;
}

export function PackageOperationsPage({
  activePackage,
  initialSection,
  saveFile = downloadPackage,
  onPackageImported = () => undefined,
  onPackageReplaced = () => undefined,
  onBackupRestored = reloadAfterRestore,
}: PackageOperationsPageProps): React.JSX.Element {
  const [exporting, setExporting] = useState(false);
  const [backingUp, setBackingUp] = useState(false);
  const [importing, setImporting] = useState(false);
  const [replacing, setReplacing] = useState(false);
  const [restoring, setRestoring] = useState(false);
  const [importPackage, setImportPackage] = useState<PackageDocument | null>(
    null,
  );
  const [importFilename, setImportFilename] = useState<string | null>(null);
  const [replacementPackage, setReplacementPackage] =
    useState<PackageDocument | null>(null);
  const [replacementFilename, setReplacementFilename] = useState<string | null>(
    null,
  );
  const [restorePackage, setRestorePackage] =
    useState<BackupPackageDocument | null>(null);
  const [restoreFilename, setRestoreFilename] = useState<string | null>(null);
  const [confirmationOpen, setConfirmationOpen] = useState(false);
  const [confirmation, setConfirmation] = useState("");
  const [restoreConfirmationOpen, setRestoreConfirmationOpen] = useState(false);
  const [restoreConfirmation, setRestoreConfirmation] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);

  const confirmationPhrase =
    activePackage === null ? "REPLACE" : `REPLACE ${activePackage.name}`;

  const performExport = async (): Promise<void> => {
    if (activePackage === null || exporting) {
      return;
    }
    setExporting(true);
    setError(null);
    setMessage(null);
    try {
      const download = await exportPackage(activePackage.id);
      saveFile(`macro-package-${activePackage.id}.json`, download.text);
      setMessage(
        `Exported ${activePackage.name} as ${String(download.byteLength)} bytes.`,
      );
    } catch (exportError: unknown) {
      setError(errorText(exportError));
    } finally {
      setExporting(false);
    }
  };

  const performBackup = async (): Promise<void> => {
    if (backingUp) {
      return;
    }
    setBackingUp(true);
    setError(null);
    setMessage(null);
    try {
      const download = await exportBackupPackage();
      saveFile("macro-keyboard-backup.json", download.text);
      setMessage(
        `Created full backup as ${String(download.byteLength)} bytes.`,
      );
    } catch (backupError: unknown) {
      setError(errorText(backupError));
    } finally {
      setBackingUp(false);
    }
  };

  const selectImportPackage = async (
    event: ChangeEvent<HTMLInputElement>,
  ): Promise<void> => {
    setError(null);
    setMessage(null);
    setImportPackage(null);
    setImportFilename(null);
    const file = event.target.files?.[0];
    if (file === undefined) {
      return;
    }
    if (file.size === 0 || file.size > PACKAGE_MAX_BYTES) {
      setError("The package to import must be between 1 byte and 512 KiB.");
      return;
    }
    try {
      const parsed: unknown = JSON.parse(await file.text());
      if (!isPackageDocument(parsed)) {
        throw new Error("The file is not a supported macro-package package.");
      }
      setImportPackage(parsed);
      setImportFilename(file.name);
      setMessage(`Validated ${file.name}. Ready to import as a new pkg.`);
    } catch (selectionError: unknown) {
      setError(errorText(selectionError));
    }
  };

  const performImport = async (): Promise<void> => {
    if (importPackage === null || importing) {
      return;
    }
    setImporting(true);
    setError(null);
    setMessage(null);
    try {
      const newPackageId = crypto.randomUUID();
      const created = await importPackageAsNewPackage(
        newPackageId,
        importPackage,
      );
      onPackageImported(created);
      setImportPackage(null);
      setImportFilename(null);
      setMessage(
        `Imported "${created.name}" as a new package (ID ${created.id}).`,
      );
    } catch (importError: unknown) {
      setError(errorText(importError));
      setMessage(null);
    } finally {
      setImporting(false);
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
    if (activePackage === null) {
      setError(
        "Select an active package before choosing a replacement package.",
      );
      return;
    }
    if (file.size === 0 || file.size > PACKAGE_MAX_BYTES) {
      setError("The replacement package must be between 1 byte and 512 KiB.");
      return;
    }
    try {
      const parsed: unknown = JSON.parse(await file.text());
      if (!isPackageDocument(parsed)) {
        throw new Error("The file is not a supported macro-package package.");
      }
      if (documentPackageId(parsed) !== activePackage.id) {
        throw new Error(
          "The package package ID does not match the selected replacement target.",
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

  const selectRestore = async (
    event: ChangeEvent<HTMLInputElement>,
  ): Promise<void> => {
    setError(null);
    setMessage(null);
    setRestorePackage(null);
    setRestoreFilename(null);
    const file = event.target.files?.[0];
    if (file === undefined) {
      return;
    }
    if (file.size === 0 || file.size > PACKAGE_MAX_BYTES) {
      setError("The backup package must be between 1 byte and 512 KiB.");
      return;
    }
    try {
      const parsed: unknown = JSON.parse(await file.text());
      if (!isBackupPackageDocument(parsed)) {
        throw new Error("The file is not a supported full-backup package.");
      }
      setRestorePackage(parsed);
      setRestoreFilename(file.name);
      setMessage(
        `Validated ${file.name}. Review the full restore before continuing.`,
      );
    } catch (selectionError: unknown) {
      setError(errorText(selectionError));
    }
  };

  const performReplacement = async (): Promise<void> => {
    if (
      activePackage === null ||
      replacementPackage === null ||
      confirmation !== confirmationPhrase ||
      replacing
    ) {
      return;
    }
    setReplacing(true);
    setError(null);
    setMessage("Restoring. Do not power off the device.");
    try {
      const committed = await replacePackage(
        activePackage.id,
        activePackage.revision,
        replacementPackage,
      );
      onPackageReplaced(committed);
      setConfirmationOpen(false);
      setConfirmation("");
      setReplacementPackage(null);
      setReplacementFilename(null);
      setMessage(
        `Replaced ${activePackage.name} with revision ${String(committed.revision)}.`,
      );
    } catch (replacementError: unknown) {
      setError(errorText(replacementError));
      setMessage(null);
    } finally {
      setReplacing(false);
    }
  };

  const performRestore = async (): Promise<void> => {
    if (
      restorePackage === null ||
      restoreConfirmation !== RESTORE_CONFIRMATION_PHRASE ||
      restoring
    ) {
      return;
    }
    setRestoring(true);
    setError(null);
    setMessage("Restoring. Do not power off the device.");
    try {
      await restoreBackupPackage(restorePackage);
      setRestoreConfirmationOpen(false);
      setRestoreConfirmation("");
      setRestorePackage(null);
      setRestoreFilename(null);
      onBackupRestored();
      setMessage("Full backup restored. Live device state has been reloaded.");
    } catch (restoreError: unknown) {
      setError(errorText(restoreError));
      setMessage(null);
    } finally {
      setRestoring(false);
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
            credentials, sessions, provisioning secrets, and encryption
            material.
          </p>
        </div>
      </div>

      <div className="boundary-message" role="status">
        Deterministic package export, import as new, transactional replacement,
        full backup, and all-or-nothing restore are available.
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
          <article className="validation-card">
            <h3>Import as new package</h3>
            <p>
              Validate a complete pkg, assign a new identity, and create it as a
              new package without changing any existing pkg.
            </p>
            <label htmlFor="import-package">Package to import</label>
            <input
              accept="application/json,.json"
              disabled={importing}
              id="import-package"
              onChange={(event) => {
                void selectImportPackage(event);
              }}
              type="file"
            />
            <button
              className="primary"
              disabled={importPackage === null || importing}
              onClick={() => {
                void performImport();
              }}
              type="button"
            >
              {importing ? "Importing…" : "Import as new package"}
            </button>
            <p className="field-help">
              {importFilename === null
                ? "Referenced global macros must already exist on the device with identical content."
                : `Ready to import ${importFilename}.`}
            </p>
          </article>
          <article className="validation-card">
            <h3>Replace selected package</h3>
            <p>
              {activePackage === null
                ? "Select an active package before choosing a transactional replacement target."
                : `Stage, validate, and atomically replace ${activePackage.name}, including its macros and their ordering.`}
            </p>
            <label htmlFor="replacement-package">Replacement package</label>
            <input
              accept="application/json,.json"
              disabled={activePackage === null || replacing}
              id="replacement-package"
              onChange={(event) => {
                void selectReplacement(event);
              }}
              type="file"
            />
            <button
              className="danger"
              disabled={
                activePackage === null ||
                replacementPackage === null ||
                replacing
              }
              onClick={() => {
                setConfirmation("");
                setConfirmationOpen(true);
              }}
              type="button"
            >
              Replace selected package
            </button>
            <p className="field-help">
              {replacementFilename === null
                ? "Referenced global macros must already exist on the device with identical content."
                : `Ready to review ${replacementFilename}.`}
            </p>
          </article>
          <article className="validation-card">
            <h3>Restore full backup</h3>
            <p>
              Replace all packages, package-owned objects, global macros,
              ordering, and optional progress in one durable transaction.
              Credentials and provisioning data are never replaced.
            </p>
            <label htmlFor="restore-backup-package">Full backup package</label>
            <input
              accept="application/json,.json"
              disabled={restoring}
              id="restore-backup-package"
              onChange={(event) => {
                void selectRestore(event);
              }}
              type="file"
            />
            <button
              className="danger"
              disabled={restorePackage === null || restoring}
              onClick={() => {
                setRestoreConfirmation("");
                setRestoreConfirmationOpen(true);
              }}
              type="button"
            >
              Restore full backup
            </button>
            <p className="field-help">
              {restoreFilename === null
                ? "Restore validates the complete staged repository before replacing live data."
                : `Ready to review ${restoreFilename}.`}
            </p>
          </article>
        </div>
      ) : (
        <div className="management-grid">
          <article className="validation-card">
            <h3>Export selected package</h3>
            <p>
              {activePackage === null
                ? "Select an active package before exporting a macro-package package."
                : `Export ${activePackage.name} with its macros and their ordering.`}
            </p>
            <button
              className="primary"
              disabled={activePackage === null || exporting}
              onClick={() => {
                void performExport();
              }}
              type="button"
            >
              {exporting ? "Exporting…" : "Export selected package"}
            </button>
            <p className="field-help">
              The downloaded JSON is generated from one locked repository
              snapshot, validated again before response, and never stored by the
              frontend.
            </p>
          </article>
          <article className="validation-card">
            <h3>Create full backup</h3>
            <p>
              Download every pkg, macro, and ordering record from one locked
              repository snapshot.
            </p>
            <button
              className="primary"
              disabled={backingUp}
              onClick={() => {
                void performBackup();
              }}
              type="button"
            >
              {backingUp ? "Creating backup…" : "Create full backup"}
            </button>
            <p className="field-help">
              The backup excludes credentials, sessions, provisioning secrets,
              and encryption keys.
            </p>
          </article>
        </div>
      )}

      <AccessibleDialog
        description={
          activePackage === null
            ? "No replacement target is selected."
            : `This replaces ${activePackage.name} and its package-owned data. Interrupted activation is recovered from the durable transaction manifest.`
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

      <AccessibleDialog
        description="This replaces the complete logical repository. Interrupted activation is recovered from the durable restore manifest, while credentials and provisioning data remain unchanged."
        onClose={() => {
          if (!restoring) {
            setRestoreConfirmationOpen(false);
            setRestoreConfirmation("");
          }
        }}
        open={restoreConfirmationOpen}
        title="Confirm full backup restore"
      >
        <label htmlFor="restore-confirmation">
          Type <strong>{RESTORE_CONFIRMATION_PHRASE}</strong> to continue.
        </label>
        <input
          autoComplete="off"
          id="restore-confirmation"
          onChange={(event) => {
            setRestoreConfirmation(event.target.value);
          }}
          value={restoreConfirmation}
        />
        <div className="dialog-actions">
          <button
            disabled={restoring}
            onClick={() => {
              setRestoreConfirmationOpen(false);
              setRestoreConfirmation("");
            }}
            type="button"
          >
            Cancel
          </button>
          <button
            className="danger"
            disabled={
              restoreConfirmation !== RESTORE_CONFIRMATION_PHRASE || restoring
            }
            onClick={() => {
              void performRestore();
            }}
            type="button"
          >
            {restoring ? "Restoring…" : "Confirm full restore"}
          </button>
        </div>
      </AccessibleDialog>
    </section>
  );
}
