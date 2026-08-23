import { describe, expect, test, vi } from "vitest";
import { MacroEditorPage } from "../src/features/macros/v2/MacroEditorPage";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import {
  buttonWithText,
  click,
  render,
  requiredElement,
  setInputValue,
} from "./render";

async function openAdvanced(): Promise<void> {
  await click(buttonWithText("Advanced"));
}

const packageId = "550e8400-e29b-41d4-a716-446655440000";
const macroId = "6ba7b810-9dad-41d1-80b4-00c04fd430c8";
const uuidPattern =
  /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;

function repository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [
      {
        id: packageId,
        name: "Build server login",
        macros: [
          {
            id: macroId,
            name: "Start the build",
            source: "make -j8{ENTER}",
            keyPressMs: 8,
            interKeyMs: 15,
          },
        ],
      },
    ],
  };
}

function sourceTextarea(): HTMLTextAreaElement {
  return requiredElement("#macro-editor-source", HTMLTextAreaElement);
}

function nameInput(): HTMLInputElement {
  return requiredElement("#macro-editor-name", HTMLInputElement);
}

describe("MacroEditorPage — V2-100 fields and byte counts", () => {
  test("shows name, source, key-press, and inter-key fields pre-filled for an edit target", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await render(
      <MacroEditorPage
        macroId={macroId}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    expect(nameInput().value).toBe("Start the build");
    expect(sourceTextarea().value).toBe("make -j8{ENTER}");
    await openAdvanced();
    expect(
      requiredElement("#macro-editor-key-press", HTMLInputElement).value,
    ).toBe("8");
    expect(
      requiredElement("#macro-editor-inter-key", HTMLInputElement).value,
    ).toBe("15");
    await unmount();
  });

  test("shows live UTF-8 byte counts for name and source", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { container, unmount } = await render(
      <MacroEditorPage
        macroId={macroId}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    expect(container.textContent).toContain("15 / 64 UTF-8 bytes");
    expect(container.textContent).toContain("15 / 4096 UTF-8 bytes");
    await unmount();
  });

  test("a blank name for a create target is invalid and blocks Save", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await render(
      <MacroEditorPage
        macroId={null}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    expect(buttonWithText("Save changes").disabled).toBe(true);
    await unmount();
  });
});

describe("MacroEditorPage — V2-100 live validation and error location", () => {
  test("shows action count and estimated duration when the source is valid", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { container, unmount } = await render(
      <MacroEditorPage
        macroId={macroId}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    expect(container.textContent).toContain("Macro is valid");
    // "make -j8" is 8 literal characters + 1 {ENTER} action = 9 actions.
    expect(container.textContent).toContain("9 actions");
    expect(container.textContent).toContain("ms estimated");
    await unmount();
  });

  test("shows the exact error location for invalid source and disables Save", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { container, unmount } = await render(
      <MacroEditorPage
        macroId={macroId}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    await setInputValue(sourceTextarea(), "ok{BADDIRECTIVE}");
    expect(container.textContent).toContain("Line 1, column 3, byte 2.");
    expect(buttonWithText("Save changes").disabled).toBe(true);
    await unmount();
  });

  test("Go to error moves the caret to the exact byte offset", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await render(
      <MacroEditorPage
        macroId={macroId}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    await setInputValue(sourceTextarea(), "ok{BADDIRECTIVE}");
    await click(buttonWithText("Go to error"));
    const textarea = sourceTextarea();
    expect(textarea.selectionStart).toBe(2);
    expect(textarea.selectionEnd).toBe(2);
    await unmount();
  });
});

describe("MacroEditorPage — V2-100 directive insertion controls", () => {
  test("inserts a named-key directive at the cursor", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await render(
      <MacroEditorPage
        macroId={null}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    const textarea = sourceTextarea();
    await setInputValue(textarea, "ab");
    textarea.setSelectionRange(2, 2);
    await click(buttonWithText("ENTER"));
    expect(textarea.value).toBe("ab{ENTER}");
    await unmount();
  });

  test("inserts a delay directive with the entered milliseconds", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await render(
      <MacroEditorPage
        macroId={null}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    await openAdvanced();
    await setInputValue(
      requiredElement("#macro-editor-delay-ms", HTMLInputElement),
      "250",
    );
    await click(buttonWithText("Insert delay"));
    expect(sourceTextarea().value).toBe("{DELAY:250}");
    await unmount();
  });

  test("builds a chord directive from modifiers and a key", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await render(
      <MacroEditorPage
        macroId={null}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    await openAdvanced();
    const ctrlToggle = buttonWithText("CTRL");
    const shiftToggle = buttonWithText("SHIFT");
    expect(ctrlToggle.getAttribute("aria-pressed")).toBe("false");
    await click(ctrlToggle);
    await click(shiftToggle);
    expect(ctrlToggle.getAttribute("aria-pressed")).toBe("true");
    expect(shiftToggle.getAttribute("aria-pressed")).toBe("true");
    await setInputValue(
      requiredElement("#macro-editor-chord-key", HTMLInputElement),
      "t",
    );
    await click(buttonWithText("Insert chord"));
    expect(sourceTextarea().value).toBe("{CTRL+SHIFT+T}");
    await unmount();
  });

  test("inserts literal brace escapes", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await render(
      <MacroEditorPage
        macroId={null}
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    await click(buttonWithText("Literal {"));
    await click(buttonWithText("Literal }"));
    expect(sourceTextarea().value).toBe("{{}}");
    await unmount();
  });
});

