import { describe, expect, test } from "vitest";
import examples from "../../contracts/v2/api/examples.json";
import {
  isActionAccepted,
  isBlobCreatedResponse,
  isBlobListResponse,
  isDiagnosticsResponse,
  isErrorEnvelope,
  isFactoryResetAccepted,
  isLimitsResponse,
  isResetSettingsAccepted,
  isSendAcceptedResponse,
  isSendStatusResponse,
  isSessionStatus,
  isSettingsResponse,
  isSettingsUpdatedResponse,
  isSetupAccepted,
  isSetupStateResponse,
  isStatusResponse,
} from "../src/v2/apiContracts";

function withUnknownField(value: unknown): Record<string, unknown> {
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    throw new Error("fixture must be an object");
  }
  return { ...value, unexpected: true };
}

function sparseArray<T>(length: number): T[] {
  const value: T[] = [];
  value.length = length;
  return value;
}

const limitKeys = [
  "packageNameMaxBytes",
  "macroNameMaxBytes",
  "macroSourceMaxBytes",
  "compiledActionsMax",
  "delayDirectiveMaxMs",
  "keyPressMaxMs",
  "interKeyMaxMs",
  "estimatedDurationMaxMs",
  "executorAbsoluteDeadlineMs",
  "jsonBodyMaxBytes",
  "blobMaxBytes",
  "activeSessionsMax",
  "sessionIdleLifetimeSeconds",
  "sessionAbsoluteLifetimeSeconds",
  "serialConfirmationTimeoutSeconds",
  "adminPasswordMinBytes",
  "adminPasswordMaxBytes",
  "snapshotRetentionTargetMax",
] as const;

