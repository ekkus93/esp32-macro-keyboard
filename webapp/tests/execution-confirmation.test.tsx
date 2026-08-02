import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { setCsrfToken } from "../src/api/client";
import { ConfirmExecutionPage } from "../src/features/execution/ConfirmExecutionPage";
import type { ExecutionConfirmationTarget } from "../src/routing";
import type {
  DeviceStatus,
  ExecutionAccepted,
  Macro,
  Settings,
} from "../src/types/models";
import {
  deviceStatus,
  executionId,
  macro,
  macroId,
  macroSet,
  macroStepId,
  procedure,
  procedureId,
  settings,
} from "./appFixtures";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";
import { buttonWithText, click, flushReact, render } from "./render";

const validation = {
  valid: true,
  actionCount: 1,
  estimatedDurationMs: 31,
} as const;

const procedureTarget: ExecutionConfirmationTarget = {
  kind: "valid",
  macroId,
  sourceContext: {
    procedureId,
    stepId: macroStepId,
  },
};

type AcceptedHandler = (
  accepted: ExecutionAccepted,
  returnHash: string,
) => void;

function success(data: unknown): object {
  return { ok: true, data };
}

function requestBody(callIndex: number): Record<string, unknown> {
  const body = getFetchCalls()[callIndex]?.body;
  if (typeof body !== "string") {
    throw new Error(`Request ${String(callIndex)} has no JSON body.`);
  }
  const parsed: unknown = JSON.parse(body);
  if (typeof parsed !== "object" || parsed === null || Array.isArray(parsed)) {
    throw new Error(`Request ${String(callIndex)} has an invalid JSON body.`);
  }
  return parsed as Record<string, unknown>;
}

function planProcedureConfirmationLoad(loadedMacro: Macro = macro): void {
  planJsonResponse(success(loadedMacro));
  planJsonResponse(success(procedure));
  planJsonResponse(success(validation));
}

async function renderConfirmation(
  options: {
    currentSettings?: Settings;
    currentStatus?: DeviceStatus;
    onAccepted?: AcceptedHandler;
    target?: ExecutionConfirmationTarget;
  } = {},
): Promise<Awaited<ReturnType<typeof render>>> {
  const view = await render(
    <ConfirmExecutionPage
      activeSet={macroSet}
      onAccepted={options.onAccepted ?? (() => undefined)}
      settings={options.currentSettings ?? settings}
      status={options.currentStatus ?? deviceStatus}
      target={options.target ?? procedureTarget}
    />,
  );
  await flushReact();
  return view;
}

describe("execution confirmation", () => {
  test("loads a persisted macro and exact procedure context without executing", async () => {
    planProcedureConfirmationLoad();
    const view = await renderConfirmation();

    expect(document.body.textContent).toContain("Confirm send");
    expect(document.body.textContent).toContain("Open terminal");
    expect(document.body.textContent).toContain("Install Debian");
    expect(document.body.textContent).toContain("Ready to request execution.");
    expect(getFetchCalls()).toHaveLength(3);
    expect(
      getFetchCalls().some((call) => call.url === "/api/v1/executions"),
    ).toBe(false);
    await view.unmount();
  });

  test("disables Send with a visible USB explanation", async () => {
    planProcedureConfirmationLoad();
    const view = await renderConfirmation({
      currentStatus: { ...deviceStatus, usbState: "disconnected" },
    });

    expect(buttonWithText("Send now").disabled).toBe(true);
    expect(document.body.textContent).toContain(
      "USB is disconnected instead of ready.",
    );
    await view.unmount();
  });

  test("rechecks state, waits for device confirmation, and submits nested context", async () => {
    setCsrfToken("csrf-token");
    const onAccepted = vi.fn<AcceptedHandler>();
    planProcedureConfirmationLoad();
    const view = await renderConfirmation({ onAccepted });

    planJsonResponse(success(deviceStatus));
    planJsonResponse(success(settings));
    planJsonResponse(success(macro));
    planJsonResponse(success(procedure));
    planJsonResponse(success(validation));

    const submission = {
      resolve: null as ((response: Response) => void) | null,
    };
    planFetch(
      () =>
        new Promise<Response>((resolve) => {
          submission.resolve = resolve;
        }),
    );

    await click(buttonWithText("Send now"));
    await flushReact();
    await flushReact();

    expect(document.body.textContent).toContain(
      "Send the confirm command on the device serial console.",
    );
    const submitCallIndex = getFetchCalls().length - 1;
    expect(getFetchCalls()[submitCallIndex]?.url).toBe("/api/v1/executions");
    expect(getFetchCalls()[submitCallIndex]?.method).toBe("POST");
    expect(getFetchCalls()[submitCallIndex]?.headers.get("X-CSRF-Token")).toBe(
      "csrf-token",
    );
    expect(requestBody(submitCallIndex)).toEqual({
      setId: macroSet.id,
      macroId,
      macroRevision: macro.revision,
      sourceContext: {
        procedureId,
        stepId: macroStepId,
      },
    });

    const resolveSubmission = submission.resolve;
    if (resolveSubmission === null) {
      throw new Error("Execution submission was not started.");
    }
    await act(async () => {
      resolveSubmission(
        jsonResponse(
          success({
            executionId,
            actionCount: validation.actionCount,
            estimatedDurationMs: validation.estimatedDurationMs,
          }),
          202,
        ),
      );
      await Promise.resolve();
    });
    await flushReact();

    expect(onAccepted).toHaveBeenCalledWith(
      {
        executionId,
        actionCount: validation.actionCount,
        estimatedDurationMs: validation.estimatedDurationMs,
      },
      `/instruction?procedureId=${procedureId}&stepId=${macroStepId}`,
    );
    await view.unmount();
  });

  test("does not submit when the macro changes during preflight", async () => {
    setCsrfToken("csrf-token");
    planProcedureConfirmationLoad();
    const view = await renderConfirmation();

    const revisedMacro = { ...macro, revision: macro.revision + 1 };
    planJsonResponse(success(deviceStatus));
    planJsonResponse(success(settings));
    planJsonResponse(success(revisedMacro));
    planJsonResponse(success(procedure));
    planJsonResponse(success(validation));

    await click(buttonWithText("Send now"));
    await flushReact();
    await flushReact();

    expect(document.body.textContent).toContain("This preview is stale.");
    expect(document.body.textContent).toContain(
      "Review the latest preview before sending.",
    );
    expect(buttonWithText("Reload preview")).toBeDefined();
    expect(
      getFetchCalls().some((call) => call.url === "/api/v1/executions"),
    ).toBe(false);
    await view.unmount();
  });

  test("falls back to a persisted global macro", async () => {
    const globalMacro: Macro = {
      schema_version: macro.schema_version,
      id: macro.id,
      revision: macro.revision,
      scope: "global",
      name: macro.name,
      source: macro.source,
      favorite: macro.favorite,
      key_press_ms: macro.key_press_ms,
      inter_key_ms: macro.inter_key_ms,
    };

    planJsonResponse(
      {
        ok: false,
        error: { code: "not_found", message: "set macro not found" },
      },
      404,
    );
    planJsonResponse(success(globalMacro));
    planJsonResponse(success(procedure));
    planJsonResponse(success(validation));

    const view = await renderConfirmation();
    expect(document.body.textContent).toContain("Global macro");
    expect(getFetchCalls()[1]?.url).toBe(`/api/v1/global/macros/${macroId}`);
    expect(getFetchCalls()[3]?.url).toBe(
      `/api/v1/global/macros/${macroId}/validate`,
    );
    await view.unmount();
  });
});
