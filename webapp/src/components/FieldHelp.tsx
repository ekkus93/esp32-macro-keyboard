/**
 * The small print under a form control — a byte counter, a format rule, a
 * length limit. Replaces `.field-help` and, with it, `.limit-exceeded`, which
 * had no other call site.
 *
 * `exceeded` selects a **complete literal class string** rather than
 * appending an override (§8.2). The two states disagree on `font-weight` and
 * `color`; as a class pair that worked only because `.limit-exceeded` sat
 * later in `styles.css` than `.field-help`, and as two utilities on one
 * element it would have been resolved by Tailwind's own stylesheet order
 * rather than by the order in `className`.
 *
 * `as` is explicit, and defaults to the tag that is always safe, because the
 * element type is load-bearing in two ways: `<p>` is invalid inside a
 * `<label>` (where six of these live), and `<Card>` carries `[&_p]:` rules
 * that apply to the paragraph form and not the span form. Changing a tag here
 * changes rendering.
 */
const FIELD_HELP_CLASS = {
  within: "mt-[0.3rem] block text-[0.8rem] font-normal text-legend-soft",
  exceeded: "mt-[0.3rem] block text-[0.8rem] font-bold text-alert",
} as const;

export interface FieldHelpProps {
  as?: "p" | "span";
  children: React.ReactNode;
  exceeded?: boolean;
}

export function FieldHelp({
  as = "span",
  children,
  exceeded = false,
}: FieldHelpProps): React.JSX.Element {
  const className = FIELD_HELP_CLASS[exceeded ? "exceeded" : "within"];
  if (as === "p") {
    return <p className={className}>{children}</p>;
  }
  return <span className={className}>{children}</span>;
}
