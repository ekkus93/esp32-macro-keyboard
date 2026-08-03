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
  expectedKeys: readonly string[],
): boolean {
  const actualKeys = Object.keys(value).sort();
  const sortedExpected = [...expectedKeys].sort();
  return (
    actualKeys.length === sortedExpected.length &&
    actualKeys.every((key, index) => key === sortedExpected[index])
  );
}

function isSetupGetRoutePolicy(value: unknown): value is SetupGetRoutePolicy {
  return (
    isPlainRecord(value) &&
    hasExactKeys(value, [
      "authentication",
      "method",
      "path",
      "requestBody",
      "responseContentType",
      "successStatus",
    ]) &&
    value.method === "GET" &&
    value.path === "/api/v1/setup" &&
    value.authentication === "none" &&
    value.requestBody === "none" &&
    value.successStatus === 200 &&
    value.responseContentType === "application/json"
  );
}

function isSetupPostRoutePolicy(value: unknown): value is SetupPostRoutePolicy {
  return (
    isPlainRecord(value) &&
    hasExactKeys(value, [
      "authentication",
      "method",
      "path",
      "requestBodyLimit",
      "requestContentType",
      "responseContentType",
      "successStatus",
    ]) &&
    value.method === "POST" &&
    value.path === "/api/v1/setup" &&
    value.authentication === "none" &&
    value.requestContentType === "application/json" &&
    value.requestBodyLimit === "jsonBodyMaxBytes" &&
    value.successStatus === 202 &&
    value.responseContentType === "application/json"
  );
}

export function isSetupRoutePolicy(value: unknown): value is SetupRoutePolicy {
  if (
    !isPlainRecord(value) ||
    !hasExactKeys(value, ["format", "provisioned", "unprovisioned", "version"]) ||
    value.format !== "esp32-macro-keyboard-setup-route-policy" ||
    value.version !== 1
  ) {
    return false;
  }

  const unprovisioned = value.unprovisioned;
  const provisioned = value.provisioned;
  if (
    !isPlainRecord(unprovisioned) ||
    !hasExactKeys(unprovisioned, ["apiRoutes", "otherApiRoutes"]) ||
    !Array.isArray(unprovisioned.apiRoutes) ||
    unprovisioned.apiRoutes.length !== 2 ||
    !Object.hasOwn(unprovisioned.apiRoutes, 0) ||
    !Object.hasOwn(unprovisioned.apiRoutes, 1) ||
    !isSetupGetRoutePolicy(unprovisioned.apiRoutes[0]) ||
    !isSetupPostRoutePolicy(unprovisioned.apiRoutes[1]) ||
    unprovisioned.otherApiRoutes !== "unavailable"
  ) {
    return false;
  }

  return (
    isPlainRecord(provisioned) &&
    hasExactKeys(provisioned, ["getSetupStatus", "postSetupStatus"]) &&
    provisioned.getSetupStatus === 404 &&
    provisioned.postSetupStatus === 409
  );
}
