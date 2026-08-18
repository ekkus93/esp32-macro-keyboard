import { useEffect, useRef, useState } from "react";
import { DangerZone } from "../../../components/DangerZone";
import { PageHeading } from "../../../components/PageHeading";
import { FieldHelp } from "../../../components/FieldHelp";
import { FormActions } from "../../../components/FormActions";
import { Card } from "../../../components/Card";
import { v2Limits } from "../../../v2/limits";
import {
  persistSelectedPackageId as defaultPersistSelectedPackageId,
  resolveSelectedPackage,
  tryPersistSelectedPackageId,
  type PackageSelectionPersistenceFailure,
} from "../../../v2/packageSelection";
import { utf8ByteLength, type RepositoryPackage } from "../../../v2/repository";
import {
  addPackage,
  createPackageId,
  deletePackage,
  duplicatePackage,
  movePackage,
  movePackageToIndex,
  renamePackage,
} from "../../../v2/repositoryEditing";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";
import { useFocusTrap } from "../../shell/v2/useFocusTrap";

/** TODO_V2 V2-133/UI_UX_SPEC_V2 §14 reordering alternative to drag and drop. */
type MoveAction = "first" | "up" | "down" | "last";

/**
 * The Package chooser and Package management screen, per UI_UX_SPEC_V2 §6
 * (TODO_V2 V2-102). Both concerns share the one "Packages" nav destination:
 * §6.1's chooser (search, macro counts, a non-dirtying **Open**) and §6.2's
 * management operations (create/rename/duplicate/reorder/delete, all
 * working-copy edits that dirty the repository). Package selection itself
 * is persisted through the same `persistSelectedPackageId` helper Phase 8's
 * startup sequence uses (TODO_V2 V2-074), so opening a package here never
 * touches the repository (SPEC_V2 §8.6 "selecting a package... does not
 * dirty the repository").
 */

export interface PackageManagementDependencies {
  persistSelectedPackageId: typeof defaultPersistSelectedPackageId;
}

function defaultDependencies(): PackageManagementDependencies {
  return { persistSelectedPackageId: defaultPersistSelectedPackageId };
}

export interface PackageManagementPageProps {
  store: RepositoryWorkingCopyStore;
  /** The currently resolved/open package (device-wide `lastSelectedPackageId`). */
  selectedPackageId: string;
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
  /** Navigates to the Macros page for whatever package is selected after Open. */
  onOpenMacros: () => void;
  dependencies?: PackageManagementDependencies;
}

function validPackageName(name: string): boolean {
  return (
    name.trim().length > 0 &&
    utf8ByteLength(name) <= v2Limits.packageNameMaxBytes
  );
}

interface CreatePackageFormProps {
  onCreate: (name: string) => void;
}

function CreatePackageForm({
  onCreate,
}: CreatePackageFormProps): React.JSX.Element {
  const [name, setName] = useState("");
  const nameBytes = utf8ByteLength(name);
  const valid = validPackageName(name);

  return (
    <form
      className="grid gap-[0.85rem]"
      onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        if (!valid) {
          return;
        }
        onCreate(name);
        setName("");
      }}
    >
      <label htmlFor="package-management-create-name">
        New package name
        <input
          id="package-management-create-name"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setName(event.currentTarget.value);
          }}
          value={name}
        />
        <FieldHelp exceeded={nameBytes > v2Limits.packageNameMaxBytes}>
          {String(nameBytes)} / {String(v2Limits.packageNameMaxBytes)} UTF-8
          bytes
        </FieldHelp>
      </label>
      <button className="primary" disabled={!valid} type="submit">
        Create package
      </button>
    </form>
  );
}

interface PackageRowProps {
  pkg: RepositoryPackage;
  index: number;
  packageCount: number;
  isSelected: boolean;
  onOpen: () => void;
  onRename: (name: string) => void;
  onDuplicate: () => void;
  onDelete: () => void;
  onMove: (action: MoveAction) => void;
}

