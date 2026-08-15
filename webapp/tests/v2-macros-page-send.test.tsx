import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import type { ActiveSendSummary } from "../src/features/shell/v2/activeSendSummary";
import {
  MacrosPage,
  type MacrosPageDependencies,
} from "../src/features/macros/v2/MacrosPage";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { SendStatusResponse } from "../src/v2/apiTypes";
import { V2ApiError } from "../src/v2/apiClient";
import type { SendMacroCallbacks, SendMacroHandle } from "../src/v2/sendClient";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
} from "./render";
import {
  accepted,
  makeRepository,
  packageId,
  renderMacrosPage,
  sendId,
  statusAt,
  tick,
} from "./macrosPageHarness";

describe("MacrosPage — V2-093 Quick Send", () => {
  test("Send issues exactly one POST, shows inline progress, and an acknowledgement that clears itself", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage();

      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);
      expect(container.textContent).toContain("Sending Start the build…");

      planJsonResponse(statusAt("running", 1));
      await tick(1000);
      expect(container.textContent).toContain(
        "Sending Start the build… action 1 of 2",
      );

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      expect(container.textContent).toContain("Sent Start the build.");

      await tick(5000);
      expect(container.textContent).not.toContain("Sent Start the build.");

      const postCalls = getFetchCalls().filter(
        (call) => call.method === "POST",
      );
      expect(postCalls).toHaveLength(1);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("disables the other Send controls while a send is active", async () => {
    vi.useFakeTimers();
    try {
      const { unmount } = await renderMacrosPage();
      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);

      const otherSend = requiredElement(
        '[aria-label="Send Open terminal"]',
        HTMLButtonElement,
      );
      expect(otherSend.disabled).toBe(true);

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("shows the serial-confirmation waiting state inline", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage();
      planJsonResponse({ ...accepted, state: "awaiting_confirmation" }, 202);
      await click(buttonWithText("Send"));
      await tick(0);
      expect(container.textContent).toContain(
        "Waiting for confirmation on the device to send Start the build… Run confirm in the device serial console to continue.",
      );

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("awaiting confirmation stays cancellable and confirmation polling never reposts", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage();
      planJsonResponse({ ...accepted, state: "awaiting_confirmation" }, 202);
      await click(buttonWithText("Send"));
      await tick(0);

      expect(container.textContent).toContain(
        "Run confirm in the device serial console",
      );
      expect(buttonWithText("Cancel and release all keys")).toBeDefined();

      planJsonResponse(statusAt("awaiting_confirmation", 0));
      await tick(1000);
      planJsonResponse(statusAt("running", 0));
      await tick(1000);

      const postCalls = getFetchCalls().filter(
        (call) => call.method === "POST" && call.url === "/api/v1/send",
      );
      expect(postCalls).toHaveLength(1);

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("Cancel and release all keys calls DELETE /api/v1/send", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage();
      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);

      planFetch((call) => {
        expect(call.method).toBe("DELETE");
        expect(call.url).toBe("/api/v1/send");
        return jsonResponse({ id: sendId }, 202);
      });
      await click(buttonWithText("Cancel and release all keys"));

      planJsonResponse(
        statusAt("cancelled", 1, { cancellationRequested: true }),
      );
      await tick(1000);
      expect(container.textContent).toContain(
        "Send Start the build was cancelled.",
      );
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test.each([
    ["cancelled" as const, "Send Start the build was cancelled."],
    ["timed_out" as const, "Send Start the build timed out."],
    ["failed" as const, "Send Start the build failed: usb_not_ready"],
  ])(
    "persists a %s acknowledgement until dismissed",
    async (state, expectedText) => {
      vi.useFakeTimers();
      try {
        const { container, unmount } = await renderMacrosPage();
        planJsonResponse(accepted, 202);
        await click(buttonWithText("Send"));
        await tick(0);

        planJsonResponse(statusAt(state, 1, { error: "usb_not_ready" }));
        await tick(1000);
        expect(container.textContent).toContain(expectedText);

        await tick(10_000);
        expect(container.textContent).toContain(expectedText);

        await click(buttonWithText("Dismiss"));
        expect(container.textContent).not.toContain(expectedText);
        await unmount();
      } finally {
        vi.useRealTimers();
      }
    },
  );

  test("reports a release error separately and persistently", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage();
      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);

      planJsonResponse(
        statusAt("completed", 2, { releaseError: "stuck_key_left_ctrl" }),
      );
      await tick(1000);
      expect(container.textContent).toContain(
        "Key release failed: stuck_key_left_ctrl",
      );

      await tick(10_000);
      expect(container.textContent).toContain(
        "Key release failed: stuck_key_left_ctrl",
      );
      await click(buttonWithText("Dismiss"));
      expect(container.textContent).not.toContain("Key release failed");
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("never includes macro source in the acknowledgement", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage();
      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);
      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      expect(container.textContent).toContain("Sent Start the build.");
      expect(container.textContent).not.toContain("make -j8{ENTER}");
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });
});

describe("MacrosPage — V2-132 landscape active-send summary", () => {
  test("reports the starting summary before the device responds", async () => {
    // `sendMacro` never resolves in this test, deliberately, to observe the
    // "starting" summary in isolation — with the real dependencies (or the
    // fake-fetch harness), the mocked `POST` resolves fast enough that a
    // React `act()` flush already reaches "active" before any assertion can
    // run, per the other test in this block.
    const store = createRepositoryWorkingCopyStore(makeRepository());
    const summaries: (ActiveSendSummary | null)[] = [];
    const { unmount } = await render(
      <MacrosPage
        dependencies={{
          sendMacro: vi.fn(() => new Promise<never>(() => undefined)),
          trackSend: vi.fn(() => ({ stop: vi.fn() })),
          cancelSend: vi.fn(() => Promise.resolve(undefined)),
          recoverSendState: vi.fn(() => Promise.resolve(null)),
        }}
        initialSend={null}
        onActiveSendChange={(summary) => {
          summaries.push(summary);
        }}
        onChangePackage={vi.fn()}
        onOpenAddMacro={vi.fn()}
        onOpenEditMacro={vi.fn()}
        onOpenPreview={vi.fn()}
        packageId={packageId}
        sendMode="quick"
        showMacroSourcePreviews={false}
        store={store}
        usbState="ready"
      />,
    );
    expect(summaries.at(-1)).toBeNull();

    await click(buttonWithText("Send"));
    const starting = summaries.at(-1);
    expect(starting).not.toBeNull();
    expect(starting?.macroName).toBe("Start the build");
    expect(starting?.statusText).toBe("Sending Start the build…");
    expect(starting?.onCancel).toBeNull();
    await unmount();
  });

  test("reports active progress, then null once the completion acknowledgement clears", async () => {
    vi.useFakeTimers();
    try {
      const summaries: (ActiveSendSummary | null)[] = [];
      const onActiveSendChange = vi.fn((summary: ActiveSendSummary | null) => {
        summaries.push(summary);
      });
      const { unmount } = await renderMacrosPage({ onActiveSendChange });
      expect(summaries.at(-1)).toBeNull();

      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);
      // By this point the mocked POST has already resolved (see the
      // deliberately-never-resolving test above for the "starting" summary
      // in isolation) — the summary already reflects "active".
      const startedActive = summaries.at(-1);
      expect(startedActive).not.toBeNull();
      expect(startedActive?.macroName).toBe("Start the build");
      expect(startedActive?.onCancel).not.toBeNull();

      planJsonResponse(statusAt("running", 1));
      await tick(1000);
      const active = summaries.at(-1);
      expect(active?.statusText).toBe("Sending Start the build… action 1 of 2");
      expect(active?.onCancel).not.toBeNull();

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      // "completed" is the brief acknowledgement, not "awaiting confirmation
      // or running" (UI_UX_SPEC_V2 §12.3) — nothing left to cancel.
      expect(summaries.at(-1)).toBeNull();
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("the reported onCancel issues the same DELETE as the inline Cancel button", async () => {
    vi.useFakeTimers();
    try {
      const summaries: (ActiveSendSummary | null)[] = [];
      const { unmount } = await renderMacrosPage({
        onActiveSendChange: (summary) => {
          summaries.push(summary);
        },
      });
      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);
      planJsonResponse(statusAt("running", 1));
      await tick(1000);

      planFetch((call) => {
        expect(call.method).toBe("DELETE");
        expect(call.url).toBe("/api/v1/send");
        return jsonResponse({ id: sendId }, 202);
      });
      const summary = summaries.at(-1);
      expect(summary).not.toBeNull();
      await act(async () => {
        summary?.onCancel?.();
        await Promise.resolve();
      });

      planJsonResponse(
        statusAt("cancelled", 1, { cancellationRequested: true }),
      );
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("never reports a summary when the caller does not ask for one", async () => {
    vi.useFakeTimers();
    try {
      const { unmount } = await renderMacrosPage();
      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);
      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });
});

describe("MacrosPage — send tracker lifetime", () => {
  function baseDependencies(): MacrosPageDependencies {
    return {
      sendMacro: vi.fn(() =>
        Promise.resolve({
          accepted,
          cancel: vi.fn(() => Promise.resolve(undefined)),
          stop: vi.fn(),
        }),
      ),
      trackSend: vi.fn(() => ({ stop: vi.fn() })),
      cancelSend: vi.fn(() => Promise.resolve(undefined)),
      recoverSendState: vi.fn(() => Promise.resolve(null)),
    };
  }

  test("stops the reload-recovery tracker on unmount", async () => {
    const stop = vi.fn();
    const dependencies = baseDependencies();
    dependencies.trackSend = vi.fn(() => ({ stop }));

    const { unmount } = await renderMacrosPage({
      dependencies,
      initialSend: statusAt("running", 1),
    });
    expect(dependencies.trackSend).toHaveBeenCalledOnce();

    await unmount();
    expect(stop).toHaveBeenCalledOnce();
  });

  test("stops the 409-recovery tracker on unmount", async () => {
    const stop = vi.fn();
    const dependencies = baseDependencies();
    dependencies.sendMacro = vi.fn(() => {
      throw new V2ApiError(409, {
        code: "already_sending",
        message: "A send is already in progress.",
      });
    });
    dependencies.recoverSendState = vi.fn(() =>
      Promise.resolve(statusAt("running", 1)),
    );
    dependencies.trackSend = vi.fn(() => ({ stop }));

    const { unmount } = await renderMacrosPage({ dependencies });
    await click(buttonWithText("Send"));
    await flushReact();
    expect(dependencies.recoverSendState).toHaveBeenCalledOnce();
    expect(dependencies.trackSend).toHaveBeenCalledOnce();

    await unmount();
    expect(stop).toHaveBeenCalledOnce();
  });

  test("stops a newly-started send tracker on unmount", async () => {
    const stop = vi.fn();
    const dependencies = baseDependencies();
    dependencies.sendMacro = vi.fn(() =>
      Promise.resolve({
        accepted,
        cancel: vi.fn(() => Promise.resolve(undefined)),
        stop,
      }),
    );

    const { unmount } = await renderMacrosPage({ dependencies });
    await click(buttonWithText("Send"));
    await flushReact();
    expect(dependencies.sendMacro).toHaveBeenCalledOnce();

    await unmount();
    expect(stop).toHaveBeenCalledOnce();
  });

  test("surfaces a tracker failure without hiding the active send", async () => {
    const dependencies = baseDependencies();
    let trackerCallbacks: SendMacroCallbacks | undefined;
    dependencies.trackSend = vi.fn(
      (_seed: SendStatusResponse, callbacks?: SendMacroCallbacks) => {
        trackerCallbacks = callbacks;
        return { stop: vi.fn() };
      },
    );

    const { unmount } = await renderMacrosPage({
      dependencies,
      initialSend: statusAt("running", 1),
    });
    await act(async () => {
      trackerCallbacks?.onError?.(new TypeError("network unavailable"));
      await Promise.resolve();
    });

    expect(document.body.textContent).toContain(
      "Send status tracking failed: network unavailable",
    );
    expect(() => buttonWithText("Cancel and release all keys")).not.toThrow();
    await unmount();
  });

  test("stops a send handle that resolves after unmount", async () => {
    const stop = vi.fn();
    let resolveSend: ((handle: SendMacroHandle) => void) | undefined;
    const pendingSend = new Promise<SendMacroHandle>((resolve) => {
      resolveSend = resolve;
    });
    const dependencies = baseDependencies();
    dependencies.sendMacro = vi.fn(() => pendingSend);

    const { unmount } = await renderMacrosPage({ dependencies });
    await click(buttonWithText("Send"));
    expect(dependencies.sendMacro).toHaveBeenCalledOnce();
    await unmount();

    resolveSend?.({
      accepted,
      cancel: vi.fn(() => Promise.resolve(undefined)),
      stop,
    });
    await flushReact();
    expect(stop).toHaveBeenCalledOnce();
  });
});
