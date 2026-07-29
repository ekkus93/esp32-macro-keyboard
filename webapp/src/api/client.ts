import type { Validator } from "./guards";
import { isRecord } from "./guards";

export interface ApiErrorBody {
  code: string;
  message: string;
  details?: unknown;
}

interface ApiSuccess {
  ok: true;
  data: unknown;
}

interface ApiFailure {
  ok: false;
  error: ApiErrorBody;
}

export interface ApiRequestOptions {
  notifyOnUnauthorized?: boolean;
  timeoutMs?: number;
}

export class ApiError extends Error {
  public constructor(
    public readonly status: number,
    public readonly body: ApiErrorBody,
    public readonly retryAfterSeconds: number | null = null,
  ) {
    super(body.message);
    this.name = "ApiError";
  }
}

let csrfToken: string | null = null;
const unauthorizedListeners = new Set<() => void>();
const defaultTimeoutMs = 10_000;
const maximumTimeoutMs = 60_000;

export function setCsrfToken(token: string | null): void {
  csrfToken = token;
}

export function subscribeUnauthorized(listener: () => void): () => void {
  unauthorizedListeners.add(listener);
  return () => {
    unauthorizedListeners.delete(listener);
  };
}

function notifyUnauthorized(): void {
  for (const listener of unauthorizedListeners) {
    listener();
  }
}

function invalidResponse(status: number, message: string): ApiError {
  return new ApiError(status, {
    code: "invalid_response",
    message,
  });
}

function isApiErrorBody(value: unknown): value is ApiErrorBody {
  if (!isRecord(value)) {
    return false;
  }
  if (typeof value.code !== "string" || typeof value.message !== "string") {
    return false;
  }
  const keys = Object.keys(value);
  return (
    keys.every((key) => ["code", "message", "details"].includes(key)) &&
    keys.includes("code") &&
    keys.includes("message")
  );
}

function parseEnvelope(
  status: number,
  value: unknown,
): ApiSuccess | ApiFailure {
  if (!isRecord(value) || typeof value.ok !== "boolean") {
    throw invalidResponse(
      status,
      "The device returned an invalid API envelope.",
    );
  }
  if (value.ok) {
    if (
      Object.keys(value).length !== 2 ||
      !Object.prototype.hasOwnProperty.call(value, "data")
    ) {
      throw invalidResponse(
        status,
        "The device returned an invalid success envelope.",
      );
    }
    return {
      ok: true,
      data: value.data,
    };
  }
  if (Object.keys(value).length !== 2 || !isApiErrorBody(value.error)) {
    throw invalidResponse(
      status,
      "The device returned an invalid failure envelope.",
    );
  }
  return {
    ok: false,
    error: value.error,
  };
}

function retryAfterSeconds(response: Response): number | null {
  const raw = response.headers.get("Retry-After");
  if (raw === null || !/^[0-9]+$/.test(raw)) {
    return null;
  }
  const parsed = Number(raw);
  return Number.isSafeInteger(parsed) && parsed >= 0 ? parsed : null;
}

function requestTimeout(options: ApiRequestOptions): number {
  const timeoutMs = options.timeoutMs ?? defaultTimeoutMs;
  if (
    !Number.isSafeInteger(timeoutMs) ||
    timeoutMs <= 0 ||
    timeoutMs > maximumTimeoutMs
  ) {
    throw new Error(
      `API request timeout must be an integer from 1 through ${String(
        maximumTimeoutMs,
      )} milliseconds.`,
    );
  }
  return timeoutMs;
}

const mutationMethods = new Set(["POST", "PUT", "PATCH", "DELETE"]);

export async function apiRequest<T>(
  path: string,
  init: RequestInit = {},
  validate: Validator<T>,
  options: ApiRequestOptions = {},
): Promise<T> {
  if (!path.startsWith("/api/")) {
    throw new Error("API requests must use same-origin /api/ paths.");
  }
  if (init.signal !== undefined) {
    throw new Error("Caller-provided abort signals are not supported.");
  }

  const method = (init.method ?? "GET").toUpperCase();
  const controller = new AbortController();
  const timeout = window.setTimeout(() => {
    controller.abort();
  }, requestTimeout(options));
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
      throw invalidResponse(
        response.status,
        "The device returned a non-JSON response.",
      );
    }

    let value: unknown;
    try {
      value = await response.json();
    } catch {
      throw invalidResponse(
        response.status,
        "The device returned malformed JSON.",
      );
    }

    const envelope = parseEnvelope(response.status, value);
    if (!response.ok || !envelope.ok) {
      if (response.status === 401) {
        setCsrfToken(null);
        if (options.notifyOnUnauthorized !== false) {
          notifyUnauthorized();
        }
      }
      const body = envelope.ok
        ? {
            code: "http_error",
            message: `Request failed with status ${String(response.status)}.`,
          }
        : envelope.error;
      throw new ApiError(response.status, body, retryAfterSeconds(response));
    }
    if (!validate(envelope.data)) {
      throw invalidResponse(
        response.status,
        "The device returned an invalid response payload.",
      );
    }
    return envelope.data;
  } finally {
    window.clearTimeout(timeout);
  }
}
