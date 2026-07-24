import { act } from "react";
import { describe, expect, test } from "vitest";
import App from "../src/App";
import { apiRequest } from "../src/api/client";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import {
  flushReact,
  render,
  requiredElement,
  setInputValue,
  submit,
} from "./render";

async function renderLogin(): Promise<Awaited<ReturnType<typeof render>>> {
  setHashSilently("/login");
  const view = await render(<App />);
  await setInputValue(
    requiredElement("#password", HTMLInputElement),
    "correct horse",
  );
  return view;
}

describe("application authentication", () => {
  test("submits the password and stores the returned CSRF token", async () => {
    planJsonResponse({ ok: true, data: { csrfToken: "csrf-123" } });
    const view = await renderLogin();
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();

    const loginCall = getFetchCalls()[0];
    expect(loginCall?.url).toBe("/api/v1/auth/login");
    expect(loginCall?.method).toBe("POST");
    expect(loginCall?.body).toBe(JSON.stringify({ password: "correct horse" }));

    planJsonResponse({ ok: true, data: { saved: true } });
    await apiRequest("/api/v1/settings", { method: "POST", body: "{}" });
    expect(getFetchCalls()[1]?.headers.get("X-CSRF-Token")).toBe("csrf-123");
    await view.unmount();
  });

  test("navigates to sets after successful login", async () => {
    planJsonResponse({ ok: true, data: { csrfToken: "csrf-123" } });
    const view = await renderLogin();
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();
    await act(async () => {
      window.dispatchEvent(new HashChangeEvent("hashchange"));
      await Promise.resolve();
    });
    expect(document.body.textContent).toContain("Choose a macro set");
    await view.unmount();
  });

  test("shows structured API failures", async () => {
    planJsonResponse(
      {
        ok: false,
        error: { code: "invalid_password", message: "Password is incorrect." },
      },
      401,
    );
    const view = await renderLogin();
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();
    expect(requiredElement("[role='alert']", HTMLElement).textContent).toContain(
      "invalid_password: Password is incorrect.",
    );
    await view.unmount();
  });

  test("shows network failures", async () => {
    planFetch(() => Promise.reject(new Error("network offline")));
    const view = await renderLogin();
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();
    expect(requiredElement("[role='alert']", HTMLElement).textContent).toContain(
      "network offline",
    );
    await view.unmount();
  });

  test("disables repeated submission while a request is in flight", async () => {
    const deferred: { resolve: ((response: Response) => void) | null } = {
      resolve: null,
    };
    planFetch(
      () =>
        new Promise<Response>((resolve) => {
          deferred.resolve = resolve;
        }),
    );
    const view = await renderLogin();
    await submit(requiredElement("form", HTMLFormElement));

    expect(getFetchCalls()).toHaveLength(1);
    expect(
      requiredElement("button[type='submit']", HTMLButtonElement).disabled,
    ).toBe(true);

    const resolver = deferred.resolve;
    if (resolver === null) {
      throw new Error("Login request was not started.");
    }
    resolver(jsonResponse({ ok: true, data: { csrfToken: "csrf-123" } }));
    await flushReact();
    await view.unmount();
  });
});
