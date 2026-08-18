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
 *
 * Reviewed deliberately (`WEBAPP_TAILWIND_TODO_2026-08-18.md` T4-3): kept as
 * a derivation rather than two independent props. Verified against every
 * call site — `PackageManagementPage`, `SnapshotsPage`'s import confirm, and
 * `SnapshotRow`'s delete confirm all pass `role="alertdialog"` with a
 * `containerRef` and get `tabIndex={-1}` for free; `SnapshotRow`'s other use
 * (a static "this is permanent" callout, no confirm flow, nothing to focus)
 * passes neither. Deriving the two together makes the bug this component
 * exists to prevent — a focus-trapped container with no `role`, or an
 * `alertdialog` assistive tech can't move focus into — structurally
 * unrepresentable rather than merely undocumented.
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
