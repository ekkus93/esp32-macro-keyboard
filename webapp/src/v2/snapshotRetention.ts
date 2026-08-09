/**
 * Advisory snapshot-retention-target evaluation, per SPEC_V2 §10.8/§11.2 and
 * UI_UX_SPEC_V2 §9.5 (TODO_V2 V2-112).
 *
 * `snapshotRetentionTarget` is purely a UI cleanup hint — never a deletion
 * rule (SPEC_V2 §10.8 "advisory UI preference, not a deletion rule"; §10.9
 * confirms no automatic deletion exists anywhere in firmware or React). This
 * module computes only whether to show a non-blocking indicator; it never
 * deletes or selects blobs for deletion — that choice always belongs to the
 * user (UI_UX_SPEC_V2 §9.5 "The user chooses which snapshots, if any, to
 * delete").
 */

export interface SnapshotRetentionStatus {
  count: number;
  target: number;
  /**
   * True only when `target` is nonzero and `count` exceeds it. A `target` of
   * `0` disables the indicator entirely (SPEC_V2 §10.8 "A target of 0
   * disables the cleanup indicator").
   */
  overTarget: boolean;
}

export function evaluateSnapshotRetention(
  count: number,
  target: number,
): SnapshotRetentionStatus {
  return {
    count,
    target,
    overTarget: target > 0 && count > target,
  };
}
