/**
 * The row of buttons that closes a form or a dialog. Replaces the
 * `.form-actions` rule; all 11 call sites were a bare `<div>` with nothing
 * else on them, so there is no variant and nothing to override.
 */
export interface FormActionsProps {
  children: React.ReactNode;
}

export function FormActions({ children }: FormActionsProps): React.JSX.Element {
  return <div className="mt-2 flex flex-wrap gap-[0.4rem]">{children}</div>;
}
