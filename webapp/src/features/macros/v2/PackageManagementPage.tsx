import { useEffect, useState } from "react";
import { v2Limits } from "../../../v2/limits";
import {
  persistSelectedPackageId as defaultPersistSelectedPackageId,
  resolveSelectedPackage,
} from "../../../v2/packageSelection";
import { utf8ByteLength, type RepositoryPackage } from "../../../v2/repository";
import {
  addPackage,
  createPackageId,
  deletePackage,
  duplicatePackage,
  movePackage,
  renamePackage,
} from "../../../v2/repositoryEditing";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";

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
      className="form-stack"
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
        <span
          className={
            nameBytes > v2Limits.packageNameMaxBytes
              ? "field-help limit-exceeded"
              : "field-help"
          }
        >
          {String(nameBytes)} / {String(v2Limits.packageNameMaxBytes)} UTF-8
          bytes
        </span>
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
  onMove: (direction: -1 | 1) => void;
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

  return (
    <article className="card management-card">
      <div>
        <h3>
          {pkg.name}
          {isSelected ? " (selected)" : ""}
        </h3>
        <p>{String(pkg.macros.length)} macros</p>
        {renaming ? (
          <form
            className="form-stack"
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
            <div className="form-actions">
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
            </div>
          </form>
        ) : null}
      </div>
      <div className="management-actions">
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
        <div className="reorder-actions">
          <button
            aria-label={`Move ${pkg.name} up`}
            disabled={index === 0}
            onClick={() => {
              onMove(-1);
            }}
            type="button"
          >
            Move up
          </button>
          <button
            aria-label={`Move ${pkg.name} down`}
            disabled={index === packageCount - 1}
            onClick={() => {
              onMove(1);
            }}
            type="button"
          >
            Move down
          </button>
        </div>
        {confirmingDelete ? (
          <div className="danger-zone" role="alertdialog">
            <p>
              Delete <strong>{pkg.name}</strong> and all{" "}
              {String(pkg.macros.length)} of its macros? This cannot be undone
              once the working copy is saved.
            </p>
            {isSelected ? (
              <p>
                This is your currently selected package — another package
                must be selected afterward.
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
          </div>
        ) : (
          <button
            aria-label={`Delete ${pkg.name}`}
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
    </article>
  );
}

export function PackageManagementPage({
  store,
  selectedPackageId,
  onSelectionChange,
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
    try {
      await deps.persistSelectedPackageId(packageId, selectedPackageId);
    } catch {
      // Best-effort device UI preference write (V2-074), matching every
      // other package-selection call site (RepositoryStartupScreen): the
      // chosen package still opens locally regardless of persistence
      // success — a persistence failure must not trap the user on this
      // screen.
    }
    onSelectionChange(packageId);
    onOpenMacros();
  };

  const createPackage = (name: string): void => {
    store.applyContentChange(
      addPackage(repository, { id: createPackageId(), name, macros: [] }),
    );
  };

  const renamePackageRow = (packageId: string, name: string): void => {
    const pkg = repository.packages.find((candidate) => candidate.id === packageId);
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

  const deletePackageRow = (packageId: string): void => {
    const next = deletePackage(repository, packageId);
    store.applyContentChange(next);
    if (packageId !== selectedPackageId) {
      return;
    }
    // UI_UX_SPEC_V2 §6.2 — resolve and persist the selection using the same
    // §3.6 algorithm startup uses, since the previously selected package no
    // longer exists.
    const resolution = resolveSelectedPackage(next, selectedPackageId);
    if (resolution.kind !== "resolved") {
      return;
    }
    if (resolution.shouldPersist) {
      deps
        .persistSelectedPackageId(resolution.packageId, selectedPackageId)
        .catch(() => {
          // Best-effort; see openPackage.
        });
    }
    onSelectionChange(resolution.packageId);
  };

  const movePackageRow = (index: number, direction: -1 | 1): void => {
    const target = index + direction;
    if (target < 0 || target >= repository.packages.length) {
      return;
    }
    store.applyContentChange(movePackage(repository, index, direction));
  };

  return (
    <section aria-labelledby="package-management-title">
      <div className="page-heading">
        <div>
          <h2 id="package-management-title">Packages</h2>
          <p>{String(repository.packages.length)} packages</p>
        </div>
      </div>

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
        <ul className="management-list">
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
                    deletePackageRow(pkg.id);
                  }}
                  onDuplicate={() => {
                    duplicatePackageRow(pkg.id);
                  }}
                  onMove={(direction) => {
                    movePackageRow(index, direction);
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
