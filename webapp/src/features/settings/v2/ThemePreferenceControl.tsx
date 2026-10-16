import type { ThemePreference } from "../../../v2/themePreference";

export interface ThemePreferenceControlProps {
  value: ThemePreference;
  onChange: (preference: ThemePreference) => void;
}

const options: { value: ThemePreference; label: string }[] = [
  { value: "system", label: "System" },
  { value: "light", label: "Light" },
  { value: "dark", label: "Dark" },
];

export function ThemePreferenceControl({
  value,
  onChange,
}: ThemePreferenceControlProps): React.JSX.Element {
  return (
    <div className="mb-2 flex min-h-12 items-center justify-between gap-3 rounded-shape-md bg-surface-container px-3 py-1">
      <label className="shrink-0 text-title-sm" htmlFor="theme-preference">
        Appearance
      </label>
      <select
        className="max-w-36"
        id="theme-preference"
        name="theme-preference"
        onChange={(event) => {
          onChange(event.currentTarget.value as ThemePreference);
        }}
        value={value}
      >
        {options.map((option) => (
          <option key={option.value} value={option.value}>
            {option.label}
          </option>
        ))}
      </select>
    </div>
  );
}
