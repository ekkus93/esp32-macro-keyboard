import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { MacrosPage } from "../src/features/macros/v2/MacrosPage";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { SendStatusResponse } from "../src/v2/apiTypes";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";
import { buttonWithText, click, render, requiredElement } from "./render";

/** Timer-driven React updates (poll callbacks, the completion-ack timeout)
 * must be flushed inside `act` or React warns and the state update can be
 * observed after the assertion rather than before it. */
async function tick(ms: number): Promise<void> {
  await act(async () => {
    await vi.advanceTimersByTimeAsync(ms);
  });
}

const packageId = "550e8400-e29b-41d4-a716-446655440000";
const macroAId = "6ba7b810-9dad-41d1-80b4-00c04fd430c8";
const macroBId = "6ba7b810-9dad-41d1-80b4-00c04fd430c9";
const sendId = "11111111-1111-4111-8111-111111111111";

function makeRepository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [
      {
        id: packageId,
        name: "Build server login",
        macros: [
          {
            id: macroAId,
            name: "Start the build",
            source: "make -j8{ENTER}",
            keyPressMs: 8,
            interKeyMs: 15,
          },
          {
            id: macroBId,
            name: "Open terminal",
            source: "{CTRL+ALT+T}",
            keyPressMs: 8,
            interKeyMs: 15,
          },
        ],
      },
    ],
  };
}

function statusAt(
  state: SendStatusResponse["state"],
  actionIndex: number,
  overrides: Partial<SendStatusResponse> = {},
): SendStatusResponse {
  return {
    id: sendId,
    state,
    actionIndex,
    actionCount: 2,
    estimatedDurationMs: 100,
    cancellationRequested: false,
    error: "",
    releaseError: "",
    ...overrides,
  };
}

const accepted = {
  id: sendId,
  state: "running" as const,
  actionCount: 2,
  estimatedDurationMs: 100,
};

interface RenderOptions {
  usbState?: "ready" | "disconnected";
  initialSend?: SendStatusResponse | null;
  showMacroSourcePreviews?: boolean;
  sendMode?: "quick" | "preview";
}

async function renderMacrosPage(options: RenderOptions = {}) {
  const store = createRepositoryWorkingCopyStore(makeRepository());
  const callbacks = {
    onChangePackage: vi.fn(),
    onOpenPreview: vi.fn(),
    onOpenAddMacro: vi.fn(),
    onOpenEditMacro: vi.fn(),
  };
  const result = await render(
    <MacrosPage
      initialSend={options.initialSend ?? null}
      onChangePackage={callbacks.onChangePackage}
      onOpenAddMacro={callbacks.onOpenAddMacro}
      onOpenEditMacro={callbacks.onOpenEditMacro}
      onOpenPreview={callbacks.onOpenPreview}
      packageId={packageId}
      sendMode={options.sendMode ?? "quick"}
      showMacroSourcePreviews={options.showMacroSourcePreviews ?? false}
      store={store}
      usbState={options.usbState ?? "ready"}
    />,
  );
  return { ...result, store, callbacks };
}

