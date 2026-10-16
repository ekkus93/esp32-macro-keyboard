export const THEME_PREFERENCE_STORAGE_KEY = "esp32-macro-keyboard.theme";

export type ThemePreference = "system" | "light" | "dark";

export function readThemePreference(): ThemePreference {
  try {
    const saved = window.localStorage.getItem(THEME_PREFERENCE_STORAGE_KEY);
    if (saved === "light" || saved === "dark") {
      return saved;
    }
  } catch {
    // Browsers can disable storage; System mode still works for this session.
  }
  return "system";
}

export function writeThemePreference(preference: ThemePreference): void {
  try {
    if (preference === "system") {
      window.localStorage.removeItem(THEME_PREFERENCE_STORAGE_KEY);
    } else {
      window.localStorage.setItem(THEME_PREFERENCE_STORAGE_KEY, preference);
    }
  } catch {
    // Keep the in-memory theme choice when browser storage is unavailable.
  }
}