function PackageRow({
  pkg,
  index,
  packageCount,
  isSelected,
  onOpen,
  onRename,
  onDuplicate,
  onDelete,
  onMove,
}: PackageRowProps): React.JSX.Element {
  const [renaming, setRenaming] = useState(false);
  const [draftName, setDraftName] = useState(pkg.name);
  const [confirmingDelete, setConfirmingDelete] = useState(false);
  const draftValid = validPackageName(draftName);
  const confirmRef = useRef<HTMLDivElement>(null);
  // See `MacroOverflowMenu`'s identical `deleteButtonRef` for why an
  // explicit `restoreFocusRef` is needed: the "Delete" trigger is unmounted
  // (replaced by the confirmation panel) the same render that activates the
  // trap.
  const deleteButtonRef = useRef<HTMLButtonElement>(null);
  useFocusTrap({
    active: confirmingDelete,
    containerRef: confirmRef,
    onClose: () => {
      setConfirmingDelete(false);
    },
    restoreFocusRef: deleteButtonRef,
  });

  return (
    <Card variant="flush">
      <div className="min-w-0">
        <h3>
          {pkg.name}
          {isSelected ? " (selected)" : ""}
        </h3>
        <p>{String(pkg.macros.length)} macros</p>
        {renaming ? (
          <form
            className="grid gap-[0.85rem]"
            onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
              event.preventDefault();
              if (!draftValid) {
                return;
              }
              onRename(draftName);
              setRenaming(false);
            }}
          >
            <label htmlFor={`package-rename-${pkg.id}`}>
              Package name
              <input
                id={`package-rename-${pkg.id}`}
                onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                  setDraftName(event.currentTarget.value);
                }}
                value={draftName}
              />
            </label>
            <FormActions>
              <button className="primary" disabled={!draftValid} type="submit">
                Save name
              </button>
              <button
                onClick={() => {
                  setDraftName(pkg.name);
                  setRenaming(false);
                }}
                type="button"
              >
                Cancel
              </button>
            </FormActions>
          </form>
        ) : null}
      </div>
      <div className="flex flex-wrap content-start gap-[0.4rem] [&_button]:flex-initial max-[32rem]:w-full">
        <button
          aria-label={`Open ${pkg.name}`}
          className="primary"
          onClick={onOpen}
          type="button"
        >
          Open
        </button>
        {renaming ? null : (
          <button
            aria-label={`Rename ${pkg.name}`}
            onClick={() => {
              setDraftName(pkg.name);
              setRenaming(true);
            }}
            type="button"
          >
            Rename
          </button>
        )}
        <button
          aria-label={`Duplicate ${pkg.name}`}
          onClick={onDuplicate}
          type="button"
        >
          Duplicate
        </button>
        <div className="grid grid-cols-2 gap-2 max-[32rem]:grid-cols-1">
          <button
            aria-label={`Move ${pkg.name} to first`}
            disabled={index === 0}
            onClick={() => {
              onMove("first");
            }}
            type="button"
          >
            Move first
          </button>
          <button
            aria-label={`Move ${pkg.name} up`}
            disabled={index === 0}
            onClick={() => {
              onMove("up");
            }}
            type="button"
          >
            Move up
          </button>
          <button
            aria-label={`Move ${pkg.name} down`}
            disabled={index === packageCount - 1}
            onClick={() => {
              onMove("down");
            }}
            type="button"
          >
            Move down
          </button>
          <button
            aria-label={`Move ${pkg.name} to last`}
            disabled={index === packageCount - 1}
            onClick={() => {
              onMove("last");
            }}
            type="button"
          >
            Move last
          </button>
        </div>
        {confirmingDelete ? (
          <DangerZone containerRef={confirmRef} role="alertdialog">
            <p>
              Delete <strong>{pkg.name}</strong> and all{" "}
              {String(pkg.macros.length)} of its macros? This cannot be undone
              once the working copy is saved.
            </p>
            {isSelected ? (
              <p>
                This is your currently selected package — another package must
                be selected afterward.
              </p>
            ) : null}
            <button
              className="danger"
              onClick={() => {
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
          </DangerZone>
        ) : (
          <button
            aria-label={`Delete ${pkg.name}`}
            className="danger"
            onClick={() => {
              setConfirmingDelete(true);
            }}
            ref={deleteButtonRef}
            type="button"
          >
            Delete
          </button>
        )}
      </div>
    </Card>
  );
}

