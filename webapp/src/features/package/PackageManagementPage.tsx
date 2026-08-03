import { useMemo, useState } from "react";
import { errorText } from "../../api/errors";
import {
  createPackage,
  deletePackage,
  duplicatePackage,
  reorderPackages,
  updatePackage,
} from "../../api/routes";
import { AccessibleDialog } from "../../components/AccessibleDialog";
import { ErrorBanner } from "../../components/ErrorBanner";
import { limits } from "../../types/limits";
import type { MacroPackage, Settings } from "../../types/models";
import { utf8ByteLength } from "../macros/macroDraft";

interface PackageManagementPageProps {
  packages: readonly MacroPackage[];
  settings: Settings;
  onPackagesChanged: (packages: MacroPackage[]) => void;
}

type EditorState =
  | { mode: "create"; package: MacroPackage }
  | { mode: "edit"; package: MacroPackage }
  | null;

interface PackageFieldErrors {
  board?: string;
  description?: string;
  manufacturer?: string;
  model?: string;
  name?: string;
}

/* The device returns packages in index order (SPEC 12.3), which is the user's
   order; the UI must preserve it rather than impose one of its own. */
function sortedPackages(packages: readonly MacroPackage[]): MacroPackage[] {
  return [...packages];
}

function newPackage(): MacroPackage {
  return {
    schema_version: 1,
    id: crypto.randomUUID(),
    revision: 1,
    name: "",
  };
}

function validatePackage(pkg: MacroPackage): PackageFieldErrors {
  const errors: PackageFieldErrors = {};
  const nameBytes = utf8ByteLength(pkg.name);
  if (nameBytes === 0) {
    errors.name = "Name is required.";
  } else if (nameBytes > limits.packageNameBytes) {
    errors.name = `Name exceeds ${String(limits.packageNameBytes)} UTF-8 bytes.`;
  }
  return errors;
}

function noErrors(errors: PackageFieldErrors): boolean {
  return Object.keys(errors).length === 0;
}

interface FieldProps {
  error: string | undefined;
  id: string;
  label: string;
  maximumBytes: number;
  value: string;
  onChange: (value: string) => void;
}

function TextField({
  error,
  id,
  label,
  maximumBytes,
  value,
  onChange,
}: FieldProps): React.JSX.Element {
  const bytes = utf8ByteLength(value);
  const helpId = `${id}-help`;
  return (
    <label className="form-stack" htmlFor={id}>
      {label}
      <input
        aria-describedby={helpId}
        aria-invalid={error === undefined ? undefined : true}
        id={id}
        onChange={(event) => {
          onChange(event.currentTarget.value);
        }}
        value={value}
      />
      <span
        className={
          error === undefined ? "field-help" : "field-help limit-exceeded"
        }
        id={helpId}
      >
        {error ?? `${String(bytes)} / ${String(maximumBytes)} UTF-8 bytes`}
      </span>
    </label>
  );
}

