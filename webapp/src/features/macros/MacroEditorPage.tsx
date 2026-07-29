import { useEffect, useMemo, useRef, useState, type FormEvent } from "react";
import { ApiError } from "../../api/client";
import { errorText } from "../../api/errors";
import {
  createSetMacro,
  getSetMacro,
  updateSetMacro,
  validateSetMacro,
} from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { MacroEditorTarget } from "../../routing";
import { replaceMacroEditorTarget } from "../../routing";
import { limits } from "../../types/limits";
import type {
  Macro,
  MacroParseLocation,
  MacroSet,
  MacroValidation,
} from "../../types/models";
import {
  byteOffsetToCodeUnit,
  createMacroDraft,
  macroDirectives,
  macroFingerprint,
  parseMacroLocation,
  validateMacroLocally,
  validationSummary,
} from "./macroDraft";

interface MacroEditorPageProps {
  activeSet: MacroSet | null;
  target: MacroEditorTarget;
  onBack: () => void;
}

type ValidationState =
  | { kind: "idle" }
  | { kind: "pending"; fingerprint: string }
  | {
      kind: "valid";
      fingerprint: string;
      result: MacroValidation;
    }
  | {
      kind: "invalid";
      fingerprint: string;
      message: string;
      location: MacroParseLocation | null;
    }
  | { kind: "error"; fingerprint: string; message: string };

function newUuid(): string {
  return crypto.randomUUID();
}

function macroBelongsToSet(macro: Macro, setId: string): boolean {
  return macro.scope === "set" && macro.set_id === setId;
}

