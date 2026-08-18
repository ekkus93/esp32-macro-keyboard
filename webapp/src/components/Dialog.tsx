/**
 * A modal surface: the scrim, the panel, and the panel's heading row.
 * Replaces `.dialog-backdrop`, `.dialog-panel`, `.dialog-heading` and its
 * `h2`/`p` descendant rule.
 *
 * These lived in a second stylesheet (`src/management.css`) that predated the
 * design system and never joined it: it hardcoded a cold slate/blue palette
 * against this app's warm PBT-and-petrol one, so every dialog rendered in a
 * different colour language than the page behind it. They are on the `@theme`
 * tokens now — the scrim is legend ink, the panel a raised keycap face, the
 * radius the same 0.5rem as every other surface.
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
 */
const DIALOG_PANEL_CLASS = {
  panel:
    "max-h-[calc(100dvh-2rem)] w-[min(100%,38rem)] overflow-y-auto rounded-keycap border border-cap-edge bg-cap p-4 shadow-[0_1.5rem_4rem_rgb(33_30_26_/_35%)]",
  danger:
    "mt-4 max-h-[calc(100dvh-2rem)] w-[min(100%,38rem)] overflow-y-auto rounded-keycap border-y border-r border-l-[3px] border-alert bg-bad-tint p-4 shadow-[0_1.5rem_4rem_rgb(33_30_26_/_35%)]",
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

export interface DialogProps {
  "aria-labelledby": string;
  children: React.ReactNode;
  containerRef: React.RefObject<HTMLDivElement | null>;
  heading: React.ReactNode;
  tone?: DialogTone;
}

export function Dialog({
  "aria-labelledby": ariaLabelledBy,
  children,
  containerRef,
  heading,
  tone = "panel",
}: DialogProps): React.JSX.Element {
  return (
    <div
      className="fixed inset-0 z-20 grid place-items-center overflow-y-auto bg-legend/70 p-4"
      role="presentation"
    >
      <div
        aria-labelledby={ariaLabelledBy}
        className={DIALOG_PANEL_CLASS[tone]}
        ref={containerRef}
        role="alertdialog"
        tabIndex={-1}
      >
        <div className={DIALOG_HEADING_CLASS}>{heading}</div>
        {children}
      </div>
    </div>
  );
}
