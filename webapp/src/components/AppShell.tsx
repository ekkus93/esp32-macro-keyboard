import type { ReactNode } from "react";
import type { Screen } from "../routing";
import type { UsbState } from "../types/models";
import { StatusBadge } from "./StatusBadge";

interface AppShellProps {
  activeSet: string;
  children: ReactNode;
  route: Screen;
  usbState: UsbState;
  navigate: (route: Screen) => void;
  onLogout: () => void;
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

export function AppShell({
  activeSet,
  children,
  route,
  usbState,
  navigate,
  onLogout,
  logoutDisabled,
}: AppShellProps): React.JSX.Element {
  const navigation = [
    ["sets", "Sets"],
    ["procedures", "Procedures"],
    ["macros", "Macros"],
    ["settings", "Settings"],
  ] as const satisfies readonly (readonly [Screen, string])[];

  return (
    <div className="app-shell">
      <header className="app-header">
        <div>
          <p className="eyebrow">ESP32 Macro Keyboard</p>
          <h1>{activeSet}</h1>
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
      <main>{children}</main>
      <nav className="bottom-nav" aria-label="Primary navigation">
        {navigation.map(([target, label]) => (
          <button
            className={route === target ? "active" : ""}
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
