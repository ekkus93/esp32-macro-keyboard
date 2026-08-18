/**
 * The full-page surface every single-task screen sits on — Sign In,
 * First-Run Setup, the reconnect screens, and every repository-startup
 * screen. Replaces the `.standalone` rule.
 *
 * Always a `<main>`, so it keeps matching the bare `main` rule that still
 * lives in `styles.css` (§5.3 — it is shared with the shell's own `<main>`
 * and styles unclassed elements). That rule, not this component, supplies
 * `px-4 py-5 [flex:1_1_auto]`.
 *
 * ## The safe-area padding `.standalone` used to carry is deliberately gone
 *
 * `.standalone` declared a four-sided
 * `calc(… + env(safe-area-inset-*))` padding, but the `.standalone, main`
 * rule that followed it in `styles.css` set `padding-inline` / `padding-block`
 * at equal specificity and therefore overrode **all four sides**. Measured
 * before this change, at 390×844: computed padding was `20px 16px`, i.e.
 * `py-5 px-4` — not the `max(2rem, 7vh)` ≈ `59px` the declaration asks for.
 * It had no effect on any rendered pixel.
 *
 * Carrying it here as utilities would have *revived* it (utilities beat the
 * components layer), which is a rendering change this refactor is not allowed
 * to make. So it is dropped, and the pre-existing gap it leaves —
 * `UI_UX_SPEC_V2` §13 "Safe-area insets are respected on devices with display
 * cutouts or gesture navigation" is not actually satisfied on these screens —
 * is recorded in the migration ledger for a separate, deliberate fix rather
 * than smuggled in as a side effect of a refactor.
 *
 * The `min-[60rem]:` width step and the `*:` child rule are the two other
 * halves of the old CSS: single-task screens get a measured 27rem column
 * inside a page that is itself capped at 48rem (64rem on wide viewports).
 *
 * `min-h-dvh`, not `min-h-screen`: `vh` is the static, largest-possible
 * viewport, `dvh` the current one net of the mobile browser's address bar. A
 * `vh` floor on a screen shorter than that static measurement left a phantom
 * gap outside the visible viewport that the address bar opened and closed on
 * scroll — read as the page sliding a little. Content that genuinely needs
 * more room than the viewport still grows past this floor.
 */
const STANDALONE_CLASS =
  "mx-auto flex min-h-dvh w-[min(100%,48rem)] flex-col bg-shell min-[60rem]:w-[min(100%,64rem)] *:mx-auto *:w-[min(100%,27rem)]";

export interface StandaloneScreenProps {
  "aria-busy"?: React.AriaAttributes["aria-busy"];
  children: React.ReactNode;
}

export function StandaloneScreen({
  "aria-busy": ariaBusy,
  children,
}: StandaloneScreenProps): React.JSX.Element {
  return (
    <main aria-busy={ariaBusy} className={STANDALONE_CLASS}>
      {children}
    </main>
  );
}
