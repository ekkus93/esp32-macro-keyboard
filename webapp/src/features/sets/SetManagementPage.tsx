import { useMemo, useState } from "react";
import { errorText } from "../../api/errors";
import {
  createSet,
  deleteSet,
  duplicateSet,
  reorderSets,
  updateSet,
} from "../../api/routes";
import { AccessibleDialog } from "../../components/AccessibleDialog";
import { ErrorBanner } from "../../components/ErrorBanner";
import { limits } from "../../types/limits";
import type { MacroSet, Settings } from "../../types/models";
import { utf8ByteLength } from "../macros/macroDraft";

interface SetManagementPageProps {
  sets: readonly MacroSet[];
  settings: Settings;
  onSetsChanged: (sets: MacroSet[]) => void;
}

type EditorState =
  | { mode: "create"; set: MacroSet }
  | { mode: "edit"; set: MacroSet }
  | null;

interface SetFieldErrors {
  board?: string;
  description?: string;
  manufacturer?: string;
  model?: string;
  name?: string;
}

/* The device returns sets in index order (SPEC 12.3), which is the user's
   order; the UI must preserve it rather than impose one of its own. */
function sortedSets(sets: readonly MacroSet[]): MacroSet[] {
  return [...sets];
}

function newSet(): MacroSet {
  return {
    schema_version: 1,
    id: crypto.randomUUID(),
    revision: 1,
    name: "",
  };
}

function validateSet(set: MacroSet): SetFieldErrors {
  const errors: SetFieldErrors = {};
  const nameBytes = utf8ByteLength(set.name);
  if (nameBytes === 0) {
    errors.name = "Name is required.";
  } else if (nameBytes > limits.setNameBytes) {
    errors.name = `Name exceeds ${String(limits.setNameBytes)} UTF-8 bytes.`;
  }
  return errors;
}

