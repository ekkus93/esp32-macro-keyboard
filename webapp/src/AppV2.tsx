import { useEffect, useState } from "react";
import { StandaloneScreen } from "./components/StandaloneScreen";
import { v2ErrorText } from "./features/auth/v2/v2ErrorText";
import { FirstRunSetupPage } from "./features/auth/v2/FirstRunSetupPage";
import { SignInPage } from "./features/auth/v2/SignInPage";
import type { ActiveSendSummary } from "./features/shell/v2/activeSendSummary";
import { AppShellV2 } from "./features/shell/v2/AppShellV2";
import { LandscapeBlockSurface } from "./features/shell/v2/LandscapeBlockSurface";
import { useDeviceStatus } from "./features/shell/v2/useDeviceStatus";
import { useLandscapePhoneBlock } from "./features/shell/v2/useLandscapePhoneBlock";
import { MacroEditorPage } from "./features/macros/v2/MacroEditorPage";
import { MacroPreviewPage } from "./features/macros/v2/MacroPreviewPage";
import { MacrosPage } from "./features/macros/v2/MacrosPage";
import { PackageManagementPage } from "./features/macros/v2/PackageManagementPage";
import { SnapshotsPage } from "./features/snapshots/v2/SnapshotsPage";
import { DeviceReconnectScreen } from "./features/settings/v2/DeviceReconnectScreen";
import type { DeviceActionKind } from "./features/settings/v2/DeviceReconnectScreen";
import { DiagnosticsPage } from "./features/settings/v2/DiagnosticsPage";
import { SettingsPage } from "./features/settings/v2/SettingsPage";
import { useDeviceReconnect } from "./features/settings/v2/useDeviceReconnect";
import type { RepositoryStartupReady } from "./features/startup/v2/RepositoryStartupScreen";
import { RepositoryStartupScreen } from "./features/startup/v2/RepositoryStartupScreen";
import { ErrorBanner } from "./components/ErrorBanner";
import { V2ApiError, subscribeUnauthorized, v2GetJson } from "./v2/apiClient";
import { isSetupStateResponse } from "./v2/apiGuards";
import { getSettings } from "./v2/settingsClient";
import {
  packageSelectionPersistenceWarning,
  tryPersistSelectedPackageId,
  type PackageSelectionPersistenceFailure,
} from "./v2/packageSelection";
import { recoverSendState } from "./v2/sendClient";
import { saveWorkingCopyAsSnapshot } from "./v2/snapshotClient";
import type { SendRecoveryState } from "./v2/startup";
import { getStatus } from "./v2/statusClient";
import {
  macroEditorTargetFromHash,
  macroPreviewTargetFromHash,
  navigateToAddMacro,
  navigateToDiagnostics,
  navigateToEditMacro,
  navigateToMacroPreview,
  navigateToMacros,
  navigateToSettings,
  navigateV2,
  routeFromHashV2,
} from "./v2/routingV2";
import type { ScreenV2 } from "./v2/routingV2";
import type { SendStatusResponse, SettingsResponse } from "./v2/apiTypes";

/**
 * The v2 application root (TODO_V2 V2-090): first-run setup or Sign In,
 * repository startup (Phase 8), and — once a package is resolved — the
 * authenticated shell with the Macros page (Phase 9). This is what
 * `main.tsx` mounts and the only application tree in the codebase — the
 * retired v1 `App` (its own routes calling the firmware-owned
 * package/macro/execution API Phase 2 deleted) was removed in V2-140.
 */

type Phase =
  | { kind: "checking" }
  | { kind: "setup" }
  | { kind: "signin" }
  | { kind: "authenticated" }
  | { kind: "device-unreachable"; message: string };

interface AuthenticatedShellProps {
  ready: RepositoryStartupReady;
}

