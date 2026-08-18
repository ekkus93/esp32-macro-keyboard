/**
 * The banner that reports what a send is doing — starting, running, awaiting
 * the device's physical confirmation, finished, or failed. Replaces
 * `.send-status`, `.send-status p` and `.send-status[role="alert"]`.
 *
 * `role` picks a **complete literal class string** rather than layering an
 * override (§8.2). It has to: `DismissibleBanner` passes the role through as
 * a runtime value, so `` `send-status-${role}` `` would compile to nothing at
 * all, and the two branches disagree on both border colour and background.
 *
 * `overlay` is different — it *appends*. Its two utilities are a width and a
 * text colour, and the base string sets neither, so there is no property for
 * them to race over. That is the same reasoning the landscape surface's
 * inline version carried before this component existed.
 */
const SEND_STATUS_CLASS = {
  status:
    "my-3 rounded-keycap border-l-[3px] border-actuate bg-send-tint px-4 py-[0.8rem] [&_p]:mb-2 [&_p]:font-bold",
  alert:
    "my-3 rounded-keycap border-l-[3px] border-alert bg-bad-tint px-4 py-[0.8rem] [&_p]:mb-2 [&_p]:font-bold",
} as const;

/**
 * The landscape orientation surface is the one place this banner is not in
 * page flow: it sits on a full-viewport overlay, so it gets a measured width
 * and the inverted ink that reads on that surface.
 */
const SEND_STATUS_OVERLAY_CLASS = "w-[min(100%,24rem)] text-legend";

export interface SendStatusProps {
  children: React.ReactNode;
  overlay?: boolean;
  role: "status" | "alert";
}

export function SendStatus({
  children,
  overlay = false,
  role,
}: SendStatusProps): React.JSX.Element {
  const className = overlay
    ? `${SEND_STATUS_CLASS[role]} ${SEND_STATUS_OVERLAY_CLASS}`
    : SEND_STATUS_CLASS[role];
  return (
    <div aria-live="polite" className={className} role={role}>
      {children}
    </div>
  );
}
