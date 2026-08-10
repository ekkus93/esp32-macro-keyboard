import { useCallback, useEffect, useRef, useState } from "react";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import type { ActiveSendSummary } from "../../shell/v2/activeSendSummary";
import { V2ApiError } from "../../../v2/apiClient";
import type { RepositoryMacro } from "../../../v2/repository";
import {
  deleteMacro,
  duplicateMacro,
  moveMacro as moveMacroInRepository,
} from "../../../v2/repositoryEditing";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";
import {
  cancelSend as defaultCancelSend,
  isTerminalSendState,
  recoverSendState as defaultRecoverSendState,
  sendMacro as defaultSendMacro,
  trackSend as defaultTrackSend,
} from "../../../v2/sendClient";
import type { SendMacroHandle } from "../../../v2/sendClient";
import type {
  SendMode,
  SendState,
  SendStatusResponse,
  UsbState,
} from "../../../v2/apiTypes";

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
  sendMode: SendMode;
  showMacroSourcePreviews: boolean;
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
  onOpenPreview: (macroId: string) => void;
  onOpenAddMacro: () => void;
  onOpenEditMacro: (macroId: string) => void;
  /** Test-only dependency injection; defaults to the real v2 send client. */
  dependencies?: MacrosPageDependencies;
}

interface MacroIdentity {
  id: string;
  name: string;
}

type SendLifecycle =
  | { kind: "idle" }
  | { kind: "starting"; macro: MacroIdentity }
  | { kind: "active"; macro: MacroIdentity | null; status: SendStatusResponse }
  | { kind: "completed"; macro: MacroIdentity | null }
  | {
      kind: "terminal-issue";
      reason: "cancelled" | "failed" | "timed_out";
      macro: MacroIdentity | null;
      status: SendStatusResponse;
    };

/** UI_UX_SPEC_V2 §5.5: "an acknowledgement for approximately three to five seconds." */
const completionAckMs = 4000;

/**
 * Narrows a terminal, non-"completed" {@link SendState} to the reason a
 * {@link SendLifecycle} "terminal-issue" banner names — `isTerminalSendState`
 * is a boolean check, not a type predicate, so callers that already know
 * they hold a terminal status still need this to get a properly narrowed
 * type rather than asserting one.
 */
function terminalIssueReason(
  state: SendState,
): "cancelled" | "failed" | "timed_out" | null {
  switch (state) {
    case "cancelled":
    case "failed":
    case "timed_out":
      return state;
    case "completed":
    case "running":
    case "awaiting_confirmation":
      return null;
  }
}

function activeStatusText(
  status: SendStatusResponse,
  macro: MacroIdentity | null,
): string {
  const label = macro !== null ? ` ${macro.name}` : "";
  if (status.state === "awaiting_confirmation") {
    return `Waiting for physical confirmation on the device to send${label}…`;
  }
  return `Sending${label}… action ${String(status.actionIndex)} of ${String(status.actionCount)}`;
}

function terminalIssueText(
  reason: "cancelled" | "failed" | "timed_out",
  macro: MacroIdentity | null,
  status: SendStatusResponse,
): string {
  const label = macro !== null ? ` ${macro.name}` : "";
  switch (reason) {
    case "cancelled":
      return `Send${label} was cancelled.`;
    case "timed_out":
      return `Send${label} timed out.`;
    case "failed":
      return status.error.length > 0
        ? `Send${label} failed: ${status.error}`
        : `Send${label} failed.`;
  }
}

interface DismissibleBannerProps {
  message: string;
  onDismiss: () => void;
  role: "status" | "alert";
  extra?: React.ReactNode;
}

function DismissibleBanner({
  message,
  onDismiss,
  role,
  extra,
}: DismissibleBannerProps): React.JSX.Element {
  return (
    <div aria-live="polite" className="send-status" role={role}>
      <p>{message}</p>
      {extra}
      <button onClick={onDismiss} type="button">
        Dismiss
      </button>
    </div>
  );
}

interface MacroRowProps {
  macro: RepositoryMacro;
  index: number;
  macroCount: number;
  revealed: boolean;
  sendDisabled: boolean;
  sending: boolean;
  onToggleReveal: () => void;
  onSend: () => void;
  onEdit: () => void;
  onPreview: () => void;
  onMove: (direction: -1 | 1) => void;
  onDuplicate: () => void;
  onDelete: () => void;
}

/**
 * Overflow menu (TODO_V2 V2-101, closing a gap Phase 9 flagged rather than
 * faked — UI_UX_SPEC_V2 §5.1 "overflow actions such as Preview and send,
 * Duplicate, Move, and Delete"). Delete asks for an explicit, name-bearing
 * confirmation before it ever touches the working copy, matching the
 * destructive-target identification UI_UX_SPEC_V2 §6.2 requires for package
 * deletion.
 */
