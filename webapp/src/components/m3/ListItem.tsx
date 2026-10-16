import { Icon } from "./Icon";
import type { IconName } from "./Icon";

/**
 * An M3 list item that is one whole tap target: a headline, optional
 * supporting text, and a trailing chevron. Used where a row's only action is
 * "choose this" (package chooser, snapshot recovery); rows that carry several
 * actions use `Card`.
 *
 * `label` overrides the accessible name when the visible text split across
 * headline and supporting text would read badly aloud or must stay stable
 * for assistive tech and tests.
 */
export interface ListItemButtonProps {
  disabled?: boolean;
  headline: string;
  label?: string;
  leadingIcon?: IconName;
  onClick: () => void;
  supporting?: string;
}

export function ListItemButton({
  disabled = false,
  headline,
  label,
  leadingIcon,
  onClick,
  supporting,
}: ListItemButtonProps): React.JSX.Element {
  return (
    <button
      aria-label={label}
      className="w-full justify-start gap-4 rounded-shape-md bg-surface-container px-4 py-3 text-left text-body-lg font-normal text-on-surface"
      disabled={disabled}
      onClick={onClick}
      type="button"
    >
      {leadingIcon === undefined ? null : (
        <span className="grid size-10 shrink-0 place-items-center rounded-full bg-primary-container text-on-primary-container">
          <Icon name={leadingIcon} />
        </span>
      )}
      <span className="min-w-0 flex-1">
        <span className="block truncate text-title-md">{headline}</span>
        {supporting === undefined ? null : (
          <span className="block truncate text-body-md text-on-surface-variant">
            {supporting}
          </span>
        )}
      </span>
      <Icon name="chevronRight" />
    </button>
  );
}