export function PackageManagementPage({
  store,
  selectedPackageId,
  persistedPackageId,
  onSelectionChange,
  onSelectionPersistenceFailure,
  onSelectionPersistenceSuccess,
  onOpenMacros,
  dependencies,
}: PackageManagementPageProps): React.JSX.Element {
  const deps = dependencies ?? defaultDependencies();
  const [snapshot, setSnapshot] = useState(() => store.getSnapshot());
  useEffect(() => store.subscribe(setSnapshot), [store]);
  const repository = snapshot.repository;

  const [search, setSearch] = useState("");

  const filtered = repository.packages.filter((pkg) =>
    pkg.name.toLowerCase().includes(search.trim().toLowerCase()),
  );

  const openPackage = async (packageId: string): Promise<void> => {
    const persistence = await tryPersistSelectedPackageId(
      packageId,
      persistedPackageId,
      deps.persistSelectedPackageId,
    );
    if (persistence.kind === "failed") {
      onSelectionPersistenceFailure(persistence.failure);
    } else {
      onSelectionPersistenceSuccess(packageId);
    }
    // Persistence is a device-wide preference, not repository content. Keep
    // the local selection usable even when the preference write failed; the
    // caller keeps the failure warning visible across this navigation.
    onSelectionChange(packageId);
    onOpenMacros();
  };

  const createPackage = (name: string): void => {
    store.applyContentChange(
      addPackage(repository, { id: createPackageId(), name, macros: [] }),
    );
  };

  const renamePackageRow = (packageId: string, name: string): void => {
    const pkg = repository.packages.find(
      (candidate) => candidate.id === packageId,
    );
    // Avoid a dirty transition after a no-op rename (TODO_V2 V2-102, same
    // rule V2-101 states for macro edits).
    if (pkg === undefined || pkg.name === name) {
      return;
    }
    store.applyContentChange(renamePackage(repository, packageId, name));
  };

  const duplicatePackageRow = (packageId: string): void => {
    const result = duplicatePackage(repository, packageId);
    if (result !== null) {
      store.applyContentChange(result.repository);
    }
  };

  const deletePackageRow = async (packageId: string): Promise<void> => {
    const next = deletePackage(repository, packageId);
    store.applyContentChange(next);
    if (packageId !== selectedPackageId) {
      return;
    }
    // UI_UX_SPEC_V2 §6.2 — resolve and persist the selection using the same
    // §3.6 algorithm startup uses, since the previously selected package no
    // longer exists.
    const resolution = resolveSelectedPackage(next, persistedPackageId);
    if (resolution.kind !== "resolved") {
      return;
    }
    if (resolution.shouldPersist) {
      const persistence = await tryPersistSelectedPackageId(
        resolution.packageId,
        persistedPackageId,
        deps.persistSelectedPackageId,
      );
      if (persistence.kind === "failed") {
        onSelectionPersistenceFailure(persistence.failure);
      } else {
        onSelectionPersistenceSuccess(resolution.packageId);
      }
    } else {
      // The local selection has returned to the already-durable preference;
      // any earlier warning is now stale even though no write was needed.
      onSelectionPersistenceSuccess(resolution.packageId);
    }
    onSelectionChange(resolution.packageId);
  };

  // TODO_V2 V2-133/UI_UX_SPEC_V2 §14: "Move first"/"Move last" alongside
  // "Move up"/"Move down" — see MacrosPage's identical rationale.
  const movePackageRow = (index: number, action: MoveAction): void => {
    const lastIndex = repository.packages.length - 1;
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
        ? movePackage(repository, index, action === "up" ? -1 : 1)
        : movePackageToIndex(repository, index, target);
    store.applyContentChange(nextRepository);
  };

  return (
    <section aria-labelledby="package-management-title">
      <PageHeading>
        <div>
          <h2 id="package-management-title">Packages</h2>
          <p>{String(repository.packages.length)} packages</p>
        </div>
      </PageHeading>

      <CreatePackageForm onCreate={createPackage} />

      <label htmlFor="package-management-search">
        Search packages
        <input
          id="package-management-search"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setSearch(event.currentTarget.value);
          }}
          type="search"
          value={search}
        />
      </label>

      {filtered.length === 0 ? (
        <p role="status">No packages match this search.</p>
      ) : (
        <ul className="my-4 grid list-none gap-3 p-0">
          {filtered.map((pkg) => {
            const index = repository.packages.findIndex(
              (candidate) => candidate.id === pkg.id,
            );
            return (
              <li key={pkg.id}>
                <PackageRow
                  index={index}
                  isSelected={pkg.id === selectedPackageId}
                  onDelete={() => {
                    void deletePackageRow(pkg.id);
                  }}
                  onDuplicate={() => {
                    duplicatePackageRow(pkg.id);
                  }}
                  onMove={(action) => {
                    movePackageRow(index, action);
                  }}
                  onOpen={() => {
                    void openPackage(pkg.id);
                  }}
                  onRename={(name) => {
                    renamePackageRow(pkg.id, name);
                  }}
                  packageCount={repository.packages.length}
                  pkg={pkg}
                />
              </li>
            );
          })}
        </ul>
      )}
    </section>
  );
}
