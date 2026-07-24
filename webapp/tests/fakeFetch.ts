import { vi } from "vitest";

export interface FetchCall {
  url: string;
  method: string;
  headers: Headers;
  credentials: RequestCredentials | undefined;
  body: BodyInit | null | undefined;
  signal: AbortSignal | null;
}

type FetchHandler = (call: FetchCall) => Promise<Response> | Response;

const handlers: FetchHandler[] = [];
const calls: FetchCall[] = [];

export function resetFakeFetch(): void {
  handlers.length = 0;
  calls.length = 0;
}

export function installFakeFetch(): void {
  vi.stubGlobal(
    "fetch",
    async (input: RequestInfo | URL, init?: RequestInit): Promise<Response> => {
      const request = input instanceof Request ? input : null;
      const call: FetchCall = {
        url: request?.url ?? String(input),
        method: init?.method ?? request?.method ?? "GET",
        headers: new Headers(init?.headers ?? request?.headers),
        credentials: init?.credentials ?? request?.credentials,
        body: init?.body,
        signal: init?.signal ?? request?.signal ?? null,
      };
      calls.push(call);
      const handler = handlers.shift();
      if (handler === undefined) {
        throw new Error(`Unexpected fetch: ${call.method} ${call.url}`);
      }
      return handler(call);
    },
  );
}

export function planFetch(handler: FetchHandler): void {
  handlers.push(handler);
}

export function jsonResponse(value: unknown, status = 200): Response {
  return new Response(JSON.stringify(value), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

export function textResponse(
  value: string,
  status = 200,
  contentType = "text/plain",
): Response {
  return new Response(value, {
    status,
    headers: { "Content-Type": contentType },
  });
}

export function planJsonResponse(value: unknown, status = 200): void {
  planFetch(() => jsonResponse(value, status));
}

export function planTextResponse(
  value: string,
  status = 200,
  contentType = "text/plain",
): void {
  planFetch(() => textResponse(value, status, contentType));
}

export function getFetchCalls(): readonly FetchCall[] {
  return calls;
}

export function assertNoPendingFetchPlans(): void {
  if (handlers.length !== 0) {
    throw new Error(`${handlers.length} planned fetch call(s) were not consumed.`);
  }
}
