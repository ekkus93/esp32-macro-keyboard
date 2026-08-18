import { SendStatus } from "../../../components/SendStatus";
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
    // `landscape-block` carries no CSS rule (the styling is inline here);
    // it is kept as a structural test hook --
    // tests/v2-app-v2-orientation.test.tsx:128 scopes its "Cancel and
    // release all keys" lookup to this element specifically, so the control
    // proven wired is the one visible in landscape rather than the
    // identically-labelled button on the hidden Macros page behind it.
    // All four safe-area insets are added, not just the horizontal pair:
    // this is the one surface that fills the viewport in *either* landscape
    // rotation, so the notch can land on either side and the home indicator
    // along the bottom edge.
    <div className="landscape-block fixed inset-0 z-[100] flex flex-col items-center justify-center gap-4 bg-legend pt-[calc(1.5rem+env(safe-area-inset-top))] pr-[calc(1.5rem+env(safe-area-inset-right))] pb-[calc(1.5rem+env(safe-area-inset-bottom))] pl-[calc(1.5rem+env(safe-area-inset-left))] text-center text-cap">
      <h1 className="m-0 text-[1.5rem]">Rotate your phone</h1>
      <p>ESP32 Macro Keyboard is designed for portrait mode.</p>
      {activeSend !== null ? (
        // `overlay` adds this surface's two overrides -- a measured width
        // and the inverted ink that reads on the full-viewport overlay.
        // Both are additive (the base sets neither), so there is no cascade
        // race; see SendStatus.tsx.
        <SendStatus overlay role="status">
          <p>{activeSend.statusText}</p>
          {activeSend.onCancel !== null ? (
            <button onClick={activeSend.onCancel} type="button">
              Cancel and release all keys
            </button>
          ) : null}
        </SendStatus>
      ) : null}
    </div>
  );
}
