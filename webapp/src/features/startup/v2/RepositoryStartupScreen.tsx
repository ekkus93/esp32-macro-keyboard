import { useEffect, useRef, useState } from "react";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import { isGzipSupported } from "../../../v2/gzip";
import { v2Limits } from "../../../v2/limits";
import {
  tryPersistSelectedPackageId,
  type PackageSelectionPersistenceFailure,
} from "../../../v2/packageSelection";
import type { Repository } from "../../../v2/repository";
import {
  createEmptyRepository,
  validateRepositoryForUse,
} from "../../../v2/repositoryValidation";
import { createRepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";
import { recoverSendState } from "../../../v2/sendClient";
import { getSettings } from "../../../v2/settingsClient";
import {
  listSnapshots,
  loadSnapshotIntoWorkingCopy,
} from "../../../v2/snapshotClient";
import type { LoadSnapshotResult } from "../../../v2/snapshotClient";
import { resolvePackageDestination, runStartup } from "../../../v2/startup";
import type {
  StartupDependencies,
  StartupLoadStage,
  StartupResult,
} from "../../../v2/startup";
import type {
  BlobSummary,
  SendStatusResponse,
  SettingsResponse,
} from "../../../v2/apiTypes";

/**
 * V2-082/V2-083/V2-084 — the authenticated repository startup state
 * machine (UI_UX_SPEC_V2 §3.4-§3.6, §8). This component owns every screen
 * between "authentication just succeeded" and "a working copy and its
 * resolved package are ready to open" — the destination it hands off to
 * (a resolved package's Macros page) is Phase 9's concern, not this one's.
 */

export interface RepositoryStartupReady {
  store: RepositoryWorkingCopyStore;
  packageId: string;
  send: SendStatusResponse | null;
  /**
   * The blob ID the working copy currently reflects, or `null` when no
   * stored blob corresponds to it (a brand-new local repository/package
   * created because none existed, or one loaded but then discarded — see
   * {@link FirstPackageForm}). TODO_V2 V2-111's "loaded snapshot indicator".
   */
  loadedBlobId: string | null;
  /** Selection opened locally but its device preference write failed. */
  selectionPersistenceFailure?: PackageSelectionPersistenceFailure;
}

export interface RepositoryStartupScreenProps {
  /**
   * When the tab already holds a live working copy from an earlier
   * successful startup (UI_UX_SPEC_V2 §3.3), pass it here. The component
   * forwards it straight to `onReady` without a single network request —
   * "restores the current route, drafts, dirty state, and send state"
   * reduces to this at the startup layer: the caller owns keeping the
   * store alive across remounts (e.g. across a reauthentication), and as
   * long as it does, this component never reruns its load sequence.
   */
  existing: RepositoryStartupReady | null;
  onReady: (ready: RepositoryStartupReady) => void;
  /** Test-only dependency injection; defaults to the real v2 API clients. */
  dependencies?: StartupDependencies;
}

function defaultDependencies(): StartupDependencies {
  return {
    isGzipSupported,
    getSettings,
    listSnapshots,
    loadBlobIntoStore: loadSnapshotIntoWorkingCopy,
    recoverSendState,
  };
}

type Phase =
  | { kind: "loading" }
  | { kind: "gzip-unsupported" }
  | { kind: "device-unreachable"; stage: StartupLoadStage; message: string }
  | { kind: "invalid-settings"; message: string }
  | { kind: "load-failed"; stage: StartupLoadStage; message: string }
  | {
      kind: "first-package";
      origin: "no-blobs" | "empty-repository";
      settings: SettingsResponse;
      send: SendStatusResponse | null;
    }
  | {
      kind: "package-chooser";
      store: RepositoryWorkingCopyStore;
      settings: SettingsResponse;
      send: SendStatusResponse | null;
      blobId: string;
    }
  | {
      kind: "snapshot-recovery";
      blobId: string;
      message: string;
      otherBlobs: readonly BlobSummary[];
      settings: SettingsResponse;
    };

function loadFailedTitle(stage: StartupLoadStage): string {
  switch (stage) {
    case "blobs":
      return "Couldn't list stored snapshots";
    case "newest-blob":
      return "Couldn't load the newest snapshot";
    case "settings":
      return "Couldn't load device settings";
  }
}

interface RetryScreenProps {
  title: string;
  message: string;
  onRetry: () => void;
}

function RetryScreen({
  title,
  message,
  onRetry,
}: RetryScreenProps): React.JSX.Element {
  return (
    <main className="standalone">
      <h1>{title}</h1>
      <ErrorBanner message={message} />
      <button className="primary" onClick={onRetry} type="button">
        Retry
      </button>
    </main>
  );
}

interface FirstPackageFormProps {
  origin: "no-blobs" | "empty-repository";
  settings: SettingsResponse;
  send: SendStatusResponse | null;
  onReady: (ready: RepositoryStartupReady) => void;
}

/**
 * TODO_V2 V2-083 — "Create Your First Repository" (no blobs at all) and
 * "Create your first package" (a loaded, valid, but empty repository)
 * share this same form: both need exactly one thing, the first package's
 * name, per UI_UX_SPEC_V2 §8's normal sequence. Package validity is
 * delegated entirely to {@link validateRepositoryForUse} (V2-071) rather
 * than duplicating its name-length/character rules here.
 */
function FirstPackageForm({
  origin,
  settings,
  send,
  onReady,
}: FirstPackageFormProps): React.JSX.Element {
  const [name, setName] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const packageIdRef = useRef<string>(crypto.randomUUID());

  const trimmedName = name.trim();
  const candidateRepository: Repository = {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [{ id: packageIdRef.current, name: trimmedName, macros: [] }],
  };
  const validation = validateRepositoryForUse(candidateRepository);

  const submit = async (): Promise<void> => {
    if (!validation.ok) {
      return;
    }
    setSubmitting(true);
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    // A content change (the new package) always dirties the working copy
    // (SPEC_V2 §8.6) — the repository exists only in this tab until Save
    // snapshot succeeds (UI_UX_SPEC_V2 §8).
    store.applyContentChange(validation.value);
    const persistence = await tryPersistSelectedPackageId(
      packageIdRef.current,
      settings.lastSelectedPackageId,
    );
    // This form always builds a brand-new local store (see `submit` above),
    // discarding whatever blob (if any) was loaded to reach this screen — so
    // the resulting working copy does not correspond to any stored blob yet.
    onReady({
      store,
      packageId: packageIdRef.current,
      send,
      loadedBlobId: null,
      ...(persistence.kind === "failed"
        ? { selectionPersistenceFailure: persistence.failure }
        : {}),
    });
  };

  return (
    <main className="standalone">
      <h1>
        {origin === "no-blobs"
          ? "Create Your First Repository"
          : "Create your first package"}
      </h1>
      {origin === "no-blobs" ? (
        <p>This device has no saved repository snapshots yet.</p>
      ) : (
        <p>The loaded repository does not contain any packages yet.</p>
      )}
      <p>
        The repository exists only in this browser tab until{" "}
        <strong>Save snapshot</strong> succeeds.
      </p>
      <form
        className="form-stack"
        onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
          event.preventDefault();
          void submit();
        }}
      >
        <label htmlFor="first-package-name">Package name</label>
        <input
          autoFocus
          disabled={submitting}
          id="first-package-name"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setName(event.target.value);
          }}
          required
          value={name}
        />
        <span className="field-help">
          1 through {v2Limits.packageNameMaxBytes} UTF-8 bytes.
        </span>
        <button
          className="primary"
          disabled={!validation.ok || submitting}
          type="submit"
        >
          {submitting ? "Creating…" : "Create package"}
        </button>
      </form>
      <p role="status">Unsaved changes</p>
    </main>
  );
}

