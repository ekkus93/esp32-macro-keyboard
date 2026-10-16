import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import {
  readThemePreference,
  THEME_PREFERENCE_STORAGE_KEY,
  writeThemePreference,
} from "../src/v2/themePreference";

beforeEach(() => {
  window.localStorage.clear();
});

afterEach(() => {
  vi.restoreAllMocks();
});

describe("theme preference", () => {
  test("uses System when no explicit preference is saved", () => {
    expect(readThemePreference()).toBe("system");
  });

  test.each(["light", "dark"] as const)(
    "persists and reads the %s choice",
    (preference) => {
      writeThemePreference(preference);
      expect(window.localStorage.getItem(THEME_PREFERENCE_STORAGE_KEY)).toBe(
        preference,
      );
      expect(readThemePreference()).toBe(preference);
    },
  );

  test("clearing an explicit choice returns to System", () => {
    writeThemePreference("dark");
    writeThemePreference("system");
    expect(readThemePreference()).toBe("system");
    expect(
      window.localStorage.getItem(THEME_PREFERENCE_STORAGE_KEY),
    ).toBeNull();
  });

  test("ignores invalid stored values", () => {
    window.localStorage.setItem(THEME_PREFERENCE_STORAGE_KEY, "sepia");
    expect(readThemePreference()).toBe("system");
  });

  test("falls back to an in-memory choice when storage is unavailable", () => {
    vi.spyOn(Storage.prototype, "getItem").mockImplementation(() => {
      throw new Error("Storage is blocked");
    });
    vi.spyOn(Storage.prototype, "setItem").mockImplementation(() => {
      throw new Error("Storage is blocked");
    });
    expect(readThemePreference()).toBe("system");
    expect(() => {
      writeThemePreference("dark");
    }).not.toThrow();
  });
});
