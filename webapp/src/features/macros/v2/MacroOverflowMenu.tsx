import { useState } from "react";
import { ConfirmDialog } from "../../../components/ConfirmDialog";
import { Menu } from "../../../components/m3/Menu";
import type { MenuItem } from "../../../components/m3/Menu";
import type { RepositoryMacro } from "../../../v2/repository";
import type { MoveAction } from "./macroSendStatus";

/**
 * The "More actions" menu on a macro row (TODO_V2 V2-101; UI_UX_SPEC_V2 §5.1
 * "overflow actions such as Duplicate, Move, and Delete"; UXR-040). Reorder
 * (UI_UX_SPEC_V2 §14: accessible Move first/up/down/last), Duplicate and
 * Delete live here so a row's visible controls are Send and Edit. Every item
 * keeps the accessible name it always had ("Move X up", "Duplicate X",
 * "Delete X").
 *
 * Delete asks for an explicit confirmation that names the macro and the
 * consequence before it touches the working copy, matching the
 * destructive-target identification UI_UX_SPEC_V2 §6.2 requires. It is a
 * modal dialog (`useFocusTrap`: traps Tab, closes on Escape, restores focus to
 * the "More actions" button), no longer a panel nested inside the menu.
 */
export function MacroOverflowMenu({
  macro,
  index,
  macroCount,
  onMove,
  onDuplicate,
  onDelete,
}: {
  macro: RepositoryMacro;
  index: number;
  macroCount: number;
  onMove: (action: MoveAction) => void;
  onDuplicate: () => void;
  onDelete: () => void;
}): React.JSX.Element {
  const [confirmingDelete, setConfirmingDelete] = useState(false);
  const cancelDelete = (): void => {
    setConfirmingDelete(false);
  };

  const isFirst = index === 0;
  const isLast = index === macroCount - 1;
  const items: readonly MenuItem[] = [
    {
      key: "up",
      label: "Move up",
      ariaLabel: `Move ${macro.name} up`,
      icon: "arrowUp",
      disabled: isFirst,
      onSelect: () => {
        onMove("up");
      },
    },
    {
      key: "down",
      label: "Move down",
      ariaLabel: `Move ${macro.name} down`,
      icon: "arrowDown",
      disabled: isLast,
      onSelect: () => {
        onMove("down");
      },
    },
    {
      key: "first",
      label: "Move to top",
      ariaLabel: `Move ${macro.name} to first`,
      icon: "arrowUp",
      disabled: isFirst,
      onSelect: () => {
        onMove("first");
      },
    },
    {
      key: "last",
      label: "Move to bottom",
      ariaLabel: `Move ${macro.name} to last`,
      icon: "arrowDown",
      disabled: isLast,
      onSelect: () => {
        onMove("last");
      },
    },
    {
      key: "duplicate",
      label: "Duplicate",
      ariaLabel: `Duplicate ${macro.name}`,
      icon: "copy",
      onSelect: onDuplicate,
    },
    {
      key: "delete",
      label: "Delete",
      ariaLabel: `Delete ${macro.name}`,
      icon: "trash",
      tone: "danger",
      onSelect: () => {
        setConfirmingDelete(true);
      },
    },
  ];

  return (
    <>
      <Menu
        items={items}
        menuLabel={`Actions for ${macro.name}`}
        triggerLabel={`More actions for ${macro.name}`}
      />
      {confirmingDelete ? (
        <ConfirmDialog
          confirmLabel="Delete macro"
          id={`delete-macro-title-${macro.id}`}
          onCancel={cancelDelete}
          onConfirm={() => {
            setConfirmingDelete(false);
            onDelete();
          }}
          title="Delete macro?"
        >
          <p>
            <strong>{macro.name}</strong> will be removed from this package.
            This cannot be undone once the working copy is saved.
          </p>
        </ConfirmDialog>
      ) : null}
    </>
  );
}
