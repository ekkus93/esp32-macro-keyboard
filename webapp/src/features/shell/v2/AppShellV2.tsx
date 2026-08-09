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
    <div className="app-shell">
      <header className="app-header">
        <div>
          <p className="eyebrow">{deviceName}</p>
          <h1>{packageName ?? "No package selected"}</h1>
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
              className="header-button"
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
      <nav aria-label="Primary navigation" className="bottom-nav">
        {navigation.map(([target, label]) => (
          <button
            aria-current={navigationActive(route, target) ? "page" : undefined}
            className={navigationActive(route, target) ? "active" : ""}
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
