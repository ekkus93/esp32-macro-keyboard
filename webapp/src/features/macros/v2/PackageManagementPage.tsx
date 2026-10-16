import { useEffect, useRef, useState } from "react";
import { ConfirmDialog } from "../../../components/ConfirmDialog";
import { Dialog } from "../../../components/Dialog";
import { Button } from "../../../components/m3/Button";
import { Icon } from "../../../components/m3/Icon";
import { Menu } from "../../../components/m3/Menu";
import type { MenuItem } from "../../../components/m3/Menu";
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

/**
 * Packages above this count get a search field. With a handful, the whole
 * list is already on screen and a search box is only clutter.
 */
const SEARCH_MIN_PACKAGES = 6;

interface PackageNameDialogProps {
  confirmLabel: string;
  /** The text field's id (kept stable for tests and label association). */
  inputId: string;
  initialName: string;
  label: string;
  onCancel: () => void;
  onSubmit: (name: string) => void;
  title: string;
}

/**
 * The focused flow for naming a package -- used for both New package and
 * Rename. A modal dialog (`useFocusTrap`: Tab stays inside, Escape cancels,
 * focus returns to what opened it) holding one field with the same byte
 * counter the rest of the app uses.
 */
function PackageNameDialog({
  confirmLabel,
  inputId,
  initialName,
  label,
  onCancel,
  onSubmit,
  title,
}: PackageNameDialogProps): React.JSX.Element {
  const [name, setName] = useState(initialName);
  const containerRef = useRef<HTMLDivElement>(null);
  useFocusTrap({ active: true, containerRef, onClose: onCancel });
  const nameBytes = utf8ByteLength(name);
  const valid = validPackageName(name);

  return (
    <Dialog
      aria-labelledby={`dialog-title-${inputId}`}
      containerRef={containerRef}
      heading={<h2 id={`dialog-title-${inputId}`}>{title}</h2>}
      role="dialog"
    >
      <form
        className="grid gap-4"
        onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
          event.preventDefault();
          if (valid) {
            onSubmit(name);
          }
        }}
      >
        <div>
          <label htmlFor={inputId}>{label}</label>
          <input
            id={inputId}
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setName(event.currentTarget.value);
            }}
            value={name}
          />
          <FieldHelp exceeded={nameBytes > v2Limits.packageNameMaxBytes}>
            {String(nameBytes)} / {String(v2Limits.packageNameMaxBytes)} UTF-8
            bytes
          </FieldHelp>
        </div>
        <FormActions>
          <button onClick={onCancel} type="button">
            Cancel
          </button>
          <button className="primary" disabled={!valid} type="submit">
            {confirmLabel}
          </button>
        </FormActions>
      </form>
    </Dialog>
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