export function MacroEditorPage({
  activeSet,
  target,
  onBack,
}: MacroEditorPageProps): React.JSX.Element {
  const [draft, setDraft] = useState<Macro | null>(null);
  const [persisted, setPersisted] = useState(false);
  const [loading, setLoading] = useState(false);
  const [loadVersion, setLoadVersion] = useState(0);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [validation, setValidation] = useState<ValidationState>({
    kind: "idle",
  });
  const [saving, setSaving] = useState(false);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);
  const [conflict, setConflict] = useState(false);
  const sourceRef = useRef<HTMLTextAreaElement>(null);
  const pendingSelection = useRef<number | null>(null);

  const targetMacroId = target.kind === "edit" ? target.macroId : null;

  useEffect(() => {
    if (activeSet === null || target.kind === "invalid") {
      setDraft(null);
      setPersisted(false);
      setLoading(false);
      setLoadError(null);
      return;
    }
    setSaveMessage(null);
    setConflict(false);
    setLoadError(null);

    if (target.kind === "create") {
      setDraft(createMacroDraft(activeSet.id, newUuid()));
      setPersisted(false);
      setLoading(false);
      return;
    }

    if (targetMacroId === null) {
      setLoadError("The macro editor URL does not identify a macro.");
      return;
    }
    let active = true;
    setDraft(null);
    setLoading(true);
    void getSetMacro(activeSet.id, targetMacroId)
      .then((loaded) => {
        if (!macroBelongsToSet(loaded, activeSet.id)) {
          throw new Error(
            "The device returned a macro outside the active set.",
          );
        }
        if (active) {
          setDraft(loaded);
          setPersisted(true);
        }
      })
      .catch((error: unknown) => {
        if (active) {
          setLoadError(errorText(error));
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
  }, [activeSet, target.kind, targetMacroId, loadVersion]);

  const localValidation = useMemo(
    () => (draft === null ? null : validateMacroLocally(draft)),
    [draft],
  );
  const fingerprint = draft === null ? "" : macroFingerprint(draft);

  useEffect(() => {
    if (
      activeSet === null ||
      draft === null ||
      localValidation === null ||
      !localValidation.valid ||
      conflict
    ) {
      setValidation({ kind: "idle" });
      return;
    }
    let active = true;
    setValidation({ kind: "pending", fingerprint });
    const timer = window.setTimeout(() => {
      void validateSetMacro(activeSet.id, draft)
        .then((result) => {
          if (active) {
            setValidation({ kind: "valid", fingerprint, result });
          }
        })
        .catch((error: unknown) => {
          if (!active) {
            return;
          }
          const location = parseMacroLocation(error);
          if (error instanceof ApiError && error.status === 422) {
            setValidation({
              kind: "invalid",
              fingerprint,
              message: errorText(error),
              location,
            });
          } else {
            setValidation({
              kind: "error",
              fingerprint,
              message: errorText(error),
            });
          }
        });
    }, 300);
    return () => {
      active = false;
      window.clearTimeout(timer);
    };
  }, [activeSet, conflict, draft, fingerprint, localValidation]);

  useEffect(() => {
    if (pendingSelection.current === null) {
      return;
    }
    const position = pendingSelection.current;
    pendingSelection.current = null;
    sourceRef.current?.focus();
    sourceRef.current?.setSelectionRange(position, position);
  }, [draft?.source]);

  const updateDraft = (replacement: Partial<Macro>): void => {
    setSaveMessage(null);
    setDraft((current) =>
      current === null ? current : { ...current, ...replacement },
    );
  };

  const insertDirective = (value: string): void => {
    const textarea = sourceRef.current;
    if (textarea === null || draft === null) {
      return;
    }
    const start = textarea.selectionStart;
    const end = textarea.selectionEnd;
    const next = `${draft.source.slice(0, start)}${value}${draft.source.slice(end)}`;
    pendingSelection.current = start + value.length;
    updateDraft({ source: next });
  };

  const goToError = (): void => {
    if (
      draft === null ||
      validation.kind !== "invalid" ||
      validation.location === null
    ) {
      return;
    }
    const position = byteOffsetToCodeUnit(
      draft.source,
      validation.location.byteOffset,
    );
    sourceRef.current?.focus();
    sourceRef.current?.setSelectionRange(position, position);
  };

  const canSave =
    draft !== null &&
    localValidation?.valid === true &&
    validation.kind === "valid" &&
    validation.fingerprint === fingerprint &&
    !saving &&
    !conflict;

  const submit = async (event: FormEvent<HTMLFormElement>): Promise<void> => {
    event.preventDefault();
    if (activeSet === null || draft === null || !canSave) {
      return;
    }
    setSaving(true);
    setSaveMessage(null);
    try {
      const committed = persisted
        ? await updateSetMacro(activeSet.id, draft, draft.revision)
        : await createSetMacro(activeSet.id, draft);
      const created = !persisted;
      setDraft(committed);
      setPersisted(true);
      setConflict(false);
      if (created) {
        replaceMacroEditorTarget(committed.id);
      }
      setSaveMessage(
        created
          ? `Created revision ${String(committed.revision)}.`
          : `Saved revision ${String(committed.revision)}.`,
      );
    } catch (error: unknown) {
      if (error instanceof ApiError && error.status === 409) {
        setConflict(true);
      } else {
        setSaveMessage(errorText(error));
      }
    } finally {
      setSaving(false);
    }
  };

  const resolveConflict = (): void => {
    if (activeSet === null) {
      return;
    }
    if (persisted) {
      setLoadVersion((version) => version + 1);
      return;
    }
    setDraft(createMacroDraft(activeSet.id, newUuid()));
    setConflict(false);
  };

  if (activeSet === null) {
    return (
      <section aria-labelledby="macro-editor-title">
        <h2 id="macro-editor-title">Macro editor</h2>
        <p>Select an active macro set before creating or editing a macro.</p>
        <button onClick={onBack} type="button">
          Back to macros
        </button>
      </section>
    );
  }

  if (target.kind === "invalid") {
    return (
      <section aria-labelledby="macro-editor-title">
        <h2 id="macro-editor-title">Macro editor</h2>
        <p className="error-message" role="alert">
          The macro editor URL contains an invalid macro identifier.
        </p>
        <button onClick={onBack} type="button">
          Back to macros
        </button>
      </section>
    );
  }

  if (loading) {
    return (
      <section aria-labelledby="macro-editor-title" aria-busy="true">
        <h2 id="macro-editor-title">Macro editor</h2>
        <p role="status">Loading macro…</p>
      </section>
    );
  }

  if (loadError !== null || draft === null || localValidation === null) {
    return (
      <section aria-labelledby="macro-editor-title">
        <h2 id="macro-editor-title">Macro editor</h2>
        <ErrorBanner message={loadError ?? "Macro is unavailable."} />
        <div className="form-actions">
          {target.kind === "edit" ? (
            <button
              onClick={() => {
                setLoadVersion((version) => version + 1);
              }}
              type="button"
            >
              Retry
            </button>
          ) : null}
          <button onClick={onBack} type="button">
            Back to macros
          </button>
        </div>
      </section>
    );
  }

  const location = validation.kind === "invalid" ? validation.location : null;

  return (
    <section aria-labelledby="macro-editor-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">{activeSet.name}</p>
          <h2 id="macro-editor-title">
            {persisted ? "Edit macro" : "Create macro"}
          </h2>
          <p>Revision {String(draft.revision)}</p>
        </div>
        <button onClick={onBack} type="button">
          Back to macros
        </button>
      </div>

      {conflict ? (
        <div className="conflict-message" role="alert">
          <strong>Stale revision.</strong>
          <p>
            This macro changed on the device after it was loaded. The local
            draft was not overwritten. Reload the latest server version before
            saving again.
          </p>
          <button onClick={resolveConflict} type="button">
            {persisted ? "Reload latest" : "Start with a new identifier"}
          </button>
        </div>
      ) : null}

      <form
        className="form-stack"
        id="macro-editor-form"
        onSubmit={(event) => {
          void submit(event);
        }}
      >
        <label htmlFor="macro-name">
          Name
          <input
            id="macro-name"
            onChange={(event) => {
              updateDraft({ name: event.currentTarget.value });
            }}
            value={draft.name}
          />
          <span className="field-help">
            {String(localValidation.nameBytes)} /{" "}
            {String(limits.macroNameBytes)} UTF-8 bytes
          </span>
        </label>

        <label className="checkbox-row" htmlFor="macro-favorite">
          <input
            checked={draft.favorite}
            id="macro-favorite"
            onChange={(event) => {
              updateDraft({ favorite: event.currentTarget.checked });
            }}
            type="checkbox"
          />
          Favorite
        </label>

        <div className="timing-grid">
          <label htmlFor="macro-key-press">
            Key press time (ms)
            <input
              id="macro-key-press"
              max={limits.durationMs}
              min="0"
              onChange={(event) => {
                updateDraft({
                  key_press_ms: event.currentTarget.valueAsNumber,
                });
              }}
              step="1"
              type="number"
              value={Number.isNaN(draft.key_press_ms) ? "" : draft.key_press_ms}
            />
          </label>
          <label htmlFor="macro-inter-key">
            Inter-key time (ms)
            <input
              id="macro-inter-key"
              max={limits.durationMs}
              min="0"
              onChange={(event) => {
                updateDraft({
                  inter_key_ms: event.currentTarget.valueAsNumber,
                });
              }}
              step="1"
              type="number"
              value={Number.isNaN(draft.inter_key_ms) ? "" : draft.inter_key_ms}
            />
          </label>
        </div>

        <div>
          <span className="field-label">Insert directive</span>
          <div className="directive-toolbar" aria-label="Macro directives">
            {macroDirectives.map((directive) => (
              <button
                key={directive.label}
                onClick={() => {
                  insertDirective(directive.value);
                }}
                type="button"
              >
                {directive.label}
              </button>
            ))}
          </div>
        </div>

        <label htmlFor="macro-source">
          Macro source
          <textarea
            id="macro-source"
            onChange={(event) => {
              updateDraft({ source: event.currentTarget.value });
            }}
            ref={sourceRef}
            spellCheck="false"
            value={draft.source}
          />
          <span
            className={
              localValidation.sourceBytes > limits.macroSourceBytes
                ? "field-help limit-exceeded"
                : "field-help"
            }
          >
            {String(localValidation.sourceBytes)} /{" "}
            {String(limits.macroSourceBytes)} UTF-8 bytes
          </span>
        </label>

        {localValidation.issues.length > 0 ? (
          <div className="error-message" role="alert">
            <strong>Correct the local fields before validation.</strong>
            <ul>
              {localValidation.issues.map((issue) => (
                <li key={issue}>{issue}</li>
              ))}
            </ul>
          </div>
        ) : null}

        <div className="validation-card" aria-live="polite">
          <h3>Server validation</h3>
          {validation.kind === "idle" ? (
            <p>Waiting for a locally valid draft.</p>
          ) : null}
          {validation.kind === "pending" ? (
            <p aria-busy="true" role="status">
              Validating…
            </p>
          ) : null}
          {validation.kind === "valid" ? (
            <>
              <p className="validation-good">Macro is valid.</p>
              <p>{validationSummary(validation.result)}</p>
            </>
          ) : null}
          {validation.kind === "invalid" ? (
            <>
              <p className="validation-bad" role="alert">
                {validation.message}
              </p>
              {location === null ? null : (
                <p>
                  Line {String(location.line)}, column {String(location.column)}
                  , byte {String(location.byteOffset)}.
                </p>
              )}
              {location === null ? null : (
                <button onClick={goToError} type="button">
                  Go to error
                </button>
              )}
            </>
          ) : null}
          {validation.kind === "error" ? (
            <p className="validation-bad" role="alert">
              Validation unavailable: {validation.message}
            </p>
          ) : null}
        </div>

        {saveMessage === null ? null : (
          <p
            className={
              saveMessage.startsWith("Created") ||
              saveMessage.startsWith("Saved")
                ? "save-message"
                : "error-message"
            }
            role="status"
          >
            {saveMessage}
          </p>
        )}

        <div className="form-actions">
          <button className="primary" disabled={!canSave} type="submit">
            {saving ? "Saving…" : persisted ? "Save macro" : "Create macro"}
          </button>
          <button onClick={onBack} type="button">
            Cancel
          </button>
        </div>
      </form>

      <details>
        <summary>Macro syntax reference</summary>
        <p>
          Printable ASCII types literally. Use doubled braces for literal
          braces, named-key or chord directives in braces, and delays from 1 to{" "}
          {String(limits.delayMs)} ms.
        </p>
        <p>
          The server parser is authoritative. The editor never executes source
          during validation.
        </p>
      </details>
    </section>
  );
}
