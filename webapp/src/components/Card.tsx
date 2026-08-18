/**
 * The keycap surface every list row, settings group and diagnostics panel
 * sits on. Replaces the `.card` rule (and, for the one call site that stacked
 * them, `.card danger-zone`).
 *
 * The three variants are **complete literal class strings**, not a base string
 * with per-call-site overrides appended, for two reasons (§8.2, §8.6 of the
 * migration plan):
 *
 *  1. Tailwind only sees literal class names in source. Anything built by
 *     interpolation emits no CSS and fails silently.
 *  2. Conflicting utilities on one element are resolved by Tailwind's own
 *     stylesheet sort order, *not* by the order they appear in `className`.
 *     `.card danger-zone` works today only because `.danger-zone` happens to
 *     come later in `styles.css`; once these become utilities that guarantee
 *     is gone. Selecting a whole variant means no two conflicting utilities
 *     ever land on the same element, so there is nothing left to race.
 *
 * The `h2`/`h3`/`p` descendant rules ride along as `[&_h2]:` / `[&_h3]:` /
 * `[&_p]:` variants rather than being pushed out to call sites: the cards
 * contain far too many paragraphs (all of Diagnostics, every form's field
 * help, every row's summary line) for restyling each one to be safe or
 * readable. They keep the same `(0,1,1)` specificity the `.card p` selector
 * had, and no rule they now outrank was previously beating them — the only
 * later same-specificity rules on those elements are `.send-status p`,
 * `.dialog-heading p` and `.page-heading h2`, and none of those three ever
 * appears inside a card.
 */
const CARD_SHAPE =
  "grid gap-4 rounded-keycap p-4 min-[34rem]:items-start min-[34rem]:[grid-template-columns:1fr_auto] [&_h2]:mb-[0.3rem] [&_h2]:text-[1.05rem] [&_h3]:mb-[0.3rem] [&_h3]:text-[1.05rem] [&_p]:my-[0.2rem] [&_p]:text-legend-soft";

const CARD_CLASS = {
  /* Stands alone in page flow, so it carries its own vertical rhythm. */
  default: `${CARD_SHAPE} my-3 border border-cap-edge bg-panel`,
  /* A row inside a `gap`-managed list: the list owns the spacing. */
  flush: `${CARD_SHAPE} border border-cap-edge bg-panel`,
  /* "This permanently deletes something" — the alert palette and the left
     accent rule, with the wider top gap `.danger-zone` set. */
  danger: `${CARD_SHAPE} mb-3 mt-4 border-y border-r border-l-[3px] border-alert bg-bad-tint`,
} as const;

export type CardVariant = keyof typeof CARD_CLASS;

export interface CardProps {
  "aria-labelledby"?: string;
  children: React.ReactNode;
  variant?: CardVariant;
}

export function Card({
  "aria-labelledby": ariaLabelledBy,
  children,
  variant = "default",
}: CardProps): React.JSX.Element {
  return (
    <article aria-labelledby={ariaLabelledBy} className={CARD_CLASS[variant]}>
      {children}
    </article>
  );
}
