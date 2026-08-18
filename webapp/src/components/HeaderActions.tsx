/**
 * A right-aligned cluster of controls — in a `<PageHeading>`, in the shell
 * header, or beside a section title. Replaces `.header-actions`; all six call
 * sites were a bare `<div>`, so there is no variant beyond the narrow-screen
 * one below.
 *
 * `[@media(width<=32rem)]:justify-start` is the whole of the old
 * `@media (width <= 32rem)` rule. It is written as a bracketed at-rule and
 * **not** as `max-[32rem]:`, which Tailwind compiles to
 * `@media not all and (min-width:32rem)` — that is `width < 32rem`, and it
 * excludes a viewport exactly 512px wide. The bracketed form compiles to
 * `@media(max-width:32rem)`, the original condition. Neither the shell
 * header nor `<PageHeading>`
 * force-stacks at that width anymore: both stay one wrapping row and
 * truncate their title before it would need a second row, rather than
 * unconditionally paying for two or three full-height blocks regardless of
 * whether the content needed the room. All this rule does is stop the
 * actions hanging off the right edge once they have wrapped onto their own
 * line.
 */
export interface HeaderActionsProps {
  children: React.ReactNode;
}

export function HeaderActions({
  children,
}: HeaderActionsProps): React.JSX.Element {
  return (
    <div className="flex flex-wrap items-center justify-end gap-[0.4rem] [@media(width<=32rem)]:justify-start">
      {children}
    </div>
  );
}
