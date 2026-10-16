/**
 * "Step 2 of 4: Wi-Fi" with a segmented bar. The text carries the meaning;
 * the bar is a decorative echo of it (`aria-hidden`), so progress is never
 * colour-only (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §3.3). Each segment is one
 * complete literal class string (no interpolation) so Tailwind sees it.
 */
export interface StepIndicatorProps {
  /** Zero-based index of the current step. */
  current: number;
  labels: readonly string[];
}

export function StepIndicator({
  current,
  labels,
}: StepIndicatorProps): React.JSX.Element {
  return (
    <div>
      <p className="m-0 text-label-lg text-on-surface-variant">
        Step {String(current + 1)} of {String(labels.length)}: {labels[current]}
      </p>
      <div aria-hidden="true" className="mt-2 flex gap-1">
        {labels.map((label, index) => (
          <span
            className={
              index <= current
                ? "h-1 flex-1 rounded-full bg-primary"
                : "h-1 flex-1 rounded-full bg-surface-container-highest"
            }
            key={label}
          />
        ))}
      </div>
    </div>
  );
}