function MacroOverflowMenu({
  macro,
  onPreview,
  onDuplicate,
  onDelete,
}: {
  macro: RepositoryMacro;
  onPreview: () => void;
  onDuplicate: () => void;
  onDelete: () => void;
}): React.JSX.Element {
  const [open, setOpen] = useState(false);
  const [confirmingDelete, setConfirmingDelete] = useState(false);

  return (
    <div className="overflow-menu">
      <button
        aria-expanded={open}
        aria-label={`More actions for ${macro.name}`}
        onClick={() => {
          setOpen((current) => !current);
          setConfirmingDelete(false);
        }}
        type="button"
      >
        More actions
      </button>
      {open ? (
        <div
          aria-label={`Actions for ${macro.name}`}
          className="overflow-panel"
        >
          <button
            aria-label={`Preview and send ${macro.name}`}
            onClick={() => {
              setOpen(false);
              onPreview();
            }}
            type="button"
          >
            Preview and send
          </button>
          <button
            aria-label={`Duplicate ${macro.name}`}
            onClick={() => {
              setOpen(false);
              onDuplicate();
            }}
            type="button"
          >
            Duplicate
          </button>
          {confirmingDelete ? (
            <div className="confirmation-panel" role="alertdialog">
              <p>
                Delete <strong>{macro.name}</strong>? This cannot be undone once
                the working copy is saved.
              </p>
              <button
                className="danger"
                onClick={() => {
                  setOpen(false);
                  setConfirmingDelete(false);
                  onDelete();
                }}
                type="button"
              >
                Confirm delete
              </button>
              <button
                onClick={() => {
                  setConfirmingDelete(false);
                }}
                type="button"
              >
                Cancel
              </button>
            </div>
          ) : (
            <button
              aria-label={`Delete ${macro.name}`}
              className="danger"
              onClick={() => {
                setConfirmingDelete(true);
              }}
              type="button"
            >
              Delete
            </button>
          )}
        </div>
      ) : null}
    </div>
  );
}

function MacroRow({
  macro,
  index,
  macroCount,
  revealed,
  sendDisabled,
  sending,
  onToggleReveal,
  onSend,
  onEdit,
  onPreview,
  onMove,
  onDuplicate,
  onDelete,
}: MacroRowProps): React.JSX.Element {
  return (
    <article className="card">
      <div>
        <h3>{macro.name}</h3>
        {revealed ? (
          <p>
            <code>{macro.source}</code>
          </p>
        ) : (
          <p>Source hidden</p>
        )}
        <button
          aria-label={
            revealed
              ? `Hide source for ${macro.name}`
              : `Reveal source for ${macro.name}`
          }
          onClick={onToggleReveal}
          type="button"
        >
          {revealed ? "Hide source" : "Reveal"}
        </button>
      </div>
      <div className="card-actions">
        <button
          aria-label={sending ? `Sending ${macro.name}` : `Send ${macro.name}`}
          className="primary"
          disabled={sendDisabled}
          onClick={onSend}
          type="button"
        >
          {sending ? "Sending…" : "Send"}
        </button>
        <button
          aria-label={`Edit ${macro.name}`}
          onClick={onEdit}
          type="button"
        >
          Edit
        </button>
        <div className="reorder-actions">
          <button
            aria-label={`Move ${macro.name} up`}
            disabled={index === 0}
            onClick={() => {
              onMove(-1);
            }}
            type="button"
          >
            Move up
          </button>
          <button
            aria-label={`Move ${macro.name} down`}
            disabled={index === macroCount - 1}
            onClick={() => {
              onMove(1);
            }}
            type="button"
          >
            Move down
          </button>
        </div>
        <MacroOverflowMenu
          macro={macro}
          onDelete={onDelete}
          onDuplicate={onDuplicate}
          onPreview={onPreview}
        />
      </div>
    </article>
  );
}

