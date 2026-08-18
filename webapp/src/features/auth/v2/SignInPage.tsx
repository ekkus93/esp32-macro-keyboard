import { useCallback, useEffect, useRef, useState } from "react";
import { StandaloneScreen } from "../../../components/StandaloneScreen";
import { V2ApiError, v2GetJson, v2PostJson } from "../../../v2/apiClient";
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

type Phase = "checking" | "form" | "check-error";

/** Minimal inline icons — no icon font/library, matching the rest of the
 * app: `verify-no-remote-assets.sh` fails the build on any webfont, and
 * nothing here depends on a package for two shapes. `aria-hidden` because
 * the enclosing button already carries the accessible name. */
function EyeIcon(): React.JSX.Element {
  return (
    <svg
      aria-hidden="true"
      fill="none"
      height="20"
      stroke="currentColor"
      strokeLinecap="round"
      strokeLinejoin="round"
      strokeWidth="2"
      viewBox="0 0 24 24"
      width="20"
    >
      <path d="M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7Z" />
      <circle cx="12" cy="12" r="3" />
    </svg>
  );
}

function EyeOffIcon(): React.JSX.Element {
  return (
    <svg
      aria-hidden="true"
      fill="none"
      height="20"
      stroke="currentColor"
      strokeLinecap="round"
      strokeLinejoin="round"
      strokeWidth="2"
      viewBox="0 0 24 24"
      width="20"
    >
      <path d="M17.94 17.94A10.94 10.94 0 0 1 12 19c-7 0-11-7-11-7a18.6 18.6 0 0 1 5.06-5.94M9.9 4.24A10.94 10.94 0 0 1 12 5c7 0 11 7 11 7a18.6 18.6 0 0 1-2.16 3.19" />
      <path d="M14.12 14.12a3 3 0 1 1-4.24-4.24" />
      <line x1="1" x2="23" y1="1" y2="23" />
    </svg>
  );
}

/**
 * V2-081 — Sign In (UI_UX_SPEC_V2 §3.2).
 *
 * A 401 from the initial session probe is the expected unauthenticated
 * state and opens the password form. Every other probe failure is a
 * connectivity/protocol/server failure, so it remains visible with Retry
 * rather than being silently reclassified as "no session".
 */
export function SignInPage({
  onAuthenticated,
}: SignInPageProps): React.JSX.Element {
  const [phase, setPhase] = useState<Phase>("checking");
  const [sessionCheckError, setSessionCheckError] = useState<string | null>(
    null,
  );
  const [adminPassword, setAdminPassword] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [passwordVisible, setPasswordVisible] = useState(false);

  const onAuthenticatedRef = useRef(onAuthenticated);
  onAuthenticatedRef.current = onAuthenticated;
  const mountedRef = useRef(false);

  const checkExistingSession = useCallback(async (): Promise<void> => {
    setPhase("checking");
    setSessionCheckError(null);
    try {
      const session = await v2GetJson("/api/v1/auth/session", isSessionStatus, {
        notifyOnUnauthorized: false,
      });
      if (mountedRef.current) {
        onAuthenticatedRef.current(session);
      }
    } catch (checkError: unknown) {
      if (!mountedRef.current) {
        return;
      }
      if (checkError instanceof V2ApiError && checkError.status === 401) {
        setPhase("form");
        return;
      }
      setSessionCheckError(v2ErrorText(checkError));
      setPhase("check-error");
    }
  }, []);

  useEffect(() => {
    mountedRef.current = true;
    void checkExistingSession();
    return () => {
      mountedRef.current = false;
    };
  }, [checkExistingSession]);

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
      <StandaloneScreen aria-busy="true">
        <p role="status">Checking for an existing session…</p>
      </StandaloneScreen>
    );
  }

  if (phase === "check-error") {
    return (
      <StandaloneScreen>
        <section>
          <h1>Unable to check session</h1>
          <ErrorBanner message={sessionCheckError} />
          <button
            className="primary"
            onClick={() => {
              void checkExistingSession();
            }}
            type="button"
          >
            Retry
          </button>
        </section>
      </StandaloneScreen>
    );
  }

  const requestValid = isLoginRequest({ adminPassword });

  return (
    <StandaloneScreen>
      <section>
        <h1>Sign in</h1>
        <form
          className="grid gap-[0.85rem]"
          onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
            void submit(event);
          }}
        >
          <label htmlFor="admin-password">Administrator password</label>
          <div className="relative">
            <input
              aria-invalid={error === null ? undefined : true}
              autoComplete="current-password"
              autoFocus
              className="pr-12"
              disabled={submitting}
              id="admin-password"
              onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                setAdminPassword(event.target.value);
              }}
              required
              type={passwordVisible ? "text" : "password"}
              value={adminPassword}
            />
            {/* A plain icon toggle, not a keycap: the bare `button` rule
                gives every button the pressed-key travel treatment, which
                reads as too heavy sitting inside a text field. h-11/w-11
                (44px) still meet the touch-target floor the rest of the app
                uses (see CheckboxRow.tsx). */}
            <button
              aria-label={passwordVisible ? "Hide password" : "Show password"}
              aria-pressed={passwordVisible}
              className="absolute right-[0.3rem] top-1/2 grid h-11 min-h-11 w-11 min-w-11 -translate-y-1/2 place-items-center rounded-keycap border-0 bg-transparent p-0 text-legend-soft hover:text-legend"
              onClick={() => {
                setPasswordVisible((visible) => !visible);
              }}
              type="button"
            >
              {passwordVisible ? <EyeOffIcon /> : <EyeIcon />}
            </button>
          </div>
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
    </StandaloneScreen>
  );
}
