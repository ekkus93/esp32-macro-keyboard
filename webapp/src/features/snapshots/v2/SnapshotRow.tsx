import { useEffect, useState } from "react";
import { Card } from "../../../components/Card";
import { ConfirmDialog } from "../../../components/ConfirmDialog";
import { Icon } from "../../../components/m3/Icon";
import { Menu } from "../../../components/m3/Menu";
import type { MenuItem } from "../../../components/m3/Menu";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import { formatBytes } from "./snapshotMessages";
import type { ReplaceSnapshotResult } from "../../../v2/snapshotClient";

interface SnapshotRowProps {
  id: string;
  sizeBytes: number;
  isLoaded: boolean;
  busy: "load" | "download" | "delete" | null;
  loadErrorMessage: string | null;
  onLoad: () => void;
  onDownload: () => void;
  onDelete: () => void;
  onReplace: () => void;
  /** Called when the replace dialog closes, so the page can forget its result. */
  onReplaceDismiss: () => void;
  replaceBusy: boolean;
  replaceResult: ReplaceSnapshotResult | null;
}

/**
 * One stored snapshot: its ID and size, a "Loaded" mark when the working copy
 * came from it, a prominent Load, and a "More actions" menu for Download,
 * Replace, and Delete (UXR-051). Delete asks for the exact snapshot ID
 * (SPEC_V2 §10; V2-111); Replace explains that it is not atomic. Both are
 * modal confirmations that name the snapshot and the consequence.
 */
export function SnapshotRow({
  id,
  sizeBytes,
  isLoaded,
  busy,
  loadErrorMessage,
  onLoad,
  onDownload,
  onDelete,
  onReplace,
  onReplaceDismiss,
  replaceBusy,
  replaceResult,
}: SnapshotRowProps): React.JSX.Element {
  const [confirmingDelete, setConfirmingDelete] = useState(false);
  const [typedId, setTypedId] = useState("");
  const [replaceOpen, setReplaceOpen] = useState(false);
  const anyBusy = busy !== null;

  // A successful replace is finished: close its dialog. A failed one stays
  // open so the person reads what happened to their snapshot.
  useEffect(() => {
    if (replaceResult?.ok === true) {
      setReplaceOpen(false);
      onReplaceDismiss();
    }
  }, [onReplaceDismiss, replaceResult]);

  const closeDelete = (): void => {
    setConfirmingDelete(false);
    setTypedId("");
  };
  const closeReplace = (): void => {
    setReplaceOpen(false);
    onReplaceDismiss();
  };

  const items: readonly MenuItem[] = [
    {
      key: "download",
      label: "Download",
      ariaLabel: `Download snapshot ${id}`,
      icon: "download",
      disabled: anyBusy,
      onSelect: onDownload,
    },
    {
      key: "replace",
      label: "Replace with working copy",
      ariaLabel: `Replace snapshot ${id} with current working copy`,
      icon: "save",
      disabled: anyBusy,
      onSelect: () => {
        setReplaceOpen(true);
      },
    },
    {
      key: "delete",
      label: "Delete",
      ariaLabel: `Delete snapshot ${id}`,
      icon: "trash",
      tone: "danger",
      disabled: anyBusy,
      onSelect: () => {
        setConfirmingDelete(true);
      },
    },
  ];

  return (
    <Card variant={isLoaded ? "selected" : "flush"}>
      <div className="min-w-0">
        <h3>Snapshot {id}</h3>
        <p className="flex flex-wrap items-center gap-x-3 gap-y-1">
          {formatBytes(sizeBytes)}
          {isLoaded ? (
            <span className="inline-flex items-center gap-1 font-semibold">
              <Icon name="check" size={16} />
              Loaded
            </span>
          ) : null}
        </p>
        {loadErrorMessage !== null ? (
          <p role="alert">
            Snapshot {id} could not be opened: {loadErrorMessage} It stays
            stored — you can download it, delete it, or load another snapshot.
          </p>
        ) : null}
      </div>
      <div className="flex items-center gap-2">
        <button
          aria-label={`Load snapshot ${id}`}
          className="primary flex-1 min-[34rem]:flex-none"
          disabled={anyBusy}
          onClick={onLoad}
          type="button"
        >
          {busy === "load" ? "Loading…" : "Load"}
        </button>
        <Menu
          items={items}
          menuLabel={`Actions for snapshot ${id}`}
          triggerLabel={`More actions for snapshot ${id}`}
        />
      </div>

      {confirmingDelete ? (
        <ConfirmDialog
          confirmDisabled={typedId !== id || anyBusy}
          confirmLabel={busy === "delete" ? "Deleting…" : "Delete snapshot"}
          id={`delete-snapshot-title-${id}`}
          onCancel={closeDelete}
          onConfirm={() => {
            closeDelete();
            onDelete();
          }}
          title="Delete snapshot?"
        >
          <p>
            This permanently deletes snapshot <strong>{id}</strong> (
            {formatBytes(sizeBytes)}). This cannot be undone. Type the snapshot
            ID to confirm.
          </p>
          <label
            className="mt-3 block"
            htmlFor={`snapshot-delete-confirm-${id}`}
          >
            Snapshot ID
            <input
              autoComplete="off"
              id={`snapshot-delete-confirm-${id}`}
              onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                setTypedId(event.currentTarget.value);
              }}
              value={typedId}
            />
          </label>
        </ConfirmDialog>
      ) : null}

      {replaceOpen ? (
        <ConfirmDialog
          confirmAriaLabel={`Confirm replace snapshot ${id}`}
          confirmDisabled={replaceBusy || replaceResult?.ok === false}
          confirmLabel={replaceBusy ? "Replacing…" : "Replace snapshot"}
          id={`replace-snapshot-title-${id}`}
          onCancel={closeReplace}
          onConfirm={() => {
            onReplace();
          }}
          title="Replace snapshot?"
        >
          <p>
            This deletes snapshot {id} first, then uploads your current working
            copy as a new snapshot. If the upload fails after the delete,
            snapshot {id} stays deleted and is <strong>not</strong> restored.
          </p>
          {replaceResult?.ok === false && replaceResult.stage === "add" ? (
            <p role="alert">
              Snapshot {id} was deleted, but uploading the replacement failed:{" "}
              {v2ErrorText(replaceResult.error)}. The deleted snapshot was{" "}
              <strong>not</strong> restored. Your working copy is still unsaved
              — use Save snapshot to store it as a new snapshot.
            </p>
          ) : null}
          {replaceResult?.ok === false && replaceResult.stage === "delete" ? (
            <p role="alert">
              Could not delete snapshot {id}: {v2ErrorText(replaceResult.error)}
              . Nothing was changed.
            </p>
          ) : null}
        </ConfirmDialog>
      ) : null}
    </Card>
  );
}
