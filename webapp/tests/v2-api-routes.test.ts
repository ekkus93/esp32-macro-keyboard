import { describe, expect, test } from "vitest";
import rawManifest from "../../contracts/v2/api/routes.json";
import { isApiRouteManifest } from "../src/v2/apiRouteManifest";

interface MutableRoute {
  id: string;
  method: string;
  path: string;
  authentication: string;
  request: {
    body: string;
    contentType: string | null;
    maximumBytes: string | null;
    unexpected?: boolean;
  };
  response: {
    contentType: string | null;
    successStatus: number;
    unexpected?: boolean;
  };
  errorStatuses: number[];
  unexpected?: boolean;
}

interface MutableManifest {
  format: string;
  version: number;
  routes: MutableRoute[];
  unexpected?: boolean;
}

function manifest(): MutableManifest {
  return structuredClone(rawManifest);
}

function routeAt(value: MutableManifest, index: number): MutableRoute {
  const route = value.routes[index];
  if (route === undefined) {
    throw new Error(`missing route ${String(index)}`);
  }
  return route;
}

describe("v2 API route manifest", () => {
  test("accepts the reviewed checked-in manifest", () => {
    expect(isApiRouteManifest(rawManifest)).toBe(true);
  });

  test("rejects route reordering and extra routes", () => {
    const reordered = manifest();
    const first = routeAt(reordered, 0);
    reordered.routes[0] = routeAt(reordered, 1);
    reordered.routes[1] = first;
    expect(isApiRouteManifest(reordered)).toBe(false);

    const extra = manifest();
    extra.routes.push({
      ...structuredClone(routeAt(extra, 0)),
      id: "legacyPackageList",
      path: "/api/v1/package",
    });
    expect(isApiRouteManifest(extra)).toBe(false);
  });

  test("rejects unknown fields at every manifest level", () => {
    const root = manifest();
    root.unexpected = true;
    expect(isApiRouteManifest(root)).toBe(false);

    const route = manifest();
    routeAt(route, 0).unexpected = true;
    expect(isApiRouteManifest(route)).toBe(false);

    const request = manifest();
    routeAt(request, 1).request.unexpected = true;
    expect(isApiRouteManifest(request)).toBe(false);

    const response = manifest();
    routeAt(response, 0).response.unexpected = true;
    expect(isApiRouteManifest(response)).toBe(false);
  });

  test("rejects changed method, path, authentication, and status", () => {
    const method = manifest();
    routeAt(method, 0).method = "POST";
    expect(isApiRouteManifest(method)).toBe(false);

    const path = manifest();
    routeAt(path, 0).path = "/api/v1/setup-state";
    expect(isApiRouteManifest(path)).toBe(false);

    const authentication = manifest();
    routeAt(authentication, 2).authentication = "session";
    expect(isApiRouteManifest(authentication)).toBe(false);

    const status = manifest();
    routeAt(status, 8).response.successStatus = 200;
    expect(isApiRouteManifest(status)).toBe(false);
  });

  test("rejects changed content type and request body limit", () => {
    const contentType = manifest();
    routeAt(contentType, 8).request.contentType = "application/json";
    expect(isApiRouteManifest(contentType)).toBe(false);

    const responseType = manifest();
    routeAt(responseType, 9).response.contentType = "application/json";
    expect(isApiRouteManifest(responseType)).toBe(false);

    const maximum = manifest();
    routeAt(maximum, 1).request.maximumBytes = "blobMaxBytes";
    expect(isApiRouteManifest(maximum)).toBe(false);
  });

  test("rejects sparse route and error-status arrays", () => {
    const routes = manifest();
    const sparseRoutes: MutableRoute[] = [];
    sparseRoutes.length = routes.routes.length;
    routes.routes = sparseRoutes;
    expect(isApiRouteManifest(routes)).toBe(false);

    const statuses = manifest();
    const sparseStatuses: number[] = [];
    sparseStatuses.length = 1;
    routeAt(statuses, 0).errorStatuses = sparseStatuses;
    expect(isApiRouteManifest(statuses)).toBe(false);
  });

  test("contains no v1 package, macro, or plural execution path", () => {
    const paths = rawManifest.routes.map((route) => route.path);
    expect(paths.some((path) => path.includes("/package"))).toBe(false);
    expect(paths.some((path) => path.includes("/macro"))).toBe(false);
    expect(paths.some((path) => path.includes("/executions"))).toBe(false);
  });
});