export function MacrosPage({
  store,
  packageId,
  usbState,
  sendMode,
  showMacroSourcePreviews,
  initialSend,
  onActiveSendChange,
  onChangePackage,
  onOpenPreview,
  onOpenAddMacro,
  onOpenEditMacro,
  dependencies,
}: MacrosPageProps): React.JSX.Element {
  const deps = dependencies ?? defaultDependencies();
  const depsRef = useRef(deps);
  depsRef.current = deps;

  const [snapshot, setSnapshot] = useState(() => store.getSnapshot());
  useEffect(() => store.subscribe(setSnapshot), [store]);

  const [revealedIds, setRevealedIds] = useState<ReadonlySet<string>>(
    new Set(),
  );
  const [lifecycle, setLifecycle] = useState<SendLifecycle>({ kind: "idle" });
  const [releaseError, setReleaseError] = useState<string | null>(null);
  const [startError, setStartError] = useState<string | null>(null);
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

  // Stable across renders (only closes over refs and React's stable setState
  // functions), so a `useEffect` can list them as dependencies without
  // re-running on every render (they only ever change identity here on the
  // initial mount, keeping the effect below's dependency array honest rather
  // than suppressed).
  const stopActiveTracking = useCallback((): void => {
    activeHandleRef.current?.stop();
    activeHandleRef.current = null;
  }, []);

  const handleStatus = useCallback(
    (macro: MacroIdentity | null, status: SendStatusResponse): void => {
      if (status.releaseError.length > 0) {
        setReleaseError(status.releaseError);
      }
      setLifecycle({ kind: "active", macro, status });
    },
    [],
  );

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
      setLifecycle({ kind: "idle" });
      setStartError(v2ErrorText(error));
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
    });
  }, [handleComplete, handleStatus]);

  // Reload recovery (V2-095, UI_UX_SPEC_V2 §5.6): resume tracking a
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
    });
    return stopActiveTracking;
  }, [handleComplete, handleStatus, initialSend, stopActiveTracking]);

  const startSend = async (macro: RepositoryMacro): Promise<void> => {
    // A persistent cancelled/failed/timed-out banner does not block a new
    // send — UI_UX_SPEC_V2 §5.5 says it lasts "until dismissed or another
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
        },
      );
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
    try {
      await depsRef.current.cancelSend();
    } catch (error: unknown) {
      setStartError(v2ErrorText(error));
    }
  }, []);

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
        statusText: activeStatusText(lifecycle.status, lifecycle.macro),
        onCancel: () => {
          void cancelActiveSend();
        },
      });
      return;
    }
    onActiveSendChange(null);
  }, [cancelActiveSend, lifecycle, onActiveSendChange]);

  const repository = snapshot.repository;
  const activePackage = repository.packages.find((pkg) => pkg.id === packageId);

  const toggleReveal = (macroId: string): void => {
    setRevealedIds((current) => {
      const next = new Set(current);
      if (next.has(macroId)) {
        next.delete(macroId);
      } else {
        next.add(macroId);
      }
      return next;
    });
  };

  const moveMacro = (index: number, direction: -1 | 1): void => {
    if (activePackage === undefined) {
      return;
    }
    const moved = activePackage.macros[index];
    const target = index + direction;
    if (
      moved === undefined ||
      target < 0 ||
      target >= activePackage.macros.length
    ) {
      return;
    }
    store.applyContentChange(
      moveMacroInRepository(repository, activePackage.id, index, direction),
    );
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

  // UI_UX_SPEC_V2 §5.5: the completion acknowledgement blocks a new send
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
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Selected package</p>
          <h2 id="macros-title">{activePackage.name}</h2>
          <p>{String(activePackage.macros.length)} macros</p>
        </div>
        <div className="header-actions">
          <button onClick={onChangePackage} type="button">
            Change
          </button>
          <button className="primary" onClick={onOpenAddMacro} type="button">
            Add macro
          </button>
        </div>
      </div>

      {lifecycle.kind === "starting" ? (
        <div aria-live="polite" className="send-status" role="status">
          <p>Sending {lifecycle.macro.name}…</p>
        </div>
      ) : null}
      {lifecycle.kind === "active" ? (
        <div aria-live="polite" className="send-status" role="status">
          <p>{activeStatusText(lifecycle.status, lifecycle.macro)}</p>
          <button
            onClick={() => {
              void cancelActiveSend();
            }}
            type="button"
          >
            Cancel and release all keys
          </button>
        </div>
      ) : null}
      {lifecycle.kind === "completed" ? (
        <div aria-live="polite" className="send-status" role="status">
          <p>
            Sent{lifecycle.macro !== null ? ` ${lifecycle.macro.name}` : ""}.
          </p>
        </div>
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

      {activePackage.macros.length === 0 ? (
        <p role="status">This package has no macros yet.</p>
      ) : (
        <div aria-label="Macro list">
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
              onMove={(direction) => {
                moveMacro(index, direction);
              }}
              onPreview={() => {
                onOpenPreview(macro.id);
              }}
              onSend={() => {
                // SPEC_V2 §14.5/UI_UX_SPEC_V2 §5.4: with `sendMode: preview`
                // the primary Send control opens Preview and Send first
                // instead of calling `POST /api/v1/send` directly (TODO_V2
                // V2-094 "Honor Always Preview when configured").
                if (sendMode === "preview") {
                  onOpenPreview(macro.id);
                  return;
                }
                void startSend(macro);
              }}
              onToggleReveal={() => {
                toggleReveal(macro.id);
              }}
              revealed={showMacroSourcePreviews || revealedIds.has(macro.id)}
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
      {sendMode === "preview" ? (
        <p>
          Always preview before sending is on — use Preview and send for the
          full preview screen.
        </p>
      ) : null}
    </section>
  );
}