export function AuthenticatedShell({
  ready,
}: AuthenticatedShellProps): React.JSX.Element {
  const { store } = ready;
  const [snapshot, setSnapshot] = useState(() => store.getSnapshot());
  useEffect(() => store.subscribe(setSnapshot), [store]);

  // Mutable so the Packages page (TODO_V2 V2-102) can change the open
  // package — via Open (persisted, non-dirtying) or by resolving a new
  // selection after the previously selected package is deleted — without
  // remounting the whole authenticated shell.
  const [packageId, setPackageId] = useState(ready.packageId);
  const [persistedPackageId, setPersistedPackageId] = useState<string | null>(
    ready.selectionPersistenceFailure !== undefined
      ? ready.selectionPersistenceFailure.previousPackageId
      : ready.packageId,
  );
  const [selectionPersistenceFailure, setSelectionPersistenceFailure] =
    useState<PackageSelectionPersistenceFailure | null>(
      ready.selectionPersistenceFailure ?? null,
    );
  const [selectionPersistenceRetrying, setSelectionPersistenceRetrying] =
    useState(false);

  const usbStatus = useDeviceStatus();
  const trustedUsbState = usbStatus.degraded
    ? "uninitialized"
    : usbStatus.usbState;

  // TODO_V2 V2-131/V2-132 (UI_UX_SPEC_V2 §12): on a landscape phone, the
  // shell below stays fully mounted (its state, timers, and any active send
  // are untouched) but is hidden behind the full-viewport orientation
  // surface, which itself surfaces macro name/progress/Cancel via
  // `activeSend` — reported by whichever page currently owns send tracking
  // (only `MacrosPage` today).
  const landscapeBlocked = useLandscapePhoneBlock();
  const [activeSend, setActiveSend] = useState<ActiveSendSummary | null>(null);

  const [settings, setSettings] = useState<SettingsResponse | null>(null);
  const [settingsError, setSettingsError] = useState<string | null>(null);
  const [settingsAttempt, setSettingsAttempt] = useState(0);
  useEffect(() => {
    let active = true;
    setSettingsError(null);
    getSettings()
      .then((loaded) => {
        if (active) {
          setSettings(loaded);
        }
      })
      .catch((error: unknown) => {
        if (active) {
          setSettingsError(v2ErrorText(error));
        }
      });
    return () => {
      active = false;
    };
  }, [settingsAttempt]);

  const selectionPersistenceSucceeded = (
    nextPackageId: string | null,
  ): void => {
    setPersistedPackageId(nextPackageId);
    setSelectionPersistenceFailure(null);
    setSettings((current) =>
      current === null
        ? null
        : { ...current, lastSelectedPackageId: nextPackageId },
    );
  };

  const selectionPersistenceFailed = (
    failure: PackageSelectionPersistenceFailure,
  ): void => {
    setSelectionPersistenceFailure(failure);
  };

  const retrySelectionPersistence = async (): Promise<void> => {
    const failure = selectionPersistenceFailure;
    if (failure === null) {
      return;
    }
    setSelectionPersistenceRetrying(true);
    const attempt = await tryPersistSelectedPackageId(
      failure.packageId,
      persistedPackageId,
    );
    if (attempt.kind === "failed") {
      selectionPersistenceFailed(attempt.failure);
    } else {
      selectionPersistenceSucceeded(failure.packageId);
    }
    setSelectionPersistenceRetrying(false);
  };

  const [route, setRoute] = useState<ScreenV2>(() => routeFromHashV2());
  useEffect(() => {
    const onHashChange = (): void => {
      setRoute(routeFromHashV2());
    };
    window.addEventListener("hashchange", onHashChange);
    return () => {
      window.removeEventListener("hashchange", onHashChange);
    };
  }, []);

  const [sendRecovery, setSendRecovery] = useState<SendRecoveryState>(
    ready.sendRecovery,
  );
  const [sendRecoveryRetrying, setSendRecoveryRetrying] = useState(false);
  const [sendHandoff, setSendHandoff] = useState<SendStatusResponse | null>(
    ready.sendRecovery.kind === "known" ? ready.sendRecovery.send : null,
  );

  const retrySendRecovery = async (): Promise<void> => {
    setSendRecoveryRetrying(true);
    try {
      const recovered = await recoverSendState();
      if (recovered === null) {
        setSendRecovery({ kind: "none" });
        setSendHandoff(null);
      } else {
        setSendRecovery({ kind: "known", send: recovered });
        setSendHandoff(recovered);
      }
    } catch (error: unknown) {
      setSendRecovery({ kind: "unavailable", message: v2ErrorText(error) });
    } finally {
      setSendRecoveryRetrying(false);
    }
  };

  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);

  // The blob ID the working copy currently reflects, if any (TODO_V2 V2-111
  // "loaded snapshot indicator"). Kept here — not inside SnapshotsPage —
  // because both the header's Save snapshot button and the Snapshots page's
  // own Save/Load/Import/Replace actions all need to update it, and this is
  // the one place both are rendered from.
  const [loadedBlobId, setLoadedBlobId] = useState<string | null>(
    ready.loadedBlobId,
  );

  // Restart/reset-settings/factory-reset (TODO_V2 V2-121) all report
  // `connectionWillClose: true` (SPEC_V2 §13.12) — the device actually goes
  // offline and comes back. While `deviceAction` is set, this shell renders
  // `DeviceReconnectScreen` instead of its normal route content, replacing
  // the whole authenticated application rather than leaving one page to
  // fail its own requests silently.
  const [deviceAction, setDeviceAction] = useState<DeviceActionKind | null>(
    null,
  );
  const reconnect = useDeviceReconnect(deviceAction !== null, getStatus);

  useEffect(() => {
    if (deviceAction === null) {
      return;
    }
    if (reconnect.phase === "reachable") {
      if (deviceAction === "factory-reset") {
        // Erases every stored blob and reprovisions the device (SPEC_V2
        // §13.12 `reprovisioningRequired: true`) — nothing survives to
        // resume, so a full reload is the honest outcome, re-running the
        // top-level provisioning check from scratch.
        window.location.reload();
        return;
      }
      // The device came back and, against the general expectation, this
      // session is still valid — resume normally. Device settings may have
      // changed under us (reset settings restores several to defaults), so
      // refetch them rather than trust the stale copy.
      setDeviceAction(null);
      setSettingsAttempt((value) => value + 1);
      return;
    }
    if (
      reconnect.phase === "needs-reauth" &&
      deviceAction === "factory-reset"
    ) {
      window.location.reload();
    }
    // Otherwise: a `401` here already triggered the shared
    // `subscribeUnauthorized` mechanism (see `getStatus`'s underlying
    // `v2GetJson`), which drops the top-level `AppV2` to Sign In without
    // discarding `ready` — this shell unmounts on its own once that
    // happens, so there is nothing further to do here.
  }, [deviceAction, reconnect.phase]);

  const saveSnapshot = async (): Promise<void> => {
    setSaving(true);
    setSaveError(null);
    try {
      const created = await saveWorkingCopyAsSnapshot(store);
      setLoadedBlobId(created.id);
    } catch (error: unknown) {
      setSaveError(v2ErrorText(error));
    } finally {
      setSaving(false);
    }
  };

  const packageName =
    snapshot.repository.packages.find((pkg) => pkg.id === packageId)?.name ??
    null;

  if (deviceAction !== null && reconnect.phase !== "reachable") {
    return (
      <DeviceReconnectScreen
        elapsedMs={reconnect.elapsedMs}
        errorMessage={reconnect.errorMessage}
        kind={deviceAction}
        onManualReload={() => {
          window.location.reload();
        }}
        phase={reconnect.phase}
      />
    );
  }

  if (settingsError !== null) {
    return (
      <StandaloneScreen>
        <h1>Couldn&apos;t load device settings</h1>
        <ErrorBanner message={settingsError} />
        <button
          onClick={() => {
            setSettingsAttempt((value) => value + 1);
          }}
          type="button"
        >
          Retry
        </button>
      </StandaloneScreen>
    );
  }

  if (settings === null) {
    return (
      <StandaloneScreen aria-busy="true">
        <p role="status">Loading device settings…</p>
      </StandaloneScreen>
    );
  }

  const content = (() => {
    switch (route) {
      case "macros":
        return (
          <MacrosPage
            initialSend={sendHandoff}
            onActiveSendChange={setActiveSend}
            onChangePackage={() => {
              navigateV2("packages");
            }}
            onOpenAddMacro={navigateToAddMacro}
            onOpenEditMacro={(macroId) => {
              navigateToEditMacro(macroId);
            }}
            onOpenPreview={(macroId) => {
              navigateToMacroPreview(macroId);
            }}
            packageId={packageId}
            sendMode={settings.sendMode}
            showMacroSourcePreviews={settings.showMacroSourcePreviews}
            store={store}
            usbState={trustedUsbState}
          />
        );
      case "macro-preview": {
        const target = macroPreviewTargetFromHash();
        if (target.kind === "invalid") {
          return (
            <section>
              <h2>Preview and send</h2>
              <p role="alert">This preview link is invalid.</p>
              <button onClick={navigateToMacros} type="button">
                Back to Macros
              </button>
            </section>
          );
        }
        return (
          <MacroPreviewPage
            macroId={target.macroId}
            onBack={navigateToMacros}
            onSendInitiated={(status) => {
              setSendHandoff(status);
            }}
            packageId={packageId}
            store={store}
            usbState={trustedUsbState}
          />
        );
      }
      case "macro-editor": {
        const target = macroEditorTargetFromHash();
        if (target.kind === "invalid") {
          return (
            <section>
              <h2>Macro editor</h2>
              <p role="alert">This macro editor link is invalid.</p>
              <button onClick={navigateToMacros} type="button">
                Back to Macros
              </button>
            </section>
          );
        }
        return (
          <MacroEditorPage
            macroId={target.kind === "edit" ? target.macroId : null}
            onBack={navigateToMacros}
            onSaved={navigateToMacros}
            packageId={packageId}
            store={store}
          />
        );
      }
      case "packages":
        return (
          <PackageManagementPage
            onOpenMacros={navigateToMacros}
            onSelectionChange={setPackageId}
            onSelectionPersistenceFailure={selectionPersistenceFailed}
            onSelectionPersistenceSuccess={selectionPersistenceSucceeded}
            persistedPackageId={persistedPackageId}
            selectedPackageId={packageId}
            store={store}
          />
        );
      case "snapshots":
        return (
          <SnapshotsPage
            loadedBlobId={loadedBlobId}
            onOpenMacros={navigateToMacros}
            onOpenPackages={() => {
              navigateV2("packages");
            }}
            onSaveSnapshot={saveSnapshot}
            onSelectionChange={setPackageId}
            onSelectionPersistenceFailure={selectionPersistenceFailed}
            onSelectionPersistenceSuccess={selectionPersistenceSucceeded}
            onWorkingCopyOriginChanged={setLoadedBlobId}
            persistedPackageId={persistedPackageId}
            retentionTarget={settings.snapshotRetentionTarget}
            saveError={saveError}
            saving={saving}
            store={store}
          />
        );
      case "settings":
        return (
          <SettingsPage
            onDeviceActionStarted={setDeviceAction}
            onOpenDiagnostics={navigateToDiagnostics}
            onSaveSnapshot={saveSnapshot}
            onSettingsChanged={setSettings}
            saveError={saveError}
            saving={saving}
            settings={settings}
            store={store}
          />
        );
      case "diagnostics":
        return <DiagnosticsPage onBack={navigateToSettings} />;
    }
  })();

  return (
    <>
      {/* TODO_V2 V2-131: hidden, not unmounted, while landscape-blocked —
          `display: none` removes it from layout and the accessibility tree
          without touching React state, so returning to portrait resumes
          exactly where the tab left off. */}
      <div style={landscapeBlocked ? { display: "none" } : undefined}>
        <AppShellV2
          deviceName={settings.deviceName}
          dirty={snapshot.dirty}
          navigate={navigateV2}
          onSaveSnapshot={() => {
            void saveSnapshot();
          }}
          packageName={packageName}
          route={route}
          saveError={saveError}
          saving={saving}
          usbState={usbStatus.usbState}
        >
          {usbStatus.degraded ? (
            <ErrorBanner
              message={`USB status is stale. Last known state: ${usbStatus.usbState}. Sending is disabled until device status refreshes.`}
            />
          ) : null}
          {sendRecovery.kind === "unavailable" ? (
            <div className="grid gap-[0.85rem]">
              <ErrorBanner
                message={`Execution state unavailable. ${sendRecovery.message} An active send may still be running on the device.`}
              />
              <button
                disabled={sendRecoveryRetrying}
                onClick={() => {
                  void retrySendRecovery();
                }}
                type="button"
              >
                {sendRecoveryRetrying
                  ? "Retrying execution status…"
                  : "Retry execution status"}
              </button>
            </div>
          ) : null}
          {selectionPersistenceFailure !== null ? (
            <div className="grid gap-[0.85rem]">
              <ErrorBanner
                message={`${packageSelectionPersistenceWarning} ${v2ErrorText(
                  selectionPersistenceFailure.error,
                )}`}
              />
              <button
                disabled={selectionPersistenceRetrying}
                onClick={() => {
                  void retrySelectionPersistence();
                }}
                type="button"
              >
                {selectionPersistenceRetrying
                  ? "Retrying selection save…"
                  : "Retry saving selection"}
              </button>
            </div>
          ) : null}
          {content}
        </AppShellV2>
      </div>
      {landscapeBlocked ? (
        <LandscapeBlockSurface activeSend={activeSend} />
      ) : null}
    </>
  );
}

