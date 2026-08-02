import { describe, expect, test } from "vitest";
import { setCsrfToken } from "../src/api/client";
import {
  createSet,
  deleteSet,
  duplicateSet,
  factoryResetDevice,
  getStorageHealth,
  reorderSets,
  resetSettings,
  restartDevice,
  updateSet,
} from "../src/api/routes";
import { macroSet, setId, settings } from "./appFixtures";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";

const secondSetId = "99999999-9999-4999-8999-999999999999";
const secondSet = {
  ...macroSet,
  id: secondSetId,
  revision: 1,
  name: "Second set",
};

function success(data: unknown): object {
  return { ok: true, data };
}

function bodyAt(index: number): unknown {
  const body = getFetchCalls()[index]?.body;
  if (typeof body !== "string") {
    throw new Error(`Request ${String(index)} has no JSON body.`);
  }
  return JSON.parse(body) as unknown;
}

describe("management API contracts", () => {
  test("serializes create, update, duplicate, reorder, and delete exactly", async () => {
    setCsrfToken("csrf-management");
    planJsonResponse(success(macroSet), 201);
    planJsonResponse(success({ ...macroSet, revision: 3 }));
    planJsonResponse(success(secondSet), 201);
    planJsonResponse(success([secondSet, macroSet]));
    planJsonResponse(success({ deleted: true, id: secondSetId }));

    await createSet(macroSet);
    await updateSet({ ...macroSet, name: "Updated" }, macroSet.revision);
    await duplicateSet(setId, {
      id: secondSetId,
      name: "Second set",
      expectedRevision: macroSet.revision,
    });
    await reorderSets([secondSetId, setId]);
    await deleteSet(secondSetId, secondSet.revision);

    expect(getFetchCalls().map((call) => [call.method, call.url])).toEqual([
      ["POST", "/api/v1/sets"],
      ["PUT", `/api/v1/sets/${setId}`],
      ["POST", `/api/v1/sets/${setId}/duplicate`],
      ["PUT", "/api/v1/sets/order"],
      ["DELETE", `/api/v1/sets/${secondSetId}`],
    ]);
    expect(bodyAt(0)).toEqual(macroSet);
    expect(bodyAt(1)).toEqual({
      expectedRevision: macroSet.revision,
      resource: { ...macroSet, name: "Updated" },
    });
    expect(bodyAt(2)).toEqual({
      id: secondSetId,
      name: "Second set",
      expectedRevision: macroSet.revision,
    });
    expect(bodyAt(3)).toEqual({ ids: [secondSetId, setId] });
    expect(bodyAt(4)).toEqual({ expectedRevision: secondSet.revision });
    for (const call of getFetchCalls()) {
      expect(call.headers.get("X-CSRF-Token")).toBe("csrf-management");
    }
  });

  test("uses bounded administration requests and strict acknowledgements", async () => {
    setCsrfToken("csrf-admin");
    planJsonResponse(success({ restartScheduled: true }), 202);
    planJsonResponse(success({ ...settings, revision: settings.revision + 1 }));
    planJsonResponse(
      success({ factoryReset: true, restartScheduled: true }),
      202,
    );

    await restartDevice();
    await resetSettings(settings.revision);
    await factoryResetDevice();

    expect(getFetchCalls().map((call) => [call.method, call.url])).toEqual([
      ["POST", "/api/v1/device/restart"],
      ["POST", "/api/v1/device/reset-settings"],
      ["POST", "/api/v1/device/factory-reset"],
    ]);
    expect(bodyAt(1)).toEqual({ expectedRevision: settings.revision });
  });

  test("validates redacted storage data", async () => {
    planJsonResponse(
      success({
        verified: false,
        webMounted: true,
        dataMounted: true,
        usedBytes: 20480,
        totalBytes: 491520,
        remainingBytes: 471040,
        setFileMaxBytes: 32768,
      }),
    );

    await expect(getStorageHealth()).resolves.toMatchObject({
      dataMounted: true,
      usedBytes: 20480,
      totalBytes: 491520,
      remainingBytes: 471040,
      setFileMaxBytes: 32768,
    });
  });

  test("fails closed on incomplete factory-reset acknowledgement", async () => {
    planFetch(() =>
      jsonResponse(
        success({ factoryReset: true, restartScheduled: false }),
        202,
      ),
    );
    await expect(factoryResetDevice()).rejects.toMatchObject({
      body: { code: "invalid_response" },
    });
  });
});
