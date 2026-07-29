import { describe, expect, test, vi } from "vitest";
import { setCsrfToken } from "../src/api/client";
import { ProcedureLibraryPage } from "../src/features/procedures/ProcedureLibraryPage";
import { ProcedureWorkflowPage } from "../src/features/procedures/ProcedureWorkflowPage";
import {
  checkpointStepId,
  instructionStepId,
  macroId,
  macroSet,
  macroStepId,
  oldStepId,
  procedure,
  procedureId,
  procedureProgressSnapshot,
  procedureSummary,
} from "./appFixtures";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import { buttonWithText, click, flushReact, render } from "./render";

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

async function renderWorkflow(
  mode: "overview" | "step",
  stepId: string | null,
  progress: object,
): Promise<Awaited<ReturnType<typeof render>>> {
  planJsonResponse(success(procedure));
  planJsonResponse(success(progress));
  const view = await render(
    <ProcedureWorkflowPage
      activeSet={macroSet}
      mode={mode}
      target={{ kind: "valid", procedureId, stepId }}
    />,
  );
  await flushReact();
  return view;
}

describe("server-backed procedures", () => {
  test("loads procedure summaries and progress", async () => {
    const onOpen = vi.fn();
    planJsonResponse(success([procedureSummary]));
    planJsonResponse(success(procedureProgressSnapshot()));
    const view = await render(
      <ProcedureLibraryPage activeSet={macroSet} onOpen={onOpen} />,
    );
    await flushReact();

    expect(document.body.textContent).toContain("Install Debian");
    expect(document.body.textContent).toContain(
      "1 completed · 0 skipped · 2 remaining",
    );
    await click(buttonWithText("Open procedure"));
    expect(onOpen).toHaveBeenCalledWith(procedureId);
    expect(getFetchCalls()[0]?.url).toBe(
      `/api/v1/sets/${macroSet.id}/procedures`,
    );
    expect(getFetchCalls()[1]?.url).toBe(
      `/api/v1/sets/${macroSet.id}/procedures/${procedureId}/progress`,
    );
    await view.unmount();
  });

  test("represents missing progress explicitly as not started", async () => {
    planJsonResponse(success([procedureSummary]));
    planJsonResponse(
      {
        ok: false,
        error: {
          code: "not_found",
          message: "progress not available",
        },
      },
      404,
    );
    const view = await render(
      <ProcedureLibraryPage activeSet={macroSet} onOpen={() => undefined} />,
    );
    await flushReact();

    expect(document.body.textContent).toContain("Not started · 3 steps");
    expect(buttonWithText("Open procedure").disabled).toBe(false);
    await view.unmount();
  });

  test("completes the current instruction without executing the next macro", async () => {
    setCsrfToken("csrf-token");
    const view = await renderWorkflow(
      "step",
      instructionStepId,
      procedureProgressSnapshot({
        currentStepId: instructionStepId,
        completedStepIds: [],
      }),
    );

    planJsonResponse(
      success(
        procedureProgressSnapshot({
          currentStepId: macroStepId,
          completedStepIds: [instructionStepId],
        }),
      ),
    );
    await click(buttonWithText("Mark instruction complete"));
    await flushReact();

    const call = getFetchCalls()[2];
    expect(call?.method).toBe("POST");
    expect(call?.url).toBe(
      `/api/v1/sets/${macroSet.id}/procedures/${procedureId}/progress/complete`,
    );
    expect(requestBody(2)).toEqual({
      expectedProcedureRevision: procedure.revision,
      stepId: instructionStepId,
    });
    expect(document.body.textContent).toContain(
      "The next step is ready; no macro was sent automatically.",
    );
    expect(
      getFetchCalls().some(
        (candidate) => candidate.url === "/api/v1/executions",
      ),
    ).toBe(false);
    await view.unmount();
  });

  test("requires an explicit checkpoint confirmation", async () => {
    setCsrfToken("csrf-token");
    const view = await renderWorkflow(
      "step",
      checkpointStepId,
      procedureProgressSnapshot({
        currentStepId: checkpointStepId,
        completedStepIds: [instructionStepId, macroStepId],
      }),
    );

    await click(buttonWithText("Confirm checkpoint"));
    expect(getFetchCalls()).toHaveLength(2);
    expect(document.body.textContent).toContain(
      "Confirm that the expected checkpoint result is present",
    );

    planJsonResponse(
      success(
        procedureProgressSnapshot({
          currentStepId: checkpointStepId,
          completedStepIds: [instructionStepId, macroStepId, checkpointStepId],
        }),
      ),
    );
    await click(buttonWithText("Confirm completion"));
    await flushReact();

    expect(getFetchCalls()).toHaveLength(3);
    expect(requestBody(2)).toEqual({
      expectedProcedureRevision: procedure.revision,
      stepId: checkpointStepId,
    });
    expect(document.body.textContent).toContain("Finished");
    await view.unmount();
  });

  test("requires confirmed true before skipping the current step", async () => {
    setCsrfToken("csrf-token");
    const view = await renderWorkflow(
      "step",
      instructionStepId,
      procedureProgressSnapshot({
        currentStepId: instructionStepId,
        completedStepIds: [],
      }),
    );

    await click(buttonWithText("Skip step"));
    expect(getFetchCalls()).toHaveLength(2);

    planJsonResponse(
      success(
        procedureProgressSnapshot({
          currentStepId: macroStepId,
          completedStepIds: [],
          skippedStepIds: [instructionStepId],
        }),
      ),
    );
    await click(buttonWithText("Confirm skip"));
    await flushReact();

    expect(getFetchCalls()[2]?.url).toBe(
      `/api/v1/sets/${macroSet.id}/procedures/${procedureId}/progress/skip`,
    );
    expect(requestBody(2)).toEqual({
      expectedProcedureRevision: procedure.revision,
      stepId: instructionStepId,
      confirmed: true,
    });
    await view.unmount();
  });

  test("resets stale progress only after confirmation", async () => {
    setCsrfToken("csrf-token");
    const view = await renderWorkflow(
      "overview",
      null,
      procedureProgressSnapshot({
        status: "stale",
        procedureRevision: procedure.revision - 1,
        currentProcedureRevision: procedure.revision,
        currentStepId: oldStepId,
        completedStepIds: [oldStepId],
      }),
    );

    expect(document.body.textContent).toContain("Saved progress is stale.");
    expect(document.body.textContent).not.toContain(
      "Mark instruction complete",
    );
    await click(buttonWithText("Reset progress"));
    expect(getFetchCalls()).toHaveLength(2);

    planJsonResponse(
      success(
        procedureProgressSnapshot({
          currentStepId: instructionStepId,
          completedStepIds: [],
        }),
      ),
    );
    await click(buttonWithText("Confirm reset"));
    await flushReact();

    const call = getFetchCalls()[2];
    expect(call?.method).toBe("DELETE");
    expect(call?.url).toBe(
      `/api/v1/sets/${macroSet.id}/procedures/${procedureId}/progress`,
    );
    expect(requestBody(2)).toEqual({
      expectedRevision: procedure.revision,
    });
    expect(document.body.textContent).toContain(
      "Procedure progress reset to the first step.",
    );
    await view.unmount();
  });

  test("supports previous and next review without changing progress", async () => {
    setHashSilently(
      `/instruction?procedureId=${procedureId}&stepId=${macroStepId}`,
    );
    const view = await renderWorkflow(
      "step",
      macroStepId,
      procedureProgressSnapshot(),
    );

    await click(buttonWithText("Previous step"));
    expect(window.location.hash).toBe(
      `#/instruction?procedureId=${procedureId}&stepId=${instructionStepId}`,
    );
    await click(buttonWithText("Next step"));
    expect(window.location.hash).toBe(
      `#/instruction?procedureId=${procedureId}&stepId=${checkpointStepId}`,
    );
    expect(getFetchCalls()).toHaveLength(2);
    await view.unmount();
  });

  test("navigates macro send and resend to confirmation without submitting", async () => {
    setHashSilently(
      `/instruction?procedureId=${procedureId}&stepId=${macroStepId}`,
    );
    const currentView = await renderWorkflow(
      "step",
      macroStepId,
      procedureProgressSnapshot(),
    );
    await click(buttonWithText("Send macro"));
    expect(window.location.hash).toBe(
      `#/confirm?procedureId=${procedureId}&stepId=${macroStepId}&macroId=${macroId}`,
    );
    expect(getFetchCalls()).toHaveLength(2);
    await currentView.unmount();

    planFetch(() => jsonResponse(success(procedure)));
    planFetch(() =>
      jsonResponse(
        success(
          procedureProgressSnapshot({
            currentStepId: checkpointStepId,
            completedStepIds: [instructionStepId, macroStepId],
          }),
        ),
      ),
    );
    const completedView = await render(
      <ProcedureWorkflowPage
        activeSet={macroSet}
        mode="step"
        target={{
          kind: "valid",
          procedureId,
          stepId: macroStepId,
        }}
      />,
    );
    await flushReact();
    await click(buttonWithText("Resend macro"));
    expect(getFetchCalls()).toHaveLength(4);
    expect(
      getFetchCalls().some(
        (candidate) => candidate.url === "/api/v1/executions",
      ),
    ).toBe(false);
    await completedView.unmount();
  });
});
