import { describe, expect, test } from "vitest";
import { ErrorBanner } from "../src/components/ErrorBanner";
import { render } from "./render";

describe("ErrorBanner", () => {
  test("hides null and empty messages", async () => {
    const view = await render(<ErrorBanner message={null} />);
    expect(document.querySelector("[role='alert']")).toBeNull();
    await view.rerender(<ErrorBanner message="" />);
    expect(document.querySelector("[role='alert']")).toBeNull();
    await view.unmount();
  });

  test("renders text safely and removes it when cleared", async () => {
    const malicious = '<img src=x onerror="window.__executed = true">';
    const view = await render(<ErrorBanner message={malicious} />);
    const alert = document.querySelector<HTMLElement>("[role='alert']");
    expect(alert?.textContent).toBe(malicious);
    expect(alert?.querySelector("img")).toBeNull();

    await view.rerender(<ErrorBanner message={null} />);
    expect(document.querySelector("[role='alert']")).toBeNull();
    await view.unmount();
  });

  test("renders long untrusted text without creating markup", async () => {
    const message = `<script>throw new Error("executed")</script>${"x".repeat(4_096)}`;
    const view = await render(<ErrorBanner message={message} />);
    const alert = document.querySelector<HTMLElement>("[role='alert']");
    expect(alert?.textContent).toBe(message);
    expect(alert?.querySelector("script")).toBeNull();
    await view.unmount();
  });
});