function noErrors(errors: SetFieldErrors): boolean {
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

export function SetManagementPage({
  sets,
  settings,
  onSetsChanged,
}: SetManagementPageProps): React.JSX.Element {
  const ordered = useMemo(() => sortedSets(sets), [sets]);
  const [editor, setEditor] = useState<EditorState>(null);
  const [duplicateSource, setDuplicateSource] = useState<MacroSet | null>(null);
  const [duplicateName, setDuplicateName] = useState("");
  const [deleteTarget, setDeleteTarget] = useState<MacroSet | null>(null);
  const [deleteConfirmation, setDeleteConfirmation] = useState("");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);

  const editorErrors = editor === null ? {} : validateSet(editor.set);

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
          ? await createSet(editor.set)
          : await updateSet(editor.set, editor.set.revision);
      const replacement =
        editor.mode === "create"
          ? [...ordered, committed]
          : ordered.map((set) => (set.id === committed.id ? committed : set));
      onSetsChanged(sortedSets(replacement));
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
      utf8ByteLength(duplicateName.trim()) > limits.setNameBytes ||
      busy
    ) {
      return;
    }
    setBusy(true);
    setError(null);
    setMessage(null);
    try {
      const committed = await duplicateSet(duplicateSource.id, {
        id: crypto.randomUUID(),
        name: duplicateName.trim(),
        expectedRevision: duplicateSource.revision,
      });
      onSetsChanged(sortedSets([...ordered, committed]));
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
      deleteTarget.id === settings.activeSetId ||
      deleteConfirmation !== deleteTarget.name ||
      busy
    ) {
      return;
    }
    setBusy(true);
    setError(null);
    setMessage(null);
    try {
      const result = await deleteSet(deleteTarget.id, deleteTarget.revision);
      if (result.id !== deleteTarget.id) {
        throw new Error("The device confirmed deletion for a different set.");
      }
      onSetsChanged(ordered.filter((set) => set.id !== deleteTarget.id));
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
      const committed = await reorderSets(replacement.map((set) => set.id));
      onSetsChanged(sortedSets(committed));
      setMessage(
        `Moved ${moved.name} to position ${String(destinationIndex + 1)}.`,
      );
    } catch (reorderError: unknown) {
      setError(errorText(reorderError));
    } finally {
      setBusy(false);
    }
  };

  const updateEditor = (field: keyof MacroSet, value: string): void => {
    setEditor((current) =>
      current === null
        ? null
        : {
            ...current,
            set: {
              ...current.set,
              [field]: value,
            },
          },
    );
  };

  return (
    <section aria-labelledby="manage-sets-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Persisted resources</p>
          <h2 id="manage-sets-title">Manage macro sets</h2>
          <p>
            Create, revise, duplicate, reorder, or delete complete macro sets.
          </p>
        </div>
        <button
          className="primary"
          disabled={busy || ordered.length >= limits.macroSets}
          onClick={() => {
            setMessage(null);
            setError(null);
            setEditor({ mode: "create", set: newSet() });
          }}
          type="button"
        >
          Create set
        </button>
      </div>

      {ordered.length >= limits.macroSets ? (
        <p className="conflict-message" role="status">
          The device already contains the maximum of {String(limits.macroSets)}{" "}
          sets.
        </p>
      ) : null}
      <ErrorBanner message={error} />
      {message === null ? null : (
        <p className="save-message" role="status" aria-live="polite">
          {message}
        </p>
      )}

      {ordered.length === 0 ? (
        <p role="status">No macro sets are stored.</p>
      ) : (
        <ol className="management-list" aria-label="Macro set order">
          {ordered.map((set, index) => {
            const active = settings.activeSetId === set.id;
            return (
              <li className="card management-card" key={set.id}>
                <div>
                  <div className="management-title-row">
                    <h3>{set.name}</h3>
                    {active ? (
                      <span className="status-badge status-good">
                        Active set
                      </span>
                    ) : null}
                  </div>
                  <dl className="metadata">
                    <dt>Position</dt>
                    <dd>
                      {String(index + 1)} of {String(ordered.length)}
                    </dd>
                    <dt>Revision</dt>
                    <dd>{String(set.revision)}</dd>
                  </dl>
                </div>

                <div className="management-actions">
                  <div
                    className="reorder-actions"
                    aria-label={`Reorder ${set.name}`}
                  >
                    <button
                      aria-label={`Move ${set.name} first`}
                      disabled={busy || index === 0}
                      onClick={() => {
                        void move(index, 0);
                      }}
                      type="button"
                    >
                      Move first
                    </button>
                    <button
                      aria-label={`Move ${set.name} up`}
                      disabled={busy || index === 0}
                      onClick={() => {
                        void move(index, index - 1);
                      }}
                      type="button"
                    >
                      Move up
                    </button>
                    <button
                      aria-label={`Move ${set.name} down`}
                      disabled={busy || index === ordered.length - 1}
                      onClick={() => {
                        void move(index, index + 1);
                      }}
                      type="button"
                    >
                      Move down
                    </button>
                    <button
                      aria-label={`Move ${set.name} last`}
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
                        setEditor({ mode: "edit", set: { ...set } });
                      }}
                      type="button"
                    >
                      Edit
                    </button>
                    <button
                      disabled={busy || ordered.length >= limits.macroSets}
                      onClick={() => {
                        setMessage(null);
                        setError(null);
                        setDuplicateSource(set);
                        setDuplicateName(`${set.name} copy`);
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
                        setDeleteTarget(set);
                        setDeleteConfirmation("");
                      }}
                      type="button"
                    >
                      Delete
                    </button>
                  </div>
                  {active ? (
                    <p className="field-help">
                      Select a different active set before deleting this one.
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
            ? "Create a new persisted macro set."
            : "Edit the selected set using its current revision."
        }
        onClose={closeDialogs}
        open={editor !== null}
        title={
          editor?.mode === "create" ? "Create macro set" : "Edit macro set"
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
              id="set-name"
              label="Name"
              maximumBytes={limits.setNameBytes}
              onChange={(value) => {
                updateEditor("name", value);
              }}
              value={editor.set.name}
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
                    ? "Create set"
                    : "Save set"}
              </button>
            </div>
          </form>
        )}
      </AccessibleDialog>

      <AccessibleDialog
        description="The duplicate receives a new identity and revision 1. Procedure progress is not copied."
        onClose={closeDialogs}
        open={duplicateSource !== null}
        title="Duplicate macro set"
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
                : utf8ByteLength(duplicateName.trim()) > limits.setNameBytes
                  ? `Name exceeds ${String(limits.setNameBytes)} UTF-8 bytes.`
                  : undefined
            }
            id="duplicate-set-name"
            label="Duplicate name"
            maximumBytes={limits.setNameBytes}
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
                utf8ByteLength(duplicateName.trim()) > limits.setNameBytes
              }
              type="submit"
            >
              {busy ? "Duplicating…" : "Duplicate set"}
            </button>
          </div>
        </form>
      </AccessibleDialog>

      <AccessibleDialog
        description="Deletion removes the set and its set-owned content. This action cannot be undone."
        onClose={closeDialogs}
        open={deleteTarget !== null}
        title="Delete macro set"
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
            <label className="form-stack" htmlFor="delete-set-confirmation">
              Set name
              <input
                autoComplete="off"
                id="delete-set-confirmation"
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
