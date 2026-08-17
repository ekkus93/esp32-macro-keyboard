import { useCallback, useRef, useState } from "react";
import type { RepositoryMacro } from "../../../v2/repository";
import { useDismissibleOverlay } from "../../shell/v2/useDismissibleOverlay";
import { useFocusTrap } from "../../shell/v2/useFocusTrap";

/**
 * Overflow menu (TODO_V2 V2-101, closing a gap Phase 9 flagged rather than
 * faked — UI_UX_SPEC_V2 §5.1 "overflow actions such as Preview and send,
 * Duplicate, Move, and Delete"). Delete asks for an explicit, name-bearing
 * confirmation before it ever touches the working copy, matching the
 * destructive-target identification UI_UX_SPEC_V2 §6.2 requires for package
 * deletion.
 *
 * TODO_V2 V2-133/UI_UX_SPEC_V2 §14: the menu itself closes on `Escape` or an
 * outside click (`useDismissibleOverlay`), and the delete confirmation traps
 * and restores focus like any other dialog (`useFocusTrap`) — this menu had
 * neither before the accessibility audit that opened V2-133.
 */
export function MacroOverflowMenu({
  macro,
  onPreview,
  onDuplicate,
  onDelete,
}: {
  macro: RepositoryMacro;
  onPreview: () => void;
  onDuplicate: () => void;
  onDelete: () => void;
}): React.JSX.Element {
  const [open, setOpen] = useState(false);
  const [confirmingDelete, setConfirmingDelete] = useState(false);
  const containerRef = useRef<HTMLDivElement>(null);
  const confirmRef = useRef<HTMLDivElement>(null);
  // The "Delete" trigger is unmounted (replaced by the confirmation panel)
  // the same render that opens the trap, so `useFocusTrap`'s own automatic
  // "whatever had focus when it activated" capture would only ever find
  // `document.body`. This ref is attached to the *replacement* "Delete"
  // button that remounts once the trap deactivates (see the hook's doc
  // comment on `restoreFocusRef`).
  const deleteButtonRef = useRef<HTMLButtonElement>(null);

  const closeMenu = useCallback((): void => {
    setOpen(false);
    setConfirmingDelete(false);
  }, []);
  useDismissibleOverlay(open && !confirmingDelete, containerRef, closeMenu);

  const cancelDelete = useCallback((): void => {
    setConfirmingDelete(false);
  }, []);
  useFocusTrap({
    active: confirmingDelete,
    containerRef: confirmRef,
    onClose: cancelDelete,
    restoreFocusRef: deleteButtonRef,
  });

  return (
    <div className="relative" ref={containerRef}>
      <button
        aria-expanded={open}
        aria-label={`More actions for ${macro.name}`}
        onClick={() => {
          setOpen((current) => !current);
          setConfirmingDelete(false);
        }}
        type="button"
      >
        More actions
      </button>
      {open ? (
        <div
          aria-label={`Actions for ${macro.name}`}
          className="mt-2 grid min-w-[12rem] gap-2 rounded-keycap border border-cap-edge bg-panel p-3 shadow-[0_0.5rem_1.5rem_rgb(33_30_26_/_18%)]"
        >
          <button
            aria-label={`Preview and send ${macro.name}`}
            onClick={() => {
              setOpen(false);
              onPreview();
            }}
            type="button"
          >
            Preview and send
          </button>
          <button
            aria-label={`Duplicate ${macro.name}`}
            onClick={() => {
              setOpen(false);
              onDuplicate();
            }}
            type="button"
          >
            Duplicate
          </button>
          {confirmingDelete ? (
            <div
              className="my-4 rounded-keycap border border-l-[3px] border-lamp bg-warning-tint p-4"
              ref={confirmRef}
              role="alertdialog"
              tabIndex={-1}
            >
              <p>
                Delete <strong>{macro.name}</strong>? This cannot be undone once
                the working copy is saved.
              </p>
              <button
                className="danger"
                onClick={() => {
                  setOpen(false);
                  setConfirmingDelete(false);
                  onDelete();
                }}
                type="button"
              >
                Confirm delete
              </button>
              <button onClick={cancelDelete} type="button">
                Cancel
              </button>
            </div>
          ) : (
            <button
              aria-label={`Delete ${macro.name}`}
              className="danger"
              onClick={() => {
                setConfirmingDelete(true);
              }}
              ref={deleteButtonRef}
              type="button"
            >
              Delete
            </button>
          )}
        </div>
      ) : null}
    </div>
  );
}
