import { useEffect, useRef } from "react";
import type { RefObject } from "react";

/**
 * Closes a non-modal overlay (a popup/overflow menu, as opposed to a
 * `role="alertdialog"` — those want {@link useFocusTrap} instead) on
 * `Escape` or on a pointer press outside its container, per UI_UX_SPEC_V2
 * §14 ("all controls keyboard accessible") and TODO_V2 V2-133's audit note
 * that `MacrosPage.tsx`'s "More actions" overflow menu had neither.
 */
export function useDismissibleOverlay(
  active: boolean,
  containerRef: RefObject<HTMLElement | null>,
  onDismiss: () => void,
): void {
  const onDismissRef = useRef(onDismiss);
  useEffect(() => {
    onDismissRef.current = onDismiss;
  }, [onDismiss]);

  useEffect(() => {
    if (!active) {
      return;
    }

    const keydown = (event: KeyboardEvent): void => {
      if (event.key === "Escape") {
        event.preventDefault();
        onDismissRef.current();
      }
    };
    const pointerdown = (event: PointerEvent): void => {
      const container = containerRef.current;
      if (
        container !== null &&
        event.target instanceof Node &&
        !container.contains(event.target)
      ) {
        onDismissRef.current();
      }
    };

    document.addEventListener("keydown", keydown);
    document.addEventListener("pointerdown", pointerdown);
    return () => {
      document.removeEventListener("keydown", keydown);
      document.removeEventListener("pointerdown", pointerdown);
    };
  }, [active, containerRef]);
}
