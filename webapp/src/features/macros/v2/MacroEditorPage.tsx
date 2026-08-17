import { useEffect, useRef, useState } from "react";
import { compileMacro } from "../../../v2/macroCompiler";
import type { MacroCompileResult } from "../../../v2/macroCompiler";
import { v2Limits } from "../../../v2/limits";
import { utf8ByteLength, type RepositoryMacro } from "../../../v2/repository";
import {
  addMacro,
  createMacroId,
  findMacro,
  findPackage,
  macrosEqual,
  updateMacro,
} from "../../../v2/repositoryEditing";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";

/**
 * The Macro editor, per UI_UX_SPEC_V2 §7.1 (TODO_V2 V2-100). Purely local:
 * validation reuses `compileMacro` (the same TypeScript implementation of
 * SPEC_V2 §7 the repository-validation and preview screens use — "run live
 * TypeScript validation against the shared corpus implementation" means
 * calling this, not a second parser), and Save writes only to the in-memory
 * working copy via `store.applyContentChange` — it never calls a firmware
 * macro route and never uploads a snapshot (SPEC_V2 §7.1 "Saving updates the
 * in-memory package... does not call a firmware macro route and does not
 * create a repository snapshot automatically").
 */

export interface MacroEditorPageProps {
  store: RepositoryWorkingCopyStore;
  packageId: string;
  /** `null` creates a new macro; a string edits the macro with that ID. */
  macroId: string | null;
  onBack: () => void;
  onSaved: () => void;
}

interface NamedDirective {
  label: string;
  token: string;
}

const namedKeyTokens = [
  "ENTER",
  "TAB",
  "ESC",
  "BACKSPACE",
  "DELETE",
  "INSERT",
  "HOME",
  "END",
  "PAGEUP",
  "PAGEDOWN",
  "UP",
  "DOWN",
  "LEFT",
  "RIGHT",
  "SPACE",
  "F1",
  "F2",
  "F3",
  "F4",
  "F5",
  "F6",
  "F7",
  "F8",
  "F9",
  "F10",
  "F11",
  "F12",
] as const;

const namedDirectives: readonly NamedDirective[] = namedKeyTokens.map(
  (token) => ({ label: token, token: `{${token}}` }),
);

const modifiers = ["CTRL", "SHIFT", "ALT", "GUI"] as const;
type Modifier = (typeof modifiers)[number];

/**
 * Converts a UTF-8 byte offset (as `compileMacro` errors report, per SPEC_V2
 * §7.10) back to a JS string index for `HTMLTextAreaElement.setSelectionRange`.
 * Walks character-by-character rather than assuming 1 byte per character —
 * correct even though this grammar's valid prefix before any error is always
 * ASCII (SPEC_V2 §7.5), so byte offset and code-unit offset happen to match
 * there; this keeps "Go to error" correct without depending on that.
 */
function byteOffsetToCodeUnit(source: string, byteOffset: number): number {
  if (byteOffset <= 0) {
    return 0;
  }
  let bytes = 0;
  let codeUnit = 0;
  for (const character of source) {
    const nextBytes = bytes + utf8ByteLength(character);
    if (nextBytes > byteOffset) {
      break;
    }
    bytes = nextBytes;
    codeUnit += character.length;
  }
  return codeUnit;
}

function estimatedDurationText(
  result: Extract<MacroCompileResult, { ok: true }>,
): string {
  return `${String(result.actions.length)} actions · ${String(result.estimatedDurationMs)} ms estimated`;
}

