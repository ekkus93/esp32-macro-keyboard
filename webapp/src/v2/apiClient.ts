import { isErrorEnvelope } from "./apiContracts";
import type { ErrorDetail } from "./apiTypes";

const jsonContentType = "application/json";
const defaultTimeoutMs = 10_000;
const maximumTimeoutMs = 60_000;

export interface V2RequestOptions {
  timeoutMs?: number;
  notifyOnUnauthorized?: boolean;
}

/**
 * A v2 API error. Unlike the v1 API, v2 success responses are the payload
 * directly (no `{ok, data}` envelope) and v2 errors are always
 * `{error: {code, message, field?, byteOffset?, line?, column?}}`. This class
 * carries the parsed error detail alongside the HTTP status.
 */
export class V2ApiError extends Error {
  public readonly status: number;
  public readonly code: string;
  public readonly field: string | undefined;
  public readonly byteOffset: number | undefined;
  public readonly line: number | undefined;
  public readonly column: number | undefined;

  public constructor(status: number, detail: ErrorDetail) {
    super(detail.message);
    this.name = "V2ApiError";
    this.status = status;
    this.code = detail.code;
    this.field = detail.field;
    this.byteOffset = detail.byteOffset;
    this.line = detail.line;
    this.column = detail.column;
  }
}

const unauthorizedListeners = new Set<() => void>();

export function subscribeUnauthorized(listener: () => void): () => void {
  unauthorizedListeners.add(listener);
  return () => {
    unauthorizedListeners.delete(listener);
  };
}

function notifyUnauthorized(): void {
  unauthorizedListeners.forEach((listener) => {
    listener();
  });
}

/**
 * Manually triggers the same "session is gone" transition an unexpected
 * `401` response would (every `subscribeUnauthorized` listener runs), for
 * call sites that already know the session ended without needing to wait
 * for a failing request to prove it — Sign Out (`POST
 * /api/v1/auth/logout` succeeding) and an administrator password change
 * (`POST /api/v1/settings/change-password` succeeding, which SPEC_V2 §13.9
 * says "invalidates all sessions including the current one"). Reusing this
 * path, rather than a bespoke one, keeps the same guarantee the session-
 * expiry case already has: the in-memory repository working copy is not
 * discarded (SPEC_V2 §7.3), only the authenticated screen is replaced with
 * Sign In.
 */
export function forceSessionEnded(): void {
  notifyUnauthorized();
}

function invalidResponse(status: number, message: string): V2ApiError {
  return new V2ApiError(status, { code: "invalid_response", message });
}

function requestTimeout(options: V2RequestOptions): number {
  const timeoutMs = options.timeoutMs ?? defaultTimeoutMs;
  if (
    !Number.isSafeInteger(timeoutMs) ||
    timeoutMs <= 0 ||
    timeoutMs > maximumTimeoutMs
  ) {
    throw new Error(
      `API request timeout must be an integer from 1 through ${String(maximumTimeoutMs)} milliseconds.`,
    );
  }
  return timeoutMs;
}

function validateApiPath(path: string): void {
  if (!path.startsWith("/api/")) {
    throw new Error("v2 API requests must use same-origin /api/ paths.");
  }
}

async function performRequest(
  path: string,
  init: RequestInit,
  options: V2RequestOptions,
): Promise<Response> {
  validateApiPath(path);
  const controller = new AbortController();
  const timeout = window.setTimeout(() => {
    controller.abort();
  }, requestTimeout(options));
  try {
    return await fetch(path, {
      ...init,
      credentials: "same-origin",
      signal: controller.signal,
    });
  } finally {
    window.clearTimeout(timeout);
  }
}

async function handleErrorResponse(
  response: Response,
  options: V2RequestOptions,
): Promise<never> {
  if (response.status === 401 && options.notifyOnUnauthorized !== false) {
    notifyUnauthorized();
  }
  let parsed: unknown;
  try {
    parsed = await response.json();
  } catch {
    throw invalidResponse(
      response.status,
      "The device returned a malformed error response.",
    );
  }
  if (!isErrorEnvelope(parsed)) {
    throw invalidResponse(
      response.status,
      "The device returned an invalid error envelope.",
    );
  }
  throw new V2ApiError(response.status, parsed.error);
}

interface JsonResult {
  status: number;
  value: unknown;
}

