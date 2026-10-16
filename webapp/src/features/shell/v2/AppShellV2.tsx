import { useEffect, useRef } from "react";
import type { ReactNode } from "react";
import { Eyebrow } from "../../../components/Eyebrow";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { Icon } from "../../../components/m3/Icon";
import type { IconName } from "../../../components/m3/Icon";
import { SendStatus } from "../../../components/SendStatus";
import { StatusBadge } from "../../../components/StatusBadge";
import type { ScreenV2 } from "../../../v2/routingV2";
import type { UsbState } from "../../../v2/apiTypes";
import type { ActiveSendSummary } from "./activeSendSummary";
import { useBeforeUnloadGuard } from "./useBeforeUnloadGuard";

/**
 * The authenticated application shell, per UI_UX_SPEC_V2 §2 (TODO_V2
 * V2-090): device name, selected package, USB state, repository state, Save
 * snapshot when dirty, and primary navigation -- an M3 navigation bar at the
 * bottom on phones and a navigation rail from 600px up
 * (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §4.3). Purely presentational
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
  /**
   * A send in progress that the current route does not display itself. The
   * Macros route shows its own status, so the caller passes `null` there.
   */
  activeSend?: ActiveSendSummary | null;
}

const navigation = [
  ["macros", "Macros", "keyboard"],
  ["packages", "Packages", "layers"],
  ["snapshots", "Snapshots", "archive"],
  ["settings", "Settings", "settings"],
] as const satisfies readonly (readonly [ScreenV2, string, IconName])[];

/**
 * `diagnostics` belongs to the Settings section for nav-highlighting
 * purposes — it is reachable only from Settings, not its own bottom-nav
 * destination (UI_UX_SPEC_V2 §4/§11).
 */
function navigationActive(route: ScreenV2, target: ScreenV2): boolean {
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
  activeSend = null,
}: AppShellV2Props): React.JSX.Element {
  useBeforeUnloadGuard(dirty);

  // A hash route change swaps the content under a focused nav button, so
  // without this a screen-reader or keyboard user is left on the button with
  // no signal that anything changed. Move focus to the content region on
  // every route change after the first render.
  const mainRef = useRef<HTMLElement>(null);
  const firstRender = useRef(true);
  useEffect(() => {
    if (firstRender.current) {
      firstRender.current = false;
      return;
    }
    mainRef.current?.focus({ preventScroll: true });
  }, [route]);

  return (
    // Locked to the viewport: the app bar and navigation are fixed chrome and
    // #main-content below is the one scrolling region, so a page can pin
    // something outside the part that scrolls. `app-shell` carries no CSS
    // rule; it is a structural test hook --
    // tests/v2-app-v2-orientation.test.tsx asserts it stays in the DOM (not
    // discarded) while hidden behind the landscape-block overlay, proving the
    // shell stayed mounted rather than reloading.
    //
    // 37.5rem (600px) is the M3 compact/medium boundary: below it the
    // navigation is a bottom bar (`order-last`), at and above it a left rail.
    // The nav is first in the DOM so keyboard order matches the rail's visual
    // order on larger screens.
    <div className="app-shell group/shell flex h-dvh w-full flex-col bg-surface min-[37.5rem]:flex-row">
      <button
        className="sr-only focus:not-sr-only focus:fixed focus:top-2 focus:left-2 focus:z-50"
        onClick={() => {
          mainRef.current?.focus();
        }}
        type="button"
      >
        Skip to main content
      </button>
      <nav
        aria-label="Primary navigation"
        className="order-last grid grid-cols-4 gap-1 bg-surface-container px-2 pt-3 pb-[calc(0.75rem+env(safe-area-inset-bottom))] min-[37.5rem]:order-none min-[37.5rem]:flex min-[37.5rem]:w-20 min-[37.5rem]:shrink-0 min-[37.5rem]:flex-col min-[37.5rem]:items-center min-[37.5rem]:gap-3 min-[37.5rem]:px-0 min-[37.5rem]:pt-[calc(1.5rem+env(safe-area-inset-top))] [@media(pointer:coarse)_and_(width<37.5rem)]:group-has-[:is(input,textarea,select):focus]/shell:hidden"
      >
        {navigation.map(([target, label, icon]) => {
          const active = navigationActive(route, target);
          return (
            <button
              aria-current={active ? "page" : undefined}
              className="min-h-14 min-w-0 flex-col gap-1 rounded-none bg-transparent p-0 text-label-md text-on-surface-variant hover:[background-image:none] active:[background-image:none] min-[37.5rem]:w-full"
              key={target}
              onClick={() => {
                navigate(target);
              }}
              type="button"
            >
              {/* The active destination is marked by the filled indicator
                  pill AND a bold label (UI_UX_SPEC_V2 §5: never colour
                  alone). Full literal string per branch, not interpolation
                  (§8.2). */}
              <span
                className={
                  active
                    ? "grid h-8 w-16 place-items-center rounded-full bg-secondary-container text-on-secondary-container"
                    : "grid h-8 w-16 place-items-center rounded-full"
                }
              >
                <Icon name={icon} />
              </span>
              <span className={active ? "font-bold text-on-surface" : ""}>
                {label}
              </span>
            </button>
          );
        })}
      </nav>
      <div className="flex min-h-0 min-w-0 flex-1 flex-col">
        {/* One compact bar. Device name, package, USB state, saved state and
            the Save action are all required (UI_UX_SPEC_V2 §2.1); they share
            one wrapping row so the bar costs one line of chrome on a wide
            screen and two on a phone, instead of the old three. */}
        <header className="flex flex-wrap items-center gap-x-3 gap-y-2 bg-surface px-4 pt-[calc(0.5rem+env(safe-area-inset-top))] pb-2">
          <div className="min-w-0 flex-1 overflow-hidden">
            <Eyebrow>{deviceName}</Eyebrow>
            <h1 className="m-0 overflow-hidden text-title-lg text-ellipsis whitespace-nowrap">
              {packageName ?? "No package selected"}
            </h1>
          </div>
          {dirty ? (
            <button
              className="primary"
              disabled={saving}
              onClick={onSaveSnapshot}
              type="button"
            >
              <Icon name="save" size={18} />
              {saving ? "Saving…" : "Save snapshot"}
            </button>
          ) : null}
          <div className="flex basis-full flex-wrap items-center gap-2 min-[37.5rem]:basis-auto">
            <StatusBadge
              label={`USB ${usbState}`}
              state={usbBadgeState(usbState)}
            />
            {/* SPEC_V2/UI_UX_SPEC_V2 §2.1: the unsaved indicator remains
                visible on every authenticated operational screen, cleared
                only by a successful snapshot upload or a deliberate
                discard. */}
            <StatusBadge
              aria-live="polite"
              label={dirty ? "Unsaved changes" : "Saved"}
              role="status"
              state="neutral"
            />
          </div>
        </header>
        {saveError === null ? null : (
          <div className="px-4 pb-2">
            <ErrorBanner message={saveError} />
          </div>
        )}
        {activeSend === null ? null : (
          <div className="px-4">
            <SendStatus role="status">
              <p>{activeSend.statusText}</p>
              {activeSend.onCancel === null ? null : (
                <button
                  className="danger"
                  onClick={activeSend.onCancel}
                  type="button"
                >
                  Cancel and release all keys
                </button>
              )}
            </SendStatus>
          </div>
        )}
        <main id="main-content" ref={mainRef} tabIndex={-1}>
          <div className="mx-auto w-full max-w-[52rem]">{children}</div>
        </main>
      </div>
    </div>
  );
}
