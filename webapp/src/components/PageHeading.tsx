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
 * The `h2` rule rides along as `[&_h2]:` variants rather than moving to the
 * six call sites: it is what makes the title truncate before it would need a
 * second row — the same protection the shell header's title has, for the same
 * reason (a package name is arbitrary-length, up to 64 UTF-8 bytes) — and
 * that protection belongs to this component, not to whoever happens to render
 * a heading inside it. None of the six `<h2>`s carries a utility class of its
 * own, so nothing races these inside the utilities layer.
 */
const PAGE_HEADING_CLASS =
  "mb-5 flex flex-wrap items-center justify-between gap-x-4 gap-y-3 border-b border-cap-edge pb-3 " +
  "[&_h2]:m-0 [&_h2]:overflow-hidden [&_h2]:text-ellipsis [&_h2]:whitespace-nowrap [&_h2]:text-[1.35rem] [&_h2]:tracking-[-0.01em]";

export interface PageHeadingProps {
  children: React.ReactNode;
}

export function PageHeading({ children }: PageHeadingProps): React.JSX.Element {
  return <div className={PAGE_HEADING_CLASS}>{children}</div>;
}
