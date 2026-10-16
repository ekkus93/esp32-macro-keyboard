import { useEffect, useRef, useState } from "react";
import { Dialog } from "../../../components/Dialog";
import { FormActions } from "../../../components/FormActions";
import { FieldHelp } from "../../../components/FieldHelp";
import { Button } from "../../../components/m3/Button";
import { Icon } from "../../../components/m3/Icon";
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
import { useFocusTrap } from "../../shell/v2/useFocusTrap";
import { MacroKeyPicker } from "./MacroKeyPicker";

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
 *
 * UXR-042 layout: Cancel and Save sit at the top of the page, so they are on
 * screen at 320x568 whatever the validation message below them says. Name,
 * source and validation follow, then a "Keys and combinations" button that
 * opens the picker (`MacroKeyPicker`) as a focused view of this same page.
 * The editor form stays mounted but hidden while the picker is open, so the
 * name, source, timing and cursor survive the round trip.
 */

export interface MacroEditorPageProps {
  store: RepositoryWorkingCopyStore;
  packageId: string;
  /** `null` creates a new macro; a string edits the macro with that ID. */
  macroId: string | null;
  onBack: () => void;
  onSaved: () => void;
}

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
  const [pickerOpen, setPickerOpen] = useState(false);
  const [advancedOpen, setAdvancedOpen] = useState(false);
  const advancedContainerRef = useRef<HTMLDivElement>(null);
  useFocusTrap({
    active: advancedOpen,
    containerRef: advancedContainerRef,
    onClose: () => {
      setAdvancedOpen(false);
    },
  });

  // After an insert, put the caret just past it. While the picker is open the
  // textarea is hidden and cannot take focus, but its selection is still
  // recorded, so the next insert lands after this one.
  useEffect(() => {
    if (pendingSelection.current === null) {
      return;
    }
    const position = pendingSelection.current;
    pendingSelection.current = null;
    if (!pickerOpen) {
      sourceRef.current?.focus();
    }
    sourceRef.current?.setSelectionRange(position, position);
  }, [source, pickerOpen]);

  // Returning from the picker puts the caret back in the source, where the
  // inserted text is. Skipped on the first render so opening the editor does
  // not by itself raise a phone's keyboard.
  const pickerWasOpen = useRef(false);
  useEffect(() => {
    if (pickerWasOpen.current && !pickerOpen) {
      sourceRef.current?.focus();
    }
    pickerWasOpen.current = pickerOpen;
  }, [pickerOpen]);

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
    <>
      {pickerOpen ? (
        <MacroKeyPicker
          onDone={() => {
            setPickerOpen(false);
          }}
          onInsert={insertAtCursor}
        />
      ) : null}
      <section aria-label="Macro editor" hidden={pickerOpen}>
        <p className="m-0 mb-2 flex min-w-0 items-center gap-2 text-label-lg text-on-surface-variant">
          <Icon name="layers" size={18} />
          <span className="truncate">{pkg.name}</span>
        </p>
        <div className="mb-4 flex justify-end gap-2">
          <button onClick={onBack} type="button">
            Cancel
          </button>
          <button
            className="primary"
            disabled={!canSave}
            form="macro-editor-form"
            type="submit"
          >
            Save changes
          </button>
        </div>

        <form
          className="grid gap-4"
          id="macro-editor-form"
          onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
            event.preventDefault();
            save();
          }}
        >
          <div>
            <label htmlFor="macro-editor-name">Name</label>
            <input
              aria-invalid={!nameValid}
              id="macro-editor-name"
              onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                setName(event.currentTarget.value);
              }}
              value={name}
            />
            <FieldHelp exceeded={nameBytes > v2Limits.macroNameMaxBytes}>
              {String(nameBytes)} / {String(v2Limits.macroNameMaxBytes)} UTF-8
              bytes
            </FieldHelp>
          </div>

          <div>
            <label htmlFor="macro-editor-source">Macro source</label>
            <textarea
              aria-invalid={!compiled.ok}
              id="macro-editor-source"
              onChange={(event: React.ChangeEvent<HTMLTextAreaElement>) => {
                setSource(event.currentTarget.value);
              }}
              ref={sourceRef}
              spellCheck="false"
              value={source}
            />
            <FieldHelp exceeded={sourceBytes > v2Limits.macroSourceMaxBytes}>
              {String(sourceBytes)} / {String(v2Limits.macroSourceMaxBytes)}{" "}
              UTF-8 bytes
            </FieldHelp>
          </div>

          <div className="flex flex-wrap gap-2">
            <Button
              icon="keyboard"
              onClick={() => {
                setPickerOpen(true);
              }}
              variant="tonal"
            >
              Keys and combinations
            </Button>
            <Button
              onClick={() => {
                setAdvancedOpen(true);
              }}
              variant="text"
            >
              Timing
            </Button>
          </div>

          {/* Plain `aria-live` text, no panel: exact error location, Go to
              error, and action count with estimated duration when valid
              (SPEC_V2 §7.1). The check here is validation only -- nothing is
              typed on the computer until a macro is sent from the Macros
              page. */}
          <div aria-live="polite">
            {compiled.ok ? (
              <p className="m-0 flex items-center gap-2 font-semibold text-success">
                <Icon name="checkCircle" size={20} />
                Macro is valid — {estimatedDurationText(compiled)}
              </p>
            ) : (
              <>
                <p
                  className="m-0 flex items-start gap-2 font-semibold text-error"
                  role="alert"
                >
                  <Icon name="error" size={20} />
                  {compiled.error.message}
                </p>
                <p className="mt-1 mb-2 text-body-md">
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
              <p className="mt-2 font-semibold text-error" role="alert">
                Name must be non-empty after trimming and at most{" "}
                {String(v2Limits.macroNameMaxBytes)} UTF-8 bytes.
              </p>
            )}
          </div>
        </form>

        {/* Timing is set-once metadata, not part of the insert workflow, so it
            lives in its own dialog rather than permanently occupying page
            space for two fields most edits never touch. `role="dialog"`, not
            `alertdialog`: it edits ordinary field values with no
            consequential action to confirm. `form` keeps both inputs
            associated with #macro-editor-form for submission despite living
            outside its DOM subtree. */}
        {advancedOpen ? (
          <Dialog
            aria-labelledby="macro-editor-advanced-title"
            containerRef={advancedContainerRef}
            heading={<h2 id="macro-editor-advanced-title">Timing</h2>}
            role="dialog"
          >
            {/* `block` on every label below: `<label>` is inline by default
                and Dialog's panel is a plain div, not flex/grid, so nothing
                else blockifies it. */}
            <label className="block" htmlFor="macro-editor-key-press">
              Key-press duration (ms)
              <input
                form="macro-editor-form"
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
            <label className="mt-4 block" htmlFor="macro-editor-inter-key">
              Inter-key delay (ms)
              <input
                form="macro-editor-form"
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

            <FormActions>
              <button
                onClick={() => {
                  setAdvancedOpen(false);
                }}
                type="button"
              >
                Done
              </button>
            </FormActions>
          </Dialog>
        ) : null}
      </section>
    </>
  );
}
