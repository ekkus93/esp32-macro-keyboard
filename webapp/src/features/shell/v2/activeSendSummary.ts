/**
 * Minimal summary of an in-progress send, reported upward by the Macros
 * page — the only screen that tracks send lifecycle; `MacroPreviewPage`
 * explicitly hands tracking off to it rather than showing its own progress
 * — so the landscape-phone orientation surface (TODO_V2 V2-132,
 * UI_UX_SPEC_V2 §12.3) can keep the macro name, current send state, and
 * Cancel and release all keys accessible while ordinary screen content is
 * replaced by the rotate-your-phone surface.
 */
export interface ActiveSendSummary {
  macroName: string | null;
  statusText: string;
  /** `null` while there is nothing yet to cancel (e.g. the request is still
   * being sent to the device). */
  onCancel: (() => void) | null;
}
