import { describe, expect, it } from "vitest";
import { limits } from "../src/types/limits";

describe("shared limits", () => {
  it("matches the firmware specification", () => {
    expect(limits.macroSourceBytes).toBe(4096);
    expect(limits.delayMs).toBe(10_000);
    expect(limits.importBytes).toBe(512 * 1024);
  });
});
