import { useEffect, useId, useRef } from "react";
import type { ReactNode } from "react";

interface AccessibleDialogProps {
  children: ReactNode;
  description: string;
  open: boolean;
  title: string;
  onClose: () => void;
}

const focusableSelector = [
  "button:not([disabled])",
  "input:not([disabled])",
  "select:not([disabled])",
  "textarea:not([disabled])",
  "a[href]",
  '[tabindex]:not([tabindex="-1"])',
].join(",");

export function AccessibleDialog({
  children,
  description,
  open,
  title,
  onClose,
}: AccessibleDialogProps): React.JSX.Element | null {
  const titleId = useId();
  const descriptionId = useId();
  const containerRef = useRef<HTMLDivElement>(null);
  const returnFocusRef = useRef<HTMLElement | null>(null);
  const onCloseRef = useRef(onClose);

  useEffect(() => {
    onCloseRef.current = onClose;
  }, [onClose]);

  useEffect(() => {
    if (!open) {
      return;
    }

    returnFocusRef.current =
      document.activeElement instanceof HTMLElement
        ? document.activeElement
        : null;
    const container = containerRef.current;
    const focusable = container?.querySelectorAll<HTMLElement>(focusableSelector);
    focusable?.item(0).focus();

    const keydown = (event: KeyboardEvent): void => {
      if (event.key === "Escape") {
        event.preventDefault();
        onCloseRef.current();
        return;
      }
      if (event.key !== "Tab" || container === null) {
        return;
      }
      const current = Array.from(
        container.querySelectorAll<HTMLElement>(focusableSelector),
      ).filter((element) => !element.hasAttribute("disabled"));
      if (current.length === 0) {
        event.preventDefault();
        container.focus();
        return;
      }
      const first = current[0];
      const last = current.at(-1);
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
      returnFocusRef.current?.focus();
      returnFocusRef.current = null;
    };
  }, [open]);

  if (!open) {
    return null;
  }

  return (
    <div className="dialog-backdrop">
      <div
        aria-describedby={descriptionId}
        aria-labelledby={titleId}
        aria-modal="true"
        className="dialog-panel"
        ref={containerRef}
        role="dialog"
        tabIndex={-1}
      >
        <div className="dialog-heading">
          <div>
            <h2 id={titleId}>{title}</h2>
            <p id={descriptionId}>{description}</p>
          </div>
          <button aria-label={`Close ${title}`} onClick={onClose} type="button">
            Close
          </button>
        </div>
        {children}
      </div>
    </div>
  );
}
