import { Icon } from "./Icon";
import type { IconName } from "./Icon";

/**
 * The M3 button family. Five variants, each one **complete literal class
 * string** (WEBAPP_TAILWIND_SPEC §8.2/§8.6): a template-built class is
 * invisible to Tailwind's scanner, and layered overrides would be resolved by
 * stylesheet order instead of by what this file says.
 *
 * Emphasis, highest to lowest: `filled` (the one primary action on a screen),
 * `tonal` (a secondary action), `outlined`, `text`, and `danger` (a
 * destructive primary). Every variant keeps the 48px touch floor the global
 * `button` rule sets, and shows its disabled state through the global
 * `button:disabled` rule.
 *
 * Bare `<button>` elsewhere in the app already renders as `tonal`; use this
 * component when a call site needs an icon or a non-default emphasis.
 */
const BUTTON_CLASS = {
  filled:
    "bg-primary px-6 text-on-primary shadow-none hover:shadow-elevation-1",
  tonal: "bg-secondary-container px-6 text-on-secondary-container",
  outlined:
    "border border-outline bg-transparent px-6 text-primary hover:bg-primary/8",
  text: "bg-transparent px-3 text-primary hover:bg-primary/8",
  danger: "bg-error px-6 text-on-error",
} as const;

export type ButtonVariant = keyof typeof BUTTON_CLASS;

export interface ButtonProps
  extends Omit<
    React.ButtonHTMLAttributes<HTMLButtonElement>,
    "children" | "className"
  > {
  children: React.ReactNode;
  icon?: IconName;
  variant?: ButtonVariant;
}

export function Button({
  children,
  icon,
  type = "button",
  variant = "tonal",
  ...rest
}: ButtonProps): React.JSX.Element {
  return (
    <button className={BUTTON_CLASS[variant]} type={type} {...rest}>
      {icon === undefined ? null : <Icon name={icon} size={18} />}
      {children}
    </button>
  );
}

/**
 * A button with no visible text. `label` is required and becomes the
 * accessible name; it is also the native tooltip (`title`), which makes the
 * control discoverable to pointer users (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE
 * §3.2: "every icon-only control needs an accessible name and a visible or
 * discoverable tooltip"). 48px square, comfortably over the 44px floor.
 */
export interface IconButtonProps
  extends Omit<
    React.ButtonHTMLAttributes<HTMLButtonElement>,
    "aria-label" | "children" | "className" | "title"
  > {
  icon: IconName;
  label: string;
}

export function IconButton({
  icon,
  label,
  type = "button",
  ...rest
}: IconButtonProps): React.JSX.Element {
  return (
    <button
      aria-label={label}
      className="size-12 bg-transparent p-0 text-on-surface-variant hover:bg-on-surface-variant/8"
      title={label}
      type={type}
      {...rest}
    >
      <Icon name={icon} />
    </button>
  );
}
