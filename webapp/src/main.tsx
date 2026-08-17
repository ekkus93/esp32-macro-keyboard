import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import AppV2 from "./AppV2";
import { ExecutionRecoveryOverlay } from "./features/shell/v2/ExecutionRecoveryOverlay";
import "./styles.css";

const root = document.getElementById("root");
if (root === null) {
  throw new Error("Missing #root element");
}

createRoot(root).render(
  <StrictMode>
    <AppV2 />
    <ExecutionRecoveryOverlay />
  </StrictMode>,
);