interface PackageChooserViewProps {
  store: RepositoryWorkingCopyStore;
  settings: SettingsResponse;
  send: SendStatusResponse | null;
  loadedBlobId: string;
  onReady: (ready: RepositoryStartupReady) => void;
}

/**
 * UI_UX_SPEC_V2 §3.4/§3.6 — shown when several packages exist and none
 * resolve from `lastSelectedPackageId` (TODO_V2 V2-084 "missing or invalid
 * selected package"). This is a minimal, functional chooser; Phase 9 owns
 * the full Package Chooser surface.
 */
function PackageChooserView({
  store,
  settings,
  send,
  loadedBlobId,
  onReady,
}: PackageChooserViewProps): React.JSX.Element {
  const [busyId, setBusyId] = useState<string | null>(null);
  const repository = store.getRepository();

  const choose = async (packageId: string): Promise<void> => {
    setBusyId(packageId);
    const persistence = await tryPersistSelectedPackageId(
      packageId,
      settings.lastSelectedPackageId,
    );
    onReady({
      store,
      packageId,
      send,
      loadedBlobId,
      ...(persistence.kind === "failed"
        ? { selectionPersistenceFailure: persistence.failure }
        : {}),
    });
  };

  return (
    <main className="standalone">
      <h1>Choose a package</h1>
      <p>Select which package to open.</p>
      <ul className="form-stack">
        {repository.packages.map((pkg) => (
          <li key={pkg.id}>
            <button
              disabled={busyId !== null}
              onClick={() => {
                void choose(pkg.id);
              }}
              type="button"
            >
              {busyId === pkg.id
                ? "Opening…"
                : `${pkg.name} (${String(pkg.macros.length)} macros)`}
            </button>
          </li>
        ))}
      </ul>
    </main>
  );
}

