import type { ReactNode } from "react";
import type { Screen } from "../routing";
import type { UsbState } from "../types/models";
import { ConnectivityBanner } from "./ConnectivityBanner";
import { StatusBadge } from "./StatusBadge";

interface AppShellProps {
  activePackage: string;
  children: ReactNode;
  route: Screen;
  usbState: UsbState;
  navigate: (route: Screen) => void;
  onLogout: () => void;
  onReconnect: () => void;
  logoutDisabled: boolean;
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

function navigationActive(route: Screen, target: Screen): boolean {
  if (target === "packages") {
    return [
      "packages",
      "manage-packages",
      "package-editor",
      "delete-package",
    ].includes(route);
  }
  if (target === "macros") {
    return [
      "macros",
      "macro-editor",
      "confirm",
      "execution",
      "result",
    ].includes(route);
  }
  if (target === "settings") {
    return ["settings", "import", "export", "diagnostics"].includes(route);
  }
  return route === target;
}

export function AppShell({
  activePackage,
  children,
  route,
  usbState,
  navigate,
  onLogout,
  onReconnect,
  logoutDisabled,
}: AppShellProps): React.JSX.Element {
  /* Order fixed by SPEC 9: Macros | Packages | Settings. */
  const navigation = [
    ["macros", "Macros"],
    ["packages", "Packages"],
    ["settings", "Settings"],
  ] as const satisfies readonly (readonly [Screen, string])[];

  return (
    <div className="app-shell">
      <header className="app-header">
        <div>
          <p className="eyebrow">ESP32 Macro Keyboard</p>
          <h1>{activePackage}</h1>
        </div>
        <div className="header-actions">
          <StatusBadge
            label={`USB ${usbState}`}
            state={usbBadgeState(usbState)}
          />
          <button
            className="header-button"
            disabled={logoutDisabled}
            onClick={onLogout}
            type="button"
          >
            Sign out
          </button>
        </div>
      </header>
      <ConnectivityBanner onReconnect={onReconnect} />
      <main id="main-content" tabIndex={-1}>
        {children}
      </main>
      <nav className="bottom-nav" aria-label="Primary navigation">
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
