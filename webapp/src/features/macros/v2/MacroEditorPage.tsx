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
    // The macro editor's own two-region split, inside #main-content (already
    // the app's one scrolling region): a fixed top part (page heading + Macro
    // source) and the scrolling form below, which is the only part of this
    // page that scrolls.
    //
    // short: (@media height <= 38rem) is load-bearing, found by testing the
    // "avoid nested scroll views" question, not by guessing: below ~38rem
    // (600px) of viewport *height* -- a landscape phone, or any short window
    // -- the fixed region (heading + Name + Macro source, whose textarea has
    // its own 12rem min-height floor) can be taller than the whole screen.
    // This <section> has no overflow:hidden of its own, so that overflow
    // doesn't stay contained -- it pushes past the section's height:100% box,
    // and #main-content's own overflow-y:auto (correctly) starts scrolling to
    // compensate. But the scrolling form's flex-computed height still gets
    // clamped to zero in that state, and a zero-height overflow:auto
    // container doesn't just hide its content, it clips it away with no
    // scroll path back to it -- the entire directive toolbar and the Save
    // footer become permanently unreachable, not merely awkward to reach.
    // Pinning also isn't a good trade at this height regardless: there isn't
    // room left to show even one row of buttons once it's pinned. So below
    // the threshold, give up on the fixed/scroll split entirely (short:h-auto
    // here, short:flex-none short:overflow-y-visible on the form below) and
    // fall back to one ordinary scrolling page through #main-content, the
    // same as every other screen in the app.
    <section
      aria-labelledby="macro-editor-title"
      className="flex h-full flex-col short:h-auto"
    >
      <div className="page-heading">
        <div className="page-heading-title">
          <p className="eyebrow dark">{pkg.name}</p>
          <h2 id="macro-editor-title">
            {macroId === null ? "Create macro" : "Edit macro"}
          </h2>
        </div>
        <button onClick={onBack} type="button">
          Cancel
        </button>
      </div>

      {/* Fixed, not merely pinned via scroll tracking: everything below
          inserts at this textarea's cursor and needs to stay visible while
          that happens. Name moved back into the scrolling form below --
          measured on the real device that keeping both Name and Macro
          source fixed left as little as 24px for the entire directive
          toolbar in the ordinary "unsaved changes" state (the 3-line app
          header when dirty makes this worse than the clean state this was
          first tested against), nowhere near enough to fit a single 44px
          button. Naming before building was a nice-to-have; it isn't worth
          that cost. `form` keeps this associated with #macro-editor-form
          for submission despite living outside its DOM subtree. */}
      <label
        className="rounded-keycap border border-cap-edge bg-panel p-3 [flex:0_0_auto]"
        htmlFor="macro-editor-source"
      >
        Macro source
        <textarea
          form="macro-editor-form"
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

      <form
        className="min-h-0 [flex:1_1_auto] overflow-y-auto overscroll-y-contain short:flex-none short:overflow-y-visible form-stack"
        id="macro-editor-form"
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
              {/* Toggle buttons, not checkboxes: a native checkbox is
                  ~22px, half the 44px touch-target floor, and wrapping it
                  in a taller label only makes the *row* legal, not the
                  control itself. A pressable keycap-style toggle is both a
                  real 44px target and matches the same "physical key"
                  language as the directive buttons above, rather than
                  mixing in a different control type. */}
              {modifiers.map((modifier) => {
                const pressed = chordModifiers.has(modifier);
                return (
                  <button
                    aria-pressed={pressed}
                    className={
                      pressed ? "chord-modifier active" : "chord-modifier"
                    }
                    key={modifier}
                    onClick={() => {
                      setChordModifiers((current) => {
                        const next = new Set(current);
                        if (next.has(modifier)) {
                          next.delete(modifier);
                        } else {
                          next.add(modifier);
                        }
                        return next;
                      });
                    }}
                    type="button"
                  >
                    {modifier}
                  </button>
                );
              })}
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
      </form>

      {/* Fixed, in the thumb-reachable bottom of the screen, not the end of
          a scroll: Save is the primary action on this page, and burying it
          behind ~30 directive buttons meant reaching it required scrolling
          past all of them first. `form` keeps Save associated with
          #macro-editor-form for submission despite living outside it. A
          hairline top border marks where the scrolling content ends, the
          same device .bottom-nav uses to mark where it begins. */}
      <div className="flex flex-wrap gap-2 border-t border-cap-edge pt-3 [flex:0_0_auto]">
        <button
          className="primary"
          disabled={!canSave}
          form="macro-editor-form"
          type="submit"
        >
          Save changes
        </button>
        <button onClick={onBack} type="button">
          Cancel
        </button>
      </div>
    </section>
  );
}
