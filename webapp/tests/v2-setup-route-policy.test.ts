import { describe, expect, test } from "vitest";
import policy from "../../contracts/v2/api/setup-route-policy.json";
import { isSetupRoutePolicy } from "../src/v2/setupRoutePolicy";

describe("v2 setup route policy", () => {
  test("accepts the canonical checked-in policy", () => {
    expect(isSetupRoutePolicy(policy)).toBe(true);
  });

  test("allows only setup GET then setup POST while unprovisioned", () => {
    const reversed = structuredClone(policy);
    reversed.unprovisioned.apiRoutes.reverse();
    expect(isSetupRoutePolicy(reversed)).toBe(false);

    const additionalRoute = structuredClone(policy) as {
      unprovisioned: { apiRoutes: unknown[] };
    };
    additionalRoute.unprovisioned.apiRoutes.push({
      method: "GET",
      path: "/api/v1/status",
    });
    expect(isSetupRoutePolicy(additionalRoute)).toBe(false);
  });

  test("rejects authentication, body, method, and status drift", () => {
    expect(
      isSetupRoutePolicy({
        ...policy,
        unprovisioned: {
          ...policy.unprovisioned,
          apiRoutes: [
            {
              ...policy.unprovisioned.apiRoutes[0],
              authentication: "session",
            },
            policy.unprovisioned.apiRoutes[1],
          ],
        },
      }),
    ).toBe(false);

    expect(
      isSetupRoutePolicy({
        ...policy,
        unprovisioned: {
          ...policy.unprovisioned,
          apiRoutes: [
            policy.unprovisioned.apiRoutes[0],
            {
              ...policy.unprovisioned.apiRoutes[1],
              requestBodyLimit: "unbounded",
            },
          ],
        },
      }),
    ).toBe(false);

    expect(
      isSetupRoutePolicy({
        ...policy,
        provisioned: {
          getSetupStatus: 409,
          postSetupStatus: 404,
        },
      }),
    ).toBe(false);
  });

  test("rejects unknown fields and sparse route arrays", () => {
    expect(isSetupRoutePolicy({ ...policy, unexpected: true })).toBe(false);

    const sparse = structuredClone(policy) as {
      unprovisioned: { apiRoutes: unknown[] };
    };
    delete sparse.unprovisioned.apiRoutes[0];
    expect(isSetupRoutePolicy(sparse)).toBe(false);
  });
});
