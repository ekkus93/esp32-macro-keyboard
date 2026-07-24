import { act, type ReactNode } from "react";
import { createRoot, type Root } from "react-dom/client";

export interface RenderResult {
  container: HTMLDivElement;
  rerender: (element: ReactNode) => Promise<void>;
  unmount: () => Promise<void>;
}

type ElementConstructor<T extends Element> = abstract new (...arguments_: never[]) => T;

export async function render(element: ReactNode): Promise<RenderResult> {
  const container = document.createElement("div");
  document.body.append(container);
  const root: Root = createRoot(container);

  const rerender = async (nextElement: ReactNode): Promise<void> => {
    await act(async () => {
      root.render(nextElement);
      await Promise.resolve();
    });
  };

  await rerender(element);
  return {
    container,
    rerender,
    unmount: async (): Promise<void> => {
      await act(async () => {
        root.unmount();
        await Promise.resolve();
      });
      container.remove();
    },
  };
}

export async function flushReact(): Promise<void> {
  await act(async () => {
    await Promise.resolve();
    await Promise.resolve();
  });
}

function setNativeValue(element: HTMLInputElement | HTMLTextAreaElement, value: string): void {
  const prototype =
    element instanceof HTMLInputElement
      ? HTMLInputElement.prototype
      : HTMLTextAreaElement.prototype;
  const descriptor = Object.getOwnPropertyDescriptor(prototype, "value");
  if (descriptor?.set === undefined) {
    throw new Error("Missing native value setter.");
  }
  descriptor.set.call(element, value);
}

export async function setInputValue(
  element: HTMLInputElement | HTMLTextAreaElement,
  value: string,
): Promise<void> {
  await act(async () => {
    setNativeValue(element, value);
    element.dispatchEvent(new Event("input", { bubbles: true }));
    element.dispatchEvent(new Event("change", { bubbles: true }));
    await Promise.resolve();
  });
}

export async function click(element: HTMLElement): Promise<void> {
  await act(async () => {
    element.dispatchEvent(new MouseEvent("click", { bubbles: true, cancelable: true }));
    await Promise.resolve();
  });
}

export async function submit(form: HTMLFormElement): Promise<void> {
  await act(async () => {
    form.dispatchEvent(new SubmitEvent("submit", { bubbles: true, cancelable: true }));
    await Promise.resolve();
  });
}

export function requiredElement<T extends Element>(
  selector: string,
  expectedType: ElementConstructor<T>,
): T {
  const element = document.querySelector(selector);
  if (element === null) {
    throw new Error(`Missing element: ${selector}`);
  }
  if (!(element instanceof expectedType)) {
    throw new Error(`Element ${selector} has an unexpected type.`);
  }
  return element;
}

export function buttonWithText(text: string): HTMLButtonElement {
  const button = Array.from(document.querySelectorAll("button")).find(
    (candidate) => candidate.textContent?.trim() === text,
  );
  if (button === undefined) {
    throw new Error(`Missing button: ${text}`);
  }
  return button;
}
