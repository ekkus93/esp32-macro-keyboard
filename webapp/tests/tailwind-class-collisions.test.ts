import { describe, expect, test } from "vitest";

import { findPropertyCollisions } from "./tailwindClassCollisions";

describe("findPropertyCollisions", () => {
  test("an empty or single-class string has no collisions", () => {
    expect(findPropertyCollisions("")).toEqual([]);
    expect(findPropertyCollisions("bg-panel")).toEqual([]);
  });

  test("two classes setting different properties do not collide", () => {
    expect(
      findPropertyCollisions("bg-panel text-legend rounded-keycap"),
    ).toEqual([]);
  });

  test("two classes setting the same property collide", () => {
    expect(findPropertyCollisions("bg-good-tint bg-bad-tint")).toEqual([
      "background-color: bg-good-tint, bg-bad-tint",
    ]);
  });

  test("the same scoped property in different scopes does not collide", () => {
    // before:bg-current styles the pseudo-element; bg-panel styles the
    // element itself -- different boxes, so no collision even though both
    // are "background-color".
    expect(findPropertyCollisions("bg-panel before:bg-current")).toEqual([]);
    // Two different descendant selectors are likewise independent scopes.
    expect(findPropertyCollisions("[&_h2]:mb-[0.3rem] [&_p]:mb-2")).toEqual([]);
  });

  test("the same descendant selector colliding on the same property is caught", () => {
    expect(findPropertyCollisions("[&_p]:mb-2 [&_p]:mb-4")).toEqual([
      "[&_p]:margin-bottom: [&_p]:mb-2, [&_p]:mb-4",
    ]);
  });

  describe("border width and colour are per-side", () => {
    test("a bare border-width shorthand collides with a single-side override", () => {
      // The real defect fixed in 1b4fda5: `border` sets border-left-width
      // (among all four sides) and `border-l-[3px]` sets it too.
      expect(findPropertyCollisions("border border-l-[3px]")).toEqual([
        "border-left-width: border, border-l-[3px]",
      ]);
    });

    test("axis and single-side width utilities on disjoint sides do not collide", () => {
      // The fix: cover all four sides with non-overlapping utilities.
      expect(
        findPropertyCollisions("border-y border-r border-l-[3px]"),
      ).toEqual([]);
    });

    test("width and colour on the same side are independent properties", () => {
      expect(findPropertyCollisions("border-l-[3px] border-l-actuate")).toEqual(
        [],
      );
    });

    test("a bare border-colour shorthand collides with a single-side colour override", () => {
      expect(
        findPropertyCollisions("border-cap-edge border-l-actuate"),
      ).toEqual(["border-left-color: border-cap-edge, border-l-actuate"]);
    });

    test("a multi-segment theme colour name is one value, not a side prefix", () => {
      // border-cap-edge must not be parsed as side "cap" + value "edge".
      expect(findPropertyCollisions("border-cap-edge")).toEqual([]);
      expect(findPropertyCollisions("border-header-button-edge")).toEqual([]);
    });

    test("a numeric bare border sets width, not colour", () => {
      expect(findPropertyCollisions("border-2 border-current")).toEqual([]);
      expect(findPropertyCollisions("border-2 border")).toEqual([
        "border-top-width: border-2, border",
        "border-right-width: border-2, border",
        "border-bottom-width: border-2, border",
        "border-left-width: border-2, border",
      ]);
    });
  });

  describe("gap-x and gap-y are independent axes", () => {
    test("gap-x and gap-y on the same element do not collide", () => {
      // The real false positive T2-2's page-level sweep found: PageHeading's
      // `gap-x-4 gap-y-3` was flagged as colliding on a single "gap" family
      // before this was split into column-gap/row-gap. column-gap and
      // row-gap are genuinely independent CSS properties.
      expect(findPropertyCollisions("gap-x-4 gap-y-3")).toEqual([]);
    });

    test("the bare gap shorthand still collides with either axis", () => {
      expect(findPropertyCollisions("gap-4 gap-x-2")).toEqual([
        "column-gap: gap-4, gap-x-2",
      ]);
      expect(findPropertyCollisions("gap-4 gap-y-2")).toEqual([
        "row-gap: gap-4, gap-y-2",
      ]);
    });
  });

  test("an unrecognized utility throws rather than passing silently", () => {
    // §4.1/§4.2's whole point is that a wrong or unaudited class must never
    // pass quietly -- the classifier enforces the same discipline on itself.
    expect(() => findPropertyCollisions("this-is-not-a-real-utility")).toThrow(
      /does not recognize the utility/,
    );
  });
});
