import { useState } from "react";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import { V2ApiError } from "../../../v2/apiClient";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { StatusBadge } from "../../../components/StatusBadge";
import { compileMacro } from "../../../v2/macroCompiler";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";
import {
  isTerminalSendState,
  recoverSendState as defaultRecoverSendState,
  sendMacro as defaultSendMacro,
} from "../../../v2/sendClient";
import type { SendStatusResponse, UsbState } from "../../../v2/apiTypes";

/**
 * Optional Preview and Send, per UI_UX_SPEC_V2 §5.4 (TODO_V2 V2-094). Reached
 * from a macro row's overflow menu, or as the primary Send destination when
 * the device-wide `sendMode` setting is `preview` (SPEC_V2 §14.5). "Send now"
 * issues the exact same `POST /api/v1/send` helper the Macros page's Quick
 * Send uses (no second send implementation) and then hands tracking off to
 * the Macros page — this screen never itself shows send progress
 * (UI_UX_SPEC_V2 §5.4 "return to the Macros page for progress").
 */

export interface MacroPreviewDependencies {
  sendMacro: typeof defaultSendMacro;
  recoverSendState: typeof defaultRecoverSendState;
}

function defaultDependencies(): MacroPreviewDependencies {
  return {
    sendMacro: defaultSendMacro,
    recoverSendState: defaultRecoverSendState,
  };
}

export interface MacroPreviewPageProps {
  store: RepositoryWorkingCopyStore;
  packageId: string;
  macroId: string;
  usbState: UsbState;
  onBack: () => void;
  /**
   * Called once a send has actually been initiated (by this screen) or is
   * already active on the device (recovered after a `409`) — the caller
   * (the app shell) hands the resulting status to the Macros page as its
   * next `initialSend` so the send is tracked there.
   */
  onSendInitiated: (status: SendStatusResponse) => void;
  dependencies?: MacroPreviewDependencies;
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

export function MacroPreviewPage({
  store,
  packageId,
  macroId,
  usbState,
  onBack,
  onSendInitiated,
  dependencies,
}: MacroPreviewPageProps): React.JSX.Element {
  const deps = dependencies ?? defaultDependencies();
  const [sending, setSending] = useState(false);
  const [sendError, setSendError] = useState<string | null>(null);

  const repository = store.getRepository();
  const pkg = repository.packages.find(
    (candidate) => candidate.id === packageId,
  );
  const macro = pkg?.macros.find((candidate) => candidate.id === macroId);

  if (pkg === undefined || macro === undefined) {
    return (
      <section aria-labelledby="preview-title">
        <h2 id="preview-title">Preview and send</h2>
        <p role="alert">This macro is no longer in the repository.</p>
        <button onClick={onBack} type="button">
          Back to Macros
        </button>
      </section>
    );
  }

  const compiled = compileMacro(macro.source, {
    keyPressMs: macro.keyPressMs,
    interKeyMs: macro.interKeyMs,
  });

  const sendNow = async (): Promise<void> => {
    setSending(true);
    setSendError(null);
    try {
      const handle = await deps.sendMacro({
        source: macro.source,
        keyPressMs: macro.keyPressMs,
        interKeyMs: macro.interKeyMs,
      });
      handle.stop();
      onSendInitiated({
        id: handle.accepted.id,
        state: handle.accepted.state,
        actionIndex: 0,
        actionCount: handle.accepted.actionCount,
        estimatedDurationMs: handle.accepted.estimatedDurationMs,
        cancellationRequested: false,
        error: "",
        releaseError: "",
      });
      onBack();
    } catch (error: unknown) {
      if (error instanceof V2ApiError && error.status === 409) {
        let current: SendStatusResponse | null;
        try {
          current = await deps.recoverSendState();
        } catch (recoverError: unknown) {
          setSending(false);
          setSendError(v2ErrorText(recoverError));
          return;
        }
        if (current === null || isTerminalSendState(current.state)) {
          setSending(false);
          setSendError(
            "Another send finished just before this one could start. Try again.",
          );
          return;
        }
        onSendInitiated(current);
        onBack();
        return;
      }
      setSending(false);
      setSendError(v2ErrorText(error));
    }
  };

  return (
    <section aria-labelledby="preview-title">
      <h2 id="preview-title">Preview and send</h2>
      <dl>
        <dt>Package</dt>
        <dd>{pkg.name}</dd>
        <dt>Macro</dt>
        <dd>{macro.name}</dd>
        <dt>Source</dt>
        <dd>
          <code>{macro.source}</code>
        </dd>
        <dt>Key-press time</dt>
        <dd>{String(macro.keyPressMs)} ms</dd>
        <dt>Inter-key delay</dt>
        <dd>{String(macro.interKeyMs)} ms</dd>
        {compiled.ok ? (
          <>
            <dt>Action count</dt>
            <dd>{String(compiled.actions.length)}</dd>
            <dt>Estimated duration</dt>
            <dd>{String(compiled.estimatedDurationMs)} ms</dd>
          </>
        ) : (
          <>
            <dt>Action count</dt>
            <dd>
              Unavailable — this source will be rejected:{" "}
              {compiled.error.message}
            </dd>
          </>
        )}
        <dt>USB</dt>
        <dd>
          <StatusBadge
            label={`USB ${usbState}`}
            state={usbBadgeState(usbState)}
          />
        </dd>
      </dl>
      <ErrorBanner message={sendError} />
      <div className="header-actions">
        <button
          className="primary"
          disabled={sending || usbState !== "ready"}
          onClick={() => {
            void sendNow();
          }}
          type="button"
        >
          {sending ? "Sending…" : "Send now"}
        </button>
        <button disabled={sending} onClick={onBack} type="button">
          Cancel
        </button>
      </div>
    </section>
  );
}
