import type { ActiveSendSummary } from "./activeSendSummary";

/**
 * The full-viewport phone-landscape orientation surface, per UI_UX_SPEC_V2
 * §12.2/§12.3 (TODO_V2 V2-131/V2-132). Rendered as a sibling of the ordinary
 * authenticated shell, never in its place — the caller keeps the ordinary
 * shell mounted and merely hides it (e.g. `display: none`) while this is
 * shown, so no route, draft, working-copy, or send state is ever lost.
 *
 * Exact copy from UI_UX_SPEC_V2 §12.2:
 * ```text
 * Rotate your phone
 * ESP32 Macro Keyboard is designed for portrait mode.
 * ```
 *
 * When `activeSend` is non-null (a send is starting, awaiting confirmation,
 * or running — §12.3), this also shows the macro name, current send state
 * or progress, and Cancel and release all keys, so the portrait requirement
 * never makes emergency cancellation inaccessible.
 */
export interface LandscapeBlockSurfaceProps {
  activeSend: ActiveSendSummary | null;
}

export function LandscapeBlockSurface({
  activeSend,
}: LandscapeBlockSurfaceProps): React.JSX.Element {
  return (
    <div className="landscape-block">
      <h1>Rotate your phone</h1>
      <p>ESP32 Macro Keyboard is designed for portrait mode.</p>
      {activeSend !== null ? (
        <div aria-live="polite" className="send-status" role="status">
          <p>{activeSend.statusText}</p>
          {activeSend.onCancel !== null ? (
            <button onClick={activeSend.onCancel} type="button">
              Cancel and release all keys
            </button>
          ) : null}
        </div>
      ) : null}
    </div>
  );
}
