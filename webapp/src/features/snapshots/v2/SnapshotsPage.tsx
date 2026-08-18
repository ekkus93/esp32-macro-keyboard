import { useEffect, useRef, useState } from "react";
import { HeaderActions } from "../../../components/HeaderActions";
import { PageHeading } from "../../../components/PageHeading";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { UnsavedChangesPrompt } from "../../shell/v2/UnsavedChangesPrompt";
import {
  persistSelectedPackageId as defaultPersistSelectedPackageId,
  resolveSelectedPackage,
  tryPersistSelectedPackageId,
  type PackageSelectionPersistenceFailure,
} from "../../../v2/packageSelection";
import type { Repository } from "../../../v2/repository";
import { saveBytesAsFile } from "../../../v2/saveFile";
import { evaluateSnapshotRetention } from "../../../v2/snapshotRetention";
import { useFocusTrap } from "../../shell/v2/useFocusTrap";
import {
  deleteSnapshot as defaultDeleteSnapshot,
  downloadSnapshotBytes as defaultDownloadSnapshotBytes,
  exportRepository as defaultExportRepository,
  importRepository as defaultImportRepository,
  listSnapshots as defaultListSnapshots,
  loadSnapshotIntoWorkingCopy as defaultLoadSnapshotIntoWorkingCopy,
  replaceSnapshotWithWorkingCopy as defaultReplaceSnapshotWithWorkingCopy,
} from "../../../v2/snapshotClient";
import type {
  ReplaceSnapshotResult,
  SnapshotListResult,
} from "../../../v2/snapshotClient";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";
import { SnapshotRow } from "./SnapshotRow";
import {
  formatBytes,
  importFailureMessage,
  loadFailureMessage,
} from "./snapshotMessages";

/**
 * Snapshot Management, Advisory retention, Dirty-work-during-load protection,
 * Unreadable-snapshot recovery, and Import/Export, per UI_UX_SPEC_V2 §9/§10
 * and SPEC_V2 §8.7/§8.8/§9/§10 (TODO_V2 V2-110 through V2-116). This is the
 * "Snapshots" bottom-navigation destination Phase 9 left as a placeholder.
 *
 * Every mutating action here is an explicit user click — nothing in this
 * module runs on a timer, after an edit, after a send, or on navigation
 * (TODO_V2 V2-110 "never autosave"). Deletion is likewise always a typed,
 * exact-blob-ID confirmation (V2-111) and never automatic (V2-111/V2-112).
 */

async function readFileBytes(file: File): Promise<Uint8Array> {
  const buffer = await file.arrayBuffer();
  return new Uint8Array(buffer);
}

export interface SnapshotsPageDependencies {
  listSnapshots: typeof defaultListSnapshots;
  loadSnapshotIntoWorkingCopy: typeof defaultLoadSnapshotIntoWorkingCopy;
  downloadSnapshotBytes: typeof defaultDownloadSnapshotBytes;
  deleteSnapshot: typeof defaultDeleteSnapshot;
  exportRepository: typeof defaultExportRepository;
  importRepository: typeof defaultImportRepository;
  replaceSnapshotWithWorkingCopy: typeof defaultReplaceSnapshotWithWorkingCopy;
  persistSelectedPackageId: typeof defaultPersistSelectedPackageId;
  saveAsFile: (bytes: Uint8Array, filename: string, mimeType: string) => void;
  readFileBytes: (file: File) => Promise<Uint8Array>;
}

function defaultDependencies(): SnapshotsPageDependencies {
  return {
    listSnapshots: defaultListSnapshots,
    loadSnapshotIntoWorkingCopy: defaultLoadSnapshotIntoWorkingCopy,
    downloadSnapshotBytes: defaultDownloadSnapshotBytes,
    deleteSnapshot: defaultDeleteSnapshot,
    exportRepository: defaultExportRepository,
    importRepository: defaultImportRepository,
    replaceSnapshotWithWorkingCopy: defaultReplaceSnapshotWithWorkingCopy,
    persistSelectedPackageId: defaultPersistSelectedPackageId,
    saveAsFile: saveBytesAsFile,
    readFileBytes,
  };
}

