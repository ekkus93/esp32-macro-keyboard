/**
 * Detects two Tailwind utility classes in one `className` string that set
 * the same CSS property in the same rendering context (WEBAPP_TAILWIND_SPEC
 * §3, rule 4: never put two such utilities on one element, because the
 * winner is decided by Tailwind's own stylesheet emission order, not by
 * anything either the code or this document states).
 *
 * This is deliberately NOT a general Tailwind class parser. It recognizes
 * exactly the utility vocabulary this codebase uses (built from
 * `webapp/src/**\/*.tsx`'s actual class tokens -- see
 * component-variant-maps.test.tsx for the six SPEC §4.2 variant maps this
 * was built to cover, and any-rendered-page.test.tsx for the whole-app
 * sweep, T2-2), and any future class shape this module does not recognize
 * will fail loudly (`classifyToken` throws) rather than silently passing an
 * unaudited token through. Extend the tables below when that happens; do
 * not add a catch-all "unknown -> ignore" branch.
 *
 * ## Scopes
 *
 * A "scope" is the variant prefix chain in front of the base utility --
 * `before:`, `hover:`, `short:`, a descendant selector like `[&_p]:`, a
 * breakpoint like `min-[34rem]:`, or "" for a bare utility with no prefix.
 * Two utilities in DIFFERENT scopes style different things (a pseudo-element
 * vs the element itself, one media-query range vs another, a hover state vs
 * the resting state) and can never collide, however similar their base
 * utility looks. Only same-scope, same-property pairs are collisions.
 *
 * ## Some utilities claim more than one property slot
 *
 * `border` (bare) sets `border-width` on all four sides at once; `border-l-
 * [3px]` sets only `border-left-width`. Those two DO collide on the left
 * side even though neither name looks like the other -- this module found a
 * real instance of exactly that (fixed in 1b4fda5). So border width,
 * border colour, padding, margin, and position offsets are each modelled as
 * four independent per-side slots (or two for an axis, like `mx-`/`px-`/
 * `inset-x-`), and `place-items-*` claims both `align-items` and
 * `justify-items`. A token claims whichever slots its side/axis suffix (or,
 * for `place-items`, its shorthand nature) implies.
 *
 * ## Two non-utility classes are recognized as claiming nothing
 *
 * `.primary` and `.danger` are real CSS rules, but they live in `@layer
 * base` -- SPEC §3 rule 1 means ANY utility in markup beats them
 * deterministically by layer order, not by the fragile same-layer emission
 * order rule 4 is about. A bare test hook (`app-shell`, `landscape-block`,
 * `storage-summary`, SPEC §7) carries no CSS rule at all. Both kinds are
 * listed in `NON_UTILITY_CLASSES` and claim no property slots -- they are
 * not silently ignored as "unrecognized"; they are recognized as safe.
 */

const SCOPE_PATTERNS = [
  /^before:/,
  /^hover:/,
  /^short:/,
  /^\[&_[a-zA-Z0-9]+\]:/,
  /^min-\[[a-zA-Z0-9.]+\]:/,
  /^max-\[[a-zA-Z0-9.]+\]:/,
  /^\[@media\([^)]*\)\]:/,
];

/** Claims no property slots -- see this module's header comment. */
const NON_UTILITY_CLASSES = new Set([
  "primary",
  "danger",
  "app-shell",
  "landscape-block",
  "storage-summary",
]);

function splitScope(token: string): { scope: string; base: string } {
  for (const pattern of SCOPE_PATTERNS) {
    const match = pattern.exec(token);
    if (match !== null) {
      return { scope: match[0], base: token.slice(match[0].length) };
    }
  }
  return { scope: "", base: token };
}

const FOUR_SIDES = ["top", "right", "bottom", "left"] as const;
type Side = (typeof FOUR_SIDES)[number];

const AXIS_SIDES: Record<string, readonly Side[]> = {
  t: ["top"],
  r: ["right"],
  b: ["bottom"],
  l: ["left"],
  x: ["left", "right"],
  y: ["top", "bottom"],
};

