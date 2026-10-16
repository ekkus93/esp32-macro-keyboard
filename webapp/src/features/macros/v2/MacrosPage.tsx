import { useCallback, useEffect, useRef, useState } from "react";
import { Button } from "../../../components/m3/Button";
import { Icon } from "../../../components/m3/Icon";
import { DeterminateProgress } from "../../../components/m3/Progress";
import { SendStatus } from "../../../components/SendStatus";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import type { ActiveSendSummary } from "../../shell/v2/activeSendSummary";
import { V2ApiError } from "../../../v2/apiClient";
import type { RepositoryMacro } from "../../../v2/repository";
import {
  deleteMacro,
  duplicateMacro,
  moveMacro as moveMacroInRepository,
  moveMacroToIndex,
} from "../../../v2/repositoryEditing";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";
import { DismissibleBanner } from "./DismissibleBanner";
import { MacroRow } from "./MacroRow";
import { UsbNotice } from "./UsbNotice";
import {
  activeStatusParts,
  activeStatusText,
  completionAckMs,
  terminalIssueReason,
  terminalIssueText,
} from "./macroSendStatus";
import type {
  MacroIdentity,
  MoveAction,
  SendLifecycle,
} from "./macroSendStatus";
import {
  cancelSend as defaultCancelSend,
  isTerminalSendState,
  recoverSendState as defaultRecoverSendState,
  sendMacro as defaultSendMacro,
  trackSend as defaultTrackSend,
} from "../../../v2/sendClient";
import type { SendMacroHandle } from "../../../v2/sendClient";
import type { SendStatusResponse, UsbState } from "../../../v2/apiTypes";

/**
 * The Macros page and its Quick Send operating console, per UI_UX_SPEC_V2
 * §5 (TODO_V2 V2-090 through V2-095). This is the default destination for a
 * resolved package and the most frequently used screen — ordinary sends
 * never leave it (V2-093, Phase 9 exit gate).
 */

export interface MacrosPageDependencies {
  sendMacro: typeof defaultSendMacro;
  trackSend: typeof defaultTrackSend;
  cancelSend: typeof defaultCancelSend;
  recoverSendState: typeof defaultRecoverSendState;
}

function defaultDependencies(): MacrosPageDependencies {
  return {
    sendMacro: defaultSendMacro,
    trackSend: defaultTrackSend,
    cancelSend: defaultCancelSend,
    recoverSendState: defaultRecoverSendState,
  };
}

export interface MacrosPageProps {
  store: RepositoryWorkingCopyStore;
  packageId: string;
  usbState: UsbState;
  /** The send recovered during startup (UI_UX_SPEC_V2 §3.4 step 8), if any. */
  initialSend: SendStatusResponse | null;
  /**
   * Reports the current active-send summary (macro name, progress, Cancel)
   * for the phone-landscape orientation surface (TODO_V2 V2-132,
   * UI_UX_SPEC_V2 §12.3), or `null` while no send is starting, awaiting
   * confirmation, or running. Optional so existing tests and call sites that
   * do not care about the orientation surface need not pass it.
   */
  onActiveSendChange?: (summary: ActiveSendSummary | null) => void;
  onChangePackage: () => void;
  onOpenAddMacro: () => void;
  onOpenEditMacro: (macroId: string) => void;
  /** Test-only dependency injection; defaults to the real v2 send client. */
  dependencies?: MacrosPageDependencies;
}

/**
 * The in-page status for a send that is running or waiting for confirmation.
 * The sentence naming the state is in the live region (announced when it
 * changes); the per-poll action counter and the bar are outside it, so a send
 * that polls once a second does not talk once a second.
 */
function ActiveSendStatus({
  lifecycle,
  cancelRequested,
  onCancel,
}: {
  lifecycle: Extract<SendLifecycle, { kind: "active" }>;
  cancelRequested: boolean;
  onCancel: () => void;
}): React.JSX.Element {
  const parts = activeStatusParts(lifecycle.status, lifecycle.macro);
  const name = lifecycle.macro === null ? "" : ` ${lifecycle.macro.name}`;
  return (
    <SendStatus role="status">
      <p>{cancelRequested ? `Cancelling${name}…` : parts.text}</p>
      {parts.progress === null ? null : (
        <>
          <DeterminateProgress
            label="Send progress"
            max={lifecycle.status.actionCount}
            value={lifecycle.status.actionIndex}
          />
          <p aria-live="off" className="mt-2 mb-0 text-body-md font-normal">
            {/* A leading space (collapsed on screen) keeps the status
                sentence and the counter reading as one string in text
                extraction, as they did before the counter left the live
                region. */}{" "}
            {parts.progress}
          </p>
        </>
      )}
      <button disabled={cancelRequested} onClick={onCancel} type="button">
        {cancelRequested ? "Cancelling…" : "Cancel and release all keys"}
      </button>
    </SendStatus>
  );
}

