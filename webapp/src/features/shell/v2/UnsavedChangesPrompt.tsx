/**
 * The reusable "you have unsaved changes" warning, per SPEC_V2 §8.7 /
 * UI_UX_SPEC_V2 §7.3 (TODO_V2 V2-103): "the warning offers the
 * context-appropriate choices: Cancel, Export working copy, Save snapshot,
 * or Discard changes" and "the UI MUST NOT claim that a closed dirty working
 * copy can be recovered."
 *
 * This component is the shared primitive every dirty-blocking action needs
 * (Sign Out, loading another snapshot, import replacement, reset settings,
 * factory reset) — but as of Phase 10 none of those actions has a surface
 * in the v2 UI yet (Sign Out and Reset/Factory reset belong to Phase 12's
 * Settings screen, V2-120/V2-121; snapshot load and import replacement
 * belong to Phase 11, V2-113/V2-116). It is built and tested here so those
 * later phases render it rather than inventing their own warning, but no
 * v2 screen invokes it yet — see the Phase 10 implementation report for
 * exactly what evidence this does and does not cover.
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
}

export function UnsavedChangesPrompt({
  actionLabel,
  onCancel,
  onExport,
  onSaveSnapshot,
  onDiscard,
  exporting = false,
  saving = false,
}: UnsavedChangesPromptProps): React.JSX.Element {
  return (
    <div className="dialog-backdrop" role="presentation">
      <div
        aria-labelledby="unsaved-changes-prompt-title"
        className="dialog-panel"
        role="alertdialog"
      >
        <div className="dialog-heading">
          <h2 id="unsaved-changes-prompt-title">Unsaved changes</h2>
        </div>
        <p>
          You have unsaved changes. Continuing to {actionLabel} will lose
          them — once this working copy is closed without saving, the
          changes cannot be recovered.
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
            Discard changes
          </button>
        </div>
      </div>
    </div>
  );
}