export function MacroEditorPage({
  store,
  packageId,
  macroId,
  onBack,
  onSaved,
}: MacroEditorPageProps): React.JSX.Element {
  const [snapshot, setSnapshot] = useState(() => store.getSnapshot());
  useEffect(() => store.subscribe(setSnapshot), [store]);
  const repository = snapshot.repository;

  const pkg = findPackage(repository, packageId);
  const located = macroId === null ? undefined : findMacro(repository, macroId);
  const original: RepositoryMacro | null =
    macroId === null ? null : (located?.macro ?? null);
  const missingEditTarget = macroId !== null && located === undefined;

  const [name, setName] = useState(original?.name ?? "");
  const [source, setSource] = useState(original?.source ?? "");
  const [keyPressMs, setKeyPressMs] = useState(original?.keyPressMs ?? 8);
  const [interKeyMs, setInterKeyMs] = useState(original?.interKeyMs ?? 15);

  const sourceRef = useRef<HTMLTextAreaElement>(null);
  const pendingSelection = useRef<number | null>(null);
  const [chordKey, setChordKey] = useState("");
  const [chordModifiers, setChordModifiers] = useState<ReadonlySet<Modifier>>(
    new Set(),
  );
  const [delayMs, setDelayMs] = useState(500);

  useEffect(() => {
    if (pendingSelection.current === null) {
      return;
    }
    const position = pendingSelection.current;
    pendingSelection.current = null;
    sourceRef.current?.focus();
    sourceRef.current?.setSelectionRange(position, position);
  }, [source]);

  if (pkg === undefined || missingEditTarget) {
    return (
      <section aria-labelledby="macro-editor-title">
        <h2 id="macro-editor-title">Macro editor</h2>
        <p role="alert">
          {pkg === undefined
            ? "The selected package is no longer in this repository."
            : "This macro is no longer in the repository."}
        </p>
        <button onClick={onBack} type="button">
          Back to Macros
        </button>
      </section>
    );
  }

  const insertAtCursor = (text: string): void => {
    const textarea = sourceRef.current;
    const start = textarea?.selectionStart ?? source.length;
    const end = textarea?.selectionEnd ?? source.length;
    const next = `${source.slice(0, start)}${text}${source.slice(end)}`;
    pendingSelection.current = start + text.length;
    setSource(next);
  };

  const insertChord = (): void => {
    const key = chordKey.trim().toUpperCase();
    if (key.length === 0 || chordModifiers.size === 0) {
      return;
    }
    const ordered = modifiers.filter((modifier) =>
      chordModifiers.has(modifier),
    );
    insertAtCursor(`{${[...ordered, key].join("+")}}`);
  };

  const insertDelay = (): void => {
    if (
      !Number.isSafeInteger(delayMs) ||
      delayMs < 1 ||
      delayMs > v2Limits.delayDirectiveMaxMs
    ) {
      return;
    }
    insertAtCursor(`{DELAY:${String(delayMs)}}`);
  };

  const nameBytes = utf8ByteLength(name);
  const nameValid =
    name.trim().length > 0 && nameBytes <= v2Limits.macroNameMaxBytes;
  const sourceBytes = utf8ByteLength(source);
  const compiled = compileMacro(source, { keyPressMs, interKeyMs });
  const canSave = nameValid && compiled.ok;

  const goToError = (): void => {
    if (compiled.ok) {
      return;
    }
    const position = byteOffsetToCodeUnit(source, compiled.error.byteOffset);
    sourceRef.current?.focus();
    sourceRef.current?.setSelectionRange(position, position);
  };

  const save = (): void => {
    if (!canSave) {
      return;
    }
    const candidate: RepositoryMacro = {
      id: macroId ?? createMacroId(),
      name,
      source,
      keyPressMs,
      interKeyMs,
    };
    if (macroId === null) {
      const next = addMacro(repository, packageId, candidate);
      store.applyContentChange(next);
      onSaved();
      return;
    }
    // Avoid a dirty transition after a no-op edit (TODO_V2 V2-101).
    if (original !== null && macrosEqual(original, candidate)) {
      onSaved();
      return;
    }
    const next = updateMacro(repository, macroId, {
      name,
      source,
      keyPressMs,
      interKeyMs,
    });
    store.applyContentChange(next);
    onSaved();
  };

  return (
    <section aria-labelledby="macro-editor-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">{pkg.name}</p>
          <h2 id="macro-editor-title">
            {macroId === null ? "Create macro" : "Edit macro"}
          </h2>
        </div>
        <button onClick={onBack} type="button">
          Cancel
        </button>
      </div>

      <form
        className="form-stack"
        onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
          event.preventDefault();
          save();
        }}
      >
        <label htmlFor="macro-editor-name">
          Name
          <input
            id="macro-editor-name"
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setName(event.currentTarget.value);
            }}
            value={name}
          />
          <span
            className={
              nameBytes > v2Limits.macroNameMaxBytes
                ? "field-help limit-exceeded"
                : "field-help"
            }
          >
            {String(nameBytes)} / {String(v2Limits.macroNameMaxBytes)} UTF-8
            bytes
          </span>
        </label>

        {/* Pinned deliberately: every directive button below inserts at this
            textarea's cursor, so it stays on screen while the buttons and
            everything else scroll underneath it -- no scrolling back and
            forth to see what a click just did. */}
        <label className="macro-source-pinned" htmlFor="macro-editor-source">
          Macro source
          <textarea
            id="macro-editor-source"
            onChange={(event: React.ChangeEvent<HTMLTextAreaElement>) => {
              setSource(event.currentTarget.value);
            }}
            ref={sourceRef}
            spellCheck="false"
            value={source}
          />
          <span
            className={
              sourceBytes > v2Limits.macroSourceMaxBytes
                ? "field-help limit-exceeded"
                : "field-help"
            }
          >
            {String(sourceBytes)} / {String(v2Limits.macroSourceMaxBytes)} UTF-8
            bytes
          </span>
        </label>

        <div>
          <span className="field-label">Insert directive</span>
          <div aria-label="Named-key directives" className="directive-grid">
            {namedDirectives.map((directive) => (
              <button
                key={directive.token}
                onClick={() => {
                  insertAtCursor(directive.token);
                }}
                type="button"
              >
                {directive.label}
              </button>
            ))}
            <button
              onClick={() => {
                insertAtCursor("{{");
              }}
              type="button"
            >
              Literal {"{"}
            </button>
            <button
              onClick={() => {
                insertAtCursor("}}");
              }}
              type="button"
            >
              Literal {"}"}
            </button>
          </div>

          <div className="toolbar-columns">
            <div aria-label="Insert delay" className="directive-toolbar">
              <label htmlFor="macro-editor-delay-ms">
                Delay (ms)
                <input
                  id="macro-editor-delay-ms"
                  max={v2Limits.delayDirectiveMaxMs}
                  min={1}
                  onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                    setDelayMs(event.currentTarget.valueAsNumber);
                  }}
                  type="number"
                  value={Number.isNaN(delayMs) ? "" : delayMs}
                />
              </label>
              <button onClick={insertDelay} type="button">
                Insert delay
              </button>
            </div>

            <div aria-label="Insert chord" className="directive-toolbar">
              {modifiers.map((modifier) => (
                <label
                  className="checkbox-row"
                  htmlFor={`macro-editor-chord-${modifier}`}
                  key={modifier}
                >
                  <input
                    checked={chordModifiers.has(modifier)}
                    id={`macro-editor-chord-${modifier}`}
                    onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                      const checked = event.currentTarget.checked;
                      setChordModifiers((current) => {
                        const next = new Set(current);
                        if (checked) {
                          next.add(modifier);
                        } else {
                          next.delete(modifier);
                        }
                        return next;
                      });
                    }}
                    type="checkbox"
                  />
                  {modifier}
                </label>
              ))}
              <label htmlFor="macro-editor-chord-key">
                Key
                <input
                  id="macro-editor-chord-key"
                  onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                    setChordKey(event.currentTarget.value);
                  }}
                  value={chordKey}
                />
              </label>
              <button onClick={insertChord} type="button">
                Insert chord
              </button>
            </div>
          </div>
        </div>

        {/* Set-once metadata, not part of the insert workflow, so it moved
            below the tools that are. */}
        <div className="timing-grid">
          <label htmlFor="macro-editor-key-press">
            Key-press duration (ms)
            <input
              id="macro-editor-key-press"
              max={v2Limits.keyPressMaxMs}
              min={0}
              onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                setKeyPressMs(event.currentTarget.valueAsNumber);
              }}
              step={1}
              type="number"
              value={Number.isNaN(keyPressMs) ? "" : keyPressMs}
            />
          </label>
          <label htmlFor="macro-editor-inter-key">
            Inter-key delay (ms)
            <input
              id="macro-editor-inter-key"
              max={v2Limits.interKeyMaxMs}
              min={0}
              onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                setInterKeyMs(event.currentTarget.valueAsNumber);
              }}
              step={1}
              type="number"
              value={Number.isNaN(interKeyMs) ? "" : interKeyMs}
            />
          </label>
        </div>

        <div className="validation-card" aria-live="polite">
          <h3>Validation</h3>
          {compiled.ok ? (
            <>
              <p className="validation-good">Macro is valid.</p>
              <p>{estimatedDurationText(compiled)}</p>
            </>
          ) : (
            <>
              <p className="validation-bad" role="alert">
                {compiled.error.message}
              </p>
              <p>
                Line {String(compiled.error.line)}, column{" "}
                {String(compiled.error.column)}, byte{" "}
                {String(compiled.error.byteOffset)}.
              </p>
              <button onClick={goToError} type="button">
                Go to error
              </button>
            </>
          )}
          {nameValid ? null : (
            <p className="validation-bad" role="alert">
              Name must be non-empty after trimming and at most{" "}
              {String(v2Limits.macroNameMaxBytes)} UTF-8 bytes.
            </p>
          )}
        </div>

        <div className="form-actions">
          <button className="primary" disabled={!canSave} type="submit">
            Save changes
          </button>
          <button onClick={onBack} type="button">
            Cancel
          </button>
        </div>
      </form>
    </section>
  );
}