function sidesForAxisLetter(letter: string): readonly Side[] | undefined {
  return letter in AXIS_SIDES ? AXIS_SIDES[letter] : undefined;
}

function isNumericValue(value: string): boolean {
  return /^(\[[0-9.]+(px|rem)\]|[0-9]+)$/.test(value);
}

/**
 * `border[-side][-value]`: bare `border`/`border-2` is a width shorthand for
 * all four sides; `border-<colorname>`/`border-current` is a colour
 * shorthand for all four sides; a side/axis suffix narrows to those sides,
 * and whether the trailing value is numeric decides width vs colour.
 */
function classifyBorderToken(base: string): string[] {
  if (base === "border") {
    return FOUR_SIDES.map((side) => `border-${side}-width`);
  }

  // Side/axis-prefixed: `border-t`, `border-l-[3px]`, `border-y-cap-edge`, ...
  const axisMatch = /^border-([trblxy])(?:-(\S+))?$/.exec(base);
  if (axisMatch !== null) {
    const axis = axisMatch[1];
    const value = axisMatch[2];
    const sides = axis === undefined ? undefined : sidesForAxisLetter(axis);
    if (sides === undefined) {
      throw new Error(`Unrecognized border utility: "${base}"`);
    }
    const kind =
      value === undefined || isNumericValue(value) ? "width" : "color";
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
  const kind = isNumericValue(bareValue) ? "width" : "color";
  return FOUR_SIDES.map((side) => `border-${side}-${kind}`);
}

/**
 * `p-`/`m-` with an optional single-letter side/axis: `p-4` (all sides),
 * `px-4`/`py-4` (an axis), `pt-3`/`pr-12`/`pb-2`/`pl-4` (one side). Unlike
 * border there is only one "kind" (the spacing value), so every match
 * claims the same property name per side.
 */
function classifySpacingToken(
  cssProperty: "padding" | "margin",
  base: string,
): string[] {
  const prefix = cssProperty === "padding" ? "p" : "m";
  const match = new RegExp(`^${prefix}([trblxy])?-(\\S+)$`).exec(base);
  const axisLetter = match?.[1];
  if (match === null) {
    throw new Error(`Unrecognized ${cssProperty} utility: "${base}"`);
  }
  const sides =
    axisLetter === undefined ? FOUR_SIDES : sidesForAxisLetter(axisLetter);
  if (sides === undefined) {
    throw new Error(`Unrecognized ${cssProperty} utility: "${base}"`);
  }
  return sides.map((side) => `${cssProperty}-${side}`);
}

/**
 * `inset-*`/`inset-x-*`/`inset-y-*`/`top-*`/`right-*`/`bottom-*`/`left-*` --
 * the position OFFSET properties, not the `position` property itself
 * (`absolute`/`fixed`/`relative`/`static`/`sticky`, handled separately as
 * its own single-property family below).
 */
function classifyPositionOffsetToken(base: string): string[] {
  if (/^inset-x-\S+$/.test(base)) {
    return ["position-left", "position-right"];
  }
  if (/^inset-y-\S+$/.test(base)) {
    return ["position-top", "position-bottom"];
  }
  if (/^inset-\S+$/.test(base)) {
    return FOUR_SIDES.map((side) => `position-${side}`);
  }
  const sideMatch = /^(top|right|bottom|left)-\S+$/.exec(base);
  if (sideMatch?.[1] !== undefined) {
    return [`position-${sideMatch[1]}`];
  }
  throw new Error(`Unrecognized position-offset utility: "${base}"`);
}

/**
 * Returns the property-family key(s) `base` occupies. Almost every utility
 * occupies exactly one; border, padding, margin, position-offset, and
 * `place-items-*` occupy several (see this module's header comment).
 */
function classifyToken(base: string): string[] {
  if (NON_UTILITY_CLASSES.has(base)) {
    return [];
  }
  if (base.startsWith("border")) {
    return classifyBorderToken(base);
  }
  if (/^p[trblxy]?-\S+$/.test(base)) {
    return classifySpacingToken("padding", base);
  }
  if (/^m[trblxy]?-\S+$/.test(base)) {
    return classifySpacingToken("margin", base);
  }
  if (/^place-items-\S+$/.test(base)) {
    return ["align-items", "justify-items"];
  }
  // `gap-x-*`/`gap-y-*` are one axis each (column-gap/row-gap
  // respectively); bare `gap`/`gap-<value>` is the shorthand for both --
  // NOT one property, despite looking like a single utility. Collapsing
  // these into one "gap" family would have made `gap-x-4 gap-y-3` (two
  // genuinely independent axes) a false-positive collision.
  if (/^gap-x-\S+$/.test(base)) {
    return ["column-gap"];
  }
  if (/^gap-y-\S+$/.test(base)) {
    return ["row-gap"];
  }
  if (/^gap(-\S+)?$/.test(base)) {
    return ["column-gap", "row-gap"];
  }
  if (/^(inset|top|right|bottom|left)-\S+$/.test(base)) {
    return classifyPositionOffsetToken(base);
  }
  // A fully arbitrary CSS-property utility, `[property-name:value]` --
  // covers `[flex:0_0_auto]`, `[grid-row:1]`,
  // `[grid-template-columns:repeat(4,auto)]` and any future one of the same
  // shape without a hand-written entry per property name.
  const arbitraryPropertyMatch = /^\[([a-z-]+):.+\]$/.exec(base);
  if (arbitraryPropertyMatch?.[1] !== undefined) {
    return [arbitraryPropertyMatch[1]];
  }

  const singleFamily: [RegExp, string][] = [
    [/^(grid|inline-flex|flex|block)$/, "display"],
    [/^(absolute|fixed|relative|static|sticky)$/, "position"],
    [/^items-\S+$/, "align-items"],
    [/^content-(start|center|end|between|around|evenly)$/, "align-content"],
    [/^justify-items-\S+$/, "justify-items"],
    [/^justify-\S+$/, "justify-content"],
    [/^flex-(row|col)(-reverse)?$/, "flex-direction"],
    [/^flex-(wrap|nowrap|wrap-reverse)$/, "flex-wrap"],
    [/^flex-(1|auto|initial|none)$/, "flex"],
    [/^grid-cols-\S+$/, "grid-template-columns"],
    [/^rounded(-\S+)?$/, "border-radius"],
    [/^w-\S+$/, "width"],
    [/^h-\S+$/, "height"],
    [/^min-w-\S+$/, "min-width"],
    [/^min-h-\S+$/, "min-height"],
    [/^max-w-\S+$/, "max-width"],
    [/^max-h-\S+$/, "max-height"],
    [/^overflow-y-\S+$/, "overflow-y"],
    [/^overflow-hidden$/, "overflow"],
    [/^overscroll-y-\S+$/, "overscroll-behavior-y"],
    [/^bg-\S+$/, "background-color"],
    [/^text-\[[0-9.]+rem\]$/, "font-size"],
    [/^text-(?!ellipsis$)\S+$/, "color"],
    [/^text-ellipsis$/, "text-overflow"],
    [/^whitespace-\S+$/, "white-space"],
    [
      /^font-(thin|light|normal|medium|semibold|bold|extrabold|black)$/,
      "font-weight",
    ],
    [/^font-(sans|mono)$/, "font-family"],
    [/^uppercase$/, "text-transform"],
    [/^tracking-\S+$/, "letter-spacing"],
    [/^shadow(-\S+)?$/, "box-shadow"],
    [/^content-\[.*\]$/, "content"],
    [/^z-\S+$/, "z-index"],
    [/^list-\S+$/, "list-style-type"],
    [/^-?translate-[xy]-\S+$/, "transform"],
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
