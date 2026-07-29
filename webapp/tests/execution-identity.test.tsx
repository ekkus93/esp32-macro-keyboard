import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { ExecutionPage } from "../src/features/execution/ExecutionPage";
import { executionId, executionStatus } from "./appFixtures";
import { planJsonResponse } from "./fakeFetch";
import { buttonWithText, flushReact, render } from "./render";

function success(data: unknown): object {
  return { ok: true, data };
}

describe("accepted execution identity", () => {
  test("does not display or cancel a different current execution", async () => {
    const onTerminal = vi.fn();
    const differentExecution = {
      ...executionStatus("running"),
      executionId: "99999999-9999-4999-8999-999999999999",
    };
    planJsonResponse(success(differentExecution));

    const view = await render(
      <ExecutionPage
        expectedExecutionId={executionId}
        onTerminal={onTerminal}
      />,
    );
    await flushReact();

    expect(document.body.textContent).toContain(
      "The device reported a different current execution.",
    );
    expect(buttonWithText("Cancel and release keys").disabled).toBe(true);
    expect(onTerminal).not.toHaveBeenCalled();
    await view.unmount();
  });

  test("accepts the matching identity on a later poll", async () => {
    vi.useFakeTimers();
    const onTerminal = vi.fn();
    planJsonResponse(
      success({
        ...executionStatus("running"),
        executionId: "99999999-9999-4999-8999-999999999999",
      }),
    );
    const view = await render(
      <ExecutionPage
        expectedExecutionId={executionId}
        onTerminal={onTerminal}
      />,
    );
    await flushReact();

    planJsonResponse(success(executionStatus("running")));
    await act(async () => {
      await vi.advanceTimersByTimeAsync(500);
      await Promise.resolve();
    });

    expect(document.body.textContent).toContain("2 / 5");
    expect(document.body.textContent).not.toContain(
      "The device reported a different current execution.",
    );
    expect(buttonWithText("Cancel and release keys").disabled).toBe(false);
    await view.unmount();
  });
});
