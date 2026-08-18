import { useRef, useState } from "react";
import { Card } from "../../../components/Card";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import { useFocusTrap } from "../../shell/v2/useFocusTrap";
import { formatBytes } from "./snapshotMessages";
import type { ReplaceSnapshotResult } from "../../../v2/snapshotClient";

interface SnapshotRowProps {
  id: string;
  sizeBytes: number;
  isLoaded: boolean;
  busy: "load" | "download" | "delete" | null;
  advancedOpen: boolean;
  loadErrorMessage: string | null;
  onLoad: () => void;
  onDownload: () => void;
  onDelete: () => void;
  onToggleAdvanced: () => void;
  onReplace: () => void;
  replaceBusy: boolean;
  replaceResult: ReplaceSnapshotResult | null;
}

export function SnapshotRow({
  id,
  sizeBytes,
  isLoaded,
  busy,
  advancedOpen,
  loadErrorMessage,
  onLoad,
  onDownload,
  onDelete,
  onToggleAdvanced,
  onReplace,
  replaceBusy,
  replaceResult,
}: SnapshotRowProps): React.JSX.Element {
  const [confirmingDelete, setConfirmingDelete] = useState(false);
  const [typedId, setTypedId] = useState("");
  const [confirmingReplace, setConfirmingReplace] = useState(false);
  const anyBusy = busy !== null;
  const deleteConfirmRef = useRef<HTMLDivElement>(null);
  // See `MacroOverflowMenu`'s identical `deleteButtonRef` (MacrosPage.tsx)
  // for why an explicit `restoreFocusRef` is needed here.
  const deleteButtonRef = useRef<HTMLButtonElement>(null);
  useFocusTrap({
    active: confirmingDelete,
    containerRef: deleteConfirmRef,
    onClose: () => {
      setConfirmingDelete(false);
      setTypedId("");
    },
    restoreFocusRef: deleteButtonRef,
  });

  return (
    <Card variant="flush">
      <div className="min-w-0">
        <h3>
          Snapshot {id}
          {isLoaded ? " (loaded)" : ""}
        </h3>
        <p>{formatBytes(sizeBytes)}</p>
        {loadErrorMessage !== null ? (
          <p role="alert">
            Snapshot {id} could not be opened: {loadErrorMessage}. It remains
            stored — download it, delete it, or choose another snapshot.
          </p>
        ) : null}
      </div>
      <div className="flex flex-wrap content-start gap-[0.4rem] [&_button]:flex-initial max-[32rem]:w-full">
        <button
          aria-label={`Load snapshot ${id}`}
          className="primary"
          disabled={anyBusy}
          onClick={onLoad}
          type="button"
        >
          {busy === "load" ? "Loading…" : "Load"}
        </button>
        <button
          aria-label={`Download snapshot ${id}`}
          disabled={anyBusy}
          onClick={onDownload}
          type="button"
        >
          {busy === "download" ? "Downloading…" : "Download"}
        </button>
        {confirmingDelete ? (
          <div
            className="danger-zone"
            ref={deleteConfirmRef}
            role="alertdialog"
            tabIndex={-1}
          >
            <p>
              This permanently deletes snapshot <strong>{id}</strong> (
              {formatBytes(sizeBytes)}). This cannot be undone. Type the
              snapshot ID to confirm.
            </p>
            <label htmlFor={`snapshot-delete-confirm-${id}`}>
              Snapshot ID
              <input
                id={`snapshot-delete-confirm-${id}`}
                onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                  setTypedId(event.currentTarget.value);
                }}
                value={typedId}
              />
            </label>
            <button
              className="danger"
              disabled={typedId !== id || anyBusy}
              onClick={() => {
                setConfirmingDelete(false);
                setTypedId("");
                onDelete();
              }}
              type="button"
            >
              {busy === "delete" ? "Deleting…" : "Confirm delete"}
            </button>
            <button
              onClick={() => {
                setConfirmingDelete(false);
                setTypedId("");
              }}
              type="button"
            >
              Cancel
            </button>
          </div>
        ) : (
          <button
            aria-label={`Delete snapshot ${id}`}
            className="danger"
            disabled={anyBusy}
            onClick={() => {
              setConfirmingDelete(true);
            }}
            ref={deleteButtonRef}
            type="button"
          >
            Delete
          </button>
        )}
        <button
          aria-expanded={advancedOpen}
          aria-label={`Show advanced options for snapshot ${id}`}
          onClick={onToggleAdvanced}
          type="button"
        >
          Advanced
        </button>
        {advancedOpen ? (
          <div className="danger-zone">
            {confirmingReplace ? (
              <>
                <p>
                  This deletes snapshot {id} first, then uploads the current
                  working copy as a new blob. There is no atomic replace
                  (SPEC_V2 §10.6): if the upload fails after the delete
                  succeeds, snapshot {id} stays deleted and is{" "}
                  <strong>not</strong> restored.
                </p>
                <button
                  aria-label={`Confirm replace snapshot ${id}`}
                  className="danger"
                  disabled={replaceBusy}
                  onClick={() => {
                    setConfirmingReplace(false);
                    onReplace();
                  }}
                  type="button"
                >
                  {replaceBusy ? "Replacing…" : "Confirm replace"}
                </button>
                <button
                  onClick={() => {
                    setConfirmingReplace(false);
                  }}
                  type="button"
                >
                  Cancel
                </button>
              </>
            ) : (
              <button
                aria-label={`Replace snapshot ${id} with current working copy`}
                onClick={() => {
                  setConfirmingReplace(true);
                }}
                type="button"
              >
                Replace with current working copy
              </button>
            )}
            {replaceResult?.ok === false && replaceResult.stage === "add" ? (
              <p role="alert">
                Snapshot {id} was deleted, but uploading the replacement failed:{" "}
                {v2ErrorText(replaceResult.error)}. The deleted snapshot was{" "}
                <strong>not</strong> restored. Your working copy is still
                unsaved — use Save snapshot to store it as a new snapshot.
              </p>
            ) : null}
            {replaceResult?.ok === false && replaceResult.stage === "delete" ? (
              <p role="alert">
                Could not delete snapshot {id}:{" "}
                {v2ErrorText(replaceResult.error)}. Nothing was changed.
              </p>
            ) : null}
          </div>
        ) : null}
      </div>
    </Card>
  );
}