interface SnapshotRecoveryViewProps {
  initialBlobId: string;
  initialMessage: string;
  initialOtherBlobs: readonly BlobSummary[];
  settings: SettingsResponse;
  deps: StartupDependencies;
  onReady: (ready: RepositoryStartupReady) => void;
  onRestart: () => void;
}

type RecoveryResolution =
  | {
      kind: "first-package";
      settings: SettingsResponse;
      send: SendStatusResponse | null;
    }
  | {
      kind: "package-chooser";
      store: RepositoryWorkingCopyStore;
      settings: SettingsResponse;
      send: SendStatusResponse | null;
      blobId: string;
    };

function invalidLoadMessage(result: LoadSnapshotResult): string {
  if (result.ok) {
    throw new Error("invalidLoadMessage called with a successful result.");
  }
  switch (result.reason) {
    case "gzip_unsupported":
      return "This browser does not support the compression APIs required to read this snapshot.";
    case "unreadable":
      return result.message;
    case "invalid":
      return "This snapshot does not contain a valid repository.";
  }
}

/**
 * TODO_V2 V2-084 — "unreadable or invalid newest snapshot" recovery.
 * SPEC_V2 §14.1/UI_UX_SPEC_V2 §3.4 forbid silently loading an older blob;
 * this view requires the user to explicitly pick one, and never deletes or
 * replaces the failed blob (SPEC_V2 §9's decompression-error handling).
 * The full Snapshot Chooser/Management surface belongs to Phase 9 — this
 * is the minimal functional recovery this phase needs.
 */
