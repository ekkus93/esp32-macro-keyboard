import { useEffect, useRef } from "react";
import { IconButton } from "./Button";

export interface SnackbarProps {
  message: string;
  onDismiss: () => void;
}

/**
 * A dismissible, polite confirmation for an action that needs no decision or
 * follow-up. It sits above the phone navigation and respects the home-indicator
 * safe area (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §6).
 */
export function Snackbar({
  message,
  onDismiss,
}: SnackbarProps): React.JSX.Element {
  const onDismissRef = useRef(onDismiss);
  onDismissRef.current = onDismiss;

  useEffect(() => {
    const timeout = window.setTimeout(() => {
      onDismissRef.current();
    }, 7000);
    return () => {
      window.clearTimeout(timeout);
    };
  }, [message]);

  return (
    <div
      aria-live="polite"
      className="fixed inset-x-4 bottom-[calc(5rem+env(safe-area-inset-bottom))] z-30 mx-auto flex max-w-[38rem] items-center justify-between gap-4 rounded-shape-md bg-inverse-surface px-4 py-3 text-body-md text-inverse-on-surface shadow-elevation-3 md:bottom-6"
      role="status"
    >
      <span>{message}</span>
      <IconButton
        icon="close"
        label="Dismiss notification"
        onClick={onDismiss}
      />
    </div>
  );
}