async function requestJson(
  path: string,
  init: RequestInit,
  options: V2RequestOptions,
): Promise<JsonResult> {
  const headers = new Headers(init.headers);
  headers.set("Accept", jsonContentType);
  if (init.body !== undefined && !headers.has("Content-Type")) {
    headers.set("Content-Type", jsonContentType);
  }
  const response = await performRequest(path, { ...init, headers }, options);
  if (!response.ok) {
    await handleErrorResponse(response, options);
  }
  if (response.status === 204) {
    return { status: response.status, value: undefined };
  }
  const contentType = response.headers.get("content-type") ?? "";
  if (!contentType.toLowerCase().startsWith(jsonContentType)) {
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
  return { status: response.status, value };
}

export async function v2GetJson<T>(
  path: string,
  guard: (value: unknown) => value is T,
  options: V2RequestOptions = {},
): Promise<T> {
  const result = await requestJson(path, { method: "GET" }, options);
  if (!guard(result.value)) {
    throw invalidResponse(
      result.status,
      "The device returned an invalid response payload.",
    );
  }
  return result.value;
}

export async function v2PostJson<T>(
  path: string,
  body: unknown,
  guard: (value: unknown) => value is T,
  options: V2RequestOptions = {},
): Promise<T> {
  // `body === undefined` means a bodyless POST (e.g. restart, per its route
  // contract's `request.body: "none"`) — omitting the `body` key entirely
  // (not setting it to `JSON.stringify(undefined)`, the string
  // `"undefined"`, which is not valid JSON, and not an explicit
  // `body: undefined` — disallowed under `exactOptionalPropertyTypes`) keeps
  // `requestJson` from adding a `Content-Type` header the route contract
  // says this request never has.
  const init: RequestInit =
    body === undefined
      ? { method: "POST" }
      : { method: "POST", body: JSON.stringify(body) };
  const result = await requestJson(path, init, options);
  if (!guard(result.value)) {
    throw invalidResponse(
      result.status,
      "The device returned an invalid response payload.",
    );
  }
  return result.value;
}

export async function v2PutJson<T>(
  path: string,
  body: unknown,
  guard: (value: unknown) => value is T,
  options: V2RequestOptions = {},
): Promise<T> {
  const result = await requestJson(
    path,
    { method: "PUT", body: JSON.stringify(body) },
    options,
  );
  if (!guard(result.value)) {
    throw invalidResponse(
      result.status,
      "The device returned an invalid response payload.",
    );
  }
  return result.value;
}

export async function v2DeleteJson<T>(
  path: string,
  guard: (value: unknown) => value is T,
  options: V2RequestOptions = {},
): Promise<T> {
  const result = await requestJson(path, { method: "DELETE" }, options);
  if (!guard(result.value)) {
    throw invalidResponse(
      result.status,
      "The device returned an invalid response payload.",
    );
  }
  return result.value;
}

/**
 * DELETE with a JSON response body whose exact shape is not yet pinned down
 * by the frozen v2 contract layer (`apiTypes.ts`/`apiGuards.ts` define no
 * dedicated type for it — SPEC_V2 only specifies the status code). Returns
 * the parsed value unvalidated; callers must not assume a shape until the
 * contract layer defines and exports a guard for it.
 */
export async function v2DeleteWithUnvalidatedJson(
  path: string,
  options: V2RequestOptions = {},
): Promise<unknown> {
  const result = await requestJson(path, { method: "DELETE" }, options);
  return result.value;
}

export async function v2DeleteNoContent(
  path: string,
  options: V2RequestOptions = {},
): Promise<void> {
  const response = await performRequest(
    path,
    { method: "DELETE", headers: { Accept: jsonContentType } },
    options,
  );
  if (!response.ok) {
    await handleErrorResponse(response, options);
  }
  if (response.status !== 204) {
    throw invalidResponse(response.status, "Expected 204 No Content.");
  }
}

/**
 * POST with no response body, for routes whose success is `204 No Content`
 * (change-password, sign-out) — `v2PostJson`'s guard parameter has nothing to
 * validate against in that case, so this is a dedicated helper rather than a
 * caller passing a vacuous guard.
 */
export async function v2PostNoContent(
  path: string,
  body: unknown,
  options: V2RequestOptions = {},
): Promise<void> {
  // See `v2PostJson` — `body === undefined` (e.g. sign-out's bodyless
  // request) must omit both the `body` key and the `Content-Type` header,
  // not send the literal string `"undefined"`.
  const init: RequestInit =
    body === undefined
      ? { method: "POST", headers: { Accept: jsonContentType } }
      : {
          method: "POST",
          headers: {
            Accept: jsonContentType,
            "Content-Type": jsonContentType,
          },
          body: JSON.stringify(body),
        };
  const response = await performRequest(path, init, options);
  if (!response.ok) {
    await handleErrorResponse(response, options);
  }
  if (response.status !== 204) {
    throw invalidResponse(response.status, "Expected 204 No Content.");
  }
}

export async function v2PostBinary<T>(
  path: string,
  bytes: Uint8Array,
  contentType: "application/gzip",
  guard: (value: unknown) => value is T,
  options: V2RequestOptions = {},
): Promise<T> {
  const headers = new Headers({
    Accept: jsonContentType,
    "Content-Type": contentType,
  });
  const body = new Uint8Array(bytes);
  const response = await performRequest(
    path,
    { method: "POST", headers, body },
    options,
  );
  if (!response.ok) {
    await handleErrorResponse(response, options);
  }
  // SPEC_V2 §10.3 / TODO_V2 V2-110: "Mark saved only after 201 Created" — the
  // only caller of this helper is the blob-create route, so this enforces
  // the exact success status rather than accepting any 2xx.
  if (response.status !== 201) {
    throw invalidResponse(response.status, "Expected 201 Created.");
  }
  const responseContentType = response.headers.get("content-type") ?? "";
  if (!responseContentType.toLowerCase().startsWith(jsonContentType)) {
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
  if (!guard(value)) {
    throw invalidResponse(
      response.status,
      "The device returned an invalid response payload.",
    );
  }
  return value;
}

export async function v2GetBinary(
  path: string,
  expectedContentType: "application/gzip",
  options: V2RequestOptions = {},
): Promise<Uint8Array> {
  const response = await performRequest(
    path,
    { method: "GET", headers: { Accept: expectedContentType } },
    options,
  );
  if (!response.ok) {
    await handleErrorResponse(response, options);
  }
  const responseContentType = response.headers.get("content-type") ?? "";
  if (!responseContentType.toLowerCase().startsWith(expectedContentType)) {
    throw invalidResponse(
      response.status,
      "The device returned an unexpected content type.",
    );
  }
  const buffer = await response.arrayBuffer();
  return new Uint8Array(buffer);
}
