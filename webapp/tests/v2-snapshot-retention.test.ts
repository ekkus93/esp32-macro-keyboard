import { describe, expect, test } from "vitest";
import { evaluateSnapshotRetention } from "../src/v2/snapshotRetention";

describe("evaluateSnapshotRetention — V2-112", () => {
  test("does not flag over-target when count is at or below target", () => {
    expect(evaluateSnapshotRetention(5, 5)).toEqual({
      count: 5,
      target: 5,
      overTarget: false,
    });
    expect(evaluateSnapshotRetention(3, 5)).toEqual({
      count: 3,
      target: 5,
      overTarget: false,
    });
  });

  test("flags over-target once count exceeds target", () => {
    expect(evaluateSnapshotRetention(6, 5)).toEqual({
      count: 6,
      target: 5,
      overTarget: true,
    });
  });

  test("a target of 0 disables the indicator regardless of count", () => {
    expect(evaluateSnapshotRetention(100, 0)).toEqual({
      count: 100,
      target: 0,
      overTarget: false,
    });
  });

  test("defaults to 5 in device settings semantics but is evaluated generically here", () => {
    // This module does not itself know the device default (SPEC_V2 §11.1) —
    // the caller reads `settings.snapshotRetentionTarget` and passes it in.
    // A sixth snapshot at the default target is exactly the boundary
    // UI_UX_SPEC_V2 §9.5 calls out: "Saving a sixth snapshot succeeds... it
    // does not automatically delete the oldest."
    expect(evaluateSnapshotRetention(6, 5).overTarget).toBe(true);
  });
});
