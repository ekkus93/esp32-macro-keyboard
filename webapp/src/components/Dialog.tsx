import { useLayoutEffect, useRef } from "react";

/**
 * A modal surface: the scrim, the panel, and the panel's heading row.
 * Replaces `.dialog-backdrop`, `.dialog-panel`, `.dialog-heading` and its
 * `h2`/`p` descendant rule.
 *
 * The M3 dialog: a `scrim` wash, and a high-container panel with the extra-
 * large corner and level-3 elevation. All values come from the `@theme`
 * tokens.
 *
 * ## `tone`
 *
 * `tone="danger"` is a **complete literal class string**, not `panel` plus a
 * `.danger-zone` class on top (§8.2, §8.6). The old markup stacked
 * `dialog-panel danger-zone`, which produced the right result only because
 * `.danger-zone` happened to sit later in `styles.css`; the two disagree on
 * border colour, left border width and background, so as markup utilities the
 * winner would have been Tailwind's own sort order rather than anything
 * either file states. The `danger` string below is that stacked result
 * written out once, `mt-4` included.
 *
 * `max-h-[calc(100dvh-2rem)]` is `dvh`, not `vh`, deliberately: `vh` measures
 * the largest-possible viewport, so on a mobile browser the bottom of a
 * full-height dialog sat behind the address bar with no scroll path to it.
 * This is a max-height on an `overflow-y: auto` panel, which is exactly the
 * case where that clips content away rather than merely looking off.
 *
 * ## `role`
 *
 * Originally hardcoded to `"alertdialog"` (reviewed deliberately,
 * `WEBAPP_TAILWIND_TODO_2026-08-18.md` T4-3): every call site at the time
 * — `ConfirmPhraseDialog`, `UnsavedChangesPrompt`, the restart confirmation
 * in `SettingsPage` — was exactly what `alertdialog` is for, a modal
 * demanding an immediate yes/no response to a consequential action, and the
 * comment here said the day a general content/editing surface actually
 * needed this component, `role` would become a required prop rather than
 * carrying an unused option. `MacroEditorPage`'s Advanced panel is
 * that day: it edits ordinary field values and triggers non-consequential
 * inserts, none needing a yes/no response, so `alertdialog`'s implicit
 * "respond now" semantics would be wrong for assistive tech there. Required, not defaulted, so every call
 * site states its intent rather than inheriting the original three's.
 */
const DIALOG_PANEL_CLASS = {
  panel:
    "max-h-[calc(100dvh-2rem)] w-[min(100%,38rem)] overflow-y-auto rounded-shape-xl bg-surface-container-high p-6 text-on-surface shadow-elevation-3",
  danger:
    "mt-4 max-h-[calc(100dvh-2rem)] w-[min(100%,38rem)] overflow-y-auto rounded-shape-xl bg-error-container p-6 text-on-error-container shadow-elevation-3",
} as const;

/**
 * The heading row stacks at and below 40rem rather than keeping the title and
 * its trailing control on one line.
 *
 * Written as a bracketed at-rule, **not** as `max-[40rem]:`, and the
 * difference is not cosmetic: Tailwind compiles `max-[40rem]:` to
 * `@media not all and (min-width:40rem)`, which is `width < 40rem` and so
 * excludes a viewport exactly 640px wide. The stylesheet rule this replaces
 * said `width <= 40rem`, which includes it. `[@media(width<=40rem)]:`
 * compiles to `@media(max-width:40rem)` — the original condition exactly.
 */
const DIALOG_HEADING_CLASS =
  "mb-4 flex items-start justify-between gap-4 [@media(width<=40rem)]:flex-col [@media(width<=40rem)]:items-stretch [&_h2]:mt-0 [&_p]:mt-0";

export type DialogTone = keyof typeof DIALOG_PANEL_CLASS;
export type DialogRole = "alertdialog" | "dialog";

export interface DialogProps {
  "aria-labelledby": string;
  children: React.ReactNode;
  containerRef: React.RefObject<HTMLDivElement | null>;
  heading: React.ReactNode;
  role: DialogRole;
  tone?: DialogTone;
}

export function Dialog({
  "aria-labelledby": ariaLabelledBy,
  children,
  containerRef,
  heading,
  role,
  tone = "panel",
}: DialogProps): React.JSX.Element {
  const surfaceRef = useRef<HTMLDivElement>(null);
  useLayoutEffect(() => {
    const surface = surfaceRef.current;
    if (surface === null) {
      return;
    }
    const previousInert: { element: HTMLElement; inert: boolean }[] = [];
    let branch: HTMLElement = surface;
    while (branch.parentElement !== null) {
      const parent: HTMLElement = branch.parentElement;
      for (const sibling of Array.from(parent.children)) {
        if (sibling === branch || !(sibling instanceof HTMLElement)) {
          continue;
        }
        previousInert.push({
          element: sibling,
          inert: sibling.hasAttribute("inert"),
        });
        sibling.setAttribute("inert", "");
      }
      if (parent === document.body) {
        break;
      }
      branch = parent;
    }
    return () => {
      for (const previous of previousInert.reverse()) {
        if (previous.inert) {
          previous.element.setAttribute("inert", "");
        } else {
          previous.element.removeAttribute("inert");
        }
      }
    };
  }, []);

  return (
    <div
      className="fixed inset-0 z-20 grid place-items-center overflow-y-auto bg-scrim/40 p-4"
      role="presentation"
      ref={surfaceRef}
    >
      <div
        aria-modal="true"
        aria-labelledby={ariaLabelledBy}
        className={DIALOG_PANEL_CLASS[tone]}
        ref={containerRef}
        role={role}
        tabIndex={-1}
      >
        <div className={DIALOG_HEADING_CLASS}>{heading}</div>
        {children}
      </div>
    </div>
  );
}
