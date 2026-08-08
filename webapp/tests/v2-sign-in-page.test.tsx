import { describe, expect, test, vi } from "vitest";
import { SignInPage } from "../src/features/auth/v2/SignInPage";
import { v2Limits } from "../src/v2/limits";
import type { SessionStatus } from "../src/v2/apiTypes";
import { getFetchCalls, jsonResponse, planFetch } from "./fakeFetch";
import {
  buttonWithText,
  flushReact,
  render,
  requiredElement,
  setInputValue,
  submit,
} from "./render";

const validSession: SessionStatus = {
  authenticated: true,
  idleExpiresInSeconds: v2Limits.sessionIdleLifetimeSeconds,
  absoluteExpiresInSeconds: v2Limits.sessionAbsoluteLifetimeSeconds,
};

function planSessionCheck(status: number, body: unknown): void {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/auth/session");
    expect(call.method).toBe("GET");
    return jsonResponse(body, status);
  });
}

function planUnauthenticatedSessionCheck(): void {
  planSessionCheck(401, {
    error: { code: "unauthorized", message: "Sign in required." },
  });
}

async function renderSignInForm(
  onAuthenticated: (session: SessionStatus) => void = () => {
    /* no-op default */
  },
): Promise<Awaited<ReturnType<typeof render>>> {
  planUnauthenticatedSessionCheck();
  const view = await render(<SignInPage onAuthenticated={onAuthenticated} />);
  await flushReact();
  return view;
}

const longEnoughPassword = "correct horse battery staple";

describe("v2 SignInPage", () => {
  test("checks for an existing session before rendering anything else", async () => {
    const view = await renderSignInForm();
    expect(getFetchCalls()).toHaveLength(1);
    expect(getFetchCalls()[0]?.url).toBe("/api/v1/auth/session");
    await view.unmount();
  });

  test("shows the sign-in form when there is no valid session", async () => {
    const view = await renderSignInForm();
    expect(document.body.textContent).toContain("Administrator password");
    expect(requiredElement("#admin-password", HTMLInputElement)).toBeDefined();
    await view.unmount();
  });

  test("calls onAuthenticated immediately without showing the form when a session already exists", async () => {
    const onAuthenticated = vi.fn<(session: SessionStatus) => void>();
    planSessionCheck(200, validSession);
    const view = await render(<SignInPage onAuthenticated={onAuthenticated} />);
    await flushReact();

    expect(onAuthenticated).toHaveBeenCalledTimes(1);
    expect(onAuthenticated).toHaveBeenCalledWith(validSession);
    expect(document.body.textContent).not.toContain("Administrator password");
    await view.unmount();
  });

  test("falls back to the form when the session check fails unexpectedly", async () => {
    planSessionCheck(500, {
      error: { code: "internal_error", message: "Unexpected failure." },
    });
    const view = await render(
      <SignInPage
        onAuthenticated={() => {
          /* not exercised in this test */
        }}
      />,
    );
    await flushReact();

    expect(document.body.textContent).toContain("Administrator password");
    await view.unmount();
  });

  test("disables sign-in until the password meets the minimum byte length", async () => {
    const view = await renderSignInForm();
    const passwordField = requiredElement("#admin-password", HTMLInputElement);
    const button = buttonWithText("Sign in");
    expect(button.disabled).toBe(true);

    await setInputValue(passwordField, "short");
    expect(button.disabled).toBe(true);

    await setInputValue(passwordField, longEnoughPassword);
    expect(button.disabled).toBe(false);
    await view.unmount();
  });

  test("submits the password and calls onAuthenticated on success", async () => {
    const onAuthenticated = vi.fn<(session: SessionStatus) => void>();
    const view = await renderSignInForm(onAuthenticated);

    await setInputValue(
      requiredElement("#admin-password", HTMLInputElement),
      longEnoughPassword,
    );
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/auth/login");
      expect(call.method).toBe("POST");
      expect(call.body).toBe(
        JSON.stringify({ adminPassword: longEnoughPassword }),
      );
      return jsonResponse(validSession, 200);
    });
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();

    expect(onAuthenticated).toHaveBeenCalledWith(validSession);
    expect(requiredElement("#admin-password", HTMLInputElement).value).toBe("");
    await view.unmount();
  });

  test("shows the server's own message on invalid credentials without adding detail", async () => {
    const onAuthenticated = vi.fn<(session: SessionStatus) => void>();
    const view = await renderSignInForm(onAuthenticated);

    await setInputValue(
      requiredElement("#admin-password", HTMLInputElement),
      longEnoughPassword,
    );
    planFetch(() =>
      jsonResponse(
        {
          error: {
            code: "invalid_credentials",
            message: "Incorrect administrator password.",
          },
        },
        403,
      ),
    );
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();

    expect(requiredElement("[role='alert']", HTMLElement).textContent).toBe(
      "invalid_credentials: Incorrect administrator password.",
    );
    expect(onAuthenticated).not.toHaveBeenCalled();
    expect(buttonWithText("Sign in").disabled).toBe(false);
    await view.unmount();
  });

  test("shows rate-limit errors from 429 responses verbatim", async () => {
    const view = await renderSignInForm();

    await setInputValue(
      requiredElement("#admin-password", HTMLInputElement),
      longEnoughPassword,
    );
    planFetch(() =>
      jsonResponse(
        {
          error: {
            code: "rate_limited",
            message: "Too many attempts. Try again later.",
          },
        },
        429,
      ),
    );
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();

    expect(requiredElement("[role='alert']", HTMLElement).textContent).toBe(
      "rate_limited: Too many attempts. Try again later.",
    );
    await view.unmount();
  });
});
