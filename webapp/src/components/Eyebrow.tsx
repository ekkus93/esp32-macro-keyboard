/**
 * The keycap legend above a title: small, tight-tracked, uppercase, and
 * truncated rather than wrapped, since what it carries — a device name, a
 * package name — is arbitrary-length user text. Replaces `.eyebrow` and
 * `.eyebrow.dark`.
 *
 * The two tones are **complete literal class strings** (§8.2). They differ
 * only in `color`, and as a class pair that resolved by specificity —
 * `.eyebrow.dark` is `(0,2,0)` against `.eyebrow`'s `(0,1,0)`. Two colour
 * utilities on one element have no such tiebreak; the winner would be
 * whichever Tailwind happened to emit later.
 *
 * `tone="dark"` is the name the old class used, and it means "sits on the
 * light page surface" — the default tone is the one that sits on the dark
 * shell header.
 */
const EYEBROW_CLASS = {
  default:
    "m-0 overflow-hidden text-ellipsis whitespace-nowrap text-[0.65rem] font-bold uppercase tracking-legend text-lamp",
  dark: "m-0 overflow-hidden text-ellipsis whitespace-nowrap text-[0.65rem] font-bold uppercase tracking-legend text-actuate",
} as const;

export type EyebrowTone = keyof typeof EYEBROW_CLASS;

export interface EyebrowProps {
  children: React.ReactNode;
  tone?: EyebrowTone;
}

export function Eyebrow({
  children,
  tone = "default",
}: EyebrowProps): React.JSX.Element {
  return <p className={EYEBROW_CLASS[tone]}>{children}</p>;
}
