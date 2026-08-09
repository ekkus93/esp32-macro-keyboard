import { useEffect } from "react";

/**
 * Registers a `beforeunload` warning while the repository working copy is
 * dirty, per SPEC_V2 §8.7/UI_UX_SPEC_V2 §7.3 (TODO_V2 V2-103): "reload and
 * tab-close warnings are registered where supported." The listener is added
 * only while `dirty` is true and removed the instant it becomes false or the
 * component unmounts, so a saved or discarded working copy never leaves a
 * stale prompt behind.
 *
 * `preventDefault()` is the current standard way to trigger the browser's
 * own confirmation prompt; the legacy `event.returnValue` string assignment
 * some old engines needed instead is deprecated. "Where supported" covers
 * browsers that don't implement `beforeunload` at all: it simply never
 * fires there, which this hook cannot and does not need to detect. The
 * browser — not this application — owns the prompt's exact wording; no code
 * path here claims unsaved work can be recovered after that native prompt
 * is dismissed.
 */
export function useBeforeUnloadGuard(dirty: boolean): void {
  useEffect(() => {
    if (!dirty) {
      return;
    }
    const onBeforeUnload = (event: BeforeUnloadEvent): void => {
      event.preventDefault();
    };
    window.addEventListener("beforeunload", onBeforeUnload);
    return () => {
      window.removeEventListener("beforeunload", onBeforeUnload);
    };
  }, [dirty]);
}
