import { apiRequest } from "./client";
import { isExecutionAccepted } from "./executionGuards";
import {
  isCancelAccepted,
  isDeviceStatus,
  isExecutionStatus,
  isEmptyRecord,
  isLoginResponse,
  isMacro,
  isMacroList,
  isMacroPackage,
  isMacroPackageList,
  isMacroValidation,
  isRestartAccepted,
  isSessionStatus,
  isSettings,
  isSetupState,
} from "./guards";
import {
  isFactoryResetAccepted,
  isFullDiagnostics,
  isPackageDeletion,
  isStorageHealth,
} from "./managementGuards";
import type {
  CancelAccepted,
  DeviceStatus,
  ExecutionAccepted,
  ExecutionStatus,
  ExecutionSubmitRequest,
  FactoryResetAccepted,
  FullDiagnostics,
  LoginResponse,
  Macro,
  MacroPackage,
  MacroValidation,
  RestartAccepted,
  SessionStatus,
  PackageDeletion,
  Settings,
  SetupState,
  StorageHealth,
} from "../types/models";

export interface SetupSubmission {
  setupCode: string;
  apSsid: string;
  apPassphrase: string;
  administratorPassword: string;
  requirePhysicalConfirmation: boolean;
  alwaysSelectPackage: boolean;
}

/* No activePackageId: the active package is repository state (SPEC 12.3) and moves only
   through selectPackage(). The settings response still reports it. */
export interface SettingsUpdate {
  expectedRevision: number;
  requirePhysicalConfirmation: boolean;
  alwaysSelectPackage: boolean;
}

export interface PackageDuplicateRequest {
  id: string;
  name: string;
  expectedRevision: number;
}

const physicalConfirmationOptions = { timeoutMs: 25_000 } as const;

export async function getSetupState(): Promise<SetupState> {
  return apiRequest("/api/v1/setup-state", {}, isSetupState, {
    notifyOnUnauthorized: false,
  });
}

export async function submitSetup(
  submission: SetupSubmission,
): Promise<SetupState> {
  return apiRequest(
    "/api/v1/setup/credentials",
    {
      method: "POST",
      body: JSON.stringify(submission),
    },
    isSetupState,
    { notifyOnUnauthorized: false },
  );
}

export async function restartAfterSetup(): Promise<RestartAccepted> {
  return apiRequest(
    "/api/v1/setup/restart",
    { method: "POST" },
    isRestartAccepted,
    { notifyOnUnauthorized: false },
  );
}

export async function getDeviceStatus(): Promise<DeviceStatus> {
  return apiRequest("/api/v1/status", {}, isDeviceStatus);
}

export async function getSession(): Promise<SessionStatus> {
  const response = await apiRequest(
    "/api/v1/auth/session",
    {},
    isSessionStatus,
  );
  return response;
}

export async function login(password: string): Promise<LoginResponse> {
  const response = await apiRequest(
    "/api/v1/auth/login",
    {
      method: "POST",
      body: JSON.stringify({ password }),
    },
    isLoginResponse,
    { notifyOnUnauthorized: false },
  );
  return response;
}

export async function logout(): Promise<void> {
  await apiRequest("/api/v1/auth/logout", { method: "POST" }, isEmptyRecord);
}

export async function getSettings(): Promise<Settings> {
  return apiRequest("/api/v1/settings", {}, isSettings);
}

export async function updateSettings(
  replacement: SettingsUpdate,
): Promise<Settings> {
  return apiRequest(
    "/api/v1/settings",
    {
      method: "PUT",
      body: JSON.stringify(replacement),
    },
    isSettings,
  );
}

export async function resetSettings(
  expectedRevision: number,
): Promise<Settings> {
  return apiRequest(
    "/api/v1/device/reset-settings",
    {
      method: "POST",
      body: JSON.stringify({ expectedRevision }),
    },
    isSettings,
    physicalConfirmationOptions,
  );
}

export async function restartDevice(): Promise<RestartAccepted> {
  return apiRequest(
    "/api/v1/device/restart",
    { method: "POST" },
    isRestartAccepted,
    physicalConfirmationOptions,
  );
}

export async function factoryResetDevice(): Promise<FactoryResetAccepted> {
  return apiRequest(
    "/api/v1/device/factory-reset",
    { method: "POST" },
    isFactoryResetAccepted,
    physicalConfirmationOptions,
  );
}

function packagePath(packageId: string): string {
  return `/api/v1/package/${encodeURIComponent(packageId)}`;
}

