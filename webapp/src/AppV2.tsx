import { useEffect, useState } from "react";
import { v2ErrorText } from "./features/auth/v2/v2ErrorText";
import { FirstRunSetupPage } from "./features/auth/v2/FirstRunSetupPage";
import { SignInPage } from "./features/auth/v2/SignInPage";
import { AppShellV2 } from "./features/shell/v2/AppShellV2";
import { PlaceholderPage } from "./features/shell/v2/PlaceholderPage";
import { useDeviceStatus } from "./features/shell/v2/useDeviceStatus";
import { MacroEditorPage } from "./features/macros/v2/MacroEditorPage";
import { MacroPreviewPage } from "./features/macros/v2/MacroPreviewPage";
import { MacrosPage } from "./features/macros/v2/MacrosPage";
import { PackageManagementPage } from "./features/macros/v2/PackageManagementPage";
import { SnapshotsPage } from "./features/snapshots/v2/SnapshotsPage";
import type { RepositoryStartupReady } from "./features/startup/v2/RepositoryStartupScreen";
import { RepositoryStartupScreen } from "./features/startup/v2/RepositoryStartupScreen";
import { ErrorBanner } from "./components/ErrorBanner";
import { V2ApiError, subscribeUnauthorized, v2GetJson } from "./v2/apiClient";
import { isSetupStateResponse } from "./v2/apiGuards";
import { getSettings } from "./v2/settingsClient";
import { saveWorkingCopyAsSnapshot } from "./v2/snapshotClient";
import {
  macroEditorTargetFromHash,
  macroPreviewTargetFromHash,
  navigateToAddMacro,
  navigateToEditMacro,
  navigateToMacroPreview,
  navigateToMacros,
  navigateV2,
  routeFromHashV2,
} from "./v2/routingV2";
import type { ScreenV2 } from "./v2/routingV2";
import type { SendStatusResponse, SettingsResponse } from "./v2/apiTypes";

/**
 * The v2 application root (TODO_V2 V2-090): first-run setup or Sign In,
 * repository startup (Phase 8), and — once a package is resolved — the
 * authenticated shell with the Macros page (Phase 9). This is what
 * `main.tsx` mounts; the retired v1 `App` remains only for its own existing
 * test coverage — its routes no longer exist on the device (Phase 2 deleted
 * the firmware-owned package/macro/execution API it called).
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

function AuthenticatedShell({
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

  const usbState = useDeviceStatus();

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

  const [sendHandoff, setSendHandoff] = useState<SendStatusResponse | null>(
    ready.send,
  );

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

  if (settingsError !== null) {
    return (
      <main className="standalone">
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
      </main>
    );
  }

  if (settings === null) {
    return (
      <main aria-busy="true" className="standalone">
        <p role="status">Loading device settings…</p>
      </main>
    );
  }

  const content = (() => {
    switch (route) {
      case "macros":
        return (
          <MacrosPage
            initialSend={sendHandoff}
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
            usbState={usbState}
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
            usbState={usbState}
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
            onWorkingCopyOriginChanged={setLoadedBlobId}
            retentionTarget={settings.snapshotRetentionTarget}
            saveError={saveError}
            saving={saving}
            selectedPackageId={packageId}
            store={store}
          />
        );
      case "settings":
        return <PlaceholderPage phase="Phase 12 (V2-120)" title="Settings" />;
    }
  })();

  return (
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
      usbState={usbState}
    >
      {content}
    </AppShellV2>
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
        <main aria-busy="true" className="standalone">
          <p role="status">Checking device mode…</p>
        </main>
      );
    case "device-unreachable":
      return (
        <main className="standalone">
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
        </main>
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
