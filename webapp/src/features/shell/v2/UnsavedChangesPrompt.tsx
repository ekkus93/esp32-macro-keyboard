import { useRef } from "react";
import { useFocusTrap } from "./useFocusTrap";

/**
 * The reusable "you have unsaved changes" warning, per SPEC_V2 §8.7 /
 * UI_UX_SPEC_V2 §7.3 (TODO_V2 V2-103): "the warning offers the
 * context-appropriate choices: Cancel, Export working copy, Save snapshot,
 * or Discard changes" and "the UI MUST NOT claim that a closed dirty working
 * copy can be recovered."
 *
 * This component is the shared primitive every dirty-blocking action needs
 * (Sign Out, loading another snapshot, import replacement, reset settings,
 * factory reset). As of Phase 11 (V2-113/V2-115), the Snapshots page wires
 * it for the two trigger points that phase owns — loading another snapshot
 * and import replacement (`SnapshotsPage.tsx`). Sign Out and Reset/Factory
 * reset belong to Phase 12's Settings screen (V2-120/V2-121) and remain
 * unwired until then.
 */

export interface UnsavedChangesPromptProps {
  /** Names the action being attempted, e.g. "sign out" or "load this snapshot". */
  actionLabel: string;
  onCancel: () => void;
  onExport: () => void;
  onSaveSnapshot: () => void;
  onDiscard: () => void;
  exporting?: boolean;
  saving?: boolean;
  /**
   * The discard button's exact label. Defaults to the generic "Discard
   * changes" (SPEC_V2 §8.7). UI_UX_SPEC_V2 §9.4 spells out a more specific
   * label for the snapshot-load trigger point: "Discard changes and load".
   */
  discardLabel?: string;
}

export function UnsavedChangesPrompt({
  actionLabel,
  onCancel,
  onExport,
  onSaveSnapshot,
  onDiscard,
  exporting = false,
  saving = false,
  discardLabel = "Discard changes",
}: UnsavedChangesPromptProps): React.JSX.Element {
  const containerRef = useRef<HTMLDivElement>(null);
  // Mounted only while this prompt is active (its caller conditionally
  // renders it), so the trap is always active for this component's lifetime
  // — TODO_V2 V2-133/UI_UX_SPEC_V2 §14.
  useFocusTrap({ active: true, containerRef, onClose: onCancel });

  return (
    <div className="dialog-backdrop" role="presentation">
      <div
        aria-labelledby="unsaved-changes-prompt-title"
        className="dialog-panel"
        ref={containerRef}
        role="alertdialog"
        tabIndex={-1}
      >
        <div className="dialog-heading">
          <h2 id="unsaved-changes-prompt-title">Unsaved changes</h2>
        </div>
        <p>
          You have unsaved changes. Continuing to {actionLabel} will lose them —
          once this working copy is closed without saving, the changes cannot be
          recovered.
        </p>
        <div className="form-actions">
          <button onClick={onCancel} type="button">
            Cancel
          </button>
          <button disabled={exporting} onClick={onExport} type="button">
            {exporting ? "Exporting…" : "Export working copy"}
          </button>
          <button
            className="primary"
            disabled={saving}
            onClick={onSaveSnapshot}
            type="button"
          >
            {saving ? "Saving…" : "Save snapshot"}
          </button>
          <button className="danger" onClick={onDiscard} type="button">
            {discardLabel}
          </button>
        </div>
      </div>
    </div>
  );
}