function SnapshotRecoveryView({
  initialBlobId,
  initialMessage,
  initialOtherBlobs,
  settings,
  deps,
  onReady,
  onRestart,
}: SnapshotRecoveryViewProps): React.JSX.Element {
  const [message, setMessage] = useState(initialMessage);
  const [candidates, setCandidates] = useState(initialOtherBlobs);
  const [busyId, setBusyId] = useState<string | null>(null);
  const [resolution, setResolution] = useState<RecoveryResolution | null>(null);

  if (resolution?.kind === "first-package") {
    return (
      <FirstPackageForm
        onReady={onReady}
        origin="empty-repository"
        send={resolution.send}
        settings={resolution.settings}
      />
    );
  }
  if (resolution?.kind === "package-chooser") {
    return (
      <PackageChooserView
        loadedBlobId={resolution.blobId}
        onReady={onReady}
        send={resolution.send}
        settings={resolution.settings}
        store={resolution.store}
      />
    );
  }

  const attempt = async (blob: BlobSummary): Promise<void> => {
    setBusyId(blob.id);
    const store = createRepositoryWorkingCopyStore(createEmptyRepository());
    let loaded: LoadSnapshotResult;
    try {
      loaded = await deps.loadBlobIntoStore(blob.id, store);
    } catch (error: unknown) {
      setMessage(v2ErrorText(error));
      setCandidates((current) => current.filter((item) => item.id !== blob.id));
      setBusyId(null);
      return;
    }
    if (!loaded.ok) {
      setMessage(invalidLoadMessage(loaded));
      setCandidates((current) => current.filter((item) => item.id !== blob.id));
      setBusyId(null);
      return;
    }

    let send: SendStatusResponse | null = null;
    try {
      send = await deps.recoverSendState();
    } catch {
      send = null;
    }

    const destination = resolvePackageDestination(
      loaded.repository,
      settings.lastSelectedPackageId,
    );
    if (destination.kind === "first-package") {
      setResolution({ kind: "first-package", settings, send });
      return;
    }
    if (destination.kind === "package-chooser") {
      setResolution({
        kind: "package-chooser",
        store,
        settings,
        send,
        blobId: blob.id,
      });
      return;
    }
    const persistence = destination.shouldPersist
      ? await tryPersistSelectedPackageId(
          destination.packageId,
          settings.lastSelectedPackageId,
        )
      : { kind: "persisted" as const };
    onReady({
      store,
      packageId: destination.packageId,
      send,
      loadedBlobId: blob.id,
      ...(persistence.kind === "failed"
        ? { selectionPersistenceFailure: persistence.failure }
        : {}),
    });
  };

  return (
    <main className="standalone">
      <h1>Snapshot recovery</h1>
      <p>Blob {initialBlobId} could not be opened automatically.</p>
      <ErrorBanner message={message} />
      {candidates.length === 0 ? (
        <p>No other stored snapshots are available to try.</p>
      ) : (
        <ul className="form-stack">
          {candidates.map((blob) => (
            <li key={blob.id}>
              <button
                disabled={busyId !== null}
                onClick={() => {
                  void attempt(blob);
                }}
                type="button"
              >
                {busyId === blob.id
                  ? "Loading…"
                  : `Load snapshot ${blob.id} (${String(blob.sizeBytes)} bytes)`}
              </button>
            </li>
          ))}
        </ul>
      )}
      <button onClick={onRestart} type="button">
        Start over
      </button>
    </main>
  );
}

