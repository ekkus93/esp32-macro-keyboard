export interface ApiErrorBody {
  code: string;
  message: string;
  details?: unknown;
}

export interface ApiSuccess<T> {
  ok: true;
  data: T;
}

export interface ApiFailure {
  ok: false;
  error: ApiErrorBody;
}

export class ApiError extends Error {
  public constructor(
    public readonly status: number,
    public readonly body: ApiErrorBody,
  ) {
    super(body.message);
  }
}

let csrfToken: string | null = null;

export function setCsrfToken(token: string | null): void {
  csrfToken = token;
}

export async function apiRequest<T>(path: string, init: RequestInit = {}): Promise<T> {
  if (!path.startsWith("/api/")) {
    throw new Error("API requests must use same-origin /api/ paths.");
  }

  const controller = new AbortController();
  const timeout = window.setTimeout(() => controller.abort(), 10_000);
  const headers = new Headers(init.headers);
  headers.set("Accept", "application/json");
  if (init.body !== undefined) {
    headers.set("Content-Type", "application/json");
  }
  if (csrfToken !== null && init.method !== undefined && init.method !== "GET") {
    headers.set("X-CSRF-Token", csrfToken);
  }

  try {
    const response = await fetch(path, {
      ...init,
      headers,
      credentials: "same-origin",
      signal: controller.signal,
    });
    const contentType = response.headers.get("content-type") ?? "";
    if (!contentType.toLowerCase().startsWith("application/json")) {
      throw new ApiError(response.status, {
        code: "invalid_response",
        message: "The device returned a non-JSON response.",
      });
    }
    const envelope = (await response.json()) as ApiSuccess<T> | ApiFailure;
    if (!response.ok || !envelope.ok) {
      const body = envelope.ok
        ? {
            code: "http_error",
            message: `Request failed with status ${response.status}.`,
          }
        : envelope.error;
      throw new ApiError(response.status, body);
    }
    return envelope.data;
  } finally {
    window.clearTimeout(timeout);
  }
}
