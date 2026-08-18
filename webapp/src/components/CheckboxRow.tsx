/**
 * A `<label>` wrapping a checkbox or radio and its text. Replaces
 * `.checkbox-row`.
 *
 * `min-h-[44px]` is the touch-target floor `UI_UX_SPEC_V2` §13 requires — the
 * label is the hit area, not just the input — so it is part of the component
 * rather than something a call site can forget.
 */
export interface CheckboxRowProps {
  children: React.ReactNode;
  htmlFor?: string;
}

export function CheckboxRow({
  children,
  htmlFor,
}: CheckboxRowProps): React.JSX.Element {
  return (
    <label
      className="flex min-h-[44px] items-center gap-[0.65rem]"
      htmlFor={htmlFor}
    >
      {children}
    </label>
  );
}
