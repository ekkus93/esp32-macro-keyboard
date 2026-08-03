import { useCallback, useEffect, useMemo, useState } from "react";
import { errorText } from "./api/errors";
import { getDeviceStatus, getSettings, listPackages } from "./api/routes";
import { AppShell } from "./components/AppShell";
import { ErrorBanner } from "./components/ErrorBanner";
import { SessionBoundary } from "./features/auth/SessionBoundary";
import { ConfirmExecutionPage } from "./features/execution/ConfirmExecutionPage";
import { ExecutionPage } from "./features/execution/ExecutionPage";
import { ExecutionResultPage } from "./features/execution/ExecutionResultPage";
import { MacroEditorPage } from "./features/macros/MacroEditorPage";
import { MacroLibraryPage } from "./features/macros/MacroLibraryPage";
import { PackageManagementPage } from "./features/package/PackageManagementPage";
import { PackageSelectionPage } from "./features/package/PackageSelectionPage";
import { DiagnosticsPage } from "./features/settings/DiagnosticsPage";
import { PackageOperationsPage } from "./features/settings/PackageOperationsPage";
import { SettingsPage } from "./features/settings/SettingsPage";
import {
  executionConfirmationTargetFromHash,
  macroEditorTargetFromHash,
  navigate,
  navigateToMacroConfirmation,
  navigateToMacroEditor,
  routeFromHash,
} from "./routing";
import type { Screen } from "./routing";
import type {
  DeviceStatus,
  ExecutionStatus,
  MacroPackage,
  Settings,
} from "./types/models";

interface AuthenticatedAppProps {
  initialStatus: DeviceStatus;
  logout: () => Promise<void>;
}

