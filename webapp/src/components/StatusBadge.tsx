/**
 * A pill reporting the state of something the user cannot see directly —
 * today the device's USB link and the working copy's saved/unsaved state.
 *
 * ## Every state has its own shape, not just its own colour
 *
 * Deliberately NOT uppercased, unlike the key legends elsewhere in the
 * interface. These carry sentences ("Unsaved changes"), not labels, and
 * `text-transform` rewrites `innerText` — which silently broke a real-browser
 * assertion matching "Unsaved changes". Legends are uppercase because they
 * label a key; status text is prose.
 *
 * `UI_UX_SPEC_V2` §14 requires that colour is never the only indicator, so
 * the `::before` dot differs structurally in all four states: a filled disc
 * with a halo (good), a hollow ring (warning), a square (bad), and a smaller
 * hollow dot (neutral). Those four treatments are load-bearing accessibility,
 * not decoration — do not collapse them into a colour swap.
 *
 * ## Why each state is one complete literal string
 *
 * This component used to build its class name as `` `status-${state}` ``,
 * which Tailwind's source scanner cannot see at all: expressed that way the
 * four state rules would simply never be emitted, and the badge would render
 * unstyled with no build error (§8.2). Beyond that, the per-state `::before`
 * treatments *override* the base one rather than adding to it — `warning`
 * replaces `bg-current` with `bg-transparent`, `bad` replaces `rounded-full`
 * with `rounded-[1px]`, `neutral` replaces both the size and the fill. Two
 * conflicting utilities on one element are resolved by Tailwind's own
 * stylesheet order, which is not something either this file or the design
 * states. So each entry below spells out the winning declarations only.
 */
const STATUS_BADGE_SHAPE =
  "inline-flex items-center gap-[0.4rem] rounded-full border border-current px-[0.6rem] py-[0.3rem] text-[0.76rem] font-bold tracking-[0.02em] before:content-['']";

const STATUS_BADGE_CLASS = {
  /* Lit: filled disc with a halo. */
  good: `${STATUS_BADGE_SHAPE} bg-good-tint text-good before:h-[0.6rem] before:w-[0.6rem] before:rounded-full before:bg-current before:shadow-[0_0_0_3px_rgb(11_92_51_/_22%)]`,
  /* Attention: hollow ring. */
  warning: `${STATUS_BADGE_SHAPE} bg-warning-tint text-warning-ink before:h-[0.6rem] before:w-[0.6rem] before:rounded-full before:border-2 before:border-current before:bg-transparent`,
  /* Fault: square, not a disc. */
  bad: `${STATUS_BADGE_SHAPE} bg-bad-tint text-alert before:h-[0.6rem] before:w-[0.6rem] before:rounded-[1px] before:bg-current`,
  /* Idle: small hollow dot. */
  neutral: `${STATUS_BADGE_SHAPE} bg-neutral-tint text-legend-soft before:h-[0.45rem] before:w-[0.45rem] before:rounded-full before:border-[1.5px] before:border-current before:bg-transparent`,
} as const;

export type StatusBadgeState = keyof typeof STATUS_BADGE_CLASS;

export interface StatusBadgeProps {
  "aria-live"?: "polite";
  label: string;
  role?: "status";
  state: StatusBadgeState;
}

export function StatusBadge({
  "aria-live": ariaLive,
  label,
  role,
  state,
}: StatusBadgeProps): React.JSX.Element {
  return (
    <span
      aria-live={ariaLive}
      className={STATUS_BADGE_CLASS[state]}
      role={role}
    >
      {label}
    </span>
  );
}