export function MacrosPage({
  store,
  packageId,
  usbState,
  initialSend,
  onActiveSendChange,
  onChangePackage,
  onOpenAddMacro,
  onOpenEditMacro,
  dependencies,
}: MacrosPageProps): React.JSX.Element {
  const deps = dependencies ?? defaultDependencies();
  const depsRef = useRef(deps);
  depsRef.current = deps;

  const [snapshot, setSnapshot] = useState(() => store.getSnapshot());
  useEffect(() => store.subscribe(setSnapshot), [store]);

  const [lifecycle, setLifecycle] = useState<SendLifecycle>({ kind: "idle" });
  const [releaseError, setReleaseError] = useState<string | null>(null);
  const [startError, setStartError] = useState<string | null>(null);
  // Set from the moment Cancel is pressed until the send reaches a terminal
  // state, so the control cannot be pressed twice and the page says what is
  // happening while the device finishes releasing keys.
  const [cancelRequested, setCancelRequested] = useState(false);
  const [moveAnnouncement, setMoveAnnouncement] = useState<string | null>(null);

  // Guards double-send on rapid taps (V2-095): set synchronously, before any
  // `await`, so a second click handler invoked before React re-renders still
  // observes it.
  const startingRef = useRef(false);
  // Guards duplicate completion handling (V2-095) across the several ways a
  // terminal status can be observed (a send this tab started, a reload
  // recovery, and a 409-triggered recovery).
  const completedSendIdsRef = useRef<Set<string>>(new Set());
  const activeHandleRef = useRef<{ stop: () => void } | null>(null);
  const mountedRef = useRef(true);

  // Stable across renders (only closes over refs and React's stable setState
  // functions), so a `useEffect` can list them as dependencies without
  // re-running on every render (they only ever change identity here on the
  // initial mount, keeping the effect below's dependency array honest rather
  // than suppressed).
  const stopActiveTracking = useCallback((): void => {
    activeHandleRef.current?.stop();
    activeHandleRef.current = null;
  }, []);

  // Every send-tracking path stores its handle in `activeHandleRef`. Stop that
  // handle when this page unmounts so polling cannot outlive the component.
  // `mountedRef` also closes the async race where `sendMacro()` or recovery
  // resolves after this cleanup has already run.
  useEffect(() => {
    mountedRef.current = true;
    return () => {
      mountedRef.current = false;
      stopActiveTracking();
    };
  }, [stopActiveTracking]);

  const handleStatus = useCallback(
    (macro: MacroIdentity | null, status: SendStatusResponse): void => {
      if (status.releaseError.length > 0) {
        setReleaseError(status.releaseError);
      }
      setLifecycle({ kind: "active", macro, status });
    },
    [],
  );

  const handleTrackingError = useCallback((error: unknown): void => {
    if (!mountedRef.current) {
      return;
    }
    activeHandleRef.current = null;
    setStartError(`Send status tracking failed: ${v2ErrorText(error)}`);
  }, []);

  const handleComplete = useCallback(
    (macro: MacroIdentity | null, status: SendStatusResponse): void => {
      if (completedSendIdsRef.current.has(status.id)) {
        return;
      }
      completedSendIdsRef.current.add(status.id);
      activeHandleRef.current = null;
      if (status.releaseError.length > 0) {
        setReleaseError(status.releaseError);
      }
      if (status.state === "completed") {
        setLifecycle({ kind: "completed", macro });
        window.setTimeout(() => {
          setLifecycle((current) =>
            current.kind === "completed" ? { kind: "idle" } : current,
          );
        }, completionAckMs);
        return;
      }
      const reason = terminalIssueReason(status.state);
      if (reason === null) {
        // Unreachable in practice: sendClient only calls onComplete after
        // isTerminalSendState(status.state) is true, and "completed" is
        // handled above. Fall back to idle rather than mislabeling a
        // terminal-issue banner.
        setLifecycle({ kind: "idle" });
        return;
      }
      setLifecycle({ kind: "terminal-issue", reason, macro, status });
    },
    [],
  );

  const recoverActiveSend = useCallback(async (): Promise<void> => {
    let current: SendStatusResponse | null;
    try {
      current = await depsRef.current.recoverSendState();
    } catch (error: unknown) {
      if (mountedRef.current) {
        setLifecycle({ kind: "idle" });
        setStartError(v2ErrorText(error));
      }
      return;
    }
    if (!mountedRef.current) {
      return;
    }
    if (current === null || isTerminalSendState(current.state)) {
      setLifecycle({ kind: "idle" });
      setStartError(
        "Another send finished just before this one could start. Try again.",
      );
      return;
    }
    setLifecycle({ kind: "active", macro: null, status: current });
    activeHandleRef.current = depsRef.current.trackSend(current, {
      onStatus: (status) => {
        handleStatus(null, status);
      },
      onComplete: (status) => {
        handleComplete(null, status);
      },
      onError: handleTrackingError,
    });
  }, [handleComplete, handleStatus, handleTrackingError]);

  // Reload recovery (V2-095, UI_UX_SPEC_V2 §5.5): resume tracking a
  // non-terminal recovered send, or restore an undismissed terminal-issue
  // banner. A recovered send carries no macro identity (the wire protocol
  // does not track one), so its banner never names a macro. `initialSend` is
  // set once by the caller for the component's lifetime, so in practice this
  // runs once, but the dependency array reflects that honestly instead of
  // asserting it.
  useEffect(() => {
    if (initialSend === null) {
      return;
    }
    if (completedSendIdsRef.current.has(initialSend.id)) {
      return;
    }
    if (isTerminalSendState(initialSend.state)) {
      completedSendIdsRef.current.add(initialSend.id);
      if (initialSend.releaseError.length > 0) {
        setReleaseError(initialSend.releaseError);
      }
      const reason = terminalIssueReason(initialSend.state);
      if (reason !== null) {
        setLifecycle({
          kind: "terminal-issue",
          reason,
          macro: null,
          status: initialSend,
        });
      }
      return;
    }
    setLifecycle({ kind: "active", macro: null, status: initialSend });
    activeHandleRef.current = depsRef.current.trackSend(initialSend, {
      onStatus: (status) => {
        handleStatus(null, status);
      },
      onComplete: (status) => {
        handleComplete(null, status);
      },
      onError: handleTrackingError,
    });
    return stopActiveTracking;
  }, [
    handleComplete,
    handleStatus,
    handleTrackingError,
    initialSend,
    stopActiveTracking,
  ]);

  const startSend = async (macro: RepositoryMacro): Promise<void> => {
    // A persistent cancelled/failed/timed-out banner does not block a new
    // send — UI_UX_SPEC_V2 §5.4 says it lasts "until dismissed or another
    // send begins" — starting one here implicitly clears it, matched by
    // `setLifecycle` below unconditionally replacing the previous state.
    const canStart =
      lifecycle.kind === "idle" || lifecycle.kind === "terminal-issue";
    if (startingRef.current || !canStart) {
      return;
    }
    startingRef.current = true;
    setStartError(null);
    const identity: MacroIdentity = { id: macro.id, name: macro.name };
    setLifecycle({ kind: "starting", macro: identity });
    try {
      const handle: SendMacroHandle = await depsRef.current.sendMacro(
        {
          source: macro.source,
          keyPressMs: macro.keyPressMs,
          interKeyMs: macro.interKeyMs,
        },
        {
          onStatus: (status) => {
            handleStatus(identity, status);
          },
          onComplete: (status) => {
            handleComplete(identity, status);
          },
          onError: handleTrackingError,
        },
      );
      if (!mountedRef.current) {
        handle.stop();
        return;
      }
      activeHandleRef.current = handle;
      setLifecycle({
        kind: "active",
        macro: identity,
        status: {
          id: handle.accepted.id,
          state: handle.accepted.state,
          actionIndex: 0,
          actionCount: handle.accepted.actionCount,
          estimatedDurationMs: handle.accepted.estimatedDurationMs,
          cancellationRequested: false,
          error: "",
          releaseError: "",
        },
      });
    } catch (error: unknown) {
      if (!mountedRef.current) {
        return;
      }
      if (error instanceof V2ApiError && error.status === 409) {
        await recoverActiveSend();
      } else {
        setLifecycle({ kind: "idle" });
        setStartError(v2ErrorText(error));
      }
    } finally {
      startingRef.current = false;
    }
  };

  // Stable across renders (closes only over `depsRef`, a ref, and React's
  // stable `setStartError` setter) so the active-send-summary effect below
  // can list it as a dependency without re-running every render.
  const cancelActiveSend = useCallback(async (): Promise<void> => {
    setCancelRequested(true);
    try {
      await depsRef.current.cancelSend();
    } catch (error: unknown) {
      setCancelRequested(false);
      setStartError(v2ErrorText(error));
    }
  }, []);

  // A cancel request only means something while a send is active.
  useEffect(() => {
    if (lifecycle.kind !== "active") {
      setCancelRequested(false);
    }
  }, [lifecycle.kind]);

  // TODO_V2 V2-132 / UI_UX_SPEC_V2 §12.3: reports the active-send summary
  // upward whenever it changes so the phone-landscape orientation surface
  // can keep macro name, progress, and Cancel and release all keys
  // accessible while this page's ordinary content is hidden behind it. Runs
  // as an effect (not inline during render) because it calls a parent
  // setState — `onActiveSendChange` is expected to be a stable setter, as
  // every current caller provides.
  useEffect(() => {
    if (onActiveSendChange === undefined) {
      return;
    }
    if (lifecycle.kind === "starting") {
      onActiveSendChange({
        macroName: lifecycle.macro.name,
        statusText: `Sending ${lifecycle.macro.name}…`,
        onCancel: null,
      });
      return;
    }
    if (lifecycle.kind === "active") {
      onActiveSendChange({
        macroName: lifecycle.macro?.name ?? null,
        statusText: cancelRequested
          ? `Cancelling${lifecycle.macro === null ? "" : ` ${lifecycle.macro.name}`}…`
          : activeStatusText(lifecycle.status, lifecycle.macro),
        onCancel: cancelRequested
          ? null
          : () => {
              void cancelActiveSend();
            },
      });
      return;
    }
    onActiveSendChange(null);
  }, [cancelActiveSend, cancelRequested, lifecycle, onActiveSendChange]);

  const repository = snapshot.repository;
  const activePackage = repository.packages.find((pkg) => pkg.id === packageId);

  // TODO_V2 V2-133/UI_UX_SPEC_V2 §14: "Move first"/"Move last" alongside
  // "Move up"/"Move down" — a keyboard-operable alternative that does not
  // require repeatedly activating an adjacent-swap control to reach either
  // end of the list.
  const moveMacro = (index: number, action: MoveAction): void => {
    if (activePackage === undefined) {
      return;
    }
    const moved = activePackage.macros[index];
    if (moved === undefined) {
      return;
    }
    const lastIndex = activePackage.macros.length - 1;
    const target =
      action === "first"
        ? 0
        : action === "last"
          ? lastIndex
          : action === "up"
            ? index - 1
            : index + 1;
    if (target < 0 || target > lastIndex || target === index) {
      return;
    }
    const nextRepository =
      action === "up" || action === "down"
        ? moveMacroInRepository(
            repository,
            activePackage.id,
            index,
            action === "up" ? -1 : 1,
          )
        : moveMacroToIndex(repository, activePackage.id, index, target);
    store.applyContentChange(nextRepository);
    setMoveAnnouncement(
      `Moved ${moved.name} to position ${String(target + 1)}.`,
    );
  };

  // V2-101 — duplicate/delete always change repository content, so both
  // unconditionally dirty the working copy (no no-op case exists: a
  // duplicate always adds an item and a delete only runs against a macro
  // that is confirmed present).
  const duplicateMacroRow = (macroId: string): void => {
    const result = duplicateMacro(repository, macroId);
    if (result !== null) {
      store.applyContentChange(result.repository);
    }
  };

  const deleteMacroRow = (macroId: string): void => {
    store.applyContentChange(deleteMacro(repository, macroId));
  };

  if (activePackage === undefined) {
    return (
      <section aria-labelledby="macros-title">
        <h2 id="macros-title">Macros</h2>
        <p role="alert">
          The selected package is no longer in this repository. Choose another
          package.
        </p>
        <button onClick={onChangePackage} type="button">
          Change package
        </button>
      </section>
    );
  }

  // UI_UX_SPEC_V2 §5.4: the completion acknowledgement blocks a new send
  // until it restores the ordinary Send control ("then restore the ordinary
  // Send control"), but a persistent cancelled/failed/timed-out banner does
  // not — it explicitly lasts "until dismissed OR another send begins".
  const sendActive =
    lifecycle.kind === "starting" ||
    lifecycle.kind === "active" ||
    lifecycle.kind === "completed";
  const sendDisabled = usbState !== "ready" || sendActive;

  return (
    <section aria-labelledby="macros-title">
      {/* The package this list belongs to, how many macros it holds, and the
          two actions that follow from it. The app bar's title is "Macros", so
          the package name appears once, here. */}
      <div className="mb-4 flex flex-wrap items-center gap-x-3 gap-y-2">
        <div className="min-w-[10rem] basis-full min-[34rem]:flex-1">
          <h2
            className="m-0 overflow-hidden text-title-lg text-ellipsis whitespace-nowrap"
            id="macros-title"
          >
            {activePackage.name}
          </h2>
          <p className="m-0 text-body-md text-on-surface-variant">
            {String(activePackage.macros.length)} macros
          </p>
        </div>
        <Button onClick={onChangePackage} variant="text">
          Change package
        </Button>
        <Button icon="add" onClick={onOpenAddMacro} variant="filled">
          Add macro
        </Button>
      </div>

      {/* The in-flight status stays at the top of the scrolling region while
          a long macro list scrolls beneath it, so Cancel is never more than
          one glance away (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §5.2). */}
      {lifecycle.kind === "starting" || lifecycle.kind === "active" ? (
        <div className="sticky top-0 z-10 -mx-1 bg-surface px-1">
          {lifecycle.kind === "starting" ? (
            <SendStatus role="status">
              <p>Sending {lifecycle.macro.name}…</p>
            </SendStatus>
          ) : null}
          {lifecycle.kind === "active" ? (
            <ActiveSendStatus
              cancelRequested={cancelRequested}
              lifecycle={lifecycle}
              onCancel={() => {
                void cancelActiveSend();
              }}
            />
          ) : null}
        </div>
      ) : null}
      {lifecycle.kind === "completed" ? (
        <SendStatus role="status">
          <p>
            Sent{lifecycle.macro !== null ? ` ${lifecycle.macro.name}` : ""}.
          </p>
        </SendStatus>
      ) : null}
      {lifecycle.kind === "terminal-issue" ? (
        <DismissibleBanner
          message={terminalIssueText(
            lifecycle.reason,
            lifecycle.macro,
            lifecycle.status,
          )}
          onDismiss={() => {
            setLifecycle({ kind: "idle" });
          }}
          role="alert"
        />
      ) : null}
      {releaseError !== null ? (
        <DismissibleBanner
          message={`Key release failed: ${releaseError}`}
          onDismiss={() => {
            setReleaseError(null);
          }}
          role="alert"
        />
      ) : null}
      {startError !== null ? (
        <DismissibleBanner
          message={startError}
          onDismiss={() => {
            setStartError(null);
          }}
          role="alert"
        />
      ) : null}
      <p role="status">{moveAnnouncement}</p>
      <UsbNotice usbState={usbState} />

      {activePackage.macros.length === 0 ? (
        <div
          className="grid justify-items-start gap-3 rounded-shape-lg bg-surface-container p-6"
          role="status"
        >
          <h3 className="m-0">No macros yet</h3>
          <p className="m-0 text-on-surface-variant">
            Add a macro to type it from your keyboard.
          </p>
          <button className="primary" onClick={onOpenAddMacro} type="button">
            <Icon name="add" size={18} />
            Add macro
          </button>
        </div>
      ) : (
        <div aria-label="Macro list" className="grid gap-3">
          {activePackage.macros.map((macro, index) => (
            <MacroRow
              index={index}
              key={macro.id}
              macro={macro}
              macroCount={activePackage.macros.length}
              onDelete={() => {
                deleteMacroRow(macro.id);
              }}
              onDuplicate={() => {
                duplicateMacroRow(macro.id);
              }}
              onEdit={() => {
                onOpenEditMacro(macro.id);
              }}
              onMove={(action) => {
                moveMacro(index, action);
              }}
              onSend={() => {
                void startSend(macro);
              }}
              sendDisabled={sendDisabled}
              sending={
                lifecycle.kind !== "idle" &&
                lifecycle.kind !== "completed" &&
                lifecycle.kind !== "terminal-issue" &&
                lifecycle.macro?.id === macro.id
              }
            />
          ))}
        </div>
      )}
    </section>
  );
}
