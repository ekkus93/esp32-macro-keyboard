/**
 * M3 linear progress, indeterminate: a track with a bar sliding across it.
 * The slide is a CSS animation, so the global reduced-motion block (which
 * cuts every animation to ~0ms) leaves a static, partly filled bar -- the
 * `label` still says what is happening, so no information is motion-only
 * (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §3.4).
 *
 * `label` is required: a progressbar with no name is announced as nothing.
 * The keyframes are defined once in styles.css (`--animate-progress-slide`).
 */
export interface LinearProgressProps {
  label: string;
}

export function LinearProgress({
  label,
}: LinearProgressProps): React.JSX.Element {
  return (
    <div
      aria-label={label}
      className="h-1 w-full overflow-hidden rounded-full bg-primary-container"
      role="progressbar"
    >
      <div className="h-full w-2/5 animate-progress-slide rounded-full bg-primary" />
    </div>
  );
}

/**
 * Determinate linear progress: `value` of `max` complete. Exposed as a
 * `progressbar` with its numeric value, which assistive technology reads on
 * request but does not announce as it changes, so a once-per-poll update
 * stays visual (UXR-041). `max` of 0 renders an empty track.
 */
export interface DeterminateProgressProps {
  label: string;
  max: number;
  value: number;
}

export function DeterminateProgress({
  label,
  max,
  value,
}: DeterminateProgressProps): React.JSX.Element {
  const fraction = max > 0 ? Math.min(1, Math.max(0, value / max)) : 0;
  return (
    <div
      aria-label={label}
      aria-valuemax={max}
      aria-valuemin={0}
      aria-valuenow={value}
      className="h-1 w-full overflow-hidden rounded-full bg-surface-container-highest"
      role="progressbar"
    >
      <div
        className="h-full rounded-full bg-primary transition-[width] duration-300 ease-standard"
        style={{ width: `${String(Math.round(fraction * 100))}%` }}
      />
    </div>
  );
}
