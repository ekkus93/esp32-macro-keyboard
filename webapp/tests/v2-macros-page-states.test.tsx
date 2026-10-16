import { describe, expect, test, vi } from "vitest";
import type { ActiveSendSummary } from "../src/features/shell/v2/activeSendSummary";
import type { UsbState } from "../src/v2/apiTypes";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import { buttonWithText, click } from "./render";
import { renderMacrosPage, statusAt, tick } from "./macrosPageHarness";

/**
 * UXR-041: the Quick Send states a person can be in, and what the page says
 * and does in each. The states already covered by the V2-093/V2-095 suites
 * (starting, running, awaiting confirmation, completed, cancelled, failed,
 * timed out, release error, 409 recovery) are not repeated here; this file
 * covers what the redesign added: the reason Send is unavailable, the
 * cancelling state, and what is and is not a live announcement.
 */

const NOT_READY: [UsbState, string][] = [
  ["disconnected", "Not connected to a computer over USB"],
  ["enumerating", "still setting up the keyboard"],
  ["suspended", "asleep or has paused the USB link"],
  ["error", "The USB connection has a problem"],
  ["uninitialized", "USB isn't ready yet"],
];

describe("MacrosPage — Send unavailable", () => {
  test.each(NOT_READY)(
    "USB %s disables Send and says why next to it",
    async (usbState, expected) => {
      const { container, unmount } = await renderMacrosPage({ usbState });
      for (const send of container.querySelectorAll<HTMLButtonElement>(
        '[aria-label^="Send "]',
      )) {
        expect(send.disabled).toBe(true);
      }
      const notice = container.querySelector('p[role="status"] > span');
      expect(notice?.textContent).toContain(expected);
      await unmount();
    },
  );

  test("USB ready shows no notice and enables Send", async () => {
    const { container, unmount } = await renderMacrosPage({
      usbState: "ready",
    });
    expect(container.querySelector('p[role="status"] > span')).toBeNull();
    for (const send of container.querySelectorAll<HTMLButtonElement>(
      '[aria-label^="Send "]',
    )) {
      expect(send.disabled).toBe(false);
    }
    await unmount();
  });
});

describe("MacrosPage — cancelling", () => {
  test("Cancel becomes a disabled Cancelling… control until the send ends, and sends one request", async () => {
    vi.useFakeTimers();
    try {
      const summaries: (ActiveSendSummary | null)[] = [];
      const { container, unmount } = await renderMacrosPage({
        initialSend: statusAt("running", 1),
        onActiveSendChange: (summary) => {
          summaries.push(summary);
        },
      });
      expect(container.textContent).toContain("Sending…");

      planJsonResponse({}, 202);
      await click(buttonWithText("Cancel and release all keys"));
      await tick(0);

      const cancelling = buttonWithText("Cancelling…");
      expect(cancelling.disabled).toBe(true);
      expect(container.textContent).toContain("Cancelling…");
      await click(cancelling);
      expect(
        getFetchCalls().filter((call) => call.method === "DELETE"),
      ).toHaveLength(1);
      // The orientation surface and shell banner stop offering Cancel too.
      expect(summaries.at(-1)?.onCancel).toBeNull();
      expect(summaries.at(-1)?.statusText).toBe("Cancelling…");

      planJsonResponse(
        statusAt("cancelled", 1, { cancellationRequested: true }),
      );
      await tick(1000);
      expect(container.textContent).toContain("was cancelled.");
      expect(container.textContent).not.toContain("Cancelling…");
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("a failed cancel request re-enables Cancel and says so", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage({
        initialSend: statusAt("running", 1),
      });
      planJsonResponse(
        { error: { code: "usb_unavailable", message: "Could not cancel." } },
        500,
      );
      await click(buttonWithText("Cancel and release all keys"));
      await tick(0);
      expect(buttonWithText("Cancel and release all keys").disabled).toBe(
        false,
      );
      expect(container.textContent).toContain("Could not cancel.");
      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });
});

describe("MacrosPage — live announcements stay restrained", () => {
  test("the per-poll action counter sits outside the live region's announced text", async () => {
    vi.useFakeTimers();
    try {
      const { container, unmount } = await renderMacrosPage({
        initialSend: statusAt("running", 1),
      });
      const counter = container.querySelector('[aria-live="off"]');
      expect(counter?.textContent).toContain("action 1 of 2");
      const bar = container.querySelector('[role="progressbar"]');
      expect(bar?.getAttribute("aria-valuenow")).toBe("1");
      expect(bar?.getAttribute("aria-valuemax")).toBe("2");

      planJsonResponse(statusAt("running", 2));
      await tick(1000);
      expect(
        container
          .querySelector('[aria-live="off"]')
          ?.textContent?.includes("action 2 of 2"),
      ).toBe(true);
      expect(
        container
          .querySelector('[role="progressbar"]')
          ?.getAttribute("aria-valuenow"),
      ).toBe("2");

      planJsonResponse(statusAt("completed", 2));
      await tick(1000);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });
});
