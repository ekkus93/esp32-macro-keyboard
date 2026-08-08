import { useEffect, useRef, useState } from "react";
import { v2GetJson, v2PostJson } from "../../../v2/apiClient";
import { isSessionStatus } from "../../../v2/apiGuards";
import { isLoginRequest } from "../../../v2/apiRequestGuards";
import type { SessionStatus } from "../../../v2/apiTypes";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { v2ErrorText } from "./v2ErrorText";

export interface SignInPageProps {
  /**
   * Called once with the sanitized session object after a successful
   * `POST /api/v1/auth/login`, or after this component discovers on mount
   * that the tab already holds a valid session. The caller owns what
   * "repository startup" means (UI_UX_SPEC_V2 §3.4) — this component's job
   * ends at the moment authentication is established.
   */
  onAuthenticated: (session: SessionStatus) => void;
}

type Phase = "checking" | "form";

/**
 * V2-081 — Sign In (UI_UX_SPEC_V2 §3.2).
 *
 * This component assumes its caller has already decided a configured
 * device is the right context to mount it (V2-082's startup state machine
 * owns that overall decision — out of this task's scope). It still
 * satisfies "show Sign In only ... without a valid session" on its own
 * terms: on mount it checks `GET /api/v1/auth/session` and, if a session is
 * already valid, calls `onAuthenticated` immediately instead of ever
 * rendering the password form.
 */
export function SignInPage({
  onAuthenticated,
}: SignInPageProps): React.JSX.Element {
  const [phase, setPhase] = useState<Phase>("checking");
  const [adminPassword, setAdminPassword] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Read through a ref (stable identity, no dependency-array entry needed)
  // so the mount-only session check below can call the latest callback
  // without re-running whenever the caller passes a new function reference.
  const onAuthenticatedRef = useRef(onAuthenticated);
  onAuthenticatedRef.current = onAuthenticated;

  useEffect(() => {
    let active = true;
    const checkExistingSession = async (): Promise<void> => {
      try {
        const session = await v2GetJson(
          "/api/v1/auth/session",
          isSessionStatus,
          { notifyOnUnauthorized: false },
        );
        if (active) {
          onAuthenticatedRef.current(session);
        }
      } catch {
        // A missing/invalid session (401) is the expected case that shows
        // the form. Any other failure (device unreachable, malformed
        // response) also falls through to the form rather than trapping
        // the user on a silent checking screen — the login submission
        // itself will surface a real connectivity problem.
        if (active) {
          setPhase("form");
        }
      }
    };
    void checkExistingSession();
    return () => {
      active = false;
    };
  }, []);

  const submit = async (
    event: React.FormEvent<HTMLFormElement>,
  ): Promise<void> => {
    event.preventDefault();
    setSubmitting(true);
    setError(null);
    try {
      const session = await v2PostJson(
        "/api/v1/auth/login",
        { adminPassword },
        isSessionStatus,
        { notifyOnUnauthorized: false },
      );
      setAdminPassword("");
      onAuthenticated(session);
    } catch (loginError: unknown) {
      setError(v2ErrorText(loginError));
    } finally {
      setSubmitting(false);
    }
  };

  if (phase === "checking") {
    return (
      <main className="standalone" aria-busy="true">
        <p role="status">Checking for an existing session…</p>
      </main>
    );
  }

  const requestValid = isLoginRequest({ adminPassword });

  return (
    <main className="standalone">
      <section>
        <h1>Sign in</h1>
        <form
          className="form-stack"
          onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
            void submit(event);
          }}
        >
          <label htmlFor="admin-password">Administrator password</label>
          <input
            aria-invalid={error === null ? undefined : true}
            autoComplete="current-password"
            autoFocus
            disabled={submitting}
            id="admin-password"
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setAdminPassword(event.target.value);
            }}
            required
            type="password"
            value={adminPassword}
          />
          <ErrorBanner message={error} />
          <button
            className="primary"
            disabled={submitting || !requestValid}
            type="submit"
          >
            {submitting ? "Signing in…" : "Sign in"}
          </button>
        </form>
      </section>
    </main>
  );
}
