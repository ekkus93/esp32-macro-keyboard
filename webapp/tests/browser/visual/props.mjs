/**
 * The computed-style properties captured for every element during a visual
 * regression walk (WEBAPP_TAILWIND_SPEC_2026-08-18.md §10.2).
 *
 * This is a fixed, curated set -- not the full `getComputedStyle()` output.
 * Two reasons: the full set includes hundreds of custom properties whose
 * *enumeration order* shifts whenever a stylesheet rule is added or removed,
 * which reads as a difference when only order changed, not any value; and
 * capturing everything for every element in a large page is materially
 * slower for no extra defect-catching power. This list is everything a
 * Tailwind utility in this codebase can plausibly set -- box model, colour,
 * typography, flex/grid layout, borders -- so a missed property here is a
 * missed utility category, not a missed individual value.
 */
export const CAPTURE_PROPS = [
  "display",
  "position",
  "top",
  "right",
  "bottom",
  "left",
  "margin-top",
  "margin-right",
  "margin-bottom",
  "margin-left",
  "padding-top",
  "padding-right",
  "padding-bottom",
  "padding-left",
  "width",
  "height",
  "min-width",
  "min-height",
  "max-width",
  "max-height",
  "color",
  "background-color",
  "background-image",
  "opacity",
  "visibility",
  "font-size",
  "font-weight",
  "font-family",
  "font-style",
  "line-height",
  "letter-spacing",
  "text-align",
  "text-transform",
  "text-decoration-line",
  "white-space",
  "text-overflow",
  "overflow-x",
  "overflow-y",
  "flex-direction",
  "flex-wrap",
  "flex-grow",
  "flex-shrink",
  "flex-basis",
  "justify-content",
  "align-items",
  "gap",
  "row-gap",
  "column-gap",
  "grid-template-columns",
  "grid-template-rows",
  "place-items",
  "border-top-width",
  "border-right-width",
  "border-bottom-width",
  "border-left-width",
  "border-top-color",
  "border-right-color",
  "border-bottom-color",
  "border-left-color",
  "border-top-left-radius",
  "border-bottom-right-radius",
  "box-shadow",
  "outline-color",
  "outline-width",
  "z-index",
  "transform",
  "transition-property",
  "list-style-type",
];

/**
 * `::before` is a separate capture (`capturePseudoElements`, capture.mjs):
 * `getComputedStyle(element)` never reports pseudo-element styling, and
 * this is exactly where UI_UX_SPEC_V2 §14's "colour is never the only
 * indicator" requirement lives (the StatusBadge dot shapes). A narrower
 * list than CAPTURE_PROPS, scoped to what a `::before` dot can vary.
 */
export const PSEUDO_CAPTURE_PROPS = [
  "content",
  "display",
  "width",
  "height",
  "border-top-left-radius",
  "border-top-right-radius",
  "border-bottom-left-radius",
  "border-bottom-right-radius",
  "background-color",
  "border-top-width",
  "border-right-width",
  "border-bottom-width",
  "border-left-width",
  "border-top-color",
  "border-left-color",
  "box-shadow",
  "color",
  "margin-left",
  "margin-right",
];
