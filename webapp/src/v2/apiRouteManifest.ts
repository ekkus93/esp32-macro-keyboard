export type ApiMethod = "GET" | "POST" | "PUT" | "DELETE";
export type ApiAuthentication =
  | "none-unprovisioned-only"
  | "none-provisioned-only"
  | "session";
export type ApiBody =
  | "none"
  | "setupRequest"
  | "loginRequest"
  | "binaryBlob"
  | "sendRequest"
  | "settingsUpdateRequest"
  | "passwordChangeRequest"
  | "resetSettingsRequest"
  | "factoryResetRequest";
export type ApiContentType = "application/json" | "application/gzip";
export type ApiMaximumBytes = "jsonBodyMaxBytes" | "blobMaxBytes";

export interface ApiRequestContract {
  body: ApiBody;
  contentType: ApiContentType | null;
  maximumBytes: ApiMaximumBytes | null;
}

export interface ApiResponseContract {
  contentType: ApiContentType | null;
  successStatus: number;
}

export interface ApiRouteContract {
  id: string;
  method: ApiMethod;
  path: string;
  authentication: ApiAuthentication;
  request: ApiRequestContract;
  response: ApiResponseContract;
  errorStatuses: number[];
}

export interface ApiRouteManifest {
  format: "esp32-macro-keyboard-api-routes";
  version: 1;
  routes: ApiRouteContract[];
}

const expectedRoutes = [
  ["setupGet", "GET", "/api/v1/setup", "none-unprovisioned-only", "none", null, null, "application/json", 200, "404"],
  ["setupPost", "POST", "/api/v1/setup", "none-unprovisioned-only", "setupRequest", "application/json", "jsonBodyMaxBytes", "application/json", 202, "400,409,413,415,422,500"],
  ["login", "POST", "/api/v1/auth/login", "none-provisioned-only", "loginRequest", "application/json", "jsonBodyMaxBytes", "application/json", 200, "400,403,413,415,422,429,500"],
  ["logout", "POST", "/api/v1/auth/logout", "session", "none", null, null, null, 204, "401,500"],
  ["session", "GET", "/api/v1/auth/session", "session", "none", null, null, "application/json", 200, "401,500"],
  ["status", "GET", "/api/v1/status", "session", "none", null, null, "application/json", 200, "401,500,503"],
  ["limits", "GET", "/api/v1/limits", "session", "none", null, null, "application/json", 200, "401,500"],
  ["blobList", "GET", "/api/v1/blob", "session", "none", null, null, "application/json", 200, "401,500,503"],
  ["blobCreate", "POST", "/api/v1/blob", "session", "binaryBlob", "application/gzip", "blobMaxBytes", "application/json", 201, "400,401,413,415,500,503,507"],
  ["blobLoad", "GET", "/api/v1/blob/{blob_id}", "session", "none", null, null, "application/gzip", 200, "400,401,404,500,503"],
  ["blobDelete", "DELETE", "/api/v1/blob/{blob_id}", "session", "none", null, null, null, 204, "400,401,404,500,503"],
  ["sendCreate", "POST", "/api/v1/send", "session", "sendRequest", "application/json", "jsonBodyMaxBytes", "application/json", 202, "400,401,409,413,415,422,500,503"],
  ["sendGet", "GET", "/api/v1/send", "session", "none", null, null, "application/json", 200, "401,404,500,503"],
  ["sendCancel", "DELETE", "/api/v1/send", "session", "none", null, null, "application/json", 202, "401,404,500,503"],
  ["settingsGet", "GET", "/api/v1/settings", "session", "none", null, null, "application/json", 200, "401,500,503"],
  ["settingsPut", "PUT", "/api/v1/settings", "session", "settingsUpdateRequest", "application/json", "jsonBodyMaxBytes", "application/json", 200, "400,401,413,415,422,500,503"],
  ["passwordChange", "POST", "/api/v1/settings/change-password", "session", "passwordChangeRequest", "application/json", "jsonBodyMaxBytes", null, 204, "400,401,403,413,415,422,500,503"],
  ["restart", "POST", "/api/v1/device/restart", "session", "none", null, null, "application/json", 202, "401,500,503"],
  ["resetSettings", "POST", "/api/v1/device/reset-settings", "session", "resetSettingsRequest", "application/json", "jsonBodyMaxBytes", "application/json", 202, "400,401,413,415,422,500,503"],
  ["factoryReset", "POST", "/api/v1/device/factory-reset", "session", "factoryResetRequest", "application/json", "jsonBodyMaxBytes", "application/json", 202, "400,401,403,413,415,422,500,503"],
  ["diagnostics", "GET", "/api/v1/diagnostics", "session", "none", null, null, "application/json", 200, "401,500,503"],
] as const;

