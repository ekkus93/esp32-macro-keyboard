import { useEffect, useState } from "react";
import type { ReactNode } from "react";
import {
  ApiError,
  setCsrfToken,
  subscribeUnauthorized,
} from "../../api/client";
import { errorText } from "../../api/errors";
import {
  getDeviceStatus,
  getSession,
  getSetupState,
  logout,
} from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { DeviceStatus, SetupState } from "../../types/models";
import { LoginPage } from "./LoginPage";
import { SetupPage } from "./SetupPage";

interface AuthenticatedSession {
  initialStatus: DeviceStatus;
  logout: () => Promise<void>;
}

interface SessionBoundaryProps {
  children: (session: AuthenticatedSession) => ReactNode;
}

type BoundaryState =
  | { kind: "loading" }
  | { kind: "setup"; setup: SetupState }
  | { kind: "login" }
  | { kind: "authenticated"; status: DeviceStatus }
  | { kind: "error"; message: string };

function isNormalModeProbeError(error: unknown): boolean {
  return (
    error instanceof ApiError && (error.status === 401 || error.status === 404)
  );
}

export function SessionBoundary({
  children,
}: SessionBoundaryProps): React.JSX.Element {
  const [state, setState] = useState<BoundaryState>({ kind: "loading" });

  useEffect(() => {
    let active = true;
    const bootstrap = async (): Promise<void> => {
      try {
        try {
          const setup = await getSetupState();
          if (active) {
            setState({ kind: "setup", setup });
          }
          return;
        } catch (setupError: unknown) {
          if (!isNormalModeProbeError(setupError)) {
            throw setupError;
          }
        }

        const status = await getDeviceStatus();
        try {
          await getSession();
        } catch (sessionError: unknown) {
          if (sessionError instanceof ApiError && sessionError.status === 401) {
            if (active) {
              setState({ kind: "login" });
            }
            return;
          }
          throw sessionError;
        }
        if (active) {
          setState({ kind: "authenticated", status });
        }
      } catch (bootstrapError: unknown) {
        if (active) {
          setState({ kind: "error", message: errorText(bootstrapError) });
        }
      }
    };
    void bootstrap();
    return () => {
      active = false;
    };
  }, []);

  useEffect(
    () =>
      subscribeUnauthorized(() => {
        setCsrfToken(null);
        setState({ kind: "login" });
      }),
    [],
  );

  const signOut = async (): Promise<void> => {
    await logout();
    setState({ kind: "login" });
  };

  switch (state.kind) {
    case "loading":
      return (
        <main className="standalone" aria-busy="true">
          <p role="status">Checking device mode and session…</p>
        </main>
      );
    case "setup":
      return (
        <main className="standalone">
          <SetupPage initialState={state.setup} />
        </main>
      );
    case "login":
      return (
        <main className="standalone">
          <LoginPage
            onSuccess={() => {
              void (async () => {
                try {
                  const status = await getDeviceStatus();
                  setState({ kind: "authenticated", status });
                  window.location.hash = "/sets";
                } catch (statusError: unknown) {
                  setState({
                    kind: "error",
                    message: errorText(statusError),
                  });
                }
              })();
            }}
          />
        </main>
      );
    case "authenticated":
      return <>{children({ initialStatus: state.status, logout: signOut })}</>;
    case "error":
      return (
        <main className="standalone">
          <h1>Device unavailable</h1>
          <ErrorBanner message={state.message} />
          <button
            type="button"
            onClick={() => {
              window.location.reload();
            }}
          >
            Retry
          </button>
        </main>
      );
  }
}
