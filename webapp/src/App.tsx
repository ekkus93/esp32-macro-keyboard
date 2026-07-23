import { useEffect, useState } from "react";
import { ApiError, apiRequest, setCsrfToken } from "./api/client";
import { AppShell } from "./components/AppShell";
import type { ExecutionStatus } from "./types/models";

const screens = [
  "setup",
  "login",
  "sets",
  "procedures",
  "procedure",
  "instruction",
  "procedure-editor",
  "macros",
  "macro-editor",
  "confirm",
  "execution",
  "result",
  "manage-sets",
  "set-editor",
  "import",
  "export",
  "delete-set",
  "settings",
  "diagnostics",
] as const;

type Screen = (typeof screens)[number];

interface LoginResponse {
  csrfToken: string;
}

interface CardProps {
  title: string;
  body: string;
  action?: React.ReactNode;
}

function routeFromHash(): Screen {
  const route = window.location.hash.replace(/^#\/?/, "") as Screen;
  return screens.includes(route) ? route : "login";
}

function errorText(error: unknown): string {
  if (error instanceof ApiError) {
    return `${error.body.code}: ${error.body.message}`;
  }
  if (error instanceof Error) {
    return error.message;
  }
  return "An unknown error occurred.";
}

function Card({ title, body, action }: CardProps): React.JSX.Element {
  return (
    <article className="card">
      <div>
        <h2>{title}</h2>
        <p>{body}</p>
      </div>
      {action}
    </article>
  );
}

function LoginPage({ onSuccess }: { onSuccess: () => void }): React.JSX.Element {
  const [password, setPassword] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const submit = async (event: React.FormEvent<HTMLFormElement>): Promise<void> => {
    event.preventDefault();
    setSubmitting(true);
    setError(null);
    try {
      const response = await apiRequest<LoginResponse>("/api/v1/auth/login", {
        method: "POST",
        body: JSON.stringify({ password }),
      });
      setCsrfToken(response.csrfToken);
      setPassword("");
      onSuccess();
    } catch (loginError: unknown) {
      setError(errorText(loginError));
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <section>
      <h1>ESP32 Macro Keyboard</h1>
      <form
        className="form-stack"
        onSubmit={(event: React.FormEvent<HTMLFormElement>) => void submit(event)}
      >
        <label htmlFor="password">Administrator password</label>
        <input
          autoComplete="current-password"
          id="password"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) =>
            setPassword(event.target.value)
          }
          required
          type="password"
          value={password}
        />
        {error === null ? null : (
          <p className="error-message" role="alert">
            {error}
          </p>
        )}
        <button
          className="primary"
          disabled={submitting || password.length === 0}
          type="submit"
        >
          {submitting ? "Signing in…" : "Sign in"}
        </button>
      </form>
    </section>
  );
}