describe("MacroEditorPage — V2-100/V2-101 save and cancel", () => {
  test("Save on a create target adds a new macro with a fresh UUID and dirties the working copy", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const onSaved = vi.fn();
    const { unmount } = await render(
      <MacroEditorPage
        macroId={null}
        onBack={vi.fn()}
        onSaved={onSaved}
        packageId={packageId}
        store={store}
      />,
    );
    await setInputValue(nameInput(), "New macro");
    await setInputValue(sourceTextarea(), "hi");
    await click(buttonWithText("Save changes"));

    expect(onSaved).toHaveBeenCalledOnce();
    expect(store.getIsDirty()).toBe(true);
    const pkg = store
      .getRepository()
      .packages.find((candidate) => candidate.id === packageId);
    expect(pkg?.macros).toHaveLength(2);
    const added = pkg?.macros[1];
    expect(added?.name).toBe("New macro");
    expect(added?.source).toBe("hi");
    expect(added?.id).toMatch(uuidPattern);
    expect(added?.id).not.toBe(macroId);
    await unmount();
  });

  test("Save on an edit target with real changes updates the macro in place and dirties the working copy", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const onSaved = vi.fn();
    const { unmount } = await render(
      <MacroEditorPage
        macroId={macroId}
        onBack={vi.fn()}
        onSaved={onSaved}
        packageId={packageId}
        store={store}
      />,
    );
    await setInputValue(nameInput(), "Renamed build");
    await click(buttonWithText("Save changes"));

    expect(onSaved).toHaveBeenCalledOnce();
    expect(store.getIsDirty()).toBe(true);
    const pkg = store
      .getRepository()
      .packages.find((candidate) => candidate.id === packageId);
    expect(pkg?.macros).toHaveLength(1);
    expect(pkg?.macros[0]?.name).toBe("Renamed build");
    expect(pkg?.macros[0]?.id).toBe(macroId);
    await unmount();
  });

  test("Save with no actual change does not dirty the working copy (avoids a no-op dirty transition)", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const onSaved = vi.fn();
    const { unmount } = await render(
      <MacroEditorPage
        macroId={macroId}
        onBack={vi.fn()}
        onSaved={onSaved}
        packageId={packageId}
        store={store}
      />,
    );
    await click(buttonWithText("Save changes"));
    expect(onSaved).toHaveBeenCalledOnce();
    expect(store.getIsDirty()).toBe(false);
    await unmount();
  });

  test("Cancel calls onBack without changing the working copy", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const onBack = vi.fn();
    const { unmount } = await render(
      <MacroEditorPage
        macroId={macroId}
        onBack={onBack}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    await setInputValue(nameInput(), "Edited but not saved");
    await setInputValue(sourceTextarea(), "z");
    await click(buttonWithText("Cancel"));

    expect(onBack).toHaveBeenCalledOnce();
    expect(store.getIsDirty()).toBe(false);
    const pkg = store
      .getRepository()
      .packages.find((candidate) => candidate.id === packageId);
    expect(pkg?.macros[0]?.name).toBe("Start the build");
    expect(pkg?.macros[0]?.source).toBe("make -j8{ENTER}");
    await unmount();
  });

  test("editing a macro that no longer exists shows an error and offers a way back", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { container, unmount } = await render(
      <MacroEditorPage
        macroId="00000000-0000-4000-8000-000000000000"
        onBack={vi.fn()}
        onSaved={vi.fn()}
        packageId={packageId}
        store={store}
      />,
    );
    expect(container.textContent).toContain(
      "This macro is no longer in the repository.",
    );
    await unmount();
  });
});
