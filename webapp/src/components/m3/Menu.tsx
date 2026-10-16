import { useCallback, useEffect, useRef, useState } from "react";
import { useDismissibleOverlay } from "../../features/shell/v2/useDismissibleOverlay";
import { Icon } from "./Icon";
import type { IconName } from "./Icon";

/**
 * An M3 menu opened by an icon button (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §8:
 * "Menus support keyboard navigation and Escape").
 *
 * Behaviour: the trigger is `aria-haspopup="menu"` with `aria-expanded`;
 * opening moves focus to the first enabled item; ArrowDown/ArrowUp move
 * between enabled items and wrap, Home/End jump to the ends, Escape and a
 * press outside close it (the shared `useDismissibleOverlay`), and Escape or
 * choosing an item returns focus to the trigger. Tab closes the menu and lets
 * focus move on naturally. Disabled items stay in the menu so a user can see
 * that "Move up" exists and is unavailable, rather than wondering where it
 * went; they are skipped by arrow navigation.
 *
 * Items select by calling `onSelect`; the menu closes first, so a handler may
 * open a dialog that takes focus without fighting the menu for it.
 */
export interface MenuItem {
  /** Accessible name when it must differ from the visible `label`. */
  ariaLabel?: string;
  disabled?: boolean;
  icon?: IconName;
  key: string;
  label: string;
  onSelect: () => void;
  tone?: "danger" | "default";
}

export interface MenuProps {
  items: readonly MenuItem[];
  /** Accessible name of the open menu. */
  menuLabel: string;
  /** Accessible name (and tooltip) of the trigger button. */
  triggerLabel: string;
}

const ITEM_CLASS = {
  default:
    "min-h-12 w-full justify-start gap-3 rounded-none bg-transparent px-4 py-2 text-left text-body-lg font-normal text-on-surface",
  danger:
    "min-h-12 w-full justify-start gap-3 rounded-none bg-transparent px-4 py-2 text-left text-body-lg font-normal text-error",
} as const;

export function Menu({
  items,
  menuLabel,
  triggerLabel,
}: MenuProps): React.JSX.Element {
  const [open, setOpen] = useState(false);
  const containerRef = useRef<HTMLDivElement>(null);
  const triggerRef = useRef<HTMLButtonElement>(null);
  const menuRef = useRef<HTMLDivElement>(null);

  const close = useCallback((): void => {
    setOpen(false);
  }, []);
  useDismissibleOverlay(open, containerRef, close);

  const enabledItems = (): HTMLButtonElement[] =>
    Array.from(
      menuRef.current?.querySelectorAll<HTMLButtonElement>(
        '[role="menuitem"]:not(:disabled)',
      ) ?? [],
    );

  // Focus the first enabled item when the menu opens.
  useEffect(() => {
    if (open) {
      enabledItems()[0]?.focus();
    }
  }, [open]);

  // Escape is owned by `useDismissibleOverlay`; return focus to the trigger
  // whenever the menu closes while focus was inside it.
  const closeAndRestoreFocus = (): void => {
    setOpen(false);
    triggerRef.current?.focus();
  };

  const onKeyDown = (event: React.KeyboardEvent<HTMLDivElement>): void => {
    const buttons = enabledItems();
    const index = buttons.findIndex((b) => b === document.activeElement);
    const move = (target: HTMLButtonElement | undefined): void => {
      event.preventDefault();
      target?.focus();
    };
    switch (event.key) {
      case "ArrowDown":
        move(buttons[(index + 1) % buttons.length]);
        break;
      case "ArrowUp":
        move(buttons[(index - 1 + buttons.length) % buttons.length]);
        break;
      case "Home":
        move(buttons[0]);
        break;
      case "End":
        move(buttons.at(-1));
        break;
      case "Escape":
        // `useDismissibleOverlay` closes it; just put focus back.
        triggerRef.current?.focus();
        break;
      case "Tab":
        setOpen(false);
        break;
      default:
        break;
    }
  };

  return (
    <div className="relative" ref={containerRef}>
      <button
        aria-expanded={open}
        aria-haspopup="menu"
        aria-label={triggerLabel}
        className="size-12 bg-transparent p-0 text-on-surface-variant hover:bg-on-surface-variant/8"
        onClick={() => {
          setOpen((current) => !current);
        }}
        ref={triggerRef}
        title={triggerLabel}
        type="button"
      >
        <Icon name="more" />
      </button>
      {open ? (
        <div
          aria-label={menuLabel}
          className="absolute top-full right-0 z-10 mt-1 grid min-w-[14rem] rounded-shape-md bg-surface-container-high py-2 shadow-elevation-2"
          onKeyDown={onKeyDown}
          ref={menuRef}
          role="menu"
        >
          {items.map((item) => (
            <button
              aria-label={item.ariaLabel}
              className={ITEM_CLASS[item.tone ?? "default"]}
              disabled={item.disabled}
              key={item.key}
              onClick={() => {
                closeAndRestoreFocus();
                item.onSelect();
              }}
              role="menuitem"
              type="button"
            >
              {item.icon === undefined ? null : (
                <Icon name={item.icon} size={20} />
              )}
              {item.label}
            </button>
          ))}
        </div>
      ) : null}
    </div>
  );
}
