/**
 * Detects two Tailwind utility classes in one `className` string that set
 * the same CSS property in the same rendering context (WEBAPP_TAILWIND_SPEC
 * §3, rule 4: never put two such utilities on one element, because the
 * winner is decided by Tailwind's own stylesheet emission order, not by
 * anything either the code or this document states).
 *
 * This is deliberately NOT a general Tailwind class parser. It recognizes
 * exactly the utility vocabulary the six variant maps in
 * `src/components/*.tsx` use (WEBAPP_TAILWIND_SPEC §4.2) -- component-variant-maps.test.tsx
 * is what exercises it, and any future variant that adds a class shape this
 * module does not recognize will fail loudly (`classifyToken` throws) rather
 * than silently passing an unaudited token through. Extend the tables below
 * when that happens; do not add a catch-all "unknown -> ignore" branch.
 *
 * ## Scopes
 *
 * A "scope" is the variant prefix chain in front of the base utility --
 * `before:`, a descendant selector like `[&_p]:`, a breakpoint like
 * `min-[34rem]:`, or "" for a bare utility with no prefix. Two utilities in
 * DIFFERENT scopes style different things (a pseudo-element vs the element
 * itself, one media-query range vs another) and can never collide, however
 * similar their base utility looks. Only same-scope, same-property pairs are
 * collisions.
 *
 * ## Border width/colour are per-side, not one property each
 *
 * `border` (bare) sets `border-width` on all four sides at once; `border-l-
 * [3px]` sets only `border-left-width`. Those two DO collide on the left
 * side even though neither name looks like the other -- this module found a
 * real instance of exactly that (fixed in 1b4fda5). So border width and
 * border colour are each modelled as four independent slots
 * (top/right/bottom/left), and a token claims whichever slots its side/axis
 * suffix implies.
 */

const SCOPE_PATTERNS = [
  /^before:/,
  /^\[&_[a-zA-Z0-9]+\]:/,
  /^min-\[[a-zA-Z0-9.]+\]:/,
  /^max-\[[a-zA-Z0-9.]+\]:/,
  /^\[@media\([^)]*\)\]:/,
];

function splitScope(token: string): { scope: string; base: string } {
  for (const pattern of SCOPE_PATTERNS) {
    const match = pattern.exec(token);
    if (match !== null) {
      return { scope: match[0], base: token.slice(match[0].length) };
    }
  }
  return { scope: "", base: token };
}

const BORDER_SIDES = ["top", "right", "bottom", "left"] as const;
type BorderSide = (typeof BORDER_SIDES)[number];

const AXIS_SIDES: Record<string, readonly BorderSide[]> = {
  t: ["top"],
  r: ["right"],
  b: ["bottom"],
  l: ["left"],
  x: ["left", "right"],
  y: ["top", "bottom"],
};

/**
 * `border[-side][-value]`: bare `border`/`border-2` is a width shorthand for
 * all four sides; `border-<colorname>`/`border-current` is a colour
 * shorthand for all four sides; a side/axis suffix narrows to those sides,
 * and whether the trailing value is numeric decides width vs colour.
 */
function isNumericBorderValue(value: string): boolean {
  return /^(\[[0-9.]+(px|rem)\]|[0-9]+)$/.test(value);
}