const methods: ReadonlySet<string> = new Set(["GET", "POST", "PUT", "DELETE"]);
const authenticationValues: ReadonlySet<string> = new Set([
  "none-unprovisioned-only",
  "none-provisioned-only",
  "session",
]);
const bodyValues: ReadonlySet<string> = new Set([
  "none",
  "setupRequest",
  "loginRequest",
  "binaryBlob",
  "sendRequest",
  "settingsUpdateRequest",
  "passwordChangeRequest",
  "resetSettingsRequest",
  "factoryResetRequest",
]);
const contentTypes: ReadonlySet<string> = new Set([
  "application/json",
  "application/gzip",
]);
const maximumValues: ReadonlySet<string> = new Set([
  "jsonBodyMaxBytes",
  "blobMaxBytes",
]);
const allowedStatuses = new Set([
  200, 201, 202, 204, 400, 401, 403, 404, 409, 413, 415, 422, 429, 500, 503,
  507,
]);

function isPlainRecord(value: unknown): value is Record<string, unknown> {
  return (
    typeof value === "object" &&
    value !== null &&
    !Array.isArray(value) &&
    Object.getPrototypeOf(value) === Object.prototype
  );
}

function hasExactKeys(
  value: Record<string, unknown>,
  expected: readonly string[],
): boolean {
  const actual = Object.keys(value).sort();
  const sortedExpected = [...expected].sort();
  return (
    actual.length === sortedExpected.length &&
    actual.every((key, index) => key === sortedExpected[index])
  );
}

function isDenseArray(value: unknown[]): boolean {
  for (let index = 0; index < value.length; index += 1) {
    if (!Object.hasOwn(value, index)) {
      return false;
    }
  }
  return true;
}

function isRequest(value: unknown): value is ApiRequestContract {
  if (
    !isPlainRecord(value) ||
    !hasExactKeys(value, ["body", "contentType", "maximumBytes"]) ||
    typeof value.body !== "string" ||
    !bodyValues.has(value.body)
  ) {
    return false;
  }
  const contentType = value.contentType;
  const maximumBytes = value.maximumBytes;
  if (
    contentType !== null &&
    (typeof contentType !== "string" || !contentTypes.has(contentType))
  ) {
    return false;
  }
  if (
    maximumBytes !== null &&
    (typeof maximumBytes !== "string" || !maximumValues.has(maximumBytes))
  ) {
    return false;
  }
  if (value.body === "none") {
    return contentType === null && maximumBytes === null;
  }
  if (value.body === "binaryBlob") {
    return contentType === "application/gzip" && maximumBytes === "blobMaxBytes";
  }
  return (
    contentType === "application/json" && maximumBytes === "jsonBodyMaxBytes"
  );
}

function isResponse(value: unknown): value is ApiResponseContract {
  if (
    !isPlainRecord(value) ||
    !hasExactKeys(value, ["contentType", "successStatus"]) ||
    typeof value.successStatus !== "number" ||
    !Number.isSafeInteger(value.successStatus) ||
    !allowedStatuses.has(value.successStatus)
  ) {
    return false;
  }
  if (value.contentType === null) {
    return value.successStatus === 204;
  }
  return (
    typeof value.contentType === "string" &&
    contentTypes.has(value.contentType) &&
    value.successStatus !== 204
  );
}

function isErrorStatuses(value: unknown, successStatus: number): value is number[] {
  if (!Array.isArray(value) || !isDenseArray(value) || value.length === 0) {
    return false;
  }
  let previous = -1;
  for (const status of value) {
    if (
      typeof status !== "number" ||
      !Number.isSafeInteger(status) ||
      !allowedStatuses.has(status) ||
      status === successStatus ||
      status <= previous
    ) {
      return false;
    }
    previous = status;
  }
  return true;
}

function isRoute(
  value: unknown,
  expected: (typeof expectedRoutes)[number],
): value is ApiRouteContract {
  if (
    !isPlainRecord(value) ||
    !hasExactKeys(value, [
      "authentication",
      "errorStatuses",
      "id",
      "method",
      "path",
      "request",
      "response",
    ]) ||
    typeof value.method !== "string" ||
    !methods.has(value.method) ||
    typeof value.authentication !== "string" ||
    !authenticationValues.has(value.authentication) ||
    !isRequest(value.request) ||
    !isResponse(value.response) ||
    !isErrorStatuses(value.errorStatuses, value.response.successStatus)
  ) {
    return false;
  }

  return (
    value.id === expected[0] &&
    value.method === expected[1] &&
    value.path === expected[2] &&
    value.authentication === expected[3] &&
    value.request.body === expected[4] &&
    value.request.contentType === expected[5] &&
    value.request.maximumBytes === expected[6] &&
    value.response.contentType === expected[7] &&
    value.response.successStatus === expected[8] &&
    value.errorStatuses.join(",") === expected[9]
  );
}

export function isApiRouteManifest(value: unknown): value is ApiRouteManifest {
  if (
    !isPlainRecord(value) ||
    !hasExactKeys(value, ["format", "routes", "version"]) ||
    value.format !== "esp32-macro-keyboard-api-routes" ||
    value.version !== 1 ||
    !Array.isArray(value.routes) ||
    !isDenseArray(value.routes) ||
    value.routes.length !== expectedRoutes.length
  ) {
    return false;
  }

  for (let index = 0; index < expectedRoutes.length; index += 1) {
    const expected = expectedRoutes[index];
    if (expected === undefined || !isRoute(value.routes[index], expected)) {
      return false;
    }
  }

  return true;
}
