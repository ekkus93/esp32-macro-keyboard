import { useEffect, useRef, useState } from "react";
import { Dialog } from "../../../components/Dialog";
import { Eyebrow } from "../../../components/Eyebrow";
import { FormActions } from "../../../components/FormActions";
import { HeaderActions } from "../../../components/HeaderActions";
import {
  PAGE_HEADING_TITLE_CLASS,
  PageHeading,
} from "../../../components/PageHeading";
import { FieldHelp } from "../../../components/FieldHelp";
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
  // CTRL/SHIFT/ALT/GUI are standalone directives too (SPEC_V2 §7.7): outside
  // a [...] group each taps and releases that modifier alone, so they insert
  // the same way as any other named key. To press several keys at once, type
  // '[' and ']' directly around the sequence, e.g. [{CTRL}{SHIFT}t] -- that
  // needs no dedicated button, since '[' and ']' are ordinary characters
  // already typeable in Macro source like any other text.
  "CTRL",
  "SHIFT",
  "ALT",
  "GUI",
] as const;

const namedDirectives: readonly NamedDirective[] = namedKeyTokens.map(
  (token) => ({ label: token, token: `{${token}}` }),
);

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
  const [delayMs, setDelayMs] = useState(500);
  const [advancedOpen, setAdvancedOpen] = useState(false);
  const advancedContainerRef = useRef<HTMLDivElement>(null);
  useFocusTrap({
    active: advancedOpen,
    containerRef: advancedContainerRef,
    onClose: () => {
      setAdvancedOpen(false);
    },
  });

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
    // No nested scroll region: an earlier version gave this <section> a
    // fixed top part plus its own separately-scrolling <form> below (with a
    // `short:` fallback to fall back out of that split entirely once the
    // fixed part alone could exceed a short viewport). That split had a bug
    // its own `short:` fallback didn't cover -- the fixed region's real
    // height (heading + Name + Macro source, all with real content) varies
    // with package/macro name length and dirty-header state, and once Save
    // changes joined it there were viewport heights *above* the short:
    // threshold where the fixed region still left the scrolling form less
    // room than its own content needed, so *that* region grew a small
    // permanent inner scrollbar -- the exact nested-scroll-view problem the
    // `short:` fallback was written to test for, just triggered from the
    // other side. Fixed by not having two scroll regions at all: this
    // <section> is now one ordinary block flowing through #main-content
    // (the app's single scrolling region, same as every other screen), and
    // the heading/Name/Macro source group below uses `sticky` instead of
    // being structurally split out -- it stays visible while the directive
    // toolbar scrolls past underneath, without ever clipping that toolbar
    // away the way a zero-height overflow:auto container would.
    <section aria-labelledby="macro-editor-title">
      <div className="sticky top-0 z-10 bg-shell pb-3">
        <PageHeading>
          <div className="min-w-0 overflow-hidden">
            <Eyebrow tone="dark">{pkg.name}</Eyebrow>
            <h2 className={PAGE_HEADING_TITLE_CLASS} id="macro-editor-title">
              {macroId === null ? "Create macro" : "Edit macro"}
            </h2>
          </div>
          {/* Save changes moved up from its own fixed bottom bar: with it gone,
            the scrolling form below gets that space back instead of paying
            for a second pinned strip. `form` keeps Save associated with
            #macro-editor-form for submission despite living outside it,
            same as Name and Macro source above. Still disabled on the same
            `canSave` condition -- moving it didn't change when it's safe to
            submit, only where the control sits. */}
          <HeaderActions>
            <button
              onClick={() => {
                setAdvancedOpen(true);
              }}
              type="button"
            >
              Advanced
            </button>
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
          </HeaderActions>
        </PageHeading>

        {/* Sticky, not merely pinned via a structural split: everything
            below inserts at Macro source's cursor and needs to stay visible
            while that happens. Name lives here too, above Macro source --
            re-measured on a real Chrome instance (the "unsaved changes"
            dirty header, a representative portrait phone height) to confirm
            it still leaves real room for the toolbar once stuck at the top
            of #main-content's scroll. `form` keeps Name and the textarea
            associated with #macro-editor-form for submission despite living
            outside its DOM subtree.

            `block` on both labels below is load-bearing, not decorative:
            `<label>` is inline by default in HTML, and neither Tailwind's
            preflight nor this app's own base `label{}` rule (font-weight
            only) overrides that. Every other label in this app has looked
            like a real block box only because it happened to be a flex or
            grid item -- flexbox/grid blockify their children regardless of
            the child's own `display` -- and this sticky wrapper is a plain
            `<div>`, the first place in the app a label sits in ordinary
            block flow. Without `block` here the label collapses to an
            inline box sized to its content's line boxes, and Macro source's
            panel below it overlaps Name instead of following it -- found by
            screenshotting this exact change, not by inspection. */}
        <label className="block" htmlFor="macro-editor-name">
          Name
          <input
            form="macro-editor-form"
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
        </label>

        <label
          className="block rounded-keycap border border-cap-edge bg-panel p-3"
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
          <FieldHelp exceeded={sourceBytes > v2Limits.macroSourceMaxBytes}>
            {String(sourceBytes)} / {String(v2Limits.macroSourceMaxBytes)} UTF-8
            bytes
          </FieldHelp>
        </label>
      </div>

      <form
        className="grid gap-[0.85rem]"
        id="macro-editor-form"
        onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
          event.preventDefault();
          save();
        }}
      >
        <div>
          <span className="font-bold">Insert directive</span>
          {/* Named-key directives are a keyboard's own row of keys, so they
              get a real grid of uniform cells rather than flex-wrap's
              ragged, label-width-dependent edge — a physical key is the same
              size regardless of what's printed on it, and "BACKSPACE"
              sitting flush against "UP" at two different heights read as
              unfinished, not deliberate. auto-fill keeps it fluid to 320px.
              0.5rem (8px) gap, not the tighter 0.4rem used elsewhere:
              adjacent 44px touch targets need 8-12px of separation to avoid
              mis-taps, and this grid is the densest cluster of controls on
              the page. Directive inserts are the small keys on the board:
              monospace legends. */}
          <div
            aria-label="Named-key directives"
            className="mt-2 grid [grid-template-columns:repeat(auto-fill,minmax(4.5rem,1fr))] gap-2 [&_button]:min-h-[44px] [&_button]:px-[0.6rem] [&_button]:py-[0.4rem] [&_button]:font-mono [&_button]:text-[0.78rem]"
          >
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

          {/* Insert delay inserts at Macro source's cursor, same as the
              directive grid above, so it stays in view here rather than
              behind Advanced -- moving it there once hid the insertion from
              view while it happened, which wasn't worth the space saved.
              There is no dedicated group builder: CTRL/SHIFT/ALT/GUI are
              ordinary buttons in the grid above, and the square brackets that
              mark a simultaneous-key group are ordinary characters -- typing
              them directly around a sequence of directive clicks needs no
              extra UI. */}
          <div
            aria-label="Insert delay"
            className="mt-2 flex flex-wrap gap-[0.4rem]"
          >
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
        </div>

        {/* No bordered panel, no "Validation" heading, in either state --
            the previous version only dropped those for the all-valid case
            and kept the full panel (border, padding, heading) the moment
            either the source or the name was invalid, which is also a
            common state (an empty "Create macro" draft hits it
            immediately) and was still paying full panel cost there. Plain
            `aria-live` text carries the same required content in every
            state (SPEC_V2 §7.1: exact error location, Go to error, action
            count and estimated duration when valid) without ever paying
            for a box to show it in. */}
        <div aria-live="polite">
          {compiled.ok ? (
            <p className="font-bold text-good">
              Macro is valid — {estimatedDurationText(compiled)}
            </p>
          ) : (
            <>
              <p className="font-bold text-alert" role="alert">
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
            <p className="font-bold text-alert" role="alert">
              Name must be non-empty after trimming and at most{" "}
              {String(v2Limits.macroNameMaxBytes)} UTF-8 bytes.
            </p>
          )}
        </div>
      </form>

      {/* Set-once metadata, not part of the insert workflow that the fixed
          region above and the directive toolbar below both serve -- moved
          into its own dialog (not the scrolling form) so it stops
          permanently occupying page space for two fields most edits never
          touch. `role="dialog"`, not `alertdialog`: this edits ordinary
          field values with no consequential action to confirm. `form`
          keeps both inputs associated with #macro-editor-form for
          submission despite living outside its DOM subtree. */}
      {advancedOpen ? (
        <Dialog
          aria-labelledby="macro-editor-advanced-title"
          containerRef={advancedContainerRef}
          heading={<h2 id="macro-editor-advanced-title">Advanced</h2>}
          role="dialog"
        >
          {/* `block` on every label below: `<label>` is inline by default
              and Dialog's panel is a plain div, not flex/grid, so nothing
              else blockifies it -- full rationale at Name's label above. */}
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
          <label className="block" htmlFor="macro-editor-inter-key">
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
  );
}