function AuthenticatedApp({
  initialStatus,
  logout,
}: AuthenticatedAppProps): React.JSX.Element {
  const [route, setRoute] = useState<Screen>(() => routeFromHash());
  const [routeHash, setRouteHash] = useState(() => window.location.hash);
  const [confirmationTarget, setConfirmationTarget] = useState(() =>
    executionConfirmationTargetFromHash(),
  );
  const [settings, setSettings] = useState<Settings | null>(null);
  const [packages, setPackages] = useState<MacroPackage[] | null>(null);
  const [status, setStatus] = useState(initialStatus);
  const [execution, setExecution] = useState<ExecutionStatus | null>(null);
  const [expectedExecutionId, setExpectedExecutionId] = useState<string | null>(
    null,
  );
  const [executionReturnHash, setExecutionReturnHash] = useState("/macros");
  const [runtimeError, setRuntimeError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);
  const [signingOut, setSigningOut] = useState(false);

  useEffect(() => {
    const onHashChange = (): void => {
      setRuntimeError(null);
      setRoute(routeFromHash());
      setRouteHash(window.location.hash);
      setConfirmationTarget(executionConfirmationTargetFromHash());
    };
    window.addEventListener("hashchange", onHashChange);
    return () => {
      window.removeEventListener("hashchange", onHashChange);
    };
  }, []);

  useEffect(() => {
    let active = true;
    const load = async (): Promise<void> => {
      setRuntimeError(null);
      try {
        const [loadedSettings, loadedPackages] = await Promise.all([
          getSettings(),
          listPackages(),
        ]);
        if (active) {
          setSettings(loadedSettings);
          setPackages(loadedPackages);
        }
      } catch (loadError: unknown) {
        if (active) {
          setRuntimeError(errorText(loadError));
        }
      }
    };
    void load();
    return () => {
      active = false;
    };
  }, [loadVersion]);

  useEffect(() => {
    let active = true;
    const refresh = async (): Promise<void> => {
      try {
        const current = await getDeviceStatus();
        if (active) {
          setStatus(current);
        }
      } catch (statusError: unknown) {
        if (active) {
          setRuntimeError(errorText(statusError));
        }
      }
    };
    const timer = window.setInterval(() => {
      void refresh();
    }, 5_000);
    return () => {
      active = false;
      window.clearInterval(timer);
    };
  }, []);

  const activePackage = useMemo(
    () => packages?.find((pkg) => pkg.id === settings?.activePackageId) ?? null,
    [packages, settings],
  );

  const navigateTo = useCallback((target: Screen): void => {
    setRuntimeError(null);
    navigate(target);
  }, []);

  const onTerminal = useCallback(
    (terminal: ExecutionStatus): void => {
      setExecution(terminal);
      navigateTo("result");
    },
    [navigateTo],
  );

  const reloadLiveState = useCallback((): void => {
    setRuntimeError(null);
    setLoadVersion((version) => version + 1);
  }, []);

  const onPackageReplaced = useCallback((replacement: MacroPackage): void => {
    setPackages((current) =>
      current === null
        ? current
        : current.map((item) =>
            item.id === replacement.id ? replacement : item,
          ),
    );
  }, []);

  const onPackageImported = useCallback((created: MacroPackage): void => {
    setPackages((current) =>
      current === null ? current : [...current, created],
    );
  }, []);

  const signOut = async (): Promise<void> => {
    setSigningOut(true);
    setRuntimeError(null);
    try {
      await logout();
    } catch (logoutError: unknown) {
      setRuntimeError(errorText(logoutError));
      setSigningOut(false);
    }
  };

  if (settings === null || packages === null) {
    return (
      <main className="standalone" aria-busy="true">
        <ErrorBanner message={runtimeError} />
        {runtimeError === null ? (
          <p role="status">Loading device configuration…</p>
        ) : (
          <button onClick={reloadLiveState} type="button">
            Retry
          </button>
        )}
      </main>
    );
  }

  const content = (() => {
    switch (route) {
      case "packages":
        return (
          <PackageSelectionPage
            onManage={() => {
              navigateTo("manage-packages");
            }}
            onSelected={setSettings}
            packages={packages}
            settings={settings}
          />
        );
      case "settings":
        return (
          <SettingsPage
            navigate={navigateTo}
            onUpdated={setSettings}
            settings={settings}
          />
        );
      case "execution":
        return (
          <ExecutionPage
            expectedExecutionId={expectedExecutionId}
            onTerminal={onTerminal}
          />
        );
      case "result":
        return (
          <ExecutionResultPage
            execution={execution}
            onReturn={() => {
              setExpectedExecutionId(null);
              window.location.hash = executionReturnHash;
            }}
          />
        );
      case "macros":
        return (
          <MacroLibraryPage
            activePackage={activePackage}
            onCreate={() => {
              navigateToMacroEditor(null);
            }}
            onEdit={(macroId) => {
              navigateToMacroEditor(macroId);
            }}
            onSend={(macroId) => {
              navigateToMacroConfirmation(macroId);
            }}
          />
        );
      case "macro-editor":
        return (
          <MacroEditorPage
            activePackage={activePackage}
            key={`${activePackage?.id ?? "none"}:${routeHash}`}
            onBack={() => {
              navigateTo("macros");
            }}
            target={macroEditorTargetFromHash()}
          />
        );
      case "confirm":
        return (
          <ConfirmExecutionPage
            activePackage={activePackage}
            key={`${activePackage?.id ?? "none"}:${routeHash}`}
            onAccepted={(accepted, returnHash) => {
              setExecution(null);
              setExpectedExecutionId(accepted.executionId);
              setExecutionReturnHash(returnHash);
              navigateTo("execution");
            }}
            settings={settings}
            status={status}
            target={confirmationTarget}
          />
        );
      case "manage-packages":
      case "package-editor":
      case "delete-package":
        return (
          <PackageManagementPage
            onPackagesChanged={setPackages}
            packages={packages}
            settings={settings}
          />
        );
      case "import":
        return (
          <PackageOperationsPage
            activePackage={activePackage}
            initialSection="import"
            onPackageImported={onPackageImported}
            onPackageReplaced={onPackageReplaced}
          />
        );
      case "export":
        return (
          <PackageOperationsPage
            activePackage={activePackage}
            initialSection="export"
          />
        );
      case "diagnostics":
        return <DiagnosticsPage />;
    }
  })();

  return (
    <AppShell
      activePackage={activePackage?.name ?? "No active macro package"}
      logoutDisabled={signingOut}
      navigate={navigateTo}
      onLogout={() => {
        if (!signingOut) {
          void signOut();
        }
      }}
      onReconnect={reloadLiveState}
      route={route}
      usbState={status.usbState}
    >
      <ErrorBanner message={runtimeError} />
      {signingOut ? <p role="status">Signing out…</p> : null}
      {content}
    </AppShell>
  );
}

export default function App(): React.JSX.Element {
  return (
    <SessionBoundary>
      {({ initialStatus, logout }) => (
        <AuthenticatedApp initialStatus={initialStatus} logout={logout} />
      )}
    </SessionBoundary>
  );
}
