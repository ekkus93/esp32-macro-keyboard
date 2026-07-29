import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { setCsrfToken } from "../src/api/client";
import { MacroEditorPage } from "../src/features/macros/MacroEditorPage";
import { MacroLibraryPage } from "../src/features/macros/MacroLibraryPage";
import { macro, macroId, macroSet } from "./appFixtures";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
  submit,
} from "./render";

const validResult = {
  valid: true,
  actionCount: 3,
  estimatedDurationMs: 69,
} as const;

async function advanceValidation(): Promise<void> {
  await act(async () => {
    await vi.advanceTimersByTimeAsync(300);
  });
  await flushReact();
}

describe("server-backed macros", () => {
  test("loads the active-set macro library", async () => {
    const onEdit = vi.fn();
    const onSend = vi.fn();
    planJsonResponse({ ok: true, data: [macro] });
    const view = await render(
      <MacroLibraryPage
        activeSet={macroSet}
        onCreate={() => undefined}
        onEdit={onEdit}
        onSend={onSend}
      />,
    );
    await flushReact();

    expect(document.body.textContent).toContain("Open terminal");
    expect(document.body.textContent).toContain("12 source bytes");
    await click(buttonWithText("Send"));
    expect(onSend).toHaveBeenCalledWith(macroId);
    await click(buttonWithText("Edit"));
    expect(onEdit).toHaveBeenCalledWith(macroId);
    expect(getFetchCalls()[0]?.url).toBe(`/api/v1/sets/${macroSet.id}/macros`);
    await view.unmount();
  });

  test("loads, validates, and updates an existing macro", async () => {
    vi.useFakeTimers();
    setCsrfToken("csrf-token");
    planJsonResponse({ ok: true, data: macro });
    planJsonResponse({ ok: true, data: validResult });
    const view = await render(
      <MacroEditorPage
        activeSet={macroSet}
        onBack={() => undefined}
        target={{ kind: "edit", macroId }}
      />,
    );
    await flushReact();
    await advanceValidation();

    expect(document.body.textContent).toContain("3 actions · 69 ms estimated");
    expect(buttonWithText("Save macro").disabled).toBe(false);

    planJsonResponse({ ok: true, data: validResult });
    await setInputValue(
      requiredElement("#macro-name", HTMLInputElement),
      "Open admin terminal",
    );
    await advanceValidation();

    planFetch((call) => {
      const body = JSON.parse(
        typeof call.body === "string" ? call.body : "",
      ) as {
        expectedRevision: number;
        resource: typeof macro;
      };
      expect(call.method).toBe("PUT");
      expect(call.url).toBe(`/api/v1/sets/${macroSet.id}/macros/${macroId}`);
      expect(body.expectedRevision).toBe(macro.revision);
      expect(body.resource.name).toBe("Open admin terminal");
      return jsonResponse({
        ok: true,
        data: {
          ...body.resource,
          revision: macro.revision + 1,
        },
      });
    });
    await submit(requiredElement("#macro-editor-form", HTMLFormElement));
    await flushReact();

    expect(document.body.textContent).toContain("Saved revision 8.");
    expect(document.body.textContent).toContain("Revision 8");
    await view.unmount();
  });

  test("shows exact parser coordinates and keeps Save disabled", async () => {
    vi.useFakeTimers();
    planJsonResponse({ ok: true, data: macro });
    planJsonResponse({ ok: true, data: validResult });
    const view = await render(
      <MacroEditorPage
        activeSet={macroSet}
        onBack={() => undefined}
        target={{ kind: "edit", macroId }}
      />,
    );
    await flushReact();
    await advanceValidation();

    planJsonResponse(
      {
        ok: false,
        error: {
          code: "macro_syntax",
          message: "macro validation failed",
          details: { line: 3, column: 4, byteOffset: 7 },
        },
      },
      422,
    );
    await setInputValue(
      requiredElement("#macro-source", HTMLTextAreaElement),
      "abc\ndef{UNKNOWN}",
    );
    await advanceValidation();

    expect(document.body.textContent).toContain("Line 3, column 4, byte 7.");
    expect(buttonWithText("Save macro").disabled).toBe(true);
    await click(buttonWithText("Go to error"));
    expect(
      requiredElement("#macro-source", HTMLTextAreaElement).selectionStart,
    ).toBe(7);
    await view.unmount();
  });

  test("inserts directives, validates, and creates a macro", async () => {
    vi.useFakeTimers();
    setCsrfToken("csrf-token");
    const view = await render(
      <MacroEditorPage
        activeSet={macroSet}
        onBack={() => undefined}
        target={{ kind: "create" }}
      />,
    );
    await flushReact();

    await setInputValue(
      requiredElement("#macro-name", HTMLInputElement),
      "Focus address bar",
    );
    await click(buttonWithText("Insert Ctrl+L"));
    expect(requiredElement("#macro-source", HTMLTextAreaElement).value).toBe(
      "{CTRL+L}",
    );
    expect(document.body.textContent).toContain("8 / 4096 UTF-8 bytes");

    planJsonResponse({ ok: true, data: validResult });
    await advanceValidation();
    expect(buttonWithText("Create macro").disabled).toBe(false);

    planFetch((call) => {
      const body = JSON.parse(
        typeof call.body === "string" ? call.body : "",
      ) as typeof macro;
      expect(call.method).toBe("POST");
      expect(call.url).toBe(`/api/v1/sets/${macroSet.id}/macros`);
      expect(body.revision).toBe(1);
      expect(body.set_id).toBe(macroSet.id);
      expect(body.source).toBe("{CTRL+L}");
      return jsonResponse({ ok: true, data: body }, 201);
    });
    await submit(requiredElement("#macro-editor-form", HTMLFormElement));
    await flushReact();

    expect(document.body.textContent).toContain("Created revision 1.");
    expect(window.location.hash).toMatch(
      /^#\/macro-editor\?macroId=[0-9a-f-]+$/i,
    );
    await view.unmount();
  });

  test("surfaces stale-revision conflicts without overwriting the draft", async () => {
    vi.useFakeTimers();
    setCsrfToken("csrf-token");
    planJsonResponse({ ok: true, data: macro });
    planJsonResponse({ ok: true, data: validResult });
    const view = await render(
      <MacroEditorPage
        activeSet={macroSet}
        onBack={() => undefined}
        target={{ kind: "edit", macroId }}
      />,
    );
    await flushReact();
    await advanceValidation();

    planJsonResponse({ ok: true, data: validResult });
    await setInputValue(
      requiredElement("#macro-source", HTMLTextAreaElement),
      "{CTRL+SHIFT+T}",
    );
    await advanceValidation();

    planJsonResponse(
      {
        ok: false,
        error: {
          code: "conflict",
          message: "could not update macro",
        },
      },
      409,
    );
    await submit(requiredElement("#macro-editor-form", HTMLFormElement));
    await flushReact();

    expect(document.body.textContent).toContain("Stale revision.");
    expect(requiredElement("#macro-source", HTMLTextAreaElement).value).toBe(
      "{CTRL+SHIFT+T}",
    );
    expect(buttonWithText("Save macro").disabled).toBe(true);

    planJsonResponse({
      ok: true,
      data: {
        ...macro,
        revision: 8,
        source: "{GUI+R}",
      },
    });
    await click(buttonWithText("Reload latest"));
    await flushReact();
    expect(requiredElement("#macro-source", HTMLTextAreaElement).value).toBe(
      "{GUI+R}",
    );
    expect(document.body.textContent).not.toContain("Stale revision.");
    await view.unmount();
  });
});
