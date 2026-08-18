import type { ReactNode } from "react";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { StatusBadge } from "../../../components/StatusBadge";
import type { ScreenV2 } from "../../../v2/routingV2";
import type { UsbState } from "../../../v2/apiTypes";
import { useBeforeUnloadGuard } from "./useBeforeUnloadGuard";

/**
 * The authenticated application shell, per UI_UX_SPEC_V2 §2 (TODO_V2
 * V2-090): device name, selected package, USB state, repository state, Save
 * snapshot when dirty, and the fixed bottom navigation. Purely presentational
 * — this component owns no network calls; its caller supplies live state and
 * the Save snapshot handler. It also registers the `beforeunload` warning
 * while dirty (TODO_V2 V2-103) since every authenticated route renders
 * inside this one shell instance, making it the single place that guard
 * needs to live.
 */

export interface AppShellV2Props {
  children: ReactNode;
  deviceName: string;
  packageName: string | null;
  usbState: UsbState;
  dirty: boolean;
  route: ScreenV2;
  navigate: (route: ScreenV2) => void;
  onSaveSnapshot: () => void;
  saving: boolean;
  saveError: string | null;
}

const navigation = [
  ["macros", "Macros"],
  ["packages", "Packages"],
  ["snapshots", "Snapshots"],
  ["settings", "Settings"],
] as const satisfies readonly (readonly [ScreenV2, string])[];

/**
 * `macro-preview` belongs to the Macros section, and `diagnostics` to the
 * Settings section (it is reachable only from Settings, not its own
 * bottom-nav destination — UI_UX_SPEC_V2 §4/§11), for nav-highlighting
 * purposes.
 */
function navigationActive(route: ScreenV2, target: ScreenV2): boolean {
  if (target === "macros") {
    return route === "macros" || route === "macro-preview";
  }
  if (target === "settings") {
    return route === "settings" || route === "diagnostics";
  }
  return route === target;
}

function usbBadgeState(
  state: UsbState,
): "good" | "warning" | "bad" | "neutral" {
  switch (state) {
    case "ready":
      return "good";
    case "enumerating":
    case "suspended":
      return "warning";
    case "error":
      return "bad";
    case "uninitialized":
    case "disconnected":
      return "neutral";
  }
}

export function AppShellV2({
  children,
  deviceName,
  packageName,
  usbState,
  dirty,
  route,
  navigate,
  onSaveSnapshot,
  saving,
  saveError,
}: AppShellV2Props): React.JSX.Element {
  useBeforeUnloadGuard(dirty);

  return (
    // Locked to the viewport, not merely a min-height floor: header and
    // bottom nav are fixed chrome and #main-content below is the one
    // scrolling region, so a page can genuinely pin something outside the
    // part that scrolls. width: min(100%, 48rem)/64rem>=60rem is shared with
    // StandaloneScreen.tsx -- kept identical by hand since both need the
    // same measured column width.
    <div
      // app-shell carries no CSS rule (styling is inline below); kept as a
      // structural test hook only --
      // tests/v2-app-v2-orientation.test.tsx:56 asserts it's still in the
      // DOM (not discarded) while hidden behind the landscape-block
      // overlay, proving the shell stayed mounted rather than reloading.
      className="app-shell mx-auto flex h-dvh flex-col overflow-hidden bg-shell w-[min(100%,48rem)] min-[60rem]:w-[min(100%,64rem)]"
    >
      {/* Every piece of status here is required (UI_UX_SPEC_V2 §2.1: device
          name, package name, USB state, repository state, Save snapshot
          when dirty) -- none of it is optional content to cut. What was
          wrong was density: a tall top/bottom padding written for a single
          row, applied to a header that then force-stacked into 2-3 rows
          below 32rem. Kept to one wrapping row instead, with tight padding
          sized to the 44px Save-snapshot button that already sets the
          row's real minimum height -- the padding was adding height on top
          of that floor, not because the content needed it. */}
      <header className="flex flex-wrap items-center justify-between gap-x-3 gap-y-[0.4rem] border-b-[3px] border-actuate bg-legend px-[0.85rem] pb-[0.4rem] pt-[calc(0.4rem+env(safe-area-inset-top))] text-panel">
        <div className="min-w-0 overflow-hidden">
          <p className="eyebrow">{deviceName}</p>
          <h1 className="m-0 overflow-hidden text-ellipsis whitespace-nowrap text-[1.05rem] tracking-[-0.01em]">
            {packageName ?? "No package selected"}
          </h1>
        </div>
        <div className="header-actions">
          <StatusBadge
            label={`USB ${usbState}`}
            state={usbBadgeState(usbState)}
          />
          {/* SPEC_V2/UI_UX_SPEC_V2 §2.1: the unsaved indicator remains
              visible on every authenticated operational screen, cleared only
              by a successful snapshot upload or a deliberate discard. */}
          <span
            aria-live="polite"
            className="status-badge status-neutral"
            role="status"
          >
            {dirty ? "Unsaved changes" : "Saved"}
          </span>
          {dirty ? (
            <button
              className="min-h-[44px] border-header-button-edge bg-header-button px-[0.65rem] py-[0.3rem] text-[0.82rem] text-cap"
              disabled={saving}
              onClick={onSaveSnapshot}
              type="button"
            >
              {saving ? "Saving…" : "Save snapshot"}
            </button>
          ) : null}
        </div>
      </header>
      <ErrorBanner message={saveError} />
      <main id="main-content" tabIndex={-1}>
        {children}
      </main>
      {/* No position: sticky here -- the shell above is a locked-height flex
          column (#main-content is the one scrolling region within it), so
          this is simply the last flex child and is always visible without
          any scroll-tracking. grid-cols-4, not a hand-rolled
          grid-template-columns: Tailwind's default for that utility is
          already `repeat(4, minmax(0, 1fr))` -- minmax(0, 1fr) over a bare
          1fr matters because a bare `1fr` track's implied minimum is its
          content's min-content width, so it refuses to shrink below that —
          at narrow viewports the four buttons (bold/uppercase/
          letter-spaced text such as "Settings") would force this row wider
          than the viewport itself. */}
      <nav
        aria-label="Primary navigation"
        className="grid grid-cols-4 gap-[0.4rem] border-t border-cap-edge bg-shell px-[0.6rem] pt-[0.6rem] pb-[calc(0.6rem+env(safe-area-inset-bottom))]"
      >
        {navigation.map(([target, label]) => (
          <button
            aria-current={navigationActive(route, target) ? "page" : undefined}
            // The active destination is marked by an inset top rule and
            // full ink as well as colour (UI_UX_SPEC_V2 §5: never colour
            // alone). Full literal string per branch, not interpolation
            // (§8.2) -- a template literal here would produce no substring
            // Tailwind's content scanner can match.
            className={
              navigationActive(route, target)
                ? "min-w-0 px-[0.35rem] text-[0.78rem] font-bold uppercase tracking-[0.04em] border-actuate-edge bg-actuate text-cap shadow-[inset_0_3px_0_var(--color-lamp)]"
                : "min-w-0 px-[0.35rem] text-[0.78rem] font-bold uppercase tracking-[0.04em]"
            }
            key={target}
            onClick={() => {
              navigate(target);
            }}
            type="button"
          >
            {label}
          </button>
        ))}
      </nav>
    </div>
  );
}
