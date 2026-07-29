import { useCallback, useEffect, useMemo, useState } from "react";
import { errorText } from "./api/errors";
import { getDeviceStatus, getSettings, listSets } from "./api/routes";
import { AppShell } from "./components/AppShell";
import { ErrorBanner } from "./components/ErrorBanner";
import { SessionBoundary } from "./features/auth/SessionBoundary";
import { ConfirmExecutionPage } from "./features/execution/ConfirmExecutionPage";
import { ExecutionPage } from "./features/execution/ExecutionPage";
import { ExecutionResultPage } from "./features/execution/ExecutionResultPage";
import { MacroEditorPage } from "./features/macros/MacroEditorPage";
import { MacroLibraryPage } from "./features/macros/MacroLibraryPage";
import { ProcedureLibraryPage } from "./features/procedures/ProcedureLibraryPage";
import { ProcedureWorkflowPage } from "./features/procedures/ProcedureWorkflowPage";
import { SetSelectionPage } from "./features/sets/SetSelectionPage";
import { SettingsPage } from "./features/settings/SettingsPage";
import { DeferredPage } from "./pages/DeferredPage";
import {
  executionConfirmationTargetFromHash,
  macroEditorTargetFromHash,
  navigate,
  navigateToMacroConfirmation,
  navigateToMacroEditor,
  navigateToProcedure,
  procedureTargetFromHash,
  routeFromHash,
} from "./routing";
import type { Screen } from "./routing";
import type {
  DeviceStatus,
  ExecutionStatus,
  MacroSet,
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
  const [settings, setSettings] = useState<Settings | null>(null);
  const [sets, setSets] = useState<MacroSet[] | null>(null);
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
        const [loadedSettings, loadedSets] = await Promise.all([
          getSettings(),
          listSets(),
        ]);
        if (active) {
          setSettings(loadedSettings);
          setSets(loadedSets);
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

  const activeSet = useMemo(
    () => sets?.find((set) => set.id === settings?.activeSetId) ?? null,
    [sets, settings],
  );
  const confirmationTarget = useMemo(
    () => executionConfirmationTargetFromHash(),
    [routeHash],
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

  if (settings === null || sets === null) {
    return (
      <main className="standalone" aria-busy="true">
        <ErrorBanner message={runtimeError} />
        {runtimeError === null ? (
          <p role="status">Loading device configuration…</p>
        ) : (
          <button
            onClick={() => {
              setLoadVersion((version) => version + 1);
            }}
            type="button"
          >
            Retry
          </button>
        )}
      </main>
    );
  }

  const deferred = (title: string, message: string): React.JSX.Element => (
    <DeferredPage title={title} message={message} />
  );

  const content = (() => {
    switch (route) {
      case "sets":
        return (
          <SetSelectionPage
            onSelected={setSettings}
            sets={sets}
            settings={settings}
          />
        );
      case "settings":
        return <SettingsPage onUpdated={setSettings} settings={settings} />;
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
      case "procedures":
        return (
          <ProcedureLibraryPage
            activeSet={activeSet}
            onOpen={(procedureId) => {
              navigateToProcedure(procedureId);
            }}
          />
        );
      case "procedure":
        return (
          <ProcedureWorkflowPage
            activeSet={activeSet}
            key={`${activeSet?.id ?? "none"}:${routeHash}`}
            mode="overview"
            target={procedureTargetFromHash("procedure")}
          />
        );
      case "instruction":
        return (
          <ProcedureWorkflowPage
            activeSet={activeSet}
            key={`${activeSet?.id ?? "none"}:${routeHash}`}
            mode="step"
            target={procedureTargetFromHash("instruction")}
          />
        );
      case "procedure-editor":
        return deferred(
          "Edit procedure",
          "Procedure editing and accessible reordering are not enabled in this slice.",
        );
      case "macros":
        return (
          <MacroLibraryPage
            activeSet={activeSet}
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
            activeSet={activeSet}
            key={`${activeSet?.id ?? "none"}:${routeHash}`}
            onBack={() => {
              navigateTo("macros");
            }}
            target={macroEditorTargetFromHash()}
          />
        );
      case "confirm":
        return (
          <ConfirmExecutionPage
            activeSet={activeSet}
            key={`${activeSet?.id ?? "none"}:${routeHash}`}
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
      case "manage-sets":
        return (
          <SetSelectionPage
            onSelected={setSettings}
            sets={sets}
            settings={settings}
          />
        );
      case "set-editor":
        return deferred(
          "Create macro set",
          "Set creation and editing are not enabled in this slice.",
        );
      case "delete-set":
        return deferred(
          "Delete macro set",
          "Deletion is unavailable until a live set and current revision are selected.",
        );
      case "import":
        return deferred(
          "Import macro set",
          "Import remains unavailable until the Phase 18 transactional package service exists.",
        );
      case "export":
        return deferred(
          "Export macro set",
          "Export remains unavailable until the Phase 18 package service exists.",
        );
      case "diagnostics":
        return deferred(
          "Diagnostics",
          "Full diagnostics aggregation remains a Phase 19 boundary.",
        );
    }
  })();

  return (
    <AppShell
      activeSet={activeSet?.name ?? "No active macro set"}
      logoutDisabled={signingOut}
      navigate={navigateTo}
      onLogout={() => {
        if (!signingOut) {
          void signOut();
        }
      }}
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
