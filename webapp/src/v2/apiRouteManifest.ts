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
  ["setupGet", "GET", "/api/v1/setup"],
  ["setupPost", "POST", "/api/v1/setup"],
  ["login", "POST", "/api/v1/auth/login"],
  ["logout", "POST", "/api/v1/auth/logout"],
  ["session", "GET", "/api/v1/auth/session"],
  ["status", "GET", "/api/v1/status"],
  ["limits", "GET", "/api/v1/limits"],
  ["blobList", "GET", "/api/v1/blob"],
  ["blobCreate", "POST", "/api/v1/blob"],
  ["blobLoad", "GET", "/api/v1/blob/{blob_id}"],
  ["blobDelete", "DELETE", "/api/v1/blob/{blob_id}"],
  ["sendCreate", "POST", "/api/v1/send"],
  ["sendGet", "GET", "/api/v1/send"],
  ["sendCancel", "DELETE", "/api/v1/send"],
  ["settingsGet", "GET", "/api/v1/settings"],
  ["settingsPut", "PUT", "/api/v1/settings"],
  ["passwordChange", "POST", "/api/v1/settings/change-password"],
  ["restart", "POST", "/api/v1/device/restart"],
  ["resetSettings", "POST", "/api/v1/device/reset-settings"],
  ["factoryReset", "POST", "/api/v1/device/factory-reset"],
  ["diagnostics", "GET", "/api/v1/diagnostics"],
] as const;

const methods = new Set<ApiMethod>(["GET", "POST", "PUT", "DELETE"]);
const authenticationValues = new Set<ApiAuthentication>([
  "none-unprovisioned-only",
  "none-provisioned-only",
  "session",
]);
const bodyValues = new Set<ApiBody>([
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
const contentTypes = new Set<ApiContentType>([
  "application/json",
  "application/gzip",
]);
const maximumValues = new Set<ApiMaximumBytes>([
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
    !bodyValues.has(value.body as ApiBody)
  ) {
    return false;
  }
  const contentType = value.contentType;
  const maximumBytes = value.maximumBytes;
  if (
    contentType !== null &&
    (typeof contentType !== "string" ||
      !contentTypes.has(contentType as ApiContentType))
  ) {
    return false;
  }
  if (
    maximumBytes !== null &&
    (typeof maximumBytes !== "string" ||
      !maximumValues.has(maximumBytes as ApiMaximumBytes))
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
    contentTypes.has(value.contentType as ApiContentType) &&
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
    value.id !== expected[0] ||
    value.method !== expected[1] ||
    value.path !== expected[2] ||
    typeof value.method !== "string" ||
    !methods.has(value.method as ApiMethod) ||
    typeof value.authentication !== "string" ||
    !authenticationValues.has(value.authentication as ApiAuthentication)
  ) {
    return false;
  }
  if (!isRequest(value.request) || !isResponse(value.response)) {
    return false;
  }
  if (!isErrorStatuses(value.errorStatuses, value.response.successStatus)) {
    return false;
  }

  if (value.id === "setupGet" || value.id === "setupPost") {
    return value.authentication === "none-unprovisioned-only";
  }
  if (value.id === "login") {
    return value.authentication === "none-provisioned-only";
  }
  return value.authentication === "session";
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
