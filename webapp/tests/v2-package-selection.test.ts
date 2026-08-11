import { describe, expect, test, vi } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import type { Repository } from "../src/v2/repository";
import { createEmptyRepository } from "../src/v2/repositoryValidation";
import {
  persistSelectedPackageId,
  resolveSelectedPackage,
  tryPersistSelectedPackageId,
} from "../src/v2/packageSelection";
import { getFetchCalls, jsonResponse, planFetch } from "./fakeFetch";

const canonical = canonicalRepository as Repository;
const canonicalPackageId = "550e8400-e29b-41d4-a716-446655440000";
const secondPackage = {
  id: "6ba7b810-9dad-41d1-80b4-00c04fd430c9",
  name: "Second package",
  macros: [],
};

const settingsResponse = {
  deviceName: "Desk Macro Keyboard",
  requireSerialConfirmation: false,
  sendMode: "quick",
  snapshotRetentionTarget: 5,
  showMacroSourcePreviews: false,
  lastSelectedPackageId: canonicalPackageId,
  apSsid: "MacroKeyboard",
  stationConfigured: false,
  stationSsid: null,
};

describe("v2 package selection resolution", () => {
  test("resolves to lastSelectedPackageId when it identifies a package in the repository", () => {
    const resolution = resolveSelectedPackage(canonical, canonicalPackageId);
    expect(resolution).toEqual({
      kind: "resolved",
      packageId: canonicalPackageId,
      shouldPersist: false,
    });
  });

  test("auto-selects the sole package and marks it for persistence when the preference does not match", () => {
    const resolution = resolveSelectedPackage(canonical, null);
    expect(resolution).toEqual({
      kind: "resolved",
      packageId: canonicalPackageId,
      shouldPersist: true,
    });
  });

  test("auto-selects the sole package even when the stored preference points at an unknown package", () => {
    const resolution = resolveSelectedPackage(
      canonical,
      "00000000-0000-4000-8000-000000000000",
    );
    expect(resolution).toEqual({
      kind: "resolved",
      packageId: canonicalPackageId,
      shouldPersist: true,
    });
  });

  test("shows the chooser for an empty repository", () => {
    const resolution = resolveSelectedPackage(createEmptyRepository(), null);
    expect(resolution).toEqual({ kind: "chooser" });
  });

  test("shows the chooser with multiple packages and no resolvable selection", () => {
    const repository: Repository = {
      ...canonical,
      packages: [...canonical.packages, secondPackage],
    };
    expect(resolveSelectedPackage(repository, null)).toEqual({
      kind: "chooser",
    });
  });

  test("resolves the matching package among several when the preference identifies one", () => {
    const repository: Repository = {
      ...canonical,
      packages: [...canonical.packages, secondPackage],
    };
    expect(resolveSelectedPackage(repository, secondPackage.id)).toEqual({
      kind: "resolved",
      packageId: secondPackage.id,
      shouldPersist: false,
    });
  });
});

describe("v2 package selection persistence", () => {
  test("does not call the network when the selection is unchanged", async () => {
    const result = await persistSelectedPackageId(
      canonicalPackageId,
      canonicalPackageId,
    );
    expect(result).toBeNull();
    expect(getFetchCalls()).toHaveLength(0);
  });

  test("does not call the network when both values are null", async () => {
    const result = await persistSelectedPackageId(null, null);
    expect(result).toBeNull();
    expect(getFetchCalls()).toHaveLength(0);
  });

  test("explicit persistence attempt reports success without throwing", async () => {
    const persist = vi.fn().mockResolvedValue(null);
    const attempt = await tryPersistSelectedPackageId(
      canonicalPackageId,
      null,
      persist,
    );

    expect(attempt).toEqual({ kind: "persisted" });
    expect(persist).toHaveBeenCalledWith(canonicalPackageId, null);
  });

  test("explicit persistence attempt preserves the failed target and prior durable selection", async () => {
    const error = new Error("settings unavailable");
    const persist = vi.fn().mockRejectedValue(error);
    const attempt = await tryPersistSelectedPackageId(
      secondPackage.id,
      canonicalPackageId,
      persist,
    );

    expect(attempt).toEqual({
      kind: "failed",
      failure: {
        packageId: secondPackage.id,
        previousPackageId: canonicalPackageId,
        error,
      },
    });
  });

  test("updates settings only when the selected ID changes", async () => {
    planFetch((call) => {
      expect(call.method).toBe("PUT");
      expect(call.url).toBe("/api/v1/settings");
      expect(call.body).toBe(
        JSON.stringify({ lastSelectedPackageId: canonicalPackageId }),
      );
      return jsonResponse(
        {
          settings: settingsResponse,
          restartRequired: false,
          reconnectRequired: false,
        },
        200,
      );
    });

    const result = await persistSelectedPackageId(canonicalPackageId, null);
    expect(result).toEqual({
      settings: settingsResponse,
      restartRequired: false,
      reconnectRequired: false,
    });
  });
});
