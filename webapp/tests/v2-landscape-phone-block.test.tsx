import { act, useEffect } from "react";
import { describe, expect, test, vi } from "vitest";
import { LandscapeBlockSurface } from "../src/features/shell/v2/LandscapeBlockSurface";
import {
  landscapePhoneMediaQuery,
  useLandscapePhoneBlock,
} from "../src/features/shell/v2/useLandscapePhoneBlock";
import { installFakeMatchMedia } from "./fakeMatchMedia";
import { buttonWithText, click, render } from "./render";

/**
 * TODO_V2 V2-131 (UI_UX_SPEC_V2 §12.4): the exact tested criteria that
 * identify a phone-sized touch device in landscape.
 */
describe("useLandscapePhoneBlock — V2-131 device classification", () => {
  function Probe({
    onValue,
  }: {
    onValue: (value: boolean) => void;
  }): React.JSX.Element {
    const blocked = useLandscapePhoneBlock();
    useEffect(() => {
      onValue(blocked);
    }, [blocked, onValue]);
    return <p>{blocked ? "blocked" : "not blocked"}</p>;
  }

  test("uses the exact UI_UX_SPEC_V2 §12.4 media query", () => {
    expect(landscapePhoneMediaQuery).toBe(
      "(orientation: landscape) and (pointer: coarse) and (max-height: 600px)",
    );
  });

  test("reflects the initial matchMedia state on mount", async () => {
    const fakeMedia = installFakeMatchMedia();
    fakeMedia.set(landscapePhoneMediaQuery, true);
    const values: boolean[] = [];
    const { container, unmount } = await render(
      <Probe
        onValue={(value) => {
          values.push(value);
        }}
      />,
    );
    expect(container.textContent).toBe("blocked");
    expect(values).toContain(true);
    await unmount();
  });

  test("reacts to a later matchMedia change event without remounting", async () => {
    const fakeMedia = installFakeMatchMedia();
    const values: boolean[] = [];
    const { container, unmount } = await render(
      <Probe
        onValue={(value) => {
          values.push(value);
        }}
      />,
    );
    expect(container.textContent).toBe("not blocked");

    act(() => {
      fakeMedia.set(landscapePhoneMediaQuery, true);
    });
    expect(container.textContent).toBe("blocked");

    act(() => {
      fakeMedia.set(landscapePhoneMediaQuery, false);
    });
    expect(container.textContent).toBe("not blocked");

    // Both transitions were observed by the same mounted component instance
    // (no remount discarded the callback) — exactly the three values a
    // single subscriber sees: initial, blocked, unblocked.
    expect(values).toEqual([false, true, false]);
    await unmount();
  });

  test("stops listening after unmount", async () => {
    const fakeMedia = installFakeMatchMedia();
    const onValue = vi.fn();
    const { unmount } = await render(<Probe onValue={onValue} />);
    onValue.mockClear();
    await unmount();
    fakeMedia.set(landscapePhoneMediaQuery, true);
    expect(onValue).not.toHaveBeenCalled();
  });
});

describe("LandscapeBlockSurface — V2-131/V2-132 rotate-your-phone surface", () => {
  test("shows the exact UI_UX_SPEC_V2 §12.2 copy with no active send", async () => {
    const { container, unmount } = await render(
      <LandscapeBlockSurface activeSend={null} />,
    );
    expect(container.textContent).toContain("Rotate your phone");
    expect(container.textContent).toContain(
      "ESP32 Macro Keyboard is designed for portrait mode.",
    );
    expect(container.querySelector("button")).toBeNull();
    await unmount();
  });

  test("V2-132: shows macro name/progress and a working Cancel when a send is active", async () => {
    const onCancel = vi.fn();
    const { container, unmount } = await render(
      <LandscapeBlockSurface
        activeSend={{
          macroName: "Start the build",
          statusText: "Sending Start the build… action 1 of 2",
          onCancel,
        }}
      />,
    );
    expect(container.textContent).toContain(
      "Sending Start the build… action 1 of 2",
    );
    await click(buttonWithText("Cancel and release all keys"));
    expect(onCancel).toHaveBeenCalledOnce();
    await unmount();
  });

  test("omits Cancel while a send is starting and not yet cancellable", async () => {
    const { container, unmount } = await render(
      <LandscapeBlockSurface
        activeSend={{
          macroName: "Start the build",
          statusText: "Sending Start the build…",
          onCancel: null,
        }}
      />,
    );
    expect(container.textContent).toContain("Sending Start the build…");
    expect(container.querySelector("button")).toBeNull();
    await unmount();
  });
});
