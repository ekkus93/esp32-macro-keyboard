/**
 * A tinted panel with a left accent rule, in the alert palette: "this
 * permanently deletes something", as distinct from the lamp palette's "this
 * needs your attention". Replaces `.danger-zone` at its four standalone call
 * sites; the fifth stacked it on `.dialog-panel` and is handled by
 * `<Dialog tone="danger">` instead.
 *
 * `role` and the `tabIndex={-1}` that goes with it are one decision, not two:
 * `useFocusTrap` moves focus to the container, which a `<div>` can only
 * accept when it is programmatically focusable. All three call sites that
 * announce themselves as an `alertdialog` need both, and none needs one
 * without the other.
 */
export interface DangerZoneProps {
  children: React.ReactNode;
  containerRef?: React.RefObject<HTMLDivElement | null>;
  role?: "alertdialog";
}

export function DangerZone({
  children,
  containerRef,
  role,
}: DangerZoneProps): React.JSX.Element {
  return (
    <div
      className="mt-4 rounded-keycap border-y border-r border-l-[3px] border-alert bg-bad-tint p-4"
      ref={containerRef}
      role={role}
      tabIndex={role === undefined ? undefined : -1}
    >
      {children}
    </div>
  );
}
