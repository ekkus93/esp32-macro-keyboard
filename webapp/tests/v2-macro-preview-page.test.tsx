import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { MacroPreviewPage } from "../src/features/macros/v2/MacroPreviewPage";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import { buttonWithText, click, render } from "./render";

async function tick(ms: number): Promise<void> {
  await act(async () => {
    await vi.advanceTimersByTimeAsync(ms);
  });
}

const packageId = "550e8400-e29b-41d4-a716-446655440000";
const macroId = "6ba7b810-9dad-41d1-80b4-00c04fd430c8";
const sendId = "11111111-1111-4111-8111-111111111111";

const repository: Repository = {
  format: "esp32-macro-keyboard-repository",
  schemaVersion: 1,
  packages: [
    {
      id: packageId,
      name: "Build server login",
      macros: [
        {
          id: macroId,
          name: "Start the build",
          source: "make -j8{ENTER}",
          keyPressMs: 8,
          interKeyMs: 15,
        },
      ],
    },
  ],
};

describe("MacroPreviewPage — V2-094 optional preview and send", () => {
  test("shows package, macro, source, timing, action count, duration, and USB state", async () => {
    const store = createRepositoryWorkingCopyStore(repository);
    const { container, unmount } = await render(
      <MacroPreviewPage
        macroId={macroId}
        onBack={vi.fn()}
        onSendInitiated={vi.fn()}
        packageId={packageId}
        store={store}
        usbState="ready"
      />,
    );
    expect(container.textContent).toContain("Build server login");
    expect(container.textContent).toContain("Start the build");
    expect(container.textContent).toContain("make -j8{ENTER}");
    expect(container.textContent).toContain("8 ms");
    expect(container.textContent).toContain("15 ms");
    expect(container.textContent).toContain("Action count");
    expect(container.textContent).toContain("Estimated duration");
    expect(container.textContent).toContain("USB ready");
    await unmount();
  });

  test("Send now issues one POST and hands the accepted status back for the Macros page", async () => {
    vi.useFakeTimers();
    try {
      const store = createRepositoryWorkingCopyStore(repository);
      const onSendInitiated = vi.fn();
      const onBack = vi.fn();
      const { unmount } = await render(
        <MacroPreviewPage
          macroId={macroId}
          onBack={onBack}
          onSendInitiated={onSendInitiated}
          packageId={packageId}
          store={store}
          usbState="ready"
        />,
      );

      planJsonResponse(
        {
          id: sendId,
          state: "running",
          actionCount: 2,
          estimatedDurationMs: 100,
        },
        202,
      );
      await click(buttonWithText("Send now"));
      await tick(0);

      expect(
        getFetchCalls().filter((call) => call.method === "POST"),
      ).toHaveLength(1);
      expect(onSendInitiated).toHaveBeenCalledWith(
        expect.objectContaining({ id: sendId, state: "running" }),
      );
      expect(onBack).toHaveBeenCalledOnce();
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("Send now is disabled unless USB is ready", async () => {
    const store = createRepositoryWorkingCopyStore(repository);
    const { unmount } = await render(
      <MacroPreviewPage
        macroId={macroId}
        onBack={vi.fn()}
        onSendInitiated={vi.fn()}
        packageId={packageId}
        store={store}
        usbState="disconnected"
      />,
    );
    expect(buttonWithText("Send now").disabled).toBe(true);
    await unmount();
  });

  test("Cancel returns without sending", async () => {
    const store = createRepositoryWorkingCopyStore(repository);
    const onBack = vi.fn();
    const { unmount } = await render(
      <MacroPreviewPage
        macroId={macroId}
        onBack={onBack}
        onSendInitiated={vi.fn()}
        packageId={packageId}
        store={store}
        usbState="ready"
      />,
    );
    await click(buttonWithText("Cancel"));
    expect(onBack).toHaveBeenCalledOnce();
    expect(getFetchCalls()).toHaveLength(0);
    await unmount();
  });

  test("409 hands off the actual current send instead of failing", async () => {
    vi.useFakeTimers();
    try {
      const store = createRepositoryWorkingCopyStore(repository);
      const onSendInitiated = vi.fn();
      const onBack = vi.fn();
      const { unmount } = await render(
        <MacroPreviewPage
          macroId={macroId}
          onBack={onBack}
          onSendInitiated={onSendInitiated}
          packageId={packageId}
          store={store}
          usbState="ready"
        />,
      );

      planJsonResponse(
        {
          error: {
            code: "already_sending",
            message: "A send is already in progress.",
          },
        },
        409,
      );
      planJsonResponse({
        id: sendId,
        state: "running",
        actionIndex: 1,
        actionCount: 2,
        estimatedDurationMs: 100,
        cancellationRequested: false,
        error: "",
        releaseError: "",
      });
      await click(buttonWithText("Send now"));
      await tick(0);

      expect(onSendInitiated).toHaveBeenCalledWith(
        expect.objectContaining({ id: sendId, actionIndex: 1 }),
      );
      expect(onBack).toHaveBeenCalledOnce();
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("shows an error and stays on the page when the macro is missing", async () => {
    const store = createRepositoryWorkingCopyStore(repository);
    const { container, unmount } = await render(
      <MacroPreviewPage
        macroId="00000000-0000-4000-8000-000000000000"
        onBack={vi.fn()}
        onSendInitiated={vi.fn()}
        packageId={packageId}
        store={store}
        usbState="ready"
      />,
    );
    expect(container.textContent).toContain(
      "This macro is no longer in the repository.",
    );
    await unmount();
  });
});
