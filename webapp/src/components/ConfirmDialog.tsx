import { useRef } from "react";
import type { ReactNode } from "react";
import { useFocusTrap } from "../features/shell/v2/useFocusTrap";
import { Dialog } from "./Dialog";
import { FormActions } from "./FormActions";

/**
 * A confirmation that names its object and its consequence
 * (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §6: "Avoid generic 'Are you sure?'"): an
 * M3 modal `alertdialog` with a title, a body the caller writes, a Cancel and
 * one confirming action labelled for what it does ("Delete macro", not "OK").
 *
 * It is mounted only while open, so it traps focus for its whole life and, on
 * close, `useFocusTrap` returns focus to whatever had it when it opened
 * (typically the control that opened the menu that opened this). Escape
 * cancels. Destructive confirmations default to the `danger` tone; the
 * confirming button then uses the error colour.
 */
export interface ConfirmDialogProps {
  children: ReactNode;
  confirmDisabled?: boolean;
  confirmLabel: string;
  /** Extra accessible name for the confirming button when its label alone is ambiguous. */
  confirmAriaLabel?: string;
  /** A stable id for the title, so the dialog's accessible name is set. */
  id: string;
  onCancel: () => void;
  onConfirm: () => void;
  title: string;
  tone?: "danger" | "panel";
}

export function ConfirmDialog({
  children,
  confirmAriaLabel,
  confirmDisabled = false,
  confirmLabel,
  id,
  onCancel,
  onConfirm,
  title,
  tone = "danger",
}: ConfirmDialogProps): React.JSX.Element {
  const containerRef = useRef<HTMLDivElement>(null);
  useFocusTrap({ active: true, containerRef, onClose: onCancel });
  return (
    <Dialog
      aria-labelledby={id}
      containerRef={containerRef}
      heading={<h2 id={id}>{title}</h2>}
      role="alertdialog"
      tone={tone}
    >
      {children}
      <FormActions>
        <button onClick={onCancel} type="button">
          Cancel
        </button>
        <button
          aria-label={confirmAriaLabel}
          className={tone === "danger" ? "danger" : "primary"}
          disabled={confirmDisabled}
          onClick={onConfirm}
          type="button"
        >
          {confirmLabel}
        </button>
      </FormActions>
    </Dialog>
  );
}