export default function App(): React.JSX.Element {
  const [screen, setScreen] = useState<Screen>(routeFromHash);
  const [execution, setExecution] = useState<ExecutionStatus | null>(null);
  const [macroSource, setMacroSource] = useState("shell{ENTER}");
  const [runtimeError, setRuntimeError] = useState<string | null>(null);

  useEffect(() => {
    const onHashChange = (): void => setScreen(routeFromHash());
    window.addEventListener("hashchange", onHashChange);
    return () => window.removeEventListener("hashchange", onHashChange);
  }, []);

  useEffect(() => {
    if (screen !== "execution") {
      return undefined;
    }

    let active = true;
    const refresh = async (): Promise<void> => {
      try {
        const current = await apiRequest<ExecutionStatus>("/api/v1/executions/current");
        if (!active) {
          return;
        }
        setExecution(current);
        setRuntimeError(null);
        if (["completed", "cancelled", "failed"].includes(current.state)) {
          window.location.hash = "/result";
        }
      } catch (pollError: unknown) {
        if (active) {
          setRuntimeError(errorText(pollError));
        }
      }
    };

    void refresh();
    const timer = window.setInterval(() => void refresh(), 500);
    return () => {
      active = false;
      window.clearInterval(timer);
    };
  }, [screen]);

  const navigate = (target: string): void => {
    setRuntimeError(null);
    window.location.hash = `/${target}`;
  };

  const cancelExecution = async (): Promise<void> => {
    setRuntimeError(null);
    try {
      await apiRequest<{ cancelRequested: boolean }>("/api/v1/executions/current/cancel", {
        method: "POST",
      });
    } catch (cancelError: unknown) {
      setRuntimeError(errorText(cancelError));
    }
  };

  const content = (() => {
    switch (screen) {
      case "setup":
        return (
          <Card
            body="Persistent encrypted provisioning is not implemented yet. Production firmware refuses to start a network until it is."
            title="First-run setup"
          />
        );
      case "login":
        return <LoginPage onSuccess={() => navigate("sets")} />;
      case "sets":
        return (
          <section>
            <h2>Choose a macro set</h2>
            <Card
              action={
                <button type="button" onClick={() => navigate("procedures")}>
                  Open
                </button>
              }
              body="ChromeOS, MrChromebox, and Debian workflows"
              title="HP Chromebook 11 G6 EE"
            />
            <button type="button" onClick={() => navigate("manage-sets")}>
              Manage sets
            </button>
          </section>
        );
      case "procedures":
        return (
          <section>
            <h2>Procedures</h2>
            <Card
              action={
                <button type="button" onClick={() => navigate("procedure")}>
                  Continue
                </button>
              }
              body="3 of 8 steps complete"
              title="ChromeOS to Debian 13"
            />
          </section>
        );
      case "procedure":
        return (
          <section>
            <h2>ChromeOS to Debian 13</h2>
            <Card
              action={
                <button type="button" onClick={() => navigate("confirm")}>
                  Send
                </button>
              }
              body="Macro step · explicit send required"
              title="Enter shell"
            />
            <button type="button" onClick={() => navigate("instruction")}>
              View manual step
            </button>
            <button type="button" onClick={() => navigate("procedure-editor")}>
              Edit procedure
            </button>
          </section>
        );
      case "instruction":
        return (
          <Card
            action={
              <button type="button" onClick={() => navigate("procedure")}>
                Mark complete
              </button>
            }
            body="Follow the model-specific service procedure, then confirm the expected hardware state."
            title="Disconnect the battery"
          />
        );
      case "procedure-editor":
        return (
          <section>
            <h2>Edit procedure</h2>
            <p>Drag steps or use Move Up, Move Down, Move First, and Move Last.</p>
            <button type="button">Add step</button>
          </section>
        );
      case "macros":
        return (
          <section>
            <h2>Macros</h2>
            <Card
              action={
                <button type="button" onClick={() => navigate("macro-editor")}>
                  Edit
                </button>
              }
              body="shell + Enter"
              title="Enter shell"
            />
          </section>
        );
      case "macro-editor":
        return (
          <section>
            <h2>Macro editor</h2>
            <label htmlFor="source">Macro source</label>
            <textarea
              id="source"
              onChange={(event: React.ChangeEvent<HTMLTextAreaElement>) =>
                setMacroSource(event.target.value)
              }
              value={macroSource}
            />
            <p>{new TextEncoder().encode(macroSource).length} bytes</p>
            <button type="button">Insert directive</button>
            <button type="button" disabled={macroSource.length === 0}>
              Save
            </button>
          </section>
        );
      case "confirm":
        return (
          <section>
            <h2>Confirm send</h2>
            <dl>
              <dt>Active set</dt>
              <dd>HP Chromebook 11 G6 EE</dd>
              <dt>Macro</dt>
              <dd>Enter shell</dd>
              <dt>USB</dt>
              <dd>Ready</dd>
            </dl>
            <button className="primary" type="button" onClick={() => navigate("execution")}>
              Send now
            </button>
          </section>
        );
      case "execution":
        return (
          <section>
            <h2>Typing macro</h2>
            <p>
              {execution === null
                ? "Waiting for device status…"
                : `${execution.actionIndex} / ${execution.actionCount}`}
            </p>
            <button
              className="danger"
              type="button"
              onClick={() => void cancelExecution()}
            >
              Cancel and release keys
            </button>
          </section>
        );
      case "result":
        return (
          <Card
            action={
              <button type="button" onClick={() => navigate("procedure")}>
                Return
              </button>
            }
            body="The next procedure step is ready but will not execute automatically."
            title={execution?.state === "failed" ? "Macro failed" : "Macro finished"}
          />
        );
      case "manage-sets":
        return (
          <section>
            <h2>Manage macro sets</h2>
            <button type="button" onClick={() => navigate("set-editor")}>
              Create set
            </button>
            <button type="button" onClick={() => navigate("import")}>
              Import
            </button>
            <button type="button" onClick={() => navigate("export")}>
              Export
            </button>
            <button type="button" onClick={() => navigate("delete-set")}>
              Delete
            </button>
          </section>
        );
      case "set-editor":
        return (
          <Card
            body="Manufacturer, model, board, purpose, and description are metadata; selection is always explicit."
            title="Create macro set"
          />
        );
      case "import":
        return (
          <Card
            body="The entire package is validated before any active data changes. Choose import as new or transactional replace."
            title="Import macro set"
          />
        );
      case "export":
        return (
          <Card
            body="Exports include referenced macros but exclude credentials, sessions, setup codes, and secrets."
            title="Export macro set"
          />
        );
      case "delete-set":
        return (
          <Card
            action={
              <button className="danger" type="button">
                Delete set
              </button>
            }
            body="Type the exact set name. Data moves to trash transactionally; shared macros remain."
            title="Delete macro set"
          />
        );
      case "settings":
        return (
          <section>
            <h2>Settings</h2>
            <Card body="Always ask which macro set to use" title="Startup set selection" />
            <Card body="Require the device button before typing" title="Physical confirmation" />
            <button type="button" onClick={() => navigate("diagnostics")}>
              Storage diagnostics
            </button>
          </section>
        );
      case "diagnostics":
        return (
          <section>
            <h2>Diagnostics</h2>
            <Card body="Mounted · no automatic formatting" title="Storage" />
            <Card body="Corrupt evidence is preserved and listed here" title="Quarantine" />
          </section>
        );
    }
  })();

  const errorBanner =
    runtimeError === null ? null : (
      <p className="error-message" role="alert">
        {runtimeError}
      </p>
    );

  if (screen === "setup" || screen === "login" || screen === "sets") {
    return (
      <main className="standalone">
        {errorBanner}
        {content}
      </main>
    );
  }
  return (
    <AppShell activeSet="HP Chromebook 11 G6 EE" navigate={navigate} route={screen}>
      {errorBanner}
      {content}
    </AppShell>
  );
}