export function PackageManagementPage({
  packages,
  settings,
  onPackagesChanged,
}: PackageManagementPageProps): React.JSX.Element {
  const ordered = useMemo(() => sortedPackages(packages), [packages]);
  const [editor, setEditor] = useState<EditorState>(null);
  const [duplicateSource, setDuplicateSource] = useState<MacroPackage | null>(
    null,
  );
  const [duplicateName, setDuplicateName] = useState("");
  const [deleteTarget, setDeleteTarget] = useState<MacroPackage | null>(null);
  const [deleteConfirmation, setDeleteConfirmation] = useState("");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);

  const editorErrors = editor === null ? {} : validatePackage(editor.package);

  const closeDialogs = (): void => {
    if (busy) {
      return;
    }
    setEditor(null);
    setDuplicateSource(null);
    setDuplicateName("");
    setDeleteTarget(null);
    setDeleteConfirmation("");
    setError(null);
  };

  const saveEditor = async (): Promise<void> => {
    if (editor === null || !noErrors(editorErrors) || busy) {
      return;
    }
    setBusy(true);
    setError(null);
    setMessage(null);
    try {
      const committed =
        editor.mode === "create"
          ? await createPackage(editor.package)
          : await updatePackage(editor.package, editor.package.revision);
      const replacement =
        editor.mode === "create"
          ? [...ordered, committed]
          : ordered.map((pkg) => (pkg.id === committed.id ? committed : pkg));
      onPackagesChanged(sortedPackages(replacement));
      setMessage(
        editor.mode === "create"
          ? `Created ${committed.name}.`
          : `Saved ${committed.name} revision ${String(committed.revision)}.`,
      );
      setEditor(null);
    } catch (saveError: unknown) {
      setError(errorText(saveError));
    } finally {
      setBusy(false);
    }
  };

  const duplicate = async (): Promise<void> => {
    if (
      duplicateSource === null ||
      utf8ByteLength(duplicateName.trim()) === 0 ||
      utf8ByteLength(duplicateName.trim()) > limits.packageNameBytes ||
      busy
    ) {
      return;
    }
    setBusy(true);
    setError(null);
    setMessage(null);
    try {
      const committed = await duplicatePackage(duplicateSource.id, {
        id: crypto.randomUUID(),
        name: duplicateName.trim(),
        expectedRevision: duplicateSource.revision,
      });
      onPackagesChanged(sortedPackages([...ordered, committed]));
      setMessage(`Duplicated ${duplicateSource.name} as ${committed.name}.`);
      setDuplicateSource(null);
      setDuplicateName("");
    } catch (duplicateError: unknown) {
      setError(errorText(duplicateError));
    } finally {
      setBusy(false);
    }
  };

  const remove = async (): Promise<void> => {
    if (
      deleteTarget === null ||
      deleteTarget.id === settings.activePackageId ||
      deleteConfirmation !== deleteTarget.name ||
      busy
    ) {
      return;
    }
    setBusy(true);
    setError(null);
    setMessage(null);
    try {
      const result = await deletePackage(
        deleteTarget.id,
        deleteTarget.revision,
      );
      if (result.id !== deleteTarget.id) {
        throw new Error(
          "The device confirmed deletion for a different package.",
        );
      }
      onPackagesChanged(ordered.filter((pkg) => pkg.id !== deleteTarget.id));
      setMessage(`Deleted ${deleteTarget.name}.`);
      setDeleteTarget(null);
      setDeleteConfirmation("");
    } catch (deleteError: unknown) {
      setError(errorText(deleteError));
    } finally {
      setBusy(false);
    }
  };

  const move = async (
    sourceIndex: number,
    destinationIndex: number,
  ): Promise<void> => {
    if (
      busy ||
      sourceIndex === destinationIndex ||
      sourceIndex < 0 ||
      destinationIndex < 0 ||
      sourceIndex >= ordered.length ||
      destinationIndex >= ordered.length
    ) {
      return;
    }
    const replacement = [...ordered];
    const [moved] = replacement.splice(sourceIndex, 1);
    if (moved === undefined) {
      return;
    }
    replacement.splice(destinationIndex, 0, moved);
    setBusy(true);
    setError(null);
    setMessage(null);
    try {
      const committed = await reorderPackages(replacement.map((pkg) => pkg.id));
      onPackagesChanged(sortedPackages(committed));
      setMessage(
        `Moved ${moved.name} to position ${String(destinationIndex + 1)}.`,
      );
    } catch (reorderError: unknown) {
      setError(errorText(reorderError));
    } finally {
      setBusy(false);
    }
  };

  const updateEditor = (field: keyof MacroPackage, value: string): void => {
    setEditor((current) =>
      current === null
        ? null
        : {
            ...current,
            package: {
              ...current.package,
              [field]: value,
            },
          },
    );
  };

  return (
    <section aria-labelledby="manage-packages-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Persisted resources</p>
          <h2 id="manage-packages-title">Manage macro packages</h2>
          <p>
            Create, revise, duplicate, reorder, or delete complete macro
            packages.
          </p>
        </div>
        <button
          className="primary"
          disabled={busy || ordered.length >= limits.macroPackages}
          onClick={() => {
            setMessage(null);
            setError(null);
            setEditor({ mode: "create", package: newPackage() });
          }}
          type="button"
        >
          Create package
        </button>
      </div>

      {ordered.length >= limits.macroPackages ? (
        <p className="conflict-message" role="status">
          The device already contains the maximum of{" "}
          {String(limits.macroPackages)} packages.
        </p>
      ) : null}
      <ErrorBanner message={error} />
      {message === null ? null : (
        <p className="save-message" role="status" aria-live="polite">
          {message}
        </p>
      )}

      {ordered.length === 0 ? (
        <p role="status">No macro packages are stored.</p>
      ) : (
        <ol className="management-list" aria-label="Macro package order">
          {ordered.map((pkg, index) => {
            const active = settings.activePackageId === pkg.id;
            return (
              <li className="card management-card" key={pkg.id}>
                <div>
                  <div className="management-title-row">
                    <h3>{pkg.name}</h3>
                    {active ? (
                      <span className="status-badge status-good">
                        Active package
                      </span>
                    ) : null}
                  </div>
                  <dl className="metadata">
                    <dt>Position</dt>
                    <dd>
                      {String(index + 1)} of {String(ordered.length)}
                    </dd>
                    <dt>Revision</dt>
                    <dd>{String(pkg.revision)}</dd>
                  </dl>
                </div>

                <div className="management-actions">
                  <div
                    className="reorder-actions"
                    aria-label={`Reorder ${pkg.name}`}
                  >
                    <button
                      aria-label={`Move ${pkg.name} first`}
                      disabled={busy || index === 0}
                      onClick={() => {
                        void move(index, 0);
                      }}
                      type="button"
                    >
                      Move first
                    </button>
                    <button
                      aria-label={`Move ${pkg.name} up`}
                      disabled={busy || index === 0}
                      onClick={() => {
                        void move(index, index - 1);
                      }}
                      type="button"
                    >
                      Move up
                    </button>
                    <button
                      aria-label={`Move ${pkg.name} down`}
                      disabled={busy || index === ordered.length - 1}
                      onClick={() => {
                        void move(index, index + 1);
                      }}
                      type="button"
                    >
                      Move down
                    </button>
                    <button
                      aria-label={`Move ${pkg.name} last`}
                      disabled={busy || index === ordered.length - 1}
                      onClick={() => {
                        void move(index, ordered.length - 1);
                      }}
                      type="button"
                    >
                      Move last
                    </button>
                  </div>
                  <div className="form-actions">
                    <button
                      disabled={busy}
                      onClick={() => {
                        setMessage(null);
                        setError(null);
                        setEditor({ mode: "edit", package: { ...pkg } });
                      }}
                      type="button"
                    >
                      Edit
                    </button>
                    <button
                      disabled={busy || ordered.length >= limits.macroPackages}
                      onClick={() => {
                        setMessage(null);
                        setError(null);
                        setDuplicateSource(pkg);
                        setDuplicateName(`${pkg.name} copy`);
                      }}
                      type="button"
                    >
                      Duplicate
                    </button>
                    <button
                      className="danger"
                      disabled={busy || active}
                      onClick={() => {
                        setMessage(null);
                        setError(null);
                        setDeleteTarget(pkg);
                        setDeleteConfirmation("");
                      }}
                      type="button"
                    >
                      Delete
                    </button>
                  </div>
                  {active ? (
                    <p className="field-help">
                      Select a different active package before deleting this
                      one.
                    </p>
                  ) : null}
                </div>
              </li>
            );
          })}
        </ol>
      )}

      <AccessibleDialog
        description={
          editor?.mode === "create"
            ? "Create a new persisted macro package."
            : "Edit the selected package using its current revision."
        }
        onClose={closeDialogs}
        open={editor !== null}
        title={
          editor?.mode === "create"
            ? "Create macro package"
            : "Edit macro package"
        }
      >
        {editor === null ? null : (
          <form
            className="form-stack"
            onSubmit={(event) => {
              event.preventDefault();
              void saveEditor();
            }}
          >
            <TextField
              error={editorErrors.name}
              id="package-name"
              label="Name"
              maximumBytes={limits.packageNameBytes}
              onChange={(value) => {
                updateEditor("name", value);
              }}
              value={editor.package.name}
            />
            <ErrorBanner message={error} />
            <div className="form-actions">
              <button disabled={busy} onClick={closeDialogs} type="button">
                Cancel
              </button>
              <button
                className="primary"
                disabled={busy || !noErrors(editorErrors)}
                type="submit"
              >
                {busy
                  ? "Saving…"
                  : editor.mode === "create"
                    ? "Create package"
                    : "Save package"}
              </button>
            </div>
          </form>
        )}
      </AccessibleDialog>

      <AccessibleDialog
        description="The duplicate receives a new identity and revision 1."
        onClose={closeDialogs}
        open={duplicateSource !== null}
        title="Duplicate macro package"
      >
        <form
          className="form-stack"
          onSubmit={(event) => {
            event.preventDefault();
            void duplicate();
          }}
        >
          <TextField
            error={
              utf8ByteLength(duplicateName.trim()) === 0
                ? "Name is required."
                : utf8ByteLength(duplicateName.trim()) > limits.packageNameBytes
                  ? `Name exceeds ${String(limits.packageNameBytes)} UTF-8 bytes.`
                  : undefined
            }
            id="duplicate-package-name"
            label="Duplicate name"
            maximumBytes={limits.packageNameBytes}
            onChange={setDuplicateName}
            value={duplicateName}
          />
          <ErrorBanner message={error} />
          <div className="form-actions">
            <button disabled={busy} onClick={closeDialogs} type="button">
              Cancel
            </button>
            <button
              className="primary"
              disabled={
                busy ||
                utf8ByteLength(duplicateName.trim()) === 0 ||
                utf8ByteLength(duplicateName.trim()) > limits.packageNameBytes
              }
              type="submit"
            >
              {busy ? "Duplicating…" : "Duplicate package"}
            </button>
          </div>
        </form>
      </AccessibleDialog>

      <AccessibleDialog
        description="Deletion removes the package and its package-owned content. This action cannot be undone."
        onClose={closeDialogs}
        open={deleteTarget !== null}
        title="Delete macro package"
      >
        {deleteTarget === null ? null : (
          <form
            className="form-stack"
            onSubmit={(event) => {
              event.preventDefault();
              void remove();
            }}
          >
            <p>
              Type <strong>{deleteTarget.name}</strong> to confirm deletion.
            </p>
            <label className="form-stack" htmlFor="delete-package-confirmation">
              Package name
              <input
                autoComplete="off"
                id="delete-package-confirmation"
                onChange={(event) => {
                  setDeleteConfirmation(event.currentTarget.value);
                }}
                value={deleteConfirmation}
              />
            </label>
            <ErrorBanner message={error} />
            <div className="form-actions">
              <button disabled={busy} onClick={closeDialogs} type="button">
                Cancel
              </button>
              <button
                className="danger"
                disabled={busy || deleteConfirmation !== deleteTarget.name}
                type="submit"
              >
                {busy ? "Deleting…" : "Delete permanently"}
              </button>
            </div>
          </form>
        )}
      </AccessibleDialog>
    </section>
  );
}