describe("v2 API response contracts", () => {
  test("accepts every canonical checked-in response example", () => {
    expect(isErrorEnvelope(examples.error)).toBe(true);
    expect(isErrorEnvelope(examples.parserError)).toBe(true);
    expect(isSetupStateResponse(examples.setupState)).toBe(true);
    expect(isSetupAccepted(examples.setupAccepted)).toBe(true);
    expect(isSessionStatus(examples.session)).toBe(true);
    expect(isStatusResponse(examples.status)).toBe(true);
    expect(isLimitsResponse(examples.limits)).toBe(true);
    expect(isBlobListResponse(examples.blobList)).toBe(true);
    expect(isBlobCreatedResponse(examples.blobCreated)).toBe(true);
    expect(isSettingsResponse(examples.settings)).toBe(true);
    expect(isSettingsUpdatedResponse(examples.settingsUpdated)).toBe(true);
    expect(isSendAcceptedResponse(examples.sendAccepted)).toBe(true);
    expect(isSendStatusResponse(examples.sendStatus)).toBe(true);
    expect(isActionAccepted(examples.restartAccepted)).toBe(true);
    expect(isResetSettingsAccepted(examples.resetSettingsAccepted)).toBe(true);
    expect(isFactoryResetAccepted(examples.factoryResetAccepted)).toBe(true);
    expect(isDiagnosticsResponse(examples.diagnostics)).toBe(true);
  });

  test("rejects unknown fields on every top-level response", () => {
    expect(isErrorEnvelope(withUnknownField(examples.error))).toBe(false);
    expect(isSetupStateResponse(withUnknownField(examples.setupState))).toBe(
      false,
    );
    expect(isSetupAccepted(withUnknownField(examples.setupAccepted))).toBe(
      false,
    );
    expect(isSessionStatus(withUnknownField(examples.session))).toBe(false);
    expect(isStatusResponse(withUnknownField(examples.status))).toBe(false);
    expect(isLimitsResponse(withUnknownField(examples.limits))).toBe(false);
    expect(isBlobListResponse(withUnknownField(examples.blobList))).toBe(false);
    expect(isBlobCreatedResponse(withUnknownField(examples.blobCreated))).toBe(
      false,
    );
    expect(isSettingsResponse(withUnknownField(examples.settings))).toBe(false);
    expect(
      isSettingsUpdatedResponse(withUnknownField(examples.settingsUpdated)),
    ).toBe(false);
    expect(
      isSendAcceptedResponse(withUnknownField(examples.sendAccepted)),
    ).toBe(false);
    expect(isSendStatusResponse(withUnknownField(examples.sendStatus))).toBe(
      false,
    );
    expect(isActionAccepted(withUnknownField(examples.restartAccepted))).toBe(
      false,
    );
    expect(
      isResetSettingsAccepted(withUnknownField(examples.resetSettingsAccepted)),
    ).toBe(false);
    expect(
      isFactoryResetAccepted(withUnknownField(examples.factoryResetAccepted)),
    ).toBe(false);
    expect(isDiagnosticsResponse(withUnknownField(examples.diagnostics))).toBe(
      false,
    );
  });

  test("rejects unknown fields in nested response objects", () => {
    expect(
      isErrorEnvelope({
        error: { ...examples.error.error, unexpected: true },
      }),
    ).toBe(false);

    for (const field of [
      "usb",
      "accessPoint",
      "station",
      "storage",
      "send",
    ] as const) {
      const status = structuredClone(examples.status) as Record<
        string,
        unknown
      >;
      status[field] = withUnknownField(status[field]);
      expect(isStatusResponse(status)).toBe(false);
    }

    expect(
      isBlobListResponse({
        ...examples.blobList,
        blobs: [withUnknownField(examples.blobList.blobs[0])],
      }),
    ).toBe(false);

    expect(
      isSettingsUpdatedResponse({
        ...examples.settingsUpdated,
        settings: withUnknownField(examples.settingsUpdated.settings),
      }),
    ).toBe(false);

    for (const field of ["memory", "usb", "wifi", "storage", "send"] as const) {
      const diagnostics = structuredClone(examples.diagnostics) as Record<
        string,
        unknown
      >;
      diagnostics[field] = withUnknownField(diagnostics[field]);
      expect(isDiagnosticsResponse(diagnostics)).toBe(false);
    }

    expect(
      isDiagnosticsResponse({
        ...examples.diagnostics,
        subsystems: [{ name: "storage", state: "healthy", unexpected: true }],
      }),
    ).toBe(false);
  });

  test("rejects sparse arrays in every API array field", () => {
    expect(
      isBlobListResponse({
        ...examples.blobList,
        blobs: sparseArray<(typeof examples.blobList.blobs)[number]>(1),
      }),
    ).toBe(false);

    const invalidNames = structuredClone(examples.diagnostics) as unknown as {
      storage: { invalidNames: string[] };
    };
    invalidNames.storage.invalidNames = sparseArray<string>(1);
    expect(isDiagnosticsResponse(invalidNames)).toBe(false);

    const temporaryFiles = structuredClone(examples.diagnostics) as unknown as {
      storage: { temporaryFiles: string[] };
    };
    temporaryFiles.storage.temporaryFiles = sparseArray<string>(1);
    expect(isDiagnosticsResponse(temporaryFiles)).toBe(false);

    const subsystems = structuredClone(examples.diagnostics) as unknown as {
      subsystems: unknown[];
    };
    subsystems.subsystems = sparseArray<unknown>(1);
    expect(isDiagnosticsResponse(subsystems)).toBe(false);
  });

  test("enforces the minimal unprovisioned setup-state surface", () => {
    expect(
      isSetupStateResponse({
        ...examples.setupState,
        provisioned: true,
      }),
    ).toBe(false);
    expect(
      isSetupStateResponse({
        ...examples.setupState,
        deviceName: "",
      }),
    ).toBe(false);

    for (const sensitiveField of [
      "setupCode",
      "apSsid",
      "stationConfigured",
      "firmwareVersion",
      "diagnostics",
      "repository",
    ]) {
      expect(
        isSetupStateResponse({
          ...examples.setupState,
          [sensitiveField]: "must-not-appear",
        }),
      ).toBe(false);
    }
  });

  test("requires all parser coordinates together", () => {
    const invalid = structuredClone(examples.parserError);
    delete (invalid.error as { column?: number }).column;
    expect(isErrorEnvelope(invalid)).toBe(false);
  });

  test("requires exact session lifetimes and every limits field", () => {
    expect(
      isSessionStatus({
        ...examples.session,
        idleExpiresInSeconds: 60,
      }),
    ).toBe(false);

    for (const key of limitKeys) {
      const missing: Partial<typeof examples.limits> = { ...examples.limits };
      expect(Reflect.deleteProperty(missing, key)).toBe(true);
      expect(isLimitsResponse(missing)).toBe(false);

      expect(
        isLimitsResponse({
          ...examples.limits,
          [key]: examples.limits[key] + 1,
        }),
      ).toBe(false);
    }
  });

  test("rejects invalid blob IDs, order, and size", () => {
    expect(
      isBlobCreatedResponse({
        id: "0004",
        sizeBytes: 1350,
      }),
    ).toBe(false);
    expect(
      isBlobCreatedResponse({
        id: "4",
        sizeBytes: 131_073,
      }),
    ).toBe(false);
    expect(
      isBlobListResponse({
        ...examples.blobList,
        blobs: [...examples.blobList.blobs].reverse(),
      }),
    ).toBe(false);
  });

  test("enforces settings UUID and station consistency", () => {
    expect(
      isSettingsResponse({
        ...examples.settings,
        lastSelectedPackageId: "550E8400-E29B-41D4-A716-446655440000",
      }),
    ).toBe(false);
    expect(
      isSettingsResponse({
        ...examples.settings,
        stationConfigured: true,
        stationSsid: null,
      }),
    ).toBe(false);
  });

  test("enforces send identity, progress, state, and limits", () => {
    expect(
      isSendAcceptedResponse({
        ...examples.sendAccepted,
        state: "completed",
      }),
    ).toBe(false);
    expect(
      isSendStatusResponse({
        ...examples.sendStatus,
        actionIndex: examples.sendStatus.actionCount + 1,
      }),
    ).toBe(false);
    expect(
      isSendStatusResponse({
        ...examples.sendStatus,
        id: examples.sendStatus.id.toUpperCase(),
      }),
    ).toBe(false);
  });

  test("distinguishes reset-settings and factory-reset responses", () => {
    expect(isFactoryResetAccepted(examples.resetSettingsAccepted)).toBe(false);
    expect(isResetSettingsAccepted(examples.factoryResetAccepted)).toBe(false);
    expect(
      isResetSettingsAccepted({
        ...examples.resetSettingsAccepted,
        repositoryBlobsPreserved: false,
      }),
    ).toBe(false);
  });

  test("rejects inconsistent absent-send summaries", () => {
    const status = structuredClone(examples.status) as {
      send: { present: boolean; state: string | null };
    };
    status.send.state = "running";
    expect(isStatusResponse(status)).toBe(false);

    const diagnostics = structuredClone(examples.diagnostics) as {
      send: { present: boolean; state: string | null };
    };
    diagnostics.send.state = "running";
    expect(isDiagnosticsResponse(diagnostics)).toBe(false);
  });

  test("rejects diagnostics secret and repository metadata fields", () => {
    const invalid = structuredClone(examples.diagnostics) as Record<
      string,
      unknown
    >;
    invalid.macroSource = "secret";
    expect(isDiagnosticsResponse(invalid)).toBe(false);
  });
});