export async function listPackages(): Promise<MacroPackage[]> {
  return apiRequest("/api/v1/package", {}, isMacroPackageList);
}

export async function getPackage(packageId: string): Promise<MacroPackage> {
  return apiRequest(packagePath(packageId), {}, isMacroPackage);
}

export async function createPackage(pkg: MacroPackage): Promise<MacroPackage> {
  return apiRequest(
    "/api/v1/package",
    {
      method: "POST",
      body: JSON.stringify(pkg),
    },
    isMacroPackage,
  );
}

export async function updatePackage(
  pkg: MacroPackage,
  expectedRevision: number,
): Promise<MacroPackage> {
  return apiRequest(
    packagePath(pkg.id),
    {
      method: "PUT",
      body: JSON.stringify({ expectedRevision, resource: pkg }),
    },
    isMacroPackage,
  );
}

export async function deletePackage(
  packageId: string,
  expectedRevision: number,
): Promise<PackageDeletion> {
  return apiRequest(
    packagePath(packageId),
    {
      method: "DELETE",
      body: JSON.stringify({ expectedRevision }),
    },
    isPackageDeletion,
  );
}

export async function duplicatePackage(
  packageId: string,
  request: PackageDuplicateRequest,
): Promise<MacroPackage> {
  return apiRequest(
    `${packagePath(packageId)}/duplicate`,
    {
      method: "POST",
      body: JSON.stringify(request),
    },
    isMacroPackage,
  );
}

export async function reorderPackages(
  ids: readonly string[],
): Promise<MacroPackage[]> {
  return apiRequest(
    "/api/v1/package/order",
    {
      method: "PUT",
      body: JSON.stringify({ ids }),
    },
    isMacroPackageList,
  );
}

export async function selectPackage(
  packageId: string,
  expectedRevision: number,
): Promise<Settings> {
  return apiRequest(
    `${packagePath(packageId)}/select`,
    {
      method: "POST",
      body: JSON.stringify({ expectedRevision }),
    },
    isSettings,
  );
}

function packageMacrosPath(packageId: string): string {
  return `${packagePath(packageId)}/macros`;
}

function packageMacroPath(packageId: string, macroId: string): string {
  return `${packageMacrosPath(packageId)}/${encodeURIComponent(macroId)}`;
}

export async function listPackageMacros(packageId: string): Promise<Macro[]> {
  return apiRequest(packageMacrosPath(packageId), {}, isMacroList);
}

export async function getPackageMacro(
  packageId: string,
  macroId: string,
): Promise<Macro> {
  return apiRequest(packageMacroPath(packageId, macroId), {}, isMacro);
}

export async function createPackageMacro(
  packageId: string,
  macro: Macro,
): Promise<Macro> {
  return apiRequest(
    packageMacrosPath(packageId),
    {
      method: "POST",
      body: JSON.stringify(macro),
    },
    isMacro,
  );
}

export async function updatePackageMacro(
  packageId: string,
  macro: Macro,
  expectedRevision: number,
): Promise<Macro> {
  return apiRequest(
    packageMacroPath(packageId, macro.id),
    {
      method: "PUT",
      body: JSON.stringify({
        expectedRevision,
        resource: macro,
      }),
    },
    isMacro,
  );
}

export async function validatePackageMacro(
  packageId: string,
  macro: Macro,
): Promise<MacroValidation> {
  return apiRequest(
    `${packageMacroPath(packageId, macro.id)}/validate`,
    {
      method: "POST",
      body: JSON.stringify(macro),
    },
    isMacroValidation,
  );
}

export async function submitExecution(
  request: ExecutionSubmitRequest,
): Promise<ExecutionAccepted> {
  return apiRequest(
    "/api/v1/executions",
    {
      method: "POST",
      body: JSON.stringify(request),
    },
    isExecutionAccepted,
    physicalConfirmationOptions,
  );
}

export async function getCurrentExecution(): Promise<ExecutionStatus> {
  return apiRequest("/api/v1/executions/current", {}, isExecutionStatus);
}

export async function cancelCurrentExecution(): Promise<CancelAccepted> {
  return apiRequest(
    "/api/v1/executions/current/cancel",
    { method: "POST" },
    isCancelAccepted,
  );
}

export async function getStorageHealth(): Promise<StorageHealth> {
  return apiRequest("/api/v1/diagnostics/storage", {}, isStorageHealth);
}

export async function getFullDiagnostics(): Promise<FullDiagnostics> {
  return apiRequest("/api/v1/diagnostics", {}, isFullDiagnostics);
}