export default function AppV2(): React.JSX.Element {
  const [phase, setPhase] = useState<Phase>({ kind: "checking" });
  const [ready, setReady] = useState<RepositoryStartupReady | null>(null);

  useEffect(() => {
    let active = true;
    const check = async (): Promise<void> => {
      try {
        await v2GetJson("/api/v1/setup", isSetupStateResponse, {
          notifyOnUnauthorized: false,
        });
        if (active) {
          setPhase({ kind: "setup" });
        }
      } catch (error: unknown) {
        if (!active) {
          return;
        }
        if (error instanceof V2ApiError && error.status === 404) {
          setPhase({ kind: "signin" });
          return;
        }
        setPhase({ kind: "device-unreachable", message: v2ErrorText(error) });
      }
    };
    void check();
    return () => {
      active = false;
    };
  }, []);

  // Session expiry (V2-095) drops the app back to Sign In without ever
  // discarding `ready` — the live in-memory working copy — so a successful
  // re-authentication resumes exactly where the tab left off.
  useEffect(
    () =>
      subscribeUnauthorized(() => {
        setPhase({ kind: "signin" });
      }),
    [],
  );

  switch (phase.kind) {
    case "checking":
      return (
        <StandaloneScreen aria-busy="true">
          <p role="status">Checking device mode…</p>
        </StandaloneScreen>
      );
    case "device-unreachable":
      return (
        <StandaloneScreen>
          <h1>Device unreachable</h1>
          <ErrorBanner message={phase.message} />
          <button
            onClick={() => {
              window.location.reload();
            }}
            type="button"
          >
            Retry
          </button>
        </StandaloneScreen>
      );
    case "setup":
      return (
        <FirstRunSetupPage
          onSetupComplete={() => {
            setPhase({ kind: "signin" });
          }}
        />
      );
    case "signin":
      return (
        <SignInPage
          onAuthenticated={() => {
            setPhase({ kind: "authenticated" });
          }}
        />
      );
    case "authenticated":
      if (ready !== null) {
        return <AuthenticatedShell ready={ready} />;
      }
      return (
        <RepositoryStartupScreen
          existing={null}
          onReady={(nextReady) => {
            setReady(nextReady);
          }}
        />
      );
  }
}
