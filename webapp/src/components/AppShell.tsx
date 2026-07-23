import type { ReactNode } from "react";
import { StatusBadge } from "./StatusBadge";

interface AppShellProps {
  activeSet: string;
  children: ReactNode;
  route: string;
  navigate: (route: string) => void;
}

export function AppShell({
  activeSet,
  children,
  route,
  navigate,
}: AppShellProps): React.JSX.Element {
  const navigation = [
    ["procedures", "Procedures"],
    ["macros", "Macros"],
    ["settings", "Settings"],
  ] as const;

  return (
    <div className="app-shell">
      <header className="app-header">
        <div>
          <p className="eyebrow">ESP32 Macro Keyboard</p>
          <h1>{activeSet}</h1>
        </div>
        <StatusBadge label="USB status pending" state="neutral" />
      </header>
      <main>{children}</main>
      <nav className="bottom-nav" aria-label="Primary navigation">
        {navigation.map(([target, label]) => (
          <button
            className={route === target ? "active" : ""}
            key={target}
            onClick={() => navigate(target)}
            type="button"
          >
            {label}
          </button>
        ))}
      </nav>
    </div>
  );
}
