/**
 * The title row at the top of a route: the heading itself, whatever context
 * line sits under it, and (usually) a `<HeaderActions>` group pushed to the
 * far end. Replaces `.page-heading` and its `.page-heading h2` descendant
 * rule.
 *
 * One wrapping row at every width, like the shell header — not the
 * unconditional two-row stack this used to force on phones. Measured on a
 * real device: on the macro editor specifically, that stacking cost 120px
 * versus roughly 64px here, and that difference was most of why the directive
 * toolbar had no room left to render at all in the ordinary "unsaved changes"
 * state.
 *
 * ## The `h2` styling lives at the six call sites, not here
 *
 * This used to ride along as `[&_h2]:` variants on this component (the same
 * truncate-before-wrapping protection the shell header's own title has, for
 * the same reason — a package name is arbitrary-length, up to 64 UTF-8
 * bytes). Moved out (T3-1, WEBAPP_TAILWIND_TODO_2026-08-18.md, closing
 * WEBAPP_TAILWIND_SPEC_2026-08-18.md §6.1's precedence hazard): `Card` also
 * carries a `[&_h2]:` rule, at the same `(0,1,1)` specificity, and the two
 * would resolve by Tailwind's emission order — not by anything either file
 * states — the moment a `PageHeading` and a `Card` ever nested. None do
 * today, but a rule that is only safe because of what currently happens to
 * be true is exactly the trap §6.1 named. Putting the utilities directly on
 * each call site's `<h2>` makes its specificity unambiguous regardless of
 * what it is ever nested inside.
 */
const PAGE_HEADING_CLASS =
  "mb-5 flex flex-wrap items-center justify-between gap-x-4 gap-y-3 border-b border-cap-edge pb-3";

/** The class every `PageHeading`'s own `<h2>` carries — see above. */
export const PAGE_HEADING_TITLE_CLASS =
  "m-0 overflow-hidden text-ellipsis whitespace-nowrap text-[1.35rem] tracking-[-0.01em]";

export interface PageHeadingProps {
  children: React.ReactNode;
}

export function PageHeading({ children }: PageHeadingProps): React.JSX.Element {
  return <div className={PAGE_HEADING_CLASS}>{children}</div>;
}
