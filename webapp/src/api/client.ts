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
    this.name = "ApiError";
  }
}

let csrfToken: string | null = null;

export function setCsrfToken(token: string | null): void {
  csrfToken = token;
}

function invalidResponse(status: number, message: string): ApiError {
  return new ApiError(status, {
    code: "invalid_response",
    message,
  });
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isApiErrorBody(value: unknown): value is ApiErrorBody {
  if (!isRecord(value)) {
    return false;
  }
  return typeof value.code === "string" && typeof value.message === "string";
}

function parseEnvelope<T>(status: number, value: unknown): ApiSuccess<T> | ApiFailure {
  if (!isRecord(value) || typeof value.ok !== "boolean") {
    throw invalidResponse(status, "The device returned an invalid API envelope.");
  }
  if (value.ok) {
    if (!Object.prototype.hasOwnProperty.call(value, "data")) {
      throw invalidResponse(status, "The device returned an invalid success envelope.");
    }
    return {
      ok: true,
      data: value.data as T,
    };
  }
  if (!isApiErrorBody(value.error)) {
    throw invalidResponse(status, "The device returned an invalid failure envelope.");
  }
  return {
    ok: false,
    error: value.error,
  };
}

const mutationMethods = new Set(["POST", "PUT", "PATCH", "DELETE"]);

export async function apiRequest<T>(path: string, init: RequestInit = {}): Promise<T> {
  if (!path.startsWith("/api/")) {
    throw new Error("API requests must use same-origin /api/ paths.");
  }
  if (init.signal !== undefined) {
    throw new Error("Caller-provided abort signals are not supported.");
  }

  const method = (init.method ?? "GET").toUpperCase();
  const controller = new AbortController();
  const timeout = window.setTimeout(() => controller.abort(), 10_000);
  const headers = new Headers(init.headers);
  headers.set("Accept", "application/json");
  if (init.body !== undefined && !headers.has("Content-Type")) {
    headers.set("Content-Type", "application/json");
  }
  if (csrfToken !== null && mutationMethods.has(method)) {
    headers.set("X-CSRF-Token", csrfToken);
  }

  try {
    const response = await fetch(path, {
      ...init,
      method,
      headers,
      credentials: "same-origin",
      signal: controller.signal,
    });
    const contentType = response.headers.get("content-type") ?? "";
    if (!contentType.toLowerCase().startsWith("application/json")) {
      throw invalidResponse(response.status, "The device returned a non-JSON response.");
    }

    let value: unknown;
    try {
      value = await response.json();
    } catch {
      throw invalidResponse(response.status, "The device returned malformed JSON.");
    }

    const envelope = parseEnvelope<T>(response.status, value);
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