export function RepositoryStartupScreen({
  existing,
  onReady,
  dependencies,
}: RepositoryStartupScreenProps): React.JSX.Element {
  const deps = dependencies ?? defaultDependencies();
  const depsRef = useRef(deps);
  depsRef.current = deps;

  const [phase, setPhase] = useState<Phase>({ kind: "loading" });
  const [attempt, setAttempt] = useState(0);

  const onReadyRef = useRef(onReady);
  onReadyRef.current = onReady;

  const restart = (): void => {
    setAttempt((value) => value + 1);
  };

  useEffect(() => {
    if (existing !== null) {
      onReadyRef.current(existing);
      return;
    }

    let active = true;
    setPhase({ kind: "loading" });

    const settle = async (
      result: Extract<StartupResult, { kind: "ready" }>,
    ): Promise<void> => {
      if (result.destination.kind === "first-package") {
        if (active) {
          setPhase({
            kind: "first-package",
            origin: "empty-repository",
            settings: result.settings,
            send: result.send,
          });
        }
        return;
      }
      if (result.destination.kind === "package-chooser") {
        if (active) {
          setPhase({
            kind: "package-chooser",
            store: result.store,
            settings: result.settings,
            send: result.send,
            blobId: result.blobId,
          });
        }
        return;
      }
      const persistence = result.destination.shouldPersist
        ? await tryPersistSelectedPackageId(
            result.destination.packageId,
            result.settings.lastSelectedPackageId,
          )
        : { kind: "persisted" as const };
      if (active) {
        onReadyRef.current({
          store: result.store,
          packageId: result.destination.packageId,
          send: result.send,
          loadedBlobId: result.blobId,
          ...(persistence.kind === "failed"
            ? { selectionPersistenceFailure: persistence.failure }
            : {}),
        });
      }
    };

    const run = async (): Promise<void> => {
      const result = await runStartup(depsRef.current);
      if (!active) {
        return;
      }
      switch (result.kind) {
        case "gzip-unsupported":
          setPhase({ kind: "gzip-unsupported" });
          return;
        case "device-unreachable":
          setPhase({
            kind: "device-unreachable",
            stage: result.stage,
            message: result.message,
          });
          return;
        case "invalid-settings":
          setPhase({ kind: "invalid-settings", message: result.message });
          return;
        case "load-failed":
          setPhase({
            kind: "load-failed",
            stage: result.stage,
            message: result.message,
          });
          return;
        case "first-repository":
          setPhase({
            kind: "first-package",
            origin: "no-blobs",
            settings: result.settings,
            send: null,
          });
          return;
        case "invalid-newest-snapshot":
          setPhase({
            kind: "snapshot-recovery",
            blobId: result.blobId,
            message: result.message,
            otherBlobs: result.otherBlobs,
            settings: result.settings,
          });
          return;
        case "ready":
          void settle(result);
      }
    };

    void run();
    return () => {
      active = false;
    };
  }, [existing, attempt]);

  switch (phase.kind) {
    case "loading":
      return (
        <main aria-busy="true" className="standalone">
          <p role="status">Opening your repository…</p>
          <p>Loading the newest snapshot from the device.</p>
        </main>
      );
    case "gzip-unsupported":
      return (
        <main className="standalone">
          <h1>Browser not supported</h1>
          <p>
            This browser does not support the compression features this
            application requires to read or save a repository. Update to a
            recent version of a browser that supports{" "}
            <code>CompressionStream</code> and <code>DecompressionStream</code>,
            then reload.
          </p>
        </main>
      );
    case "device-unreachable":
      return (
        <RetryScreen
          message={phase.message}
          onRetry={restart}
          title="Device unreachable"
        />
      );
    case "invalid-settings":
      return (
        <RetryScreen
          message={phase.message}
          onRetry={restart}
          title="Couldn't load device settings"
        />
      );
    case "load-failed":
      return (
        <RetryScreen
          message={phase.message}
          onRetry={restart}
          title={loadFailedTitle(phase.stage)}
        />
      );
    case "first-package":
      return (
        <FirstPackageForm
          onReady={onReady}
          origin={phase.origin}
          send={phase.send}
          settings={phase.settings}
        />
      );
    case "package-chooser":
      return (
        <PackageChooserView
          loadedBlobId={phase.blobId}
          onReady={onReady}
          send={phase.send}
          settings={phase.settings}
          store={phase.store}
        />
      );
    case "snapshot-recovery":
      return (
        <SnapshotRecoveryView
          deps={deps}
          initialBlobId={phase.blobId}
          initialMessage={phase.message}
          initialOtherBlobs={phase.otherBlobs}
          onReady={onReady}
          onRestart={restart}
          settings={phase.settings}
        />
      );
  }
}
