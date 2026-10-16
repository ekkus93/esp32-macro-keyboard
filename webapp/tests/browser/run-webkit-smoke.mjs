import { webkit } from "playwright";

import { runMacroEditingWorkflows } from "./workflows/macroEditing.mjs";
import {
  runKeyboardNavigationWorkflow,
  runMacroEditorKeyboardViewportWorkflow,
  runStartupVisibleControlsWorkflow,
  runVisibleControlsWorkflow,
} from "./workflows/visibleControls.mjs";

const browser = await webkit.launch({ headless: true });

try {
  await runVisibleControlsWorkflow(browser);
  console.log("WebKit visible controls, touch targets, and axe scans passed.");
  await runKeyboardNavigationWorkflow(browser);
  await runMacroEditorKeyboardViewportWorkflow(browser);
  console.log("WebKit touch keyboard and editor viewport workflows passed.");
  await runStartupVisibleControlsWorkflow(browser);
  console.log("WebKit startup responsive-control workflow passed.");
  await runMacroEditingWorkflows(browser);
  console.log("WebKit macro editing and package workflows passed.");
} finally {
  await browser.close();
}
