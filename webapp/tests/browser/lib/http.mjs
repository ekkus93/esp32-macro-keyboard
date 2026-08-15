import { extname } from "node:path";

export function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

export function contentType(path) {
  switch (extname(path)) {
    case ".css":
      return "text/css; charset=utf-8";
    case ".html":
      return "text/html; charset=utf-8";
    case ".js":
      return "text/javascript; charset=utf-8";
    case ".json":
      return "application/json; charset=utf-8";
    case ".svg":
      return "image/svg+xml";
    default:
      return "application/octet-stream";
  }
}

export function sendJson(response, status_, data) {
  const body = JSON.stringify(data);
  response.writeHead(status_, {
    "Content-Type": "application/json; charset=utf-8",
    "Content-Length": Buffer.byteLength(body),
    "Cache-Control": "no-store",
  });
  response.end(body);
}

export function sendError(response, status_, code, message) {
  sendJson(response, status_, { error: { code, message } });
}

export async function requestBody(request) {
  const chunks = [];
  for await (const chunk of request) {
    chunks.push(chunk);
  }
  const text = Buffer.concat(chunks).toString("utf8");
  return text.length === 0 ? null : JSON.parse(text);
}

export async function rawRequestBody(request) {
  const chunks = [];
  for await (const chunk of request) {
    chunks.push(chunk);
  }
  return Buffer.concat(chunks);
}

/**
 * Advances the fixture's one active send by one simulated poll, per
 * `source` (each scenario macro uses a distinct, recognizable source so one
 * real macro row exercises one Phase 9 exit-gate terminal state).
 */
