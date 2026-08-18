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
 * ## The four-sided padding is load-bearing, and it has to win
 *
 * `pt-…7vh…` is the generous top margin a single-task screen gets, and the
 * three `env(safe-area-inset-*)` terms are `UI_UX_SPEC_V2` §13 ("Safe-area
 * insets are respected on devices with display cutouts or gesture
 * navigation"). Both only work if this padding beats the `main` rule's
 * `px-4 py-5`, which it does here because utilities outrank the components
 * layer.
 *
 * That ordering is worth stating because it was got wrong once. In the
 * original stylesheet the `.standalone` rule carrying this padding sat
 * *after* `.standalone, main`, so it won on source order. When the shell
 * chrome was inlined (T3-1) the two `.standalone` rules were merged into one
 * placed *before* that shared rule, which silently flipped the cascade and
 * dropped the top padding from ~59-63px to 20px on every single-task screen.
 * Nothing caught it until the migration's final full-page diff, because no
 * per-task check had rendered a standalone screen since. Keep these four
 * utilities on this element; do not move them into a class.
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
  "mx-auto flex min-h-dvh w-[min(100%,48rem)] flex-col bg-shell " +
  "pt-[calc(max(2rem,7vh)+env(safe-area-inset-top))] " +
  "pr-[calc(1rem+env(safe-area-inset-right))] " +
  "pb-[calc(1.25rem+env(safe-area-inset-bottom))] " +
  "pl-[calc(1rem+env(safe-area-inset-left))] " +
  "min-[60rem]:w-[min(100%,64rem)] *:mx-auto *:w-[min(100%,27rem)]";

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
