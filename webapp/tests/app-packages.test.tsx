import { describe, expect, test } from "vitest";
import App from "../src/App";
import {
  macroPackage,
  planAuthenticatedBootstrap,
  packageId,
  settings,
} from "./appFixtures";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setCheckboxChecked,
  setInputValue,
} from "./render";

const secondPackage = {
  ...macroPackage,
  id: "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
  name: "Workshop desktop workflow",
};

describe("server-backed package selection", () => {
  test("shows live metadata and filters by search", async () => {
    setHashSilently("/packages");
    planAuthenticatedBootstrap({
      activePackageId: null,
      packages: [macroPackage, secondPackage],
    });
    const view = await render(<App />);
    await flushReact();

    expect(document.body.textContent).toContain("Lab bench workflow");
    expect(document.body.textContent).toContain("Workshop desktop workflow");

    await setInputValue(
      requiredElement("#package-search", HTMLInputElement),
      "Workshop",
    );
    expect(document.body.textContent).not.toContain("Lab bench workflow");
    expect(document.body.textContent).toContain("Workshop desktop workflow");
    await view.unmount();
  });

  // SPEC 24.5 item: package switching

  // SPEC 24.5 item: active-package visibility

  test("selects a package with the settings revision and updates the header", async () => {
    setHashSilently("/packages");
    planAuthenticatedBootstrap({ activePackageId: null });
    const view = await render(<App />);
    await flushReact();

    planJsonResponse({
      ok: true,
      data: {
        ...settings,
        revision: settings.revision + 1,
        activePackageId: packageId,
      },
    });
    await click(buttonWithText("Use this package"));
    await flushReact();

    const call = getFetchCalls()[5];
    expect(call?.url).toBe(`/api/v1/package/${packageId}/select`);
    expect(call?.method).toBe("POST");
    expect(call?.body).toBe(
      JSON.stringify({ expectedRevision: settings.revision }),
    );
    expect(document.querySelector(".app-header")?.textContent).toContain(
      "Lab bench workflow",
    );
    expect(
      window.localStorage.getItem("esp32-macro-keyboard.recent-package-ids"),
    ).toContain(packageId);
    await view.unmount();
  });

  // SPEC 24.5 item: stale-edit conflict UI

  test("shows revision conflicts instead of silently accepting selection", async () => {
    setHashSilently("/packages");
    planAuthenticatedBootstrap({ activePackageId: null });
    const view = await render(<App />);
    await flushReact();

    planJsonResponse(
      {
        ok: false,
        error: {
          code: "conflict",
          message: "Settings revision is stale.",
        },
      },
      409,
    );
    await click(buttonWithText("Use this package"));
    await flushReact();

    expect(
      requiredElement("[role='alert']", HTMLElement).textContent,
    ).toContain("conflict: Settings revision is stale.");
    expect(document.querySelector(".app-header")?.textContent).toContain(
      "No active macro package",
    );
    await view.unmount();
  });

  test("persists real settings updates", async () => {
    setHashSilently("/settings");
    planAuthenticatedBootstrap();
    const view = await render(<App />);
    await flushReact();

    const checkbox = requiredElement(
      "input[type='checkbox']",
      HTMLInputElement,
    );
    await setCheckboxChecked(checkbox, false);
    planJsonResponse({
      ok: true,
      data: {
        ...settings,
        revision: settings.revision + 1,
        alwaysSelectPackage: false,
      },
    });
    await click(buttonWithText("Save settings"));
    await flushReact();

    const call = getFetchCalls()[5];
    expect(call?.url).toBe("/api/v1/settings");
    expect(call?.method).toBe("PUT");
    /* No activePackageId: the active package is repository state (SPEC 12.3) and moves
       only through the select route, so a settings PUT must not carry it. */
    expect(call?.body).toBe(
      JSON.stringify({
        expectedRevision: settings.revision,
        requirePhysicalConfirmation: settings.requirePhysicalConfirmation,
        alwaysSelectPackage: false,
      }),
    );
    await view.unmount();
  });
});