export interface SnapshotsPageProps {
  store: RepositoryWorkingCopyStore;
  /** Updates the caller's local selection state; never touches the repository. */
  onSelectionChange: (packageId: string) => void;
  /** Last package ID known to have persisted successfully on the device. */
  persistedPackageId: string | null;
  /** Makes a failed preference write visible outside this page/navigation. */
  onSelectionPersistenceFailure: (
    failure: PackageSelectionPersistenceFailure,
  ) => void;
  /** Clears any prior persistence warning after a successful preference write. */
  onSelectionPersistenceSuccess: (packageId: string | null) => void;
  /** Navigates to the Macros page once a package resolves after load/import. */
  onOpenMacros: () => void;
  /** Navigates to the Package chooser when no package resolves after load/import. */
  onOpenPackages: () => void;
  /** `settings.snapshotRetentionTarget` (SPEC_V2 §10.8, default `5`). */
  retentionTarget: number;
  /** The blob ID the working copy currently reflects, if any (V2-111). */
  loadedBlobId: string | null;
  /** Called after a save, load, replace, or import changes that association. */
  onWorkingCopyOriginChanged: (blobId: string | null) => void;
  /** The single shared Save snapshot handler (same one the header button uses). */
  onSaveSnapshot: () => Promise<void>;
  saving: boolean;
  saveError: string | null;
  dependencies?: SnapshotsPageDependencies;
}

interface PendingReplace {
  id: string;
  busy: boolean;
  result: ReplaceSnapshotResult | null;
}

