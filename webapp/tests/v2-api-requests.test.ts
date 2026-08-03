import { describe, expect, test } from "vitest";
import examples from "../../contracts/v2/api/examples.json";
import {
  isFactoryResetRequest,
  isLoginRequest,
  isPasswordChangeRequest,
  isResetSettingsRequest,
  isSendRequest,
  isSettingsUpdateRequest,
  isSetupRequest,
} from "../src/v2/apiContracts";

function withUnknownField(value: unknown): Record<string, unknown> {
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    throw new Error("fixture must be an object");
  }
  return { ...value, unexpected: true };
}

describe("v2 API request contracts", () => {
  test("accepts all canonical checked-in request examples", () => {
    expect(isSetupRequest(examples.setupRequest)).toBe(true);
    expect(isLoginRequest(examples.loginRequest)).toBe(true);
    expect(isSettingsUpdateRequest(examples.settingsUpdate)).toBe(true);
    expect(isPasswordChangeRequest(examples.passwordChangeRequest)).toBe(true);
    expect(isSendRequest(examples.sendRequest)).toBe(true);
    expect(isResetSettingsRequest(examples.resetSettingsRequest)).toBe(true);
    expect(isFactoryResetRequest(examples.factoryResetRequest)).toBe(true);
  });

  test("rejects unknown fields on every JSON request", () => {
    expect(isSetupRequest(withUnknownField(examples.setupRequest))).toBe(false);
    expect(isLoginRequest(withUnknownField(examples.loginRequest))).toBe(false);
    expect(
      isSettingsUpdateRequest(withUnknownField(examples.settingsUpdate)),
    ).toBe(false);
    expect(
      isPasswordChangeRequest(withUnknownField(examples.passwordChangeRequest)),
    ).toBe(false);
    expect(isSendRequest(withUnknownField(examples.sendRequest))).toBe(false);
    expect(
      isResetSettingsRequest(withUnknownField(examples.resetSettingsRequest)),
    ).toBe(false);
    expect(
      isFactoryResetRequest(withUnknownField(examples.factoryResetRequest)),
    ).toBe(false);
  });

  test("enforces setup code, UTF-8, Wi-Fi, and password boundaries", () => {
    expect(
      isSetupRequest({
        ...examples.setupRequest,
        setupCode: "1234567",
      }),
    ).toBe(false);
    expect(
      isSetupRequest({
        ...examples.setupRequest,
        deviceName: "é".repeat(17),
      }),
    ).toBe(false);
    expect(
      isSetupRequest({
        ...examples.setupRequest,
        apSsid: "s".repeat(33),
      }),
    ).toBe(false);
    expect(
      isSetupRequest({
        ...examples.setupRequest,
        apPassphrase: "1234567",
      }),
    ).toBe(false);
    expect(
      isSetupRequest({
        ...examples.setupRequest,
        adminPassword: "p".repeat(11),
      }),
    ).toBe(false);
  });

  test("accepts strict partial settings and rejects ambiguous updates", () => {
    expect(isSettingsUpdateRequest({ sendMode: "preview" })).toBe(true);
    expect(isSettingsUpdateRequest({ station: null })).toBe(true);
    expect(isSettingsUpdateRequest({})).toBe(false);
    expect(isSettingsUpdateRequest({ station: {} })).toBe(false);
    expect(
      isSettingsUpdateRequest({
        station: { ssid: "Office", passphrase: "" },
      }),
    ).toBe(false);
    expect(isSettingsUpdateRequest({ snapshotRetentionTarget: 101 })).toBe(
      false,
    );
    expect(
      isSettingsUpdateRequest({
        lastSelectedPackageId: "550E8400-E29B-41D4-A716-446655440000",
      }),
    ).toBe(false);
  });

  test("enforces password-change boundaries", () => {
    expect(
      isPasswordChangeRequest({
        ...examples.passwordChangeRequest,
        newPassword: "p".repeat(11),
      }),
    ).toBe(false);
    expect(
      isPasswordChangeRequest({
        ...examples.passwordChangeRequest,
        newPassword: "é".repeat(65),
      }),
    ).toBe(false);
  });

  test("enforces send byte and timing boundaries", () => {
    expect(
      isSendRequest({
        source: "s".repeat(4096),
        keyPressMs: 0,
        interKeyMs: 10_000,
      }),
    ).toBe(true);
    expect(
      isSendRequest({
        ...examples.sendRequest,
        source: "é".repeat(2049),
      }),
    ).toBe(false);
    expect(
      isSendRequest({
        ...examples.sendRequest,
        keyPressMs: -1,
      }),
    ).toBe(false);
    expect(
      isSendRequest({
        ...examples.sendRequest,
        interKeyMs: 10_001,
      }),
    ).toBe(false);
  });

  test("requires exact destructive confirmation phrases", () => {
    expect(isResetSettingsRequest({ confirmation: "reset settings" })).toBe(
      false,
    );
    expect(
      isFactoryResetRequest({
        ...examples.factoryResetRequest,
        confirmation: "Factory Reset",
      }),
    ).toBe(false);
    expect(
      isFactoryResetRequest({
        ...examples.factoryResetRequest,
        adminPassword: "short",
      }),
    ).toBe(false);
  });
});
