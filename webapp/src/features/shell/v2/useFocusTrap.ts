import { useEffect, useRef } from "react";
import type { RefObject } from "react";

/**
 * Shared dialog focus-trap-and-restore behavior, per UI_UX_SPEC_V2 §14
 * ("Dialogs trap focus and restore it to their invoking control") and
 * TODO_V2 V2-133. This is the v2 equivalent of the logic
 * `components/AccessibleDialog.tsx` (the retired v1 tree) already had —
 * extracted into a hook so every v2 `role="alertdialog"`/`role="dialog"`
 * surface can apply it to its own markup instead of each reimplementing it,
 * since the v2 dialogs (`MacrosPage`'s delete confirmation,
 * `PackageManagementPage`'s danger zone, `SnapshotsPage`'s danger zones,
 * `SettingsPage`'s confirm dialogs, `UnsavedChangesPrompt`) do not share
 * `AccessibleDialog`'s markup/CSS and were previously plain `<div>`s with no
 * focus management at all.
 *
 * While `active`, this:
 * - moves initial focus to the first focusable element inside the container;
 * - wraps `Tab`/`Shift+Tab` at the container's boundary so focus never
 *   escapes to the page behind the dialog;
 * - closes on `Escape` via `onClose`;
 * - restores focus to whatever had it before the dialog opened, the instant
 *   it becomes inactive or unmounts.
 */

const focusableSelector = [
  "button:not([disabled])",
  "input:not([disabled])",
  "select:not([disabled])",
  "textarea:not([disabled])",
  "a[href]",
  '[tabindex]:not([tabindex="-1"])',
].join(",");

export interface UseFocusTrapOptions {
  /** Whether the dialog is currently open/mounted and should trap focus. */
  active: boolean;
  /** The dialog's outermost focusable container (needs `tabIndex={-1}`). */
  containerRef: RefObject<HTMLElement | null>;
  /** Called on `Escape`. Typically the same handler a Cancel button uses. */
  onClose: () => void;
  /**
   * Overrides the automatic "whatever had focus when the trap activated"
   * capture for where focus is restored once the trap deactivates. Some
   * callers (a row's "Delete" button that a ternary swaps out for its own
   * confirmation panel, rather than layering the confirmation over a
   * persistent trigger) unmount the invoking control in the very render
   * that activates the trap — by the time this hook's own effect runs, that
   * original DOM node is already gone, so the automatic capture would only
   * ever find `document.body`. Pass a ref that the *replacement* trigger
   * element (rendered once the trap deactivates) attaches to instead; React
   * assigns it during the same commit that unmounts the dialog, before this
   * hook's cleanup runs, so it already points at the right node.
   */
  restoreFocusRef?: RefObject<HTMLElement | null>;
}

export function useFocusTrap({
  active,
  containerRef,
  onClose,
  restoreFocusRef,
}: UseFocusTrapOptions): void {
  const onCloseRef = useRef(onClose);
  useEffect(() => {
    onCloseRef.current = onClose;
  }, [onClose]);

  useEffect(() => {
    if (!active) {
      return;
    }

    const returnFocusTo =
      document.activeElement instanceof HTMLElement
        ? document.activeElement
        : null;
    const container = containerRef.current;
    const initial =
      container?.querySelectorAll<HTMLElement>(focusableSelector).item(0) ??
      container ??
      null;
    initial?.focus();

    const keydown = (event: KeyboardEvent): void => {
      if (event.key === "Escape") {
        event.preventDefault();
        onCloseRef.current();
        return;
      }
      if (event.key !== "Tab" || container === null) {
        return;
      }
      const focusable = Array.from(
        container.querySelectorAll<HTMLElement>(focusableSelector),
      );
      if (focusable.length === 0) {
        event.preventDefault();
        container.focus();
        return;
      }
      const first = focusable[0];
      const last = focusable.at(-1);
      if (first === undefined || last === undefined) {
        return;
      }
      if (event.shiftKey && document.activeElement === first) {
        event.preventDefault();
        last.focus();
      } else if (!event.shiftKey && document.activeElement === last) {
        event.preventDefault();
        first.focus();
      }
    };

    document.addEventListener("keydown", keydown);
    return () => {
      document.removeEventListener("keydown", keydown);
      // When `restoreFocusRef` is supplied, the dedicated effect below
      // handles restoration instead — see its own comment for why that
      // needs to be a separate effect rather than read here.
      if (restoreFocusRef === undefined) {
        returnFocusTo?.focus();
      }
    };
  }, [active, containerRef, restoreFocusRef]);

  // Handles `restoreFocusRef` in its own effect, deliberately separate from
  // the cleanup above: a cleanup closure reading `someRef.current` is
  // reading whatever value the ref held at the moment cleanup runs, which
  // is exactly what's wanted here (the *replacement* trigger element React
  // just committed) — but is also exactly the pattern
  // `react-hooks/exhaustive-deps` flags as usually-a-bug ("will likely have
  // changed by the time this effect cleanup function runs"), because it
  // usually isn't wanted. Reading the same ref from a plain effect body
  // instead, keyed on the `active` transition, gets the correct live value
  // without tripping that check.
  const wasActiveRef = useRef(active);
  useEffect(() => {
    if (restoreFocusRef !== undefined && wasActiveRef.current && !active) {
      restoreFocusRef.current?.focus();
    }
    wasActiveRef.current = active;
  }, [active, restoreFocusRef]);
}
