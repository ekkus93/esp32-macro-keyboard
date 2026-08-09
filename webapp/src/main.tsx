import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import AppV2 from "./AppV2";
import "./styles.css";
import "./management.css";

const root = document.getElementById("root");
if (root === null) {
  throw new Error("Missing #root element");
}

createRoot(root).render(
  <StrictMode>
    <AppV2 />
  </StrictMode>,
);
