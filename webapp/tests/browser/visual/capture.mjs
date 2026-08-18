import { CAPTURE_PROPS, PSEUDO_CAPTURE_PROPS } from "./props.mjs";

/**
 * Walks every element in the current document, capturing tag, className,
 * a rounded bounding box, and the fixed computed-style property set
 * (props.mjs). This is the primary regression signal
 * (WEBAPP_TAILWIND_SPEC_2026-08-18.md §10.2) -- a full-page screenshot is a
 * useful sanity check but cannot see inside a scroll container or behind a
 * modal overlay, both of which this walk sees directly.
 *
 * Rounds rect coordinates to 2 decimal places: sub-pixel layout jitter
 * between two otherwise-identical runs (font hinting, fractional viewport
 * math) would otherwise read as a difference with no real cause.
 */
export async function captureDocument(page) {
  return page.evaluate((props) => {
    function round(n) {
      return Math.round(n * 100) / 100;
    }
    return Array.from(document.querySelectorAll("*")).map((element) => {
      const style = window.getComputedStyle(element);
      const values = {};
      for (const name of props) {
        values[name] = style.getPropertyValue(name);
      }
      const rect = element.getBoundingClientRect();
      return {
        tag: element.tagName,
        // getAttribute, not element.className: on an SVG element
        // `className` is an SVGAnimatedString, not a string, and
        // String(element.className) on one produces the literal text
        // "[object SVGAnimatedString]" -- found by T2-3's token collector,
        // which choked on exactly that text as an unrecognized class.
        // getAttribute("class") returns the real attribute text (or null,
        // hence ?? "") for both HTML and SVG elements uniformly.
        cls: element.getAttribute("class") ?? "",
        rect: [
          round(rect.x),
          round(rect.y),
          round(rect.width),
          round(rect.height),
        ],
        values,
      };
    });
  }, CAPTURE_PROPS);
}

/**
 * Captures `::before` computed style for every element that has one with
 * non-`none` content -- see props.mjs for why this needs its own pass.
 */
export async function capturePseudoElements(page) {
  return page.evaluate((props) => {
    const results = [];
    for (const element of document.querySelectorAll("*")) {
      const style = window.getComputedStyle(element, "::before");
      if (style.getPropertyValue("content") === "none") {
        continue;
      }
      const values = {};
      for (const name of props) {
        values[name] = style.getPropertyValue(name);
      }
      results.push({
        tag: element.tagName,
        // getAttribute, not element.className: on an SVG element
        // `className` is an SVGAnimatedString, not a string, and
        // String(element.className) on one produces the literal text
        // "[object SVGAnimatedString]" -- found by T2-3's token collector,
        // which choked on exactly that text as an unrecognized class.
        // getAttribute("class") returns the real attribute text (or null,
        // hence ?? "") for both HTML and SVG elements uniformly.
        cls: element.getAttribute("class") ?? "",
        values,
      });
    }
    return results;
  }, PSEUDO_CAPTURE_PROPS);
}

/**
 * One scenario's full capture: the element walk, the pseudo-element walk,
 * and (unless `screenshot: false`) a full-page PNG. Screenshots are kept
 * out of the committed baseline (see baselines/README.md) but are still
 * captured here so a failing run can write them to a scratch directory for
 * human inspection -- see run-visual-tests.mjs's `--diff-dir`.
 */
export async function captureScenario(page, { screenshot = true } = {}) {
  const elements = await captureDocument(page);
  const pseudoElements = await capturePseudoElements(page);
  const png = screenshot ? await page.screenshot({ fullPage: true }) : null;
  return { elements, pseudoElements, png };
}