function classifyBorderToken(base: string): string[] {
  if (base === "border") {
    return BORDER_SIDES.map((side) => `border-${side}-width`);
  }

  // Side/axis-prefixed: `border-t`, `border-l-[3px]`, `border-y-cap-edge`, ...
  // Checked before the bare-colour branch because a side/axis letter (t/r/
  // b/l/x/y) is itself a valid single-character prefix, and `AXIS_SIDES`'s
  // keys are exactly the only single-letter segments this vocabulary uses.
  const axisMatch = /^border-([trblxy])(?:-(\S+))?$/.exec(base);
  if (axisMatch !== null) {
    const axis = axisMatch[1];
    const value = axisMatch[2];
    const sides = axis === undefined ? undefined : AXIS_SIDES[axis];
    if (sides === undefined) {
      throw new Error(`Unrecognized border utility: "${base}"`);
    }
    const kind =
      value === undefined || isNumericBorderValue(value) ? "width" : "color";
    return sides.map((side) => `border-${side}-${kind}`);
  }

  // Bare, no side/axis: `border-2` (width, all sides) or `border-cap-edge` /
  // `border-current` (colour, all sides) -- the theme's colour tokens can be
  // multi-segment (`cap-edge`, `header-button-edge`), so the whole remainder
  // after `border-` is one value, not split into further segments.
  const bareMatch = /^border-(\S+)$/.exec(base);
  const bareValue = bareMatch?.[1];
  if (bareValue === undefined) {
    throw new Error(`Unrecognized border utility: "${base}"`);
  }
  const kind = isNumericBorderValue(bareValue) ? "width" : "color";
  return BORDER_SIDES.map((side) => `border-${side}-${kind}`);
}

/**
 * Returns the property-family key(s) `base` occupies. Almost every utility
 * occupies exactly one; only border width/colour occupy several (one per
 * side, see above).
 */
function classifyToken(base: string): string[] {
  if (base.startsWith("border")) {
    return classifyBorderToken(base);
  }
  const singleFamily: [RegExp, string][] = [
    [/^(grid|inline-flex|block)$/, "display"],
    [/^items-\S+$/, "align-items"],
    [/^gap(-\S+)?$/, "gap"],
    [/^\[grid-template-columns:\S+\]$/, "grid-template-columns"],
    [/^rounded(-\S+)?$/, "border-radius"],
    [/^p-\S+$/, "padding-all"],
    [/^px-\S+$/, "padding-x"],
    [/^py-\S+$/, "padding-y"],
    [/^m-\S+$/, "margin-all"],
    [/^my-\S+$/, "margin-y"],
    [/^mt-\S+$/, "margin-top"],
    [/^mb-\S+$/, "margin-bottom"],
    [/^w-\S+$/, "width"],
    [/^h-\S+$/, "height"],
    [/^max-h-\S+$/, "max-height"],
    [/^overflow-y-\S+$/, "overflow-y"],
    [/^overflow-hidden$/, "overflow"],
    [/^bg-\S+$/, "background-color"],
    [/^text-\[[0-9.]+rem\]$/, "font-size"],
    [/^text-(?!ellipsis$)\S+$/, "color"],
    [/^text-ellipsis$/, "text-overflow"],
    [/^whitespace-\S+$/, "white-space"],
    [
      /^font-(thin|light|normal|medium|semibold|bold|extrabold|black)$/,
      "font-weight",
    ],
    [/^uppercase$/, "text-transform"],
    [/^tracking-\S+$/, "letter-spacing"],
    [/^shadow(-\S+)?$/, "box-shadow"],
    [/^content-\[.*\]$/, "content"],
  ];
  for (const [pattern, family] of singleFamily) {
    if (pattern.test(base)) {
      return [family];
    }
  }
  throw new Error(
    `tailwindClassCollisions.ts does not recognize the utility "${base}" -- ` +
      "add it to classifyToken's table (see this module's header comment).",
  );
}

/**
 * Finds every pair of classes in `classNameString` that set the same CSS
 * property in the same scope. Returns a human-readable description per
 * collision, or an empty array if there are none.
 */
export function findPropertyCollisions(classNameString: string): string[] {
  const tokens = classNameString.trim().split(/\s+/).filter(Boolean);
  const claims = new Map<string, string[]>();
  for (const token of tokens) {
    const { scope, base } = splitScope(token);
    for (const family of classifyToken(base)) {
      const key = `${scope}${family}`;
      const owners = claims.get(key) ?? [];
      owners.push(token);
      claims.set(key, owners);
    }
  }
  const collisions: string[] = [];
  for (const [key, owners] of claims) {
    if (owners.length > 1) {
      collisions.push(`${key}: ${owners.join(", ")}`);
    }
  }
  return collisions;
}