/**
 * One package: its name and size, a "Selected" mark when it is the open
 * package, a prominent Open, and a "More actions" menu for rename, duplicate,
 * reorder and delete (UXR-050; the same pattern as a macro row). Rename and
 * delete each open a focused dialog.
 */
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
  const [confirmingDelete, setConfirmingDelete] = useState(false);

  const isFirst = index === 0;
  const isLast = index === packageCount - 1;
  const items: readonly MenuItem[] = [
    {
      key: "rename",
      label: "Rename",
      ariaLabel: `Rename ${pkg.name}`,
      icon: "edit",
      onSelect: () => {
        setRenaming(true);
      },
    },
    {
      key: "duplicate",
      label: "Duplicate",
      ariaLabel: `Duplicate ${pkg.name}`,
      icon: "copy",
      onSelect: onDuplicate,
    },
    {
      key: "up",
      label: "Move up",
      ariaLabel: `Move ${pkg.name} up`,
      icon: "arrowUp",
      disabled: isFirst,
      onSelect: () => {
        onMove("up");
      },
    },
    {
      key: "down",
      label: "Move down",
      ariaLabel: `Move ${pkg.name} down`,
      icon: "arrowDown",
      disabled: isLast,
      onSelect: () => {
        onMove("down");
      },
    },
    {
      key: "first",
      label: "Move to top",
      ariaLabel: `Move ${pkg.name} to first`,
      icon: "arrowUp",
      disabled: isFirst,
      onSelect: () => {
        onMove("first");
      },
    },
    {
      key: "last",
      label: "Move to bottom",
      ariaLabel: `Move ${pkg.name} to last`,
      icon: "arrowDown",
      disabled: isLast,
      onSelect: () => {
        onMove("last");
      },
    },
    {
      key: "delete",
      label: "Delete",
      ariaLabel: `Delete ${pkg.name}`,
      icon: "trash",
      tone: "danger",
      onSelect: () => {
        setConfirmingDelete(true);
      },
    },
  ];

  return (
    <Card variant={isSelected ? "selected" : "flush"}>
      <div className="min-w-0">
        <h3>{pkg.name}</h3>
        <p className="flex flex-wrap items-center gap-x-3 gap-y-1">
          {String(pkg.macros.length)} macros
          {isSelected ? (
            <span className="inline-flex items-center gap-1 font-semibold">
              <Icon name="check" size={16} />
              Selected
            </span>
          ) : null}
        </p>
      </div>
      <div className="flex items-center gap-2">
        <button
          aria-label={`Open ${pkg.name}`}
          className={
            isSelected
              ? "flex-1 min-[34rem]:flex-none"
              : "primary flex-1 min-[34rem]:flex-none"
          }
          onClick={onOpen}
          type="button"
        >
          Open
        </button>
        <Menu
          items={items}
          menuLabel={`Actions for ${pkg.name}`}
          triggerLabel={`More actions for ${pkg.name}`}
        />
      </div>
      {renaming ? (
        <PackageNameDialog
          confirmLabel="Save name"
          initialName={pkg.name}
          inputId={`package-rename-${pkg.id}`}
          label="Package name"
          onCancel={() => {
            setRenaming(false);
          }}
          onSubmit={(name) => {
            setRenaming(false);
            onRename(name);
          }}
          title="Rename package"
        />
      ) : null}
      {confirmingDelete ? (
        <ConfirmDialog
          confirmLabel="Delete package"
          id={`delete-package-title-${pkg.id}`}
          onCancel={() => {
            setConfirmingDelete(false);
          }}
          onConfirm={() => {
            setConfirmingDelete(false);
            onDelete();
          }}
          title="Delete package?"
        >
          <p>
            Delete <strong>{pkg.name}</strong> and all{" "}
            {String(pkg.macros.length)} of its macros? This cannot be undone
            once the working copy is saved.
          </p>
          {isSelected ? (
            <p>
              This is your currently selected package — another package must be
              selected afterward.
            </p>
          ) : null}
        </ConfirmDialog>
      ) : null}
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
  const [creating, setCreating] = useState(false);

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
      <div className="mb-4 flex flex-wrap items-center gap-x-3 gap-y-2">
        <h2
          className="m-0 min-w-[8rem] flex-1 text-title-lg"
          id="package-management-title"
        >
          {String(repository.packages.length)} packages
        </h2>
        <Button
          icon="add"
          onClick={() => {
            setCreating(true);
          }}
          variant="filled"
        >
          New package
        </Button>
      </div>

      {repository.packages.length >= SEARCH_MIN_PACKAGES ? (
        <div className="mb-4">
          <label htmlFor="package-management-search">Search packages</label>
          <input
            id="package-management-search"
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setSearch(event.currentTarget.value);
            }}
            type="search"
            value={search}
          />
        </div>
      ) : null}

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

      {creating ? (
        <PackageNameDialog
          confirmLabel="Create package"
          initialName=""
          inputId="package-management-create-name"
          label="New package name"
          onCancel={() => {
            setCreating(false);
          }}
          onSubmit={(name) => {
            setCreating(false);
            createPackage(name);
          }}
          title="New package"
        />
      ) : null}
    </section>
  );
}
