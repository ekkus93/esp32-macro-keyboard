import { describe, expect, test, vi } from "vitest";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import { buttonWithText, click, requiredElement } from "./render";
import {
  accepted,
  macroAId,
  macroBId,
  packageId,
  renderMacrosPage,
  statusAt,
  tick,
} from "./macrosPageHarness";

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
