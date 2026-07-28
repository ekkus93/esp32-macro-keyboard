import { useState } from "react";
import { ApiError } from "../../api/client";
import { errorText } from "../../api/errors";
import { login } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";

interface LoginPageProps {
  onSuccess: () => void;
}

export function LoginPage({ onSuccess }: LoginPageProps): React.JSX.Element {
  const [password, setPassword] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [retryAfter, setRetryAfter] = useState<number | null>(null);

  const submit = async (
    event: React.FormEvent<HTMLFormElement>,
  ): Promise<void> => {
    event.preventDefault();
    setSubmitting(true);
    setError(null);
    setRetryAfter(null);
    try {
      await login(password);
      setPassword("");
      onSuccess();
    } catch (loginError: unknown) {
      setError(errorText(loginError));
      if (loginError instanceof ApiError) {
        setRetryAfter(loginError.retryAfterSeconds);
      }
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <section>
      <h1>ESP32 Macro Keyboard</h1>
      <form
        className="form-stack"
        onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
          void submit(event);
        }}
      >
        <label htmlFor="password">Administrator password</label>
        <input
          autoComplete="current-password"
          id="password"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setPassword(event.target.value);
          }}
          required
          type="password"
          value={password}
        />
        <ErrorBanner message={error} />
        {retryAfter === null ? null : (
          <p role="status">
            Try again in {String(retryAfter)} second
            {retryAfter === 1 ? "" : "s"}.
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
