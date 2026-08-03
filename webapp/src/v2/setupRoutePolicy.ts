export interface SetupGetRoutePolicy {
  method: "GET";
  path: "/api/v1/setup";
  authentication: "none";
  requestBody: "none";
  successStatus: 200;
  responseContentType: "application/json";
}

export interface SetupPostRoutePolicy {
  method: "POST";
  path: "/api/v1/setup";
  authentication: "none";
  requestContentType: "application/json";
  requestBodyLimit: "jsonBodyMaxBytes";
  successStatus: 202;
  responseContentType: "application/json";
}

export interface SetupRoutePolicy {
  format: "esp32-macro-keyboard-setup-route-policy";
  version: 1;
  unprovisioned: {
    apiRoutes: [SetupGetRoutePolicy, SetupPostRoutePolicy];
    otherApiRoutes: "unavailable";
  };
  provisioned: {
    getSetupStatus: 404;
    postSetupStatus: 409;
  };
}

function all(checks: readonly boolean[]): boolean {
  return checks.every(Boolean);
}

function isPlainRecord(value: unknown): value is Record<string, unknown> {
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    return false;
  }
  return Object.getPrototypeOf(value) === Object.prototype;
}

function hasExactKeys(
  value: Record<string, unknown>,
  expectedKeys: readonly string[],
): boolean {
  const actualKeys = Object.keys(value).sort();
  const sortedExpected = [...expectedKeys].sort();
  return all([
    actualKeys.length === sortedExpected.length,
    actualKeys.every((key, index) => key === sortedExpected[index]),
  ]);
}

function isSetupGetRoutePolicy(value: unknown): value is SetupGetRoutePolicy {
  if (!isPlainRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "authentication",
      "method",
      "path",
      "requestBody",
      "responseContentType",
      "successStatus",
    ]),
    value.method === "GET",
    value.path === "/api/v1/setup",
    value.authentication === "none",
    value.requestBody === "none",
    value.successStatus === 200,
    value.responseContentType === "application/json",
  ]);
}

function isSetupPostRoutePolicy(value: unknown): value is SetupPostRoutePolicy {
  if (!isPlainRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, [
      "authentication",
      "method",
      "path",
      "requestBodyLimit",
      "requestContentType",
      "responseContentType",
      "successStatus",
    ]),
    value.method === "POST",
    value.path === "/api/v1/setup",
    value.authentication === "none",
    value.requestContentType === "application/json",
    value.requestBodyLimit === "jsonBodyMaxBytes",
    value.successStatus === 202,
    value.responseContentType === "application/json",
  ]);
}

function hasPolicyIdentity(value: Record<string, unknown>): boolean {
  return all([
    hasExactKeys(value, ["format", "provisioned", "unprovisioned", "version"]),
    value.format === "esp32-macro-keyboard-setup-route-policy",
    value.version === 1,
  ]);
}

function isRouteTuple(value: unknown): boolean {
  if (!Array.isArray(value)) {
    return false;
  }
  return all([
    value.length === 2,
    Object.hasOwn(value, 0),
    Object.hasOwn(value, 1),
    isSetupGetRoutePolicy(value[0]),
    isSetupPostRoutePolicy(value[1]),
  ]);
}

function isUnprovisionedPolicy(value: unknown): boolean {
  if (!isPlainRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["apiRoutes", "otherApiRoutes"]),
    isRouteTuple(value.apiRoutes),
    value.otherApiRoutes === "unavailable",
  ]);
}

function isProvisionedPolicy(value: unknown): boolean {
  if (!isPlainRecord(value)) {
    return false;
  }
  return all([
    hasExactKeys(value, ["getSetupStatus", "postSetupStatus"]),
    value.getSetupStatus === 404,
    value.postSetupStatus === 409,
  ]);
}

export function isSetupRoutePolicy(value: unknown): value is SetupRoutePolicy {
  if (!isPlainRecord(value)) {
    return false;
  }
  return all([
    hasPolicyIdentity(value),
    isUnprovisionedPolicy(value.unprovisioned),
    isProvisionedPolicy(value.provisioned),
  ]);
}
