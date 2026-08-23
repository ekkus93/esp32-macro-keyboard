import { act } from "react";
import { vi } from "vitest";
import type { ActiveSendSummary } from "../src/features/shell/v2/activeSendSummary";
import {
  MacrosPage,
  type MacrosPageDependencies,
} from "../src/features/macros/v2/MacrosPage";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { SendStatusResponse } from "../src/v2/apiTypes";
import { render } from "./render";

/** Timer-driven React updates (poll callbacks, the completion-ack timeout)
 * must be flushed inside `act` or React warns and the state update can be
 * observed after the assertion rather than before it. */
export async function tick(ms: number): Promise<void> {
  await act(async () => {
    await vi.advanceTimersByTimeAsync(ms);
  });
}

export const packageId = "550e8400-e29b-41d4-a716-446655440000";
export const macroAId = "6ba7b810-9dad-41d1-80b4-00c04fd430c8";
export const macroBId = "6ba7b810-9dad-41d1-80b4-00c04fd430c9";
export const sendId = "11111111-1111-4111-8111-111111111111";

export function makeRepository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [
      {
        id: packageId,
        name: "Build server login",
        macros: [
          {
            id: macroAId,
            name: "Start the build",
            source: "make -j8{ENTER}",
            keyPressMs: 8,
            interKeyMs: 15,
          },
          {
            id: macroBId,
            name: "Open terminal",
            source: "[{CTRL}{ALT}t]",
            keyPressMs: 8,
            interKeyMs: 15,
          },
        ],
      },
    ],
  };
}

export function statusAt(
  state: SendStatusResponse["state"],
  actionIndex: number,
  overrides: Partial<SendStatusResponse> = {},
): SendStatusResponse {
  return {
    id: sendId,
    state,
    actionIndex,
    actionCount: 2,
    estimatedDurationMs: 100,
    cancellationRequested: false,
    error: "",
    releaseError: "",
    ...overrides,
  };
}

export const accepted = {
  id: sendId,
  state: "running" as const,
  actionCount: 2,
  estimatedDurationMs: 100,
};

export interface RenderOptions {
  usbState?: "ready" | "disconnected";
  initialSend?: SendStatusResponse | null;
  showMacroSourcePreviews?: boolean;
  sendMode?: "quick" | "preview";
  onActiveSendChange?: (summary: ActiveSendSummary | null) => void;
  dependencies?: MacrosPageDependencies;
}

export async function renderMacrosPage(options: RenderOptions = {}) {
  const store = createRepositoryWorkingCopyStore(makeRepository());
  const callbacks = {
    onChangePackage: vi.fn(),
    onOpenPreview: vi.fn(),
    onOpenAddMacro: vi.fn(),
    onOpenEditMacro: vi.fn(),
  };
  const result = await render(
    <MacrosPage
      {...(options.dependencies === undefined
        ? {}
        : { dependencies: options.dependencies })}
      initialSend={options.initialSend ?? null}
      onActiveSendChange={options.onActiveSendChange ?? (() => undefined)}
      onChangePackage={callbacks.onChangePackage}
      onOpenAddMacro={callbacks.onOpenAddMacro}
      onOpenEditMacro={callbacks.onOpenEditMacro}
      onOpenPreview={callbacks.onOpenPreview}
      packageId={packageId}
      sendMode={options.sendMode ?? "quick"}
      showMacroSourcePreviews={options.showMacroSourcePreviews ?? false}
      store={store}
      usbState={options.usbState ?? "ready"}
    />,
  );
  return { ...result, store, callbacks };
}