describe("MacrosPage — V2-091 macro list", () => {
  test("shows ordered macros, macro count, and Add macro", async () => {
    const { container, callbacks } = await renderMacrosPage();
    expect(container.textContent).toContain("Build server login");
    expect(container.textContent).toContain("2 macros");
    const names = Array.from(container.querySelectorAll("h3")).map(
      (element) => element.textContent,
    );
    expect(names).toEqual(["Start the build", "Open terminal"]);

    await click(buttonWithText("Add macro"));
    expect(callbacks.onOpenAddMacro).toHaveBeenCalledOnce();

    await click(buttonWithText("Change"));
    expect(callbacks.onChangePackage).toHaveBeenCalledOnce();
  });

  test("Send is disabled unless USB is ready", async () => {
    const { container } = await renderMacrosPage({ usbState: "disconnected" });
    const sendButtons = Array.from(
      container.querySelectorAll<HTMLButtonElement>('[aria-label^="Send "]'),
    );
    expect(sendButtons).toHaveLength(2);
    for (const button of sendButtons) {
      expect(button.disabled).toBe(true);
    }
  });

  test("accessible reordering moves a macro and announces the move", async () => {
    const { container, store } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="Move Start the build down"]',
        HTMLButtonElement,
      ),
    );
    const firstPackage = store
      .getRepository()
      .packages.find((pkg) => pkg.id === packageId);
    expect(firstPackage).toBeDefined();
    const names = firstPackage?.macros.map((macro) => macro.name);
    expect(names).toEqual(["Open terminal", "Start the build"]);
    expect(store.getIsDirty()).toBe(true);
    expect(container.textContent).toContain(
      "Moved Start the build to position 2.",
    );
  });

  test("Edit calls onOpenEditMacro with exactly the clicked row's macro ID", async () => {
    const { callbacks } = await renderMacrosPage();
    await click(
      requiredElement('[aria-label="Edit Open terminal"]', HTMLButtonElement),
    );
    expect(callbacks.onOpenEditMacro).toHaveBeenCalledExactlyOnceWith(
      macroBId,
    );
  });

  test("the first row cannot move up and the last row cannot move down", async () => {
    await renderMacrosPage();
    const moveUpFirst = requiredElement(
      '[aria-label="Move Start the build up"]',
      HTMLButtonElement,
    );
    const moveDownLast = requiredElement(
      '[aria-label="Move Open terminal down"]',
      HTMLButtonElement,
    );
    expect(moveUpFirst.disabled).toBe(true);
    expect(moveDownLast.disabled).toBe(true);
  });
});

describe("MacrosPage — V2-092 macro-source privacy", () => {
  test("hides source by default behind a non-revealing placeholder", async () => {
    const { container } = await renderMacrosPage();
    expect(container.textContent).not.toContain("make -j8{ENTER}");
    expect(container.textContent).toContain("Source hidden");
  });

  test("a temporary per-row reveal shows only that row's source", async () => {
    const { container } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="Reveal source for Start the build"]',
        HTMLButtonElement,
      ),
    );
    expect(container.textContent).toContain("make -j8{ENTER}");
    expect(container.textContent).not.toContain("{CTRL+ALT+T}");

    await click(
      requiredElement(
        '[aria-label="Hide source for Start the build"]',
        HTMLButtonElement,
      ),
    );
    expect(container.textContent).not.toContain("make -j8{ENTER}");
  });

  test("honors the device-wide show-source-previews preference", async () => {
    const { container } = await renderMacrosPage({
      showMacroSourcePreviews: true,
    });
    expect(container.textContent).toContain("make -j8{ENTER}");
    expect(container.textContent).toContain("{CTRL+ALT+T}");
  });
});

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
        "Waiting for physical confirmation on the device to send Start the build…",
      );

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

