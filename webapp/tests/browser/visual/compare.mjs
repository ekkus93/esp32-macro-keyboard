/**
 * Compares two `captureScenario()` results (compare.mjs pairs with
 * capture.mjs) as sorted key->value maps, per
 * WEBAPP_TAILWIND_SPEC_2026-08-18.md §10.2 -- comparing raw
 * `getComputedStyle()` iteration order reads custom-property *enumeration*
 * shifts (which change when stylesheet rules are added/removed) as value
 * differences, which they are not. Comparing named properties by key sidesteps
 * that entirely; props.mjs's fixed list is what makes this safe.
 *
 * Returns a list of human-readable diff strings; an empty list means the two
 * captures are equivalent. Never throws on a *content* difference -- only on
 * malformed input (a scenario that captured nothing).
 */
export function compareCaptures(baseline, current, label) {
  const diffs = [];

  if (baseline.elements.length !== current.elements.length) {
    diffs.push(
      `${label}: element count ${String(baseline.elements.length)} -> ${String(current.elements.length)}`,
    );
    // A structural mismatch makes positional comparison meaningless past
    // this point -- report it alone rather than a wall of misaligned noise.
    return diffs;
  }

  for (let i = 0; i < baseline.elements.length; i += 1) {
    const before = baseline.elements[i];
    const after = current.elements[i];
    const where = `${label}[${String(i)}] <${before.tag} class=${JSON.stringify(before.cls)}>`;

    if (before.tag !== after.tag) {
      diffs.push(`${where}: tag changed to ${after.tag}`);
      continue;
    }
    if (
      before.rect[0] !== after.rect[0] ||
      before.rect[1] !== after.rect[1] ||
      before.rect[2] !== after.rect[2] ||
      before.rect[3] !== after.rect[3]
    ) {
      diffs.push(
        `${where}: rect ${JSON.stringify(before.rect)} -> ${JSON.stringify(after.rect)}`,
      );
    }
    const keys = new Set([
      ...Object.keys(before.values),
      ...Object.keys(after.values),
    ]);
    for (const key of keys) {
      const beforeValue = before.values[key];
      const afterValue = after.values[key];
      if (beforeValue !== afterValue) {
        diffs.push(
          `${where} ${key}: ${JSON.stringify(beforeValue)} -> ${JSON.stringify(afterValue)}`,
        );
      }
    }
  }

  diffs.push(
    ...comparePseudoElements(
      baseline.pseudoElements,
      current.pseudoElements,
      label,
    ),
  );

  return diffs;
}

function comparePseudoElements(baseline, current, label) {
  const diffs = [];
  if (baseline.length !== current.length) {
    diffs.push(
      `${label}: ::before count ${String(baseline.length)} -> ${String(current.length)}`,
    );
    return diffs;
  }
  for (let i = 0; i < baseline.length; i += 1) {
    const before = baseline[i];
    const after = current[i];
    const where = `${label} ::before[${String(i)}] <${before.tag} class=${JSON.stringify(before.cls)}>`;
    const keys = new Set([
      ...Object.keys(before.values),
      ...Object.keys(after.values),
    ]);
    for (const key of keys) {
      const beforeValue = before.values[key];
      const afterValue = after.values[key];
      if (beforeValue !== afterValue) {
        diffs.push(
          `${where} ${key}: ${JSON.stringify(beforeValue)} -> ${JSON.stringify(afterValue)}`,
        );
      }
    }
  }
  return diffs;
}
