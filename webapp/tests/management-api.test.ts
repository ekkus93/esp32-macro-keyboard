import { describe, expect, test } from "vitest";
import {
  createPackage,
  deletePackage,
  duplicatePackage,
  factoryResetDevice,
  getStorageHealth,
  reorderPackages,
  resetSettings,
  restartDevice,
  updatePackage,
} from "../src/api/routes";
import { macroPackage, packageId, settings } from "./appFixtures";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";

const secondPackageId = "99999999-9999-4999-8999-999999999999";
const secondPackage = {
  ...macroPackage,
  id: secondPackageId,
  revision: 1,
  name: "Second package",
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
    planJsonResponse(success(macroPackage), 201);
    planJsonResponse(success({ ...macroPackage, revision: 3 }));
    planJsonResponse(success(secondPackage), 201);
    planJsonResponse(success([secondPackage, macroPackage]));
    planJsonResponse(success({ deleted: true, id: secondPackageId }));

    await createPackage(macroPackage);
    await updatePackage(
      { ...macroPackage, name: "Updated" },
      macroPackage.revision,
    );
    await duplicatePackage(packageId, {
      id: secondPackageId,
      name: "Second package",
      expectedRevision: macroPackage.revision,
    });
    await reorderPackages([secondPackageId, packageId]);
    await deletePackage(secondPackageId, secondPackage.revision);

    expect(getFetchCalls().map((call) => [call.method, call.url])).toEqual([
      ["POST", "/api/v1/package"],
      ["PUT", `/api/v1/package/${packageId}`],
      ["POST", `/api/v1/package/${packageId}/duplicate`],
      ["PUT", "/api/v1/package/order"],
      ["DELETE", `/api/v1/package/${secondPackageId}`],
    ]);
    expect(bodyAt(0)).toEqual(macroPackage);
    expect(bodyAt(1)).toEqual({
      expectedRevision: macroPackage.revision,
      resource: { ...macroPackage, name: "Updated" },
    });
    expect(bodyAt(2)).toEqual({
      id: secondPackageId,
      name: "Second package",
      expectedRevision: macroPackage.revision,
    });
    expect(bodyAt(3)).toEqual({ ids: [secondPackageId, packageId] });
    expect(bodyAt(4)).toEqual({ expectedRevision: secondPackage.revision });
  });

  test("uses bounded administration requests and strict acknowledgements", async () => {
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
        packageFileMaxBytes: 32768,
        temporariesRemovedAtBoot: 0,
        discardedObjectCount: 0,
        discardedObjects: [],
      }),
    );

    await expect(getStorageHealth()).resolves.toMatchObject({
      dataMounted: true,
      usedBytes: 20480,
      totalBytes: 491520,
      remainingBytes: 471040,
      packageFileMaxBytes: 32768,
      temporariesRemovedAtBoot: 0,
      discardedObjectCount: 0,
      discardedObjects: [],
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
