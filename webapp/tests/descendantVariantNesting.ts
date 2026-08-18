/**
 * Detects the exact hazard WEBAPP_TAILWIND_SPEC_2026-08-18.md §6.1 names:
 * two components that own the same descendant variant (`[&_p]:`, `[&_h2]:`,
 * ...) both scoping the same element, because one is rendered inside the
 * other. When that happens the two rules sit at identical specificity and
 * the winner is decided by Tailwind's emission order -- not by anything
 * either component states.
 *
 * `Card`'s `[&_p]:` in particular cannot be moved to its call sites the way
 * `PageHeading`'s `[&_h2]:` was (T3-1): Card contains far too many
 * paragraphs (all of Diagnostics, every form's field help, every row's
 * summary line) for restyling each individually to be safe or readable --
 * this is the guard `SPEC` §6.1 / T3-1's own instructions name as the
 * alternative to moving utilities when moving is not feasible.
 *
 * Runs in jsdom: this only reads `className` strings and DOM ancestry, both
 * of which jsdom computes correctly (unlike layout/computed style, which it
 * cannot -- see WEBAPP_TAILWIND_SPEC_2026-08-18.md §10.1).
 */

const DESCENDANT_VARIANT_PATTERN = /^\[&_([a-zA-Z0-9]+)\]:/;

export interface DescendantVariantNestingViolation {
  element: Element;
  tag: string;
  scope: string;
  scopingAncestors: Element[];
}

/**
 * Every descendant-variant scope (`[&_p]:`, `[&_h2]:`, ...) present anywhere
 * in `element`'s own class list, deduplicated.
 */
function scopesOwnedBy(element: Element): Set<string> {
  const classAttribute = element.getAttribute("class") ?? "";
  const scopes = new Set<string>();
  for (const token of classAttribute.split(/\s+/)) {
    const match = DESCENDANT_VARIANT_PATTERN.exec(token);
    const scopeTag = match?.[1];
    if (scopeTag !== undefined) {
      scopes.add(scopeTag);
    }
  }
  return scopes;
}

/**
 * Walks every element under `root`, and for each, walks its ancestors
 * looking for more than one that owns a descendant-variant scope matching
 * the element's own tag name. Two or more such ancestors is the violation:
 * both apply to this element, at equal specificity, with no author-visible
 * tiebreak.
 */
export function findDescendantVariantNestingViolations(
  root: ParentNode,
): DescendantVariantNestingViolation[] {
  const violations: DescendantVariantNestingViolation[] = [];
  for (const element of root.querySelectorAll("*")) {
    const tag = element.tagName.toLowerCase();
    const scopingAncestors: Element[] = [];
    let ancestor = element.parentElement;
    while (ancestor !== null) {
      if (scopesOwnedBy(ancestor).has(tag)) {
        scopingAncestors.push(ancestor);
      }
      ancestor = ancestor.parentElement;
    }
    if (scopingAncestors.length > 1) {
      violations.push({ element, tag, scope: `[&_${tag}]:`, scopingAncestors });
    }
  }
  return violations;
}

export function describeViolation(
  violation: DescendantVariantNestingViolation,
): string {
  const ancestorDescriptions = violation.scopingAncestors
    .map(
      (ancestor) =>
        `<${ancestor.tagName.toLowerCase()} class="${ancestor.getAttribute("class") ?? ""}">`,
    )
    .join(" nested inside ");
  return (
    `<${violation.element.tagName.toLowerCase()}> is scoped by ${violation.scope} ` +
    `from ${String(violation.scopingAncestors.length)} ancestors at once: ${ancestorDescriptions}`
  );
}