export function SnapshotsPage({
  store,
  persistedPackageId,
  onSelectionChange,
  onSelectionPersistenceFailure,
  onSelectionPersistenceSuccess,
  onOpenMacros,
  onOpenPackages,
  retentionTarget,
  loadedBlobId,
  onWorkingCopyOriginChanged,
  onSaveSnapshot,
  saving,
  saveError,
  dependencies,
}: SnapshotsPageProps): React.JSX.Element {
  const deps = dependencies ?? defaultDependencies();
  const depsRef = useRef(deps);
  depsRef.current = deps;

  const [list, setList] = useState<
    | { kind: "loading" }
    | { kind: "error"; message: string }
    | { kind: "loaded"; result: SnapshotListResult }
  >({ kind: "loading" });

  const refreshList = (): void => {
    setList({ kind: "loading" });
    depsRef.current
      .listSnapshots()
      .then((result) => {
        setList({ kind: "loaded", result });
      })
      .catch((error: unknown) => {
        setList({ kind: "error", message: v2ErrorText(error) });
      });
  };

  useEffect(refreshList, [loadedBlobId]);

  const [rowBusy, setRowBusy] = useState<{
    id: string;
    action: "load" | "download" | "delete";
  } | null>(null);
  const [rowError, setRowError] = useState<string | null>(null);
  const [loadError, setLoadError] = useState<{
    blobId: string;
    message: string;
  } | null>(null);
  const [advancedOpenId, setAdvancedOpenId] = useState<string | null>(null);
  const [pendingReplace, setPendingReplace] = useState<PendingReplace | null>(
    null,
  );

  const [pendingLoad, setPendingLoad] = useState<string | null>(null);
  const [exportingWorkingCopy, setExportingWorkingCopy] = useState(false);
  const [exportError, setExportError] = useState<string | null>(null);

  const [importState, setImportState] = useState<
    | { kind: "idle" }
    | { kind: "reading" }
    | { kind: "error"; message: string }
    | {
        kind: "ready";
        repository: Repository;
        packageCount: number;
        macroCount: number;
      }
  >({ kind: "idle" });
  const [confirmingImportWhileDirty, setConfirmingImportWhileDirty] =
    useState(false);
  const fileInputRef = useRef<HTMLInputElement | null>(null);
  const importConfirmRef = useRef<HTMLDivElement>(null);
  useFocusTrap({
    active: importState.kind === "ready",
    containerRef: importConfirmRef,
    onClose: () => {
      setImportState({ kind: "idle" });
    },
  });

  const exportWorkingCopy = async (): Promise<void> => {
    setExportingWorkingCopy(true);
    setExportError(null);
    try {
      const exported = await depsRef.current.exportRepository(
        store.getRepository(),
      );
      depsRef.current.saveAsFile(
        exported.bytes,
        exported.filename,
        exported.mimeType,
      );
    } catch (error: unknown) {
      setExportError(`Could not export working copy: ${v2ErrorText(error)}`);
    } finally {
      setExportingWorkingCopy(false);
    }
  };

  const afterWorkingCopyReplaced = async (
    repository: Repository,
  ): Promise<void> => {
    const resolution = resolveSelectedPackage(repository, persistedPackageId);
    if (resolution.kind === "chooser") {
      onOpenPackages();
      return;
    }
    if (resolution.shouldPersist) {
      const persistence = await tryPersistSelectedPackageId(
        resolution.packageId,
        persistedPackageId,
        depsRef.current.persistSelectedPackageId,
      );
      if (persistence.kind === "failed") {
        onSelectionPersistenceFailure(persistence.failure);
      } else {
        onSelectionPersistenceSuccess(resolution.packageId);
      }
    } else {
      onSelectionPersistenceSuccess(resolution.packageId);
    }
    onSelectionChange(resolution.packageId);
    onOpenMacros();
  };

  const performLoad = async (id: string): Promise<void> => {
    setRowBusy({ id, action: "load" });
    setRowError(null);
    try {
      const result = await depsRef.current.loadSnapshotIntoWorkingCopy(
        id,
        store,
      );
      if (!result.ok) {
        setLoadError({ blobId: id, message: loadFailureMessage(result) });
        return;
      }
      setLoadError(null);
      onWorkingCopyOriginChanged(id);
      await afterWorkingCopyReplaced(result.repository);
    } catch (error: unknown) {
      setLoadError({ blobId: id, message: v2ErrorText(error) });
    } finally {
      setRowBusy(null);
    }
  };

  const requestLoad = (id: string): void => {
    if (store.getIsDirty()) {
      setPendingLoad(id);
      return;
    }
    void performLoad(id);
  };

  const saveThenLoad = async (): Promise<void> => {
    const id = pendingLoad;
    if (id === null) {
      return;
    }
    await onSaveSnapshot();
    if (!store.getIsDirty()) {
      setPendingLoad(null);
      await performLoad(id);
    }
  };

  const download = async (id: string): Promise<void> => {
    setRowBusy({ id, action: "download" });
    setRowError(null);
    try {
      const bytes = await depsRef.current.downloadSnapshotBytes(id);
      depsRef.current.saveAsFile(
        bytes,
        `snapshot-${id}.emk-repository.json.gz`,
        "application/gzip",
      );
    } catch (error: unknown) {
      setRowError(v2ErrorText(error));
    } finally {
      setRowBusy(null);
    }
  };

  const deleteRow = async (id: string): Promise<void> => {
    setRowBusy({ id, action: "delete" });
    setRowError(null);
    try {
      await depsRef.current.deleteSnapshot(id);
      if (loadedBlobId === id) {
        onWorkingCopyOriginChanged(null);
      }
      refreshList();
    } catch (error: unknown) {
      setRowError(v2ErrorText(error));
    } finally {
      setRowBusy(null);
    }
  };

  const replaceRow = async (id: string): Promise<void> => {
    setPendingReplace({ id, busy: true, result: null });
    const result = await depsRef.current.replaceSnapshotWithWorkingCopy(
      id,
      store,
    );
    setPendingReplace({ id, busy: false, result });
    if (result.ok) {
      onWorkingCopyOriginChanged(result.created.id);
    }
  };

  const chooseFile = (): void => {
    fileInputRef.current?.click();
  };

  const onFileChosen = async (
    event: React.ChangeEvent<HTMLInputElement>,
  ): Promise<void> => {
    const file = event.currentTarget.files?.[0];
    event.currentTarget.value = "";
    if (file === undefined) {
      return;
    }
    setImportState({ kind: "reading" });
    try {
      const bytes = await depsRef.current.readFileBytes(file);
      const result = await depsRef.current.importRepository(bytes);
      if (!result.ok) {
        setImportState({
          kind: "error",
          message: importFailureMessage(result),
        });
        return;
      }
      setImportState({
        kind: "ready",
        repository: result.repository,
        packageCount: result.packageCount,
        macroCount: result.macroCount,
      });
    } catch (error: unknown) {
      setImportState({ kind: "error", message: v2ErrorText(error) });
    }
  };

  const applyImport = async (repository: Repository): Promise<void> => {
    store.applyImport(repository);
    onWorkingCopyOriginChanged(null);
    setImportState({ kind: "idle" });
    setConfirmingImportWhileDirty(false);
    await afterWorkingCopyReplaced(repository);
  };

  const confirmImport = (repository: Repository): void => {
    if (store.getIsDirty()) {
      setConfirmingImportWhileDirty(true);
      return;
    }
    void applyImport(repository);
  };

  const saveThenImport = async (repository: Repository): Promise<void> => {
    await onSaveSnapshot();
    if (!store.getIsDirty()) {
      await applyImport(repository);
    }
  };

  const retention = evaluateSnapshotRetention(
    list.kind === "loaded" ? list.result.blobs.length : 0,
    retentionTarget,
  );

  return (
    <section aria-labelledby="snapshots-title">
      <PageHeading>
        <div>
          <h2 id="snapshots-title">Snapshots</h2>
          {list.kind === "loaded" ? (
            <dl
              // storage-summary carries no CSS rule (styling is inline
              // below); kept as a test hook only --
              // webapp/tests/browser/workflows/snapshots.mjs:24.
              className="storage-summary mt-2 grid justify-start gap-x-5 gap-y-[0.15rem] text-[0.85rem] [grid-template-columns:repeat(2,auto)] min-[26rem]:[grid-template-columns:repeat(4,auto)]"
            >
              <dt className="[grid-row:1]">Stored</dt>
              <dd className="[grid-row:2] text-base font-bold">
                {String(list.result.blobs.length)}
              </dd>
              <dt className="[grid-row:1]">Used</dt>
              <dd className="[grid-row:2] text-base font-bold">
                {formatBytes(list.result.usedBytes)}
              </dd>
              <dt className="[grid-row:1] max-[26rem]:[grid-row:3]">Free</dt>
              <dd className="[grid-row:2] text-base font-bold max-[26rem]:[grid-row:4]">
                {formatBytes(list.result.remainingBytes)}
              </dd>
              <dt className="[grid-row:1] max-[26rem]:[grid-row:3]">
                Retention target
              </dt>
              <dd className="[grid-row:2] text-base font-bold max-[26rem]:[grid-row:4]">
                {String(retentionTarget)}
              </dd>
            </dl>
          ) : null}
        </div>
        <HeaderActions>
          <button
            className="primary"
            disabled={saving}
            onClick={() => {
              void onSaveSnapshot();
            }}
            type="button"
          >
            {saving ? "Saving…" : "Save current snapshot"}
          </button>
        </HeaderActions>
      </PageHeading>

      <ErrorBanner message={saveError} />
      <ErrorBanner message={rowError} />
      <ErrorBanner message={exportError} />

      {retention.overTarget ? (
        <p role="status">
          {String(retention.count)} snapshots are stored, above the retention
          target of {String(retention.target)}. This is advisory only — nothing
          is deleted automatically. Choose any snapshots below to delete if you
          want to clean up.
        </p>
      ) : null}

      {list.kind === "loading" ? <p role="status">Loading snapshots…</p> : null}
      {list.kind === "error" ? (
        <div>
          <ErrorBanner message={list.message} />
          <button onClick={refreshList} type="button">
            Retry
          </button>
        </div>
      ) : null}
      {list.kind === "loaded" && list.result.blobs.length === 0 ? (
        <p role="status">No snapshots are stored on this device yet.</p>
      ) : null}
      {list.kind === "loaded" ? (
        <div aria-label="Snapshot list">
          {list.result.blobs.map((blob) => (
            <SnapshotRow
              advancedOpen={advancedOpenId === blob.id}
              busy={rowBusy?.id === blob.id ? rowBusy.action : null}
              id={blob.id}
              isLoaded={loadedBlobId === blob.id}
              key={blob.id}
              loadErrorMessage={
                loadError?.blobId === blob.id ? loadError.message : null
              }
              onDelete={() => {
                void deleteRow(blob.id);
              }}
              onDownload={() => {
                void download(blob.id);
              }}
              onLoad={() => {
                requestLoad(blob.id);
              }}
              onReplace={() => {
                void replaceRow(blob.id);
              }}
              onToggleAdvanced={() => {
                setAdvancedOpenId((current) =>
                  current === blob.id ? null : blob.id,
                );
              }}
              replaceBusy={
                pendingReplace?.id === blob.id ? pendingReplace.busy : false
              }
              replaceResult={
                pendingReplace?.id === blob.id ? pendingReplace.result : null
              }
              sizeBytes={blob.sizeBytes}
            />
          ))}
        </div>
      ) : null}

      {pendingLoad !== null ? (
        <UnsavedChangesPrompt
          actionLabel={`load snapshot ${pendingLoad}, replacing your working copy`}
          discardLabel="Discard changes and load"
          exporting={exportingWorkingCopy}
          onCancel={() => {
            setPendingLoad(null);
          }}
          onDiscard={() => {
            const id = pendingLoad;
            store.discardChanges();
            setPendingLoad(null);
            void performLoad(id);
          }}
          onExport={() => {
            void exportWorkingCopy();
          }}
          onSaveSnapshot={() => {
            void saveThenLoad();
          }}
          saving={saving}
        />
      ) : null}

      <section aria-labelledby="import-export-title">
        <h3 id="import-export-title">Import and export</h3>
        <button
          disabled={exportingWorkingCopy}
          onClick={() => {
            void exportWorkingCopy();
          }}
          type="button"
        >
          {exportingWorkingCopy ? "Exporting…" : "Export working copy"}
        </button>
        <input
          accept=".gz,application/gzip"
          hidden
          onChange={(event) => {
            void onFileChosen(event);
          }}
          ref={fileInputRef}
          type="file"
        />
        <button
          disabled={importState.kind === "reading"}
          onClick={chooseFile}
          type="button"
        >
          {importState.kind === "reading" ? "Reading…" : "Import repository…"}
        </button>

        {importState.kind === "error" ? (
          <ErrorBanner message={importState.message} />
        ) : null}

        {importState.kind === "ready" ? (
          <div
            className="danger-zone"
            ref={importConfirmRef}
            role="alertdialog"
            tabIndex={-1}
          >
            <p>
              This file contains {String(importState.packageCount)} packages and{" "}
              {String(importState.macroCount)} macros. Importing replaces your
              entire working copy and marks it unsaved — nothing uploads to the
              device until you Save snapshot.
            </p>
            <button
              className="primary"
              onClick={() => {
                confirmImport(importState.repository);
              }}
              type="button"
            >
              Replace working copy with this import
            </button>
            <button
              onClick={() => {
                setImportState({ kind: "idle" });
              }}
              type="button"
            >
              Cancel
            </button>
          </div>
        ) : null}

        {confirmingImportWhileDirty && importState.kind === "ready" ? (
          <UnsavedChangesPrompt
            actionLabel="import this repository, replacing your working copy"
            discardLabel="Discard changes and import"
            exporting={exportingWorkingCopy}
            onCancel={() => {
              setConfirmingImportWhileDirty(false);
            }}
            onDiscard={() => {
              store.discardChanges();
              void applyImport(importState.repository);
            }}
            onExport={() => {
              void exportWorkingCopy();
            }}
            onSaveSnapshot={() => {
              void saveThenImport(importState.repository);
            }}
            saving={saving}
          />
        ) : null}
      </section>
    </section>
  );
}
