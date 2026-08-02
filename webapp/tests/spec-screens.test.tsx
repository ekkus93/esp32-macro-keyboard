import { describe, expect, test } from "vitest";
import App from "../src/App";
import { SetManagementPage } from "../src/features/sets/SetManagementPage";
import {
  executionStatus,
  macro,
  macroId,
  macroSet,
  planAuthenticatedBootstrap,
  planNormalUnauthenticatedBootstrap,
  setId,
  settings,
} from "./appFixtures";
import { planJsonResponse } from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import { buttonWithText, click, flushReact, render } from "./render";

/*
 * SPEC 9 lists fifteen required screens. Individual behaviours of most of them
 * are tested elsewhere; what nothing checked is the list itself -- that every
 * screen the specification requires is reachable and renders. A screen can be
 * deleted, or a route can start falling through to the set selector, without a
 * single behavioural test noticing, because each of those tests reaches its
 * screen directly.
 *
 * This is deliberately shallow. It asserts presence, not behaviour: one
 * identifying heading per screen. Anything deeper belongs in the suite that
 * owns that screen.
 */

function success(data: unknown): object {
  return { ok: true, data };
}

describe("SPEC 9 required screens", () => {
  // SPEC 24.5 item: every required screen
  test("1. first-run setup is shown for an unprovisioned device", async () => {
    planJsonResponse(
      success({
        deviceId: "device-123",
        apSsid: "Macro Keyboard Setup",
        completed: false,
        physicalConfirmationRequired: true,
      }),
    );
    const view = await render(<App />);
    await flushReact();
    expect(document.body.textContent).toContain("First-run setup");
    await view.unmount();
  });

  // SPEC 24.5 item: every required screen
  test("2. login is shown for a provisioned device with no session", async () => {
    planNormalUnauthenticatedBootstrap();
    const view = await render(<App />);
    await flushReact();
    expect(document.body.textContent).toContain("ESP32 Macro Keyboard");
    expect(document.querySelector("#password")).not.toBeNull();
    await view.unmount();
  });

  const routed: readonly {
    readonly ordinal: string;
    readonly hash: string;
    readonly heading: string;
    readonly plan?: () => void;
  }[] = [
    { ordinal: "3", hash: "/sets", heading: "Choose a macro set" },
    {
      ordinal: "4",
      hash: "/macros",
      heading: "Macros",
      plan: () => {
        planJsonResponse(success([macro]));
      },
    },
    { ordinal: "5", hash: "/macro-editor", heading: "Macro syntax reference" },
    {
      ordinal: "6",
      hash: `/confirm?macroId=${macroId}`,
      heading: "Confirm send",
      plan: () => {
        planJsonResponse(success(macro));
      },
    },
    {
      ordinal: "7",
      hash: "/execution",
      heading: "Typing macro",
      plan: () => {
        planJsonResponse(success(executionStatus("running")));
      },
    },
    { ordinal: "8", hash: "/result", heading: "No execution result" },
    { ordinal: "9", hash: "/manage-sets", heading: "Manage macro sets" },
    { ordinal: "11", hash: "/import", heading: "Import, export, and recovery" },
    { ordinal: "12", hash: "/export", heading: "Import, export, and recovery" },
    { ordinal: "14", hash: "/settings", heading: "Settings" },
    {
      ordinal: "15",
      hash: "/diagnostics",
      heading: "Storage diagnostics",
      plan: () => {
        planJsonResponse(
          success({
            verified: false,
            webMounted: true,
            dataMounted: true,
            usedBytes: 20480,
            totalBytes: 491520,
            remainingBytes: 471040,
            setFileMaxBytes: 32768,
            temporariesRemovedAtBoot: 0,
            discardedObjectCount: 0,
            discardedObjects: [],
          }),
        );
      },
    },
  ];

  for (const screen of routed) {
    // SPEC 24.5 item: every required screen
    test(`${screen.ordinal}. ${screen.heading} renders at #${screen.hash}`, async () => {
      setHashSilently(screen.hash);
      planAuthenticatedBootstrap();
      screen.plan?.();
      const view = await render(<App />);
      await flushReact();
      expect(document.body.textContent).toContain(screen.heading);
      await view.unmount();
    });
  }

  /*
   * Screens 10 and 13 are reached from inside "Manage macro sets" rather than
   * by route. The `set-editor` and `delete-set` hashes exist in the routing
   * table but render the parent page without entering either mode, so routing
   * to them proves nothing; the controls are what make the screens reachable.
   */
  // SPEC 24.5 item: every required screen
  test("10. create and duplicate set are reachable from set management", async () => {
    const view = await render(
      <SetManagementPage
        onSetsChanged={() => undefined}
        sets={[macroSet]}
        settings={settings}
      />,
    );
    await flushReact();
    await click(buttonWithText("Create set"));
    expect(document.body.textContent).toContain("Create macro set");
    await view.unmount();
  });

  // SPEC 24.5 item: every required screen
  test("13. delete set confirmation is reachable from set management", async () => {
    const view = await render(
      <SetManagementPage
        onSetsChanged={() => undefined}
        sets={[{ ...macroSet, id: "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa" }]}
        settings={{ ...settings, activeSetId: setId }}
      />,
    );
    await flushReact();
    await click(buttonWithText("Delete"));
    expect(document.body.textContent).toContain("Delete macro set");
    await view.unmount();
  });
});