describe("MacrosPage — V2-095 reload and race handling", () => {
  test("prevents double-send on rapid repeated taps", async () => {
    vi.useFakeTimers();
    try {
      const { unmount } = await renderMacrosPage();
      planJsonResponse(accepted, 202);
      const button = buttonWithText("Send");
      await click(button);
      await click(button);
      await click(button);
      await tick(0);

      const postCalls = getFetchCalls().filter(
        (call) => call.method === "POST",
      );
      expect(postCalls).toHaveLength(1);

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("recovers and resumes tracking a non-terminal send after reload, without posting", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage({
        initialSend: statusAt("running", 1),
      });
      expect(container.textContent).toContain("action 1 of 2");
      expect(
        getFetchCalls().filter((call) => call.method === "POST"),
      ).toHaveLength(0);

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      expect(container.textContent).toContain("Sent.");
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("restores an undismissed terminal-issue banner after reload", async () => {
    const { container, unmount } = await renderMacrosPage({
      initialSend: statusAt("failed", 1, { error: "usb_not_ready" }),
    });
    expect(container.textContent).toContain("Send failed: usb_not_ready");
    await unmount();
  });

  test("does not resurrect a stale completed acknowledgement after reload", async () => {
    const { container, unmount } = await renderMacrosPage({
      initialSend: statusAt("completed", 2),
    });
    expect(container.textContent).not.toContain("Sent");
    await unmount();
  });

  test("handles 409 by showing the actual current send instead of failing silently", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage();
      planJsonResponse(
        {
          error: {
            code: "already_sending",
            message: "A send is already in progress.",
          },
        },
        409,
      );
      planJsonResponse(statusAt("running", 1));
      await click(buttonWithText("Send"));
      await tick(0);
      expect(container.textContent).toContain("action 1 of 2");

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });
});

describe("MacrosPage — V2-101 overflow menu (Duplicate/Delete)", () => {
  test("Duplicate adds a copy immediately after the original and dirties the working copy", async () => {
    const { container, store, unmount } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="More actions for Start the build"]',
        HTMLButtonElement,
      ),
    );
    await click(
      requiredElement(
        '[aria-label="Duplicate Start the build"]',
        HTMLButtonElement,
      ),
    );

    const pkg = store
      .getRepository()
      .packages.find((candidate) => candidate.id === packageId);
    expect(pkg?.macros.map((m) => m.name)).toEqual([
      "Start the build",
      "Start the build copy",
      "Open terminal",
    ]);
    expect(store.getIsDirty()).toBe(true);
    expect(container.textContent).toContain("Start the build copy");
    await unmount();
  });

  test("Delete requires an explicit, name-bearing confirmation before it changes anything", async () => {
    const { store, unmount } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="More actions for Start the build"]',
        HTMLButtonElement,
      ),
    );
    await click(
      requiredElement(
        '[aria-label="Delete Start the build"]',
        HTMLButtonElement,
      ),
    );

    // Not yet deleted, and not yet dirty — only a confirmation panel is showing.
    expect(store.getIsDirty()).toBe(false);
    expect(
      store
        .getRepository()
        .packages.find((candidate) => candidate.id === packageId)?.macros,
    ).toHaveLength(2);

    await click(buttonWithText("Cancel"));
    expect(store.getIsDirty()).toBe(false);
    await unmount();
  });

  test("Confirming delete removes exactly the targeted macro and dirties the working copy", async () => {
    const { store, unmount } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="More actions for Start the build"]',
        HTMLButtonElement,
      ),
    );
    await click(
      requiredElement(
        '[aria-label="Delete Start the build"]',
        HTMLButtonElement,
      ),
    );
    await click(buttonWithText("Confirm delete"));

    const pkg = store
      .getRepository()
      .packages.find((candidate) => candidate.id === packageId);
    expect(pkg?.macros.map((m) => m.id)).toEqual([macroBId]);
    expect(store.getIsDirty()).toBe(true);
    await unmount();
  });

  test("the overflow menu still offers Preview and send", async () => {
    const { callbacks, unmount } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="More actions for Start the build"]',
        HTMLButtonElement,
      ),
    );
    await click(
      requiredElement(
        '[aria-label="Preview and send Start the build"]',
        HTMLButtonElement,
      ),
    );
    expect(callbacks.onOpenPreview).toHaveBeenCalledWith(macroAId);
    await unmount();
  });
});

describe("MacrosPage — V2-094 honoring Always Preview", () => {
  test("sendMode 'preview' routes the primary Send control to Preview and send instead of quick-sending", async () => {
    const { callbacks, unmount } = await renderMacrosPage({
      sendMode: "preview",
    });
    await click(buttonWithText("Send"));
    expect(callbacks.onOpenPreview).toHaveBeenCalledWith(macroAId);
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(0);
    await unmount();
  });

  test("sendMode 'quick' still sends directly without opening preview", async () => {
    vi.useFakeTimers();
    try {
      const { callbacks, unmount } = await renderMacrosPage({
        sendMode: "quick",
      });
      planJsonResponse(accepted, 202);
      await click(buttonWithText("Send"));
      await tick(0);
      expect(callbacks.onOpenPreview).not.toHaveBeenCalled();
      expect(
        getFetchCalls().filter((call) => call.method === "POST"),
      ).toHaveLength(1);
      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });
});
